#pragma once
#include <string>

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    // Inicia el puente de audio (capturadora USB -> audifonos)
    bool start(const std::string& preferredCaptureName = "");

    // Detiene el audio
    void stop();

private:
    void* miniaudioDevice;
    bool isRunning;
};
