#include "blind_cup_guide/SpeechSynthesizer.h"

#include <iostream>
#include <stdexcept>

#if defined(_WIN32) && defined(BCG_ENABLE_SAPI)
#define NOMINMAX
#include <Windows.h>
#include <sapi.h>
#endif

namespace bcg {
namespace {

class ConsoleSpeechSynthesizer final : public ISpeechSynthesizer {
public:
    void speak(const std::string& text) override {
        std::cout << "[speech] " << text << '\n';
    }
};

#if defined(_WIN32) && defined(BCG_ENABLE_SAPI)

std::wstring utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return {};
    }

    const int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, text.data(),
                                               static_cast<int>(text.size()), nullptr, 0);
    if (sizeNeeded <= 0) {
        return {};
    }

    std::wstring wide(static_cast<std::size_t>(sizeNeeded), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(),
                        sizeNeeded);
    return wide;
}

class WindowsSapiSpeechSynthesizer final : public ISpeechSynthesizer {
public:
    WindowsSapiSpeechSynthesizer() {
        const HRESULT coInitResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        comInitialized_ = SUCCEEDED(coInitResult);
        if (FAILED(coInitResult) && coInitResult != RPC_E_CHANGED_MODE) {
            throw std::runtime_error("CoInitializeEx failed.");
        }

        const HRESULT voiceResult = CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_ALL,
                                                     IID_ISpVoice,
                                                     reinterpret_cast<void**>(&voice_));
        if (FAILED(voiceResult) || voice_ == nullptr) {
            throw std::runtime_error("Windows SAPI voice creation failed.");
        }
    }

    ~WindowsSapiSpeechSynthesizer() override {
        if (voice_) {
            voice_->Release();
            voice_ = nullptr;
        }
        if (comInitialized_) {
            CoUninitialize();
        }
    }

    void speak(const std::string& text) override {
        const auto wide = utf8ToWide(text);
        if (wide.empty()) {
            return;
        }
        voice_->Speak(wide.c_str(), SPF_ASYNC | SPF_PURGEBEFORESPEAK, nullptr);
    }

private:
    bool comInitialized_ = false;
    ISpVoice* voice_ = nullptr;
};

#endif

} // namespace

std::unique_ptr<ISpeechSynthesizer> createSpeechSynthesizer(bool consoleOnly) {
    if (consoleOnly) {
        return std::make_unique<ConsoleSpeechSynthesizer>();
    }

#if defined(_WIN32) && defined(BCG_ENABLE_SAPI)
    try {
        return std::make_unique<WindowsSapiSpeechSynthesizer>();
    } catch (const std::exception& error) {
        std::cerr << "SAPI unavailable, falling back to console speech: " << error.what() << '\n';
    }
#endif

    return std::make_unique<ConsoleSpeechSynthesizer>();
}

} // namespace bcg
