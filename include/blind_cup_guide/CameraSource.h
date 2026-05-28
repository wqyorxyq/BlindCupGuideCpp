#pragma once

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

#include <optional>

namespace bcg {

class CameraSource {
public:
    bool open(int cameraIndex);
    std::optional<cv::Mat> readFrame();
    bool isOpened() const;

private:
    cv::VideoCapture capture_;
};

} // namespace bcg
