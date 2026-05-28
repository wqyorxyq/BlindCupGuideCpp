#include "blind_cup_guide/Application.h"

#include "blind_cup_guide/CameraSource.h"

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

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

std::string englishInstruction(InstructionKind kind) {
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

void drawOverlay(cv::Mat& frame,
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

    const auto text = englishInstruction(instruction.kind);
    if (!text.empty()) {
        cv::rectangle(frame, cv::Rect{0, 0, frame.cols, 48}, cv::Scalar{20, 20, 20}, cv::FILLED);
        cv::putText(frame, text, cv::Point{16, 32}, cv::FONT_HERSHEY_SIMPLEX, 0.85,
                    cv::Scalar{255, 255, 255}, 2);
    }
}

} // namespace

Application::Application(AppOptions options,
                         std::unique_ptr<IObjectDetector> detector,
                         std::unique_ptr<ISpeechSynthesizer> speech)
    : options_(options),
      detector_(std::move(detector)),
      speech_(std::move(speech)) {
    if (!detector_) {
        throw std::invalid_argument("Application requires an object detector.");
    }
    if (!speech_) {
        throw std::invalid_argument("Application requires a speech synthesizer.");
    }
}

int Application::run() {
    CameraSource camera;
    if (!camera.open(options_.cameraIndex)) {
        std::cerr << "Cannot open camera index " << options_.cameraIndex << ".\n";
        return 2;
    }

    std::cout << "Camera opened. Press ESC or Q to quit.\n";

    while (true) {
        auto frame = camera.readFrame();
        if (!frame) {
            std::cerr << "Camera returned an empty frame.\n";
            return 3;
        }

        auto detections = detector_->detect(*frame);
        auto instruction = guidance_.evaluate(frame->size(), detections);
        speakIfNeeded(instruction);

        if (options_.displayWindow) {
            drawOverlay(*frame, detections, instruction);
            cv::imshow("Blind Cup Guide", *frame);
            const int key = cv::waitKey(1);
            if (key == 27 || key == 'q' || key == 'Q') {
                break;
            }
        }

        if (instruction.kind == InstructionKind::Correct && options_.stopOnSuccess) {
            if (options_.displayWindow) {
                cv::waitKey(1200);
            }
            break;
        }
    }

    return 0;
}

void Application::speakIfNeeded(const Instruction& instruction) {
    if (instruction.kind == InstructionKind::None || instruction.spokenText.empty()) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    const bool changed = instruction.kind != lastInstruction_;
    const bool expired = lastSpeechTime_ == std::chrono::steady_clock::time_point{} ||
                         now - lastSpeechTime_ >= options_.repeatSpeechAfter;

    if (changed || expired) {
        speech_->speak(instruction.spokenText);
        lastInstruction_ = instruction.kind;
        lastSpeechTime_ = now;
    }
}

} // namespace bcg
