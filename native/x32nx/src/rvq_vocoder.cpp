#include "rvq_vocoder.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace x32nx {

ResidualVectorQuantizer::ResidualVectorQuantizer(
    std::vector<Codebook> cascade
)
    : cascade_(std::move(cascade))
{
    validate();
}

void ResidualVectorQuantizer::set_codebooks(
    std::vector<Codebook> cascade
)
{
    cascade_ = std::move(cascade);
    validate();
}

float ResidualVectorQuantizer::l2_distance(
    const std::vector<float>& a,
    const std::vector<float>& b
)
{
    if (a.size() != b.size()) {
        throw std::invalid_argument(
            "RVQ dimension mismatch"
        );
    }

    float sum = 0.f;

    for (size_t i = 0; i < a.size(); ++i) {
        const float delta = a[i] - b[i];
        sum += delta * delta;
    }

    return sum;
}

void ResidualVectorQuantizer::validate() const
{
    size_t dimension = 0;

    for (const auto& codebook : cascade_) {
        if (codebook.vectors.empty()) {
            throw std::invalid_argument(
                "RVQ empty codebook"
            );
        }

        if (codebook.vectors.size() > 256) {
            throw std::invalid_argument(
                "RVQ supports max 256 vectors per codebook"
            );
        }

        for (const auto& vector : codebook.vectors) {
            if (vector.empty()) {
                throw std::invalid_argument(
                    "RVQ empty vector"
                );
            }

            if (dimension == 0) {
                dimension = vector.size();
            }

            if (vector.size() != dimension) {
                throw std::invalid_argument(
                    "RVQ inconsistent vector dimensions"
                );
            }
        }
    }
}

std::vector<uint8_t>
ResidualVectorQuantizer::encode_dynamic_rvq(
    const std::vector<float>& embedding,
    int max_bytes_budget
) const
{
    if (
        embedding.empty() ||
        max_bytes_budget <= 0 ||
        cascade_.empty()
    ) {
        return {};
    }

    if (
        cascade_.front().vectors.front().size() !=
        embedding.size()
    ) {
        throw std::invalid_argument(
            "RVQ embedding dimension mismatch"
        );
    }

    const int depth =
        std::min<int>(
            max_bytes_budget,
            static_cast<int>(cascade_.size())
        );

    std::vector<uint8_t> indices;
    indices.reserve(static_cast<size_t>(depth));

    std::vector<float> residual = embedding;

    for (int stage = 0; stage < depth; ++stage) {
        float best_distance =
            std::numeric_limits<float>::max();

        size_t best_index = 0;

        for (
            size_t i = 0;
            i < cascade_[stage].vectors.size();
            ++i
        ) {
            const float distance =
                l2_distance(
                    residual,
                    cascade_[stage].vectors[i]
                );

            if (distance < best_distance) {
                best_distance = distance;
                best_index = i;
            }
        }

        indices.push_back(
            static_cast<uint8_t>(best_index)
        );

        const auto& chosen =
            cascade_[stage].vectors[best_index];

        for (size_t j = 0; j < residual.size(); ++j) {
            residual[j] -= chosen[j];
        }
    }

    return indices;
}

std::vector<float>
ResidualVectorQuantizer::decode(
    const std::vector<uint8_t>& indices,
    size_t embedding_dimension
) const
{
    std::vector<float> reconstruction(
        embedding_dimension,
        0.f
    );

    const size_t depth =
        std::min(indices.size(), cascade_.size());

    for (size_t stage = 0; stage < depth; ++stage) {
        const size_t index = indices[stage];

        if (index >= cascade_[stage].vectors.size()) {
            throw std::out_of_range(
                "RVQ index out of range"
            );
        }

        const auto& vector =
            cascade_[stage].vectors[index];

        if (vector.size() != embedding_dimension) {
            throw std::invalid_argument(
                "RVQ decode dimension mismatch"
            );
        }

        for (size_t i = 0; i < embedding_dimension; ++i) {
            reconstruction[i] += vector[i];
        }
    }

    return reconstruction;
}

}
