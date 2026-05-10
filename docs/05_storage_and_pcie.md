# Storage Architecture & PCIe

## The Core Problem: Data Rates

```
Raw dual 4K @ 30fps (Raw12):
  3840 × 2160 × 12-bit × 2 sensors × 30fps ÷ 8 = ~750 MB/s continuous

Depth maps @ 30fps (16-bit disparity):
  3840 × 2160 × 2 bytes × 30fps = ~480 MB/s

Total uncompressed pipeline output: ~1.2 GB/s
```

No storage medium keeps up with raw output — compression before writing is mandatory.

---

## Storage Tiers

| Tier | Medium | Capacity | Bandwidth | Role |
|---|---|---|---|---|
| L1 — Working | DDR4/LPDDR4 | 8–16 GB | ~17 GB/s | Frame buffers, in-flight processing |
| L2 — Fast persist | NVMe via PCIe x4 | 256 GB–2 TB | ~1–3 GB/s | Compressed video, depth sequences |
| L3 — Boot/config | eMMC 5.1 | 32–64 GB | ~300 MB/s | OS, calibration, logs |
| L4 — Archive | USB 3.0 external | Unlimited | ~400 MB/s | Field data offload |
| On-chip | FPGA BRAM (~8–16 Mb) | ~1–2 MB | GB/s range | LUTs, line buffers, FIFOs only |

---

## Compression Impact

| Stream | Raw Rate | After Compression | Ratio |
|---|---|---|---|
| RGB video (H.265) | 750 MB/s | 2–5 MB/s | 150:1 |
| Depth maps (lossless) | 480 MB/s | 80–120 MB/s | 4–6:1 |
| Depth maps (lossy) | 480 MB/s | 5–15 MB/s | 30–100:1 |
| Point clouds (LAS/LZ4) | variable | 10–50 MB/s | — |

---

## H.265 Encoder — Critical Decision

PolarFire SoC has **no hardened H.265 encoder**. Options:

| Option | Verdict |
|---|---|
| FPGA soft H.265 encoder | Possible but consumes significant fabric resources |
| External encoder IC (Hantro H1/H2 via PCIe) | Best throughput, offloads fabric |
| Software encode on RISC-V | Too slow (~1–2fps at 4K) — not viable |
| Record at reduced resolution/framerate | Simplest — record decimated RGB, keep full-res depth |
| Raw NVMe burst recording | NVMe sustains ~1 GB/s; ~1–2 seconds of raw dual-4K buffer |

**Note:** Zynq UltraScale+ ZU5EV has a hardened H.265 encoder — a strong reason to prefer it over PolarFire SoC for this application. See `08_soc_alternatives.md`.

---

## PCIe Slot (M.2 NVMe)

### PolarFire SoC PCIe Capability
- Hardened **PCIe Gen2 x4 Root Complex** — no fabric resources consumed
- Maps directly into AXI fabric interconnect
- Both RISC-V MSS and FPGA DMA engines can master PCIe transactions

### Slot Specification
- **M.2 M-key 2280** (PCIe x4 Gen2) — primary NVMe
- **M.2 E-key** (PCIe x1 + USB 2.0) — optional WiFi/BT

### Board-Level Additions
| Component | Part | Notes |
|---|---|---|
| M.2 M-key connector | Molex 2199230400 | |
| M.2 E-key connector | Amphenol MDT420M01001 | |
| NVMe power switch | TPS2561DRCR | MSS GPIO-controlled power cycle |
| 3.3V buck for M.2 | TPS62130ARGR | Up to 3A peak for NVMe burst |

### Signal Integrity
| Requirement | Spec |
|---|---|
| Differential impedance | 85Ω ±10% |
| Max trace length | ≤ 8 inches (Gen2) |
| Within-lane pair skew | < 5 mil |
| AC coupling caps | 100nF per TX lane, within 200 mil of connector |
| Via stubs | Back-drill if > 6 layers |

**Layout rule:** Place M.2 slot as close to PCIe Root Complex as possible; route PCIe before power and signal nets.

---

## Bandwidth Validation

```
PCIe Gen2 x4 theoretical max:    ~2.0 GB/s
NVMe SSD sustained write:         ~1.0–1.5 GB/s
Required write rate (compressed): ~82 MB/s

Headroom: ~12× — PCIe link is not the bottleneck
```

NVMe write **endurance (TBW)** is the practical concern for long continuous recording sessions.

---

## Storage Budget Example (1 TB NVMe)

```
Recording @ 30fps, compressed:
  RGB H.265 (15 Mbps)       =  1.9 MB/s
  Depth lossless (80 MB/s)  = 80.0 MB/s
  Metadata + logs           =  0.1 MB/s
  Total                     ~ 82 MB/s

1 TB NVMe at 82 MB/s = ~3.4 hours continuous

At 30 Mbps depth (lossy):   12+ hours on 1 TB
```

---

## DMA Path (FPGA → NVMe)

```
FPGA Disparity Engine ──▶ AXI DMA ──▶ DDR ring buffer ──▶ Linux kernel DMA ──▶ NVMe (f2fs)
```

- Zero CPU copy in critical path when using `dma_buf` / zero-copy kernel path
- DDR acts as large ring buffer — drain asynchronously to NVMe
- **Filesystem:** f2fs over ext4 for NVMe — better sustained write performance for sequential recording workloads
