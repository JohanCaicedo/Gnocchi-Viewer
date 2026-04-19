#pragma once
#include "CaptureManager.hpp"
#include "VideoProcessor.hpp"
#include "AudioManager.hpp"
#include "AppWindow.hpp"
#include "ConfigManager.hpp"
#include <string>
#include <vector>
#include <windows.h>

#define IDM_DEVICE_0 1000
#define IDM_DEVICE_MAX 1010

#define IDM_VIDEO_720_30 2100
#define IDM_VIDEO_720_60 2101
#define IDM_VIDEO_1080_30 2102
#define IDM_VIDEO_1080_60 2103

#define IDM_FORMAT_MJPEG 2200
#define IDM_FORMAT_YUY2 2201

#define IDM_DENOISE_TOGGLE 3000
#define IDM_DENOISE_00 3001
#define IDM_DENOISE_05 3002
#define IDM_DENOISE_10 3003

#define IDM_AA_TOGGLE 4000

#define IDM_FRAMEGEN_TOGGLE 4500

#define IDM_AI_RTX_TOGGLE 5000
#define IDM_AI_FSRCNN_TOGGLE 5004
#define IDM_TARGET_1080 5001
#define IDM_TARGET_1440 5002
#define IDM_TARGET_2160 5003
#define IDM_RTX_QUALITY_BICUBIC 5020
#define IDM_RTX_QUALITY_LOW 5021
#define IDM_RTX_QUALITY_MEDIUM 5022
#define IDM_RTX_QUALITY_HIGH 5023
#define IDM_RTX_QUALITY_ULTRA 5024
#define IDM_RTX_QUALITY_HB_LOW 5025
#define IDM_RTX_QUALITY_HB_MEDIUM 5026
#define IDM_RTX_QUALITY_HB_HIGH 5027
#define IDM_RTX_QUALITY_HB_ULTRA 5028
#define IDM_AI_SPATIAL_NEAREST 5010
#define IDM_AI_SPATIAL_BILINEAR 5011
#define IDM_AI_SPATIAL_BICUBIC 5012
#define IDM_AI_SPATIAL_LANCZOS4 5013
#define IDM_AI_SPATIAL_SHARP_BILINEAR 5014
#define IDM_AI_ANIME4K_TOGGLE 5015

#define IDM_SAVE_CONFIG 6000

#define IDM_TAKE_SCREENSHOT 7000
#define IDM_FPS_VIEWER_TOGGLE 7001

class Application {
public:
    Application(int deviceIndex);
    ~Application();

    void run();
    void handleWin32Command(int menuId);

private:
    enum class VideoModePreset {
        P720_30 = 0,
        P720_60 = 1,
        P1080_30 = 2,
        P1080_60 = 3
    };

    int deviceIndex;
    bool isRunning;
    bool enableAA;
    bool enableDenoise;
    bool enableFrameGeneration;
    bool showFpsViewer;
    bool forceMjpg;
    AIType currentAIType;
    bool yuy2AvailableForCurrentMode;

    int capWidth = 1920;
    int capHeight = 1080;
    int capFps = 60;
    int srWidth = 3840;
    int srHeight = 2160;
    float denoiseStrength = 0.0f;
    int rtxQuality = GPUUpscaler::kModeMjpegDefault;

    std::vector<DeviceInfo> devices;

    CaptureManager captureManager;
    VideoProcessor videoProcessor;
    AudioManager audioManager;
    AppWindow appWindow;

    bool pendingCaptureRestart;
    bool pendingAIInit;
    bool pendingDenoiseInit;
    bool pendingScreenshot;

    void handleInput();
    void setupNativeMenu();
    void updateMenuChecks();
    void applyVideoPreset(VideoModePreset preset);
    VideoModePreset getCurrentVideoPreset() const;
    void refreshFormatAvailability();
    void applyNegotiatedCaptureState();
    void updateWindowTitle();
    void scheduleRTXReinitIfActive();
};
