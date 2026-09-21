#include "multiplexer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

namespace x32nx {
namespace {

int16_t quantize_delta(float value)
{
    const float scaled =
        std::clamp(value, -1.0f, 1.0f) * 32767.0f;

    return static_cast<int16_t>(
        std::lround(scaled)
    );
}

void push_u16(
    std::vector<uint8_t>& output,
    uint16_t value
)
{
    output.push_back(
        static_cast<uint8_t>((value >> 8) & 0xFF)
    );

    output.push_back(
        static_cast<uint8_t>(value & 0xFF)
    );
}

void push_i16(
    std::vector<uint8_t>& output,
    int16_t value
)
{
    push_u16(
        output,
        static_cast<uint16_t>(value)
    );
}

}

float VaseCommunicantMultiplexer::kinetic_energy(
    const std::vector<cv::Point3f>& current,
    const std::vector<cv::Point3f>& previous
)
{
    if (current.empty()) return 0.f;
    if (previous.size() != current.size()) return 1.f;

    float max_deviation = 0.f;

    for (size_t i = 0; i < current.size(); ++i) {
        const float dx =
            current[i].x - previous[i].x;
        const float dy =
            current[i].y - previous[i].y;
        const float dz =
            current[i].z - previous[i].z;

        max_deviation =
            std::max(
                max_deviation,
                std::sqrt(
                    dx * dx +
                    dy * dy +
                    dz * dz
                )
            );
    }

    return max_deviation;
}

std::vector<uint8_t>
VaseCommunicantMultiplexer::encode_semantic_mesh(
    const std::vector<cv::Point3f>& current,
    const std::vector<cv::Point3f>& previous,
    int byte_budget
)
{
    if (current.empty() || byte_budget < 10) {
        return {};
    }

    struct Candidate {
        size_t index;
        float energy;
        cv::Point3f delta;
    };

    std::vector<Candidate> candidates;
    candidates.reserve(current.size());

    const bool has_reference =
        previous.size() == current.size();

    for (size_t i = 0; i < current.size(); ++i) {
        const cv::Point3f base =
            has_reference
                ? previous[i]
                : cv::Point3f{};

        const cv::Point3f delta =
            current[i] - base;

        const float energy =
            delta.x * delta.x +
            delta.y * delta.y +
            delta.z * delta.z;

        candidates.push_back({
            i,
            energy,
            delta
        });
    }

    std::sort(
        candidates.begin(),
        candidates.end(),
        [](
            const Candidate& a,
            const Candidate& b
        ) {
            return a.energy > b.energy;
        }
    );

    const size_t max_points =
        static_cast<size_t>(
            (byte_budget - 2) / 8
        );

    const size_t count =
        std::min(
            max_points,
            candidates.size()
        );

    std::vector<uint8_t> output;
    output.reserve(2 + count * 8);

    push_u16(
        output,
        static_cast<uint16_t>(count)
    );

    for (size_t i = 0; i < count; ++i) {
        const auto& candidate =
            candidates[i];

        if (candidate.index > 0xFFFF) break;

        push_u16(
            output,
            static_cast<uint16_t>(candidate.index)
        );

        push_i16(
            output,
            quantize_delta(candidate.delta.x)
        );

        push_i16(
            output,
            quantize_delta(candidate.delta.y)
        );

        push_i16(
            output,
            quantize_delta(candidate.delta.z)
        );
    }

    return output;
}

VaseCommunicantMultiplexer::MultiplexedPacket
VaseCommunicantMultiplexer::process_frame(
    const std::vector<cv::Point3f>& current_mesh,
    const std::vector<cv::Point3f>& previous_mesh,
    const std::vector<float>& audio_embedding,
    const ResidualVectorQuantizer& rvq
) const
{
    const float motion =
        kinetic_energy(
            current_mesh,
            previous_mesh
        );

    int planned_video_budget =
        motion > 0.05f
            ? static_cast<int>(
                BYTES_PER_FRAME * 0.70f
            )
            : motion > 0.01f
                ? static_cast<int>(
                    BYTES_PER_FRAME * 0.40f
                )
                : 4;

    planned_video_budget =
        std::clamp(
            planned_video_budget,
            0,
            BYTES_PER_FRAME - 1
        );

    auto video =
        encode_semantic_mesh(
            current_mesh,
            previous_mesh,
            planned_video_budget
        );

    const int audio_budget =
        std::max(
            0,
            BYTES_PER_FRAME -
            static_cast<int>(video.size()) -
            1
        );

    auto audio =
        rvq.encode_dynamic_rvq(
            audio_embedding,
            audio_budget
        );

    const uint8_t ratio =
        static_cast<uint8_t>(
            std::clamp(
                static_cast<int>(
                    (video.size() * 100) /
                    BYTES_PER_FRAME
                ),
                0,
                100
            )
        );

    return {
        ratio,
        std::move(video),
        std::move(audio)
    };
}

}
