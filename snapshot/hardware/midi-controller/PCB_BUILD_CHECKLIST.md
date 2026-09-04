# DronemakerClone MIDI Controller — PCB Build Checklist

> Everything you need to build the first version. Collect all parts first, then follow the phases in order.

## Parts to Collect

Gather everything below before starting. Tick each off as you find it.

### From Component Boxes

| Item | Qty | What to Look For | Got It? |
|------|-----|-----------------|---------|
| 220Ω resistors (axial, 1/4W) | 24 | Red-Red-Brown bands (or SMD markings if pre-sorted) | [ ] |
| 100nF ceramic capacitors | 3 | Small disc/rectangle marked "104" | [ ] |
| 47µF electrolytic capacitors | 2 | Cylindrical, polarity marked (stripe = negative) | [ ] |
| DIP-16 IC sockets | 3 | 16-pin dual-in-line sockets with notch at one end | [ ] |
| 74HC595 shift registers (DIP-16) | 3 | SN74HC595N, 16-pin IC with dot/notch marking pin 1 | [ ] |
| JST XH 4-pin headers (B4B-XH-A) | 8 | Small white right-angle or straight headers, 4 pins | [ ] |
| JST XH 6-pin headers (B6B-XH-A) | 8 | Same style, 6 pins | [ ] |
| Female pin socket strips (1×24, 2.54mm) | 2 | Long strips of female sockets for Teensy | [ ] |
| Teensy 4.1 | 1 | With headers already soldered (or solder them first) | [ ] |

### From Wiring/Connector Boxes

| Item | Qty | Notes | Got It? |
|------|-----|-------|---------|
| JST XH 4-pin pre-made cables | 8 | For encoder connections (from 460pc kit) | [ ] |
| JST XH 6-pin pre-made cables | 8 | For button connections (from 460pc kit) | [ ] |

### Pre-Assembled Sub-Assemblies

| Item | Qty | Notes | Got It? |
|------|-----|-------|---------|
| Encoder sub-assemblies (PEC16 + JST cable) | 8 | If you've already wired these up | [ ] |
| Button sub-assemblies (Adafruit 3350 + JST cable) | 8 | If you've already wired these up | [ ] |

### Tools Needed

| Tool | Got It? |
|------|---------|
| Soldering iron + solder | [ ] |
| Multimeter (for continuity + voltage checks) | [ ] |
| Flush cutters (for trimming leads) | [ ] |
| Helping hands / PCB holder | [ ] |
| USB cable (micro-USB or USB-C depending on your Teensy) | [ ] |
| Heat-shrink tubing (for button terminals) | [ ] |

---

## Build Phases

### Phase 1: Passives (~15 min)

Solder smallest components first. They're easiest to place when the board is flat.

| Step | Component | Ref | Notes | Done? |
|------|-----------|-----|-------|-------|
| 1.1 | 220Ω resistors | R1-R24 | No polarity — either direction is fine. Bend leads, insert, solder, trim. | [ ] |
| 1.2 | 100nF ceramic caps | C1, C2, C6 | No polarity. | [ ] |
| 1.3 | 47µF electrolytic caps | C4, C5 | **POLARITY MATTERS.** Longer leg = positive. Stripe on body = negative. Match to PCB silkscreen +/- markings. | [ ] |

**Checkpoint:** Visual inspection. No solder bridges? All joints shiny?

### Phase 2: Sockets & Headers (~20 min)

| Step | Component | Ref | Notes | Done? |
|------|-----------|-----|-------|-------|
| 2.1 | DIP-16 IC sockets | U1, U2, U3 | **Notch must match silkscreen.** All three notches face the same direction. Solder one corner pin first, check alignment, then complete. | [ ] |
| 2.2 | JST XH 4-pin headers | J1-J8 | Dry-fit a JST housing first to confirm orientation. Solder one pin, check it's flush, then complete. | [ ] |
| 2.3 | JST XH 6-pin headers | J9-J16 | Same — dry-fit first. | [ ] |
| 2.4 | Female socket strips | J17, J18 | **Alignment trick:** Insert the Teensy into the sockets (don't power it), place the whole assembly on the PCB, solder the socket strips while the Teensy holds them straight. Then remove the Teensy. | [ ] |

**Checkpoint:** All headers sitting flush? No tilted sockets?

### Phase 3: Power Test — BEFORE inserting ICs (~5 min)

**Do NOT insert the shift registers or Teensy yet.**

| Step | Action | Expected | Done? |
|------|--------|----------|-------|
| 3.1 | Plug Teensy into sockets, connect USB to computer | Teensy powers on (LED blinks) | [ ] |
| 3.2 | Measure 5V rail with multimeter (VIN pad or 5V bus point) | ~5.0V | [ ] |
| 3.3 | Measure 3.3V rail (3.3V pad on PCB) | ~3.3V | [ ] |
| 3.4 | Check continuity: 5V to GND | **Must NOT beep** (no short) | [ ] |
| 3.5 | Check continuity: 3.3V to GND | **Must NOT beep** (no short) | [ ] |
| 3.6 | Check continuity: 5V to 3.3V | **Must NOT beep** (no short) | [ ] |
| 3.7 | Unplug USB, remove Teensy | | [ ] |

**If any check fails: STOP. Find and fix the short/issue before proceeding.**

### Phase 4: Insert ICs (~5 min)

| Step | Component | Notes | Done? |
|------|-----------|-------|-------|
| 4.1 | Insert 74HC595 #1 into U1 socket | Pin 1 dot matches socket notch. Gently press — should not require force. If legs splay, carefully bend them inward on a flat surface. | [ ] |
| 4.2 | Insert 74HC595 #2 into U2 | Same orientation. | [ ] |
| 4.3 | Insert 74HC595 #3 into U3 | Same orientation. | [ ] |
| 4.4 | Insert Teensy into J17/J18 sockets | Align carefully — USB port should face the board edge. | [ ] |

### Phase 5: Firmware Upload & Basic Test (~10 min)

| Step | Action | Notes | Done? |
|------|--------|-------|-------|
| 5.1 | Connect USB to computer | Teensy should appear as USB device | [ ] |
| 5.2 | Open Arduino IDE, select Teensy 4.1, select USB Type: "Serial + MIDI" | | [ ] |
| 5.3 | Upload **Test 1: Serial Monitor** firmware | Tests basic connectivity | [ ] |
| 5.4 | Upload **Test 2: Button Test** firmware | Connect one button, verify in serial monitor | [ ] |
| 5.5 | Upload **Test 3: Encoder Test** firmware | Connect one encoder, verify rotation + push | [ ] |
| 5.6 | Upload **Test 4: LED Test** firmware | Connect one button, verify R/G/B individually | [ ] |
| 5.7 | Upload **Test 5: All Inputs** firmware | Connect all controls, verify all 8 buttons + 8 encoders | [ ] |
| 5.8 | Upload **Test 6: Full MIDI** firmware | Final firmware — verify MIDI output in DAW/MIDI monitor | [ ] |

### Phase 6: Connect All Controls (~30 min)

| Step | Action | Done? |
|------|--------|-------|
| 6.1 | Plug all 8 encoder JST cables into J1-J8 | [ ] |
| 6.2 | Plug all 8 button JST cables into J9-J16 | [ ] |
| 6.3 | Verify each encoder in serial monitor (turn + push) | [ ] |
| 6.4 | Verify each button in serial monitor (press + LED colours) | [ ] |
| 6.5 | Run MIDI firmware, verify all controls send correct MIDI | [ ] |

---

## Quick Reference

### Pin Assignments

| Teensy Pins | Signal |
|-------------|--------|
| 0-15 | Encoder 1-8 (A/B channels, pairs) |
| 16-23 | Encoder 1-8 push switches |
| 24-31 | Button 1-8 switches |
| 32 | Shift register DATA |
| 34 | Shift register CLOCK (swapped from original 33 — PCB layout fix) |
| 33 | Shift register LATCH (swapped from original 34 — PCB layout fix) |

### Shift Register Wiring

| Chip | Role | Byte Order |
|------|------|------------|
| U1 | Green LED cathodes | Shifted out LAST |
| U2 | Red LED cathodes | Shifted out SECOND |
| U3 | Blue LED cathodes | Shifted out FIRST |

### Key Rules
- Shift registers run on **3.3V** (not 5V)
- LED anodes connect to **5V**
- LOW output = LED ON, HIGH output = LED OFF
- All switches are active-low with **INPUT_PULLUP** in firmware
- ICs always in **sockets**, never soldered
