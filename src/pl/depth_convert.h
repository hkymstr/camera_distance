#pragma once
#include <stdint.h>
#include "../include/stereo_config.h"
#include "../include/stereo_types.h"

// ─── PL HLS: disparity → depth conversion ───────────────────────────────────
//
// depth_mm[d] = (FOCAL_LENGTH_PX × BASELINE_MM) / d   for d ∈ [1, MAX_DISP)
// depth_mm[0] = 0  (invalid / no disparity)
//
// The 256-entry LUT is pre-loaded into BRAM at startup and is accessed every
// cycle (sequential reads), achieving II=1 in a fully pipelined datapath.
//
// Optionally applies left-right consistency check: a pixel is marked invalid
// (depth = 0) if |disp_L[x] − disp_R[x − disp_L[x]]| > LR_THRESH.

#define LR_THRESH  1    // left-right check tolerance (pixels)

void depth_convert(
    const uint8_t*  disp_L,       // [IMG_HEIGHT][IMG_WIDTH]  left disparity map
    const uint8_t*  disp_R,       // [IMG_HEIGHT][IMG_WIDTH]  right disparity map
    uint16_t*       depth_out,    // [IMG_HEIGHT][IMG_WIDTH]  depth in mm
    const uint16_t  depth_lut[DEPTH_LUT_SIZE],
    uint32_t        height,
    uint32_t        width);

// Build the 256-entry depth LUT; call once on PS at startup
void build_depth_lut(uint16_t lut[DEPTH_LUT_SIZE]);
