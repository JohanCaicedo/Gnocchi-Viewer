#pragma once
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

enum class CaptureBackendType {
    OPENCV = 0,
    MEDIA_FOUNDATION = 1,
    AUTO = 2
};

enum class CapturePixelFormat {
    UNKNOWN = 0,
    MJPEG = 1,
    YUY2 = 2,
    H264 = 3
};

struct DeviceInfo {
    int hwIndex;
    std::string name;
};

struct CaptureSessionInfo {
    int requestedWidth = 1920;
    int requestedHeight = 1080;
    int requestedFps = 60;
    CapturePixelFormat requestedFormat = CapturePixelFormat::MJPEG;

    int effectiveWidth = 0;
    int effectiveHeight = 0;
    int effectiveFps = 0;
    CapturePixelFormat effectiveFormat = CapturePixelFormat::UNKNOWN;

    bool requestedResolutionApplied = false;
    bool requestedFpsApplied = false;
    bool requestedFormatApplied = false;
};

class CaptureManager {
public:
    CaptureManager();
    ~CaptureManager();

    static std::vector<DeviceInfo> getAvailableDevices();
    static bool probeMode(int deviceIndex, int width, int height, int fps, CapturePixelFormat format, CaptureSessionInfo& outInfo);
    static const char* pixelFormatToString(CapturePixelFormat format);

    bool initialize(int deviceIndex, int width = 1920, int height = 1080, int fps = 60, bool useMjpeg = true,
                    CaptureBackendType backend = CaptureBackendType::OPENCV);
    bool getNextFrame(cv::Mat& outFrame);
    void release();
    const CaptureSessionInfo& getSessionInfo() const { return sessionInfo; }

private:
    cv::VideoCapture cap;
    CaptureSessionInfo sessionInfo;

    static CapturePixelFormat decodeFourcc(double fourccValue);
    static bool isRequestedModeApplied(const CaptureSessionInfo& info);
    static void finalizeSessionInfo(cv::VideoCapture& capture, const cv::Mat& frame, CaptureSessionInfo& info, bool allowRequestedFormatFallback);
};
