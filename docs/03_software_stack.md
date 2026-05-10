# Software / Firmware Stack

## Stack Overview

```
┌─────────────────────────────────────────────────────┐
│           User Application Layer                    │
│  (depth map consumer, SLAM, object detection)       │
├─────────────────────────────────────────────────────┤
│         Depth Processing Library                    │
│  (calibration, filtering, point cloud generation)   │
├─────────────────────────────────────────────────────┤
│       Camera Abstraction Layer                      │
│  (V4L2 drivers or custom FPGA AXI DMA driver)       │
├─────────────────────────────────────────────────────┤
│      Linux (OpenSBI + U-Boot + HSS)                 │
│       on RISC-V application cores                   │
├─────────────────────────────────────────────────────┤
│   HSS (Hart Software Services) Bootloader           │
│     Microchip bare-metal SoC init                   │
└─────────────────────────────────────────────────────┘
```

---

## Boot Sequence

| Stage | Component | Role |
|---|---|---|
| 1 | HSS (Hart Software Services) | First-stage bootloader; initializes DDR, clocks, PCIe; loads U-Boot |
| 2 | U-Boot | Second-stage boot; loads Linux kernel from eMMC |
| 3 | Linux kernel | Microchip maintains PolarFire SoC BSP — use their Yocto layer |
| 4 | Userspace | Application, drivers, depth pipeline |

**Yocto BSP:** `meta-polarfire-soc-yocto-bsp` (Microchip GitHub)

---

## Kernel Drivers Required

| Driver | Purpose |
|---|---|
| Custom AXI DMA driver (kernel module) | Pull frames and depth maps from FPGA into Linux userspace via mmap/DMA-BUF |
| DRM/KMS driver | Display controller for MIPI DSI panel |
| NVMe (built-in) | `CONFIG_NVME_CORE`, `CONFIG_BLK_DEV_NVME` |
| PCIe host | `CONFIG_PCIE_MICROCHIP_HOST` (mainlined Linux 5.14+) |
| Ethernet | VSC8541 RGMII PHY driver |

---

## Depth Post-Processing (Userspace)

| Task | Implementation |
|---|---|
| Temporal filtering | IIR filter on consecutive depth maps — reduces noise at cost of motion latency |
| Hole filling | Fill occluded/invalid pixels from neighboring valid disparity values |
| Confidence masking | Reject pixels with high disparity variance (unreliable matches) |
| Point cloud generation | `depth = (f × B) / d` per pixel → (X, Y, Z) in camera frame |
| SLAM | ORB-SLAM3 — heavy; run on downsampled depth input on 4× RISC-V A53 cores |

---

## Calibration Tool

Run offline using OpenCV on a host PC:

```python
# Key OpenCV functions
cv2.calibrateCamera()     # per-sensor intrinsics
cv2.stereoCalibrate()     # stereo extrinsics (R, T)
cv2.stereoRectify()       # compute rectification maps
cv2.initUndistortRectifyMap()  # generate per-pixel remap LUTs
```

Export remap LUTs → burn to FPGA BRAM at boot via U-Boot script or HSS payload.

**Target:** RMS reprojection error < 0.3px. Re-run calibration if mechanical mount is disturbed.

---

## Display UI

| Option | Use case | Notes |
|---|---|---|
| **LVGL on framebuffer** | Lightweight embedded UI | Best fit — runs well on RISC-V, minimal dependencies |
| Wayland + weston | Full desktop compositor | Heavier; use if complex UI needed |
| Direct framebuffer write | Minimal/debug UI | Simplest possible |

---

## Recording Pipeline

```
FPGA DMA ──▶ DDR ring buffer ──▶ kernel DMA ──▶ NVMe (f2fs)
                                    │
                                    ├──▶ H.265 encoder (hardware IC or SW) ──▶ RGB video file
                                    └──▶ Lossless depth encoder ──▶ depth sequence file
```

**Filesystem:** f2fs recommended over ext4 for NVMe — optimized for flash write patterns of continuous recording.

**Depth encoding:** PNG sequence (lossless) or custom RLE. Do not lossy-compress depth data without careful validation.

---

## Post-Processing Feasibility on RISC-V

| Task | Feasible? | Notes |
|---|---|---|
| Temporal filter / hole fill | Yes | Runs easily in userspace |
| Point cloud generation | Yes | ~10–50ms per frame |
| SLAM (ORB-SLAM3) | Marginal | Run on downsampled input; 4× A53 cores at ~1.7GHz will struggle at full 4K/30fps |
| Object detection on depth | Yes (limited) | Run on downsampled depth map |
| H.265 software encode | No | ~1–2fps realistic on RISC-V — needs hardware encoder |
