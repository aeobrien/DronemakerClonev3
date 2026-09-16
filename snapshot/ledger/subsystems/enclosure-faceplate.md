# Enclosure & Faceplate

## Overview

The physical housing for the instrument. A Hammond 1456KK4BKBK 30-degree slope console enclosure (10" × 10.2" × 4", aluminium) with a laser-cut faceplate carrying the touchscreen, encoders, buttons, pots, and LEDs, and a backplate carrying audio I/O connectors, USB ports, and the phantom power switch. All internal components (Pi, UM2 PCB, Teensy PCB, power supply, DAC HAT) mount inside.

## Status

**Current state:** All dimensions confirmed. Design brief prepared for outsourcing to a CAD designer (`hardware/enclosure/design-brief/`). Multiple STEP iterations attempted (v1–v9) — boolean approach produced artifacts. Professional CAD work needed to produce the final manufacturing-ready STEP/DXF.
**Last updated:** 2026-04-08

## Components

| Component | Role | Part Number | Ref |
|-----------|------|-------------|-----|
| Enclosure | Main housing | Hammond 1456KK4BKBK | 10" × 10.2" × 4", 30° slope, aluminium |
| Faceplate | Top panel with control cutouts | TBD — laser cut | Needs design file |
| Backplate | Rear panel with I/O cutouts | TBD — laser cut or hand-drilled | |

## Connections

| Connects To | Interface | Notes |
|-------------|-----------|-------|
| MIDI Controller | Panel-mount encoders + buttons | 8 encoder holes, 8 button holes in faceplate |
| Audio Interface (UM2 Mod) | Panel-mount jacks + pots + LEDs | Backplate: XLR/TRS, instrument, L/R out, phantom switch. Faceplate: pots, LEDs |
| Headphone Monitor | Panel-mount TRS socket | Location TBD |
| Raspberry Pi Platform | Internal mounting + screen cutout | Waveshare 7" screen visible through faceplate |

## Design Notes

**All mounting dimensions gathered — see [FACEPLATE_DIMENSIONS.md](../FACEPLATE_DIMENSIONS.md) for the complete reference.**

**Design brief v2:** `hardware/enclosure/design-brief/` — complete package for outsourcing:
- `DESIGN_BRIEF_v2.md` — written specification with dimensions table, layout intent, clearances
- `layout_slope_face.svg` — visual layout guide (slope: screen, encoders, pots, LEDs)
- `layout_front_face.svg` — visual layout guide (front: pushbuttons)
- `layout_back_panel.svg` — visual layout guide (back: audio jacks, USB)
- `models/` — FaceplateOriginal.step, 1456KK4BKBK_enclosure.stp, display.stp
- `datasheets/` — 9 PDFs: encoder, jacks, pot, enclosure, pushbutton, combo XLR, USB feedthrough, USB-C keystone, Neutrik D-size cutout template

**SVGs are layout guides, not technical drawings.** Designer owns technical accuracy; dimensions table is the source of truth.

**Screen orientation:** HDMI + micro-USB ports exit the **right edge** of the display board (viewed from front). Cutout centred on panel; mount holes offset −1.9mm from panel centre. **Screen is mounted inverted (180°)** — designer's CAD spec'd it this way, and mounting right-way-up would push the encoder row further from the screen. Display + touch rotation handled in software (`display_rotate=2` in `/boot/config.txt` or equivalent).

**Manufacturing:** Panel is pre-bent — requires CNC milling, not laser cutting. Needs local machine shop with fixturing capability.

**STEP iterations:** `hardware/enclosure/Faceplate_v1.step` through `v9` — exploratory, not manufacturing-ready. Use the design brief + original STEP for the final design.

### Faceplate Layout (top panel: 242.8 × 239.3mm usable, 1.63mm thick aluminium)

- **Buttons (8×):** Along the shortest edge of the device. 16mm holes, ~25mm centre-to-centre.
- **Screen:** On the sloped surface, centred.
- **Encoders (8×):** In a row below the screen. **9.3mm holes** (corrected from datasheet; previously spec'd 9.5mm). **3.3mm locator holes** 9mm above each encoder centre (previously missing from design — sent to designer 2026-04-08). Knobs (15.1mm dia) align with screen active area edges (154.2mm wide). Centre-to-centre: **19.87mm** (4.77mm gap between knobs, 3.07mm clearance between encoder bodies behind panel).
- **Pots — left side (2×, vertical):** Top = XLR/TRS gain, Bottom = instrument gain. 7mm holes.
- **Pots — right side (2×, vertical):** Top = master output volume, Bottom = headphone volume. 7mm holes.
- **LEDs (4×):** Two by each left-side pot (clip + signal indicators). **6.2mm holes** (7.8mm flange, measured 2026-04-02).
- **Level metering:** Software meters on touchscreen (not hardware). Avoids UM2 circuit modification.

**Cutout totals:** 1 rectangle (screen) + 8 encoder + 8 button + 4 pot + 4 LED = **1 rectangle + 24 round holes**

### Encoder Spacing Calculation

| Parameter | Value |
|-----------|-------|
| Display active width | 154.2mm |
| Encoder knob diameter | 15.1mm |
| 8 knobs total width | 120.8mm |
| Remaining space | 33.4mm |
| 7 inter-knob gaps | 4.77mm each |
| Centre-to-centre | 19.87mm |
| Encoder body width | 16.8mm |
| Behind-panel clearance | 3.07mm (min spec 1.2mm) |

Leftmost knob left edge aligns with left edge of display active area. Rightmost knob right edge aligns with right edge. Verified: works within tolerances.

### Knobs

| Knob | For | Body dia | Max dia | Height | Shaft | Source |
|------|-----|----------|---------|--------|-------|--------|
| Plain Aluminium (small) | Encoders ×8 | 15.1mm | 15.1mm | 15.2mm | 6mm round/splined, grub screw | [eBay](https://www.ebay.co.uk/itm/386817735048) |
| Solid Aluminium (20mm) | Pots ×4 | 18.0mm | 20.1mm | 15.5mm | 6mm round, grub screw | [eBay](https://www.ebay.co.uk/itm/386329209097) |

### Backplate Layout (rear panel: 233.4 × 72.2mm opening)

6 D-size + 1 keystone, left to right (viewed from rear):

| # | Component | Part | Notes |
|---|-----------|------|-------|
| 1 | Headphone output | Neutrik NJ3FP6C-BAG | 1/4" TRS |
| 2 | Left output | Neutrik NJ3FP6C-BAG | 1/4" TRS |
| 3 | Right output | Neutrik NJ3FP6C-BAG | 1/4" TRS |
| 4 | Instrument XLR | Neutrik NCJ6FI-S (or NCJ9FI-S — TBC) | Combo XLR/TRS mic/line in |
| 5 | TRS input | Neutrik NJ3FP6C-BAG | 1/4" TRS instrument in |
| 6 | USB-A data | Neutrik NAUSB3-B | USB 3.0, D-size cutout. USB-A outside, USB-B inside |
| 7 | USB-C power | TUK KUCC1820BPM | Pi power, 200mm pigtail. Standard keystone cutout: 16.2 × 14.7mm |

6 × D-size + 1 × keystone. At ~30mm D-size spacing = ~180mm + keystone ~20mm ≈ 200mm total. Fits within 233.4mm with ~17mm margin each side. Phantom power switch stays on UM2 board internally.

### Internal Layout

- Pi on standoffs, DAC HAT stacked on Pi GPIO
- UM2 PCB on standoffs
- MIDI controller PCB on standoffs (163.6 × 81.1mm)
- Cable management: separate digital and analogue wiring
- Internal USB cables: micro USB→A (Teensy→Pi), USB-A→B ×2 (UM2→Pi, Pi→Neutrik NAUSB3-B). All ~15cm.
- **Dry fit needed** to verify all boards fit and confirm clearances

### Mounting & Securing

**PCBs (Pi, UM2, MIDI controller) — mounting approach TBD, see open questions.**

**Waveshare screen:**
- **4 × M3 mounting holes** in the faceplate (sloped portion), 157.0 × 115.0mm centre-to-centre. Screen has built-in standoff pillars that sit flush against the panel.
- Screen bolts directly to the faceplate from behind — no adhesive standoffs needed. Use M3 screws through the panel into the screen's standoff pillars.
- Optional: 1mm foam tape between standoff pillars and panel surface as vibration damper.
- STEP file for verification: `https://files.waveshare.com/upload/f/f4/7inch_cad.zip`
- Total footprint incl. standoffs: ~164.9 × 115mm (standoffs extend ~4mm beyond board top/bottom).
- HDMI ribbon cable is the most fragile connection — route carefully, secure with a small cable tie or tape to prevent strain.

**DAC HAT:**
- Stacks directly on Pi GPIO header. The header friction + GPIO pins provide mechanical retention.
- If loose, add a nylon standoff between HAT and Pi board at the opposite corner from the GPIO header.

**Wiring:**
- All UM2 pot/LED/jack wiring to be re-done with **heat shrink tubing** on every solder joint. Gaps between pots are very tight — bare joints risk shorting.
- Use stranded hookup wire (22–26 AWG) for all connections to panel-mount components — flexes without breaking.
- Cut wire lengths precisely to minimise excess inside the enclosure.

**What NOT to use:**
- Hot glue — softens with Pi/PSU heat, peels off aluminium
- Epoxy — overkill, permanent, brittle under vibration

**Manufacturer:** Panel is pre-bent, so online laser-cut services (SendCutSend, Protocase flat panels) won't work. Need a local CNC machining shop that can fixture the bent panel and mill each surface. Search for "CNC machining" or "precision machining" locally.

**Ventilation:** Hammond enclosure has ventilation slots (22.2mm deep × 6.4mm wide). Pi 5 and switched-mode PSU generate heat. Aluminium case conducts heat well but thermal management needs consideration.

## Open Questions

### Panel Design (outsourced)
- Post design brief v2 on Fiverr and commission professional CAD work
- All datasheets gathered and included in brief package
- TUK keystone mounting: verify if screw holes needed when component arrives (~2026-04-07)
- Source CNC machining service for pre-bent panel (local machine shop or maker space)

### Assembly (after parts arrive)
- Combo jack: NCJ6FI-S vs NCJ9FI-S (pin compatibility with phantom power — does NOT block panel design, same D-size cutout)
- Internal PCB mounting — decision pending dry fit:
  - UM2 (4mm holes, ×4): 4mm self-adhesive clips
  - MIDI controller (5mm holes, ×4): VHB tape or 3.18mm clips + superglue
  - Pi 5 (2.7mm holes, ×4): VHB tape or adhesive foam pads
  - Simplest: 4mm clips for UM2, VHB tape for Pi + MIDI controller
- Internal cable routing — analogue/digital separation
- Thermal: passive ventilation via existing slots sufficient?
- Dry fit: all boards in enclosure once MIDI PCB arrives
