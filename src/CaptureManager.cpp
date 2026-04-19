#include "CaptureManager.hpp"
#include <opencv2/core/utils/logger.hpp>
#include <mfapi.h>
#include <mfidl.h>
#include <mfobjects.h>
#include <windows.h>
#include <iostream>
#include <cmath>

namespace {
std::string wideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return {};

    const int requiredSize = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (requiredSize <= 1) return {};

    std::string utf8(requiredSize - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, utf8.data(), requiredSize, nullptr, nullptr);
    return utf8;
}

std::vector<DeviceInfo> enumerateMfVideoDevices() {
    std::vector<DeviceInfo> devices;
    IMFAttributes* attributes = nullptr;
    IMFActivate** activates = nullptr;
    UINT32 count = 0;

    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        return devices;
    }

    hr = MFCreateAttributes(&attributes, 1);
    if (SUCCEEDED(hr)) {
        hr = attributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
    }
    if (SUCCEEDED(hr)) {
        hr = MFEnumDeviceSources(attributes, &activates, &count);
    }

    if (SUCCEEDED(hr)) {
        for (UINT32 i = 0; i < count; ++i) {
            WCHAR* friendlyName = nullptr;
            UINT32 friendlyNameLength = 0;
            if (SUCCEEDED(activates[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &friendlyName, &friendlyNameLength))) {
                DeviceInfo info;
                info.hwIndex = static_cast<int>(i);
                info.name = wideToUtf8(friendlyName);
                if (info.name.empty()) {
                    info.name = "Dispositivo de Video [" + std::to_string(i) + "]";
                }
                devices.push_back(info);
                CoTaskMemFree(friendlyName);
            }
        }
    }

    if (activates) {
        for (UINT32 i = 0; i < count; ++i) {
            if (activates[i]) activates[i]->Release();
        }
        CoTaskMemFree(activates);
    }
    if (attributes) {
        attributes->Release();
    }
    MFShutdown();
    return devices;
}

int fourccForFormat(CapturePixelFormat format) {
    switch (format) {
    case CapturePixelFormat::YUY2:
        return cv::VideoWriter::fourcc('Y', 'U', 'Y', '2');
    case CapturePixelFormat::H264:
        return cv::VideoWriter::fourcc('H', '2', '6', '4');
    case CapturePixelFormat::MJPEG:
    default:
        return cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
    }
}
}

CaptureManager::CaptureManager() = default;

CaptureManager::~CaptureManager() {
    release();
}

const char* CaptureManager::pixelFormatToString(CapturePixelFormat format) {
    switch (format) {
    case CapturePixelFormat::MJPEG: return "MJPEG";
    case CapturePixelFormat::YUY2: return "YUY2";
    case CapturePixelFormat::H264: return "H264";
    default: return "UNKNOWN";
    }
}

CapturePixelFormat CaptureManager::decodeFourcc(double fourccValue) {
    const int code = static_cast<int>(fourccValue);
    const char a = static_cast<char>(code & 0xFF);
    const char b = static_cast<char>((code >> 8) & 0xFF);
    const char c = static_cast<char>((code >> 16) & 0xFF);
    const char d = static_cast<char>((code >> 24) & 0xFF);

    if (a == 'M' && b == 'J' && c == 'P' && d == 'G') return CapturePixelFormat::MJPEG;
    if (a == 'Y' && b == 'U' && c == 'Y' && d == '2') return CapturePixelFormat::YUY2;
    if (a == 'H' && b == '2' && c == '6' && d == '4') return CapturePixelFormat::H264;
    return CapturePixelFormat::UNKNOWN;
}

bool CaptureManager::isRequestedModeApplied(const CaptureSessionInfo& info) {
    const bool resolutionMatches = info.effectiveWidth == info.requestedWidth && info.effectiveHeight == info.requestedHeight;
    const bool fpsMatches = std::abs(info.effectiveFps - info.requestedFps) <= 1;
    const bool formatMatches = info.effectiveFormat == info.requestedFormat;
    return resolutionMatches && fpsMatches && formatMatches;
}

void CaptureManager::finalizeSessionInfo(cv::VideoCapture& capture, const cv::Mat& frame, CaptureSessionInfo& info, bool allowRequestedFormatFallback) {
    info.effectiveWidth = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH));
    info.effectiveHeight = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT));
    info.effectiveFps = static_cast<int>(std::lround(capture.get(cv::CAP_PROP_FPS)));
    info.effectiveFormat = decodeFourcc(capture.get(cv::CAP_PROP_FOURCC));

    if (info.effectiveWidth <= 0 && !frame.empty()) info.effectiveWidth = frame.cols;
    if (info.effectiveHeight <= 0 && !frame.empty()) info.effectiveHeight = frame.rows;
    if (info.effectiveFps <= 0) info.effectiveFps = info.requestedFps;

    info.requestedResolutionApplied = (info.effectiveWidth == info.requestedWidth && info.effectiveHeight == info.requestedHeight);
    info.requestedFpsApplied = std::abs(info.effectiveFps - info.requestedFps) <= 1;

    if (info.effectiveFormat == CapturePixelFormat::UNKNOWN && allowRequestedFormatFallback && !frame.empty()) {
        info.effectiveFormat = info.requestedFormat;
    }

    info.requestedFormatApplied = (info.effectiveFormat == info.requestedFormat);
}

bool CaptureManager::probeMode(int deviceIndex, int width, int height, int fps, CapturePixelFormat format, CaptureSessionInfo& outInfo) {
    outInfo = {};
    outInfo.requestedWidth = width;
    outInfo.requestedHeight = height;
    outInfo.requestedFps = fps;
    outInfo.requestedFormat = format;

    cv::VideoCapture probe;
    if (!probe.open(deviceIndex, cv::CAP_MSMF)) {
        return false;
    }

    probe.set(cv::CAP_PROP_FOURCC, fourccForFormat(format));
    probe.set(cv::CAP_PROP_FRAME_WIDTH, width);
    probe.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    probe.set(cv::CAP_PROP_FPS, fps);
    probe.set(cv::CAP_PROP_BUFFERSIZE, 1);

    cv::Mat frame;
    if (probe.grab()) {
        probe.retrieve(frame);
    }

    finalizeSessionInfo(probe, frame, outInfo, false);

    probe.release();
    return true;
}

bool CaptureManager::initialize(int deviceIndex, int width, int height, int fps, bool useMjpeg, CaptureBackendType backend) {
    release();

    CapturePixelFormat requestedFormat = useMjpeg ? CapturePixelFormat::MJPEG : CapturePixelFormat::YUY2;
    sessionInfo = {};
    sessionInfo.requestedWidth = width;
    sessionInfo.requestedHeight = height;
    sessionInfo.requestedFps = fps;
    sessionInfo.requestedFormat = requestedFormat;

    const bool ignoredBackend = backend == CaptureBackendType::MEDIA_FOUNDATION || backend == CaptureBackendType::AUTO;
    if (ignoredBackend) {
        std::cout << "[Capture] Se fuerza OpenCV/MSMF para mantener estabilidad y evitar la ruta MF Source Reader." << std::endl;
    }

    cap.open(deviceIndex, cv::CAP_MSMF);
    if (!cap.isOpened()) {
        std::cerr << "Error: No se pudo abrir la capturadora USB en el indice " << deviceIndex << "." << std::endl;
        return false;
    }

    cap.set(cv::CAP_PROP_FOURCC, fourccForFormat(requestedFormat));
    cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    cap.set(cv::CAP_PROP_FPS, fps);
    cap.set(cv::CAP_PROP_BUFFERSIZE, 1);

    cv::Mat warmupFrame;
    if (cap.grab()) {
        cap.retrieve(warmupFrame);
    }

    finalizeSessionInfo(cap, warmupFrame, sessionInfo, true);

    std::cout << "[Capture] OpenCV/MSMF activo: "
              << sessionInfo.effectiveWidth << "x" << sessionInfo.effectiveHeight
              << " @ " << sessionInfo.effectiveFps << " FPS ("
              << pixelFormatToString(sessionInfo.effectiveFormat) << ")" << std::endl;

    if (!isRequestedModeApplied(sessionInfo)) {
        std::cout << "[Capture] Aviso: el modo real no coincide con lo pedido. Solicitado "
                  << width << "x" << height << " @ " << fps << " FPS ("
                  << pixelFormatToString(requestedFormat) << "), negociado "
                  << sessionInfo.effectiveWidth << "x" << sessionInfo.effectiveHeight
                  << " @ " << sessionInfo.effectiveFps << " FPS ("
                  << pixelFormatToString(sessionInfo.effectiveFormat) << ")." << std::endl;
    }

    return true;
}

bool CaptureManager::getNextFrame(cv::Mat& outFrame) {
    if (!cap.isOpened()) return false;
    if (!cap.grab()) return false;
    return cap.retrieve(outFrame);
}

void CaptureManager::release() {
    if (cap.isOpened()) {
        cap.release();
    }
    sessionInfo = {};
}

std::vector<DeviceInfo> CaptureManager::getAvailableDevices() {
    std::cout << "Escaneando dispositivos de video..." << std::endl;
    std::vector<DeviceInfo> devices;
    std::vector<DeviceInfo> enumeratedDevices = enumerateMfVideoDevices();

    const auto previousLogLevel = cv::utils::logging::getLogLevel();
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_ERROR);

    if (!enumeratedDevices.empty()) {
        for (const DeviceInfo& enumeratedDevice : enumeratedDevices) {
            cv::VideoCapture testCap;
            if (testCap.open(enumeratedDevice.hwIndex, cv::CAP_MSMF)) {
                devices.push_back(enumeratedDevice);
                testCap.release();
            }
        }
    }

    if (devices.empty()) {
        for (int i = 0; i < 5; i++) {
            cv::VideoCapture testCap;
            if (testCap.open(i, cv::CAP_MSMF)) {
                DeviceInfo info;
                info.hwIndex = i;
                info.name = "Dispositivo de Video [" + std::to_string(i) + "]";
                devices.push_back(info);
                testCap.release();
            }
        }
    }

    cv::utils::logging::setLogLevel(previousLogLevel);

    for (const DeviceInfo& device : devices) {
        std::cout << " - [" << device.hwIndex << "] " << device.name << std::endl;
    }

    std::cout << "Encontrados " << devices.size() << " dispositivo(s)." << std::endl;
    return devices;
}
