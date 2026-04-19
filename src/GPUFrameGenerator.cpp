#include "GPUFrameGenerator.hpp"
#include <iostream>

namespace {
std::wstring getExecutableDir() {
    wchar_t exePath[MAX_PATH] = {};
    if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) == 0) {
        return L"";
    }

    std::wstring path(exePath);
    const size_t lastSlash = path.find_last_of(L"\\/");
    if (lastSlash == std::wstring::npos) {
        return L"";
    }
    return path.substr(0, lastSlash);
}
}

GPUFrameGenerator::GPUFrameGenerator()
    : dllHandle(nullptr),
      initialized(false),
      frameWidth(0),
      frameHeight(0),
      createFn(nullptr),
      registerFn(nullptr),
      unregisterFn(nullptr),
      processFn(nullptr),
      destroyFn(nullptr) {}

GPUFrameGenerator::~GPUFrameGenerator() {
    release();
}

bool GPUFrameGenerator::initialize(int width, int height) {
    release();

    frameWidth = width;
    frameHeight = height;

    if (!loadLibrary()) {
        return false;
    }

    if (!loadExports()) {
        release();
        return false;
    }

    initialized = true;
    setStatus("FRUC listo para integracion futura.");
    std::cout << "[FRUC] Base cargada. " << lastStatus << std::endl;
    return true;
}

void GPUFrameGenerator::release() {
    initialized = false;
    createFn = nullptr;
    registerFn = nullptr;
    unregisterFn = nullptr;
    processFn = nullptr;
    destroyFn = nullptr;

    if (dllHandle) {
        FreeLibrary(dllHandle);
        dllHandle = nullptr;
    }
}

bool GPUFrameGenerator::loadLibrary() {
    const std::wstring primaryPath = getPrimaryDllPath();
    dllHandle = LoadLibraryExW(primaryPath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!dllHandle) {
        const std::wstring fallbackPath = getFallbackDllPath();
        dllHandle = LoadLibraryExW(fallbackPath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    }

    if (!dllHandle) {
        setStatus("No se pudo cargar NvOFFRUC.dll.");
        std::cerr << "[FRUC] " << lastStatus << std::endl;
        return false;
    }

    return true;
}

bool GPUFrameGenerator::loadExports() {
    createFn = reinterpret_cast<PtrToFuncNvOFFRUCCreate>(GetProcAddress(dllHandle, CreateProcName));
    registerFn = reinterpret_cast<PtrToFuncNvOFFRUCRegisterResource>(GetProcAddress(dllHandle, RegisterResourceProcName));
    unregisterFn = reinterpret_cast<PtrToFuncNvOFFRUCUnregisterResource>(GetProcAddress(dllHandle, UnregisterResourceProcName));
    processFn = reinterpret_cast<PtrToFuncNvOFFRUCProcess>(GetProcAddress(dllHandle, ProcessProcName));
    destroyFn = reinterpret_cast<PtrToFuncNvOFFRUCDestroy>(GetProcAddress(dllHandle, DestroyProcName));

    if (!createFn || !registerFn || !unregisterFn || !processFn || !destroyFn) {
        setStatus("La DLL de FRUC no expone todas las funciones esperadas.");
        std::cerr << "[FRUC] " << lastStatus << std::endl;
        return false;
    }

    return true;
}

std::wstring GPUFrameGenerator::getPrimaryDllPath() const {
    return getExecutableDir() + L"\\NvOFFRUC.dll";
}

std::wstring GPUFrameGenerator::getFallbackDllPath() const {
    const char* root = CAPTURADORA_OPTICAL_FLOW_SDK_DIR;
    const std::string narrow(root ? root : "");
    std::wstring sdkRoot(narrow.begin(), narrow.end());
    return sdkRoot + L"\\NvOFFRUC\\NvOFFRUCSample\\bin\\win64\\NvOFFRUC.dll";
}

void GPUFrameGenerator::setStatus(const std::string& status) {
    lastStatus = status;
}
