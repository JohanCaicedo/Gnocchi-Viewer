#pragma once

#include <string>
#include <windows.h>
#include "NvOFFRUC.h"

class GPUFrameGenerator {
public:
    GPUFrameGenerator();
    ~GPUFrameGenerator();

    bool initialize(int width, int height);
    void release();

    bool isReady() const { return initialized; }
    bool isLibraryLoaded() const { return dllHandle != nullptr; }
    const std::string& getLastStatus() const { return lastStatus; }

private:
    bool loadLibrary();
    bool loadExports();
    std::wstring getPrimaryDllPath() const;
    std::wstring getFallbackDllPath() const;
    void setStatus(const std::string& status);

    HMODULE dllHandle;
    bool initialized;
    int frameWidth;
    int frameHeight;
    std::string lastStatus;

    PtrToFuncNvOFFRUCCreate createFn;
    PtrToFuncNvOFFRUCRegisterResource registerFn;
    PtrToFuncNvOFFRUCUnregisterResource unregisterFn;
    PtrToFuncNvOFFRUCProcess processFn;
    PtrToFuncNvOFFRUCDestroy destroyFn;
};
