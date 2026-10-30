# MIDI Controller

## Overview

Custom USB MIDI controller built around a Teensy 4.1. Reads 8 rotary encoders (with push switches) and 8 illuminated RGB push buttons. Sends MIDI CC/Note messages over USB to the Raspberry Pi. RGB LEDs driven by three daisy-chained 74HC595 shift registers. All controls are panel-mounted and connect to the PCB via JST XH connectors.

## Status

**Current state:** All PCBs soldered (2026-04-10). Firmware tests 1-3 passed. Test 4 (LEDs) revealed PCB layout bug: SRCLK/RCLK (pins 11/12) transposed on U1 and U3 (U2 correct). Fix: firmware swaps CLOCK to pin 34, LATCH to pin 33; U2 gets physical pin-lift to cross-wire pins 11↔12. Fix applied to all firmware files, awaiting physical verification.
**Last updated:** 2026-04-11

## PCB

- **KiCad project:** `hardware/midi-controller/`
- **Board dimensions:** 163.6 × 81.1mm (fits Hammond enclosure comfortably)
- **Gerbers ready:** Pre-packaged for JLCPCB, PCBWay, Elecrow, FusionPCB in `gerber_to_order/`
- **Verified against:** PCB design brief, netlist verification doc, and build guide. All signal routing matches — encoder channels, button switches, shift register daisy chain, LED cathode routing, power rails, and Teensy pin assignments all correct.
- **Single PCB replaces the two-board perfboard architecture** from the original build guide. Strictly better — fewer connections, no inter-board cable failure points.

### Designator mapping (PCB vs. original docs)

| Original | PCB | Notes |
|----------|-----|-------|
| E1-E8 | J1-J8 | Encoder JST headers (standard KiCad J prefix) |
| B1-B8 | J9-J16 | Button JST headers |
| C3 | C6 | Third decoupling cap (renumbered) |

### Resistor value note

The build guide specifies 150Ω resistors; the PCB uses **220Ω**. The build guide itself noted 220Ω as the more conservative option. At 220Ω with 3.3V shift registers, LED current is ~5-6mA per channel (~45mA total for 8 channels on) — well under the 74HC595's 70mA package limit. LEDs will be slightly dimmer but clearly visible. The buttons also have built-in current-limiting resistors, so actual current is even lower.

## Components

| Component | Role | Part Number | Ref |
|-----------|------|-------------|-----|
| Teensy 4.1 | MCU — encoder/button reading, USB MIDI, LED driving | PJRC Teensy 4.1 | In 2×24 pin sockets (HC-PM254-8.5H-1x24PZ) |
| Rotary encoders ×8 | Parameter control (A/B channels + push switch) | PEC16-4220F-S0024 | JST XH 4-pin headers J1-J8 |
| RGB push buttons ×8 | Loop/mode selection with LED feedback | Adafruit 3350 (16mm 6V RGB momentary) | JST XH 6-pin headers J9-J16 |
| 74HC595 shift registers ×3 | LED cathode drivers (U1=green, U2=red, U3=blue) | SN74HC595N / 2-1571552-4 (DIP-16 socket) | Daisy-chained, socketed |
| Resistors ×24 | Current limiting for LEDs | 220Ω axial | R1-R24 |
| Decoupling caps ×3 | Per-IC decoupling | 100nF ceramic (SR215E104MAR) | C1, C2, C6 |
| Bulk caps ×2 | Rail stabilisation | 47µF electrolytic (ECA2AM470B) | C4 (+5V), C5 (+3.3V) |

## Connections

| Connects To | Interface | Notes |
|-------------|-----------|-------|
| Raspberry Pi Platform | USB MIDI | Class-compliant, no drivers needed |
| Software — UI & Control | MIDI CC/Note messages | CC 20-27 encoders, Notes 36-43 buttons, Notes 44-51 loop toggles |
| Enclosure & Faceplate | Panel-mount controls | Encoders and buttons mount in faceplate cutouts |

## Design Notes

**Power:** Two rails from Teensy USB — +5V (VIN) for LED anodes, +3.3V (regulated) for shift registers and logic. Total ~250-300mA worst case, well within USB 500mA.

**Shift registers at 3.3V:** Intentional — Teensy is 3.3V logic. VIH at 3.3V VCC = 2.31V, comfortably met by Teensy. Common-anode LEDs: 5V anode, cathode sunk through 595 outputs. LOW = LED on, HIGH (~3.3V) = 1.7V across LED = below Vf = LED off. If 74HCT595 DIP-16 becomes available, it's a drop-in replacement (swap VCC to 5V).

**Daisy chain:** Teensy pin 32 (DATA) → U1 SER → U1 Q7' → U2 SER → U2 Q7' → U3 SER. CLOCK (pin 33) and LATCH (pin 34) shared to all three. Firmware byte order: shift out blue FIRST (→U3), then red (→U2), then green LAST (→U1).

**Encoder pins:** Encoders 1-7 on bottom row (pins 0-13), encoder 8 on top row (pins 14-15). Push switches on top row (pins 16-23, reversed order). Buttons on pins 24-31.

**All components through-hole, all ICs socketed.** Prototype-friendly.

**Pull-ups:** All switch/encoder inputs use Teensy internal INPUT_PULLUP. No external pull-up resistors. Active-low (LOW = pressed/shorted).

**Adafruit 3350 button pinout:** 6 pins total. Blue, Green, Red = LED cathodes. C+ = common anode (5V). Two unlabelled pins = switch NO contacts (interchangeable).

**JST loom wiring (pre-wired 6-pin looms):** Wire colours do NOT match function names. Mapping:

| Wire Colour | PCB Pin | Function |
|-------------|---------|----------|
| Black | sw7 | Switch (to Teensy GPIO) |
| Red | gnd | Ground |
| White | 5v | LED common anode → C+ |
| Yellow | red | Red LED cathode |
| Blue | green | Green LED cathode |
| Green | blue | Blue LED cathode |

## Documentation

- Build guide: `docs/midi-controller/dronemaker-midi-controller-build-guide-v3.md`
- PCB design brief: `docs/midi-controller/dronemaker-pcb-design-brief-230326 (1).md`
- Netlist verification: `docs/midi-controller/netlist-verification-230326 (1).txt`
- Schematic PDF: `docs/midi-controller/MIDI Controller Schematic.pdf`
- PCB layout image: `docs/midi-controller/MIDI Controller PCB Layout.jpeg`

**PCB errata (rev 1):** SRCLK (pin 11) and RCLK (pin 12) are swapped on U1 and U3 footprints. U2 is correct. Root cause: KiCad PCB net assignment error — confirmed in `.kicad_pcb` file (U1/U3 pad 11 assigned to `/LATCH` net, pad 12 to `/CLOCK` net, opposite of U2). Workaround: firmware pin swap + U2 physical pin-lift. Fix in KiCad for rev 2: swap net assignments on U1/U3 pads 11 and 12, or fix the schematic wiring.

## Open Questions

- Firmware implementation (Teensy Arduino environment) — test sketch exists in build guide
- 14-bit MIDI CC for fine-resolution parameters (e.g., filter cutoff)?
- LED colour scheme / feedback patterns — build guide proposes: green=playing, red=recording, yellow (R+G)=recording while playing, blue=armed/ready, white (all)=special, off=idle. Colours set by Pi via incoming MIDI messages to Teensy.
- LED brightness with 220Ω + built-in resistors at 3.3V — may need real-world testing
