#include "blind_cup_guide/DetectionTypes.h"

#include <algorithm>
#include <cctype>

namespace bcg {
namespace {

std::string lowerCopy(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

} // namespace

cv::Point2f centerOf(const cv::Rect& box) {
    return {static_cast<float>(box.x) + static_cast<float>(box.width) * 0.5F,
            static_cast<float>(box.y) + static_cast<float>(box.height) * 0.5F};
}

DetectionRole roleFromLabel(const std::string& label) {
    const auto normalized = lowerCopy(label);
    if (normalized == "cup" || normalized == "mug" || normalized == "glass") {
        return DetectionRole::Cup;
    }
    if (normalized == "hand" || normalized == "palm" || normalized == "finger") {
        return DetectionRole::Hand;
    }
    if (normalized == "other" || normalized == "object" || normalized == "not_cup") {
        return DetectionRole::Other;
    }
    return DetectionRole::Unknown;
}

std::string roleName(DetectionRole role) {
    switch (role) {
    case DetectionRole::Cup:
        return "cup";
    case DetectionRole::Hand:
        return "hand";
    case DetectionRole::Other:
        return "other";
    case DetectionRole::Unknown:
        return "unknown";
    }
    return "unknown";
}

} // namespace bcg
