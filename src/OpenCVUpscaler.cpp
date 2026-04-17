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

void OpenCVUpscaler::release() {
    isLoaded = false;
}
