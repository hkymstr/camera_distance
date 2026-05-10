# Bill of Materials

**Pricing basis:** 1–5 unit prototype quantities, May 2026.  
Items marked **RFQ** require a formal quote. All prices USD.

---

## 1. Primary SoC

| Qty | Part | MPN | Unit Cost | Total | Notes |
|---|---|---|---|---|---|
| 1 | PolarFire SoC MPFS250T (484-FCBGA) | MPFS250T-FCVG484I | ~$385 | $385 | Digi-Key; upgrade to MPFS460T (~$625) if SGM disparity overflows fabric |

---

## 2. Image Sensors & Camera Modules

| Qty | Part | MPN / Vendor | Unit Cost | Total | Notes |
|---|---|---|---|---|---|
| 2 | Sony IMX585 8.3MP CMOS Sensor Module (MIPI CSI-2, C/CS mount) | FSM-IMX585C-04G-V1A — FRAMOS | **RFQ ~$220–$300** | ~$500 | Rolling shutter; global shutter variant (IMX565) costs ~30% more |
| 2 | M12 Lens, 4K-rated, 8mm FL | e.g. Kowa LM8NCL or equivalent | ~$45 | $90 | Match to sensor format (1/1.2") |
| 1 | Stereo lens baseline mount/bracket | Custom or off-shelf stereo rig | ~$40 | $40 | Rigid aluminum — critical for calibration stability |

---

## 3. Memory

| Qty | Part | MPN | Unit Cost | Total | Notes |
|---|---|---|---|---|---|
| 2 | LPDDR4x 8GB die (×2 = 16GB total), 4266 MT/s | Micron MT53E4G32D4NQ-046 or equiv. | ~$38 | $76 | LPDDR4x being phased out — order early or qualify Samsung K4UBE3D4AA-MGCL as alt |
| 1 | eMMC 5.1 64GB (BGA153) | Kingston EMMC08G-TB29-P15C or Micron MTFC64GAJAEE | ~$18 | $18 | OS + calibration data |

---

## 4. Storage

| Qty | Part | MPN | Unit Cost | Total | Notes |
|---|---|---|---|---|---|
| 1 | NVMe SSD 1TB, M.2 2280, PCIe Gen3 x4 | Samsung 980 Pro MZ-V8P1T0B | ~$100 | $100 | Gen3 sufficient — PolarFire PCIe is Gen2 |
| 1 | M.2 M-key 2280 connector | Molex 2199230400 | ~$3 | $3 | |
| 1 | M.2 E-key connector (WiFi/BT slot) | Amphenol MDT420M01001 | ~$3 | $3 | |
| 1 | NVMe power switch IC | TPS2561DRCR | ~$2 | $2 | Hot-plug / power cycle from MSS GPIO |

---

## 5. Display Subsystem

| Qty | Part | MPN / Vendor | Unit Cost | Total | Notes |
|---|---|---|---|---|---|
| 1 | 7" IPS LCD, 1920×1200, MIPI DSI 4-lane, 500nit | Kadi Display / Raystar or equiv. | ~$75 | $75 | |
| 1 | Capacitive touch controller (I2C) | GT911 (Goodix) | ~$3 | $3 | Often included in panel assembly |
| 1 | MIPI DSI → HDMI bridge IC | ADV7535BSWZ (Analog Devices) | ~$15 | $15 | DSI-in, HDMI-out; I2C config from MSS |
| 1 | HDMI-A right-angle connector | Amphenol RHDME or equiv. | ~$2 | $2 | |
| 1 | LED backlight driver | TPS61187RTET | ~$3 | $3 | PWM dimming from FPGA GPIO |
| 1 | FPC connector 40-pin 0.5mm (panel) | Hirose FH12-40S-0.5SH | ~$2 | $2 | |

---

## 6. Clocking & Synchronization

| Qty | Part | MPN | Unit Cost | Total | Notes |
|---|---|---|---|---|---|
| 1 | Jitter-attenuating clock synthesizer | Si5332A (Skyworks/SiLabs) | ~$6 | $6 | Distribute MCLK to both sensors + SoC ref clocks |
| 1 | 25MHz TCXO (reference for Si5332) | TXC 7M-25.000MEEQ-T | ~$4 | $4 | ±0.5ppm for long-term sensor sync |

---

## 7. Connectivity

| Qty | Part | MPN / Vendor | Unit Cost | Total | Notes |
|---|---|---|---|---|---|
| 1 | WiFi 6 + BT 5.2 M.2 module (E-key, 2230) | Intel AX200NGW or AX210NGW | ~$18 | $18 | AX210 adds WiFi 6E |
| 1 | Gigabit Ethernet PHY | VSC8541XKN (Microchip) | ~$8 | $8 | RGMII to MSS |
| 1 | Ethernet transformer/magnetics | Bourns SM8002AEL | ~$3 | $3 | |
| 1 | RJ45 with integrated magnetics | BEL SI-52007-F | ~$4 | $4 | |
| 1 | USB 3.2 Type-C connector | Amphenol GSD0502301HR | ~$2 | $2 | Debug/offload |

---

## 8. Power Management

| Qty | Part | MPN | Unit Cost | Total | Notes |
|---|---|---|---|---|---|
| 1 | USB-C PD controller | FUSB307BMPX (onsemi) | ~$4 | $4 | Negotiate 20V/5A (100W) from USB-C PD supply |
| 1 | Main buck — 5V/6A | TPS62H160AQVF | ~$3 | $3 | |
| 1 | FPGA core — 1.0V/8A buck | TPS62130ARGR | ~$3 | $3 | |
| 1 | FPGA I/O / DDR — 1.8V/4A buck | TPS62150ARGR | ~$3 | $3 | |
| 1 | Sensor analog — 2.8V LDO | TPS7A4701RGWT | ~$3 | $3 | Low-noise; critical for sensor SNR |
| 1 | Sensor digital — 1.2V LDO | TPS7A2301PDBVR | ~$2 | $2 | |
| 1 | 3.3V rail — 3A LDO | TPS7A8300ARJFT | ~$2 | $2 | |
| 6 | Bulk decoupling caps (100µF MLCC X5R) | Various | ~$0.50 | $3 | Per power domain |

---

## 9. Passives, Connectors & Miscellaneous

| Qty | Part | Unit Cost | Total | Notes |
|---|---|---|---|---|
| 2 | FPC connector 30-pin 0.5mm (sensor MIPI) | ~$3 | $6 | One per sensor |
| 1 | PCIe M.2 standoff + screw hardware | ~$2 | $2 | |
| 1 | Passive component kit (0402 R/C, ferrites, TVS) | ~$35 | $35 | Decoupling, ESD, pull-ups |
| 1 | JTAG/UART debug header (2.54mm 10-pin) | ~$1 | $1 | |
| 1 | Heat sink + thermal pad (SoC) | ~$8 | $8 | 20×20mm Cu heatsink |
| 1 | 40mm fan (optional active cooling) | ~$6 | $6 | If ambient > 40°C |

---

## 10. PCB Fabrication (Prototype)

| Qty | Description | Unit Cost | Total | Notes |
|---|---|---|---|---|
| 5 | Custom PCB, 8-layer, ~150×120mm, ENIG, controlled impedance | ~$80/ea | $400 | PCBWay or Würth Elektronik proto service |

---

## 11. PCB Assembly (PCBA)

| Qty | Description | Cost | Notes |
|---|---|---|---|
| 1 | SMT assembly, 1 board | ~$350–$600 | Add $150 for X-ray BGA inspection |

---

## Cost Summary

| Category | Subtotal |
|---|---|
| Primary SoC | $385 |
| Sensors + optics | $630 |
| Memory | $94 |
| Storage | $108 |
| Display subsystem | $100 |
| Clocking | $10 |
| Connectivity | $35 |
| Power management | $28 |
| Passives & connectors | $58 |
| PCB fabrication (1 of 5) | $80 |
| PCB assembly (1 board) | $475 |
| **Prototype unit total** | **~$2,003** |

---

## Volume Pricing Notes

- PCB cost per unit drops to ~$25 at 50-unit volume
- PCBA drops to ~$150 at 100-unit volume
- Estimated production BOM (100 units, no NRE, no SSD): **~$850–$950/unit**
- NVMe SSD can be treated as customer-supplied in deployments

---

## Alternative: Zynq UltraScale+ ZU5EV BOM Delta

Swapping PolarFire SoC MPFS250T for ZU5EV saves ~$150 at system level and adds H.265 encoder + native MIPI CSI-2. See `08_soc_alternatives.md` for full comparison.
