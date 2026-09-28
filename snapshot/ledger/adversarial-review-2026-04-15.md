# Adversarial PCB Review — 2026-04-15

**Scope:** Visual inspection of 4 updated PCB designs (Loom + DronemakerClone) from 3D KiCad renders.
**Method:** 3 roles (Sceptic, Experienced Builder, Signal Integrity) x 3 LLMs (Claude Opus 4.6, GPT o3, Gemini 2.5 Pro) = 9 independent critiques.
**Result:** 9/9 successful, 0 errors.

---

## Dronemaker-Specific Findings

### Unanimous Blockers

1. **Shift register SRCLK/RCLK pin swap (U1/U3, pins 11/12)** — Every single reviewer flagged this as completely unverifiable from renders. This was the most critical revision item. One reviewer noted there is "equal probability it was fixed in the wrong direction, or fixed on U1 but missed on U3." Must verify via schematic or netlist diff before fabrication.

2. **RC debounce network (32x 10k resistors + 16x 10nF caps)** — Component count appears correct in renders but values, topology (series vs pull-up positioning), and per-channel net assignments cannot be confirmed. A single misrouted net in the 48-component passive network is statistically plausible and invisible from renders.

### Assembly Concerns

3. **Dense passive arrays** — The debounce resistor/cap grid is tightly packed. Multiple reviewers recommended:
   - Replace discrete resistors with bussed resistor network arrays (RN packs)
   - Add silkscreen grouping boxes around each encoder's RC filter components
   - Ensure component values are printed on silkscreen where space permits

4. **Connector labeling** — Encoder connectors (ENC_1-ENC_8) and LED/switch connectors (SW1-SW8) need clear, readable labels. Pin-1 indicators on all headers.

### Signal Integrity

5. **Debounce component placement** — If series resistors and caps are placed centrally rather than adjacent to encoder connectors, long unfiltered traces will act as noise antennas. Should be as close to connector pins as possible.

6. **Shift register clock routing** — SRCLK/RCLK traces near analog paths (pot signals) could inject switching noise.

---

## Full Review (All 4 Boards)

The full review covering all Loom and Dronemaker boards is saved at:
`~/Dev/PCBBuilder/projects/loom-controller/adversarial-review-2026-04-15.md`

---

## Consensus: Actionable Next Steps for Dronemaker Brain

### Must-do before fabrication

1. **Request schematic PDF** — verify U1/U3 pins 11 (SRCLK) and 12 (RCLK) are correctly assigned
2. **Request netlist diff** between old and new revision — fastest way to confirm the swap
3. **Run ERC/DRC** in KiCad — request zero-error report
4. **Request BOM** — confirm all resistor values (10k) and capacitor values (10nF)

### Should-do

5. Request copper layer view around shift registers and debounce network
6. Verify debounce component proximity to encoder connector pins
7. Check for silkscreen readability at assembly scale

### Recommended improvements

8. Replace individual debounce resistors with resistor network arrays
9. Add silkscreen grouping boxes per encoder RC network
10. Add test points for key signals if not already present

---

## Raw Critiques (Dronemaker-Relevant Excerpts)

### Sceptic via Claude
> "The shift register SRCLK/RCLK pin swap fix is completely unverifiable from 3D renders alone. This was the single most critical electrical bug in the previous revision. Without schematics or netlist diff, we are taking on faith that the transposition was corrected -- and there is equal probability it was 'fixed' by swapping it the wrong direction, or that it was fixed on U1 but missed on U3."

### Sceptic via GPT
> "SRCLK/RCLK swap on the Dronemaker shift-registers is invisible in the 3D view; pins 11/12 could still be reversed, crippling LED driving with no external sign."

### Builder via Claude
> "Misplacing a single 10k series resistor with a 10k pull-up (identical values, different circuit function) would be nearly impossible to catch visually and would cause subtle encoder misbehavior."

### Signal Integrity via Claude
> "Without 1-10nF caps between each CD74HC4067 common output and GND, the sampled analog signals will carry digital switching noise from mux channel-select transitions directly into the Teensy ADC inputs."

### Signal Integrity via GPT
> "RC debounce networks are concentrated mid-board instead of at encoder connectors, creating long high-impedance traces that can pick up switching noise."
