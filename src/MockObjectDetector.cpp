#include "blind_cup_guide/MockObjectDetector.h"

#include <algorithm>
#include <chrono>

namespace bcg {

MockObjectDetector::MockObjectDetector()
    : startedAt_(std::chrono::steady_clock::now()) {}

std::vector<Detection> MockObjectDetector::detect(const cv::Mat& frame) {
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startedAt_);
    const float t = static_cast<float>(elapsed.count() % 12000) / 12000.0F;

    const int width = frame.cols;
    const int height = frame.rows;

    const cv::Rect cupBox{
        static_cast<int>(width * 0.24F),
        static_cast<int>(height * 0.42F),
        static_cast<int>(width * 0.13F),
        static_cast<int>(height * 0.24F)};

    const cv::Rect otherBox{
        static_cast<int>(width * 0.62F),
        static_cast<int>(height * 0.45F),
        static_cast<int>(width * 0.16F),
        static_cast<int>(height * 0.20F)};

    const float handXFactor = t < 0.45F
                                  ? 0.64F
                                  : std::max(0.25F, 0.64F - ((t - 0.45F) / 0.45F) * 0.40F);
    const cv::Rect handBox{
        static_cast<int>(width * handXFactor),
        static_cast<int>(height * 0.50F),
        static_cast<int>(width * 0.15F),
        static_cast<int>(height * 0.18F)};

    return {
        Detection{"cup", DetectionRole::Cup, 0.96F, cupBox},
        Detection{"other", DetectionRole::Other, 0.92F, otherBox},
        Detection{"hand", DetectionRole::Hand, 0.95F, handBox},
    };
}

} // namespace bcg
