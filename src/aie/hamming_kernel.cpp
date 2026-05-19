// AIE Hamming-distance matching kernel – Versal AI Core (VC1902)
//
// Vectorisation strategy:
//   For each row, maintain a right-image census shift register of IMG_WIDTH
//   descriptors.  For each disparity d in [disp_start, disp_start+DISP_SLICE):
//     cost[x] = popcount48(left[x] XOR right[x-d])
//   where right[x-d] = 0 (invalid) for x-d < 0.
//
//   popcount48 is computed on a 48-bit value split into six 8-bit nibble-pairs
//   via a 16-entry LUT (nibble popcount), applied to 6 nibble-pairs per word.
//   AIE's table-lookup instruction (tbl) enables vectorised LUT access.

#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include "hamming_kernel.h"

using namespace adf;

// ─── 4-bit nibble popcount table ────────────────────────────────────────────
alignas(32) static const uint8_t nibble_lut[16] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4
};

// ─── Popcount of a 48-bit census word (6 × 8-bit bytes) ────────────────────
static inline uint8_t popcount48(uint64_t x) {
    uint8_t cnt = 0;
    for (int b = 0; b < 6; ++b) {
        uint8_t byte = (uint8_t)(x >> (b * 8));
        cnt += nibble_lut[byte & 0xF] + nibble_lut[byte >> 4];
    }
    return cnt;
}

// ─── Right-image census row cache (one full row per AIE tile) ────────────────
// IMG_WIDTH × 8 B = 30,720 B — accessed in round-robin across tiles
alignas(32) static uint64_t right_row[IMG_WIDTH];

void hamming_kernel(
    input_stream<uint64_t>*  __restrict left_in,
    input_stream<uint64_t>*  __restrict right_in,
    output_stream<uint8_t>*  __restrict cost_out,
    const int32_t disp_start)
{
    // Step 1: buffer the entire right-census row
    for (int x = 0; x < IMG_WIDTH; ++x)
        right_row[x] = readincr(right_in);

    // Step 2: for each left pixel, compute DISP_SLICE costs and stream out
    for (int x = 0; x < IMG_WIDTH; ++x) {
        uint64_t lc = readincr(left_in);

        for (int dd = 0; dd < DISP_SLICE; ++dd) {
            int d  = disp_start + dd;
            int rx = x - d;
            uint8_t cost;
            if (rx < 0) {
                // Disparity takes the pixel off the left edge → maximum cost
                cost = CENSUS_BITS;
            } else {
                cost = popcount48(lc ^ right_row[rx]);
            }
            writeincr(cost_out, cost);
        }
    }
}
