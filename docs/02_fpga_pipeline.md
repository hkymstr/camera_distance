# FPGA Fabric Pipeline

## Overview

The FPGA fabric handles everything that must run at pixel clock rates.  
The RISC-V MSS handles OS, calibration, and higher-level post-processing.

---

## Full Pipeline Block Diagram

```
Sensor L ──▶ [D-PHY RX] ──▶ [CSI-2 Demux] ──▶ [Debayer/RAW] ──▶ ┐
                                                                   ├──▶ [Sync FIFO] ──▶ [ISP] ──▶ [Rectification] ──▶ [Disparity Engine] ──▶ DDR
Sensor R ──▶ [D-PHY RX] ──▶ [CSI-2 Demux] ──▶ [Debayer/RAW] ──▶ ┘
                                                                          │
                                                               ┌──────────▼──────────┐
                                                               │   Compositor / OSD  │◀── UI overlays (RISC-V)
                                                               │  (blend RGB + depth) │
                                                               └──────────┬──────────┘
                                                                          │
                                                               ┌──────────▼──────────┐
                                                               │  Display Controller  │
                                                               │  (timing gen, VSYNC) │
                                                               └──────────┬──────────┘
                                                                          │
                                                               ┌──────────▼──────────┐
                                                               │   MIPI DSI TX        │──▶ Panel
                                                               │   (+ HDMI bridge)    │──▶ External monitor
                                                               └─────────────────────┘
```

---

## Block Descriptions

| Block | Function | Notes |
|---|---|---|
| MIPI D-PHY RX | Deserialize high-speed lanes | Use Microchip IP or open-source; PolarFire has hardened SerDes |
| CSI-2 Controller | Packet parsing, ECC, header decode | Available as Microchip IP core |
| Frame Buffer (DMA) | Write raw frames to DDR | Ping-pong buffers; AXI4 DMA |
| ISP Pipeline | Demosaic, black level, white balance, gamma | Pipelines at full 4K rate in fabric |
| Rectification Engine | Undistort + stereo-align both images | Pre-computed remap LUTs in BRAM; bilinear interpolation in fabric |
| Disparity Engine | Stereo matching — see algorithm options below | Most resource-intensive block |
| Depth Map Output | Convert disparity → metric depth | `depth = (focal_length × baseline) / disparity` |
| Compositor / OSD | Blend RGB + depth layers + UI overlay | Alpha-blendable layer stack |
| Display Controller | Timing generation, VSYNC, pixel clock | Drives MIPI DSI TX |

---

## Disparity Algorithm Options

| Algorithm | Quality | FPGA Resources | Latency | Notes |
|---|---|---|---|---|
| SAD (Sum of Absolute Differences) | Low | Low | < 1 frame | Poor on textureless regions |
| Census Transform | Medium | Medium | < 1 frame | Robust to illumination changes |
| SGM (Semi-Global Matching) | High | High | ~1 frame | Best quality; may require MPFS460T |

**Recommendation:** Start with Census Transform for resource budget estimation; prototype SGM in simulation before committing to device.

### Disparity-to-Depth Conversion (per pixel)

```
depth_m = (focal_length_px × baseline_m) / disparity_px
```

Implemented as a LUT or fixed-point divider in fabric — zero CPU cost.

---

## Compositor / OSD Layers

| Layer | Content | Notes |
|---|---|---|
| 0 | Rectified RGB (left sensor reference view) | Full resolution |
| 1 | Depth map — heat-map colorized | Near = red, far = blue; 256-entry palette LUT in BRAM |
| 2 | OSD overlay | Framerate, depth at cursor, status text; written by RISC-V |

Alpha blend between layers is configurable from RISC-V at runtime.

---

## Resource Budget Guidance

| Block | Estimated LUT Usage (MPFS250T) |
|---|---|
| Dual MIPI D-PHY RX + CSI-2 | ~8K LUTs |
| ISP (demosaic, WB, gamma) | ~15K LUTs |
| Rectification (bilinear remap) | ~10K LUTs |
| Census Transform disparity | ~40K LUTs |
| SGM disparity (full) | ~80–120K LUTs |
| Compositor + display controller | ~12K LUTs |
| DMA + AXI interconnect | ~10K LUTs |
| **Total (Census)** | **~95K LUTs** |
| **Total (SGM)** | **~155–195K LUTs** |

MPFS250T has 254K LEs — SGM fits with margin. On MPFS095T (Discovery Kit) SGM will not fit at 4K.

---

## Key Design Notes

- **Ping-pong frame buffers in DDR** — never read and write the same buffer simultaneously
- **Epipolar constraint** — after rectification, disparity search is strictly horizontal (1D), which is what makes fabric implementation tractable
- **Disparity search range** — 256px default; larger range increases minimum detectable distance but costs fabric resources and latency
- **Sub-pixel refinement** — parabolic interpolation on the cost curve to achieve ±0.5px disparity resolution; implement in fixed-point arithmetic
