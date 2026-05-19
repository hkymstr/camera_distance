#pragma once
#include <stdint.h>
#include "../include/stereo_config.h"
#include "../include/stereo_types.h"

// ─── PL HLS: temporal IIR filter + hole filling ─────────────────────────────
//
// Stage 1 – Temporal IIR (per-pixel exponential moving average):
//   out[t] = α × raw[t] + (1−α) × out[t−1]  where α = IIR_ALPHA / 256
//   Applied only to valid pixels (depth != 0).  Invalid pixels propagate the
//   previous valid value to avoid ghosting.
//
// Stage 2 – Hole filling (single-pass left→right scanline fill):
//   For each invalid pixel, linearly interpolate between the nearest valid
//   left and right neighbours on the same row.  Maximum fill span = FILL_MAX.
//   Pixels beyond FILL_MAX from both neighbours remain 0.

#define IIR_ALPHA   192     // 192/256 ≈ 0.75  (fast response, moderate smoothing)
#define FILL_MAX    32      // maximum gap (pixels) to fill across

void post_process(
    const uint16_t* depth_in,    // [IMG_HEIGHT][IMG_WIDTH]  current raw depth
    uint16_t*       depth_prev,  // [IMG_HEIGHT][IMG_WIDTH]  previous IIR output (R/W)
    uint16_t*       depth_out,   // [IMG_HEIGHT][IMG_WIDTH]  final output
    uint32_t        height,
    uint32_t        width);
