# Display Subsystem

## Interface Options

| Interface | Best For | Notes |
|---|---|---|
| **MIPI DSI** (preferred) | Embedded panel | 1–4 lanes, up to 4K on 4-lane; compact, low power |
| **LVDS** | Industrial/automotive panels | Common for 7"–15", robust |
| **HDMI** (via fabric) | External monitor, dev/debug | Needs HDMI PHY bridge chip — not native on PolarFire |

**Recommendation:** MIPI DSI to embedded panel + ADV7535 HDMI bridge for external monitor during development.

---

## Panel Selection

| Criterion | Recommendation |
|---|---|
| Resolution | 1920×1080 minimum; 2560×1600 if budget allows |
| Size | 7"–10" for embedded; 15"+ for desktop use |
| Interface | MIPI DSI 4-lane |
| Refresh | 60Hz minimum; 120Hz for smooth depth visualization |
| Touch | Optional capacitive touch via I2C to RISC-V |

---

## Carrier Board Additions for Display

| Component | Part | Notes |
|---|---|---|
| MIPI DSI connector | FPC 40-pin 0.5mm (Hirose FH12-40S-0.5SH) | To panel |
| HDMI bridge IC | ADV7535BSWZ (Analog Devices) | DSI-in, HDMI-out; I2C config from MSS |
| HDMI-A connector | Amphenol RHDME or equiv. | Right-angle |
| Backlight driver | TPS61187RTET | PWM dimming from FPGA GPIO |
| Touch controller | GT911 (Goodix) or FT5336 | I2C to RISC-V MSS |

---

## Compositor Layers

The compositor sits between the depth pipeline and display controller in FPGA fabric:

| Layer | Content | Notes |
|---|---|---|
| 0 | Rectified RGB (left sensor) | Reference view |
| 1 | Depth map — heat-map colorized | 256-entry palette LUT in BRAM; near = red, far = blue |
| 2 | OSD | Framerate, depth at cursor, status; written by RISC-V |

Alpha blend between layers is runtime-configurable from RISC-V.

---

## Software

| Component | Role |
|---|---|
| DRM/KMS kernel driver | Linux display controller driver (Microchip reference available) |
| LVGL | Lightweight UI framework for embedded display; runs on framebuffer |
| ADV7535 I2C driver | Configure HDMI bridge from Linux (standard ADV7535 driver in kernel) |

---

## Key Notes

- **MIPI DSI TX IP** — verify Microchip has supported DSI TX core for PolarFire, or use DSI bridge chip (DSI-to-LVDS or DSI-to-HDMI) to offload PHY concern
- **Colorization** — disparity-to-color LUT runs in FPGA at zero CPU cost
- Depth map colorization and RGB overlay are done in hardware — display output requires no CPU bandwidth
