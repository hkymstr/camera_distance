#pragma once
#include <stdint.h>
#include "../include/stereo_config.h"
#include "../include/stereo_types.h"

// ─── Top-level stereo pipeline ───────────────────────────────────────────────
//
// Integrates PL HLS kernels with the AIE graph via PLIO ports.
// Data flow:
//
//   DDR ──[AXI4-MM]──▶ rectify_L ──[PLIO 128b]──▶ AIE census_L (×4 tiles)
//   DDR ──[AXI4-MM]──▶ rectify_R ──[PLIO 128b]──▶ AIE census_R (×4 tiles)
//                                                          │
//                                          ┌──────────────▼──────────────┐
//                                          │ AIE hamming match (×16 tiles)│
//                                          └──────────────┬──────────────┘
//                                                         │
//                                          ┌──────────────▼──────────────┐
//                                          │  AIE SGM aggregation (×8)   │
//                                          └──────────────┬──────────────┘
//                                                         │
//                                          ┌──────────────▼──────────────┐
//                                          │  AIE WTA + LR check  (×4)   │
//                                          └──────────────┬──────────────┘
//                                                         │
//                              ◀──[PLIO 128b]─────────────┘
//                              │
//          depth_convert ◀─────┘   (PL HLS, BRAM LUT, II=1)
//                │
//          post_process ◀──── depth_prev buffer (ping-pong DDR)
//                │
//                ▼
//          DDR ──[AXI4-MM]── depth_out [H][W] uint16

void stereo_pipeline(
    // Raw sensor frames (DDR)
    const uint8_t*  raw_L,        // [IMG_HEIGHT][IMG_WIDTH]
    const uint8_t*  raw_R,        // [IMG_HEIGHT][IMG_WIDTH]
    // Rectification LUTs (DDR, loaded from calibration file at boot)
    const uint64_t* lut_L,        // [IMG_HEIGHT][IMG_WIDTH]
    const uint64_t* lut_R,        // [IMG_HEIGHT][IMG_WIDTH]
    // Depth LUT (256 entries, pre-computed)
    const uint16_t* depth_lut,    // [DEPTH_LUT_SIZE]
    // Persistent IIR state (ping-pong, caller manages frame index)
    uint16_t*       depth_prev,   // [IMG_HEIGHT][IMG_WIDTH]
    // Output
    uint16_t*       depth_out);   // [IMG_HEIGHT][IMG_WIDTH]  depth in mm
