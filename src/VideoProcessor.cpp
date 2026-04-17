#include "VideoProcessor.hpp"
#include <iostream>
#include <windows.h>

VideoProcessor::VideoProcessor() {}

VideoProcessor::~VideoProcessor() {}

bool VideoProcessor::initUpscaler(int srcW, int srcH, int dstW, int dstH, AIType type, int quality) {
    upOutW = dstW;
    upOutH = dstH;
    
    if (type == AIType::NVIDIA_RTX) {
        return upscaler.initialize(srcW, srcH, dstW, dstH, quality);
    } else if (type == AIType::OPENCV_FSRCNN) {
        int scale = (dstW / srcW >= 2) ? 2 : 2; 

        // Resolucion ABSOLUTA del modelo mediante Win32 API protegiendonos de llamadas CWD
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::string pathStr(exePath);
        size_t lastSlash = pathStr.find_last_of("\\/");
        std::string absoluteModelPath = pathStr.substr(0, lastSlash) + "\\models\\FSRCNN_x2.pb";

        return cvUpscaler.initialize(absoluteModelPath, scale);
    }
    return false;
}

bool VideoProcessor::initDenoiser(int width, int height, float strength) {
    return denoiser.initialize(width, height, strength);
}

void VideoProcessor::processFrame(const cv::Mat& inputFrame, cv::Mat& outputFrame, AIType upscalerType, bool enableDenoise) {
    if (inputFrame.empty()) return;

    // Solo RTX Upscaler
    if (upscalerType == AIType::NVIDIA_RTX && !enableDenoise) {
        if (!upscaler.upscale(inputFrame, outputFrame)) {
            inputFrame.copyTo(outputFrame);
        }
        return;
    }

    // Solo FSRCNN
    if (upscalerType == AIType::OPENCV_FSRCNN && !enableDenoise) {
        if (!cvUpscaler.processFrame(inputFrame, outputFrame)) {
            cv::resize(inputFrame, outputFrame, cv::Size(upOutW, upOutH), 0, 0, cv::INTER_LANCZOS4);
        }
        return;
    }

    // Solo Denoise
    if (enableDenoise && upscalerType == AIType::NONE) {
        if (!denoiser.denoise(inputFrame, outputFrame)) {
            inputFrame.copyTo(outputFrame);
        }
        return;
    }

    // Denoise -> RTX Upscaler (Zero-copy)
    if (enableDenoise && upscalerType == AIType::NVIDIA_RTX) {
        if (denoiser.denoiseToGPU(inputFrame)) {
            const NvCVImage* d_denoisedBuffer = denoiser.getOutputGPUBuffer();
            if (upscaler.upscaleFromGPU(d_denoisedBuffer, outputFrame)) {
                return;
            }
        }
        // Fallback
        inputFrame.copyTo(outputFrame);
        return;
    }

    // Denoise -> FSRCNN (Hybrid)
    if (enableDenoise && upscalerType == AIType::OPENCV_FSRCNN) {
        if (denoiser.denoise(inputFrame, denoiseInternalOutput)) {
            if (!cvUpscaler.processFrame(denoiseInternalOutput, outputFrame)) {
                cv::resize(denoiseInternalOutput, outputFrame, cv::Size(upOutW, upOutH), 0, 0, cv::INTER_LANCZOS4);
            }
        } else {
            // fallback
            inputFrame.copyTo(outputFrame);
        }
        return;
    }

    inputFrame.copyTo(outputFrame);
}

bool VideoProcessor::isUpscalerReady() const {
    return upscaler.isReady();
}

bool VideoProcessor::isDenoiserReady() const {
    return denoiser.isReady();
}

void VideoProcessor::releaseUpscaler() {
    upscaler.release();
    cvUpscaler.release();
}

void VideoProcessor::releaseDenoiser() {
    denoiser.release();
}
