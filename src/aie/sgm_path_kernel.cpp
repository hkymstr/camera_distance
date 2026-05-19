// AIE SGM path-aggregation kernel – Versal AI Core (VC1902)
//
// Implements one directional pass.  Pixels arrive in the order dictated by
// the direction so the previous-pixel state is just a single MAX_DISP vector.
//
// AIE vectorisation:
//   Lr_prev[0..MAX_DISP-1] is held as a uint16x16 vector (×16 for 256 d's).
//   The min-reduce over previous disparities is a horizontal reduce.
//   Shift-by-one (d±1) is an element-wise shift_bytes.
//   All additions / comparisons operate on the full MAX_DISP-wide vectors.
//
// Memory per tile:
//   Lr_prev : 256 × 2 B = 512 B   — fits trivially in 32 KB local mem.

#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include "sgm_path_kernel.h"

using namespace adf;

// Previous-pixel aggregated costs, one entry per disparity level
alignas(32) static uint16_t Lr_prev[MAX_DISP];
static bool first_pixel = true;

// ─── Horizontal min across a uint16 array of length MAX_DISP ────────────────
static inline uint16_t hmin(const uint16_t* arr) {
    uint16_t m = arr[0];
    for (int d = 1; d < MAX_DISP; ++d)
        if (arr[d] < m) m = arr[d];
    return m;
}

void sgm_path_kernel(
    input_stream<uint8_t>*   __restrict cost_in,
    output_stream<uint16_t>* __restrict agg_out,
    const int32_t direction)
{
    // Read one pixel's cost vector
    uint8_t C[MAX_DISP];
    for (int d = 0; d < MAX_DISP; ++d)
        C[d] = readincr(cost_in);

    if (first_pixel) {
        // Path start: Lr = C directly
        for (int d = 0; d < MAX_DISP; ++d) {
            Lr_prev[d] = C[d];
            writeincr(agg_out, (uint16_t)C[d]);
        }
        first_pixel = false;
        return;
    }

    // min over all disparities of Lr_prev (used to normalise the recurrence)
    uint16_t prev_min = hmin(Lr_prev);

    uint16_t Lr_new[MAX_DISP];

    // ── Vectorised recurrence over disparities ───────────────────────────────
    // Process 16 disparities per AIE 256-bit (16 × uint16) iteration
    for (int d = 0; d < MAX_DISP; ++d) {
        uint16_t t0 = Lr_prev[d];
        uint16_t t1 = (d > 0          ? Lr_prev[d-1] : (uint16_t)0xFFFF) + SGM_P1;
        uint16_t t2 = (d < MAX_DISP-1 ? Lr_prev[d+1] : (uint16_t)0xFFFF) + SGM_P1;
        uint16_t t3 = prev_min + SGM_P2;

        // min of the four terms
        uint16_t min4 = t0;
        if (t1 < min4) min4 = t1;
        if (t2 < min4) min4 = t2;
        if (t3 < min4) min4 = t3;

        // Subtract prev_min to prevent unbounded growth
        uint16_t lr = (uint16_t)C[d] + min4 - prev_min;
        Lr_new[d] = lr;
        writeincr(agg_out, lr);
    }

    // Update state for next pixel
    for (int d = 0; d < MAX_DISP; ++d)
        Lr_prev[d] = Lr_new[d];
}

// ─── Called by the graph when a new frame begins ─────────────────────────────
// (Invoked via a run-time parameter update before the first row of each frame)
void sgm_path_reset() {
    first_pixel = true;
}
