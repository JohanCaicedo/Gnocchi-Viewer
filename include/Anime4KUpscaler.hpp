#pragma once

#include <memory>
#include <opencv2/opencv.hpp>

namespace ac::core {
class Processor;
}

class Anime4KUpscaler {
public:
    Anime4KUpscaler();
    ~Anime4KUpscaler();

    bool initialize(int srcWidth, int srcHeight, int dstWidth, int dstHeight);
    bool processFrame(const cv::Mat& input, cv::Mat& output);
    void release();

    bool isReady() const { return initialized; }

private:
    std::shared_ptr<ac::core::Processor> processor;
    bool initialized;
    int inputWidth;
    int inputHeight;
    int outputWidth;
    int outputHeight;
    double upscaleFactor;

    cv::Mat rgbInput;
    cv::Mat rgbOutput;
};
