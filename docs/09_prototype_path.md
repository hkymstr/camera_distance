# Prototype Path & Dev Board Options

## Why Not the PolarFire SoC Discovery Kit?

The **MPFS-DISCO-KIT** ($139) was evaluated with the Raspberry Pi AI Camera ($70, Sony IMX500).

| Issue | Detail |
|---|---|
| **Single CSI port** | Discovery Kit has one MIPI CSI-2 connector — stereo depth requires two synchronized cameras |
| **MPFS095T fabric** | 93K LEs — too small for the full stereo pipeline at 4K (need ~155K+ for SGM) |
| **IMX500 rolling shutter** | RPi AI Camera uses rolling shutter — causes parallax errors in stereo matching |
| **IMX500 at 4K** | Full 4056×3040 resolution runs at only 10fps (not 30fps); 30fps requires 2028×1520 |
| **IMX500 software** | On-chip AI inference is tied to RPi libcamera/AITRIOS stack — not supported on PolarFire Linux |

**The Discovery Kit is suitable for:** Linux bring-up, MIPI streaming from a single camera, display pipeline testing, basic 1080p ISP work.  
**It cannot support:** Stereo depth, 4K@30fps, production-equivalent fabric workload.

---

## Recommended Two-Phase Prototype Path

### Phase 1 — Algorithm Validation (~$130 total)

**Hardware:** Raspberry Pi 5 + 2× Raspberry Pi Camera Module 3

| Item | Cost |
|---|---|
| Raspberry Pi 5 (4GB) | ~$60 |
| RPi Camera Module 3 (×2) | ~$25 each |
| FFC cables, mounts | ~$20 |
| **Total** | **~$130** |

**Why RPi 5:**
- Has two native CSI-2 ports — true dual camera support out of the box
- Full `libcamera` support for Camera Module 3 (IMX708, 12MP, global shutter option available)
- Run OpenCV stereo calibration, rectification, and SGM disparity entirely in software
- Validates algorithm correctness before committing to FPGA implementation
- Much faster iteration — Python/C++ changes take seconds, not FPGA synthesis hours

**What you prove in Phase 1:**
- Stereo calibration workflow (intrinsics, extrinsics, rectification LUTs)
- SGM disparity quality vs Census Transform vs SAD
- Depth map accuracy at various distances against ground truth
- Post-processing pipeline (temporal filter, hole fill, point cloud)
- Display overlay compositing logic

**What Phase 1 does not prove:**
- Real-time 4K FPGA pipeline performance
- Latency at 30fps
- Power consumption

---

### Phase 2 — FPGA Stereo Bring-Up

Move to FPGA dev board once algorithm is proven. Best options:

#### Option A — AMD KV260 Vision AI Starter Kit (~$249) — Recommended

| Spec | Value |
|---|---|
| SoC | Zynq UltraScale+ ZU5EV |
| Fabric | ~256K LUTs |
| Camera ports | 2× MIPI CSI-2 (Raspberry Pi compatible FPC) |
| H.265 encoder | Yes (hardened) |
| Display | MIPI DSI + HDMI |
| Storage | M.2 NVMe slot |
| OS | Ubuntu 22.04 pre-configured |

- **Two camera ports** — plug in two RPi Camera Module 3 or compatible MIPI cameras directly
- AMD ships a stereo vision reference design for KV260
- Vivado + Vitis AI toolchain — large community
- H.265 encoder solves the video recording problem from day one
- Closest to the ZU5EV production SoC (recommended in `08_soc_alternatives.md`)

**Compatible cameras for KV260 stereo:**
- Raspberry Pi Camera Module 3 (IMX708) — rolling shutter, $25 each
- Arducam IMX296 Global Shutter Module — global shutter, ~$30 each (preferred for depth accuracy)
- e-con Systems See3CAM_27CUG (IMX900) — global shutter, ~$80 each

#### Option B — PolarFire SoC Video Kit (~$499)

| Spec | Value |
|---|---|
| SoC | MPFS250T (full production target) |
| Fabric | ~254K LUTs |
| Camera | Video-specific connectors |
| Display | HDMI |

- Uses the exact production SoC — direct fabric transfer to custom board
- More expensive; fewer camera ecosystem options than KV260
- Choose this if you are committed to PolarFire for production

#### Option C — PolarFire SoC Icicle Kit (~$399)

- MPFS250T, more I/O expansion than Discovery Kit
- Still requires custom camera breakout boards for dual MIPI
- Less camera ecosystem support than KV260

---

## Phase 3 — Custom PCB

Once Phase 2 validates the stereo pipeline:

1. Transfer FPGA fabric design (bitstream + IP) to custom carrier board
2. Use BOM from `07_bom.md` as the starting component list
3. Replace dev board cameras with FRAMOS FSM-IMX585 modules (global shutter variant preferred)
4. Add rigid stereo baseline bracket with known geometry
5. Re-run full stereo calibration on assembled hardware

---

## Summary

```
Phase 1: RPi 5 + 2× RPi Camera Module 3    (~$130)
         ↓ prove algorithm, calibration, depth pipeline

Phase 2: AMD KV260 + 2× Arducam IMX296     (~$309)
         ↓ prove FPGA implementation, real-time performance

Phase 3: Custom PCB (BOM from doc 07)       (~$2,003 prototype)
         ↓ production design
```

Total prototype investment before custom PCB: **~$440**
