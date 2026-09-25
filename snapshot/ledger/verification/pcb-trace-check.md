# PCB Trace Visual Inspection Checklist

**Date:** 2026-04-15
**Purpose:** Critical signal paths to visually trace on the PCB layout before ordering. Open the PCB in KiCad, highlight each net, and verify routing.

## Priority 1 — Must Check (show-stopper if wrong)

### 1.1 Shift Register Clock and Latch (Rev 1 bug area)

- [ ] **CLOCK net:** Trace from Teensy pin to U1 pin 11, then to U2 pin 11, then to U3 pin 11
  - All three connections are to pin 11 (not pin 12)
  - No unintended connections to other pins
- [ ] **LATCH net:** Trace from Teensy pin to U1 pin 12, then to U2 pin 12, then to U3 pin 12
  - All three connections are to pin 12 (not pin 11)
  - No unintended connections to other pins
- [ ] Clock and latch traces do not swap or cross incorrectly

### 1.2 Daisy Chain Data Path

- [ ] **DATA:** Teensy pin 32 -> U1 pin 14 (SER). Direct connection, no other loads on this net
- [ ] **U1 Q7' -> U2 SER:** U1 pin 9 -> U2 pin 14. Clean point-to-point
- [ ] **U2 Q7' -> U3 SER:** U2 pin 9 -> U3 pin 14. Clean point-to-point
- [ ] U3 pin 9 (Q7') is either unconnected or goes to a test point only

### 1.3 Power Connections

- [ ] **3.3V to all three SR VCC (pin 16):** Continuous trace/pour, not broken
- [ ] **GND to all three SR GND (pin 8):** Connected to ground plane
- [ ] **5V to LED anodes:** Trace the 5V distribution to all 8 button connectors (J9-J16)
- [ ] **No 3.3V/5V shorts:** Check that 3.3V and 5V rails do not connect

### 1.4 SRCLR Tied High

- [ ] U1 pin 10, U2 pin 10, U3 pin 10 all connect to 3.3V
- [ ] No accidental connection to GND or a signal net

### 1.5 OE Pin Connection

- [ ] Document what U1/U2/U3 pin 13 (OE) connects to: GND / Teensy GPIO / floating?
- [ ] If floating: **THIS IS A BUG** — outputs will be tristated
- [ ] All three OE pins connect to the same net

---

## Priority 2 — Should Check (functional correctness)

### 2.1 Encoder Debounce Networks (spot-check 2 of 16)

Pick two encoder channels (e.g., ENC1_A and ENC5_B) and trace the complete path:

For each channel:
- [ ] Encoder connector pin -> series 10k resistor -> Teensy GPIO pin
- [ ] Junction between series R and Teensy: 10k pull-up to 3.3V
- [ ] Junction between series R and Teensy: 10nF cap to GND
- [ ] Three components connect at the same node (series R output, pull-up, cap)
- [ ] Pull-up goes to 3.3V (not 5V)
- [ ] Cap goes to GND (not floating)

### 2.2 LED Current Limiting (spot-check 2 of 24)

Pick two LED outputs (e.g., U1 QA and U2 QH) and trace:

- [ ] SR output pin -> 220R resistor -> button connector pin (cathode)
- [ ] Button connector has 5V on anode pin and GND connection
- [ ] Resistor is in series (not bypassed or shorted)

### 2.3 Teensy Socket Connections

- [ ] J17 and J18 (24-pin headers) are correctly oriented for Teensy 4.1
- [ ] Pin 1 marking on PCB matches Teensy pin 1
- [ ] USB port end of Teensy aligns with board edge (for cable access)
- [ ] All encoder GPIO assignments match intended Teensy pins
- [ ] All SR control pins (DATA, CLOCK, LATCH) match intended Teensy pins

---

## Priority 3 — Layout Quality (reliability)

### 3.1 Decoupling Capacitor Placement

- [ ] C1 (100nF) is within 5mm of U1 VCC/GND pins
- [ ] C2 (100nF) is within 5mm of U2 VCC/GND pins
- [ ] C6 (100nF) is within 5mm of U3 VCC/GND pins
- [ ] Decoupling cap traces are short and direct (no long traces to VCC/GND)
- [ ] Caps connect to the IC's GND pin, not just to "somewhere on the ground plane"

### 3.2 Ground Plane

- [ ] Solid ground plane present (no large cutouts or splits under signal traces)
- [ ] Ground plane covers area under shift registers
- [ ] Ground plane covers area under encoder debounce networks
- [ ] No ground plane split between digital and analog sections that would force return current through a narrow neck

### 3.3 Signal Routing Separation

- [ ] SR clock/latch traces are NOT routed adjacent to encoder filter traces
- [ ] If they must cross, they cross at 90 degrees on different layers
- [ ] Minimum spacing between fast digital (SR clock) and filtered analog (encoder) nets: 3x trace width or 0.5mm, whichever is greater

### 3.4 Power Trace Width

- [ ] 5V trace to LED distribution: adequate width for worst-case current (~300mA)
  - Minimum recommended: 20 mil (0.5mm) for 300mA on outer layer
  - Preferred: 30+ mil or copper pour
- [ ] 3.3V trace to SR cluster: adequate for ~50mA
  - Minimum: 10 mil (0.25mm)

### 3.5 Connector Footprints

- [ ] All 8x JST 4-pin (J1-J8) footprints match actual connector part
- [ ] All 8x JST 6-pin (J9-J16) footprints match actual connector part
- [ ] Pin 1 orientation is consistent and marked on silkscreen
- [ ] Connectors are accessible (not blocked by other components)
- [ ] Adequate board edge clearance for connector mating

---

## Notes

- When highlighting a net in KiCad, also check for any unexpected connections (shorts to other nets)
- Use DRC (Design Rule Check) to catch spacing violations
- Use ERC (Electrical Rule Check) to catch unconnected pins
- If any check fails, document the issue and fix before ordering
