#pragma warning(push)
#pragma warning(disable: 4244)
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#pragma warning(pop)
#include "AudioManager.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {
std::string toLowerCopy(const std::string& value) {
    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return lower;
}

std::vector<std::string> tokenizeName(const std::string& value) {
    std::string lower = toLowerCopy(value);
    for (char& c : lower) {
        if (!std::isalnum(static_cast<unsigned char>(c))) {
            c = ' ';
        }
    }

    std::istringstream iss(lower);
    std::vector<std::string> tokens;
    std::string token;
    static const std::set<std::string> ignoredTokens = {
        "audio", "video", "digital", "entrada", "input", "microphone", "microfono",
        "usb", "device", "dispositivo", "capture", "capturadora", "webcam"
    };

    while (iss >> token) {
        if (token.size() < 3) continue;
        if (ignoredTokens.count(token) > 0) continue;
        tokens.push_back(token);
    }
    return tokens;
}

int computeMatchScore(const std::string& preferredCaptureName, const std::string& candidateName) {
    if (preferredCaptureName.empty()) return 0;

    const std::string preferredLower = toLowerCopy(preferredCaptureName);
    const std::string candidateLower = toLowerCopy(candidateName);
    int score = 0;

    const std::vector<std::string> preferredTokens = tokenizeName(preferredCaptureName);
    for (const std::string& token : preferredTokens) {
        if (candidateLower.find(token) != std::string::npos) {
            score += 4;
        }
    }

    if (preferredLower == candidateLower) {
        score += 100;
    }

    return score;
}

bool looksLikeCaptureAudio(const std::string& name) {
    const std::string lower = toLowerCopy(name);
    return lower.find("usb") != std::string::npos ||
           lower.find("digital") != std::string::npos ||
           lower.find("fhd") != std::string::npos ||
           lower.find("capture") != std::string::npos ||
           lower.find("hdmi") != std::string::npos ||
           lower.find("uvc") != std::string::npos;
}
}

// Callback de audio de muy baja latencia, recoge el input (capturadora) y lo pone en el output (audifonos)
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    if (pInput != NULL && pOutput != NULL) {
        const float* pIn = (const float*)pInput;
        float* pOut = (float*)pOutput;
        ma_uint32 channels = pDevice->playback.channels;
        for (ma_uint32 i = 0; i < frameCount * channels; ++i) {
            pOut[i] = pIn[i];
        }
    }
}

AudioManager::AudioManager() : miniaudioDevice(nullptr), isRunning(false) {
}

AudioManager::~AudioManager() {
    stop();
}

bool AudioManager::start(const std::string& preferredCaptureName) {
    if (isRunning) return true;

    ma_context context;
    if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
        std::cerr << "[Audio] Fallo al inicializar contexto de audio." << std::endl;
        return false;
    }

    ma_device_info* pPlaybackInfos;
    ma_uint32 playbackCount;
    ma_device_info* pCaptureInfos;
    ma_uint32 captureCount;

    if (ma_context_get_devices(&context, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) != MA_SUCCESS) {
        ma_context_uninit(&context);
        return false;
    }

    ma_device_id targetCaptureId;
    bool found = false;
    int bestScore = -1;
    std::string selectedName;

    for (ma_uint32 i = 0; i < captureCount; ++i) {
        const std::string name = pCaptureInfos[i].name;
        const int score = computeMatchScore(preferredCaptureName, name);
        if (score > bestScore) {
            bestScore = score;
            targetCaptureId = pCaptureInfos[i].id;
            selectedName = name;
            found = true;
        }
    }

    if (bestScore <= 0) {
        found = false;
        for (ma_uint32 i = 0; i < captureCount; ++i) {
            const std::string name = pCaptureInfos[i].name;
            if (looksLikeCaptureAudio(name)) {
                targetCaptureId = pCaptureInfos[i].id;
                selectedName = name;
                found = true;
                break;
            }
        }
    }

    if (found) {
        std::cout << "[Audio] Audio enlazado a la capturadora: " << selectedName;
        if (!preferredCaptureName.empty()) {
            std::cout << " | Video: " << preferredCaptureName;
        }
        std::cout << std::endl;
    } else {
        std::cout << "[Audio] No se encontro una entrada de audio claramente asociada; se intentara usar el dispositivo por defecto." << std::endl;
    }

    ma_context_uninit(&context);

    ma_device_config config = ma_device_config_init(ma_device_type_duplex);
    config.capture.pDeviceID  = found ? &targetCaptureId : NULL;
    config.capture.format     = ma_format_f32;
    config.capture.channels   = 2;
    config.playback.pDeviceID = NULL;
    config.playback.format    = ma_format_f32;
    config.playback.channels  = 2;
    config.sampleRate         = 48000;
    config.dataCallback       = data_callback;
    config.periodSizeInFrames = 480;

    ma_device* pDevice = new ma_device;
    if (ma_device_init(NULL, &config, pDevice) != MA_SUCCESS) {
        std::cerr << "[Audio] Error inicializando el puente de audio." << std::endl;
        delete pDevice;
        return false;
    }

    if (ma_device_start(pDevice) != MA_SUCCESS) {
        std::cerr << "[Audio] Error arrancando motor de audio." << std::endl;
        ma_device_uninit(pDevice);
        delete pDevice;
        return false;
    }

    miniaudioDevice = pDevice;
    isRunning = true;
    std::cout << "[Audio] Engine de Latencia Ultra-Baja activo y funcionando." << std::endl;
    return true;
}

void AudioManager::stop() {
    if (isRunning && miniaudioDevice) {
        ma_device* pDevice = (ma_device*)miniaudioDevice;
        ma_device_uninit(pDevice);
        delete pDevice;
        miniaudioDevice = nullptr;
        isRunning = false;
        std::cout << "[Audio] Engine detenido." << std::endl;
    }
}
