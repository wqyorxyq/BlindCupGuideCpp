#include "blind_cup_guide/OpenCvDnnObjectDetector.h"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace bcg {
namespace {

cv::Rect clampBox(const cv::Rect& box, const cv::Size& frameSize) {
    const cv::Rect frameRect{0, 0, frameSize.width, frameSize.height};
    return box & frameRect;
}

} // namespace

OpenCvDnnObjectDetector::OpenCvDnnObjectDetector(DnnDetectorOptions options)
    : options_(std::move(options)),
      labels_(loadLabels(options_.labelsPath)),
      net_(cv::dnn::readNetFromONNX(options_.modelPath)) {
    if (labels_.empty()) {
        labels_ = {"cup", "hand", "other"};
    }
    if (net_.empty()) {
        throw std::runtime_error("Failed to load ONNX model: " + options_.modelPath);
    }
}

std::vector<Detection> OpenCvDnnObjectDetector::detect(const cv::Mat& frame) {
    cv::Mat blob;
    cv::dnn::blobFromImage(frame, blob, 1.0 / 255.0, options_.inputSize, cv::Scalar{}, true, false);
    net_.setInput(blob);

    std::vector<cv::Mat> outputs;
    net_.forward(outputs, net_.getUnconnectedOutLayersNames());

    std::vector<cv::Rect> boxes;
    std::vector<float> confidences;
    std::vector<int> classIds;

    for (const auto& rawOutput : outputs) {
        cv::Mat output;
        rawOutput.convertTo(output, CV_32F);
        const cv::Mat rows = outputToRows(output);

        for (int rowIndex = 0; rowIndex < rows.rows; ++rowIndex) {
            const float* row = rows.ptr<float>(rowIndex);
            const int cols = rows.cols;
            if (cols < 6) {
                continue;
            }

            const bool yoloV8Like = static_cast<int>(labels_.size()) + 4 == cols;
            const int classStart = yoloV8Like ? 4 : 5;
            const float objectness = yoloV8Like ? 1.0F : row[4];
            if (objectness < options_.confidenceThreshold * 0.5F) {
                continue;
            }

            int bestClass = -1;
            float bestScore = 0.0F;
            for (int classIndex = classStart; classIndex < cols; ++classIndex) {
                if (row[classIndex] > bestScore) {
                    bestScore = row[classIndex];
                    bestClass = classIndex - classStart;
                }
            }

            const float confidence = objectness * bestScore;
            if (bestClass < 0 || confidence < options_.confidenceThreshold) {
                continue;
            }

            const auto box = clampBox(toFrameBox(row, frame.size()), frame.size());
            if (box.empty()) {
                continue;
            }

            boxes.push_back(box);
            confidences.push_back(confidence);
            classIds.push_back(bestClass);
        }
    }

    std::vector<int> keptIndexes;
    cv::dnn::NMSBoxes(boxes, confidences, options_.confidenceThreshold,
                      options_.nmsThreshold, keptIndexes);

    std::vector<Detection> detections;
    detections.reserve(keptIndexes.size());
    for (int keptIndex : keptIndexes) {
        const auto classId = classIds[keptIndex];
        const std::string label = classId >= 0 && classId < static_cast<int>(labels_.size())
                                      ? labels_[classId]
                                      : "class_" + std::to_string(classId);

        detections.push_back(Detection{
            label,
            roleFromLabel(label),
            confidences[keptIndex],
            boxes[keptIndex],
        });
    }

    return detections;
}

std::vector<std::string> OpenCvDnnObjectDetector::loadLabels(const std::string& labelsPath) {
    std::ifstream input(labelsPath);
    if (!input) {
        return {};
    }

    std::vector<std::string> labels;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            labels.push_back(line);
        }
    }
    return labels;
}

cv::Mat OpenCvDnnObjectDetector::outputToRows(const cv::Mat& output) {
    if (output.dims == 2) {
        return output;
    }

    if (output.dims == 3) {
        const int dim1 = output.size[1];
        const int dim2 = output.size[2];
        cv::Mat matrix(dim1, dim2, CV_32F, const_cast<float*>(output.ptr<float>()));
        if (dim1 < dim2) {
            cv::Mat transposed;
            cv::transpose(matrix, transposed);
            return transposed;
        }
        return matrix;
    }

    const int attributes = output.size[output.dims - 1];
    const int rows = static_cast<int>(output.total()) / attributes;
    return output.reshape(1, rows);
}

cv::Rect OpenCvDnnObjectDetector::toFrameBox(const float* row, const cv::Size& frameSize) const {
    float centerX = row[0];
    float centerY = row[1];
    float width = row[2];
    float height = row[3];

    const bool normalized = centerX <= 1.5F && centerY <= 1.5F && width <= 1.5F && height <= 1.5F;
    const float scaleX = normalized ? static_cast<float>(frameSize.width)
                                    : static_cast<float>(frameSize.width) /
                                          static_cast<float>(options_.inputSize.width);
    const float scaleY = normalized ? static_cast<float>(frameSize.height)
                                    : static_cast<float>(frameSize.height) /
                                          static_cast<float>(options_.inputSize.height);

    const int left = static_cast<int>((centerX - width * 0.5F) * scaleX);
    const int top = static_cast<int>((centerY - height * 0.5F) * scaleY);
    const int boxWidth = static_cast<int>(width * scaleX);
    const int boxHeight = static_cast<int>(height * scaleY);

    return {left, top, boxWidth, boxHeight};
}

} // namespace bcg
