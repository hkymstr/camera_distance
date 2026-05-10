# Camera Distance System

Dual 4K stereo depth camera system design using a PolarFire SoC (or equivalent).  
Capable of real-time distance measurement, display output, and NVMe-backed recording.

## Project Goals

- Read dual 4K image sensors simultaneously
- Compute stereo disparity and generate real-time depth maps
- Display RGB + depth overlay on an embedded panel
- Record compressed video and depth sequences to NVMe storage
- Run Linux on RISC-V (or ARM) application cores for post-processing

## Repository Structure

```
docs/
  01_hardware_architecture.md   Board-level hardware design
  02_fpga_pipeline.md           FPGA fabric block pipeline
  03_software_stack.md          Linux/RTOS software stack
  04_display_subsystem.md       Display hardware and compositor
  05_storage_and_pcie.md        Storage architecture and PCIe slot
  06_depth_analysis.md          Stereo depth range and accuracy analysis
  07_bom.md                     Full Bill of Materials with pricing
  08_soc_alternatives.md        Alternative SoC/FPGA options vs PolarFire
  09_prototype_path.md          Recommended dev board prototype path
```

## Quick Reference — Depth Performance

| Range | Accuracy | Use |
|---|---|---|
| 1.6m – 5m | ~1–30mm | Manipulation, precise obstacle detection |
| 5m – 20m | ~30mm–12cm | Navigation, person/vehicle detection |
| 20m – 40m | ~50cm | Object presence detection |
| > 40m | Noise-dominated | Not reliable |

Baseline: 15cm. Lens: 8mm. Sensor: Sony IMX585 4K.

## Primary SoC

**Microchip PolarFire SoC MPFS250T** — quad RISC-V 64-bit cores + 254K LE FPGA fabric.  
See [docs/08_soc_alternatives.md](docs/08_soc_alternatives.md) for cheaper alternatives,  
including the recommended **Zynq UltraScale+ ZU5EV** (~$75 cheaper, more features).

## Prototype Path

See [docs/09_prototype_path.md](docs/09_prototype_path.md).  
Short version:
1. **Phase 1** — Raspberry Pi 5 + 2× RPi Camera Module 3 (~$130) — algorithm validation
2. **Phase 2** — AMD KV260 Vision AI Kit (~$249) — FPGA stereo bring-up
3. **Phase 3** — Custom PCB from BOM — production
