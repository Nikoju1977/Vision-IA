#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace x32nx {

struct Codebook {
    std::vector<std::vector<float>> vectors;
};

class ResidualVectorQuantizer {
public:
    ResidualVectorQuantizer() = default;
    explicit ResidualVectorQuantizer(std::vector<Codebook> cascade);

    void set_codebooks(std::vector<Codebook> cascade);

    std::vector<uint8_t> encode_dynamic_rvq(
        const std::vector<float>& audio_embedding,
        int max_bytes_budget
    ) const;

    std::vector<float> decode(
        const std::vector<uint8_t>& indices,
        size_t embedding_dimension
    ) const;

private:
    std::vector<Codebook> cascade_;

    static float l2_distance(
        const std::vector<float>& a,
        const std::vector<float>& b
    );

    void validate() const;
};

}
