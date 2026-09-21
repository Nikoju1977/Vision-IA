#pragma once

#include <cstdint>
#include <vector>

namespace x32nx {

std::vector<uint8_t> apply_fec_encoding(const std::vector<uint8_t>& payload);
std::vector<uint8_t> decode_fec_rs(const std::vector<uint8_t>& codeword);

}
