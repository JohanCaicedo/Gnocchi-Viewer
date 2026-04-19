#include "Anime4KUpscaler.hpp"

#include <iostream>

#include <AC/Core/Image.hpp>
#include <AC/Core/Processor.hpp>

Anime4KUpscaler::Anime4KUpscaler()
    : initialized(false),
      inputWidth(0),
      inputHeight(0),
      outputWidth(0),
      outputHeight(0),
      upscaleFactor(1.0) {}

Anime4KUpscaler::~Anime4KUpscaler() {
    release();
}

bool Anime4KUpscaler::initialize(int srcWidth, int srcHeight, int dstWidth, int dstHeight) {
    release();

    inputWidth = srcWidth;
    inputHeight = srcHeight;
    outputWidth = dstWidth;
    outputHeight = dstHeight;

    if (srcWidth <= 0 || srcHeight <= 0 || dstWidth <= 0 || dstHeight <= 0) {
        std::cerr << "[Anime4K] Dimensiones invalidas." << std::endl;
        return false;
    }

    const double scaleX = static_cast<double>(dstWidth) / static_cast<double>(srcWidth);
    const double scaleY = static_cast<double>(dstHeight) / static_cast<double>(srcHeight);
    upscaleFactor = std::min(scaleX, scaleY);

    if (upscaleFactor <= 1.0) {
        std::cout << "[Anime4K] Omitido: la resolucion objetivo no supera la entrada." << std::endl;
        return true;
    }

    processor = ac::core::Processor::create("cpu", 0, "acnet-hdn0");
    if (!processor || !processor->ok()) {
        std::cerr << "[Anime4K] No se pudo crear el procesador Anime4KCPP."
                  << (processor ? std::string(" Error: ") + processor->error() : std::string())
                  << std::endl;
        processor.reset();
        return false;
    }

    initialized = true;
    std::cout << "[Anime4K] Inicializado con modelo ACNet-HDN0 (CPU). "
              << srcWidth << "x" << srcHeight << " -> "
              << dstWidth << "x" << dstHeight << std::endl;
    return true;
}

bool Anime4KUpscaler::processFrame(const cv::Mat& input, cv::Mat& output) {
    if (!initialized || !processor || input.empty()) {
        return false;
    }

    try {
        if (input.cols != inputWidth || input.rows != inputHeight) {
            cv::Mat resizedInput;
            cv::resize(input, resizedInput, cv::Size(inputWidth, inputHeight), 0, 0, cv::INTER_LINEAR);
            cv::cvtColor(resizedInput, rgbInput, cv::COLOR_BGR2RGB);
        } else {
            cv::cvtColor(input, rgbInput, cv::COLOR_BGR2RGB);
        }

        ac::core::Image srcImage(rgbInput.cols, rgbInput.rows, rgbInput.channels(), ac::core::Image::UInt8, rgbInput.data, static_cast<int>(rgbInput.step));
        ac::core::Image dstImage = processor->process(srcImage, upscaleFactor);

        if (dstImage.empty()) {
            return false;
        }

        rgbOutput.create(dstImage.height(), dstImage.width(), CV_8UC3);
        dstImage.to(rgbOutput.data, static_cast<int>(rgbOutput.step));

        cv::Mat bgrOutput;
        cv::cvtColor(rgbOutput, bgrOutput, cv::COLOR_RGB2BGR);

        if (bgrOutput.cols != outputWidth || bgrOutput.rows != outputHeight) {
            cv::resize(bgrOutput, output, cv::Size(outputWidth, outputHeight), 0, 0, cv::INTER_LANCZOS4);
        } else {
            bgrOutput.copyTo(output);
        }
        return true;
    } catch (const std::exception& ex) {
        std::cerr << "[Anime4K] Error procesando frame: " << ex.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "[Anime4K] Error desconocido procesando frame." << std::endl;
        return false;
    }
}

void Anime4KUpscaler::release() {
    processor.reset();
    initialized = false;
    upscaleFactor = 1.0;
}
