#pragma once

// ─── Image geometry ────────────────────────────────────────────────────────
#define IMG_WIDTH       3840
#define IMG_HEIGHT      2160
#define IMG_PIXELS      (IMG_WIDTH * IMG_HEIGHT)

// ─── Disparity search ───────────────────────────────────────────────────────
#define MAX_DISP        256     // disparity search range [0, MAX_DISP)
#define MIN_DISP        0

// ─── Census transform (7×7 window → 48 bits) ────────────────────────────────
#define CENSUS_WIN_H    7
#define CENSUS_WIN_W    7
#define CENSUS_BITS     48      // (7*7) - 1 center pixel = 48

// ─── SGM penalties ──────────────────────────────────────────────────────────
#define SGM_P1          7       // penalty for |Δd| == 1
#define SGM_P2          86      // penalty for |Δd|  > 1
#define SGM_NUM_PATHS   4       // horizontal, vertical, 2 diagonals

// ─── Camera calibration (8 mm lens, IMX585, 4K) ─────────────────────────────
#define FOCAL_LENGTH_PX 2800    // focal length in pixels
#define BASELINE_MM     150     // stereo baseline in millimetres

// ─── Depth output ───────────────────────────────────────────────────────────
// depth_mm = (FOCAL_LENGTH_PX * BASELINE_MM) / disparity_px
// LUT converts disparity [1..MAX_DISP-1] → depth in mm (uint16, max ~420 m)
#define DEPTH_LUT_SIZE  256

// ─── Versal AI Core (VC1902) tile budget ────────────────────────────────────
// Total: 400 AIE tiles available on VC1902
// Allocation:
//   Census (left + right)  : 8  tiles  (4 each, tile per horizontal stripe)
//   Hamming matching        : 16 tiles  (each handles MAX_DISP/16 = 16 disparities)
//   SGM path aggregation    : 8  tiles  (4 paths × 2 tiles per path)
//   Winner-take-all         : 4  tiles  (left-right consistency check included)
//   Total                   : 36 tiles  (well within 400 tile budget)
#define AIE_CENSUS_TILES_PER_IMG  4
#define AIE_HAMMING_TILES         16
#define AIE_SGM_TILES_PER_PATH    2
#define AIE_WTA_TILES             4

// ─── PLIO data width (bits) ─────────────────────────────────────────────────
#define PLIO_WIDTH_BITS 128     // 128-bit PLIO chosen for DDR bandwidth alignment
