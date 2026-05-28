#pragma once

#include "blind_cup_guide/DetectionTypes.h"

#include <opencv2/core.hpp>

#include <string>
#include <vector>

namespace bcg {

enum class InstructionKind {
    None,
    Correct,
    MoveLeft,
    MoveRight,
    PutHandInFrame,
    CupNotFound
};

struct Instruction {
    InstructionKind kind = InstructionKind::None;
    std::string spokenText;
};

class GuidanceEngine {
public:
    Instruction evaluate(const cv::Size& frameSize,
                         const std::vector<Detection>& detections) const;

private:
    static const Detection* bestDetectionForRole(const std::vector<Detection>& detections,
                                                 DetectionRole role);
    static const Detection* bestOtherObject(const std::vector<Detection>& detections);
    static bool isGrabbing(const Detection& hand, const Detection& target);
};

} // namespace bcg
