#include "quality_control.h"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <stdexcept>

namespace x32nx {

double get_ssim(
    const cv::Mat& source,
    const cv::Mat& inferred
)
{
    if (
        source.empty() ||
        inferred.empty()
    ) {
        throw std::invalid_argument(
            "SSIM requires non-empty images"
        );
    }

    if (
        source.size() != inferred.size() ||
        source.type() != inferred.type()
    ) {
        throw std::invalid_argument(
            "SSIM images must have identical size and type"
        );
    }

    cv::Mat i1;
    cv::Mat i2;

    source.convertTo(i1, CV_32F);
    inferred.convertTo(i2, CV_32F);

    const cv::Mat i1_2 = i1.mul(i1);
    const cv::Mat i2_2 = i2.mul(i2);
    const cv::Mat i1_i2 = i1.mul(i2);

    cv::Mat mu1;
    cv::Mat mu2;

    cv::GaussianBlur(
        i1,
        mu1,
        cv::Size(11, 11),
        1.5
    );

    cv::GaussianBlur(
        i2,
        mu2,
        cv::Size(11, 11),
        1.5
    );

    const cv::Mat mu1_2 =
        mu1.mul(mu1);

    const cv::Mat mu2_2 =
        mu2.mul(mu2);

    const cv::Mat mu1_mu2 =
        mu1.mul(mu2);

    cv::Mat sigma1_2;
    cv::Mat sigma2_2;
    cv::Mat sigma12;

    cv::GaussianBlur(
        i1_2,
        sigma1_2,
        cv::Size(11, 11),
        1.5
    );

    sigma1_2 -= mu1_2;

    cv::GaussianBlur(
        i2_2,
        sigma2_2,
        cv::Size(11, 11),
        1.5
    );

    sigma2_2 -= mu2_2;

    cv::GaussianBlur(
        i1_i2,
        sigma12,
        cv::Size(11, 11),
        1.5
    );

    sigma12 -= mu1_mu2;

    constexpr double c1 = 6.5025;
    constexpr double c2 = 58.5225;

    cv::Mat numerator =
        (2 * mu1_mu2 + c1)
            .mul(
                2 * sigma12 + c2
            );

    cv::Mat denominator =
        (mu1_2 + mu2_2 + c1)
            .mul(
                sigma1_2 +
                sigma2_2 +
                c2
            );

    cv::Mat ssim_map;

    cv::divide(
        numerator,
        denominator,
        ssim_map
    );

    const cv::Scalar mean =
        cv::mean(ssim_map);

    const int channels =
        source.channels();

    double total = 0.0;

    for (
        int channel = 0;
        channel < std::min(channels, 4);
        ++channel
    ) {
        total += mean[channel];
    }

    return total /
        static_cast<double>(
            std::max(1, std::min(channels, 4))
        );
}

bool should_request_full_keyframe(
    const cv::Mat& source,
    const cv::Mat& inferred,
    double minimum_ssim
)
{
    return get_ssim(
        source,
        inferred
    ) < minimum_ssim;
}

}
