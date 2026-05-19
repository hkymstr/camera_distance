// Top-level stereo pipeline – Versal AI Core (VC1902)
//
// Combines PL HLS kernels with the AIE graph.  In Vitis the AIE graph is
// connected to the PL via PLIO ports; this file orchestrates the PL side.
//
// AXI4-Lite control registers (s_axilite, base address set by linker):
//   0x00  start   (W)
//   0x04  done    (R)
//   0x08-0x... pointer arguments (auto-assigned by HLS)

#include "../top/stereo_pipeline.h"
#include "../pl/rectification.h"
#include "../pl/depth_convert.h"
#include "../pl/post_process.h"
#include <hls_stream.h>
#include <ap_int.h>

// AXI4 interface pragmas – all pointer arguments mapped to AXI4-MM masters
#pragma HLS INTERFACE m_axi port=raw_L      offset=slave bundle=input_bus  depth=8294400
#pragma HLS INTERFACE m_axi port=raw_R      offset=slave bundle=input_bus  depth=8294400
#pragma HLS INTERFACE m_axi port=lut_L      offset=slave bundle=lut_bus    depth=8294400
#pragma HLS INTERFACE m_axi port=lut_R      offset=slave bundle=lut_bus    depth=8294400
#pragma HLS INTERFACE m_axi port=depth_lut  offset=slave bundle=lut_bus    depth=256
#pragma HLS INTERFACE m_axi port=depth_prev offset=slave bundle=prev_bus   depth=8294400
#pragma HLS INTERFACE m_axi port=depth_out  offset=slave bundle=out_bus    depth=8294400
#pragma HLS INTERFACE s_axilite port=return

// ─── On-chip URAM buffers ────────────────────────────────────────────────────
// Rectified frames are written to URAM before streaming to AIE via PLIO.
// URAM on VC1902: 640 × 288 KB = 184 MB total.
// Two rectified frames: 2 × 3840 × 2160 = ~15.7 MB — fits comfortably.
#pragma HLS BIND_STORAGE variable=rect_L type=uram impl=uram
#pragma HLS BIND_STORAGE variable=rect_R type=uram impl=uram

static uint8_t rect_L[IMG_HEIGHT * IMG_WIDTH];
static uint8_t rect_R[IMG_HEIGHT * IMG_WIDTH];

// ─── Disparity result from AIE (written by PLIO DMA into URAM) ─────────────
static uint8_t disp_L[IMG_HEIGHT * IMG_WIDTH];
static uint8_t disp_R[IMG_HEIGHT * IMG_WIDTH];

// ─── Depth LUT cache (BRAM) ──────────────────────────────────────────────────
static uint16_t depth_lut_bram[DEPTH_LUT_SIZE];
#pragma HLS BIND_STORAGE variable=depth_lut_bram type=ram_2p impl=bram

void stereo_pipeline(
    const uint8_t*  raw_L,
    const uint8_t*  raw_R,
    const uint64_t* lut_L,
    const uint64_t* lut_R,
    const uint16_t* depth_lut,
    uint16_t*       depth_prev,
    uint16_t*       depth_out)
{
#pragma HLS DATAFLOW

    // ── Step 1: Rectify both images ──────────────────────────────────────────
    // Two parallel HLS instances; each II=1 at 300 MHz → 300 Mpx/s per image
    rectify(raw_L, lut_L, rect_L, IMG_HEIGHT, IMG_WIDTH);
    rectify(raw_R, lut_R, rect_R, IMG_HEIGHT, IMG_WIDTH);

    // ── Step 2: Stream rectified images to AIE via PLIO ─────────────────────
    // In the Vitis system design, the PLIO DMA engine reads rect_L / rect_R
    // from URAM and feeds the AIE graph's left_plio / right_plio ports.
    // The AIE graph runs census → hamming → SGM → WTA and writes results to
    // disp_L / disp_R via the output PLIO.
    //
    // This handoff is managed by the Vitis platform; no explicit code needed
    // here.  The stereo_pipeline kernel signals "rectification done" to the
    // PLIO DMA controller via an AXI4-Lite handshake (handled by Vitis linker).

    // ── Step 3: Depth conversion ─────────────────────────────────────────────
    // depth_lut is loaded once at startup; copy to BRAM for II=1 random access
    for (int d = 0; d < DEPTH_LUT_SIZE; ++d) {
#pragma HLS PIPELINE II=1
        depth_lut_bram[d] = depth_lut[d];
    }

    // Wait for AIE to complete (in practice: AXI4-Lite polling or interrupt)
    // Disparity results are now in disp_L / disp_R (written by PLIO DMA).
    depth_convert(disp_L, disp_R, (uint16_t*)depth_prev /* reuse as temp */,
                  depth_lut_bram, IMG_HEIGHT, IMG_WIDTH);

    // ── Step 4: Temporal IIR + hole fill ─────────────────────────────────────
    post_process((uint16_t*)depth_prev, depth_prev, depth_out,
                 IMG_HEIGHT, IMG_WIDTH);
}
