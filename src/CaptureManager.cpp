#include "CaptureManager.hpp"
#include <opencv2/core/utils/logger.hpp>
#include <mfapi.h>
#include <mfidl.h>
#include <mfobjects.h>
#include <windows.h>
#include <iostream>

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
}

CaptureManager::CaptureManager() {}

CaptureManager::~CaptureManager() {
    release();
}

bool CaptureManager::initialize(int deviceIndex, int width, int height, int fps, bool useMjpeg) {
    cap.open(deviceIndex, cv::CAP_MSMF);

    if (!cap.isOpened()) {
        std::cerr << "Error: No se pudo abrir la capturadora USB en el indice " << deviceIndex << "." << std::endl;
        return false;
    }

    if (useMjpeg) {
        cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    cap.set(cv::CAP_PROP_FPS, fps);
    cap.set(cv::CAP_PROP_BUFFERSIZE, 1);

    double realWidth = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    double realHeight = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    double realFPS = cap.get(cv::CAP_PROP_FPS);
    std::cout << "Dispositivo " << deviceIndex
              << " configurado: " << realWidth << "x" << realHeight
              << " @ " << realFPS << " FPS (" << (useMjpeg ? "MJPEG" : "Uncompressed") << ")" << std::endl;

    return true;
}

bool CaptureManager::getNextFrame(cv::Mat& outFrame) {
    if (!cap.grab()) return false;
    return cap.retrieve(outFrame);
}

void CaptureManager::release() {
    if (cap.isOpened()) {
        cap.release();
    }
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
