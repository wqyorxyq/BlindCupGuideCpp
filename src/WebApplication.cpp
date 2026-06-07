#include "blind_cup_guide/WebApplication.h"

#include "blind_cup_guide/CameraSource.h"
#include "blind_cup_guide/FrameAnnotator.h"
#include "blind_cup_guide/IObjectDetector.h"
#include "blind_cup_guide/MockObjectDetector.h"
#include "blind_cup_guide/OpenCvDnnObjectDetector.h"
#include "blind_cup_guide/WebAssets.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <utility>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <Shellapi.h>
#endif

namespace bcg {
namespace {

HttpResponse textResponse(std::string body, std::string contentType) {
    HttpResponse response;
    response.body = std::move(body);
    response.contentType = std::move(contentType);
    return response;
}

HttpResponse jsonResponse(std::string body) {
    return textResponse(std::move(body), "application/json; charset=utf-8");
}

HttpResponse notFound() {
    HttpResponse response;
    response.status = 404;
    response.body = "Not found";
    return response;
}

bool formBool(const std::map<std::string, std::string>& form,
              const std::string& key,
              bool currentValue) {
    const auto found = form.find(key);
    if (found == form.end()) {
        return currentValue;
    }

    const auto& value = found->second;
    return value == "true" || value == "1" || value == "on" || value == "yes";
}

int formInt(const std::map<std::string, std::string>& form,
            const std::string& key,
            int currentValue) {
    const auto found = form.find(key);
    if (found == form.end() || found->second.empty()) {
        return currentValue;
    }
    return std::stoi(found->second);
}

float formFloat(const std::map<std::string, std::string>& form,
                const std::string& key,
                float currentValue) {
    const auto found = form.find(key);
    if (found == form.end() || found->second.empty()) {
        return currentValue;
    }
    return std::stof(found->second);
}

std::filesystem::path formPath(const std::map<std::string, std::string>& form,
                               const std::string& key,
                               const std::filesystem::path& currentValue) {
    const auto found = form.find(key);
    if (found == form.end() || found->second.empty()) {
        return currentValue;
    }
    return found->second;
}

std::string jsonEscape(const std::string& text) {
    std::ostringstream output;
    for (const unsigned char ch : text) {
        switch (ch) {
        case '\\':
            output << "\\\\";
            break;
        case '"':
            output << "\\\"";
            break;
        case '\n':
            output << "\\n";
            break;
        case '\r':
            output << "\\r";
            break;
        case '\t':
            output << "\\t";
            break;
        default:
            if (ch < 0x20) {
                output << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                       << static_cast<int>(ch);
            } else {
                output << static_cast<char>(ch);
            }
            break;
        }
    }
    return output.str();
}

std::unique_ptr<IObjectDetector> createDetector(const WebAppSettings& settings,
                                                bool& modelLoaded) {
    modelLoaded = false;
    if (settings.useMockDetector || !std::filesystem::exists(settings.modelPath)) {
        return std::make_unique<MockObjectDetector>();
    }

    DnnDetectorOptions detectorOptions;
    detectorOptions.modelPath = settings.modelPath.string();
    detectorOptions.labelsPath = settings.labelsPath.string();
    detectorOptions.confidenceThreshold = settings.confidenceThreshold;
    modelLoaded = true;
    return std::make_unique<OpenCvDnnObjectDetector>(std::move(detectorOptions));
}

std::string modeText(const WebAppSettings& settings, bool modelLoaded) {
    if (settings.useMockDetector) {
        return "mock";
    }
    return modelLoaded ? "model" : "mock-no-model";
}

void openBrowserUrl(const std::string& url) {
#ifdef _WIN32
    ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
    (void)url;
#endif
}

std::vector<unsigned char> buildPlaceholderJpeg(const std::string& message) {
    cv::Mat image(540, 960, CV_8UC3, cv::Scalar{20, 22, 18});
    cv::putText(image, "Blind Cup Guide", cv::Point{38, 72}, cv::FONT_HERSHEY_SIMPLEX, 1.2,
                cv::Scalar{230, 236, 226}, 2);
    cv::putText(image, message, cv::Point{38, 132}, cv::FONT_HERSHEY_SIMPLEX, 0.82,
                cv::Scalar{186, 196, 181}, 2);
    std::vector<unsigned char> encoded;
    cv::imencode(".jpg", image, encoded, {cv::IMWRITE_JPEG_QUALITY, 82});
    return encoded;
}

cv::Mat previewFrame(const cv::Mat& frame, int maxWidth) {
    if (maxWidth <= 0 || frame.cols <= maxWidth) {
        return frame;
    }

    const double scale = static_cast<double>(maxWidth) / static_cast<double>(frame.cols);
    cv::Mat resized;
    cv::resize(frame, resized, cv::Size{}, scale, scale, cv::INTER_AREA);
    return resized;
}

} // namespace

WebApplication::WebApplication(WebAppOptions options)
    : options_(std::move(options)),
      settings_(options_.initialSettings) {}

WebApplication::~WebApplication() {
    stopCamera();
    if (server_) {
        server_->stop();
    }
}

int WebApplication::run() {
    server_ = std::make_unique<LocalHttpServer>(
        options_.port, [this](const HttpRequest& request) { return route(request); });
    server_->start();

    const auto url = "http://127.0.0.1:" + std::to_string(options_.port) + "/";
    std::cout << "Web interface: " << url << '\n';
    std::cout << "Press Enter in this console to quit.\n";

    if (options_.openBrowser) {
        openBrowserUrl(url);
    }

    std::string line;
    if (!std::getline(std::cin, line)) {
        while (server_->isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds{1});
        }
    }
    stopCamera();
    server_->stop();
    return 0;
}

HttpResponse WebApplication::route(const HttpRequest& request) {
    if (request.method == "GET" && request.path == "/") {
        return textResponse(web_assets::indexHtml(), "text/html; charset=utf-8");
    }
    if (request.method == "GET" && request.path == "/styles.css") {
        return textResponse(web_assets::stylesCss(), "text/css; charset=utf-8");
    }
    if (request.method == "GET" && request.path == "/app.js") {
        return textResponse(web_assets::appJs(), "application/javascript; charset=utf-8");
    }
    if (request.method == "GET" && request.path == "/status") {
        return statusResponse();
    }
    if (request.method == "GET" && request.path == "/snapshot.jpg") {
        return snapshotResponse();
    }
    if (request.method == "POST" && request.path == "/api/start") {
        return startResponse(request);
    }
    if (request.method == "POST" && request.path == "/api/stop") {
        return stopResponse();
    }
    if (request.method == "POST" && request.path == "/api/settings") {
        return updateSettingsResponse(request, false);
    }
    if (request.method == "POST" && request.path == "/api/test-speech") {
        return testSpeechResponse();
    }
    return notFound();
}

HttpResponse WebApplication::statusResponse() {
    return jsonResponse(buildStatusJson());
}

HttpResponse WebApplication::snapshotResponse() {
    auto jpeg = latestJpegOrPlaceholder();
    HttpResponse response;
    response.contentType = "image/jpeg";
    response.body.assign(reinterpret_cast<const char*>(jpeg.data()), jpeg.size());
    return response;
}

HttpResponse WebApplication::updateSettingsResponse(const HttpRequest& request,
                                                    bool startAfterUpdate) {
    bool wasRunning = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        wasRunning = state_.running;
    }

    if (wasRunning) {
        stopCamera();
    }

    applySettingsForm(parseUrlEncoded(request.body));

    if (startAfterUpdate || wasRunning) {
        startCamera();
    }

    return statusResponse();
}

HttpResponse WebApplication::startResponse(const HttpRequest& request) {
    return updateSettingsResponse(request, true);
}

HttpResponse WebApplication::stopResponse() {
    stopCamera();
    return statusResponse();
}

HttpResponse WebApplication::testSpeechResponse() {
    WebAppSettings settingsSnapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        settingsSnapshot = settings_;
    }

    auto speech = createSpeechSynthesizer(settingsSnapshot.consoleSpeech);
    speech->speak(u8"做对了！");
    std::this_thread::sleep_for(std::chrono::milliseconds{1300});
    return statusResponse();
}

void WebApplication::startCamera() {
    if (cameraThread_.joinable() && !cameraRequested_) {
        cameraThread_.join();
    }

    WebAppSettings settingsSnapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_.running) {
            return;
        }
        settingsSnapshot = settings_;
        state_.running = true;
        state_.success = false;
        state_.error.clear();
        state_.mode = "starting";
        state_.instruction = {InstructionKind::None, ""};
        state_.updatedAt = std::chrono::steady_clock::now();
        latestJpeg_.clear();
    }

    lastInstruction_ = InstructionKind::None;
    lastSpeechTime_ = {};
    cameraRequested_ = true;
    cameraThread_ = std::thread(&WebApplication::cameraLoop, this, settingsSnapshot);
}

void WebApplication::stopCamera() {
    cameraRequested_ = false;
    if (cameraThread_.joinable()) {
        cameraThread_.join();
    }

    std::lock_guard<std::mutex> lock(mutex_);
    state_.running = false;
    if (state_.error.empty() && !state_.success) {
        state_.mode = "stopped";
    }
    state_.updatedAt = std::chrono::steady_clock::now();
}

void WebApplication::cameraLoop(WebAppSettings settings) {
    bool modelLoaded = false;
    std::unique_ptr<IObjectDetector> detector;
    try {
        detector = createDetector(settings, modelLoaded);
    } catch (const std::exception& error) {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.running = false;
        state_.mode = "error";
        state_.error = error.what();
        cameraRequested_ = false;
        return;
    }

    CameraSource camera;
    if (!camera.open(settings.cameraIndex)) {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.running = false;
        state_.mode = "error";
        state_.error = "Cannot open camera index " + std::to_string(settings.cameraIndex) + ".";
        cameraRequested_ = false;
        return;
    }

    auto speech = createSpeechSynthesizer(settings.consoleSpeech);
    GuidanceEngine guidance;
    std::uint64_t frameCount = 0;
    std::vector<Detection> detections;
    Instruction instruction{InstructionKind::None, ""};
    bool detectionReady = false;
    auto lastDetectionAt = std::chrono::steady_clock::time_point{};
    auto lastPreviewAt = std::chrono::steady_clock::time_point{};
    const auto inferenceInterval =
        std::chrono::milliseconds{std::max(100, settings.inferenceIntervalMs)};
    const auto previewInterval =
        std::chrono::milliseconds{std::max(100, settings.previewIntervalMs)};
    cv::setNumThreads(std::max(1, settings.openCvThreads));

    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_.running = true;
        state_.modelLoaded = modelLoaded;
        state_.mode = modeText(settings, modelLoaded);
        state_.error.clear();
        state_.updatedAt = std::chrono::steady_clock::now();
    }

    while (cameraRequested_) {
        auto frame = camera.readFrame();
        if (!frame) {
            std::lock_guard<std::mutex> lock(mutex_);
            state_.error = "Camera returned an empty frame.";
            state_.mode = "error";
            break;
        }

        const auto now = std::chrono::steady_clock::now();
        const bool shouldRunDetection =
            !detectionReady || lastDetectionAt == std::chrono::steady_clock::time_point{} ||
            now - lastDetectionAt >= inferenceInterval;

        if (shouldRunDetection) {
            detections = detector->detect(*frame);
            instruction = guidance.evaluate(frame->size(), detections);
            speakIfNeeded(instruction, *speech, settings.speechEnabled);
            lastDetectionAt = std::chrono::steady_clock::now();
            detectionReady = true;
        }

        const bool shouldUpdatePreview =
            lastPreviewAt == std::chrono::steady_clock::time_point{} ||
            now - lastPreviewAt >= previewInterval;

        std::vector<unsigned char> encoded;
        if (shouldUpdatePreview) {
            cv::Mat annotated = frame->clone();
            drawDebugOverlay(annotated, detections, instruction);
            const cv::Mat preview = previewFrame(annotated, settings.previewMaxWidth);
            cv::imencode(".jpg", preview, encoded,
                         {cv::IMWRITE_JPEG_QUALITY, std::clamp(settings.jpegQuality, 35, 90)});
            lastPreviewAt = now;
        }

        ++frameCount;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!encoded.empty()) {
                latestJpeg_ = std::move(encoded);
            }
            state_.running = true;
            state_.success = instruction.kind == InstructionKind::Correct;
            state_.modelLoaded = modelLoaded;
            state_.mode = modeText(settings, modelLoaded);
            state_.frameWidth = frame->cols;
            state_.frameHeight = frame->rows;
            state_.frameCount = frameCount;
            state_.instruction = instruction;
            state_.detections = std::move(detections);
            state_.updatedAt = std::chrono::steady_clock::now();
        }

        if (instruction.kind == InstructionKind::Correct && !settings.keepRunningAfterSuccess) {
            cameraRequested_ = false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    std::lock_guard<std::mutex> lock(mutex_);
    state_.running = false;
    if (state_.error.empty()) {
        state_.mode = state_.success ? "success" : "stopped";
    }
    state_.updatedAt = std::chrono::steady_clock::now();
}

void WebApplication::speakIfNeeded(const Instruction& instruction,
                                   ISpeechSynthesizer& speech,
                                   bool speechEnabled) {
    if (!speechEnabled || instruction.kind == InstructionKind::None ||
        instruction.spokenText.empty()) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    const bool changed = instruction.kind != lastInstruction_;
    const bool expired = lastSpeechTime_ == std::chrono::steady_clock::time_point{} ||
                         now - lastSpeechTime_ >= std::chrono::milliseconds{1600};

    if (changed || expired) {
        speech.speak(instruction.spokenText);
        lastInstruction_ = instruction.kind;
        lastSpeechTime_ = now;
    }
}

void WebApplication::applySettingsForm(const std::map<std::string, std::string>& form) {
    std::lock_guard<std::mutex> lock(mutex_);
    settings_.cameraIndex = std::max(0, formInt(form, "cameraIndex", settings_.cameraIndex));
    settings_.useMockDetector = formBool(form, "useMockDetector", settings_.useMockDetector);
    settings_.speechEnabled = formBool(form, "speechEnabled", settings_.speechEnabled);
    settings_.consoleSpeech = formBool(form, "consoleSpeech", settings_.consoleSpeech);
    settings_.keepRunningAfterSuccess =
        formBool(form, "keepRunningAfterSuccess", settings_.keepRunningAfterSuccess);
    settings_.inferenceIntervalMs =
        std::clamp(formInt(form, "inferenceIntervalMs", settings_.inferenceIntervalMs), 100, 3000);
    settings_.previewIntervalMs =
        std::clamp(formInt(form, "previewIntervalMs", settings_.previewIntervalMs), 100, 3000);
    settings_.previewMaxWidth =
        std::clamp(formInt(form, "previewMaxWidth", settings_.previewMaxWidth), 320, 1280);
    settings_.jpegQuality =
        std::clamp(formInt(form, "jpegQuality", settings_.jpegQuality), 35, 90);
    settings_.openCvThreads =
        std::clamp(formInt(form, "openCvThreads", settings_.openCvThreads), 1, 8);
    settings_.confidenceThreshold =
        std::clamp(formFloat(form, "confidenceThreshold", settings_.confidenceThreshold), 0.05F,
                   0.95F);
    settings_.modelPath = formPath(form, "modelPath", settings_.modelPath);
    settings_.labelsPath = formPath(form, "labelsPath", settings_.labelsPath);
}

std::string WebApplication::buildStatusJson() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream json;
    json << "{";
    json << "\"running\":" << (state_.running ? "true" : "false") << ",";
    json << "\"modelLoaded\":" << (state_.modelLoaded ? "true" : "false") << ",";
    json << "\"success\":" << (state_.success ? "true" : "false") << ",";
    json << "\"mode\":\"" << jsonEscape(state_.mode) << "\",";
    json << "\"error\":\"" << jsonEscape(state_.error) << "\",";
    json << "\"instructionKind\":\"" << instructionKindName(state_.instruction.kind) << "\",";
    json << "\"instructionText\":\"" << jsonEscape(state_.instruction.spokenText) << "\",";
    json << "\"frameWidth\":" << state_.frameWidth << ",";
    json << "\"frameHeight\":" << state_.frameHeight << ",";
    json << "\"frameCount\":" << state_.frameCount << ",";
    json << "\"settings\":{";
    json << "\"cameraIndex\":" << settings_.cameraIndex << ",";
    json << "\"useMockDetector\":" << (settings_.useMockDetector ? "true" : "false") << ",";
    json << "\"speechEnabled\":" << (settings_.speechEnabled ? "true" : "false") << ",";
    json << "\"consoleSpeech\":" << (settings_.consoleSpeech ? "true" : "false") << ",";
    json << "\"keepRunningAfterSuccess\":"
         << (settings_.keepRunningAfterSuccess ? "true" : "false") << ",";
    json << "\"inferenceIntervalMs\":" << settings_.inferenceIntervalMs << ",";
    json << "\"previewIntervalMs\":" << settings_.previewIntervalMs << ",";
    json << "\"previewMaxWidth\":" << settings_.previewMaxWidth << ",";
    json << "\"jpegQuality\":" << settings_.jpegQuality << ",";
    json << "\"openCvThreads\":" << settings_.openCvThreads << ",";
    json << "\"confidenceThreshold\":" << settings_.confidenceThreshold << ",";
    json << "\"modelPath\":\"" << jsonEscape(settings_.modelPath.string()) << "\",";
    json << "\"labelsPath\":\"" << jsonEscape(settings_.labelsPath.string()) << "\"";
    json << "},";
    json << "\"detections\":[";
    for (std::size_t index = 0; index < state_.detections.size(); ++index) {
        const auto& detection = state_.detections[index];
        if (index > 0) {
            json << ",";
        }
        json << "{";
        json << "\"label\":\"" << jsonEscape(detection.label) << "\",";
        json << "\"role\":\"" << roleName(detection.role) << "\",";
        json << "\"confidence\":" << detection.confidence << ",";
        json << "\"x\":" << detection.box.x << ",";
        json << "\"y\":" << detection.box.y << ",";
        json << "\"width\":" << detection.box.width << ",";
        json << "\"height\":" << detection.box.height;
        json << "}";
    }
    json << "]";
    json << "}";
    return json.str();
}

std::vector<unsigned char> WebApplication::latestJpegOrPlaceholder() {
    std::vector<unsigned char> jpeg;
    std::string error;
    bool running = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        jpeg = latestJpeg_;
        error = state_.error;
        running = state_.running;
    }

    if (!jpeg.empty()) {
        return jpeg;
    }

    if (!error.empty()) {
        return buildPlaceholderJpeg(error);
    }
    return buildPlaceholderJpeg(running ? "Starting camera..." : "Camera stopped");
}

} // namespace bcg
