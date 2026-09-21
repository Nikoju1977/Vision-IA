#pragma once

#include <cstdint>
#include <opencv2/core.hpp>
#include <vector>

#include "rvq_vocoder.h"

namespace x32nx {

class VaseCommunicantMultiplexer {
public:
    static constexpr int MAX_BITRATE_BPS = 32000;
    static constexpr int FPS = 30;
    static constexpr int BYTES_PER_FRAME =
        (MAX_BITRATE_BPS / 8) / FPS;

    struct MultiplexedPacket {
        uint8_t allocation_ratio = 0;
        std::vector<uint8_t> video_payload;
        std::vector<uint8_t> audio_payload;
    };

    MultiplexedPacket process_frame(
        const std::vector<cv::Point3f>& current_mesh,
        const std::vector<cv::Point3f>& previous_mesh,
        const std::vector<float>& audio_embedding,
        const ResidualVectorQuantizer& rvq
    ) const;

private:
    static float kinetic_energy(
        const std::vector<cv::Point3f>& current,
        const std::vector<cv::Point3f>& previous
    );

    static std::vector<uint8_t> encode_semantic_mesh(
        const std::vector<cv::Point3f>& current,
        const std::vector<cv::Point3f>& previous,
        int byte_budget
    );
};

}
