# Faceplate Design Brief — DronemakerClone

## Overview

Design cutouts for a **single-piece bent aluminium panel** (faceplate + backplate) for the Hammond 1456KK4BKBK console enclosure. The panel is the top/front piece that includes:

1. A **flat top section** (horizontal)
2. A **30-degree sloped section** facing the user (screen, encoders, pots, LEDs)
3. A **short vertical section** at the front (pushbuttons)
4. A **back panel** section (audio I/O, USB connectors)

The original panel STEP file is provided with no component cutouts — only the enclosure's own ventilation slots and mounting hardware.

---

## Deliverables

1. Modified STEP file with all cutouts
2. Assembly STEP file with component models placed for visual verification
3. DXF or PDF drawing with all cutout dimensions for manufacturing review

---

## Provided Files

| File | Description |
|------|-------------|
| `models/FaceplateOriginal.step` | Clean panel — no component cutouts, only vent slots and Hammond hardware |
| `models/display.stp` | Waveshare 7" HDMI LCD (C) — exact 3D model from manufacturer |
| `datasheets/PEC16_encoder.pdf` | Bourns PEC16-4220F-S0024 rotary encoder |
| `datasheets/NJ3FP6C-BAG_jack.pdf` | Neutrik NJ3FP6C-BAG 1/4" TRS jack |
| `datasheets/Hammond_1456KK4BKBK.pdf` | Enclosure drawing |
| `datasheets/Alpha_9mm_pot.pdf` | Alpha RV09 9mm potentiometer |
| `layout_slope_face.svg` | 2D layout diagram — sloped surface (screen, encoders, pots, LEDs) |
| `layout_front_face.svg` | 2D layout diagram — front vertical face (pushbuttons) |
| `layout_back_panel.svg` | 2D layout diagram — back panel (audio I/O, USB) |

**Not available as downloadable PDFs (reference links provided instead):**
- Adafruit 3350 RGB pushbutton: https://thepihut.com/products/rugged-metal-pushbutton-16mm-6v-rgb-momentary
- Neutrik NCJ6FI-S combo XLR/TRS: https://www.neutrik.com/en/product/ncj6fi-s
- Neutrik NAUSB3-B USB feedthrough: https://www.neutrik.com/en/product/nausb3-b
- Waveshare 7" LCD (C) wiki: https://www.waveshare.com/wiki/7inch_HDMI_LCD_(C)

---

## Panel Geometry

The panel is **one continuous piece of 1.63mm aluminium** (14-gauge) bent into shape. It is NOT separate front and back panels.

Key surfaces for cutouts:

| Surface | Description | Normal direction (approx) |
|---------|-------------|--------------------------|
| Flat top | Horizontal top of enclosure | Vertical (upward) |
| Slope | 30-degree angled face, facing the user | 30° from vertical, forward-and-up |
| Front vertical | Short face at the bottom-front | Forward (horizontal) |
| Back panel | Vertical face at the rear | Rearward (horizontal) |

---

## Faceplate Cutouts (Slope Surface)

### 1. Screen Cutout — CRITICAL

**Approach:** Active-area cutout (Option A). The display board mounts BEHIND the panel. The board's bezel overlaps the cutout edges. Only the active display pixels are visible through the cutout.

| Parameter | Value |
|-----------|-------|
| Cutout size | **155 × 87mm** (154.21 × 85.92mm active area + ~0.8mm tolerance) |
| Shape | Rectangle |

**IMPORTANT:** The display's active area is NOT centred on its mounting holes. Use the provided `display.stp` STEP model to extract the exact relationship. From our analysis:
- Mount hole centre: X=0, Z=0 (in display's native coords)
- Active area centre: X=+1.9mm, Z=-5.15mm (offset from mount centre)
- The cutout must be centred on the ACTIVE AREA position, not the mount centre

**Screen mounting holes:**
- 4 × M3 clearance holes (3.2-3.3mm diameter)
- Spacing: 157.0mm horizontal × 115.0mm vertical (from display STEP model)
- These must be OUTSIDE the cutout, in solid panel material
- The mounting holes must align exactly with the display model's standoff positions

**Placement on slope:** The screen should be positioned so that:
- There is approximately 10-15mm gap between the bottom of the cutout and the top of the encoder holes below
- The top mounting hole is at least 5mm from the top edge of the slope
- The bottom mounting holes clear the encoder row

### 2. Encoder Holes — 8 × 9.5mm round

| Parameter | Value |
|-----------|-------|
| Hole diameter | **9.5mm** |
| Quantity | 8 |
| Surface | Slope (30-degree face) |
| Arrangement | Single horizontal row below the screen |

**Alignment rule:** The encoder knobs (15.1mm diameter) must align with the active display area (154.21mm wide, from datasheet). Specifically:
- Left edge of leftmost knob = left edge of active pixel area
- Right edge of rightmost knob = right edge of active pixel area
- **Account for the display's 1.9mm X offset** when calculating these positions

**Spacing calculation:**
- Active pixel area width: 154.21mm (centred at X=+1.9mm from mount centre)
- Active pixel left edge: 1.9 - 77.105 = -75.205mm
- Active pixel right edge: 1.9 + 77.105 = +79.005mm
- Knob radius: 7.55mm
- Leftmost encoder centre: -75.205 + 7.55 = **-67.655mm**
- Rightmost encoder centre: 79.005 - 7.55 = **+71.455mm**
- Span: 139.11mm across 7 gaps
- **Centre-to-centre: 19.87mm**

**Behind-panel clearance:** Encoder body is 16.0 × 16.0mm square, 6.5mm deep behind panel. At 19.87mm centre-to-centre, the bodies have 3.87mm clearance — acceptable.

### 3. Pot Holes — 4 × 7.0mm round

| Parameter | Value |
|-----------|-------|
| Hole diameter | **7.0mm** |
| Quantity | 4 (2 left, 2 right) |
| Surface | Slope (30-degree face) |

**Layout:**
- **Left pair** at X = -101mm: XLR/TRS gain (upper), Instrument gain (lower)
- **Right pair** at X = +101mm: Master volume (upper), Headphone volume (lower)
- Upper pots: approximately 25mm above screen mount centre along slope
- Lower pots: approximately 25mm below screen mount centre along slope
- Left and right pairs are mirror images

**Pot knobs:** 20.1mm max diameter. Ensure 2mm+ clearance between knob edges.

### 4. LED Bezel Holes — 4 × 8.0mm round (TBC — may be ~7.5mm)

| Parameter | Value |
|-----------|-------|
| Hole diameter | **8.0mm** (may need adjustment to ~7.5mm — confirm with physical bezel when it arrives) |
| Quantity | 4 |
| Surface | Slope (30-degree face) |

**Layout:** Two LEDs per left-side pot, stacked vertically (along the slope), positioned to the LEFT of each left-side pot.
- LED centre X = -116mm
- Vertically offset ±7mm along the slope from each left pot centre
- LED bezel outer diameter: ~10mm. Ensure clearance from pot knobs (20.1mm dia at X=-101).
- Gap between LED bezel edge and pot knob edge: 116 - 101 - 10/2 - 20.1/2 = ~4.95mm

### 5. Screen Mounting Holes — see Screen Cutout section above

---

## Faceplate Cutouts (Front Vertical Face)

### 6. Pushbutton Holes — 8 × 16.0mm round

| Parameter | Value |
|-----------|-------|
| Hole diameter | **16.0mm** |
| Quantity | 8 |
| Surface | Front vertical face |
| Arrangement | Single horizontal row, centred vertically on the face |
| Centre-to-centre | **25.0mm** |
| Row centre X | 0 (centred) |

Pushbutton positions: X = -87.5, -62.5, -37.5, -12.5, +12.5, +37.5, +62.5, +87.5

---

## Backplate Cutouts (Rear Panel Opening: 233.4 × 72.2mm)

All connectors are mounted in the rear panel opening. The rear panel is a separate section from the slope — it's the vertical face at the back of the enclosure.

### 7. Audio Connectors — 5 × Neutrik D-size cutouts

| # | Component | Cutout |
|---|-----------|--------|
| 1 | Combo XLR/TRS input (NCJ6FI-S) | D-shape: 24mm dia circle with flat, 20mm wide + 2 × M3 screw holes |
| 2 | Instrument input (NJ3FP6C-BAG) | D-shape: 24mm dia circle with flat, 19mm wide + 2 × M3 screw holes |
| 3 | Left output (NJ3FP6C-BAG) | D-shape: same as above |
| 4 | Right output (NJ3FP6C-BAG) | D-shape: same as above |
| 5 | Headphone output (NJ3FP6C-BAG) | D-shape: same as above |

### 8. USB-A Data — 1 × Neutrik D-size cutout

| Component | Cutout |
|-----------|--------|
| Neutrik NAUSB3-B | D-size: 24.0 × 19.2mm rounded rectangle + 2 × M3 screw holes at 19mm centres |

### 9. USB-C Power — 1 × Standard Keystone cutout

| Component | Cutout |
|-----------|--------|
| TUK KUCC1820BPM | Standard keystone: **16.2 × 14.7mm** rectangle |

**Backplate arrangement:** 6 × D-size + 1 × keystone across 233.4mm width.
Suggested order (left to right): XLR/TRS, Instrument, L out, R out, Headphone, USB-A, USB-C.
At ~30mm spacing for D-size connectors: 6 × 30 = 180mm + keystone ~20mm = ~200mm. Fits within 233.4mm with ~17mm margin each side.

Connectors should be centred vertically in the 72.2mm opening.

**Neutrik D-size cutout template:** Available from https://www.neutrik.com — search for "D-size panel cutout DXF".

---

## Component Dimensions Quick Reference

| Component | Mounting hole | Behind-panel depth | Above-panel height | Body width |
|-----------|--------------|-------------------|--------------------|-----------|
| PEC16 encoder | 9.5mm round | 6.5mm body + 2.7mm pins | 7mm bushing + 12mm shaft = 19mm | 16 × 16mm square |
| Adafruit 3350 button | 16mm round | 20mm (Ø16 barrel) | 1.5mm (Ø18.5 cap) | 22 × 22mm housing |
| Alpha 9mm pot | 7mm round | 5mm body | 8mm shaft | 9 × 9mm |
| 5mm LED bezel | ~7.5-8mm round (TBC) | ~2mm | ~2mm flange | ~10mm flange |
| Waveshare screen | 3.2-3.3mm round (×4) | ~14mm (board + standoffs) | Flush with panel | 165 × 100mm board |

---

## Critical Notes

1. **The panel is ONE PIECE** — faceplate and backplate are the same bent metal part. All cutouts go in one manufacturing step.

2. **The display active area is NOT centred** on the mounting holes. This is the single most important thing to get right. Use the provided STEP model — do not assume symmetry.

3. **Encoder knobs align with the active pixel area**, accounting for the display's X offset. The knob edges, not the hole edges, define the alignment.

4. **Panel thickness is 1.63mm.** All components are verified compatible with this thickness.

5. **LED hole size is tentative** (8mm). Physical bezels have been ordered but not yet measured. The designer should make this parametric if possible, or we can confirm before final output.

6. **No drilling for internal mounting.** All internal PCBs mount with adhesive clips. The only holes in this panel are the component cutouts listed above.

---

## Contact

If anything is unclear, ask before cutting. Better to clarify a dimension than to guess.
