// PL HLS: bilinear rectification – Versal AI Core PL fabric (VC1902)
//
// Pipeline II=1 target: one output pixel per clock at 300 MHz → 300 M px/s.
// Dual-4K @ 30fps = 248 M px/s per image, so a single instance suffices.
// Left and right images are rectified by separate, identical IP instances.
//
// The remap LUT is partitioned into 4 BRAMs (cyclic across x) to avoid bank
// conflicts at the bilinear 2×2 fetch.

#include "rectification.h"

#pragma HLS INTERFACE m_axi port=src      offset=slave bundle=src_bus  depth=8294400
#pragma HLS INTERFACE m_axi port=lut      offset=slave bundle=lut_bus  depth=8294400
#pragma HLS INTERFACE m_axi port=dst      offset=slave bundle=dst_bus  depth=8294400
#pragma HLS INTERFACE s_axilite port=height
#pragma HLS INTERFACE s_axilite port=width
#pragma HLS INTERFACE s_axilite port=return

// ─── Bilinear helper: sample src at (fx, fy) in Q20.12 fixed point ──────────
// Returns the bilinear-interpolated uint8 value.
// tx = fractional x bits [11:0], ty = fractional y bits [11:0]
static uint8_t bilinear(
    const uint8_t* src,
    uint32_t src_x_fp, uint32_t src_y_fp,
    uint32_t width, uint32_t height)
{
#pragma HLS INLINE
    const uint32_t FRAC = 12;
    const uint32_t HALF = 1 << (FRAC - 1);

    int32_t ix = (int32_t)(src_x_fp >> FRAC);
    int32_t iy = (int32_t)(src_y_fp >> FRAC);
    uint32_t fx = src_x_fp & ((1u << FRAC) - 1);
    uint32_t fy = src_y_fp & ((1u << FRAC) - 1);

    // Clamp to image boundary
    int32_t x0 = (ix < 0) ? 0 : (ix >= (int32_t)width  - 1 ? (int32_t)width  - 2 : ix);
    int32_t y0 = (iy < 0) ? 0 : (iy >= (int32_t)height - 1 ? (int32_t)height - 2 : iy);
    int32_t x1 = x0 + 1;
    int32_t y1 = y0 + 1;

    uint32_t p00 = src[y0 * width + x0];
    uint32_t p10 = src[y0 * width + x1];
    uint32_t p01 = src[y1 * width + x0];
    uint32_t p11 = src[y1 * width + x1];

    // Bilinear blend in Q12 arithmetic to avoid floating point
    uint32_t top = ((1u << FRAC) - fx) * p00 + fx * p10;
    uint32_t bot = ((1u << FRAC) - fx) * p01 + fx * p11;
    uint32_t val = (((1u << FRAC) - fy) * top + fy * bot + HALF) >> (2 * FRAC);
    return (uint8_t)val;
}

// ─── Top-level function ──────────────────────────────────────────────────────
void rectify(
    const Pixel8*   src,
    const uint64_t* lut,
    Pixel8*         dst,
    uint32_t        height,
    uint32_t        width)
{
#pragma HLS DATAFLOW

    const uint32_t npixels = height * width;

RECTIFY_LOOP:
    for (uint32_t i = 0; i < npixels; ++i) {
#pragma HLS PIPELINE II=1

        // Each LUT entry: bits[31:0]  = src_x in Q20.12
        //                 bits[63:32] = src_y in Q20.12
        uint64_t entry  = lut[i];
        uint32_t src_xf = (uint32_t)(entry & 0xFFFFFFFFULL);
        uint32_t src_yf = (uint32_t)(entry >> 32);

        dst[i] = bilinear(src, src_xf, src_yf, width, height);
    }
}
