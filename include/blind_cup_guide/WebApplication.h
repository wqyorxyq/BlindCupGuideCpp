#pragma once

#include "blind_cup_guide/DetectionTypes.h"
#include "blind_cup_guide/GuidanceEngine.h"
#include "blind_cup_guide/LocalHttpServer.h"
#include "blind_cup_guide/SpeechSynthesizer.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace bcg {

struct WebAppSettings {
    int cameraIndex = 0;
    bool useMockDetector = false;
    bool speechEnabled = true;
    bool consoleSpeech = false;
    bool keepRunningAfterSuccess = true;
    int inferenceIntervalMs = 650;
    int previewIntervalMs = 220;
    int previewMaxWidth = 960;
    int jpegQuality = 68;
    int openCvThreads = 2;
    float confidenceThreshold = 0.35F;
    std::filesystem::path modelPath = "models/cup_detector.onnx";
    std::filesystem::path labelsPath = "models/labels.txt";
};

struct WebAppOptions {
    int port = 8080;
    bool openBrowser = true;
    WebAppSettings initialSettings;
};

class WebApplication {
public:
    explicit WebApplication(WebAppOptions options);
    ~WebApplication();

    int run();

private:
    struct RuntimeState {
        bool running = false;
        bool modelLoaded = false;
        bool success = false;
        int frameWidth = 0;
        int frameHeight = 0;
        std::uint64_t frameCount = 0;
        std::string mode = "stopped";
        std::string error;
        Instruction instruction;
        std::vector<Detection> detections;
        std::chrono::steady_clock::time_point updatedAt{};
    };

    HttpResponse route(const HttpRequest& request);
    HttpResponse statusResponse();
    HttpResponse snapshotResponse();
    HttpResponse updateSettingsResponse(const HttpRequest& request, bool startAfterUpdate);
    HttpResponse startResponse(const HttpRequest& request);
    HttpResponse stopResponse();
    HttpResponse testSpeechResponse();

    void startCamera();
    void stopCamera();
    void cameraLoop(WebAppSettings settings);
    void speakIfNeeded(const Instruction& instruction,
                       ISpeechSynthesizer& speech,
                       bool speechEnabled);
    void applySettingsForm(const std::map<std::string, std::string>& form);
    std::string buildStatusJson();
    std::vector<unsigned char> latestJpegOrPlaceholder();

    WebAppOptions options_;
    WebAppSettings settings_;
    RuntimeState state_;
    std::vector<unsigned char> latestJpeg_;
    std::mutex mutex_;
    std::thread cameraThread_;
    std::atomic<bool> cameraRequested_{false};
    InstructionKind lastInstruction_ = InstructionKind::None;
    std::chrono::steady_clock::time_point lastSpeechTime_{};
    std::unique_ptr<LocalHttpServer> server_;
};

} // namespace bcg
