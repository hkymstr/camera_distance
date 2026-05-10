# Stereo Depth Range & Accuracy Analysis

## Governing Equation

```
depth = (focal_length_px × baseline_m) / disparity_px
```

All range limits and accuracy figures flow from this equation.

---

## Design Parameters

| Parameter | Value | Notes |
|---|---|---|
| Sensor | Sony IMX585, 3840×2160 | 2.9µm pixel pitch, 1/1.2" format |
| Lens focal length | 8mm | Common for this sensor format |
| Focal length (pixels) | ~2,760 px | = (8mm / 11.1mm sensor width) × 3840px |
| Baseline | 15cm | Recommended starting point |
| Sub-pixel disparity resolution | ±0.5px | Achievable with SGM + sub-pixel refinement |
| Max disparity search range | 256px | Typical SGM configuration |

---

## Range Limits

### Minimum Range (near cutoff)

```
depth_min = (2760 × 0.15) / 256 = ~1.6m
```

Objects closer than ~1.6m produce disparities larger than the search window and drop out of the depth map. This is a fundamental stereo blind spot.

To shrink minimum range: increase disparity search window (costs FPGA resources) or shorten baseline.

### Maximum Range (far cutoff)

```
depth_max = (2760 × 0.15) / 0.5px = ~828m (theoretical)
```

Practical limits reduce this significantly:

| Limiting factor | Practical max |
|---|---|
| Texture (featureless sky, flat walls) | ~30m |
| Sensor noise / SNR | ~50m |
| Calibration accuracy | ~80m |
| Algorithm ceiling (SGM + sub-pixel) | ~100m |

**Practical usable maximum: 30–50m in good lighting with textured scene.**

---

## Accuracy vs Distance

```
depth_error = depth² × disparity_error / (focal_length_px × baseline)
            = depth² × 0.5 / (2760 × 0.15)
            = depth² × 0.5 / 414
```

| Distance | Depth Error | Usable For |
|---|---|---|
| 0.5m | ~0.3mm | Fine manipulation |
| 1m | ~1.2mm | Precise obstacle detection |
| 3m | ~11mm | Navigation, person detection |
| 5m | ~30mm | Vehicle / obstacle avoidance |
| 10m | ~121mm (12cm) | Coarse detection |
| 20m | ~484mm (48cm) | Object presence only |
| 30m | ~1.1m | Barely usable |
| 50m | ~3m | Noise-dominated |

---

## Effect of Baseline on Range and Accuracy

Doubling the baseline doubles range and halves error at every distance:

| Baseline | Usable Max Range | Error @ 10m | Min Range |
|---|---|---|---|
| 6cm (narrow) | ~15m | ~48cm | 0.6m |
| **15cm (design)** | **~40m** | **~12cm** | **1.6m** |
| 30cm (wide) | ~80m | ~6cm | 3.2m |
| 60cm (very wide) | ~150m | ~3cm | 6.5m |

Wider baseline = better far-field accuracy, larger near-field blind spot. Choose baseline for your primary working range.

---

## Effect of Lens Focal Length on Range

Longer focal length trades field of view for range:

| Focal Length | FoV (H) | f_px | Usable Max Range | Min Range |
|---|---|---|---|---|
| 4mm (wide) | ~100° | 1,380px | ~20m | 0.8m |
| **8mm (design)** | **~55°** | **2,760px** | **~40m** | **1.6m** |
| 16mm (tele) | ~30° | 5,520px | ~80m | 3.2m |
| 35mm (long tele) | ~14° | 12,075px | ~175m | 7m |

---

## Comparison to Other Sensing Technologies

| Technology | Min Range | Max Range | Accuracy @ 10m | Works in Dark? |
|---|---|---|---|---|
| **Stereo vision (this design)** | **~1.5m** | **~40m** | **~12cm** | No |
| Time-of-Flight (ToF) | 0.1m | 5–8m | ~1cm | Yes |
| Structured light (e.g. RealSense) | 0.2m | 10m | ~2mm | No |
| LiDAR (mid-range) | 0.5m | 100–200m | ~2cm | Yes |
| Radar | 1m | 200m+ | ~50cm | Yes |

Stereo is best in the **1.5m – 30m** sweet spot in good lighting.

---

## Recommendations for This Design

- **Keep 15cm baseline** — covers the most useful range without excessive near-field blind spot
- **Use 8mm lenses** — balanced FoV and range
- **If > 30m range needed:** Widen baseline to 30cm and/or switch to 16mm lens
- **If < 1m range needed:** Add VL53L5CX ToF sensor (I2C, cheap, 0–4m) alongside the stereo pair
- **If > 50m range needed:** Plan for LiDAR fusion — stereo alone is not reliable at that range

---

## Summary

```
Minimum depth:  ~1.6m  (hard cutoff — disparity search limit)
Sweet spot:     1.6m – 20m  (cm-level accuracy, reliable)
Degraded range: 20m – 40m   (dm-level accuracy, detection only)
Practical max:  ~40m         (beyond this, noise dominates)
```
