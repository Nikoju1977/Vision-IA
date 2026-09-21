#include <gtest/gtest.h>

#include <opencv2/core.hpp>

#include <string>
#include <vector>

#include "crypto_chacha.h"
#include "multiplexer.h"
#include "quality_control.h"
#include "rvq_vocoder.h"
#include "spatial_kalman_filter.h"

TEST(
    ChaCha20Poly1305,
    RoundTripAndAuthentication
)
{
    x32nx::ChaChaKey key{};

    for (
        size_t i = 0;
        i < key.size();
        ++i
    ) {
        key[i] =
            static_cast<unsigned char>(
                i + 1
            );
    }

    const std::string input =
        "Vision-IA X32-NX";

    const std::vector<uint8_t> aad = {
        0x58,
        0x32,
        0x01
    };

    const auto encrypted =
        x32nx::encrypt_protobuf(
            input,
            key,
            aad
        );

    std::string output;

    ASSERT_TRUE(
        x32nx::decrypt_and_verify(
            encrypted,
            key,
            output,
            aad
        )
    );

    EXPECT_EQ(output, input);

    auto altered = encrypted;
    altered.tag[0] ^= 0x01;

    EXPECT_FALSE(
        x32nx::decrypt_and_verify(
            altered,
            key,
            output,
            aad
        )
    );
}

TEST(
    QualityControl,
    IdenticalImagesHavePerfectSSIM
)
{
    cv::Mat image(
        64,
        64,
        CV_8UC1,
        cv::Scalar(127)
    );

    EXPECT_NEAR(
        x32nx::get_ssim(
            image,
            image
        ),
        1.0,
        1e-5
    );
}

TEST(
    SpatialKalman,
    PreservesPointCount
)
{
    x32nx::SpatialKalmanFilter filter(
        3
    );

    const std::vector<cv::Point3f> points = {
        {0.1f, 0.2f, 0.3f},
        {0.4f, 0.5f, 0.6f},
        {0.7f, 0.8f, 0.9f}
    };

    const auto output =
        filter.smooth_mesh(points);

    EXPECT_EQ(
        output.size(),
        points.size()
    );
}

TEST(
    RVQ,
    EncodesWithinByteBudget
)
{
    std::vector<x32nx::Codebook> books = {
        {{
            {0.f, 0.f},
            {1.f, 1.f}
        }},
        {{
            {0.f, 0.f},
            {0.25f, 0.25f}
        }}
    };

    x32nx::ResidualVectorQuantizer rvq(
        books
    );

    const auto indices =
        rvq.encode_dynamic_rvq(
            {0.9f, 0.9f},
            1
        );

    ASSERT_EQ(
        indices.size(),
        1u
    );

    EXPECT_EQ(
        indices[0],
        1u
    );
}

TEST(
    Multiplexer,
    HonorsFrameBudget
)
{
    std::vector<x32nx::Codebook> books = {
        {{
            {0.f, 0.f},
            {1.f, 1.f}
        }},
        {{
            {0.f, 0.f},
            {0.25f, 0.25f}
        }}
    };

    x32nx::ResidualVectorQuantizer rvq(
        books
    );

    x32nx::VaseCommunicantMultiplexer mux;

    std::vector<cv::Point3f> previous(
        20,
        cv::Point3f(0.f, 0.f, 0.f)
    );

    std::vector<cv::Point3f> current(
        20,
        cv::Point3f(0.1f, 0.f, 0.f)
    );

    const auto packet =
        mux.process_frame(
            current,
            previous,
            {0.9f, 0.9f},
            rvq
        );

    EXPECT_LE(
        packet.video_payload.size() +
            packet.audio_payload.size() +
            1,
        static_cast<size_t>(
            x32nx::
                VaseCommunicantMultiplexer::
                BYTES_PER_FRAME
        )
    );

    EXPECT_LE(
        packet.allocation_ratio,
        100
    );
}
