# Hardware Architecture

## System Overview

**Goal:** Dual 4K stereo camera with real-time depth estimation  
**Primary SoC:** Microchip PolarFire SoC MPFS250T  
- Quad RISC-V 64-bit application cores + FPGA fabric
- FPGA handles pixel-level throughput; RISC-V runs OS and higher-level algorithms

---

## Sensor Selection

| Criterion | Recommendation |
|---|---|
| Resolution | 4K (3840×2160 or 4056×3040) |
| Interface | MIPI CSI-2 (4-lane per sensor) or SLVS-EC |
| Candidate sensors | Sony IMX585 (8MP, MIPI), Sony IMX334 (8MP MIPI), OmniVision OV16B10 |
| Frame rate | 30fps minimum at 4K; 60fps preferred for depth accuracy |
| Shutter | Global shutter strongly preferred — eliminates rolling shutter parallax error |
| Baseline | 10–20cm physical separation; 15cm recommended starting point |

---

## Board-Level Block Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        Carrier Board                            │
│                                                                 │
│  ┌──────────┐  MIPI CSI-2   ┌──────────────────────────────┐  │
│  │ Sensor L │──4L, 12Gbps──▶│                              │  │
│  └──────────┘               │      PolarFire SoC           │  │
│                             │                              │  │
│  ┌──────────┐  MIPI CSI-2   │  FPGA Fabric  │  RISC-V MSS │  │
│  │ Sensor R │──4L, 12Gbps──▶│               │             │  │
│  └──────────┘               └──────┬─────────┴──────┬──────┘  │
│                                    │                │          │
│  ┌──────────────┐          ┌───────▼──┐      ┌──────▼──────┐  │
│  │ LPDDR4x 16GB │◀─────────│ Mem Ctrl │      │  Gigabit    │  │
│  └──────────────┘          └──────────┘      │  Ethernet   │  │
│                                              └─────────────┘  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────────┐  │
│  │  PCIe x4 │  │   USB 3  │  │ eMMC/SD  │  │ Power Mgmt   │  │
│  │ (NVMe)   │  │ (debug)  │  │ (OS)     │  │ (PMIC/LDOs)  │  │
│  └──────────┘  └──────────┘  └──────────┘  └──────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Key Hardware Decisions

### Synchronization
- Hardware sync line between both sensors (shared FSIN/VSYNC signal)
- Critical for stereo — unsynchronized frames produce incorrect depth maps
- Common MCLK distributed to both sensors from a single VCXO or Si5341 jitter cleaner

### Memory Bandwidth
- Two 4K sensors at 30fps Raw12 ≈ ~3 GB/s aggregate
- LPDDR4x at 4266 MT/s with 32-bit bus gives ~17 GB/s peak — ample headroom

### Power Domains
| Domain | Voltage | Notes |
|---|---|---|
| Sensor analog | 2.8V | Low-noise LDO — critical for SNR |
| Sensor digital | 1.2V | LDO |
| FPGA core | 1.0V | High-current buck |
| FPGA I/O / DDR | 1.8V | Buck |
| System 3.3V | 3.3V | M.2, misc I/O |
| Total system | — | ~15–25W depending on algorithm load |

---

## PCIe Slot — M.2 NVMe

### Format
- M.2 M-key 2280 (PCIe x4 Gen2) — primary NVMe storage
- M.2 E-key (PCIe x1 + USB 2.0) — optional WiFi/BT module

### Signal Integrity Requirements

| Requirement | Spec |
|---|---|
| Differential pair impedance | 85Ω ±10% |
| Max trace length | ≤ 8 inches (20cm) for Gen2 |
| Pair-to-pair skew (within lane) | < 5 mil length mismatch |
| Reference plane | Continuous ground — no splits under PCIe traces |
| AC coupling caps | 100nF per TX lane, within 200 mil of connector |

### M.2 Power Rail

| Rail | Current | Notes |
|---|---|---|
| 3.3V | Up to 3A peak | Dedicated buck; add TPS2561 power switch for MSS-controlled power cycle |
| 3.3V aux (M2_WAKE) | ~1 mA | Always-on |
| PERST# | GPIO from MSS | Open-drain, 100Ω series |

---

## Calibration Flow

1. **Intrinsic calibration** — checkerboard per sensor (focal length, principal point, distortion)
2. **Stereo extrinsic calibration** — find R, T between sensor optical centers
3. **Rectification** — compute homography to align epipolar lines horizontally
4. **Accuracy check** — RMS reprojection error < 0.3px
5. **Storage** — burn to eMMC/flash; loaded at boot and pushed to FPGA LUTs

---

## Development Phases

| Phase | Deliverable |
|---|---|
| 1 | Dev board bring-up (see prototype path doc) |
| 2 | Single sensor streaming to DDR, Linux display |
| 3 | Dual sensor sync, raw stereo frames to host |
| 4 | Rectification in FPGA, verify epipolar alignment |
| 5 | Disparity engine in fabric, depth map output |
| 6 | Full calibration pipeline, accuracy characterization |
| 7 | Custom PCB integration, power optimization |
