#pragma once
#include <opencv2/opencv.hpp>
#include <opencv2/dnn_superres.hpp>
#include <string>

enum class SpatialScalerType {
    NEAREST = 0,
    BILINEAR = 1,
    BICUBIC = 2,
    LANCZOS4 = 3,
    SHARP_BILINEAR = 4
};

class OpenCVUpscaler {
public:
    OpenCVUpscaler();
    ~OpenCVUpscaler();

    // Inicializa el modelo FSRCNN cargando el .pb y seteando el algoritmo.
    bool initialize(const std::string& modelPath, int scale);
    
    // Procesa el frame con IA o fallback (si no se cargo el modelo).
    bool processFrame(const cv::Mat& inFrame, cv::Mat& outFrame);

    // Escaladores espaciales ligeros estilo "lossless".
    bool processSpatialFrame(const cv::Mat& inFrame, cv::Mat& outFrame, cv::Size targetSize, SpatialScalerType spatialType);
    
    // Libera recursos.
    void release();

private:
    static int getInterpolation(SpatialScalerType spatialType);
    static void applyLightSharpen(cv::Mat& frame);

    cv::dnn_superres::DnnSuperResImpl sr;
    bool isLoaded;
};
