#include "vision_engine.h"

#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/kernels/register.h>
#include <tensorflow/lite/model.h>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace {

size_t tensor_element_count(const TfLiteTensor* tensor)
{
    if (!tensor || !tensor->dims) return 0;

    size_t count = 1;
    for (int i = 0; i < tensor->dims->size; ++i) {
        count *= static_cast<size_t>(tensor->dims->data[i]);
    }
    return count;
}

cv::Rect clamp_square_roi(
    const cv::Rect& face,
    const cv::Size& size,
    float scale = 1.45f
)
{
    const float cx = face.x + face.width * 0.5f;
    const float cy = face.y + face.height * 0.5f;
    const float side = std::max(face.width, face.height) * scale;

    int x = static_cast<int>(std::lround(cx - side * 0.5f));
    int y = static_cast<int>(std::lround(cy - side * 0.5f));
    int w = static_cast<int>(std::lround(side));
    int h = w;

    x = std::max(0, x);
    y = std::max(0, y);
    w = std::min(w, size.width - x);
    h = std::min(h, size.height - y);

    const int final_side = std::max(1, std::min(w, h));
    return cv::Rect(x, y, final_side, final_side);
}

}

VisionEngine::VisionEngine(const std::string& model_path)
{
    model_ = tflite::FlatBufferModel::BuildFromFile(model_path.c_str());
    if (!model_) {
        throw std::runtime_error("TFLite: impossible de charger le modèle: " + model_path);
    }

    tflite::ops::builtin::BuiltinOpResolver resolver;
    tflite::InterpreterBuilder builder(*model_, resolver);

    if (builder(&interpreter_) != kTfLiteOk || !interpreter_) {
        throw std::runtime_error("TFLite: création de l'interpréteur impossible.");
    }

    if (interpreter_->AllocateTensors() != kTfLiteOk) {
        throw std::runtime_error("TFLite: AllocateTensors a échoué.");
    }

    if (interpreter_->inputs().size() != 1) {
        throw std::runtime_error("TFLite: le modèle doit exposer exactement une entrée image.");
    }

    input_index_ = interpreter_->inputs()[0];
    const TfLiteTensor* input = interpreter_->tensor(input_index_);

    if (!input || !input->dims || input->dims->size != 4) {
        throw std::runtime_error("TFLite: forme d'entrée inattendue.");
    }

    input_height_ = input->dims->data[1];
    input_width_ = input->dims->data[2];
    input_channels_ = input->dims->data[3];

    if (input_width_ <= 0 || input_height_ <= 0 || input_channels_ != 3) {
        throw std::runtime_error("TFLite: entrée image invalide.");
    }

    const char* env_cascade = std::getenv("VISIONIA_HAAR_CASCADE");
    const std::vector<std::string> cascade_candidates = {
        env_cascade ? env_cascade : "",
        "/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
        "/usr/share/opencv/haarcascades/haarcascade_frontalface_default.xml"
    };

    for (const auto& candidate : cascade_candidates) {
        if (candidate.empty()) continue;
        if (detector_.load(candidate)) {
            detector_ready_ = true;
            break;
        }
    }
}

VisionEngine::~VisionEngine() = default;

cv::Rect VisionEngine::select_face_roi(const cv::Mat& image)
{
    if (image.empty()) {
        throw std::invalid_argument("VisionEngine: image vide.");
    }

    if (detector_ready_) {
        cv::Mat gray;
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
        cv::equalizeHist(gray, gray);

        std::vector<cv::Rect> faces;
        detector_.detectMultiScale(
            gray,
            faces,
            1.1,
            3,
            0,
            cv::Size(40, 40)
        );

        if (!faces.empty()) {
            const auto largest = std::max_element(
                faces.begin(),
                faces.end(),
                [](const cv::Rect& a, const cv::Rect& b) {
                    return a.area() < b.area();
                }
            );

            return clamp_square_roi(*largest, image.size());
        }
    }

    const int side = std::min(image.cols, image.rows);
    return cv::Rect(
        (image.cols - side) / 2,
        (image.rows - side) / 2,
        side,
        side
    );
}

std::vector<VisionEngine::Landmark>
VisionEngine::process_frame(const std::string& image_path)
{
    cv::Mat image = cv::imread(image_path, cv::IMREAD_COLOR);

    if (image.empty()) {
        throw std::runtime_error("VisionEngine: image illisible: " + image_path);
    }

    const cv::Rect roi = select_face_roi(image);
    cv::Mat cropped = image(roi);

    cv::Mat rgb;
    cv::cvtColor(cropped, rgb, cv::COLOR_BGR2RGB);

    cv::Mat resized;
    cv::resize(
        rgb,
        resized,
        cv::Size(input_width_, input_height_),
        0.0,
        0.0,
        cv::INTER_LINEAR
    );

    TfLiteTensor* input = interpreter_->tensor(input_index_);
    const size_t expected_values =
        static_cast<size_t>(input_width_) *
        static_cast<size_t>(input_height_) *
        static_cast<size_t>(input_channels_);

    if (tensor_element_count(input) != expected_values) {
        throw std::runtime_error("TFLite: taille d'entrée incohérente.");
    }

    if (input->type == kTfLiteFloat32) {
        float* dst = interpreter_->typed_tensor<float>(input_index_);
        if (!dst) throw std::runtime_error("TFLite: buffer float d'entrée indisponible.");

        for (int y = 0; y < resized.rows; ++y) {
            const cv::Vec3b* row = resized.ptr<cv::Vec3b>(y);
            for (int x = 0; x < resized.cols; ++x) {
                const size_t base =
                    (static_cast<size_t>(y) * resized.cols + x) * 3;
                dst[base + 0] = row[x][0] / 255.0f;
                dst[base + 1] = row[x][1] / 255.0f;
                dst[base + 2] = row[x][2] / 255.0f;
            }
        }
    }
    else if (input->type == kTfLiteUInt8) {
        uint8_t* dst = interpreter_->typed_tensor<uint8_t>(input_index_);
        if (!dst) throw std::runtime_error("TFLite: buffer uint8 d'entrée indisponible.");
        std::memcpy(dst, resized.data, expected_values);
    }
    else {
        throw std::runtime_error("TFLite: type d'entrée non pris en charge.");
    }

    if (interpreter_->Invoke() != kTfLiteOk) {
        throw std::runtime_error("TFLite: Invoke a échoué.");
    }

    const TfLiteTensor* landmark_tensor = nullptr;

    for (int output_index : interpreter_->outputs()) {
        const TfLiteTensor* tensor = interpreter_->tensor(output_index);
        if (
            tensor &&
            tensor->type == kTfLiteFloat32 &&
            tensor_element_count(tensor) >= 468u * 3u
        ) {
            landmark_tensor = tensor;
            break;
        }
    }

    if (!landmark_tensor) {
        throw std::runtime_error("TFLite: tenseur de landmarks 468x3 introuvable.");
    }

    const float* raw = landmark_tensor->data.f;
    if (!raw) {
        throw std::runtime_error("TFLite: sortie landmarks vide.");
    }

    std::vector<Landmark> landmarks;
    landmarks.reserve(468);

    const float image_width = static_cast<float>(image.cols);
    const float image_height = static_cast<float>(image.rows);

    for (size_t i = 0; i < 468; ++i) {
        const float local_x = raw[i * 3 + 0] / static_cast<float>(input_width_);
        const float local_y = raw[i * 3 + 1] / static_cast<float>(input_height_);
        const float local_z = raw[i * 3 + 2] / static_cast<float>(input_width_);

        const float x =
            (static_cast<float>(roi.x) + local_x * roi.width) /
            image_width;

        const float y =
            (static_cast<float>(roi.y) + local_y * roi.height) /
            image_height;

        const float z =
            local_z *
            static_cast<float>(roi.width) /
            image_width;

        landmarks.push_back({x, y, z});
    }

    return landmarks;
}
