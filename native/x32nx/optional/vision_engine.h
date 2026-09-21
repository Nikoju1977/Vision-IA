#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/objdetect.hpp>

#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/model.h>

class VisionEngine {
public:
    using Landmark = std::array<float, 3>;

    explicit VisionEngine(const std::string& model_path);
    ~VisionEngine();

    VisionEngine(const VisionEngine&) = delete;
    VisionEngine& operator=(const VisionEngine&) = delete;

    std::vector<Landmark> process_frame(const std::string& image_path);

    int input_width() const noexcept { return input_width_; }
    int input_height() const noexcept { return input_height_; }
    bool face_detector_ready() const noexcept { return detector_ready_; }

private:
    cv::Rect select_face_roi(const cv::Mat& image);

    std::unique_ptr<tflite::FlatBufferModel> model_;
    std::unique_ptr<tflite::Interpreter> interpreter_;

    cv::CascadeClassifier detector_;
    bool detector_ready_ = false;

    int input_index_ = -1;
    int input_width_ = 0;
    int input_height_ = 0;
    int input_channels_ = 0;
};
