#pragma once
#include <adf.h>
#include "../include/stereo_config.h"

// ─── AIE Hamming-distance matching kernel ───────────────────────────────────
//
// Each tile is responsible for a slice of MAX_DISP / AIE_HAMMING_TILES = 16
// consecutive disparity levels.  It receives one full row of left and right
// census descriptors and emits a cost-volume slice:
//   cost[y][x][d] = popcount(left_census[y][x] XOR right_census[y][x-d])
// for d in [disp_start, disp_start + DISP_SLICE).
//
// Input  : left-census  stream (uint64, IMG_WIDTH values)
//          right-census stream (uint64, IMG_WIDTH values)
// Output : cost stream  (uint8, IMG_WIDTH × DISP_SLICE values per row)
//          packed as DISP_SLICE consecutive bytes per pixel, x-major

#define DISP_SLICE      (MAX_DISP / AIE_HAMMING_TILES)   // 16

void hamming_kernel(
    adf::input_stream<uint64_t>*  __restrict left_in,
    adf::input_stream<uint64_t>*  __restrict right_in,
    adf::output_stream<uint8_t>*  __restrict cost_out,
    const int32_t                            disp_start);
