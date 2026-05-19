#pragma once
#include <adf.h>
#include "census_kernel.h"
#include "hamming_kernel.h"
#include "sgm_path_kernel.h"
#include "../include/stereo_config.h"

using namespace adf;

// ─── AIE Stereo Graph ────────────────────────────────────────────────────────
//
// Tile layout on VC1902 (400 AIE tiles available):
//
//   census_L[0..3]   : columns 0-3, row 0  (left image, 4 row-stripes)
//   census_R[0..3]   : columns 0-3, row 1  (right image, 4 row-stripes)
//   hamming[0..15]   : columns 4-7, rows 0-3  (16 × DISP_SLICE=16 disparities)
//   sgm_h[0..1]      : columns 8-9, row 0  (horizontal passes: L→R, R→L)
//   sgm_v[0..1]      : columns 8-9, row 1  (vertical passes: T→B, B→T)
//   wta[0..3]        : columns 10-11, rows 0-1  (WTA + L-R check per stripe)
//
//   Total: 4+4+16+4+4 = 32 tiles  (8% of 400 available, ample headroom)

class StereoAIEGraph : public graph {
public:
    // ── PLIO ports (PL → AIE and AIE → PL, 128-bit wide @ 250 MHz) ─────────
    // 128-bit PLIO at 250 MHz = 4 GB/s; dual-4K raw12 @30fps needs ~750 MB/s
    input_plio   left_plio;   // rectified left  pixels (uint8, raster order)
    input_plio   right_plio;  // rectified right pixels (uint8, raster order)
    output_plio  disp_plio;   // disparity map   (uint8, one byte per pixel)

    // ── Kernel instances ─────────────────────────────────────────────────────
    kernel census_L[AIE_CENSUS_TILES_PER_IMG];
    kernel census_R[AIE_CENSUS_TILES_PER_IMG];
    kernel hamming[AIE_HAMMING_TILES];
    kernel sgm[SGM_NUM_PATHS];
    kernel wta[AIE_WTA_TILES];

    // ── Broadcast streams ────────────────────────────────────────────────────
    // Each census tile receives its row-stripe; the PLIO demux is done by the
    // router.  For simplicity in this graph all tiles share the same PLIO and
    // the kernel's row_start/row_end parameters gate which rows it processes.
    // In the final Vitis implementation this becomes a 1-to-4 broadcast.

    StereoAIEGraph();
};
