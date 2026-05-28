#pragma once

#include "blind_cup_guide/DetectionTypes.h"

#include <opencv2/core.hpp>

#include <vector>

namespace bcg {

class IObjectDetector {
public:
    virtual ~IObjectDetector() = default;
    virtual std::vector<Detection> detect(const cv::Mat& frame) = 0;
};

} // namespace bcg
