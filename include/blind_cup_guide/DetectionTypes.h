#pragma once

#include <opencv2/core.hpp>

#include <string>
#include <vector>

namespace bcg {

enum class DetectionRole {
    Cup,
    Hand,
    Other,
    Unknown
};

struct Detection {
    std::string label;
    DetectionRole role = DetectionRole::Unknown;
    float confidence = 0.0F;
    cv::Rect box;
};

cv::Point2f centerOf(const cv::Rect& box);
DetectionRole roleFromLabel(const std::string& label);
std::string roleName(DetectionRole role);

} // namespace bcg
