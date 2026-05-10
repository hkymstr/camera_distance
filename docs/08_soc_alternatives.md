# SoC / FPGA Alternatives

Alternatives to the PolarFire SoC MPFS250T (~$385) for the dual 4K stereo depth pipeline.

---

## Comparison Table

| Device | Chip Cost | Processor | Fabric LUTs | PCIe | MIPI CSI-2 | H.265 Encoder | Toolchain |
|---|---|---|---|---|---|---|---|
| **PolarFire SoC MPFS250T** (baseline) | ~$385 | 4× RISC-V @ 1.7GHz | ~254K | Gen2 x4 (hard) | Via fabric | No | Libero SoC |
| **Zynq UltraScale+ ZU3EG** | ~$230 | 4× ARM A53 @ 1.3GHz | ~154K | Gen2 x4 (hard) | Via fabric | No | Vivado |
| **Zynq UltraScale+ ZU5EV** | ~$310 | 4× ARM A53 + 2× R5 | ~256K | Gen2 x4 (hard) | Native 4-lane | Yes | Vivado |
| **AMD Kria K26 SOM** | $419 (SOM) | 4× ARM A53 + 2× R5 | ~256K | Gen2 x4 (hard) | Native 4-lane | Yes | Vivado + Ubuntu |
| **Intel Cyclone V SoC (5CSEBA6)** | ~$100 | 2× ARM A9 @ 800MHz | ~110K | Gen1 x4 (hard) | No | No | Quartus Prime |
| **Zynq-7045** | ~$240 | 2× ARM A9 @ 1GHz | ~218K | Gen1 x4 (hard) | No | No | Vivado |

---

## Option 1 — Zynq UltraScale+ ZU3EG (~$230) — Best Price/Performance

**Save ~$155 vs MPFS250T.**

- Quad ARM Cortex-A53 at 1.3GHz — significantly faster than PolarFire RISC-V for Linux workloads
- 154K LUTs is less than MPFS250T — may require Census Transform instead of full SGM, or reduced resolution
- PCIe Gen2 x4 hard IP — same capability as PolarFire
- No native MIPI D-PHY — same situation as PolarFire; use IP in fabric or external PHY
- Vivado is the industry-standard FPGA toolchain — better IP ecosystem than Libero SoC
- PetaLinux BSP is mature and well-documented

**Tradeoff:** No hardware H.265 encoder; slightly less fabric.

---

## Option 2 — Zynq UltraScale+ ZU5EV (~$310) — Recommended Overall

**Save ~$75 vs MPFS250T — with significantly more capability.**

- Quad ARM A53 + dual R5 real-time cores
- 256K LUTs — matches MPFS250T fabric
- **Native MIPI CSI-2 interface** — eliminates MIPI D-PHY IP from fabric, freeing resources
- **Hardware H.264/H.265 encoder** — solves video recording problem at no added cost
- Hardware JPEG encoder
- Mali-400 GPU for display compositing
- Net effect: remove external H.265 encoder IC, eliminate MIPI D-PHY IP concerns, faster Linux performance

**This is the strongest overall recommendation for this system.**

### BOM Delta vs MPFS250T

| Line Item | MPFS250T | ZU5EV | Delta |
|---|---|---|---|
| SoC | $385 | $310 | -$75 |
| H.265 encoder IC | not solved | included | -$30–50 saved |
| MIPI D-PHY IP | potentially paid | eliminated | -$0–$2,000 NRE |
| **Net prototype saving** | | | **~-$150** |

---

## Option 3 — AMD Kria K26 SOM ($419) — Fastest Time to Working Hardware

Higher chip-level cost, but the SOM includes hardware that would otherwise be on your BOM:

| Included on K26 SOM | BOM Savings |
|---|---|
| 4GB DDR4 | ~$76 |
| 16GB eMMC | ~$18 |
| Full PMIC (power tree) | ~$25 |
| Aluminum heatspreader | ~$8 |
| Security module (TPM) | ~$5 |
| **Total savings** | **~$132** |

**Net Kria K26 premium over bare ZU5EV: ~$109. Net BOM savings: ~$132. Kria K26 is ~$23 cheaper system-level.**

Additional advantages:
- Pre-certified FCC/CE — reduces your own certification burden
- AMD ships Ubuntu 22.04 with full hardware acceleration pre-configured
- MIPI CSI-2 connectors defined in SOM spec — carrier board just needs FPC connectors
- M.2 and PCIe already broken out in SOM carrier spec

**Tradeoff:** Constrained to K26 pinout for carrier board design. Less freedom, much faster bring-up.

---

## Option 4 — Intel Cyclone V SoC (~$100) — Budget Only

**Save ~$285, but with significant caveats:**

- Only 110K LEs — stereo disparity pipeline alone fills 60–70% of fabric
- Dual ARM Cortex-A9 at 800MHz — slower for Linux + post-processing
- PCIe Gen1 only — NVMe capped at ~250 MB/s (enough for compressed recording)
- No native MIPI — requires external MIPI bridge chip (~$15)
- **Only viable if you drop to 1080p processing** — 4K dual-sensor throughput will overwhelm 110K LEs
- Cyclone V is a 2013-era part — EOL risk on long programs

**Only recommended if budget is the absolute primary constraint and 1080p resolution is acceptable.**

---

## Decision Guide

| Goal | Recommendation |
|---|---|
| Lowest chip cost, accept 1080p | Cyclone V SoC (~$100) |
| Best price/performance, full 4K | **ZU3EG (~$230)** |
| Best features at near-same price | **ZU5EV (~$310)** ← recommended |
| Fastest time to working hardware | **Kria K26 SOM ($419, net ~$23 cheaper system-level)** |
| Keep PolarFire (RISC-V, low power, security focus) | MPFS250T |
