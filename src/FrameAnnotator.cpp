#include "blind_cup_guide/FrameAnnotator.h"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace bcg {
namespace {

cv::Scalar colorForRole(DetectionRole role) {
    switch (role) {
    case DetectionRole::Cup:
        return {40, 210, 70};
    case DetectionRole::Hand:
        return {60, 140, 255};
    case DetectionRole::Other:
        return {50, 80, 230};
    case DetectionRole::Unknown:
        return {210, 210, 210};
    }
    return {210, 210, 210};
}

} // namespace

std::string instructionKindName(InstructionKind kind) {
    switch (kind) {
    case InstructionKind::Correct:
        return "correct";
    case InstructionKind::MoveLeft:
        return "moveLeft";
    case InstructionKind::MoveRight:
        return "moveRight";
    case InstructionKind::PutHandInFrame:
        return "putHandInFrame";
    case InstructionKind::CupNotFound:
        return "cupNotFound";
    case InstructionKind::None:
        return "none";
    }
    return "none";
}

std::string instructionOverlayText(InstructionKind kind) {
    switch (kind) {
    case InstructionKind::Correct:
        return "Correct";
    case InstructionKind::MoveLeft:
        return "Move left";
    case InstructionKind::MoveRight:
        return "Move right";
    case InstructionKind::PutHandInFrame:
        return "Show hand";
    case InstructionKind::CupNotFound:
        return "Cup not found";
    case InstructionKind::None:
        return "";
    }
    return "";
}

void drawDebugOverlay(cv::Mat& frame,
                      const std::vector<Detection>& detections,
                      const Instruction& instruction) {
    for (const auto& detection : detections) {
        const auto color = colorForRole(detection.role);
        cv::rectangle(frame, detection.box, color, 2);

        std::ostringstream label;
        label << detection.label << " " << std::fixed << std::setprecision(0)
              << (detection.confidence * 100.0F) << "%";

        const auto origin = cv::Point{detection.box.x, std::max(18, detection.box.y - 6)};
        cv::putText(frame, label.str(), origin, cv::FONT_HERSHEY_SIMPLEX, 0.55, color, 2);
    }

    const auto text = instructionOverlayText(instruction.kind);
    if (!text.empty()) {
        cv::rectangle(frame, cv::Rect{0, 0, frame.cols, 48}, cv::Scalar{20, 20, 20}, cv::FILLED);
        cv::putText(frame, text, cv::Point{16, 32}, cv::FONT_HERSHEY_SIMPLEX, 0.85,
                    cv::Scalar{255, 255, 255}, 2);
    }
}

} // namespace bcg
