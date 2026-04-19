#pragma once
#include <string>

struct AppConfig {
    bool loaded = false;
    int deviceIndex = -1;
    int deviceHwIndex = -1;
    int capWidth = 1920;
    int capHeight = 1080;
    int capFps = 60;
    int captureBackend = 0;
    int srWidth = 3840;
    int srHeight = 2160;
    float denoiseStrength = 0.0f;
    int aiType = 0;
    bool enableDenoise = false;
    bool enableAI = false;
    bool enableFrameGeneration = false;
    bool showFpsViewer = true;
    bool enableAA = false;
    bool forceMjpg = true;
};

class ConfigManager {
public:
    static AppConfig loadConfig(const std::string& filepath);
    static void saveConfig(const std::string& filepath, const AppConfig& config);
};
