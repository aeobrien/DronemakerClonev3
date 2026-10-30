# DronemakerClone — Ledger

> Standalone spectral drone instrument: Raspberry Pi 5, custom MIDI controller, modified Behringer UM2, touchscreen UI, housed in a Hammond console enclosure.

## Status

**Lane:** personal
**Phase:** Prototyping — PCB build + enclosure design review
**Last updated:** 2026-04-11

## Subsystems

| Subsystem | Status | Doc |
|-----------|--------|-----|
| Software — DSP Engine | Working (gain compensation implemented) | [link](subsystems/software-dsp-engine.md) |
| Software — UI & Control | Working (MIDI Learn fixed, Pi layout default on all platforms) | [link](subsystems/software-ui-control.md) |
| MIDI Controller | PCB layout bug found (SRCLK/RCLK swap on U1/U3). Firmware pin swap applied. U2 pin-lift needs rework. Buttons/encoders work independently of shift registers. | [link](subsystems/midi-controller.md) |
| Audio Interface (UM2 Mod) | Nearly complete — XLR/TRS jack + LEDs remaining, re-wire with heat shrink pending | [link](subsystems/audio-interface-um2.md) |
| Headphone Monitor | Architecture decided (TPA6133A2 on HAT, A10K pot + 1KΩ resistors, software untested) | [link](subsystems/headphone-monitor.md) |
| Raspberry Pi Platform | Working | [link](subsystems/raspberry-pi-platform.md) |
| Enclosure & Faceplate | Revision 1 files received from designer. Both revisions applied (screen/encoder clearance + locator holes). Dimensions verified. Hackspace contacted for CNC fabrication. | [link](subsystems/enclosure-faceplate.md) |

## Key Decisions

See [decisions/LOG.md](decisions/LOG.md) for the full decision log.

## Open Questions

### Panel Design
- Designer's Revision 1 files received 2026-04-09 (DXF + STEP). Full dimension comparison done — all within spec.
- Systematic +0.1mm manufacturing tolerance on all cutouts (standard CNC practice).
- Standoff-to-encoder clearance ~0.5mm after revision — tight but functional.
- Panel is ONE piece (faceplate + backplate = single bent aluminium)
- Cheltenham Hackspace contacted for CNC fabrication (visited open evening Apr 9, email sent Apr 10, membership requested)

### Hardware — PCB Build
- **PCB layout bug confirmed:** SRCLK (pin 11) and RCLK (pin 12) transposed on U1 and U3 footprints; U2 correct. Found via continuity testing 2026-04-11. Root cause: KiCad PCB net assignment error.
- **Firmware fix applied:** CLOCK_PIN and LATCH_PIN swapped in all firmware files (pin 34=CLOCK, pin 33=LATCH). This corrects U1 and U3.
- **U2 physical pin-lift attempted** but initial test showed poor results — plan is to undo, test U1 in isolation first to confirm firmware fix works, then redo U2 cross-wire with better connections.
- **Shift registers only drive LEDs** — buttons (pins 24-31) and encoders (pins 0-23) are read directly by Teensy and work independently. Production MIDI firmware can run without working LEDs.
- Encoder test passed all 8 sockets — PEC16-4xxxF pinout confirmed (A bottom-left, B bottom-middle, C/common bottom-right, S/W top interchangeable). Encoder loom wired and working.
- USB overcurrent triggered once during LED test — likely transient inrush, not reproduced. Monitor.
- 4 of 8 button looms have incorrect switch wiring. Wiring is consistent across all looms — systematic crimping error. 1 button has loose blue wire.

### Hardware
- Combo jack part: NCJ6FI-S vs NCJ9FI-S (pin compatibility with phantom power — does NOT block panel design)
- LED specs: verify 5mm replacement forward voltage/current vs original UM2 LEDs
- Internal PCB mounting: 4mm clips for UM2, VHB tape or clips for Pi + MIDI controller (decision pending dry fit)
- Dry fit needed: all boards in enclosure once MIDI PCB arrives

### Software
- Dual audio output in JUCE: needs testing (UM2 + DAC HAT via ALSA)
- Teensy firmware: encoder/button reading, USB MIDI, LED driving
- Software level meters on touchscreen
- Headphone PFL routing
- 14-bit MIDI CC for fine-resolution parameters?

### General
- Pi platform: real-time scheduling, PSU noise, ventilation

## Notes

Project docs organised under `docs/`:
- `docs/vision/` — Vision Statement, Technical Brief, Spec
- `docs/midi-controller/` — Build guide, PCB design brief, netlist verification, schematic, PCB layout
- `docs/audio-interface/` — UM2 Teardown + all teardown photos
- `docs/software/` — Encoder roadmap, Loop Playback Systems, Implementation Checklist, Future Ideas
- `docs/pi/` — Raspberry Pi Roadmap
- `docs/reference/` — SoundMagic PDF, Notepad, Example Code

Hardware files under `hardware/`:
- `hardware/midi-controller/` — KiCad project, gerbers, production files
- `hardware/enclosure/` — STEP models (original + iterations), FreeCAD scripts
- `hardware/enclosure/design-brief/` — CAD design brief, DXF layouts, datasheets, STEP models for outsourcing
- `hardware/components/` — Component STEP models (Waveshare display)
