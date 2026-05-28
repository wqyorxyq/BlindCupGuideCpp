#include "blind_cup_guide/Application.h"

#include "blind_cup_guide/CameraSource.h"
#include "blind_cup_guide/FrameAnnotator.h"

#include <opencv2/highgui.hpp>

#include <iostream>
#include <stdexcept>

namespace bcg {

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
            drawDebugOverlay(*frame, detections, instruction);
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
