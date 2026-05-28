#pragma once

#include <memory>
#include <string>

namespace bcg {

class ISpeechSynthesizer {
public:
    virtual ~ISpeechSynthesizer() = default;
    virtual void speak(const std::string& text) = 0;
};

std::unique_ptr<ISpeechSynthesizer> createSpeechSynthesizer(bool consoleOnly);

} // namespace bcg
