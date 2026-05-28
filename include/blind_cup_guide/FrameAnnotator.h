#pragma once

#include "blind_cup_guide/DetectionTypes.h"
#include "blind_cup_guide/GuidanceEngine.h"

#include <opencv2/core.hpp>

#include <string>
#include <vector>

namespace bcg {

std::string instructionKindName(InstructionKind kind);
std::string instructionOverlayText(InstructionKind kind);
void drawDebugOverlay(cv::Mat& frame,
                      const std::vector<Detection>& detections,
                      const Instruction& instruction);

} // namespace bcg
