#pragma once
#include <stdint.h>

#ifdef __SYNTHESIS__
#  include <ap_int.h>
#  include <hls_stream.h>
using Pixel8   = ap_uint<8>;
using Pixel16  = ap_uint<16>;
using Disp8    = ap_uint<8>;
using Cost8    = ap_uint<8>;
using Cost16   = ap_uint<16>;
using Coord16  = ap_uint<16>;
#else
using Pixel8   = uint8_t;
using Pixel16  = uint16_t;
using Disp8    = uint8_t;
using Cost8    = uint8_t;
using Cost16   = uint16_t;
using Coord16  = uint16_t;
#endif

// ─── PL stream element: packed left+right pixel pair ────────────────────────
struct PixelPair {
    Pixel8 L;
    Pixel8 R;
};

// ─── Rectification LUT entry: source (x, y) in fixed-point Q12 ─────────────
// Bilinear weights encoded as: src_x = val >> 12, frac_x = val & 0xFFF
struct RemapEntry {
    uint32_t src_x;   // Q20.12 fixed-point source column
    uint32_t src_y;   // Q20.12 fixed-point source row
};

// ─── Disparity output pixel ─────────────────────────────────────────────────
struct DispPixel {
    Disp8   disp;
    uint8_t confidence;   // 0 = invalid, 255 = high confidence
};

// ─── Depth output pixel (millimetres) ───────────────────────────────────────
using DepthMM = uint16_t;   // 0 = invalid, max ~65535 mm (~65 m)
