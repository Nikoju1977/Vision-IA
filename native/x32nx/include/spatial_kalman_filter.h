#pragma once

#include <opencv2/core.hpp>
#include <opencv2/video/tracking.hpp>

#include <stdexcept>
#include <vector>

namespace x32nx {

class SpatialKalmanFilter {
public:
    explicit SpatialKalmanFilter(size_t points_count)
        : filters_(points_count),
          initialized_(points_count, false)
    {
        for (auto& kf : filters_) {
            kf.init(6, 3, 0, CV_32F);

            kf.transitionMatrix =
                (cv::Mat_<float>(6, 6) <<
                    1,0,0,1,0,0,
                    0,1,0,0,1,0,
                    0,0,1,0,0,1,
                    0,0,0,1,0,0,
                    0,0,0,0,1,0,
                    0,0,0,0,0,1);

            kf.measurementMatrix =
                (cv::Mat_<float>(3, 6) <<
                    1,0,0,0,0,0,
                    0,1,0,0,0,0,
                    0,0,1,0,0,0);

            cv::setIdentity(
                kf.processNoiseCov,
                cv::Scalar::all(1e-4)
            );

            cv::setIdentity(
                kf.measurementNoiseCov,
                cv::Scalar::all(1e-2)
            );

            cv::setIdentity(
                kf.errorCovPost,
                cv::Scalar::all(1.0)
            );
        }
    }

    std::vector<cv::Point3f> smooth_mesh(
        const std::vector<cv::Point3f>& measurements
    )
    {
        if (measurements.size() != filters_.size()) {
            throw std::invalid_argument(
                "Kalman: unexpected number of points"
            );
        }

        std::vector<cv::Point3f> out;
        out.reserve(measurements.size());

        for (size_t i = 0; i < measurements.size(); ++i) {
            auto& kf = filters_[i];
            const auto& p = measurements[i];

            if (!initialized_[i]) {
                kf.statePost =
                    (cv::Mat_<float>(6, 1) <<
                        p.x, p.y, p.z,
                        0.f, 0.f, 0.f);

                initialized_[i] = true;
            }

            kf.predict();

            const cv::Mat measurement =
                (cv::Mat_<float>(3, 1) <<
                    p.x, p.y, p.z);

            const cv::Mat estimated =
                kf.correct(measurement);

            out.emplace_back(
                estimated.at<float>(0),
                estimated.at<float>(1),
                estimated.at<float>(2)
            );
        }

        return out;
    }

private:
    std::vector<cv::KalmanFilter> filters_;
    std::vector<bool> initialized_;
};

}
