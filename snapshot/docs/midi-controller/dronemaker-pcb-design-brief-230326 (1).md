# Dronemaker MIDI Controller — PCB Design Brief (v2)

## What this is

A custom USB MIDI controller for a live music instrument. A Teensy 4.1
microcontroller reads 8 rotary encoders and 8 illuminated push buttons,
sending MIDI messages over USB. The buttons have RGB LEDs driven by
three daisy-chained 74HC595 shift registers. All controls are
panel-mounted in a separate enclosure and connect to this PCB via JST
XH connectors.

## Important note

This design brief and the accompanying netlist verification document
have been carefully prepared but not independently verified by a
professional. I would appreciate it if you could cross-check the
netlist and pin assignments against the component datasheets as part
of the design process, and flag anything that doesn't look right
before routing. I'd rather catch a mistake early than discover it
on a manufactured board.

## Power

Two rails, both supplied by the Teensy from USB:

- **+5V** from Teensy VIN pin. Supplies LED anodes on button headers
  (pin 3 on each 6-pin header) and bulk capacitor C4.
- **+3.3V** from Teensy 3.3V regulated output. Supplies all three
  74HC595 VCC and MR pins, decoupling caps C1-C3, and bulk capacitor C5.
- **GND** shared across everything.

Total current budget: ~250mA worst case (Teensy ~100mA, LEDs ~130mA
worst case at 220 ohm, shift registers ~1mA). Well within USB 500mA.

### Why the shift registers run at 3.3V (not 5V)

**This is an intentional design decision — please do not "correct" the
shift register VCC to 5V.**

The Teensy 4.1 is a 3.3V logic device. The 74HC595 VCC is tied to 3.3V
so that the input HIGH threshold (70% of VCC = 2.31V) is comfortably
met by the Teensy's 3.3V outputs. The LEDs still switch correctly
because they are common-anode (anodes on 5V, cathodes sunk through the
shift register outputs). Output LOW = ~0V = LED on. Output HIGH = ~3.3V
= 1.7V across LED = below forward voltage = LED off.

## Components

| Ref | Component | Footprint | Notes |
|-----|-----------|-----------|-------|
| T1 | Teensy 4.1 | 2x 1x24 pin socket, 15.24mm row spacing | Removable. USB end at board edge. See pinout below. |
| U1 | 74HC595 (DIP-16 in socket) | DIP-16_W7.62mm_Socket | IC #1 - GREEN LED cathodes |
| U2 | 74HC595 (DIP-16 in socket) | DIP-16_W7.62mm_Socket | IC #2 - RED LED cathodes |
| U3 | 74HC595 (DIP-16 in socket) | DIP-16_W7.62mm_Socket | IC #3 - BLUE LED cathodes |
| R1-R8 | 220 ohm axial | R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm | U1 Q0-Q7 to button green cathodes |
| R9-R16 | 220 ohm axial | Same | U2 Q0-Q7 to button red cathodes |
| R17-R24 | 220 ohm axial | Same | U3 Q0-Q7 to button blue cathodes |
| C1-C3 | 100nF ceramic | C_Disc_D3.0mm_W1.6mm_P2.50mm | Decoupling, one per shift register |
| C4 | 47uF electrolytic | CP_Radial_D5.0mm_P2.50mm | Bulk cap, +5V to GND, near Teensy |
| C5 | 47uF electrolytic | CP_Radial_D5.0mm_P2.50mm | Bulk cap, +3.3V to GND, near shift registers |
| E1-E8 | JST XH 4-pin header | JST_XH_B4B-XH-A_1x04_P2.50mm_Vertical | Encoder connections |
| B1-B8 | JST XH 6-pin header | JST_XH_B6B-XH-A_1x06_P2.50mm_Vertical | Button connections |

All components are through-hole. All ICs are in sockets.

## Teensy 4.1 physical pin layout

**This is the most critical section of this document.** The Teensy 4.1
does NOT have a simple "pins 0-23 on one side, pins 24+ on the other"
layout. The pin numbering is interleaved across both rows, and the top
row runs in reverse order. Please verify against the Teensy 4.1 pinout
card (included with the datasheet links).

Orientation: USB connector on the left, looking down at the top of the
board. The Teensy sits in two 1x24 pin socket headers with 15.24mm row
spacing and 2.54mm pin pitch.

### Bottom row (T1_BOT) - left to right from USB end

| Pad | Teensy pin | Signal | Net name |
|-----|-----------|--------|----------|
| 0 | GND | Ground | GND |
| 1 | 0 | Encoder 1 channel A | ENC1_A |
| 2 | 1 | Encoder 1 channel B | ENC1_B |
| 3 | 2 | Encoder 2 channel A | ENC2_A |
| 4 | 3 | Encoder 2 channel B | ENC2_B |
| 5 | 4 | Encoder 3 channel A | ENC3_A |
| 6 | 5 | Encoder 3 channel B | ENC3_B |
| 7 | 6 | Encoder 4 channel A | ENC4_A |
| 8 | 7 | Encoder 4 channel B | ENC4_B |
| 9 | 8 | Encoder 5 channel A | ENC5_A |
| 10 | 9 | Encoder 5 channel B | ENC5_B |
| 11 | 10 | Encoder 6 channel A | ENC6_A |
| 12 | 11 | Encoder 6 channel B | ENC6_B |
| 13 | 12 | Encoder 7 channel A | ENC7_A |
| 14 | 3.3V | Regulated 3.3V | +3V3 |
| 15 | 24 | Button 1 switch | BTN1 |
| 16 | 25 | Button 2 switch | BTN2 |
| 17 | 26 | Button 3 switch | BTN3 |
| 18 | 27 | Button 4 switch | BTN4 |
| 19 | 28 | Button 5 switch | BTN5 |
| 20 | 29 | Button 6 switch | BTN6 |
| 21 | 30 | Button 7 switch | BTN7 |
| 22 | 31 | Button 8 switch | BTN8 |
| 23 | 32 | Shift register DATA | DATA |

### Top row (T1_TOP) - left to right from USB end

**Note: GPIO pin numbers run in REVERSE order on this row.**

| Pad | Teensy pin | Signal | Net name |
|-----|-----------|--------|----------|
| 0 | VIN | 5V from USB | +5V |
| 1 | GND | Ground | GND |
| 2 | 3.3V | Regulated 3.3V | +3V3 |
| 3 | 23 | Encoder 8 push switch | ENC8_SW |
| 4 | 22 | Encoder 7 push switch | ENC7_SW |
| 5 | 21 | Encoder 6 push switch | ENC6_SW |
| 6 | 20 | Encoder 5 push switch | ENC5_SW |
| 7 | 19 | Encoder 4 push switch | ENC4_SW |
| 8 | 18 | Encoder 3 push switch | ENC3_SW |
| 9 | 17 | Encoder 2 push switch | ENC2_SW |
| 10 | 16 | Encoder 1 push switch | ENC1_SW |
| 11 | 15 | Encoder 8 channel B | ENC8_B |
| 12 | 14 | Encoder 8 channel A | ENC8_A |
| 13 | 13 | Encoder 7 channel B | ENC7_B |
| 14 | GND | Ground | GND |
| 15 | 41 | Spare | Not connected |
| 16 | 40 | Spare | Not connected |
| 17 | 39 | Spare | Not connected |
| 18 | 38 | Spare | Not connected |
| 19 | 37 | Spare | Not connected |
| 20 | 36 | Spare | Not connected |
| 21 | 35 | Spare | Not connected |
| 22 | 34 | Shift register LATCH | LATCH |
| 23 | 33 | Shift register CLOCK | CLOCK |

### Summary of signal distribution across rows

- **Bottom row carries:** encoder A/B for encoders 1-7 (pins 0-12),
  3.3V power, all 8 button switches (pins 24-31), and shift register
  DATA (pin 32).
- **Top row carries:** VIN/GND/3.3V power, encoder push switches for
  all 8 encoders (pins 16-23, reversed), encoder 7B + encoder 8 A/B
  (pins 13-15, reversed), GND, spare pins (41-35), and shift register
  LATCH + CLOCK (pins 34, 33).

## 74HC595 pin assignments (identical on all three)

| Pin | Function | Net |
|-----|----------|-----|
| 16 | VCC | +3.3V |
| 10 | MR (master reset) | +3.3V (held high = never reset) |
| 8 | GND | GND |
| 13 | OE (output enable) | GND (held low = outputs active) |
| 11 | SRCLK | CLOCK (shared, from Teensy pin 33) |
| 12 | RCLK | LATCH (shared, from Teensy pin 34) |
| 15 | Q0 | Output 0 -> 220R -> button 1 cathode |
| 1 | Q1 | Output 1 -> 220R -> button 2 cathode |
| 2 | Q2 | Output 2 -> 220R -> button 3 cathode |
| 3 | Q3 | Output 3 -> 220R -> button 4 cathode |
| 4 | Q4 | Output 4 -> 220R -> button 5 cathode |
| 5 | Q5 | Output 5 -> 220R -> button 6 cathode |
| 6 | Q6 | Output 6 -> 220R -> button 7 cathode |
| 7 | Q7 | Output 7 -> 220R -> button 8 cathode |
| 14 | SER (data in) | See daisy chain below |
| 9 | Q7' (serial out) | See daisy chain below |

Note: Q0 is on physical pin 15, not pin 1.

### Which IC drives which LED colour

- U1 outputs -> 220R -> **green** cathodes (button header pin 5)
- U2 outputs -> 220R -> **red** cathodes (button header pin 4)
- U3 outputs -> 220R -> **blue** cathodes (button header pin 6)

### Daisy chain

    Teensy pin 32 (DATA) -> U1 pin 14 (SER)
    U1 pin 9 (Q7')      -> U2 pin 14 (SER)
    U2 pin 9 (Q7')      -> U3 pin 14 (SER)
    U3 pin 9 (Q7')        not connected

CLOCK (Teensy pin 33) and LATCH (Teensy pin 34) connect to all three
ICs in parallel.

## Connector pinouts

### Button headers (B1-B8): JST XH 6-pin

| Pin | Signal |
|-----|--------|
| 1 | Switch (to Teensy) |
| 2 | GND |
| 3 | LED anode (+5V) |
| 4 | Red LED cathode (from U2 via 220R) |
| 5 | Green LED cathode (from U1 via 220R) |
| 6 | Blue LED cathode (from U3 via 220R) |

### Encoder headers (E1-E8): JST XH 4-pin

| Pin | Signal |
|-----|--------|
| 1 | Channel A (to Teensy) |
| 2 | Channel B (to Teensy) |
| 3 | Push switch (to Teensy) |
| 4 | GND |

## Decoupling

| Cap | Type | Between | Placement |
|-----|------|---------|-----------|
| C1 | 100nF ceramic | U1 pin 16 (+3.3V) and pin 8 (GND) | As close to U1 as possible |
| C2 | 100nF ceramic | U2 pin 16 (+3.3V) and pin 8 (GND) | As close to U2 as possible |
| C3 | 100nF ceramic | U3 pin 16 (+3.3V) and pin 8 (GND) | As close to U3 as possible |
| C4 | 47uF electrolytic | +5V and GND | Near Teensy |
| C5 | 47uF electrolytic | +3.3V and GND | Near shift registers |

## Layout preferences

- Teensy on the left edge, USB connector accessible at the board edge
- Encoder headers (E1-E8) along the top
- Button headers (B1-B8) along the bottom
- Shift registers and resistors in the centre-right area
- Board will mount inside a Hammond 1456KK4 enclosure (internal
  dimensions approximately 250mm x 250mm x 65mm sloped)
- Board size is flexible - whatever works for clean routing
- Mounting holes at corners (M3, 5mm from each board edge)

These are suggestions - use whatever placement gives the cleanest routing.

## Signal characteristics

- All digital signals, no analog
- No high-speed signals — shift register clock is software-driven,
  well under 1MHz
- No impedance matching or differential pair requirements
- All switch inputs use Teensy internal pull-ups (no external
  pull-up resistors needed)
- No hardware debouncing is included; debouncing is handled in firmware

## Design constraints

- Please do not change connector pinouts, signal assignments, or
  component footprints without checking with me first
- For the Teensy 4.1, please use two generic 1x24 pin socket connector
  symbols in the schematic and map the nets exactly as listed in the
  physical layout tables above
- This is a prototype — standard hobby fab rules are fine (e.g.
  JLCPCB/PCBWay defaults, 1.6mm board, 1oz copper)
- Please include clear silkscreen labels for all connectors, IC
  orientation markers, and power rail test points


