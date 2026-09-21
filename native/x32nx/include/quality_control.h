#pragma once

#include <opencv2/core.hpp>

namespace x32nx {

double get_ssim(
    const cv::Mat& source,
    const cv::Mat& inferred
);

bool should_request_full_keyframe(
    const cv::Mat& source,
    const cv::Mat& inferred,
    double minimum_ssim = 0.85
);

}
