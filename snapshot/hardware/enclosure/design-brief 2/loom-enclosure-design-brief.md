# Panel Cutout Design Brief

## What You're Designing

Cutouts for a **pre-bent aluminium faceplate** belonging to a Hammond 1456KK4BKBK console enclosure. The panel is a single continuous piece of 1.63mm aluminium, already bent into four surfaces:

1. **Flat top** — no cutouts needed
2. **30° slope** — screen, encoders, pots, LEDs
3. **Front vertical face** — pushbuttons
4. **Back panel** — audio connectors, USB

Your job is to produce manufacturing-ready output (STEP with cutouts + DXF/PDF drawings) from the provided faceplate STEP, display STEP model, and the dimensions in this document.

---

## Provided Files

| File | What it is |
|------|------------|
| `models/1456KK4BKBK_faceplate_only.stp` | The original panel — no cutouts, only Hammond's vent slots and hardware |
| `models/1456KK4BKBK_whole_enclosure.stp` | Full Hammond enclosure 3D model — shows how the panel sits in the box |
| `models/display.stp` | Waveshare 7" display — exact 3D model from manufacturer |
| `layout_slope_face.svg` | Visual guide — slope surface layout |
| `layout_front_face.svg` | Visual guide — front face layout |
| `layout_back_panel.svg` | Visual guide — back panel layout |
| `datasheets/` | Component datasheets (see below) |

### About the layout SVGs

**These are layout guides, not technical drawings.** They show the intended arrangement and labelling of components on each surface — which component goes where, and how they relate to each other. Though I've aimed for accuracy wherever possible, they are not guaranteed to be to scale and should not be used for dimensions. Use the dimensions table below as the source of truth for all measurements. The SVGs are there so you can see the design intent.

---

## Deliverables

1. Modified STEP file with all cutouts placed in the original panel
2. Assembly STEP with the display model placed, for visual verification
3. DXF or PDF per surface with dimensioned cutout positions, for manufacturing review

---

## Cutout Dimensions — Source of Truth

### Slope surface (30° face)

| Component | Qty | Hole shape | Hole size | Notes |
|-----------|-----|-----------|-----------|-------|
| **Screen** | 1 | Rectangle | **155 × 87 mm** | See critical note below |
| Screen mount holes | 4 | Round | 3.3 mm (M3 clearance) | 157.0 × 115.0 mm spacing — extract positions from `display.stp` |
| **Encoders** | 8 | Round | **9.5 mm** | Single row below screen, 19.87 mm centre-to-centre |
| **Pots** | 4 | Round | **7.0 mm** | 2 left of screen, 2 right of screen |
| **LED bezels** | 4 | Round | **6.2 mm** | 2 per left-side pot, stacked vertically. 7.8 mm flange retains bezel. |

### Front vertical face

| Component | Qty | Hole shape | Hole size | Notes |
|-----------|-----|-----------|-----------|-------|
| **Pushbuttons** | 8 | Round | **16.0 mm** | Single row, centred vertically, 25.0 mm centre-to-centre |

### Back panel (233.4 × 72.2 mm opening)

| Component | Qty | Cutout type | Notes |
|-----------|-----|-------------|-------|
| Combo XLR/TRS jack | 1 | Neutrik D-size | NCJ6FI-S — see datasheet |
| 1/4" TRS jacks | 4 | Neutrik D-size | NJ3FP6C-BAG — see datasheet |
| USB-A feedthrough | 1 | Neutrik D-size | NAUSB3-B — 24.0 × 19.2 mm rounded rect + 2×M3 at 19 mm centres |
| USB-C power | 1 | Keystone snap-in | 16.2 × 14.7 mm rectangle — standard keystone cutout, no screw holes |

**Backplate order (left to right, viewed from rear):** Headphone Out, L Out, R Out, Instrument XLR, TRS, USB-A, USB-C

**Neutrik D-size cutout template:** Included in datasheets – standard template for all D-size connectors.

---

## Critical: Screen Placement

The screen cutout and active display area must be **centred on the panel**. However, the display's active pixel area is not centred on its mounting holes. This means the display board's mounting holes will be slightly off-centre on the panel.

From the manufacturer's STEP model:

- Mounting hole centre: X = 0, Z = 0 (in the display's own coordinate system)
- Active area centre: **X = +1.9 mm, Z = −5.15 mm** (offset from mount centre)

So to centre the active area on the panel, the mounting holes must be shifted **−1.9 mm** horizontally from panel centre. Please verify this offset from the provided `display.stp` rather than relying solely on these numbers.


### Encoder alignment depends on the screen

The 8 encoder positions are defined by the positions of the **knob that will fit onto the encoders**. These are 15.1 mm in diameter each, and must span exactly the width of the active pixel area (154.21 mm):
- Left edge of leftmost knob = left edge of active display area
- Right edge of rightmost knob = right edge of active display area

This means encoder hole positions are derived from the screen's active area position. 

### Screen orientation — IMPORTANT

The Waveshare 7" HDMI LCD (C) has its **HDMI mini and micro-USB power ports on one of the long edges** of the board. When correctly oriented in landscape mode (1024 × 600, wide):

- **Ports are on the right edge** of the board (as viewed from the front / user side)
- When mounted behind the panel and viewed from behind (inside the enclosure), ports face **left**
- This means cables route toward the **right side of the panel** (the volume pots side) when viewed from the front

Verify orientation from the `display.stp` model. The slope face SVG layout guide marks the cable exit side for reference.

### Internal clearance

Several components have significant behind-panel depth. Two areas to watch:

1. **Screen cable routing:** The HDMI and micro-USB cables exit from the **right edge** of the display board (toward the volume pots side, as viewed from the front). Ensure the right-side pots have sufficient clearance from the screen board edge to allow these cables to route without fouling. The SVG layout guide shows the approximate pot positions (X ≈ +101 mm from centre) — verify that the screen board edge (~82.5 mm from centre) leaves adequate cable routing space.

2. **Button-to-encoder clearance:** The pushbuttons (Adafruit 3350) have a 22 × 22 × 20 mm barrel behind the panel. The encoders (PEC16) have a 16.8 × 16.8 × 6.5 mm body behind the panel. Because the buttons are on the front vertical face and the encoders are on the slope, these bodies converge inside the enclosure at the bend. I believe there's sufficient space that they won't interfer, but please verify that the back of the button barrels clears the back of the encoder bodies.

---

## Layout Intent — Key Relationships

These describe the design intent. Use the SVG guides for visual reference. Adjusting positions to meet clearance and manufacturability requirements may well be necessary, but please try to keep to the visual reference as much as possible.

**Slope surface:**
- Screen is the dominant element, positioned in the upper portion of the slope
- 8 encoders in a row directly below the screen, knobs visually aligned with the screen width
- 2 pots flanking the screen on the left (gain controls), 2 on the right (volume controls)
- 4 LED bezels to the left of the left-side pots, 2 per pot, stacked vertically
- Approximate pot X position: ±101 mm from centre. Approximate LED X position: −116 mm from centre

**Front vertical face:**
- 8 pushbuttons in a single centred row. The face is approximately 24.7 mm tall.

**Back panel:**
- 7 connectors (6 D-size + 1 keystone) spread across the 233.4 mm opening
- Connectors centred vertically in the 72.2 mm opening height

---

## Component Clearances to Watch

| Concern | Detail |
|---------|--------|
| Encoder knob-to-knob gap | 4.77 mm at 19.87 mm c-to-c with 15.1 mm knobs — tight but acceptable |
| Encoder-to-screen mount hole | Bottom screen mount holes must clear the encoder row. Verify spacing. |
| Pot knob-to-LED bezel | Pot knobs are 20.1 mm dia at X = ±101. LED bezels are ~10 mm dia at X = −116. Gap is ~5 mm — acceptable but verify. |
| Screen cables to right pots | HDMI + micro-USB cables exit the right edge of the screen PCB (viewed from front). Must not foul the right-side volume pots. |
| Button barrels to encoder bodies | Button barrels (20 mm deep) and encoder bodies (6.5 mm deep) converge at the slope/front bend. Verify clearance inside the enclosure. |
| Button-to-button gap | Buttons are 16 mm hole, 18.5 mm cap, at 25 mm c-to-c — 6.5 mm gap between caps. Fine. |
| Behind-panel encoder bodies | 16.8 mm square bodies at 19.87 mm c-to-c = 3.07 mm clearance. Tight but fits. |

---

## Component Quick Reference

| Component | Mounting hole | Body behind panel | Knob/cap diameter |
|-----------|--------------|-------------------|-------------------|
| PEC16 encoder | 9.5 mm round | 16.8 × 16.8 × 6.5 mm | 15.1 mm knob |
| Adafruit 3350 pushbutton | 16.0 mm round | 22 × 22 × 20 mm barrel | 18.5 mm cap |
| Alpha 9mm pot | 7.0 mm round | 9 × 9 × 5 mm | 20.1 mm knob |
| 5mm LED bezel | 6.2 mm round | ~2 mm | 7.8 mm flange |
| Waveshare 7" display | 3.3 mm round (×4) | ~14 mm (board + standoffs) | — |

---

## Panel Details

- **Material:** 1.63 mm aluminium (14-gauge)
- **Panel is pre-bent** — all four surfaces are already formed. Cutouts go into the existing bent part.

---

## Datasheets Included

| File | Component |
|------|-----------|
| `datasheets/PEC16_encoder.pdf` | Bourns PEC16-4220F-S0024 rotary encoder |
| `datasheets/NJ3FP6C-BAG_jack.pdf` | Neutrik NJ3FP6C-BAG 1/4" TRS jack (full Neutrik Plugs & Jacks catalog — see chassis jacks section) |
| `datasheets/Neutrick NCJ6FI-S.pdf` | Neutrik NCJ6FI-S combo XLR/TRS jack |
| `datasheets/Neutrik NAUSB3.pdf` | Neutrik NAUSB3-B USB-A feedthrough |
| `datasheets/Hammond_1456KK4BKBK.pdf` | Hammond enclosure drawing |
| `datasheets/Alpha_9mm_pot.pdf` | Alpha RV09 9mm potentiometer |
| `datasheets/Adafruit_3350.pdf` | Adafruit 3350 RGB momentary pushbutton |
| `datasheets/TUK KUCC1820Bpm.pdf` | TUK USB-C keystone panel mount |
| `datasheets/7inch-hdmi-lcd-c-panel-dimension.pdf` | Waveshare 7" HDMI LCD (C) panel dimensions |

**Waveshare display wiki:** https://www.waveshare.com/wiki/7inch_HDMI_LCD_(C) — full product documentation including dimensions, mounting, and connector positions.

**Important — Neutrik D-size panel cutout template:**
All six D-size connectors on the back panel use the standard Neutrik D-size panel cutout. See `datasheets/Neutrik_D-size_panel_cutout.pdf` for the exact template. 

---

## Questions?

If anything is unclear, please feel free to ask for clarification – I'm happy to answer questions at any point.
