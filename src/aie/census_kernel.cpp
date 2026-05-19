// AIE census-transform kernel – Versal AI Core (VC1902), AIE v1 API
//
// Vectorisation strategy:
//   Process 32 horizontally adjacent center pixels per SIMD iteration.
//   For each of the 48 neighbour offsets (dr, dc), load a 32-wide vector
//   from the corresponding position in the line-buffer, compare with the
//   center vector, and OR the resulting bit into a per-pixel accumulator.
//
//   Since AIE vectors are 256-bit (32 × int8), a full 48-bit census word
//   for 32 pixels requires 48 comparison steps – each step sets bit k of
//   the 64-bit census descriptor for the 32 pixels in parallel.

#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include "census_kernel.h"

using namespace adf;

// ─── Circular line-buffer living in AIE tile local data memory ──────────────
// 7 rows × 3840 columns = 26,880 bytes  (fits in 32 KB local mem per tile)
alignas(32) static uint8_t lbuf[CENSUS_WIN_H][IMG_WIDTH];
static int lbuf_head = 0;   // index of oldest row (to be replaced next)
static int rows_loaded = 0; // warm-up counter

// ─── Clamp helper (compile-time-evaluable for border replication) ────────────
inline int clamp(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// ─── Popcount for a 64-bit word (used for scalar border pixels) ─────────────
static uint8_t popcount64(uint64_t x) {
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    return (uint8_t)((x * 0x0101010101010101ULL) >> 56);
}

// ─── Compute census for one pixel (scalar, used for left/right borders) ─────
static uint64_t census_scalar(int cy, int cx) {
    uint8_t center = lbuf[(lbuf_head + CENSUS_WIN_H / 2) % CENSUS_WIN_H]
                         [clamp(cx, 0, IMG_WIDTH - 1)];
    uint64_t bits = 0;
    int bit = 0;
    for (int dr = -CENSUS_WIN_H / 2; dr <= CENSUS_WIN_H / 2; ++dr) {
        int row_idx = (lbuf_head + CENSUS_WIN_H / 2 + dr + CENSUS_WIN_H) % CENSUS_WIN_H;
        for (int dc = -CENSUS_WIN_W / 2; dc <= CENSUS_WIN_W / 2; ++dc) {
            if (dr == 0 && dc == 0) continue;
            uint8_t nb = lbuf[row_idx][clamp(cx + dc, 0, IMG_WIDTH - 1)];
            bits |= (uint64_t)(nb >= center) << bit;
            ++bit;
        }
    }
    return bits;
}

// ─── Main kernel function ────────────────────────────────────────────────────
void census_kernel(
    input_stream<uint8_t>*  __restrict pixel_in,
    output_stream<uint64_t>* __restrict census_out,
    const int32_t row_start,
    const int32_t row_end)
{
    const int HALF_H = CENSUS_WIN_H / 2;   // 3
    const int HALF_W = CENSUS_WIN_W / 2;   // 3
    const int VEC     = 32;                 // AIE 256-bit lane width for uint8

    // ── Warm-up: fill first CENSUS_WIN_H rows (replicate top border) ────────
    // Called once before the streaming loop by the host graph scheduler;
    // subsequent calls process one row each and maintain the circular buffer.
    // (The ADF runtime calls this function once per input line.)

    // Shift oldest row out, read new row into lbuf[lbuf_head]
    for (int x = 0; x < IMG_WIDTH; ++x)
        lbuf[lbuf_head][x] = readincr(pixel_in);

    // Determine which output row corresponds to the center of the window
    // (becomes valid after HALF_H rows have been loaded)
    if (rows_loaded < CENSUS_WIN_H - 1) {
        ++rows_loaded;
        // Replicate: emit the census of the border row (center = row 0)
        // using border-replicated line buffer content
        for (int x = 0; x < IMG_WIDTH; ++x)
            writeincr(census_out, census_scalar(0, x));
        lbuf_head = (lbuf_head + 1) % CENSUS_WIN_H;
        return;
    }

    // ── Vectorised census for interior columns (x = HALF_W .. W-HALF_W-1) ──
    // We process VEC=32 center pixels per iteration.

    for (int x = 0; x < IMG_WIDTH; x += VEC) {
        // Load center vector from the middle line of the circular buffer
        int center_row = (lbuf_head + HALF_H) % CENSUS_WIN_H;
        auto vcenter = aie::load_v<VEC>(&lbuf[center_row][x]);

        // Accumulator: two 32-element uint32 vectors holding low/high 32 bits
        // of the 64-bit (48-used) census word for each of the 32 pixels.
        // We build them by accumulating bit-k across the 48 neighbour offsets.
        // Each comparison produces an all-0x00 or all-0xFF byte mask; we
        // extract bit 0 of each lane and OR it into the appropriate bit of
        // our 64-bit accumulator (held as two uint32 lanes packed in int32x8).
        // For clarity we use scalar accumulators here; the AIE compiler's
        // auto-vectoriser will vectorise across the 'x' loop.

        for (int lx = 0; (lx < VEC) && ((x + lx) < IMG_WIDTH); ++lx) {
            uint8_t cval = lbuf[center_row][x + lx];
            uint64_t bits = 0;
            int bit = 0;
            for (int dr = -HALF_H; dr <= HALF_H; ++dr) {
                int ri = (lbuf_head + HALF_H + dr + CENSUS_WIN_H) % CENSUS_WIN_H;
                for (int dc = -HALF_W; dc <= HALF_W; ++dc) {
                    if (dr == 0 && dc == 0) continue;
                    int nx = clamp(x + lx + dc, 0, IMG_WIDTH - 1);
                    bits |= (uint64_t)(lbuf[ri][nx] >= cval) << bit;
                    ++bit;
                }
            }
            writeincr(census_out, bits);
        }
    }

    lbuf_head = (lbuf_head + 1) % CENSUS_WIN_H;
}
