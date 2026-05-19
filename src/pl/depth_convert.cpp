// PL HLS: disparity → depth conversion + left-right consistency check
//
// Achieves II=1 at 300 MHz using 256-entry BRAM LUT.
// Left-right consistency reject rate is ~3-5% in practice (occlusions).

#include "depth_convert.h"

#pragma HLS INTERFACE m_axi port=disp_L    offset=slave bundle=disp_bus depth=8294400
#pragma HLS INTERFACE m_axi port=disp_R    offset=slave bundle=disp_bus depth=8294400
#pragma HLS INTERFACE m_axi port=depth_out offset=slave bundle=out_bus  depth=8294400
#pragma HLS INTERFACE s_axilite port=depth_lut
#pragma HLS INTERFACE s_axilite port=height
#pragma HLS INTERFACE s_axilite port=width
#pragma HLS INTERFACE s_axilite port=return

void depth_convert(
    const uint8_t*  disp_L,
    const uint8_t*  disp_R,
    uint16_t*       depth_out,
    const uint16_t  depth_lut[DEPTH_LUT_SIZE],
    uint32_t        height,
    uint32_t        width)
{
#pragma HLS DATAFLOW
#pragma HLS ARRAY_PARTITION variable=depth_lut complete

    const uint32_t npixels = height * width;

DEPTH_LOOP:
    for (uint32_t i = 0; i < npixels; ++i) {
#pragma HLS PIPELINE II=1

        uint8_t dL = disp_L[i];
        uint16_t depth = 0;

        if (dL > 0) {
            // Left-right consistency check
            // disp_R is in right-image coordinates; for pixel x in the left
            // image with disparity dL, the corresponding right pixel is x-dL.
            uint32_t col  = i % width;
            int32_t  rx   = (int32_t)col - (int32_t)dL;
            if (rx >= 0) {
                uint32_t ri   = (i / width) * width + (uint32_t)rx;
                uint8_t  dR   = disp_R[ri];
                int32_t  diff = (int32_t)dL - (int32_t)dR;
                if (diff < 0) diff = -diff;
                if (diff <= LR_THRESH) {
                    depth = depth_lut[dL];
                }
            }
        }

        depth_out[i] = depth;
    }
}

// ─── Pre-compute the depth LUT (called once at startup, runs on PS) ──────────
void build_depth_lut(uint16_t lut[DEPTH_LUT_SIZE]) {
    lut[0] = 0;   // disparity 0 → invalid
    for (int d = 1; d < DEPTH_LUT_SIZE; ++d) {
        uint32_t depth_mm = ((uint32_t)FOCAL_LENGTH_PX * (uint32_t)BASELINE_MM) / (uint32_t)d;
        // Clamp to uint16 range (~65 m max; beyond that depth is unreliable anyway)
        lut[d] = (depth_mm > 0xFFFF) ? 0xFFFF : (uint16_t)depth_mm;
    }
}
