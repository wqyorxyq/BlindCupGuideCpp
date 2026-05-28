#include "blind_cup_guide/CameraSource.h"

namespace bcg {

bool CameraSource::open(int cameraIndex) {
    capture_.open(cameraIndex, cv::CAP_ANY);
    if (!capture_.isOpened()) {
        return false;
    }

    capture_.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    capture_.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
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
