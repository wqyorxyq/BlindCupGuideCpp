#pragma once

#include "blind_cup_guide/IObjectDetector.h"

#include <chrono>

namespace bcg {

class MockObjectDetector final : public IObjectDetector {
public:
    MockObjectDetector();
    std::vector<Detection> detect(const cv::Mat& frame) override;

private:
    std::chrono::steady_clock::time_point startedAt_;
};

} // namespace bcg
