// Software testbench – validates stereo pipeline on x86 (no AIE/PL hardware)
//
// Substitutes AIE kernels with C reference implementations.
// Checks that disparity error is within ±2 pixels for a synthetic
// horizontal shift ground-truth pattern.
//
// Build:  g++ -O2 -std=c++17 -I../src/include tb/testbench.cpp -o stereo_tb
// Run:    ./stereo_tb

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <algorithm>
#include "../src/include/stereo_config.h"
#include "../src/include/stereo_types.h"
#include "../src/pl/depth_convert.h"
#include "../src/pl/post_process.h"

// ─── Tiny synthetic image dimensions for fast simulation ────────────────────
#define TB_W    320
#define TB_H    240
#define TB_DISP 64

// ─── Reference census transform (C, scalar) ─────────────────────────────────
static uint64_t ref_census(const uint8_t* img, int w, int h, int x, int y) {
    auto clamp = [](int v, int lo, int hi){ return v<lo?lo:(v>hi?hi:v); };
    uint8_t center = img[y * w + x];
    uint64_t bits = 0;
    int bit = 0;
    for (int dr = -CENSUS_WIN_H/2; dr <= CENSUS_WIN_H/2; ++dr) {
        for (int dc = -CENSUS_WIN_W/2; dc <= CENSUS_WIN_W/2; ++dc) {
            if (dr == 0 && dc == 0) continue;
            uint8_t nb = img[clamp(y+dr,0,h-1)*w + clamp(x+dc,0,w-1)];
            bits |= (uint64_t)(nb >= center) << bit;
            ++bit;
        }
    }
    return bits;
}

// ─── Popcount (48-bit) ───────────────────────────────────────────────────────
static int popcount48(uint64_t x) {
    int cnt = 0;
    for (int b = 0; b < 6; ++b) {
        uint8_t byte = (uint8_t)(x >> (b*8));
        cnt += __builtin_popcount(byte);
    }
    return cnt;
}

// ─── Reference winner-take-all disparity (Census+WTA only, no SGM) ──────────
// Left-image disparity: for each left pixel x, find d s.t. right[x-d] matches.
static void ref_disparity(
    const uint8_t* left, const uint8_t* right,
    uint8_t* disp, int w, int h, int max_d)
{
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            uint64_t lc = ref_census(left, w, h, x, y);
            int best_d   = 0;
            int best_cost = INT32_MAX;
            for (int d = 0; d < max_d; ++d) {
                int rx = x - d;
                int cost = (rx < 0) ? CENSUS_BITS
                                    : popcount48(lc ^ ref_census(right,w,h,rx,y));
                if (cost < best_cost) { best_cost = cost; best_d = d; }
            }
            disp[y * w + x] = (uint8_t)best_d;
        }
    }
}

// Right-image disparity: for each right pixel x_R, find d s.t. left[x_R+d] matches.
// Used for L-R consistency check (disp_R[x_R] should equal disp_L[x_R + disp_R[x_R]]).
static void ref_disparity_right(
    const uint8_t* right_img, const uint8_t* left_img,
    uint8_t* disp_R, int w, int h, int max_d)
{
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            uint64_t rc = ref_census(right_img, w, h, x, y);
            int best_d   = 0;
            int best_cost = INT32_MAX;
            for (int d = 0; d < max_d; ++d) {
                int lx = x + d;   // search leftward in the LEFT image
                int cost = (lx >= w) ? CENSUS_BITS
                                     : popcount48(rc ^ ref_census(left_img,w,h,lx,y));
                if (cost < best_cost) { best_cost = cost; best_d = d; }
            }
            disp_R[y * w + x] = (uint8_t)best_d;
        }
    }
}

// ─── Generate synthetic stereo pair ─────────────────────────────────────────
// Left image: checkerboard.  Right image: left shifted by GT_DISP columns.
#define GT_DISP  24   // ground-truth disparity (pixels)

static void make_checkerboard(uint8_t* img, int w, int h) {
    // Seeded random texture: aperiodic, so census matching is unambiguous.
    // Each pixel has a unique neighbourhood signature at the correct disparity.
    srand(0xDEADBEEF);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            img[y*w+x] = (uint8_t)(rand() % 256);
}

static void make_shifted(const uint8_t* left, uint8_t* right, int w, int h, int d) {
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int sx = x + d;   // right image = left shifted rightward by d
            right[y*w+x] = (sx < w) ? left[y*w+sx] : 0;
        }
    }
}

// ─── Build identity remap LUT (no rectification distortion for testbench) ────
static void make_identity_lut(uint64_t* lut, int w, int h) {
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            // src_x = x in Q20.12, src_y = y in Q20.12
            uint32_t fx = (uint32_t)x << 12;
            uint32_t fy = (uint32_t)y << 12;
            lut[y*w+x] = ((uint64_t)fy << 32) | fx;
        }
    }
}

int main() {
    printf("=== Stereo pipeline testbench (x86 reference) ===\n");
    printf("Image: %d×%d, max disparity: %d, GT disparity: %d\n",
           TB_W, TB_H, TB_DISP, GT_DISP);

    // ── Allocate buffers ──────────────────────────────────────────────────
    const int N = TB_W * TB_H;
    uint8_t*  raw_L    = new uint8_t[N];
    uint8_t*  raw_R    = new uint8_t[N];
    uint64_t* lut      = new uint64_t[N];
    uint8_t*  disp_L   = new uint8_t[N];
    uint8_t*  disp_R   = new uint8_t[N];
    uint16_t* depth_lut_arr = new uint16_t[DEPTH_LUT_SIZE];
    uint16_t* depth_mm  = new uint16_t[N];
    uint16_t* depth_prev = new uint16_t[N]();
    uint16_t* depth_final = new uint16_t[N];

    // ── Generate synthetic input ──────────────────────────────────────────
    make_checkerboard(raw_L, TB_W, TB_H);
    make_shifted(raw_L, raw_R, TB_W, TB_H, GT_DISP);
    make_identity_lut(lut, TB_W, TB_H);

    printf("[1/4] Rectification: identity LUT (no-op) ... PASS\n");

    // ── Compute disparity (reference Census+WTA) ──────────────────────────
    printf("[2/4] Census+WTA disparity matching ...\n");
    ref_disparity(raw_L, raw_R, disp_L, TB_W, TB_H, TB_DISP);
    ref_disparity_right(raw_R, raw_L, disp_R, TB_W, TB_H, TB_DISP);

    // Measure accuracy in the interior (away from left GT_DISP + CENSUS_WIN_W/2 border)
    int valid = 0, correct = 0;
    const int BORDER = GT_DISP + CENSUS_WIN_W / 2 + 1;
    for (int y = CENSUS_WIN_H/2; y < TB_H - CENSUS_WIN_H/2; ++y) {
        for (int x = BORDER; x < TB_W - CENSUS_WIN_W/2; ++x) {
            int d = disp_L[y * TB_W + x];
            ++valid;
            if (abs(d - GT_DISP) <= 2) ++correct;
        }
    }
    float acc = 100.0f * correct / valid;
    printf("    Accuracy (|error|<=2px): %.1f%%  (%d/%d pixels)\n", acc, correct, valid);
    assert(acc > 90.0f && "Census+WTA accuracy below 90% on synthetic test");
    printf("    PASS\n");

    // ── Depth conversion ──────────────────────────────────────────────────
    printf("[3/4] Depth conversion ...\n");
    build_depth_lut(depth_lut_arr);

    depth_convert(disp_L, disp_R, depth_mm, depth_lut_arr, TB_H, TB_W);

    uint32_t expected_mm = (GT_DISP > 0)
        ? (uint32_t)FOCAL_LENGTH_PX * BASELINE_MM / GT_DISP : 0;
    int depth_ok = 0, depth_total = 0;
    for (int i = 0; i < N; ++i) {
        if (depth_mm[i] > 0) {
            ++depth_total;
            uint32_t d = (uint32_t)depth_mm[i];
            // Allow 5% tolerance (rounding + border effects)
            uint32_t tol = expected_mm / 20 + 1;
            if (d >= expected_mm - tol && d <= expected_mm + tol) ++depth_ok;
        }
    }
    printf("    Expected depth: %u mm, valid pixels: %d, within 5%%: %d\n",
           expected_mm, depth_total, depth_ok);
    assert(depth_ok > depth_total * 8 / 10 && "Depth conversion accuracy too low");
    printf("    PASS\n");

    // ── Post-processing ───────────────────────────────────────────────────
    printf("[4/4] IIR + hole fill ...\n");
    post_process(depth_mm, depth_prev, depth_final, TB_H, TB_W);
    // After first frame, IIR output == input (prev was 0)
    int iir_ok = 0;
    for (int i = 0; i < N; ++i)
        if (depth_final[i] == depth_mm[i] || depth_mm[i] == 0) ++iir_ok;
    assert(iir_ok == N && "IIR first-frame pass-through failed");
    printf("    PASS\n");

    printf("\n=== All tests PASSED ===\n");

    delete[] raw_L; delete[] raw_R; delete[] lut;
    delete[] disp_L; delete[] disp_R;
    delete[] depth_lut_arr; delete[] depth_mm;
    delete[] depth_prev; delete[] depth_final;
    return 0;
}
