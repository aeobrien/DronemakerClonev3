# Datasheet Verification Checklist

**Date:** 2026-04-15
**Purpose:** Values extracted from the adversarial review that must be verified against physical datasheets before Rev 2 fabrication.

## 74HC595 Shift Register (U1, U2, U3)

Check against your specific manufacturer's datasheet (NXP, TI, or ON Semi — they differ slightly).

| Parameter | Assumed Value | Datasheet Page | Verified? | Actual Value |
|-----------|--------------|----------------|-----------|-------------|
| VCC operating range | 2.0-6.0V | Abs Max / Recommended | [ ] | |
| Max sink current per output (IOL) at VCC=3.3V | 6mA | DC Characteristics | [ ] | |
| Max total GND pin current (IGND) | 70mA | Abs Max Ratings | [ ] | |
| VOL at IOL=6mA, VCC=3.3V | ~0.1-0.3V | DC Characteristics | [ ] | |
| VOH at IOH=?, VCC=3.3V | ~3.0-3.3V | DC Characteristics | [ ] | |
| Output clamp diode voltage (to VCC) | Exists? | ESD / Abs Max | [ ] | |
| 5V tolerance on outputs when VCC=3.3V | Not rated | Abs Max | [ ] | |
| Quiescent supply current (ICC) at 3.3V | ~1mA | DC Characteristics | [ ] | |
| SRCLK max frequency at 3.3V | ? MHz | AC Characteristics | [ ] | |
| Input rise/fall time requirements | ? ns | AC Characteristics | [ ] | |
| Pin 11 function | SRCLK (shift clock) | Pin diagram | [ ] | |
| Pin 12 function | RCLK (storage/latch clock) | Pin diagram | [ ] | |
| Pin 13 function | OE (output enable, active LOW) | Pin diagram | [ ] | |
| Pin 10 function | SRCLR (shift register clear, active LOW) | Pin diagram | [ ] | |
| Pin 14 function | SER (serial data input) | Pin diagram | [ ] | |
| Pin 9 function | Q7' (serial data output) | Pin diagram | [ ] | |

### Critical calculation to verify:

With VCC=3.3V and VOL from datasheet:
- Red LED current = (5V - Vf_red - VOL) / R_limit = ___mA
- Is this within the per-pin IOL max? [ ] Yes [ ] No
- With all 8 outputs sinking: total = ___mA. Under IGND max? [ ] Yes [ ] No

## PEC16 Rotary Encoder

| Parameter | Assumed Value | Datasheet Page | Verified? | Actual Value |
|-----------|--------------|----------------|-----------|-------------|
| Contact bounce time | 1-5ms | Electrical specs | [ ] | |
| Contact resistance (max) | ? ohm | Electrical specs | [ ] | |
| Max operating voltage | ? V | Abs Max | [ ] | |
| Mechanical life (cycles) | ? | Mechanical specs | [ ] | |
| Push switch bounce time | ? ms | Electrical specs | [ ] | |
| Push switch contact resistance | ? ohm | Electrical specs | [ ] | |
| Pulses per revolution | ? | Mechanical specs | [ ] | |
| Detent torque | ? | Mechanical specs | [ ] | |

### Critical calculation to verify:

With actual bounce time from datasheet:
- RC time constant (tau) with current design: 10k x 10nF = 100us
- Is tau >= bounce time? [ ] Yes [ ] No (if No, increase cap)
- Recommended tau: at least 1x bounce time, ideally 2x

## Adafruit 3350 RGB Pushbutton

| Parameter | Assumed Value | Datasheet Page | Verified? | Actual Value |
|-----------|--------------|----------------|-----------|-------------|
| LED type | Common-anode RGB (NOT NeoPixel) | Product page / datasheet | [ ] | |
| Built-in current-limiting resistors? | Unknown — MUST CHECK | Schematic / product page | [x] | Yes — datasheet rates "LED voltage: 6V" (PM161F-10E datasheet, Adafruit_3350.pdf). Same physical button sold as 6V and 12V variants, so internal series R is required. Verified 2026-04-18. |
| If built-in resistors, values | N/A | Schematic | [x] | Not stated explicitly. Inferred ~200R red / ~150R green-blue from (Vsupply - Vf) / 20mA. At 5V drive with 220R external: ~7mA red, ~5-6mA green/blue — within 74HC595 absolute-max IOL. |
| Red LED forward voltage (Vf) | ~2.0V | LED specs | [ ] | |
| Green LED forward voltage (Vf) | ~3.0-3.2V | LED specs | [ ] | |
| Blue LED forward voltage (Vf) | ~3.0-3.2V | LED specs | [ ] | |
| Max LED forward current (If) | ~20mA typical | LED specs | [ ] | |
| Switch bounce time | ? ms | Switch specs | [ ] | |
| Pin assignment (which pin is anode, R, G, B, switch) | ? | Pinout diagram | [ ] | |

### CRITICAL: Is this a NeoPixel (WS2812)?

If the Adafruit 3350 is NeoPixel-based, the entire 74HC595 LED driving design is INVALID. NeoPixels require a timed serial data protocol, not parallel sink driving. **Verify this FIRST.**

- [ ] Confirmed: Adafruit 3350 is standard common-anode RGB (compatible with 595 design)
- [ ] Problem: Adafruit 3350 is NeoPixel-based (design must be reworked)

## Teensy 4.1 (IMXRT1062)

| Parameter | Assumed Value | Datasheet Page | Verified? | Actual Value |
|-----------|--------------|----------------|-----------|-------------|
| GPIO VIH threshold | ~0.7 x VCC = 2.31V | IMXRT1062 datasheet | [ ] | |
| GPIO VIL threshold | ~0.3 x VCC = 0.99V | IMXRT1062 datasheet | [ ] | |
| Internal pull-up resistance (INPUT_PULLUP) | ~22-37k ohm | IMXRT1062 datasheet | [ ] | |
| 3.3V regulator max output current | ~250mA (from module) | Teensy docs | [ ] | |
| VIN pin: 5V output capability | Via USB VBUS | Teensy docs | [ ] | |
| Pin 32 GPIO function | General purpose | Teensy pinout | [ ] | |
| Available hardware SPI pins | MOSI/SCK/CS | Teensy pinout | [ ] | |
| USB current negotiation | Configured for 500mA? | Firmware | [ ] | |
