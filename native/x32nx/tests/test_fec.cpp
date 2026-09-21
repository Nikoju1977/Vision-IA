#include <gtest/gtest.h>

#include <cstdint>
#include <numeric>
#include <vector>

#include "fec_rs.h"
#include "rs255223.h"

namespace {

std::vector<uint8_t> standard_payload()
{
    std::vector<uint8_t> data(RS_K);
    std::iota(
        data.begin(),
        data.end(),
        0
    );
    return data;
}

}

TEST(
    ValidationReedSolomon,
    CorrigeDeZeroASeizeSymboles
)
{
    const auto payload =
        standard_payload();

    const auto encoded =
        x32nx::apply_fec_encoding(
            payload
        );

    for (
        int errors = 0;
        errors <=
            RS_MAX_UNKNOWN_ERRORS;
        ++errors
    ) {
        auto damaged = encoded;

        for (
            int i = 0;
            i < errors;
            ++i
        ) {
            const size_t position =
                static_cast<size_t>(
                    (i * 13 + 7) %
                    RS_N
                );

            damaged[position] ^=
                static_cast<uint8_t>(
                    1 + i * 7
                );
        }

        EXPECT_EQ(
            x32nx::decode_fec_rs(
                damaged
            ),
            payload
        )
            << "failure with "
            << errors
            << " damaged symbols";
    }
}

TEST(
    ValidationReedSolomon,
    RejetDefaillanceCritique17Symboles
)
{
    const auto payload =
        standard_payload();

    auto damaged =
        x32nx::apply_fec_encoding(
            payload
        );

    for (
        int i = 100;
        i < 117;
        ++i
    ) {
        damaged[
            static_cast<size_t>(i)
        ] = 0xFF;
    }

    EXPECT_TRUE(
        x32nx::decode_fec_rs(
            damaged
        ).empty()
    );
}
