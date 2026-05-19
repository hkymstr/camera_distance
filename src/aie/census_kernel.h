#pragma once
#include <adf.h>
#include "../include/stereo_config.h"

// ─── AIE census-transform kernel ────────────────────────────────────────────
//
// Each tile covers (IMG_HEIGHT / AIE_CENSUS_TILES_PER_IMG) rows and the full
// IMG_WIDTH columns.  Rows are streamed in line-by-line; the kernel maintains
// a circular line-buffer of CENSUS_WIN_H rows in its 32 KB local data memory.
//
// Input  : uint8  pixel stream, IMG_WIDTH values per line
// Output : uint64 census-descriptor stream (one 64-bit word per pixel;
//          only the low 48 bits are meaningful)
//
// Border policy: replicate — edge pixels use the nearest valid neighbour so
// that output dimensions exactly match input dimensions.
//
// AIE tile local-memory usage:
//   Line buffers : CENSUS_WIN_H × IMG_WIDTH × 1 B = 7 × 3840 = 26,880 B
//   Output row   :                IMG_WIDTH × 8 B =     3840 × 8 = 30,720 B
//   ─────────────────────────────────────────────────────────────────────────
//   Total                                                     ≈ 57,600 B
//
//   AIE tile data memory = 32 KB shared between two neighbour tiles (64 KB
//   physical).  For VC1902 each tile sees 32 KB directly addressable, so the
//   line-buffer fits; the output row is written straight to stream, not held.

void census_kernel(
    adf::input_stream<uint8_t>*  __restrict  pixel_in,
    adf::output_stream<uint64_t>* __restrict census_out,
    const int32_t                            row_start,   // first row this tile owns
    const int32_t                            row_end);    // one-past-last row
