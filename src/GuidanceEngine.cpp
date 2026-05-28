#include "blind_cup_guide/GuidanceEngine.h"

#include <algorithm>
#include <cmath>

namespace bcg {
namespace {

Instruction instruction(InstructionKind kind, const char* spokenText) {
    return {kind, spokenText};
}

float intersectionRatioToSmallerBox(const cv::Rect& a, const cv::Rect& b) {
    const auto intersection = a & b;
    if (intersection.empty()) {
        return 0.0F;
    }

    const auto smallerArea = static_cast<float>(std::max(1, std::min(a.area(), b.area())));
    return static_cast<float>(intersection.area()) / smallerArea;
}

float centerDistance(const cv::Rect& a, const cv::Rect& b) {
    const auto ca = centerOf(a);
    const auto cb = centerOf(b);
    const auto dx = ca.x - cb.x;
    const auto dy = ca.y - cb.y;
    return std::sqrt(dx * dx + dy * dy);
}

} // namespace

Instruction GuidanceEngine::evaluate(const cv::Size& frameSize,
                                     const std::vector<Detection>& detections) const {
    (void)frameSize;

    const auto* cup = bestDetectionForRole(detections, DetectionRole::Cup);
    const auto* hand = bestDetectionForRole(detections, DetectionRole::Hand);

    if (!cup) {
        return instruction(InstructionKind::CupNotFound, u8"我还没看到杯子");
    }
    if (!hand) {
        return instruction(InstructionKind::PutHandInFrame, u8"请把手放到画面中");
    }

    if (isGrabbing(*hand, *cup)) {
        return instruction(InstructionKind::Correct, u8"做对了！");
    }

    const auto* other = bestOtherObject(detections);
    const bool grabbingOther = other != nullptr && isGrabbing(*hand, *other);
    (void)grabbingOther;

    const auto cupCenter = centerOf(cup->box);
    const auto handCenter = centerOf(hand->box);
    if (cupCenter.x < handCenter.x) {
        return instruction(InstructionKind::MoveLeft, u8"往左边一点");
    }
    return instruction(InstructionKind::MoveRight, u8"往右边一点");
}

const Detection* GuidanceEngine::bestDetectionForRole(const std::vector<Detection>& detections,
                                                      DetectionRole role) {
    const Detection* best = nullptr;
    for (const auto& detection : detections) {
        if (detection.role != role) {
            continue;
        }
        if (!best || detection.confidence > best->confidence) {
            best = &detection;
        }
    }
    return best;
}

const Detection* GuidanceEngine::bestOtherObject(const std::vector<Detection>& detections) {
    const Detection* best = nullptr;
    for (const auto& detection : detections) {
        if (detection.role != DetectionRole::Other && detection.role != DetectionRole::Unknown) {
            continue;
        }
        if (!best || detection.confidence > best->confidence) {
            best = &detection;
        }
    }
    return best;
}

bool GuidanceEngine::isGrabbing(const Detection& hand, const Detection& target) {
    constexpr float minIntersectionRatio = 0.12F;
    const auto ratio = intersectionRatioToSmallerBox(hand.box, target.box);
    if (ratio >= minIntersectionRatio) {
        return true;
    }

    const auto maxUsefulDistance =
        0.45F * static_cast<float>(std::max({hand.box.width, hand.box.height,
                                             target.box.width, target.box.height}));
    return centerDistance(hand.box, target.box) <= maxUsefulDistance;
}

} // namespace bcg
