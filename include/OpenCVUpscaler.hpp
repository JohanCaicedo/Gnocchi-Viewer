#pragma once
#include <opencv2/opencv.hpp>
#include <opencv2/dnn_superres.hpp>
#include <string>

class OpenCVUpscaler {
public:
    OpenCVUpscaler();
    ~OpenCVUpscaler();

    // Inicializa el modelo FSRCNN cargando el .pb y seteando el algoritmo.
    bool initialize(const std::string& modelPath, int scale);
    
    // Procesa el frame con IA o fallback (si no se cargo el modelo).
    bool processFrame(const cv::Mat& inFrame, cv::Mat& outFrame);
    
    // Libera recursos.
    void release();

private:
    cv::dnn_superres::DnnSuperResImpl sr;
    bool isLoaded;
};
