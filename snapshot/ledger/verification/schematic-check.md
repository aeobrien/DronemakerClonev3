# Schematic Pin-by-Pin Connection Checklist

**Date:** 2026-04-15
**Purpose:** Verify every pin connection on the three 74HC595 shift registers matches the intended design. Check each against the schematic AND the PCB netlist.

## 74HC595 Pin Reference (DIP-16)

| Pin | Function | Direction |
|-----|----------|-----------|
| 1-7 | QA-QG (parallel outputs 0-6) | Output |
| 8 | GND | Power |
| 9 | Q7' (serial data out) | Output |
| 10 | SRCLR (shift register clear, active LOW) | Input |
| 11 | SRCLK (shift register clock) | Input |
| 12 | RCLK (storage register clock / latch) | Input |
| 13 | OE (output enable, active LOW) | Input |
| 14 | SER (serial data input) | Input |
| 15 | QH (parallel output 7) | Output |
| 16 | VCC | Power |

---

## U1 — First Shift Register (receives data from Teensy)

| Pin | Expected Connection | Net Name | Schematic OK? | PCB Netlist OK? |
|-----|-------------------|----------|---------------|-----------------|
| 1 (QA) | LED channel via 220R | | [ ] | [ ] |
| 2 (QB) | LED channel via 220R | | [ ] | [ ] |
| 3 (QC) | LED channel via 220R | | [ ] | [ ] |
| 4 (QD) | LED channel via 220R | | [ ] | [ ] |
| 5 (QE) | LED channel via 220R | | [ ] | [ ] |
| 6 (QF) | LED channel via 220R | | [ ] | [ ] |
| 7 (QG) | LED channel via 220R | | [ ] | [ ] |
| **8 (GND)** | **GND** | GND | [ ] | [ ] |
| **9 (Q7')** | **U2 pin 14 (SER)** — daisy chain | | [ ] | [ ] |
| **10 (SRCLR)** | **3.3V (tied HIGH)** — never cleared | | [ ] | [ ] |
| **11 (SRCLK)** | **Shared CLOCK net from Teensy** | SR_CLK | [ ] | [ ] |
| **12 (RCLK)** | **Shared LATCH net from Teensy** | SR_LATCH | [ ] | [ ] |
| **13 (OE)** | **GND (tied LOW) or Teensy GPIO** — verify! | | [ ] | [ ] |
| **14 (SER)** | **Teensy pin 32 (DATA)** | SR_DATA | [ ] | [ ] |
| 15 (QH) | LED channel via 220R | | [ ] | [ ] |
| **16 (VCC)** | **3.3V** with 100nF decoupling (C1) | 3V3 | [ ] | [ ] |

### U1 Decoupling: C1 (100nF) between pin 16 (VCC) and pin 8 (GND)
- [ ] C1 present in schematic
- [ ] C1 placed within 5mm of U1 on PCB
- [ ] Short, direct traces (no vias in decoupling loop if possible)

---

## U2 — Second Shift Register (receives data from U1)

| Pin | Expected Connection | Net Name | Schematic OK? | PCB Netlist OK? |
|-----|-------------------|----------|---------------|-----------------|
| 1 (QA) | LED channel via 220R | | [ ] | [ ] |
| 2 (QB) | LED channel via 220R | | [ ] | [ ] |
| 3 (QC) | LED channel via 220R | | [ ] | [ ] |
| 4 (QD) | LED channel via 220R | | [ ] | [ ] |
| 5 (QE) | LED channel via 220R | | [ ] | [ ] |
| 6 (QF) | LED channel via 220R | | [ ] | [ ] |
| 7 (QG) | LED channel via 220R | | [ ] | [ ] |
| **8 (GND)** | **GND** | GND | [ ] | [ ] |
| **9 (Q7')** | **U3 pin 14 (SER)** — daisy chain | | [ ] | [ ] |
| **10 (SRCLR)** | **3.3V (tied HIGH)** | | [ ] | [ ] |
| **11 (SRCLK)** | **Shared CLOCK net** (same as U1 pin 11) | SR_CLK | [ ] | [ ] |
| **12 (RCLK)** | **Shared LATCH net** (same as U1 pin 12) | SR_LATCH | [ ] | [ ] |
| **13 (OE)** | **Same connection as U1 pin 13** | | [ ] | [ ] |
| **14 (SER)** | **U1 pin 9 (Q7')** — daisy chain input | | [ ] | [ ] |
| 15 (QH) | LED channel via 220R | | [ ] | [ ] |
| **16 (VCC)** | **3.3V** with 100nF decoupling (C2) | 3V3 | [ ] | [ ] |

### U2 Decoupling: C2 (100nF) between pin 16 (VCC) and pin 8 (GND)
- [ ] C2 present in schematic
- [ ] C2 placed within 5mm of U2 on PCB
- [ ] Short, direct traces

---

## U3 — Third Shift Register (last in chain)

| Pin | Expected Connection | Net Name | Schematic OK? | PCB Netlist OK? |
|-----|-------------------|----------|---------------|-----------------|
| 1 (QA) | LED channel via 220R | | [ ] | [ ] |
| 2 (QB) | LED channel via 220R | | [ ] | [ ] |
| 3 (QC) | LED channel via 220R | | [ ] | [ ] |
| 4 (QD) | LED channel via 220R | | [ ] | [ ] |
| 5 (QE) | LED channel via 220R | | [ ] | [ ] |
| 6 (QF) | LED channel via 220R | | [ ] | [ ] |
| 7 (QG) | LED channel via 220R | | [ ] | [ ] |
| **8 (GND)** | **GND** | GND | [ ] | [ ] |
| **9 (Q7')** | **Unconnected** (end of chain) or test point | | [ ] | [ ] |
| **10 (SRCLR)** | **3.3V (tied HIGH)** | | [ ] | [ ] |
| **11 (SRCLK)** | **Shared CLOCK net** (same as U1, U2) | SR_CLK | [ ] | [ ] |
| **12 (RCLK)** | **Shared LATCH net** (same as U1, U2) | SR_LATCH | [ ] | [ ] |
| **13 (OE)** | **Same connection as U1, U2 pin 13** | | [ ] | [ ] |
| **14 (SER)** | **U2 pin 9 (Q7')** — daisy chain input | | [ ] | [ ] |
| 15 (QH) | LED channel via 220R | | [ ] | [ ] |
| **16 (VCC)** | **3.3V** with 100nF decoupling (C6) | 3V3 | [ ] | [ ] |

### U3 Decoupling: C6 (100nF) between pin 16 (VCC) and pin 8 (GND)
- [ ] C6 present in schematic
- [ ] C6 placed within 5mm of U3 on PCB
- [ ] Short, direct traces

---

## Daisy Chain Verification

Trace the complete data path through all three shift registers:

| Segment | From | To | Net Name | Verified? |
|---------|------|----|----------|-----------|
| 1 | Teensy pin 32 | U1 pin 14 (SER) | SR_DATA | [ ] |
| 2 | U1 pin 9 (Q7') | U2 pin 14 (SER) | | [ ] |
| 3 | U2 pin 9 (Q7') | U3 pin 14 (SER) | | [ ] |
| 4 | U3 pin 9 (Q7') | Unconnected / TP | | [ ] |

## Shared Bus Verification

| Signal | Teensy Pin | U1 Pin | U2 Pin | U3 Pin | All Match? |
|--------|-----------|--------|--------|--------|------------|
| CLOCK | ? | 11 | 11 | 11 | [ ] |
| LATCH | ? | 12 | 12 | 12 | [ ] |
| OE | ? or GND | 13 | 13 | 13 | [ ] |
| SRCLR | 3.3V | 10 | 10 | 10 | [ ] |

## Cross-Check: Pin 11 vs Pin 12 (Rev 1 Bug)

This was the critical Rev 1 bug (SRCLK/RCLK swapped). Triple-check:

- [ ] Pin 11 on ALL three ICs connects to the CLOCK signal (SRCLK)
- [ ] Pin 12 on ALL three ICs connects to the LATCH signal (RCLK)
- [ ] Pin 11 is NOT connected to LATCH
- [ ] Pin 12 is NOT connected to CLOCK
- [ ] Verified in schematic
- [ ] Verified in PCB netlist
- [ ] Verified by visual trace on PCB layout
