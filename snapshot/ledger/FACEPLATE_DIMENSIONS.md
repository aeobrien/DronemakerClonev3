# Faceplate & Backplate — Component Mounting Dimensions

> Reference document for laser-cut panel design. All dimensions in mm unless noted.
> Last updated: 2026-03-30

---

## Enclosure: Hammond 1456KK4BKBK

| Parameter | mm | inches |
|---|---|---|
| Overall external (D × W) | 266.3 × 258.6 | 10.48 × 10.18 |
| Rear height (external) | 106.8 | 4.21 |
| Front height (external) | 98.4 | 3.87 |
| Slope angle | 30° | |
| **Top panel overall (L × W)** | **259.9 × 242.6** | 10.23 × 9.55 |
| **Top panel usable area (inner edges)** | **242.8 × 239.3** | 9.56 × 9.42 |
| Top panel thickness | 1.63 (14-gauge aluminium) | 0.064 |
| Top panel mounting holes | 4.80mm dia, 4 places | |
| Top panel screw holes | 2.79mm dia, 8 places | |
| Screw hole spacing (L × W) | 248.8 × 233.4 | 9.80 × 9.19 |
| **Rear panel opening (W × H)** | **233.4 × 72.2** | 9.19 × 2.84 |
| Internal rear height | 101.6 | 4.00 |
| Internal depth at base | 231.4 | 9.11 |
| Internal width between walls | 254.0 | 10.00 |
| Case wall thickness | 2.06 (12-gauge aluminium) | 0.081 |
| Ventilation slot depth | 22.2 | 0.88 |
| Ventilation slot width | 6.4 | 0.25 |

Source: [Hammond drawing](https://www.hammfg.com/files/parts/pdf/1456KK4BKBK.pdf)

---

## Faceplate Components

### Waveshare 7" HDMI LCD (C) — Touchscreen

| Parameter | mm |
|---|---|
| Board overall (H × V × D) | 164.9 × 107.0 × 8.0 |
| **Active display area (H × V)** | **154.2 × 85.9** |
| Bezel width (horizontal, per side) | 5.35 |
| Bezel width (vertical, per side) | 10.52 |
| Resolution | 1024 × 600 |
| Power | 5V, ≥500mA (2.5W) |
| **Mounting holes** | **4 × M3** (one at each corner, on standoff pillars) |
| **Mounting hole spacing (H × V)** | **157.0 × 115.0mm** centre-to-centre |
| Hole diameter | ~3.2mm (M3 clearance) |
| Total footprint incl. standoffs | ~164.9 × 115.0mm (standoffs extend ~4mm beyond board top/bottom) |

**Faceplate cutout:** Rectangular. Size depends on mounting strategy:
- **Option A — active area only:** ~155 × 87mm cutout, board mounts behind panel with bezel overlapping
- **Option B — full board visible:** ~166 × 108mm cutout with mounting bracket/frame behind

**Note:** Mounting hole standoffs protrude beyond the board vertically. The 115mm vertical span must be accounted for in faceplate design — the mounting holes are wider apart than the board itself.

STEP file for verification: [7inch_cad.zip](https://files.waveshare.com/upload/f/f4/7inch_cad.zip) (contains `1118.stp`)

Source: [Waveshare wiki](https://www.waveshare.com/wiki/7inch_HDMI_LCD_(C))

---

### PEC16-4220F-S0024 — Rotary Encoder (×8)

| Parameter | mm |
|---|---|
| **Mounting hole** | **9.3 +0.2/-0 mm** round (nominal 9.3, max 9.5) |
| **Anti-rotation locator hole** | **3.3 +0.2/-0 mm** round, **9.0mm** from main hole centre |
| Bushing thread | M9 × 0.75 |
| Bushing outer diameter | 9.0 |
| Shaft diameter | 6.0 (flatted D-shaft, insulated) |
| Shaft length above mounting surface | 20.0 |
| Threaded bushing protrusion | 3.1 |
| **Max panel thickness (with washer + nut)** | **~2.7** |
| Max panel thickness (nut only, no washer) | ~3.1 |
| Body width | 16.8 |
| Body height | 10.5 |
| Total depth behind panel (body + pins) | 17.7 |
| Nut: M9 × 0.75, across flats | 11.5 |
| Washer outer dia | 15.0 |

**Minimum spacing between encoders:** ~18mm centre-to-centre (body width 16.8mm + clearance). Recommended: **20mm+** for comfortable finger access with knobs.

Source: [Bourns PEC16 datasheet](https://www.bourns.com/pdfs/pec16.pdf)

---

### 16mm RGB Momentary Pushbutton (×8)

| Parameter | mm |
|---|---|
| **Mounting hole** | **16mm** round |
| Panel thickness | 1–7mm |
| Body dimensions | 22.0 × 22.0 × 19.5 |
| Operating stroke | 2.0 |
| LED rated voltage | 6V (operates 3–6V; add 1KΩ for 12/24V) |
| LED current | ~20mA per colour |
| Switch current rating | Not rated; recommend ≤1A / 24VDC |
| Contact resistance | <50mΩ |

**Minimum spacing between buttons:** ~24mm centre-to-centre (body 22mm + clearance). Recommended: **25mm+**.

**Electrical note:** Switch and LED circuits are isolated. Common-anode RGB LED. Cathodes driven by shift register outputs via 220Ω resistors from MIDI controller PCB.

Source: [The Pi Hut product page](https://thepihut.com/products/rugged-metal-pushbutton-16mm-6v-rgb-momentary)

---

### Alpha 9mm Potentiometer — UM2 Pots (×3) + Headphone Vol (×1)

| Parameter | mm |
|---|---|
| **Mounting hole** | **7.0mm** round |
| Bushing thread | M7 × 0.75 |
| Bushing outer diameter | 7.0 |
| Shaft diameter | **6.0** (round — NOT 6.35mm) |
| Body | 9.0 × 9.0 × 5.0 |

4 pots total on faceplate: XLR/TRS gain (left top), instrument gain (left bottom), master volume (right top), headphone volume (right bottom). First 3 are original UM2 pots. Headphone pot is spare dual A10K (or new dual A1K if sourced).

**Knob compatibility:** 6.0mm shaft. The 20mm Solid Aluminium knobs (see below) fit. Standard 6.35mm (1/4") knobs will be loose.

Source: Alpha RV09 series standard dimensions

---

### Knobs — Encoder (×8)

| Parameter | Value |
|---|---|
| Type | Plain Solid Aluminium, no position indicator |
| **Body / max diameter** | **15.1mm** |
| Height | 15.2mm |
| Shaft type | 6mm round / splined, grub screw |
| Min shaft depth | 6.0mm |
| Max shaft depth | 12.4mm |
| Weight | 6.23g |

Source: [eBay](https://www.ebay.co.uk/itm/386817735048)

---

### Knobs — Pot (×4)

| Parameter | Value |
|---|---|
| Type | 20mm Large Solid Aluminium |
| Body diameter | 18.0mm |
| **Max diameter** | **20.1mm** |
| Height | 15.5mm |
| Shaft type | 6mm round, grub screw |
| Min shaft depth | 8.2mm |
| Max shaft depth | 13.7mm |
| Weight | 10.47g |

Source: [eBay](https://www.ebay.co.uk/itm/386329209097)

---

### 5mm LED in Panel-Mount Bezel (×4)

| Parameter | mm |
|---|---|
| **Mounting hole** | **6.2mm** round |
| Bezel flange diameter | 7.8 |
| Bezel body diameter | ~6.1–6.2 |
| Panel thickness | 1–3mm |
| LED diameter | 5.0 (T-1 3/4) |

**Measured 2026-04-02:** Flange 7.8mm, body ~6.1–6.2mm, clip section ~7mm (compresses to fit through hole). 6.2mm hole gives 0.8mm flange overlap per side — sufficient retention.

**Note:** Original UM2 LEDs were 3mm in a dual-LED plastic housing. Replacements are individual 5mm LEDs in chrome bezels. Need to verify forward voltage/current of original UM2 LEDs to confirm 5mm replacements are electrically compatible — likely not critical for indicator LEDs.

---

## Backplate Components

### Neutrik NCJ6FI-S — Combo XLR/TRS Jack (×1)

| Parameter | mm |
|---|---|
| **Panel cutout** | **D-shape: ≥23.8mm dia circle with flat, 20mm wide** |
| Cutout height | 23.0 |
| Mounting screw holes | 2 × 3.2mm dia (M3) |
| Front face | 27.0 × 20.0 |
| Total depth (front to back) | ~35.5 |
| Max panel thickness | 7.0 |
| Mounting direction | **Rear** (inserts from behind panel) |

Source: [Neutrik NCJ6FI-S](https://www.neutrik.com/en/product/ncj6fi-s), drawing 3102ST1729

---

### Neutrik NJ3FP6C-BAG — 1/4" Jack (×3: instrument in, L out, R out)

| Parameter | mm |
|---|---|
| **Panel cutout** | **D-shape: 24.0mm dia circle with flat, 19mm wide** |
| Cutout height | 24.0 |
| Mounting screw holes | 2 × 3.4mm dia (M3) |
| Shell size (flange) | 31 × 26 |
| Depth behind panel | 29.0 |
| Mounting direction | **Front** |

Source: [Neutrik NJ3FP6C-BAG](https://www.neutrik.com/en/product/nj3fp6c-bag)

---

### TUK KUCC1820BPM — USB-C Panel Mount (×1, Pi power)

| Parameter | mm |
|---|---|
| **Panel cutout** | **Standard keystone: 16.2 × 14.7 rectangle** |
| Panel mount type | Standard profile keystone, black |
| Internal cable | 200mm USB-C pigtail (plugs directly into Pi) |

Source: TUK product page / standard keystone spec

---

### Neutrik NAUSB3-B — USB 3.0 Feedthrough (×1, external MIDI/data)

| Parameter | mm |
|---|---|
| **Panel cutout** | **D-size: 24.0 × 19.2 rounded rectangle** |
| Mounting screw holes | 2 × M3 at 19.0mm centres |
| Panel thickness | 1–6mm |
| Connector | USB-A (outside) / USB-B (inside), reversible |

Source: [Neutrik NAUSB3-B](https://www.neutrik.com/en/product/nausb3-b)

---

### Headphone TRS Socket (×1, backplate)

Neutrik NJ3FP6C-BAG (same as output jacks) — D-size cutout, same dimensions as above.

---

## Encoder Row Spacing (8 encoders below screen)

| Parameter | Value |
|---|---|
| Display active width | 154.2mm |
| Encoder knob diameter | 15.1mm |
| 8 knobs total width | 120.8mm |
| Remaining space (gaps) | 33.4mm |
| Number of gaps | 7 |
| **Gap between knobs** | **4.77mm** |
| **Centre-to-centre** | **19.87mm** |
| Encoder body width | 16.8mm |
| Behind-panel body clearance | 3.07mm (min spec ~1.2mm) |

Alignment rule: leftmost knob left edge = left edge of display active area; rightmost knob right edge = right edge of display active area.

---

## Cutout Summary for Panel Design

### Faceplate (top panel: 242.8 × 239.3mm usable)

| Qty | Component | Cutout | Shape |
|-----|-----------|--------|-------|
| 1 | Touchscreen | ~155 × 87mm (TBC) | Rectangle |
| 8 | Encoders | 9.5mm | Round |
| 8 | Pushbuttons | 16mm | Round |
| 4 | Pots | 7mm | Round |
| 4 | LED bezels | 6.2mm | Round |

**Total cutouts on faceplate:** 1 rectangle + 24 round holes

### Backplate (rear panel: 233.4 × 72.2mm)

| Qty | Component | Cutout | Shape |
|-----|-----------|--------|-------|
| 1 | Combo XLR/TRS | D-size (≥23.8mm) | D-shape |
| 4 | 1/4" jacks (instrument in, L out, R out, headphone) | D-size (24mm) | D-shape |
| 1 | USB-C (Pi power) | Keystone (16.2 × 14.7mm) | Rectangle |
| 1 | USB-A (data/MIDI) | D-size (24mm) | D-shape |

**Total cutouts on backplate:** 6 × D-size + 1 × keystone. D-size at ~30mm spacing = ~180mm + keystone ~20mm = ~200mm, fits within 233.4mm.

---

## Panel Thickness Compatibility Check

Top panel is **1.63mm**. All faceplate components are compatible:
- Encoders: max 2.7mm with washer ✓
- Pushbuttons: 1–7mm ✓
- Pots: fine at 1.63mm ✓
- LED bezels: 1–3mm ✓

Rear panel thickness TBC (may be the case wall at 2.06mm, or a separate panel). All backplate components support up to 6–7mm ✓

---

## Still Needed

- [x] ~~Waveshare screen mounting hole positions~~ — confirmed: 4 × M3, 157.0 × 115.0mm
- [x] ~~USB-C panel mount~~ — TUK KUCC1820BPM, standard keystone 16.2 × 14.7mm
- [x] ~~USB-A feedthrough~~ — Neutrik NAUSB3-B, D-size cutout
- [ ] Verify original UM2 LED specs for 5mm replacement compatibility
- [ ] Download Neutrik D-size cutout DXF template from neutrik.com
- [ ] Verify v1 STEP hole positions and sizes against all component datasheets
- [ ] Confirm pot knob (20.1mm dia) clearance on left/right side pairs
