#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace x32nx {

constexpr uint16_t X32NX_MAGIC = 0x5832;
constexpr std::size_t RS_CODEWORD_SIZE = 255;

struct SecurityMonitor {
    double tokens = 40.0;

    std::chrono::steady_clock::time_point last_update =
        std::chrono::steady_clock::now();

    uint64_t accepted = 0;
    uint64_t rejected_rate = 0;
    uint64_t rejected_size = 0;
    uint64_t rejected_magic = 0;
};

inline bool consume_rate_token(
    SecurityMonitor& monitor,
    double max_datagrams_per_second = 160.0,
    double max_burst = 40.0
)
{
    using Clock =
        std::chrono::steady_clock;

    const auto now = Clock::now();

    const double elapsed =
        std::chrono::duration<double>(
            now - monitor.last_update
        ).count();

    monitor.last_update = now;

    monitor.tokens =
        std::min(
            max_burst,
            monitor.tokens +
                elapsed *
                max_datagrams_per_second
        );

    if (monitor.tokens < 1.0) {
        ++monitor.rejected_rate;
        return false;
    }

    monitor.tokens -= 1.0;
    return true;
}

inline bool is_packet_safe_for_fec(
    const uint8_t* raw_buffer,
    std::size_t received_bytes,
    SecurityMonitor& monitor
)
{
    if (!raw_buffer) return false;

    if (!consume_rate_token(monitor)) {
        return false;
    }

    constexpr std::size_t expected_size =
        sizeof(uint16_t) +
        RS_CODEWORD_SIZE;

    if (received_bytes != expected_size) {
        ++monitor.rejected_size;
        return false;
    }

    const uint16_t packet_magic =
        (
            static_cast<uint16_t>(
                raw_buffer[0]
            ) << 8
        ) |
        static_cast<uint16_t>(
            raw_buffer[1]
        );

    if (packet_magic != X32NX_MAGIC) {
        ++monitor.rejected_magic;
        return false;
    }

    ++monitor.accepted;
    return true;
}

}
