// PL HLS: temporal IIR filter + scanline hole filling
//
// Both stages are single-pass (one read + one write per pixel), pipelined II=1.
// depth_prev lives in DDR ping-pong buffers managed by the caller.

#include "post_process.h"

#pragma HLS INTERFACE m_axi port=depth_in   offset=slave bundle=in_bus   depth=8294400
#pragma HLS INTERFACE m_axi port=depth_prev offset=slave bundle=prev_bus  depth=8294400
#pragma HLS INTERFACE m_axi port=depth_out  offset=slave bundle=out_bus   depth=8294400
#pragma HLS INTERFACE s_axilite port=height
#pragma HLS INTERFACE s_axilite port=width
#pragma HLS INTERFACE s_axilite port=return

void post_process(
    const uint16_t* depth_in,
    uint16_t*       depth_prev,
    uint16_t*       depth_out,
    uint32_t        height,
    uint32_t        width)
{
    const uint32_t npixels = height * width;

    // ── Stage 1: Temporal IIR ─────────────────────────────────────────────
    // Allocate intermediate buffer in BRAM (one scanline at a time to save URAM)
    // For full-frame II=1: use on-chip line buffer + streaming into Stage 2.
    // Here we write to depth_out (then re-read for hole fill) to keep it simple;
    // a fully pipelined version would use hls::stream between stages.
IIR_LOOP:
    for (uint32_t i = 0; i < npixels; ++i) {
#pragma HLS PIPELINE II=1

        uint16_t raw  = depth_in[i];
        uint16_t prev = depth_prev[i];
        uint16_t iir;

        if (raw == 0) {
            // Invalid pixel: hold previous value, do not update IIR state
            iir = prev;
        } else if (prev == 0) {
            // First valid frame: initialise IIR state
            iir = raw;
        } else {
            // α = IIR_ALPHA/256 ≈ 0.75
            // out = α*raw + (1-α)*prev
            //     = (IIR_ALPHA*raw + (256-IIR_ALPHA)*prev) >> 8
            uint32_t blended = (uint32_t)IIR_ALPHA * raw
                             + (uint32_t)(256 - IIR_ALPHA) * prev;
            iir = (uint16_t)(blended >> 8);
        }

        depth_prev[i] = iir;   // update IIR state in-place
        depth_out[i]  = iir;   // write stage-1 output
    }

    // ── Stage 2: Scanline hole fill ──────────────────────────────────────
    // Left→right pass per scanline.
    // Tracks the last valid pixel value and span length; fills forward.
FILL_ROWS:
    for (uint32_t row = 0; row < height; ++row) {
        uint32_t base = row * width;

        uint16_t left_val  = 0;
        uint32_t gap_start = 0;
        bool     in_gap    = false;

FILL_COLS:
        for (uint32_t col = 0; col < width; ++col) {
#pragma HLS PIPELINE II=1

            uint32_t idx = base + col;
            uint16_t v   = depth_out[idx];

            if (v != 0) {
                if (in_gap && left_val != 0) {
                    // Fill the gap with linear interpolation between left_val and v
                    uint32_t span = col - gap_start;
                    if (span <= FILL_MAX) {
                        for (uint32_t f = gap_start; f < col; ++f) {
                            uint16_t interp = (uint16_t)(
                                ((uint32_t)left_val * (col - f) +
                                 (uint32_t)v        * (f - gap_start)) / span);
                            depth_out[base + f] = interp;
                        }
                    }
                }
                left_val  = v;
                in_gap    = false;
            } else {
                if (!in_gap) {
                    gap_start = col;
                    in_gap    = true;
                }
            }
        }
    }
}
