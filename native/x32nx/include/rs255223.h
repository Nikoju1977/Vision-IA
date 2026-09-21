#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RS_N 255
#define RS_K 223
#define RS_PARITY (RS_N - RS_K)
#define RS_MAX_UNKNOWN_ERRORS (RS_PARITY / 2)

typedef struct rs255223_decode_result {
    int success;
    int corrected_symbols;
} rs255223_decode_result;

void rs255223_init(void);
void rs255223_encode_parity(const uint8_t msg[RS_K], uint8_t parity[RS_PARITY]);
void rs255223_encode_codeword(const uint8_t msg[RS_K], uint8_t codeword[RS_N]);
int rs255223_codeword_is_valid(const uint8_t codeword[RS_N]);
rs255223_decode_result rs255223_decode(uint8_t codeword[RS_N]);

#ifdef __cplusplus
}
#endif
