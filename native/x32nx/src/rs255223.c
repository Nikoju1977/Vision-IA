#include "rs255223.h"

#include <string.h>

#define RS_POLY_BUFFER 96

static uint8_t gf_exp[512];
static uint8_t gf_log[256];
static uint8_t generator_poly[RS_PARITY + 1];
static int initialized = 0;

static uint8_t gf_mul(uint8_t a, uint8_t b)
{
    if (a == 0 || b == 0) return 0;
    return gf_exp[(unsigned int)gf_log[a] + (unsigned int)gf_log[b]];
}

static uint8_t gf_inv(uint8_t a)
{
    if (a == 0) return 0;
    return gf_exp[255u - (unsigned int)gf_log[a]];
}

static uint8_t gf_div(uint8_t a, uint8_t b)
{
    if (b == 0 || a == 0) return 0;
    return gf_exp[((int)gf_log[a] + 255 - (int)gf_log[b]) % 255];
}

static uint8_t gf_pow_alpha(int power)
{
    power %= 255;
    if (power < 0) power += 255;
    return gf_exp[power];
}

static uint8_t poly_eval(const uint8_t* poly, int length, uint8_t x)
{
    uint8_t y = poly[0];
    for (int i = 1; i < length; ++i) y = gf_mul(y, x) ^ poly[i];
    return y;
}

static void init_galois_field(void)
{
    uint16_t x = 1;
    memset(gf_exp, 0, sizeof(gf_exp));
    memset(gf_log, 0, sizeof(gf_log));

    for (int i = 0; i < 255; ++i) {
        gf_exp[i] = (uint8_t)x;
        gf_log[(uint8_t)x] = (uint8_t)i;
        x <<= 1;
        if (x & 0x100) x ^= 0x11D;
    }

    for (int i = 255; i < 512; ++i) gf_exp[i] = gf_exp[i - 255];
}

static void build_generator_polynomial(void)
{
    uint8_t current[RS_PARITY + 1] = {0};
    uint8_t next[RS_PARITY + 1] = {0};
    current[0] = 1;
    int degree = 0;

    for (int root_index = 1; root_index <= RS_PARITY; ++root_index) {
        memset(next, 0, sizeof(next));
        const uint8_t root = gf_exp[root_index];

        for (int j = 0; j <= degree; ++j) {
            next[j] ^= current[j];
            next[j + 1] ^= gf_mul(current[j], root);
        }

        ++degree;
        memcpy(current, next, sizeof(current));
    }

    memcpy(generator_poly, current, sizeof(generator_poly));
}

void rs255223_init(void)
{
    init_galois_field();
    build_generator_polynomial();
    initialized = 1;
}

void rs255223_encode_parity(const uint8_t msg[RS_K], uint8_t parity[RS_PARITY])
{
    if (!initialized) rs255223_init();

    uint8_t work[RS_N];
    memcpy(work, msg, RS_K);
    memset(work + RS_K, 0, RS_PARITY);

    for (int i = 0; i < RS_K; ++i) {
        const uint8_t factor = work[i];
        if (factor == 0) continue;

        for (int j = 1; j <= RS_PARITY; ++j) {
            work[i + j] ^= gf_mul(generator_poly[j], factor);
        }
    }

    memcpy(parity, work + RS_K, RS_PARITY);
}

void rs255223_encode_codeword(const uint8_t msg[RS_K], uint8_t codeword[RS_N])
{
    memcpy(codeword, msg, RS_K);
    rs255223_encode_parity(msg, codeword + RS_K);
}

static void calculate_syndromes(
    const uint8_t codeword[RS_N],
    uint8_t syndromes[RS_PARITY + 1]
)
{
    syndromes[0] = 0;
    for (int i = 1; i <= RS_PARITY; ++i) {
        syndromes[i] = poly_eval(codeword, RS_N, gf_exp[i]);
    }
}

static int syndromes_are_zero(const uint8_t syndromes[RS_PARITY + 1])
{
    for (int i = 1; i <= RS_PARITY; ++i) {
        if (syndromes[i] != 0) return 0;
    }
    return 1;
}

int rs255223_codeword_is_valid(const uint8_t codeword[RS_N])
{
    if (!initialized) rs255223_init();
    uint8_t syndromes[RS_PARITY + 1];
    calculate_syndromes(codeword, syndromes);
    return syndromes_are_zero(syndromes);
}

static void poly_scale(
    const uint8_t* poly,
    int length,
    uint8_t factor,
    uint8_t* output
)
{
    for (int i = 0; i < length; ++i) output[i] = gf_mul(poly[i], factor);
}

static int poly_add(
    const uint8_t* a,
    int a_length,
    const uint8_t* b,
    int b_length,
    uint8_t* output
)
{
    const int length = a_length > b_length ? a_length : b_length;
    const int a_offset = length - a_length;
    const int b_offset = length - b_length;

    memset(output, 0, RS_POLY_BUFFER);

    for (int i = 0; i < a_length; ++i) output[a_offset + i] ^= a[i];
    for (int i = 0; i < b_length; ++i) output[b_offset + i] ^= b[i];

    return length;
}

static int poly_mul(
    const uint8_t* a,
    int a_length,
    const uint8_t* b,
    int b_length,
    uint8_t* output
)
{
    const int length = a_length + b_length - 1;
    memset(output, 0, RS_POLY_BUFFER);

    for (int j = 0; j < b_length; ++j) {
        for (int i = 0; i < a_length; ++i) {
            output[i + j] ^= gf_mul(a[i], b[j]);
        }
    }

    return length;
}

static int find_error_locator(
    const uint8_t syndromes[RS_PARITY + 1],
    uint8_t error_locator[RS_POLY_BUFFER]
)
{
    uint8_t old_locator[RS_POLY_BUFFER] = {0};
    int old_length = 1;
    int locator_length = 1;

    old_locator[0] = 1;
    error_locator[0] = 1;

    for (int i = 0; i < RS_PARITY; ++i) {
        const int syndrome_index = i + 1;
        uint8_t discrepancy = syndromes[syndrome_index];

        for (int j = 1; j < locator_length; ++j) {
            discrepancy ^= gf_mul(
                error_locator[locator_length - 1 - j],
                syndromes[syndrome_index - j]
            );
        }

        old_locator[old_length++] = 0;

        if (discrepancy == 0) continue;

        if (old_length > locator_length) {
            uint8_t new_locator[RS_POLY_BUFFER] = {0};
            uint8_t replacement_old[RS_POLY_BUFFER] = {0};
            const int previous_locator_length = locator_length;

            poly_scale(old_locator, old_length, discrepancy, new_locator);
            poly_scale(
                error_locator,
                previous_locator_length,
                gf_inv(discrepancy),
                replacement_old
            );

            memcpy(error_locator, new_locator, (size_t)old_length);
            memset(error_locator + old_length, 0, RS_POLY_BUFFER - old_length);
            locator_length = old_length;

            memcpy(old_locator, replacement_old, (size_t)previous_locator_length);
            memset(
                old_locator + previous_locator_length,
                0,
                RS_POLY_BUFFER - previous_locator_length
            );
            old_length = previous_locator_length;
        }

        uint8_t scaled_old[RS_POLY_BUFFER] = {0};
        uint8_t sum[RS_POLY_BUFFER] = {0};

        poly_scale(old_locator, old_length, discrepancy, scaled_old);
        locator_length = poly_add(
            error_locator,
            locator_length,
            scaled_old,
            old_length,
            sum
        );

        memcpy(error_locator, sum, (size_t)locator_length);
        memset(error_locator + locator_length, 0, RS_POLY_BUFFER - locator_length);
    }

    int first_nonzero = 0;

    while (
        first_nonzero < locator_length - 1 &&
        error_locator[first_nonzero] == 0
    ) {
        ++first_nonzero;
    }

    if (first_nonzero > 0) {
        memmove(
            error_locator,
            error_locator + first_nonzero,
            (size_t)(locator_length - first_nonzero)
        );
        memset(
            error_locator + locator_length - first_nonzero,
            0,
            (size_t)first_nonzero
        );
        locator_length -= first_nonzero;
    }

    const int error_count = locator_length - 1;

    if (
        error_count <= 0 ||
        error_count > RS_MAX_UNKNOWN_ERRORS
    ) {
        return -1;
    }

    return locator_length;
}

static int find_error_positions(
    const uint8_t* error_locator,
    int locator_length,
    int positions[RS_PARITY]
)
{
    const int expected_errors = locator_length - 1;
    uint8_t reversed[RS_POLY_BUFFER] = {0};

    for (int i = 0; i < locator_length; ++i) {
        reversed[i] = error_locator[locator_length - 1 - i];
    }

    int found = 0;

    for (int i = 0; i < RS_N; ++i) {
        if (poly_eval(reversed, locator_length, gf_exp[i]) == 0) {
            if (found >= RS_PARITY) return -1;
            positions[found++] = RS_N - 1 - i;
        }
    }

    return found == expected_errors ? found : -1;
}

static int build_errata_locator(
    const int* coefficient_positions,
    int count,
    uint8_t* output
)
{
    uint8_t locator[RS_POLY_BUFFER] = {0};
    locator[0] = 1;
    int locator_length = 1;

    for (int i = 0; i < count; ++i) {
        const uint8_t factor[2] = {
            gf_pow_alpha(coefficient_positions[i]),
            1
        };
        uint8_t next[RS_POLY_BUFFER] = {0};

        const int next_length = poly_mul(
            locator,
            locator_length,
            factor,
            2,
            next
        );

        memcpy(locator, next, (size_t)next_length);
        memset(locator + next_length, 0, RS_POLY_BUFFER - next_length);
        locator_length = next_length;
    }

    memcpy(output, locator, (size_t)locator_length);
    return locator_length;
}

static int correct_errors(
    uint8_t codeword[RS_N],
    const uint8_t syndromes[RS_PARITY + 1],
    const int* error_positions,
    int error_count
)
{
    int coefficient_positions[RS_PARITY];

    for (int i = 0; i < error_count; ++i) {
        coefficient_positions[i] = RS_N - 1 - error_positions[i];
    }

    uint8_t errata_locator[RS_POLY_BUFFER] = {0};
    const int errata_locator_length = build_errata_locator(
        coefficient_positions,
        error_count,
        errata_locator
    );

    uint8_t reversed_syndromes[RS_PARITY + 1];

    for (int i = 0; i <= RS_PARITY; ++i) {
        reversed_syndromes[i] = syndromes[RS_PARITY - i];
    }

    uint8_t product[RS_POLY_BUFFER] = {0};

    const int product_length = poly_mul(
        reversed_syndromes,
        RS_PARITY + 1,
        errata_locator,
        errata_locator_length,
        product
    );

    const int evaluator_length = error_count + 1;
    if (evaluator_length > product_length) return 0;

    uint8_t evaluator[RS_POLY_BUFFER] = {0};

    memcpy(
        evaluator,
        product + product_length - evaluator_length,
        (size_t)evaluator_length
    );

    uint8_t roots[RS_PARITY] = {0};

    for (int i = 0; i < error_count; ++i) {
        const int l = 255 - coefficient_positions[i];
        roots[i] = gf_pow_alpha(-l);
    }

    for (int i = 0; i < error_count; ++i) {
        const uint8_t xi = roots[i];
        const uint8_t xi_inverse = gf_inv(xi);
        uint8_t locator_derivative = 1;

        for (int j = 0; j < error_count; ++j) {
            if (i == j) continue;

            locator_derivative = gf_mul(
                locator_derivative,
                (uint8_t)(1 ^ gf_mul(xi_inverse, roots[j]))
            );
        }

        if (locator_derivative == 0) return 0;

        const uint8_t numerator = poly_eval(
            evaluator,
            evaluator_length,
            xi_inverse
        );

        const uint8_t magnitude = gf_div(
            numerator,
            locator_derivative
        );

        codeword[error_positions[i]] ^= magnitude;
    }

    return 1;
}

rs255223_decode_result rs255223_decode(uint8_t codeword[RS_N])
{
    rs255223_decode_result result = {0, 0};

    if (!initialized) rs255223_init();

    uint8_t syndromes[RS_PARITY + 1];
    calculate_syndromes(codeword, syndromes);

    if (syndromes_are_zero(syndromes)) {
        result.success = 1;
        return result;
    }

    uint8_t error_locator[RS_POLY_BUFFER] = {0};

    const int locator_length = find_error_locator(
        syndromes,
        error_locator
    );

    if (locator_length < 0) return result;

    int error_positions[RS_PARITY] = {0};

    const int error_count = find_error_positions(
        error_locator,
        locator_length,
        error_positions
    );

    if (
        error_count < 0 ||
        error_count > RS_MAX_UNKNOWN_ERRORS
    ) {
        return result;
    }

    uint8_t repaired[RS_N];
    memcpy(repaired, codeword, RS_N);

    if (!correct_errors(
        repaired,
        syndromes,
        error_positions,
        error_count
    )) {
        return result;
    }

    calculate_syndromes(repaired, syndromes);

    if (!syndromes_are_zero(syndromes)) return result;

    memcpy(codeword, repaired, RS_N);
    result.success = 1;
    result.corrected_symbols = error_count;

    return result;
}
