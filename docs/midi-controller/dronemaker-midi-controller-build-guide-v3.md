# Dronemaker MIDI Controller

## Project Overview & Complete Build Guide (v3 — corrected)

---

## WHAT IS THIS?

The Dronemaker is a standalone electronic music instrument designed for
ambient and loop-based live performance. It runs custom music software on
a Raspberry Pi 5, housed in a Hammond 1456KK4 sloped desktop enclosure
alongside a gutted Behringer UM2 audio interface and a 7" touchscreen.

This document covers one sub-system of the Dronemaker: the **built-in
MIDI controller**. This is a custom USB MIDI controller built around a
Teensy 4.1 microcontroller. It provides the physical controls — buttons
and encoders — that the performer uses to interact with the music software
running on the Pi.

The MIDI controller connects to the Pi via USB. From the Pi's perspective,
it looks like any standard USB MIDI device. From the performer's
perspective, it's the entire control surface of the instrument.

---

## WHAT DOES THE MIDI CONTROLLER DO?

**8 rotary encoders with push switches.** Continuous-rotation knobs that
send MIDI CC messages as they're turned, controlling parameters in the
music software. Each encoder also has a push switch (press the shaft down)
which sends a separate MIDI message for mode switching, muting, etc.

**8 RGB illuminated buttons.** Momentary push buttons that send MIDI note
or CC messages when pressed. Each button has a built-in RGB LED ring
controlled by the Teensy, providing visual feedback about state:

- **Green** = loop is playing
- **Red** = loop is recording
- **Yellow** (red + green) = recording while playing
- **Blue** = channel armed / ready
- **White** (all three) = special state
- **Off** = idle

Any combination of red, green, and blue is possible. Colour assignments
are determined by the music software via incoming MIDI messages.

---

## SIGNAL CHAIN

```
Performer
    │
    ├── turns encoder ──→ Teensy reads A/B channels ──→ sends MIDI CC to Pi
    ├── pushes encoder ──→ Teensy reads switch pin ───→ sends MIDI CC to Pi
    ├── presses button ──→ Teensy reads switch pin ───→ sends MIDI note to Pi
    │
    │                      Pi music software decides what colour to show
    │                              │
    │                              ▼
    │                      Pi sends MIDI message to Teensy
    │                              │
    │                              ▼
    │                      Teensy shifts out 3 bytes to shift registers
    │                              │
    │                              ▼
    └── sees LED colour ←── Shift registers drive button RGB LEDs
```

All communication travels over a single USB cable, which also provides
power to the Teensy.

---

## DESIGN DECISIONS & JUSTIFICATIONS

This section explains key choices in the circuit design. It exists so
that anyone reviewing or continuing this build understands why things
are done the way they are.

### Why shift registers instead of direct GPIO?

Each RGB button has 3 LED channels. With 8 buttons, that's 24 LED
outputs. The Teensy 4.1 has 42 header pins, and after encoder and button
switch signals consume 35 of them, only 7 remain — not enough for 24 LEDs.
Three 74HC595 shift registers provide 24 outputs from just 3 Teensy pins.

### Why 74HC595 powered at 3.3V (not 5V)?

The Teensy 4.1 is a 3.3V logic device. Its GPIO pins output a maximum
of 3.3V. The 74HC595 shift register, when powered at 5V, requires a
minimum input HIGH voltage (VIH) of 3.5V (70% of VCC per the datasheet).
The Teensy's 3.3V output falls below this threshold.

While this works in practice for many hobby builds (the typical switching
threshold is ~2.5V, well below 3.3V), it is technically out of spec and
could behave unreliably with different chip batches or at temperature
extremes.

The ideal fix would be to use **74HCT595** chips (TTL-compatible inputs,
VIH = 2.0V), but these are now difficult to source in DIP-16 through-hole
packages — most stock is surface-mount only.

Our solution: **power the 74HC595s from the Teensy's 3.3V output** instead
of the 5V bus. At 3.3V VCC, the HC595's VIH threshold drops to 2.31V
(70% of 3.3V). The Teensy's 3.3V output is comfortably above this.
Logic levels are now properly matched and fully within spec.

**Why the LEDs still work correctly at 3.3V shift register VCC:**

The button LEDs are common-anode. The anodes connect to the 5V bus.
The shift register cathode outputs switch between ~0V (LOW) and ~3.3V (HIGH).

- **Output LOW (0V):** Voltage across LED = 5V - 0V = 5V. Current flows
  through the LED and its built-in resistor. LED is ON.
- **Output HIGH (3.3V):** Voltage across LED = 5V - 3.3V = 1.7V. This is
  below the forward voltage of the LED (~2V for red, ~3V for green/blue).
  No current flows. LED is fully OFF.

This gives clean on/off switching with no ghost illumination.

**If 74HCT595 DIP-16 chips become available later:** They are a direct
drop-in replacement. The chips are in sockets. Swap HC for HCT, change
the VCC wire from 3.3V back to 5V, and everything works. No other changes.

### Why 150Ω series resistors on every LED cathode?

The Adafruit 3350 buttons have built-in current-limiting resistors
designed for direct connection at 3–6V. However, when driven through a
shift register, the total package current matters. A 74HC595 has an
absolute maximum total GND pin current of approximately 70mA. At 5V
supply through the built-in resistors alone, each LED channel draws
approximately 20mA. With 8 LEDs on one chip simultaneously, that's
160mA — more than double the safe limit.

Adding a 150Ω external series resistor on each cathode line reduces the
current to approximately 8mA per channel. With all 8 channels on, that's
64mA — safely under the 70mA limit. At 8mA, the LEDs are still clearly
visible. This is important for the test phase where all LEDs may be on
simultaneously (e.g. the white test), and in normal use when multiple
loops are playing.

**With the shift registers running at 3.3V**, the output LOW voltage is
slightly higher (~0.4V rather than ~0.1V at 5V VCC), and the output
current capability is reduced. The 150Ω resistors ensure the current
stays well within the 3.3V-powered chip's comfortable operating range.
The brightness reduction is minimal and the LEDs are still clearly visible.

**Note:** The 150Ω value places the design near the upper safe range of
the 74HC595's total package current. If higher brightness or long-term
reliability is critical, consider 220Ω resistors or dedicated LED drivers.

### Why two boards?

A single 24 × 18 hole perfboard cannot fit the Teensy (24 pins per side),
8 encoder headers, 8 button headers, and 3 shift register ICs. Splitting
across two boards keeps the layout uncrowded. Board 1 places encoder
headers near the Teensy's left-side pins (0–23). Board 2 places button
headers near the shift register outputs they connect to, minimising wire
run length.

### Why sockets everywhere?

The Teensy sits in female socket headers. The shift registers sit in
DIP-16 IC sockets. This means any component can be removed without
desoldering — essential for debugging, replacing a faulty chip, or
swapping to HCT variants in the future.

### Pull-up strategy (important for firmware)

All button switches and encoder push switches are wired between the signal
pin and ground. They produce a LOW signal when pressed. **The Teensy's
internal pull-up resistors must be enabled in firmware** using
`pinMode(pin, INPUT_PULLUP)` for every switch input.

The Bourns PEC16 encoder channels A and B use the same arrangement — they
short to ground (the common pin C) as the shaft turns. The Encoder library
handles pull-ups internally, but for safety, the encoder A/B pins should
also be configured with INPUT_PULLUP.

Without pull-ups, the switch/encoder inputs float and produce garbage
readings. This is a firmware dependency, not a hardware one — there are
no external pull-up resistors in this build.

**Note on encoder noise:** If encoder readings are unstable, small
capacitors (10–100nF) from A and B to GND can help filter contact bounce.

### Power rail dependency

The LED anodes connect to the 5V bus and the shift register cathode
outputs switch between 0V and 3.3V. Both rails must be present together
for correct LED switching — do not power one rail independently of the
other. In this design both rails are always present via USB power, so
this is not a concern during normal operation, but be aware of it if
testing with bench supplies.

### ESD diode concern (non-issue)

With LED anodes at 5V and shift register outputs at 3.3V (when HIGH),
there is 1.7V across the LED in the off state. This is below the forward
voltage of every LED colour (~2V red, ~3V green/blue), so no forward
current flows and no reverse current path through the shift register's
internal ESD diodes is activated.

---

## POWER BUDGET

| Component | Current draw |
|-----------|-------------|
| Teensy 4.1 | ~100mA |
| Worst case LEDs (all 24 on at 8mA each with 150Ω resistors) | ~192mA |
| Shift registers (3× quiescent) | ~1mA |
| **Total worst case** | **~293mA** |

The Raspberry Pi 5's USB ports can supply 500mA per port (600mA with
USB 3.0). At 293mA worst case, this build is comfortably within the USB
power budget. Typical draw will be lower since not all 24 LED channels
will be on simultaneously in normal use.

---

## COMPLETE COMPONENT LIST

| Component | Qty | Purpose |
|-----------|-----|---------|
| Teensy 4.1 (Ethernet variant, with headers) | 1 | Microcontroller — USB MIDI device |
| Bourns PEC16-4220F-S0024 rotary encoder | 8 | Continuous rotation control with push switch |
| Adafruit 3350 RGB 16mm momentary button | 8 | Loop triggers with RGB LED state feedback |
| 74HC595 shift register (DIP-16) | 3 | Drive 24 LED channels from 3 Teensy pins |
| DIP-16 IC socket | 3 | Socket for each shift register — replaceable |
| 100nF ceramic capacitor | 3 | Decoupling — one per shift register |
| 10–47µF electrolytic capacitor | 2 | Bulk decoupling — one per board |
| 150Ω resistor | 24 | Current limiting — one per LED cathode output |
| Perfboard 24 × 18 holes | 1 | Board 1 |
| Perfboard 24 × 36 holes (double-wide: two 5×7cm boards left connected) | 1 | Board 2 |
| Female socket header strips (8-pin, 2.54mm) | 6 | Mount Teensy removably |
| JST XH 6-pin header (XH-6A) | 8 | Board-mount for buttons |
| Pre-made JST XH 6-pin cable (XH-6Y housing, 6 wires) | 8 | Wire-end for button sub-assemblies |
| JST XH 4-pin header (XH-4A) | 8 | Board-mount for encoders |
| Pre-made JST XH 4-pin cable (XH-4Y housing, 4 wires) | 8 | Wire-end for encoder sub-assemblies |
| JST XH 6-pin header (XH-6A) — inter-board | 6 | 3 per board, for disconnectable inter-board link |
| Pre-made JST XH 6-pin cable — inter-board | 3 | Connect Board 1 to Board 2 (pin 6 unused on each) |
| Solid aluminium knob caps (6mm) | 8 | Encoder shaft caps |
| Hookup wire (assorted colours) | — | Internal wiring, encoder bridge wires |
| Spade connectors (assorted) | — | Prototyping / test connections |

**Note on the Teensy Ethernet variant:** The base Teensy 4.1 has Ethernet
pads on the underside but no physical RJ45 jack. Some add-on kits include
one. Check your specific board — if it has a physical RJ45 jack on the end,
account for the extra length during dry-fit on the perfboard. The Ethernet
capability is unused in this project.

---

## PIN ASSIGNMENTS

### Teensy Left Side — Encoders (pins 0–23)

| Pin | Signal | Component |
|-----|--------|-----------|
| 0 | Encoder 1 — channel A | Enc 1 |
| 1 | Encoder 1 — channel B | Enc 1 |
| 2 | Encoder 2 — A | Enc 2 |
| 3 | Encoder 2 — B | Enc 2 |
| 4 | Encoder 3 — A | Enc 3 |
| 5 | Encoder 3 — B | Enc 3 |
| 6 | Encoder 4 — A | Enc 4 |
| 7 | Encoder 4 — B | Enc 4 |
| 8 | Encoder 5 — A | Enc 5 |
| 9 | Encoder 5 — B | Enc 5 |
| 10 | Encoder 6 — A | Enc 6 |
| 11 | Encoder 6 — B | Enc 6 |
| 12 | Encoder 7 — A | Enc 7 |
| 13 | Encoder 7 — B | Enc 7 |
| 14 | Encoder 8 — A | Enc 8 |
| 15 | Encoder 8 — B | Enc 8 |
| 16 | Encoder 1 — push switch | Enc 1 |
| 17 | Encoder 2 — push switch | Enc 2 |
| 18 | Encoder 3 — push switch | Enc 3 |
| 19 | Encoder 4 — push switch | Enc 4 |
| 20 | Encoder 5 — push switch | Enc 5 |
| 21 | Encoder 6 — push switch | Enc 6 |
| 22 | Encoder 7 — push switch | Enc 7 |
| 23 | Encoder 8 — push switch | Enc 8 |

### Teensy Right Side — via inter-board link (pins 24–34)

| Pin | Signal |
|-----|--------|
| 24–31 | Button 1–8 switch signals |
| 32 | Shift register DATA |
| 33 | Shift register CLOCK |
| 34 | Shift register LATCH |
| 35–41 | Spare (7 pins for future expansion) |

### Power pins

| Teensy Pin | Voltage | Destination |
|------------|---------|-------------|
| GND (×2 minimum) | 0V | Both boards' GND bus |
| VIN | 5V (from USB) | Both boards' 5V bus → LED anodes |
| 3.3V | 3.3V (regulated) | Board 2 3.3V rail → shift register VCC + MR |

---

## TWO-BOARD ARCHITECTURE

### Board 1 — Teensy + encoders (24 × 18 perfboard)

Contains: Teensy 4.1 (in socket headers), 8× encoder JST headers,
5V bus, GND bus, 3.3V rail, bulk decoupling cap, 3× inter-board JST
headers (J1, J2, J3).

### Board 2 — Buttons + shift registers (24 × 36 double-wide perfboard)

Uses two 5×7cm boards left connected, giving 24 rows × 36 usable columns
with a ~2-column seam gap at the midpoint.

Contains: 8× button JST headers, 3× 74HC595 (in IC sockets), 3× 100nF
caps, 24× 150Ω resistors, 5V bus, GND bus, 3.3V rail, bulk decoupling
cap, 3× inter-board JST headers (J1, J2, J3).

### Inter-board link (3× JST XH 6-pin cables)

The two boards connect via 3 pre-made 6-pin JST XH cables plugged into
matching headers (J1, J2, J3) on each board. Pin 6 is unused on all
three connectors.

**J1 — Button signals 1–5:**

| Pin | Signal | From Board 1 | To Board 2 |
|-----|--------|-------------|------------|
| 1 | Button 1 switch | Teensy pin 24 | B1 header pin 1 |
| 2 | Button 2 switch | Teensy pin 25 | B2 header pin 1 |
| 3 | Button 3 switch | Teensy pin 26 | B3 header pin 1 |
| 4 | Button 4 switch | Teensy pin 27 | B4 header pin 1 |
| 5 | Button 5 switch | Teensy pin 28 | B5 header pin 1 |
| 6 | (unused) | — | — |

**J2 — Button signals 6–8 + shift register control:**

| Pin | Signal | From Board 1 | To Board 2 |
|-----|--------|-------------|------------|
| 1 | Button 6 switch | Teensy pin 29 | B6 header pin 1 |
| 2 | Button 7 switch | Teensy pin 30 | B7 header pin 1 |
| 3 | Button 8 switch | Teensy pin 31 | B8 header pin 1 |
| 4 | Shift register DATA | Teensy pin 32 | Chip #1 pin 14 (SER) |
| 5 | Shift register CLOCK | Teensy pin 33 | All chips pin 11 (SRCLK) |
| 6 | (unused) | — | — |

**J3 — Shift register control + power:**

| Pin | Signal | From Board 1 | To Board 2 |
|-----|--------|-------------|------------|
| 1 | Shift register LATCH | Teensy pin 34 | All chips pin 12 (RCLK) |
| 2 | 5V | Board 1 5V bus | Board 2 5V bus |
| 3 | 3.3V | Board 1 3.3V rail | Board 2 3.3V rail |
| 4 | GND | Board 1 GND bus | Board 2 GND bus |
| 5 | GND | Board 1 GND bus | Board 2 GND bus |
| 6 | (unused) | — | — |

Note: Power is grouped on J3 so that voltage and ground are always
connected or disconnected together. Two GND pins (J3 pins 4 and 5)
reduce ground impedance — a single shared ground between digital
switching signals, LED current return, and sensitive encoder signals
invites noise. The boards can be separated for independent testing by
unplugging the three cables.

---

## CONNECTOR WIRING

### Button: JST XH 6-pin

| JST pin | Wire colour | Signal | Destination |
|---------|-------------|--------|-------------|
| 1 | Black | Switch NO | Teensy pin (via inter-board) |
| 2 | Red | Switch COM (GND) | GND bus |
| 3 | White | LED anode (+) | 5V bus |
| 4 | Yellow | Red cathode (−) | Chip #2 output via 150Ω resistor |
| 5 | Dark blue | Green cathode (−) | Chip #1 output via 150Ω resistor |
| 6 | Dark green | Blue cathode (−) | Chip #3 output via 150Ω resistor |

### Encoder: JST XH 4-pin

| JST pin | Wire colour | Signal | Destination |
|---------|-------------|--------|-------------|
| 1 | Black | Channel A | Teensy pin (0,2,4,6,8,10,12,14) |
| 2 | Red | Channel B | Teensy pin (1,3,5,7,9,11,13,15) |
| 3 | White | Push switch S2 | Teensy pin (16–23) |
| 4 | Yellow | GND (bridged C+S1) | GND bus |

Note on wire colour: These colours match pre-made JST XH cables. The
same colour carries different signals on button vs encoder looms (e.g.
black is switch NO on buttons but channel A on encoders). Within a
sub-assembly this is unambiguous — the JST pin count (6 vs 4) identifies
the loom type. If tracing a loose wire between boards, use the
inter-board wiring table to identify it.

---

## SHIFT REGISTER CIRCUIT (BOARD 2)

### 74HC595 pinout (DIP-16, notch at top)

```
         ┌───── U ─────┐
   Q1  1 │             │ 16  VCC ── 3.3V rail (NOT 5V)
   Q2  2 │             │ 15  Q0
   Q3  3 │             │ 14  SER (DATA in)
   Q4  4 │   74HC595   │ 13  OE (output enable)
   Q5  5 │             │ 12  RCLK (LATCH)
   Q6  6 │             │ 11  SRCLK (CLOCK)
   Q7  7 │             │ 10  MR (master reset)
  GND  8 │             │  9  Q7' (serial out → next chip)
         └─────────────┘
```

**IMPORTANT: VCC (pin 16) and MR (pin 10) connect to the 3.3V rail,
NOT the 5V bus.** This is the key design decision that solves the logic
level mismatch. See the Design Decisions section for full justification.

### Signal routing

```
Teensy pin 32 (DATA)  ─────────→  Chip #1 pin 14
Teensy pin 33 (CLOCK) ──┬───────→  Chip #1 pin 11
                        ├───────→  Chip #2 pin 11
                        └───────→  Chip #3 pin 11
Teensy pin 34 (LATCH) ──┬───────→  Chip #1 pin 12
                        ├───────→  Chip #2 pin 12
                        └───────→  Chip #3 pin 12
Chip #1 pin 9 (Q7') ───────────→  Chip #2 pin 14
Chip #2 pin 9 (Q7') ───────────→  Chip #3 pin 14
```

### Fixed connections (identical on all 3 chips)

| Pin | Name | Wire to | Why |
|-----|------|---------|-----|
| 16 | VCC | **3.3V rail** | Chip power — matched to Teensy logic |
| 8 | GND | GND bus | Ground |
| 10 | MR | **3.3V rail** | HIGH = never reset |
| 13 | OE | GND bus | LOW = outputs always active |

Plus: one 100nF ceramic cap between pin 16 and pin 8 on each chip.

### Decoupling note

In addition to the 100nF caps per chip, place a 10–47µF electrolytic
capacitor on Board 2 between the 3.3V rail and GND, near the shift
register area. This bulk cap absorbs current spikes when multiple LEDs
switch simultaneously. Place another bulk cap on Board 1 between 5V
and GND near the Teensy.

### Output map

**IMPORTANT — Byte order note:**

The shift registers are daisy-chained: Teensy → Chip #1 → Chip #2 → Chip #3.
When sending data via shiftOut, the **first byte shifted gets pushed to the
last chip** in the chain. Therefore, the firmware must send bytes in
REVERSE chain order: blue first (ends up in Chip #3), then red (Chip #2),
then green last (stays in Chip #1).

| Chip | Colour | Role | shiftOut order |
|------|--------|------|---------------|
| #1 | Green | Green cathodes | Sent LAST (3rd shiftOut call) |
| #2 | Red | Red cathodes | Sent 2nd |
| #3 | Blue | Blue cathodes | Sent FIRST (1st shiftOut call) |

**Chip #1 outputs → green cathodes (via 150Ω resistors):**

| Pin | Output | → Resistor → | Header pin |
|-----|--------|-------------|------------|
| 15 | Q0 | 150Ω → | B1 pin 5 |
| 1 | Q1 | 150Ω → | B2 pin 5 |
| 2 | Q2 | 150Ω → | B3 pin 5 |
| 3 | Q3 | 150Ω → | B4 pin 5 |
| 4 | Q4 | 150Ω → | B5 pin 5 |
| 5 | Q5 | 150Ω → | B6 pin 5 |
| 6 | Q6 | 150Ω → | B7 pin 5 |
| 7 | Q7 | 150Ω → | B8 pin 5 |

**Chip #2 outputs → red cathodes (via 150Ω resistors):**

Same pattern: pins 15,1,2,3,4,5,6,7 → 150Ω → B1–B8 pin 4.

**Chip #3 outputs → blue cathodes (via 150Ω resistors):**

Same pattern: pins 15,1,2,3,4,5,6,7 → 150Ω → B1–B8 pin 6.

Note: Q0 is on physical pin 15, not pin 1. Q1–Q7 are on pins 1–7.

### LED control in firmware

Common-anode, inverted logic: **bit 0 = LED ON, bit 1 = LED OFF.**

```cpp
void updateLEDs(byte green, byte red, byte blue) {
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, blue);   // → Chip #3 (last in chain)
    shiftOut(dataPin, clockPin, MSBFIRST, red);    // → Chip #2
    shiftOut(dataPin, clockPin, MSBFIRST, green);  // → Chip #1 (first in chain)
    digitalWrite(latchPin, HIGH);
}

// Examples (bit 0 = on, bit 1 = off):
updateLEDs(0x00, 0xFF, 0xFF);  // all 8 buttons green
updateLEDs(0xFF, 0x00, 0xFF);  // all 8 buttons red
updateLEDs(0xFF, 0xFF, 0x00);  // all 8 buttons blue
updateLEDs(0x00, 0x00, 0xFF);  // all yellow (red + green)
updateLEDs(0x00, 0x00, 0x00);  // all white (R + G + B)

// Button 1 only green (bit 0 = button 1, via Q0):
updateLEDs(0b11111110, 0xFF, 0xFF);
```

Note: bit 0 (LSB) corresponds to Q0, which is wired to button 1. Bit 7
corresponds to Q7, wired to button 8.

---

## BUILD INSTRUCTIONS

### Tools required

- Soldering iron + solder
- Wire strippers
- Multimeter with continuity mode
- Masking tape + pen (for labelling)
- Heat-shrink tubing (strongly recommended — see note below)
- Spade connectors (for prototyping/testing before soldering)
- Computer with Arduino IDE (for testing)

### Heat-shrink advisory

Heat-shrink is **strongly recommended** on all button terminal solder
joints and encoder bridge wires. The buttons are panel-mounted inside a
metal enclosure. Exposed solder joints near metal surfaces risk short
circuits. Use small-diameter heat-shrink on each individual terminal,
applied before soldering adjacent terminals.

### Wire colour convention

| Colour | Meaning |
|--------|---------|
| Black | Pin 1 signal (switch NO on buttons, channel A on encoders) |
| Red | Pin 2 signal (switch GND on buttons, channel B on encoders) |
| White | Pin 3 signal (LED anode on buttons, push switch on encoders) |
| Yellow | Pin 4 signal (red cathode on buttons, GND on encoders) |
| Dark blue | Pin 5 — green cathode (6-pin button loom only) |
| Dark green | Pin 6 — blue cathode (6-pin button loom only) |

---

### TASK 1: Prepare button looms (6-pin JST)

**Purpose:** 8 pre-made 6-pin JST XH cables, trimmed and stripped for
soldering to button terminals.
**Time:** 15–20 minutes.

These are pre-made JST XH cables — the housing end is already crimped.
You only need to cut them to length and strip the bare ends for soldering.

#### Trim and strip

For each of the 8 pre-made 6-pin cables:
1. Cut all 6 wires to 15cm from the JST housing
2. Strip ~5mm of insulation from each bare end (the soldering end)

#### Verify pin order

Confirm the wire colours match the expected pin assignments on each cable:
- Pin 1: Black (switch NO)
- Pin 2: Red (switch GND)
- Pin 3: White (LED anode)
- Pin 4: Yellow (red cathode)
- Pin 5: Dark blue (green cathode)
- Pin 6: Dark green (blue cathode)

Label each cable B1–B8.

**Output:** 8 labelled 6-pin JST looms, pre-crimped at the housing end,
stripped and ready for soldering at the component end.

---

### TASK 2: Build Board 2 — buttons + shift registers

**Purpose:** Assemble the button/shift register board.
**Time:** 90–120 minutes.

#### Step 1: Plan layout and bus rails

On the double-wide perfboard (24 rows × 36 usable columns, seam gap at
the midpoint):
- Run **5V bus** along the second-to-last row (for LED anodes)
- Run **GND bus** along the last row
- Run **3.3V rail** along a row near the shift registers (for chip VCC)
- Confirm none of the three rails touch each other

Bus rails run the full width of both halves, bridging the seam gap with
wire. No components should straddle the seam — place each component
entirely on the left or right half.

#### Step 2: IC sockets

1. Place IC #1 (green) on the left half. Place IC #2 (red) and IC #3
   (blue) on the right half, with space between them for resistors.
   Leave at least 3 columns to the left of each IC for flat-mounted
   resistors.
2. All notches pointing the same direction
3. Solder all pins
4. Label: #1 (green), #2 (red), #3 (blue)
5. **Do not insert chips yet**

#### Step 3: Fixed wiring per socket

For EACH of the 3 IC sockets:
1. Pin 16 (VCC) → wire to **3.3V rail** (NOT 5V bus)
2. Pin 8 (GND) → wire to GND bus
3. Pin 10 (MR) → wire to **3.3V rail** (NOT 5V bus)
4. Pin 13 (OE) → wire to GND bus
5. Solder a 100nF ceramic cap between pin 16 and pin 8

**Double-check:** VCC and MR go to 3.3V. OE and GND go to GND. Getting
VCC and MR mixed up with OE and GND will damage the chips.

#### Step 4: Bulk decoupling

Solder a 10–47µF electrolytic capacitor between the 3.3V rail and GND,
near the shift register area. **Observe polarity** — the longer leg is
positive (3.3V), the shorter leg or the stripe side is negative (GND).

#### Step 5: Shared clock, latch, and daisy chain

1. Wire socket #1 pin 11 → #2 pin 11 → #3 pin 11 (CLOCK)
2. Wire socket #1 pin 12 → #2 pin 12 → #3 pin 12 (LATCH)
3. Wire socket #1 pin 9 → socket #2 pin 14 (daisy chain).
   This wire crosses the seam gap — bridge it with a wire on the solder side.
4. Wire socket #2 pin 9 → socket #3 pin 14 (daisy chain)
5. Leave tails on socket #1 pins 11, 12, and 14 for wiring to inter-board JST headers

#### Step 6: Button JST headers

1. Place 8 JST XH-6A headers in two rows of 4 at the top of the board

**IMPORTANT — JST header orientation check:** Before soldering each
header, dry-fit an XH-6Y housing into it. Confirm that pin 1 on the
housing corresponds to the correct side of the header. JST XH connectors
are polarised (they only fit one way), but if you solder the header
facing the wrong direction, pin 1 and pin 6 swap. Mark pin 1 on the
perfboard with a pen before soldering.

2. Solder all 8 headers
3. Label B1–B8

#### Step 7: Button ground and anode buses

1. Daisy-chain all 8 headers' pin 2 (switch GND) → GND bus
2. Daisy-chain all 8 headers' pin 3 (LED anode) → **5V bus** (not 3.3V)

#### Step 8: Resistors and shift register outputs to button headers

For each of the 24 LED cathode connections, solder a 150Ω resistor
between the shift register output pin and the button header pin. The
resistor sits on the perfboard — one leg at the chip socket pin, the
other at the header pin.

**Chip #1 → 150Ω → green cathodes (header pin 5):**

| Socket #1 pin | → 150Ω → | Header | Pin |
|---------------|----------|--------|-----|
| 15 (Q0) | 150Ω | B1 | 5 |
| 1 (Q1) | 150Ω | B2 | 5 |
| 2 (Q2) | 150Ω | B3 | 5 |
| 3 (Q3) | 150Ω | B4 | 5 |
| 4 (Q4) | 150Ω | B5 | 5 |
| 5 (Q5) | 150Ω | B6 | 5 |
| 6 (Q6) | 150Ω | B7 | 5 |
| 7 (Q7) | 150Ω | B8 | 5 |

**Chip #2 → 150Ω → red cathodes (header pin 4):**

Same pattern: pins 15,1,2,3,4,5,6,7 → 150Ω → B1–B8 pin 4.

**Chip #3 → 150Ω → blue cathodes (header pin 6):**

Same pattern: pins 15,1,2,3,4,5,6,7 → 150Ω → B1–B8 pin 6.

#### Step 9: Inter-board JST headers

Solder 3× JST XH 6-pin headers (J1, J2, J3) at the bottom of the board.
These are the Board 2 side of the inter-board link. Before soldering each
header, dry-fit a 6-pin plug to confirm pin 1 orientation. Mark pin 1 on
the board.

Wire from each header to the board:
- J1 pins 1–5: wire to B1–B5 header pin 1 respectively
- J2 pins 1–3: wire to B6–B8 header pin 1 respectively
- J2 pin 4: wire to Chip #1 pin 14 (DATA/SER)
- J2 pin 5: wire to all 3 chips pin 11 (CLOCK/SRCLK)
- J3 pin 1: wire to all 3 chips pin 12 (LATCH/RCLK)
- J3 pin 2: wire to 5V bus
- J3 pin 3: wire to 3.3V rail
- J3 pins 4–5: wire to GND bus
- J1 pin 6, J2 pin 6, J3 pin 6: leave unconnected

**Output:** Board 2 complete except inter-board cables and chips.

---

### TASK 3: Build Board 1 — Teensy + encoders

**Purpose:** Assemble the Teensy board. Requires the Teensy for alignment.
**Time:** 45–60 minutes.

#### Step 1: Bus rails

On a second 24 × 18 perfboard:
- 5V bus along second-to-last row
- GND bus along last row
- 3.3V rail along a row near the Teensy (this taps from the Teensy's 3.3V pin)

#### Step 2: Teensy socket headers

1. Take 6 strips of 8-pin female socket header (3 per side)
2. Push Teensy's male pins into the sockets to hold alignment
3. Place assembly on perfboard
4. Flip and solder corner pins first, check alignment, then solder all
5. Remove Teensy (pull straight up)
6. Wire at least 2 GND socket pins → GND bus
7. Wire VIN socket pin → 5V bus
8. Wire 3.3V socket pin → 3.3V rail

#### Step 3: Verify power (with Teensy removed)

1. Multimeter continuity: GND socket pin ↔ GND bus = beep
2. VIN socket pin ↔ 5V bus = beep
3. 3.3V socket pin ↔ 3.3V rail = beep
4. GND bus ↔ 5V bus = NO beep
5. GND bus ↔ 3.3V rail = NO beep
6. 5V bus ↔ 3.3V rail = NO beep

If any of checks 4–6 beep, there is a short. Find and fix before continuing.

#### Step 4: Bulk decoupling

Solder a 10–47µF electrolytic cap between 5V bus and GND near the Teensy.
Observe polarity.

#### Step 5: Encoder JST headers

1. Place 8 JST XH-4A headers in two rows of 4, above the Teensy

**JST header orientation check:** Dry-fit an XH-4Y housing before soldering
each header. Confirm pin 1 alignment. Mark pin 1 on the board.

2. Solder and label E1–E8
3. Daisy-chain all pin 4 (GND) → GND bus

#### Step 6: Encoder signal wiring

| Header | Pin 1 (A) → Teensy | Pin 2 (B) → Teensy | Pin 3 (SW) → Teensy |
|--------|--------------------|--------------------|---------------------|
| E1 | 0 | 1 | 16 |
| E2 | 2 | 3 | 17 |
| E3 | 4 | 5 | 18 |
| E4 | 6 | 7 | 19 |
| E5 | 8 | 9 | 20 |
| E6 | 10 | 11 | 21 |
| E7 | 12 | 13 | 22 |
| E8 | 14 | 15 | 23 |

24 wires total.

#### Step 7: Inter-board JST headers

Solder 3× JST XH 6-pin headers (J1, J2, J3) below the Teensy area.
Dry-fit plugs before soldering to confirm pin 1 orientation.

Wire from Teensy socket pins and bus rails to each header:
- J1 pins 1–5: Teensy pins 24–28
- J2 pins 1–3: Teensy pins 29–31
- J2 pin 4: Teensy pin 32 (DATA)
- J2 pin 5: Teensy pin 33 (CLOCK)
- J3 pin 1: Teensy pin 34 (LATCH)
- J3 pin 2: 5V bus
- J3 pin 3: 3.3V rail
- J3 pins 4–5: GND bus
- J1 pin 6, J2 pin 6, J3 pin 6: leave unconnected

**Output:** Board 1 complete.

---

### TASK 4: Connect the two boards

**Purpose:** Link the two boards using pre-made JST cables.
**Time:** 2 minutes.

Plug 3 pre-made 6-pin JST XH cables between the matching headers:
- J1 on Board 1 ↔ J1 on Board 2
- J2 on Board 1 ↔ J2 on Board 2
- J3 on Board 1 ↔ J3 on Board 2

The cables are identical — orientation is enforced by the polarised JST
connectors. Confirm each plug clicks in fully.

**Output:** Both boards connected. Boards can be separated at any time
by unplugging the three cables.

---

### TASK 5: Prototype-test one button with spade connectors

**Purpose:** Verify button terminal identification and wiring plan using
temporary connections before soldering all 8 looms permanently.
**Time:** 20–30 minutes.

This task uses spade connectors and jumper wires to test ONE button
without soldering anything to it. If the wiring is wrong, you can
disconnect and try again with no damage.

#### Step 1: Identify switch terminals

1. Attach spade connectors to short jumper wires
2. Clip spade connectors onto the two gold switch terminals
3. Set multimeter to continuity
4. Touch probes to the other ends of the jumper wires
5. Should NOT beep. Press button — should beep. Release — stops.
6. That confirms which two terminals are the switch (NO and COM)

#### Step 2: Identify LED terminals

1. You need a 5V source. If Board 1 is built, plug the Teensy into USB
   and use the VIN (5V) and GND from the board. Otherwise use a 3V
   coin cell battery.
2. Attach spade connectors to jumper wires for 5V and GND
3. Refer to the button datasheet for likely terminal layout, then confirm:
4. Touch 5V to each of the 4 LED pins in turn while grounding the others
5. The **anode** is the one pin where, when you connect 5V to it, you can
   light any of the other three by grounding them individually
6. Once anode is found, ground each of the other three one at a time:
   - Note which lights RED
   - Note which lights GREEN
   - Note which lights BLUE
7. Mark all 6 terminals on the button with a pen or masking tape label
8. Photograph the terminal layout for reference

#### Step 3: Test with Board 2 (optional, if board is built)

1. Insert the 74HC595 chips into their sockets on Board 2
2. Plug the Teensy in and upload the test sketch from Task 8
3. Wire the button to a Board 2 header using spade connectors + jumper
   wires matching the JST pinout:
   - Header pin 1 ← switch NO
   - Header pin 2 ← switch COM
   - Header pin 3 ← LED anode
   - Header pin 4 ← red cathode
   - Header pin 5 ← green cathode
   - Header pin 6 ← blue cathode
4. Run the LED test — confirm correct colours light up
5. Press button — confirm Serial Monitor shows it

If colours are wrong, you've identified the terminals incorrectly. Re-test
with the multimeter/battery. Much better to discover this now than after
soldering 8 button looms.

**Output:** Confirmed terminal identification. Confidence to solder all 8.**

---

### TASK 6: Solder button looms to buttons

**Purpose:** Attach the pre-made 6-pin JST looms to button terminals.
**Time:** 60–90 minutes.

Using the terminal identification from Task 5:

For each button (×8):
1. Tin the stripped bare ends of all 6 wires with solder
2. **Apply heat-shrink** to each wire BEFORE soldering (slide it up the
   wire out of the way — you'll shrink it down over the joint after)
3. Black wire (JST pin 1) → switch terminal (NO or COM — either one)
4. Red wire (JST pin 2) → other switch terminal
5. White wire (JST pin 3) → LED anode (+)
6. Yellow wire (JST pin 4) → red cathode
7. Dark blue wire (JST pin 5) → green cathode
8. Dark green wire (JST pin 6) → blue cathode
9. Slide heat-shrink over each solder joint and shrink with heat gun
   or lighter held at distance

#### Test each button

Multimeter continuity on JST pin 1 and pin 2: beeps only when pressed.

**Output:** 8 button sub-assemblies with 6-pin JST plugs.

---

### TASK 7: Build encoder sub-assemblies

**Purpose:** Wire 8 encoders with 4-pin JST plugs.
**Time:** 60 minutes.

**Requires:** Bourns PEC16 encoders to have arrived.

#### Prepare cables

For each of the 8 pre-made 4-pin JST XH cables:
1. Cut all 4 wires to 15cm from the JST housing
2. Strip ~5mm of insulation from each bare end

Also cut 8× short bridge wire (~3cm, any colour).

#### Identify pins (once, on one encoder)

The Bourns PEC16 has 5 pins:
- Encoder side (3 in a row): A, C (middle = common/GND), B
- Switch side (2 pins): S1 (GND), S2 (signal)

Confirm with the datasheet and multimeter.

#### Wire each encoder (×8)

1. Bridge C to S1 with the short wire (both are ground)
2. **Apply heat-shrink** over the bridge wire solder joints
3. Black wire (JST pin 1) → pin A
4. Red wire (JST pin 2) → pin B
5. Yellow wire (JST pin 4) → bridge point (C/S1)
6. White wire (JST pin 3) → pin S2
7. Label E1–E8

#### Test each encoder

For each encoder, use multimeter continuity:
- **Push switch:** Pin 3 (white) ↔ Pin 4 (yellow): beeps only when shaft pushed down
- **Channel A:** Pin 1 (black) ↔ Pin 4 (yellow): intermittent beeps when turning
- **Channel B:** Pin 2 (red) ↔ Pin 4 (yellow): intermittent beeps when turning
- Confirm BOTH channels toggle. If only one beeps, that channel is not connected.

**Output:** 8 encoder sub-assemblies with 4-pin JST plugs.

---

### TASK 8: Pre-power safety checklist

**Purpose:** Verify all wiring before applying power for the first time.
**Time:** 15–20 minutes.

**Do this with the Teensy REMOVED and the 74HC595 chips NOT inserted.**

Using multimeter continuity, verify:

#### Board 1

| Test | Expected |
|------|----------|
| 5V bus ↔ GND bus | NO beep (no short) |
| 3.3V rail ↔ GND bus | NO beep (no short) |
| 5V bus ↔ 3.3V rail | NO beep (no short) |
| Each encoder header pin 4 ↔ GND bus | Beep (connected) |
| Teensy GND socket pins ↔ GND bus | Beep |
| Teensy VIN socket pin ↔ 5V bus | Beep |
| Teensy 3.3V socket pin ↔ 3.3V rail | Beep |

#### Board 2

| Test | Expected |
|------|----------|
| 5V bus ↔ GND bus | NO beep |
| 3.3V rail ↔ GND bus | NO beep |
| 5V bus ↔ 3.3V rail | NO beep |
| Each button header pin 2 ↔ GND bus | Beep (switch ground connected) |
| Each button header pin 3 ↔ 5V bus | Beep (LED anode connected) |
| IC socket pin 16 ↔ 3.3V rail (all 3) | Beep (VCC correct) |
| IC socket pin 8 ↔ GND bus (all 3) | Beep (GND correct) |
| IC socket pin 10 ↔ 3.3V rail (all 3) | Beep (MR correct) |
| IC socket pin 13 ↔ GND bus (all 3) | Beep (OE correct) |
| IC socket pin 16 ↔ 5V bus (all 3) | NO beep (VCC must NOT be on 5V) |

#### Inter-board cables

With all three inter-board cables plugged in:

| Test | Expected |
|------|----------|
| Board 1 GND bus ↔ Board 2 GND bus | Beep (connected via J3 pins 4+5) |
| Board 1 5V bus ↔ Board 2 5V bus | Beep (connected via J3 pin 2) |
| Board 1 3.3V rail ↔ Board 2 3.3V rail | Beep (connected via J3 pin 3) |
| Teensy pin 24 socket ↔ B1 header pin 1 | Beep (BTN1 via J1 pin 1) |
| Teensy pin 32 socket ↔ Chip #1 pin 14 | Beep (DATA via J2 pin 4) |
| Teensy pin 33 socket ↔ any chip pin 11 | Beep (CLOCK via J2 pin 5) |
| Teensy pin 34 socket ↔ any chip pin 12 | Beep (LATCH via J3 pin 1) |
| Adjacent J-connector pins ↔ each other | NO beep (no cross-shorts) |

Unplug each cable in turn and confirm the corresponding signals lose
continuity. This verifies the cables are carrying the signals and not a
solder bridge on the board.

#### Chip orientation check

Before inserting chips, confirm: the notch on each IC socket aligns with
the intended pin 1 location. Inserting a chip backwards will destroy it.

**Output:** All checks passed. Safe to insert chips and apply power.

---

### TASK 9: Final assembly and test

**Purpose:** Full system test.
**Time:** 30 minutes.

#### Step 1: Insert chips

Push the three 74HC595 chips into their sockets. Notch aligns with socket notch.

#### Step 2: Plug everything in

1. Push Teensy into Board 1 sockets
2. Plug all 8 button sub-assemblies into Board 2 headers
3. Plug all 8 encoder sub-assemblies into Board 1 headers
4. Connect Teensy to computer via USB

#### Step 3: Upload test sketch

```cpp
#include <Encoder.h>

const int dataPin = 32;
const int clockPin = 33;
const int latchPin = 34;

const int buttonPins[] = {24, 25, 26, 27, 28, 29, 30, 31};
const int encSwitchPins[] = {16, 17, 18, 19, 20, 21, 22, 23};

Encoder enc1(0, 1);
Encoder enc2(2, 3);
Encoder enc3(4, 5);
Encoder enc4(6, 7);
Encoder enc5(8, 9);
Encoder enc6(10, 11);
Encoder enc7(12, 13);
Encoder enc8(14, 15);
Encoder* encoders[] = {&enc1, &enc2, &enc3, &enc4,
                       &enc5, &enc6, &enc7, &enc8};

void updateLEDs(byte green, byte red, byte blue) {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, blue);   // → Chip #3 (last in chain)
  shiftOut(dataPin, clockPin, MSBFIRST, red);    // → Chip #2
  shiftOut(dataPin, clockPin, MSBFIRST, green);  // → Chip #1 (first in chain)
  digitalWrite(latchPin, HIGH);
}

void setup() {
  Serial.begin(9600);
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);

  for (int i = 0; i < 8; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);   // Active-low, internal pull-up
    pinMode(encSwitchPins[i], INPUT_PULLUP); // Active-low, internal pull-up
  }

  // Encoder A/B pins are handled by the Encoder library, which enables
  // internal pull-ups automatically. No additional configuration needed.

  Serial.println("=== DRONEMAKER MIDI CONTROLLER TEST ===");
  Serial.println("");
  Serial.println("Phase 1: LED colour cycle (watch the buttons)");
  delay(500);

  updateLEDs(0x00, 0xFF, 0xFF);
  Serial.println("All GREEN"); delay(1500);

  updateLEDs(0xFF, 0x00, 0xFF);
  Serial.println("All RED"); delay(1500);

  updateLEDs(0xFF, 0xFF, 0x00);
  Serial.println("All BLUE"); delay(1500);

  updateLEDs(0x00, 0x00, 0xFF);
  Serial.println("All YELLOW (red+green)"); delay(1500);

  updateLEDs(0xFF, 0x00, 0x00);
  Serial.println("All MAGENTA (red+blue)"); delay(1500);

  updateLEDs(0x00, 0xFF, 0x00);
  Serial.println("All CYAN (green+blue)"); delay(1500);

  updateLEDs(0x00, 0x00, 0x00);
  Serial.println("All WHITE (all on)"); delay(1500);

  updateLEDs(0xFF, 0xFF, 0xFF);
  Serial.println("All OFF"); delay(500);

  Serial.println("Chase green...");
  for (int i = 0; i < 8; i++) {
    byte p = ~(1 << i);
    updateLEDs(p, 0xFF, 0xFF);
    delay(150);
  }
  Serial.println("Chase red...");
  for (int i = 0; i < 8; i++) {
    byte p = ~(1 << i);
    updateLEDs(0xFF, p, 0xFF);
    delay(150);
  }
  Serial.println("Chase blue...");
  for (int i = 0; i < 8; i++) {
    byte p = ~(1 << i);
    updateLEDs(0xFF, 0xFF, p);
    delay(150);
  }

  updateLEDs(0xFF, 0xFF, 0xFF);
  Serial.println("");
  Serial.println("Phase 2: Interactive — press buttons and turn encoders");
  Serial.println("Buttons light green while held.");
  Serial.println("Watch Serial Monitor for readings.");
}

bool lastPressed[8] = {false, false, false, false, false, false, false, false};
long lastEncPos[8] = {0, 0, 0, 0, 0, 0, 0, 0};

void loop() {
  byte greenState = 0xFF;  // all off by default

  for (int i = 0; i < 8; i++) {
    bool pressed = (digitalRead(buttonPins[i]) == LOW);
    if (pressed) greenState &= ~(1 << i);
    if (pressed && !lastPressed[i]) {
      Serial.print("BTN "); Serial.print(i + 1); Serial.println(" PRESSED");
    }
    if (!pressed && lastPressed[i]) {
      Serial.print("BTN "); Serial.print(i + 1); Serial.println(" released");
    }
    lastPressed[i] = pressed;
  }

  updateLEDs(greenState, 0xFF, 0xFF);

  // Encoder push switches — print on press only
  for (int i = 0; i < 8; i++) {
    if (digitalRead(encSwitchPins[i]) == LOW) {
      Serial.print("ENC "); Serial.print(i + 1); Serial.println(" PUSH");
      delay(200);  // simple debounce
    }
  }

  // Encoder rotation — print on change
  for (int i = 0; i < 8; i++) {
    long pos = encoders[i]->read();
    if (pos != lastEncPos[i]) {
      Serial.print("ENC "); Serial.print(i + 1);
      Serial.print(" = "); Serial.println(pos);
      lastEncPos[i] = pos;
    }
  }

  delay(10);
}
```

#### Step 4: What to check

**LED colour cycle (automatic on power-up):**
- All 8 buttons should show: green → red → blue → yellow → cyan → magenta → white → off → chase
- If green and blue are swapped: the shiftOut byte order is wrong in firmware
- If all colours are wrong: chips may be in the wrong sockets (#1/#2/#3 ordering)
- If one button shows wrong colour: that button's cathode wires are crossed
- If no LEDs light at all: check 5V bus → anode daisy-chain, 3.3V rail → chip VCC,
  and inter-board 5V + 3.3V + GND connections

**Button test (press each):**
- Serial Monitor shows "BTN X PRESSED" on press, "BTN X released" on release
- Button lights green while held
- If a button doesn't register: check pin 1 wiring and inter-board JST cables

**Encoder test (turn and push each):**
- "ENC X = [value]" counts smoothly up/down when turned
- "ENC X PUSH" when shaft pressed down
- If encoder counts erratically or only in one direction: A and B wires may
  be swapped — just swap them at the JST plug (no resoldering needed)

**Output:** Everything works. Ready for MIDI firmware.

---

## TASK SEQUENCE

| Order | Task | What | When |
|-------|------|------|------|
| 1 | Task 1 | Prepare button looms (6-pin JST) | Now |
| 2 | Task 2 | Build Board 2 (shift registers + buttons) | Now / when 595s arrive |
| 3 | Task 3 | Build Board 1 (Teensy + encoders) | When Teensy arrives |
| 4 | Task 4 | Connect inter-board JST cables | Both boards done |
| 5 | Task 5 | Prototype-test one button with spade connectors | When buttons arrive |
| 6 | Task 6 | Solder looms to all 8 buttons | After Task 5 confirms wiring |
| 7 | Task 7 | Build encoder sub-assemblies | When encoders arrive |
| 8 | Task 8 | Pre-power safety checklist | Before first power-on |
| 9 | Task 9 | Full system test | Everything connected |

Tasks 5–7 can happen in any order depending on part arrival.
Task 8 must happen before Task 9.

---

## AFTER THE TEST

Once Task 9 passes, the hardware is complete. The next step is writing
the MIDI firmware — replacing the test sketch with code that sends proper
MIDI CC and note messages, and receives incoming MIDI to set LED colours.
The Teensy's USB MIDI library handles the protocol; you just need to map
each physical control to a MIDI channel/CC number and define the LED
colour responses.

The firmware, the music software running on the Pi, and the faceplate/
enclosure design are separate work outside the scope of this document.
