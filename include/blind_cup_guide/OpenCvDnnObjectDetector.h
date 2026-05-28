#pragma once

#include "blind_cup_guide/IObjectDetector.h"

#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>

#include <string>
#include <vector>

namespace bcg {

struct DnnDetectorOptions {
    std::string modelPath;
    std::string labelsPath;
    float confidenceThreshold = 0.35F;
    float nmsThreshold = 0.45F;
    cv::Size inputSize{640, 640};
};

class OpenCvDnnObjectDetector final : public IObjectDetector {
public:
    explicit OpenCvDnnObjectDetector(DnnDetectorOptions options);
    std::vector<Detection> detect(const cv::Mat& frame) override;

private:
    static std::vector<std::string> loadLabels(const std::string& labelsPath);
    static cv::Mat outputToRows(const cv::Mat& output);
    cv::Rect toFrameBox(const float* row, const cv::Size& frameSize) const;

    DnnDetectorOptions options_;
    std::vector<std::string> labels_;
    cv::dnn::Net net_;
};

} // namespace bcg
