#include "OpenCVUpscaler.hpp"
#include <iostream>

OpenCVUpscaler::OpenCVUpscaler() : isLoaded(false) {}

OpenCVUpscaler::~OpenCVUpscaler() {
    release();
}

bool OpenCVUpscaler::initialize(const std::string& modelPath, int scale) {
    if (isLoaded) release();

    try {
        sr.readModel(modelPath);
        sr.setModel("fsrcnn", scale);
        
        // Habilitamos OpenCL (GPU genérica) si está disponible
        sr.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        sr.setPreferableTarget(cv::dnn::DNN_TARGET_OPENCL);

        isLoaded = true;
        std::cout << "[OpenCVUpscaler] FSRCNN x" << scale << " Inicializado (ONNX/PB)." << std::endl;
        return true;
    } catch (const cv::Exception& e) {
        std::cerr << "[OpenCVUpscaler] Error cargando modelo FSRCNN: " << e.what() << std::endl;
        isLoaded = false;
        return false;
    }
}

bool OpenCVUpscaler::processFrame(const cv::Mat& inFrame, cv::Mat& outFrame) {
    if (!isLoaded || inFrame.empty()) return false;

    try {
        sr.upsample(inFrame, outFrame);
        return true;
    } catch (...) {
        return false;
    }
}

bool OpenCVUpscaler::processSpatialFrame(const cv::Mat& inFrame, cv::Mat& outFrame, cv::Size targetSize, SpatialScalerType spatialType) {
    if (inFrame.empty() || targetSize.width <= 0 || targetSize.height <= 0) {
        return false;
    }

    try {
        cv::resize(inFrame, outFrame, targetSize, 0.0, 0.0, getInterpolation(spatialType));
        if (spatialType == SpatialScalerType::SHARP_BILINEAR) {
            applyLightSharpen(outFrame);
        }
        return true;
    } catch (...) {
        return false;
    }
}

void OpenCVUpscaler::release() {
    isLoaded = false;
}

int OpenCVUpscaler::getInterpolation(SpatialScalerType spatialType) {
    switch (spatialType) {
    case SpatialScalerType::NEAREST:
        return cv::INTER_NEAREST;
    case SpatialScalerType::BILINEAR:
    case SpatialScalerType::SHARP_BILINEAR:
        return cv::INTER_LINEAR;
    case SpatialScalerType::BICUBIC:
        return cv::INTER_CUBIC;
    case SpatialScalerType::LANCZOS4:
    default:
        return cv::INTER_LANCZOS4;
    }
}

void OpenCVUpscaler::applyLightSharpen(cv::Mat& frame) {
    cv::Mat sharpened;
    static const cv::Mat kernel = (cv::Mat_<float>(3, 3) <<
        0.0f, -0.5f, 0.0f,
        -0.5f, 3.0f, -0.5f,
        0.0f, -0.5f, 0.0f);
    cv::filter2D(frame, sharpened, -1, kernel);
    sharpened.copyTo(frame);
}
