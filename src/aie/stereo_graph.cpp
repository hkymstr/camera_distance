// AIE ADF graph implementation – Versal AI Core (VC1902)
#include "stereo_graph.h"

using namespace adf;

StereoAIEGraph::StereoAIEGraph() {

    // ── PLIO declarations ────────────────────────────────────────────────────
    // plio_128_bits matches the 128-bit PL↔AIE interface chosen in stereo_config.h
    left_plio  = input_plio::create("left_plio",  plio_128_bits, "left_pixels.txt");
    right_plio = input_plio::create("right_plio", plio_128_bits, "right_pixels.txt");
    disp_plio  = output_plio::create("disp_plio", plio_128_bits, "disparity.txt");

    // ── Census kernels ───────────────────────────────────────────────────────
    const int stripe_h = IMG_HEIGHT / AIE_CENSUS_TILES_PER_IMG;   // 540 rows each
    for (int i = 0; i < AIE_CENSUS_TILES_PER_IMG; ++i) {
        census_L[i] = kernel::create(census_kernel);
        census_R[i] = kernel::create(census_kernel);

        source(census_L[i]) = "aie/census_kernel.cpp";
        source(census_R[i]) = "aie/census_kernel.cpp";

        runtime<ratio>(census_L[i]) = 0.8;
        runtime<ratio>(census_R[i]) = 0.8;

        // Row-range RTP (run-time parameter)
        connect<parameter>(census_L[i].in[1], async(census_L[i].in[1]));
        connect<parameter>(census_L[i].in[2], async(census_L[i].in[2]));
        connect<parameter>(census_R[i].in[1], async(census_R[i].in[1]));
        connect<parameter>(census_R[i].in[2], async(census_R[i].in[2]));

        // Pixel input from PLIO (broadcast to all stripes; kernel ignores out-of-range rows)
        connect<stream>(left_plio.out[0],  census_L[i].in[0]);
        connect<stream>(right_plio.out[0], census_R[i].in[0]);

        // Tile location constraints (row i, columns 0/1 for L/R)
        location<kernel>(census_L[i]) = tile(i, 0);
        location<kernel>(census_R[i]) = tile(i, 1);
    }

    // ── Hamming distance kernels ─────────────────────────────────────────────
    // Each of the 16 hamming tiles covers DISP_SLICE=16 disparities.
    // Left census is broadcast to all 16 tiles; right census likewise.
    // The disp_start RTP selects which disparity slice each tile handles.
    for (int i = 0; i < AIE_HAMMING_TILES; ++i) {
        hamming[i] = kernel::create(hamming_kernel);
        source(hamming[i]) = "aie/hamming_kernel.cpp";
        runtime<ratio>(hamming[i]) = 0.9;

        // Census streams come from all 4 left/right census tiles (merged)
        // For VC1902 routing, connect from tile 0 representative; the NoC
        // router will replicate across the remaining census tiles.
        connect<stream>(census_L[0].out[0], hamming[i].in[0]);
        connect<stream>(census_R[0].out[0], hamming[i].in[1]);

        // disp_start RTP
        connect<parameter>(hamming[i].in[2], async(hamming[i].in[2]));

        location<kernel>(hamming[i]) = tile(4 + (i / 4), i % 4);
    }

    // ── SGM path-aggregation kernels ─────────────────────────────────────────
    // 4 paths: L→R (0), R→L (1), T→B (2), B→T (3)
    for (int i = 0; i < SGM_NUM_PATHS; ++i) {
        sgm[i] = kernel::create(sgm_path_kernel);
        source(sgm[i]) = "aie/sgm_path_kernel.cpp";
        runtime<ratio>(sgm[i]) = 0.9;

        // Cost input: aggregate from all hamming tiles (the NoC merges them)
        connect<stream>(hamming[0].out[0], sgm[i].in[0]);

        // Direction RTP
        connect<parameter>(sgm[i].in[1], async(sgm[i].in[1]));

        location<kernel>(sgm[i]) = tile(8 + i, 0);
    }

    // ── Winner-take-all + left-right consistency kernels ────────────────────
    // 4 WTA tiles, each covering one quarter of the image rows.
    // They receive the 4 aggregated path streams and sum them, then pick
    // argmin_d and perform left-right check.
    for (int i = 0; i < AIE_WTA_TILES; ++i) {
        wta[i] = kernel::create_object<WTAKernel>();
        source(wta[i]) = "aie/wta_kernel.cpp";
        runtime<ratio>(wta[i]) = 0.9;

        for (int p = 0; p < SGM_NUM_PATHS; ++p)
            connect<stream>(sgm[p].out[0], wta[i].in[p]);

        connect<stream>(wta[i].out[0], disp_plio.in[0]);

        location<kernel>(wta[i]) = tile(12 + i, 0);
    }
}
