#include "blind_cup_guide/Application.h"
#include "blind_cup_guide/MockObjectDetector.h"
#include "blind_cup_guide/OpenCvDnnObjectDetector.h"
#include "blind_cup_guide/WebApplication.h"

#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

namespace {

struct CliOptions {
    int cameraIndex = 0;
    int webPort = 8080;
    bool webMode = true;
    bool openBrowser = true;
    bool displayWindow = true;
    bool consoleSpeech = false;
    bool forceMock = false;
    bool keepRunningAfterSuccess = false;
    float confidenceThreshold = 0.35F;
    std::filesystem::path modelPath = "models/cup_detector.onnx";
    std::filesystem::path labelsPath = "models/labels.txt";
};

void printUsage(const char* programName) {
    std::cout
        << "Usage: " << programName << " [options]\n\n"
        << "Options:\n"
        << "  --web                  Start the browser operation UI, default\n"
        << "  --cli                  Use the original OpenCV debug window mode\n"
        << "  --port <port>          Web UI port, default 8080\n"
        << "  --no-open-browser      Start the Web UI without opening a browser\n"
        << "  --camera <index>       Camera index, default 0\n"
        << "  --model <path>         ONNX detector path, default models/cup_detector.onnx\n"
        << "  --labels <path>        Label file path, default models/labels.txt\n"
        << "  --confidence <value>   Confidence threshold, default 0.35\n"
        << "  --mock                 Use simulated detections to test guidance flow\n"
        << "  --console-tts          Print speech text instead of using Windows SAPI\n"
        << "  --no-display           Do not show the debug camera window\n"
        << "  --keep-running         Continue after the cup is reached\n"
        << "  --help                 Show this help\n";
}

CliOptions parseArgs(int argc, char** argv) {
    CliOptions options;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto requireValue = [&](const std::string& name) -> std::string {
            if (i + 1 >= argc) {
                throw std::invalid_argument(name + " requires a value.");
            }
            return argv[++i];
        };

        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            std::exit(0);
        } else if (arg == "--web") {
            options.webMode = true;
        } else if (arg == "--cli") {
            options.webMode = false;
        } else if (arg == "--port") {
            options.webPort = std::stoi(requireValue(arg));
        } else if (arg == "--no-open-browser") {
            options.openBrowser = false;
        } else if (arg == "--camera") {
            options.cameraIndex = std::stoi(requireValue(arg));
        } else if (arg == "--model") {
            options.modelPath = requireValue(arg);
        } else if (arg == "--labels") {
            options.labelsPath = requireValue(arg);
        } else if (arg == "--confidence") {
            options.confidenceThreshold = std::stof(requireValue(arg));
        } else if (arg == "--mock") {
            options.forceMock = true;
        } else if (arg == "--console-tts") {
            options.consoleSpeech = true;
        } else if (arg == "--no-display") {
            options.displayWindow = false;
        } else if (arg == "--keep-running") {
            options.keepRunningAfterSuccess = true;
        } else {
            throw std::invalid_argument("Unknown option: " + arg);
        }
    }

    return options;
}

std::unique_ptr<bcg::IObjectDetector> createDetector(const CliOptions& options) {
    if (options.forceMock || !std::filesystem::exists(options.modelPath)) {
        if (!options.forceMock) {
            std::cout << "Model not found at " << options.modelPath
                      << "; using mock detector for now.\n";
        }
        return std::make_unique<bcg::MockObjectDetector>();
    }

    bcg::DnnDetectorOptions detectorOptions;
    detectorOptions.modelPath = options.modelPath.string();
    detectorOptions.labelsPath = options.labelsPath.string();
    detectorOptions.confidenceThreshold = options.confidenceThreshold;
    return std::make_unique<bcg::OpenCvDnnObjectDetector>(std::move(detectorOptions));
}

int runWebApplication(const CliOptions& cli) {
    bcg::WebAppOptions webOptions;
    webOptions.port = cli.webPort;
    webOptions.openBrowser = cli.openBrowser;
    webOptions.initialSettings.cameraIndex = cli.cameraIndex;
    webOptions.initialSettings.useMockDetector = cli.forceMock;
    webOptions.initialSettings.speechEnabled = true;
    webOptions.initialSettings.consoleSpeech = cli.consoleSpeech;
    webOptions.initialSettings.keepRunningAfterSuccess = true;
    webOptions.initialSettings.confidenceThreshold = cli.confidenceThreshold;
    webOptions.initialSettings.modelPath = cli.modelPath;
    webOptions.initialSettings.labelsPath = cli.labelsPath;

    bcg::WebApplication app{webOptions};
    return app.run();
}

} // namespace

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    try {
        const auto cli = parseArgs(argc, argv);
        if (cli.webMode) {
            return runWebApplication(cli);
        }

        bcg::AppOptions appOptions;
        appOptions.cameraIndex = cli.cameraIndex;
        appOptions.displayWindow = cli.displayWindow;
        appOptions.stopOnSuccess = !cli.keepRunningAfterSuccess;

        bcg::Application app{
            appOptions,
            createDetector(cli),
            bcg::createSpeechSynthesizer(cli.consoleSpeech),
        };
        return app.run();
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
