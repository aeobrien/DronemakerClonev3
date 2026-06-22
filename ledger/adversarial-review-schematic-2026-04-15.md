# Adversarial Review — DronemakerClone MIDI Controller PCB Rev 2

**Date:** 2026-04-15
**Subject:** Schematic & design review before Rev 2 fabrication
**Method:** Multi-LLM adversarial review (3 roles x 3 providers = 9 critiques)

| Role | Claude (Opus 4.6) | GPT (o3) | Gemini (2.5 Pro) |
|------|-------------------|-----------|-------------------|
| Sceptic | 5 concerns, 5 suggestions | 5 concerns, 5 suggestions | 5 concerns, 5 suggestions |
| Experienced Builder | 5 concerns, 5 suggestions | 5 concerns, 5 suggestions | 5 concerns, 5 suggestions |
| Signal Integrity | 5 concerns, 5 suggestions* | 5 concerns, 5 suggestions | 5 concerns, 5 suggestions |

*Claude SI response required raw text extraction (JSON parse failed but content captured).

---

## Critical Issues (consensus across multiple reviewers)

### 1. LED CURRENT EXCEEDS 74HC595 RATINGS — UNANIMOUS (9/9)

Every reviewer flagged this. At 3.3V VCC, the 74HC595 max sink current is ~6mA/pin, with 70mA absolute max for the entire package GND pin. Current calculations:

- **Red LED** (Vf ~2.0V): (5V - 2.0V - 0.1V) / 220R = **13.2mA** — over 2x the per-pin rating
- **Green/Blue** (Vf ~3.0-3.2V): (5V - 3.1V - 0.1V) / 220R = **8.2mA** — still over spec
- **Worst case per IC** (8 channels active): >100mA total, vs 70mA abs max

**Recommended fix:** Increase LED resistors to **470R** minimum. At 470R:
- Red: ~6.2mA (within spec)
- Green/Blue: ~4.0mA (safe)
- Total per IC (8ch): ~40mA (under 70mA limit)

Alternative: Switch to dedicated LED drivers (TPIC6B595, TLC5916) or run 595s at 5V with level shifters.

### 2. ENCODER VOLTAGE DIVIDER BUG — FLAGGED BY 5/9

The 10k series resistor + 10k external pull-up creates a voltage divider. When encoder contact closes (grounding through 10k series R), the Teensy pin sees:

- With external 10k pull-up only: V = 10k/(10k+10k) x 3.3V = **1.65V** — in the undefined zone
- With external 10k + internal ~30k pull-up: V = 7.5k/(7.5k+10k) x 3.3V = **~1.41V** — still marginal

Teensy 4.1 (IMXRT1062) VIH threshold is ~0.7 x VCC = 2.31V, VIL is ~0.3 x VCC = 0.99V. The voltage with contact closed lands in the undefined region.

**Recommended fix (choose one):**
- **Option A (simplest):** Remove external pull-ups. Use INPUT_PULLUP only (~30k internal). Series 10k + 30k pull-up gives safe low of ~0.82V. Increase cap to 100nF for tau = 30k x 100nF = 3ms.
- **Option B:** Keep external 10k pull-ups, reduce series R to 1k. Low voltage = 10k/(10k+1k) x 0V... actually Teensy sees ~0.3V. Change firmware to INPUT (not INPUT_PULLUP).

### 3. INADEQUATE DECOUPLING — FLAGGED BY 7/9

- Only 3x 100nF for three ICs — just barely adequate if placed correctly
- **No bulk capacitor on the 3.3V rail** — the 47uF caps appear to be on 5V only
- No high-frequency decoupling near the 5V LED power distribution point
- Simultaneous LED switching can draw up to 312mA transient on 5V rail

**Recommended fix:**
- Add 10-47uF bulk cap on 3.3V rail near SR cluster
- Add 100nF + 100uF near the 5V LED power entry point
- Verify each 100nF cap is within 3-5mm of its 595 VCC/GND pins

### 4. ENCODER PULL-UP CONFLICT — FLAGGED BY 5/9

Design specifies both external 10k pull-ups AND Teensy INPUT_PULLUP. These are contradictory — pick one strategy and be explicit about it. The redundancy changes the RC time constant and the voltage divider ratio.

**Recommended fix:** Keep external 10k pull-ups (they're on the board), change firmware to `INPUT` (not `INPUT_PULLUP`).

### 5. MISSING OE PIN MANAGEMENT — FLAGGED BY 4/9

The 74HC595 OE (pin 13) connection is not documented. If tied LOW permanently, outputs are undefined during Teensy boot (random LED flash, current spike on unstable 5V rail).

**Recommended fix:** Route OE to a Teensy GPIO with 10k pull-up to 3.3V (outputs disabled at power-on). Drive LOW in firmware after clearing shift registers. Also enables future PWM brightness control.

### 6. DEBOUNCE TIME CONSTANT TOO SHORT — FLAGGED BY 6/9

10k x 10nF = 100us tau, corner frequency ~1.6kHz. PEC16 encoder bounce is typically 1-5ms. The filter is too fast to meaningfully suppress bounce.

**Recommended fix:** Increase caps from 10nF to 100nF. With 10k pull-up: tau = 10k x 100nF = 1ms, fc ~160Hz. Much better matched to mechanical bounce. Combine with 2-4ms software debounce.

---

## Important Issues (flagged by multiple reviewers)

### 7. SHIFT REGISTER CLOCK SIGNAL INTEGRITY

Fast CMOS edges (2-5ns rise time) on unterminated clock/latch traces can ring, especially across three daisy-chained ICs. Risk of double-clocking and data corruption.

**Fix:** Add 33-47R series termination resistors at Teensy outputs for CLOCK and LATCH signals. Route these traces away from encoder filter traces (3x trace-width separation minimum).

### 8. CONNECTOR RELIABILITY

JST connectors (non-locking) under mechanical stress from encoder rotation and button presses are a known failure mode for performance MIDI controllers. 16 connectors with 80+ crimp terminals.

**Fix:** Consider locking connectors (JST-XH, Molex KK), or reinforce with hot glue for personal builds. Add mounting holes near Teensy USB port for strain relief.

### 9. NO ESD/PROTECTION

No TVS diodes on USB, no polyfuse on VIN, no ESD protection on external connectors. One static hit can kill the Teensy.

**Fix for stage use:** Add USBLC6-2 on USB lines, 500mA polyfuse on VBUS.

### 10. ADAFRUIT 3350 VERIFICATION NEEDED

One reviewer (Gemini Sceptic) flagged that Adafruit 3350 may be NeoPixel-based (WS2812), which would be fundamentally incompatible with the 595 driver topology. **This needs immediate verification against the actual datasheet.** If NeoPixels, the entire LED driving design is invalid.

Additionally, verify whether the Adafruit 3350 has built-in current-limiting resistors (common on Adafruit products). This changes the LED current calculation significantly.

---

## Suggestions Worth Considering

| Suggestion | Source | Priority |
|-----------|--------|----------|
| Use different resistor values per RGB colour for balanced brightness | Gemini Builder | Nice-to-have |
| Route SR data to hardware SPI pins for faster LED updates | Gemini Builder | Nice-to-have |
| Add test points on CLOCK, LATCH, DATA, encoder A/B | Claude Builder, Gemini Builder | Recommended |
| Add debug LED on spare GPIO | Claude Builder | Nice-to-have |
| Add silkscreen labels on all connectors | Gemini Builder | Recommended |
| Add mounting holes (especially near USB) | Gemini Builder | Recommended |

---

## Overall Assessment Summary

**Claude Sceptic:** "Architecture is sound, but LED current limiting is the most serious issue. Fix resistor values, clarify pull-up strategy, verify Adafruit 3350 built-in components before fabrication."

**GPT Sceptic:** "LED drive scheme and debounce implementation ignore datasheet limits. Fix over-current, back-feed, and edge-rate issues or expect early failures."

**Gemini Sceptic:** "Multiple critical flaws — potential Adafruit 3350 NeoPixel incompatibility, overcurrent on shift registers, missing push-button debounce."

**Claude Builder:** "Critical voltage divider error in encoder debounce will cause unreliable reads — must be fixed before fabrication."

**GPT Builder:** "Rev 2 fixes Rev 1 show-stoppers, but LED drive scheme, encoder filtering, and mechanical robustness need work."

**Gemini Builder:** "Strong revision with good theory, but practical weaknesses in mechanical robustness, assembly, and firmware performance."

**All Signal Integrity reviewers:** Consensus on inadequate decoupling, unterminated clock lines, and crosstalk risk between SR clock and encoder filter nets.

---

## Action Items Before Fabrication

1. **VERIFY** Adafruit 3350 is NOT NeoPixel-based (check datasheet immediately)
2. **INCREASE** LED resistors from 220R to 470R minimum
3. **FIX** encoder voltage divider (remove external pull-ups OR reduce series R)
4. **INCREASE** debounce caps from 10nF to 100nF
5. **ADD** bulk decoupling on 3.3V rail (10-47uF near SR cluster)
6. **ADD** bulk decoupling on 5V rail near LED power distribution
7. **ADD** 33R series termination on CLOCK and LATCH signals
8. **DOCUMENT** OE pin connection (consider routing to GPIO)
9. **ADD** test points on critical signals
10. **VERIFY** Adafruit 3350 built-in resistor values
