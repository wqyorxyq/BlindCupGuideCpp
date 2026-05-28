#pragma once

#include "blind_cup_guide/GuidanceEngine.h"
#include "blind_cup_guide/IObjectDetector.h"
#include "blind_cup_guide/SpeechSynthesizer.h"

#include <chrono>
#include <memory>

namespace bcg {

struct AppOptions {
    int cameraIndex = 0;
    bool displayWindow = true;
    bool stopOnSuccess = true;
    std::chrono::milliseconds repeatSpeechAfter{1600};
};

class Application {
public:
    Application(AppOptions options,
                std::unique_ptr<IObjectDetector> detector,
                std::unique_ptr<ISpeechSynthesizer> speech);

    int run();

private:
    void speakIfNeeded(const Instruction& instruction);

    AppOptions options_;
    std::unique_ptr<IObjectDetector> detector_;
    std::unique_ptr<ISpeechSynthesizer> speech_;
    GuidanceEngine guidance_;
    InstructionKind lastInstruction_ = InstructionKind::None;
    std::chrono::steady_clock::time_point lastSpeechTime_{};
};

} // namespace bcg
