#pragma once
#include <opencv2/opencv.hpp>
#include "GPUDenoiser.hpp"
#include "GPUFrameGenerator.hpp"
#include "GPUUpscaler.hpp"
#include "OpenCVUpscaler.hpp"

enum class AIType {
    NONE = 0,
    NVIDIA_RTX = 1,
    OPENCV_FSRCNN = 2,
    SPATIAL_NEAREST = 3,
    SPATIAL_BILINEAR = 4,
    SPATIAL_BICUBIC = 5,
    SPATIAL_LANCZOS4 = 6,
    SPATIAL_SHARP_BILINEAR = 7
};

class VideoProcessor {
public:
    VideoProcessor();
    ~VideoProcessor();

    // Inicializa el escalador (FSRCNN o NVIDIA)
    bool initUpscaler(int inWidth, int inHeight, int outWidth, int outHeight, AIType type = AIType::NVIDIA_RTX, int quality = GPUUpscaler::kModeMjpegDefault);

    // Inicializa el denoiser GPU
    bool initDenoiser(int width, int height, float strength = 0.0f);

    // Procesa el frame (Cadena de IA)
    void processFrame(const cv::Mat& inFrame, cv::Mat& outFrame, AIType upscalerType, bool enableDenoise = false);

    // Destruye el upscaler para cambiar resoluciones
    void releaseUpscaler();

    // Destruye el denoiser
    void releaseDenoiser();

    // Verifica si el upscaler esta listo
    bool isUpscalerReady() const;

    // Verifica si el denoiser esta listo
    bool isDenoiserReady() const;

    // Base para la futura integracion de FRUC.
    bool initFrameGenerator(int width, int height);
    void releaseFrameGenerator();
    bool isFrameGeneratorReady() const;

private:
    static bool isSpatialType(AIType type);
    static SpatialScalerType toSpatialScalerType(AIType type);

    GPUUpscaler upscaler;
    GPUDenoiser denoiser;
    GPUFrameGenerator frameGenerator;
    OpenCVUpscaler cvUpscaler;
    
    cv::Mat denoiseInternalOutput;
    cv::Mat fsrcnnInternalOutput;
    
    // Dimensiones para FSRCNN / RTX
    int upOutW, upOutH;
    bool fsrcnnEnabled;
};
