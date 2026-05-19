#pragma once
#include <adf.h>
#include "../include/stereo_config.h"

// ─── AIE SGM path-aggregation kernel ────────────────────────────────────────
//
// Implements one directional pass of the SGM recurrence:
//
//   Lr(p, d) = C(p, d)
//            + min( Lr(p−r, d),
//                   Lr(p−r, d−1) + P1,
//                   Lr(p−r, d+1) + P1,
//                   min_k Lr(p−r, k) + P2 )
//            − min_k Lr(p−r, k)
//
// One kernel instance handles a single scan direction (LEFT→RIGHT, RIGHT→LEFT,
// TOP→BOTTOM, or BOTTOM→TOP).  The 2D problem is serialised: pixels arrive in
// raster order consistent with the chosen direction.
//
// Input  : cost_in  – per-pixel cost slice [MAX_DISP] values, uint8
// Output : agg_out  – accumulated path cost slice [MAX_DISP] values, uint16
//
// The kernel accumulates in uint16 to avoid overflow over long paths
// (worst-case: IMG_WIDTH × (P2 + C_max) ≈ 3840 × (86 + 48) = ~515 K, fits
//  in uint16 after the min-subtraction which keeps values ≤ P2 + C_max ≤ 134).

void sgm_path_kernel(
    adf::input_stream<uint8_t>*  __restrict cost_in,
    adf::output_stream<uint16_t>* __restrict agg_out,
    const int32_t                            direction);  // 0=L→R 1=R→L 2=T→B 3=B→T
