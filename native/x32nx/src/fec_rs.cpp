#include "fec_rs.h"

#include "rs255223.h"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace x32nx {

std::vector<uint8_t> apply_fec_encoding(const std::vector<uint8_t>& payload)
{
    if (payload.size() != RS_K) {
        throw std::invalid_argument(
            "RS(255,223): payload must contain exactly 223 bytes"
        );
    }

    std::array<uint8_t, RS_N> codeword{};
    rs255223_encode_codeword(payload.data(), codeword.data());

    return {codeword.begin(), codeword.end()};
}

std::vector<uint8_t> decode_fec_rs(const std::vector<uint8_t>& codeword)
{
    if (codeword.size() != RS_N) return {};

    std::array<uint8_t, RS_N> repaired{};
    std::copy(codeword.begin(), codeword.end(), repaired.begin());

    const rs255223_decode_result result =
        rs255223_decode(repaired.data());

    if (!result.success) return {};

    return {
        repaired.begin(),
        repaired.begin() + RS_K
    };
}

}
