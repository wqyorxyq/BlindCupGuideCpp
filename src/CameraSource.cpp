#include "blind_cup_guide/CameraSource.h"

namespace bcg {

bool CameraSource::open(int cameraIndex, cv::Size frameSize) {
    capture_.open(cameraIndex, cv::CAP_ANY);
    if (!capture_.isOpened()) {
        return false;
    }

    capture_.set(cv::CAP_PROP_FRAME_WIDTH, frameSize.width);
    capture_.set(cv::CAP_PROP_FRAME_HEIGHT, frameSize.height);
    capture_.set(cv::CAP_PROP_FPS, 15);
    return true;
}

std::optional<cv::Mat> CameraSource::readFrame() {
    cv::Mat frame;
    if (!capture_.read(frame) || frame.empty()) {
        return std::nullopt;
    }
    return frame;
}

bool CameraSource::isOpened() const {
    return capture_.isOpened();
}

} // namespace bcg
