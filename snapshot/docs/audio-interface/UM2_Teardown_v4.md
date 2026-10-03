# Behringer UM2 Teardown & Integration Guide — Revision 4

> **Safety principle:** No connection in this guide is final until verified by continuity measurement, voltage measurement, and functional test. Every assumption is marked as such and accompanied by a validation step.

> **Revision note (v4):** This revision restructures Phases 2 and 3 into an incremental workflow. Instead of stripping all components, then wiring all replacements, each subsystem is now desoldered, wired, and tested before moving to the next. This means you always have a partially-functional board to fall back on if something goes wrong during a later step — particularly important given the risk of pad damage during desoldering of the combo XLR/TRS jack, which is now the second-to-last removal rather than occurring while the board is fully stripped.

---

## Project Context

### What We Are Building

The DronemakerClone is a standalone electronic instrument for performing and improvising ambient music, built around a spectral drone engine running as a JUCE C++17 application on a Raspberry Pi 5. The instrument takes live audio input, transforms it into evolving drones via real-time STFT processing, layers loops, applies effects (filter, delay, granular, tremolo, distortion, tape), and routes LFO modulation to effect parameters — all controlled from a physical enclosure with a capacitive touchscreen and rotary encoders.

The instrument is housed in a Hammond 1456KK4BKBK 30-degree slope console enclosure (10" × 10.2" × 4", aluminium). The faceplate carries a 7" touchscreen, 8 rotary encoders (PEC16-4220F-S0024), 8 RGB momentary pushbuttons (16mm), 4 potentiometers, and 4 LEDs. The backplate carries audio I/O connectors and a phantom power switch. A Teensy 4.1 inside the enclosure handles encoder/button reading and sends MIDI CC over USB to the Pi.

The enclosure also contains: a Raspberry Pi 5 with switched-mode power supply, a 7" capacitive touchscreen with HDMI and USB connections, the Teensy 4.1, LED drivers (SN74HC595N shift registers), and associated digital wiring. This is a significant amount of digital noise-generating circuitry sharing space with analogue audio paths — a consideration that affects wiring decisions throughout this guide.

### What This Document Covers

This document describes the process of taking a Behringer UM2 USB audio interface, removing its PCB from its original enclosure, desoldering the board-mounted connectors and controls, and wiring panel-mount replacement components that will be mounted in the DronemakerClone's laser-cut faceplate and backplate. The UM2's PCB — which contains the preamps, ADC/DAC, USB audio controller, and all supporting circuitry — is retained and mounted internally. The active analogue circuitry, power regulation, and digital conversion stages are retained, but many original interface components are removed and remotely wired with substitute parts. This changes the mechanical and some electrical characteristics of the front-end (pot values, wire lengths, connector types) while preserving the core signal processing and conversion chain.

This is a more invasive modification than simply remounting a board. Many of the original mechanical components have electrical behaviours beyond their obvious function — switching contacts, ground paths, impedance relationships — and removing them without understanding these behaviours can cause subtle or serious failures. This guide attempts to identify and address all such hidden dependencies, but the builder should treat every step as potentially revealing new information that changes the plan.

### Why a UM2

The UM2 provides mono input (via combo XLR/TRS and 1/4" instrument jacks) and stereo output, with phantom power, preamp gain control, and USB audio class compliance — everything needed for the instrument's audio I/O. Using an existing interface avoids the significant complexity of designing a custom audio input stage with appropriate impedance handling for microphones, instrument pickups, and line sources. The UM2 was chosen specifically because it is inexpensive, simple (single PCB, all through-hole user-facing components), and uses a USB audio class chip that Linux/ALSA supports natively.

### The UM2 Board

Board identifier: C003-01099-000 / PCB302082REVJ_01. Combo XLR/TRS jack manufactured by ZWEE (visible branding on jack face). Single-layer PCB. All user-facing components are through-hole, with solder joints accessible from the back of the board.

> **Board-specific warning:** All pad positions, pin counts, component values, and circuit behaviours described in this guide are specific to board revision PCB302082REVJ_01. Other UM2 units may have different board revisions with different layouts, component values, or circuit topology. If your board has a different identifier, treat every mapping step as unverified — do not assume pad positions or behaviours match this guide.

**Single-layer PCB implications:** On a single-layer board, every trace runs on one side only. There are no internal layers or vias to provide alternative routing. This means a lifted pad (copper trace torn from the board during desoldering) is harder to repair than on a multi-layer board — you may need to bridge the break with a wire, and identifying where the trace was going requires careful visual inspection. High-resolution photographs of both board sides before starting work are essential insurance.

The board provides:
- 1 × combo XLR/TRS input (mic/line, with 48V phantom power)
- 1 × 1/4" TS instrument input
- 1 × 1/4" TRS headphone output
- 2 × RCA stereo outputs (left and right)
- 1 × USB-B connection (audio interface to computer/host)
- 3 × potentiometers (mic/line gain, instrument gain, output volume)
- 1 × phantom power on/off switch
- 1 × direct monitor on/off switch
- 6 × LEDs (clip and signal for each input, plus power and phantom indicators)

### The Approach

The PCB is retained as the functional core of the audio system. The user-facing mechanical components — connectors, potentiometers, switches, and selected LEDs — are desoldered and replaced with panel-mount equivalents wired back to the original PCB pads using hookup wire. The board is then mounted inside the enclosure using standoffs through its existing mounting holes.

**Incremental workflow:** Each subsystem is desoldered, wired to its replacement, and tested before moving to the next. The USB-B socket remains in place throughout, providing a test connection to a computer. This means that at every stage, you have a partially-functional board and can verify that each modification works before committing to the next one. If a pad lifts or a trace is damaged during a difficult removal (particularly the combo XLR/TRS jack), you know that everything wired so far is already proven working.

Components that don't need panel access (direct monitor switch, power LED, phantom power LED) stay soldered to the board. The direct monitor switch will be set to "off" (output only) and physically secured in that position with a small amount of hot glue to prevent accidental changes during servicing.

The UM2's USB connection to the Pi is internal — a short USB cable runs inside the enclosure. No panel-mount USB socket is needed.

The UM2's headphone output is not relocated to the panel. An independent headphone monitoring system (separate DAC and headphone amp, driven from the Pi) replaces it entirely, providing an independent monitor bus for loops, sub-mixes, or click tracks. The headphone jack is removed from the board, but its pads require investigation before deciding whether to leave them open or bridge switching contacts (see Step 2.3).

---

## Safety Rules

These rules apply throughout the entire teardown and wiring process:

1. **Never solder, desolder, or rework any component while the board is powered.** Disconnect USB before any iron work.
2. **Phantom power discharge:** After powering down the board with phantom power engaged, wait at least 30 seconds before touching the combo jack or phantom switch pads. The 48V phantom supply charges capacitors that take time to discharge. Verify with a multimeter (measure DC between XLR pin 2/3 pads and pin 1 pad — should read <1V) before reworking.
3. **Never force a component during desoldering.** If it doesn't move freely, a pin is still connected. Forcing risks lifting a pad.
4. **Photograph both sides of the board before each major removal step.** On a single-layer PCB, these photos are your backup if a trace is damaged and needs to be reconstructed.
5. **Phantom power hot-plug caution:** Disengage phantom power before plugging or unplugging XLR microphones where practical, especially during test phases. While balanced phantom power should not damage properly wired equipment, hot-plugging with phantom engaged causes loud pops and increases the risk of mistakes during a prototype build.

---

## Components Summary

### Components to Remove from the UM2 Board

| # | Component | Board Label | Solder Points | Replacement |
|---|-----------|-------------|---------------|-------------|
| 1 | Combo XLR/TRS input jack | — | 11 (mix of signal, switching, and structural) | Neutrik NCJ6FI-S panel-mount combo jack (see Part Selection Warning in Step 1.6 — NCJ9FI-S may be required instead) |
| 2 | 1/4" instrument input jack | — | 5 (3 electrical + 2 structural; may include switch contact) | Neutrik NJ3FP6C-BAG panel-mount 1/4" jack |
| 3 | Headphone output jack | — | 5 (3 electrical + 2 structural; may include switching contacts in main output path) | Not replaced on panel. Pads may need bridging — see Step 2.3. |
| 4 | Left RCA output | — | 2 (signal + ground) | Neutrik NJ3FP6C-BAG panel-mount 1/4" jack (tip=signal, sleeve=ground) |
| 5 | Right RCA output | — | 2 (signal + ground) | Neutrik NJ3FP6C-BAG panel-mount 1/4" jack (tip=signal, sleeve=ground) |
| 6 | USB-B socket | — | 6 (~4 functional + ~2 structural/shield) | Internal USB cable to Pi (not panel-mounted) |
| 7 | Phantom power switch | S1 | 8 (6 functional + 2 structural) | Multicomp Pro 1MD1T1B1M1QE DPDT on-on toggle, panel-mount |
| 8 | Mic/line gain pot | VR1 (05C50K) | 8 (6 electrical + 2 structural) — dual-gang | Alpha 9mm dual-gang C50K, round shaft 6.35mm (Tayda RD902F-40-15R1-C50K) — exact resistance and taper match |
| 9 | Instrument gain pot | VR2 (W50K) | 5 (3 electrical + 2 structural) — single-gang | Alpha 9mm single-gang A50K, round shaft 6.35mm (Thonk RD901F-40-15R1-A50K) — same resistance, audio taper substituted for W taper |
| 10 | Output volume pot | VR3 (A20K) | 8 (6 electrical + 2 structural) — dual-gang | Alpha 9mm dual-gang A10K, round shaft 6.35mm (Tayda RD902F-40-15R1-A10K) — PROVISIONAL, see Pot Decisions below |
| 11 | Input 1 clip/signal LEDs | DS1 | TBD (may be 4 or 3 — check for shared pins) | 5mm red and green LEDs in 5mm panel-mount bezels |
| 12 | Input 2 clip/signal LEDs | DS2/DS3 | TBD (may be 4 or 3 — check for shared pins) | 5mm red and green LEDs in 5mm panel-mount bezels |

### Components Staying on the Board

| Component | Board Label | Setting | Notes |
|-----------|-------------|---------|-------|
| Direct monitor switch | S2 | Set to "off" (output only) | Secured with hot glue after setting. Hidden internal setting — document its position. |
| Power LED | — | Left as-is | Not needed on panel |
| Phantom power LED | — | Left as-is | Not needed on panel |

### Pot Decisions

**Mic/line gain (05C50K → dual-gang C50K):** Exact match for resistance and taper. No concerns.

**Instrument gain (W50K → single-gang A50K):** Same resistance. Audio taper substituted for W taper. The feel across the rotation will differ slightly (W taper is logarithmic first half, reverse-log second half; audio taper is purely logarithmic). For a set-and-forget gain control, this is acceptable.

**Output volume (A20K → dual-gang A10K): PROVISIONAL — must be validated in-circuit before final installation.**

The replacement pot has half the resistance of the original (10kΩ instead of 20kΩ). This matters because of a capacitor that's almost certainly sitting in the signal path just before the pot. Here's why:

Audio circuits use a small capacitor before the volume pot to block any DC voltage from reaching the output. That cap and the pot together form a simple filter that rolls off very low bass frequencies. The frequency where this rolloff starts depends on both the cap value and the pot value: **f = 1 / (2π × R × C)**. Halving the pot resistance (R) doubles that frequency.

In practice, Behringer almost certainly used a large, cheap electrolytic capacitor here (10µF or bigger), which would put the rolloff frequency well below the range of human hearing regardless of which pot is used. But because this is a drone instrument where sub-bass is essential, it's worth checking rather than assuming. Step 1.2a walks through finding and reading that capacitor value, after which you can confirm the A10K is safe or decide you need to source an A20K.

Two secondary concerns are evaluated by ear during testing (Step 2.5): whether the output stage is happy driving the lower impedance load, and whether the usable range of the knob feels compressed (volume going from silent to full in a narrow portion of the rotation). These are unlikely to be problems but are easy to check.

### Dual-Gang vs Single-Gang

A dual-gang pot has two independent resistive tracks sharing one shaft. The UM2 uses dual-gang pots for the mic/line gain (balanced input: hot and cold paths) and the output volume (stereo: left and right channels). The instrument gain pot is single-gang (mono input).

When wiring dual-gang replacement pots, there are 6 electrical pins in two rows of 3. Each row is one gang. **Do not cross gangs** — gang A pads on the PCB must connect to gang A pins on the replacement, and gang B to gang B. Crossed gangs will cause incorrect gain behaviour and potentially noise.

The Alpha dual-gang pot pin layout (viewed from the front, shaft towards you): gang A is the row closest to the shaft, gang B is the row furthest. Within each row, pin 1 (CCW end) is on the left, pin 2 (wiper) in the centre, pin 3 (CW end) on the right. **Verify this against the Tayda/Alpha datasheet before wiring.**

---

## Phase 0: Pre-Teardown Verification

Before removing any components, verify every function of the UM2. This establishes a baseline.

**Equipment needed for Phase 0:** You will need RCA cables connected to powered monitors, an amplifier, or another known-good line-level monitoring setup. Test 0.3 specifically requires listening to the RCA outputs while plugging in headphones — this test cannot be deferred, as it determines whether removing the headphone jack will break the main output path.

### Test 0.1 — USB Recognition

Connect the UM2 to a computer via USB. Verify it is recognised as a USB audio device. Note the device name.

### Test 0.2 — Audio Output

Play audio from the computer. Verify sound from both RCA outputs. Verify the output volume pot controls the level. Verify the headphone output works.

### Test 0.3 — Headphone/Output Interaction (CRITICAL)

**This test determines whether the headphone jack contains switching contacts in the main output path.**

1. With audio playing through the RCA outputs, plug headphones into the headphone jack.
2. Monitor the RCA outputs using powered speakers, an amplifier, or a known-good line-level monitoring setup.
3. **Do the RCA outputs mute when headphones are plugged in?**
   - If YES: the headphone jack or its surrounding circuit includes switching that affects the main output path. Removing this jack will break the main output path. You MUST bridge these contacts after removal (see Step 2.3).
   - If NO: the headphone output is a parallel tap and removing it should not affect the main outputs.

**Record the result. This is a critical finding that affects the entire wiring plan.**

### Test 0.4 — XLR Input

Connect a microphone to the XLR input. Set gain to ~50%. Verify signal reaches the computer. Verify clip LED (red) and signal LED (green) respond.

### Test 0.5 — Phantom Power

With a condenser microphone on the XLR input: engage phantom power switch, verify phantom LED illuminates, verify condenser mic produces signal. Disengage phantom power, verify condenser mic goes silent.

### Test 0.6 — Instrument Input

Connect a line source to the 1/4" instrument input. Verify signal reaches the computer. Verify gain pot and LEDs respond.

### Test 0.7 — Direct Monitor

Engage the direct monitor switch. Verify input signal is heard directly in the outputs. Disengage. Set to "off" — this is its permanent position.

**If any test fails, do not proceed. The board has a pre-existing fault.**

---

## Phase 1: Pad Mapping

Map every solder point you will disconnect. Use a multimeter in resistance/continuity mode. This multimeter does not beep — watch for low, stable readings (<1Ω) indicating direct connections, vs fluctuating readings indicating paths through circuit components.

**Take annotated photos of every finding.** These are your primary reference during wiring.

Phase 1 maps all components before any desoldering begins. You need a complete picture of the board's wiring before you start modifying it — decisions in one area (e.g., combo jack switching contacts) can affect decisions elsewhere.

### 1.0 — PCB Mounting Hole Ground Check

The UM2 board will be mounted inside the metal enclosure using standoffs through its existing mounting holes. If those mounting holes are connected to the board's ground network and the standoffs are metal, this creates an unintended direct ground path from the PCB to the enclosure chassis — exactly the ground loop the grounding strategy is designed to prevent.

1. Set your multimeter to continuity mode.
2. Touch one probe to the copper ring around one of the board's mounting holes (on the solder side).
3. Touch the other probe to a known ground point on the board (e.g., the sleeve pad of one of the RCA jacks, or USB pin 4).
4. If continuity exists: the mounting holes are grounded. Use **nylon standoffs** (or metal standoffs with nylon washers between the standoff and the board) to isolate the PCB from the chassis.
5. If no continuity: the mounting holes are isolated. Metal standoffs are fine.

**Record the result.** This determines your standoff hardware choice.

### 1.1 — Phantom Power Switch (COMPLETE)

Confirmed DPDT. Pin layout viewed from solder side:

```
1  2  3
4  5  6
```

Plus 2 structural pins (continuity to housing).

- OFF: 1—2, 4—5
- ON: 2—3, 5—6
- Commons: 2, 5

### 1.2 — Pots (Dual-Gang: VR1 and VR3)

8 solder points each: 6 electrical (two gangs of 3), 2 structural.

1. Identify structural pins (continuity to pot body).
2. Within each group of 3: turn pot fully CCW, find the pair with near-zero resistance (pin 1 and wiper/pin 2). Remaining pin is pin 3.
3. **Verify:** turn fully CW and confirm the wiper (pin 2) now shows near-zero resistance to pin 3 instead.
4. Measure total resistance across pins 1–3 of each gang to confirm value matches the label.
5. Identify which group of 3 is gang A and which is gang B. Note their physical positions relative to the board edge/shaft.

**Note on C-taper pot (VR1):** On a reverse-log pot, the relationship between physical rotation and resistance is inverted compared to a standard audio pot. The pin identification procedure still works (you're finding the wiper mechanically), but be aware that "fully CCW" puts the wiper at a different resistance ratio than on an audio pot. The important thing is correctly identifying which pin is the wiper — the taper only affects how resistance changes across the travel. To be certain: after identifying the wiper at fully CCW, verify by turning fully CW and confirming the wiper now shows near-zero resistance to the other end pin. If both checks are consistent, your pin labelling is correct regardless of taper type.

**Record:** Annotated photo for each pot showing structural pads, gang A (pins 1, 2, 3), gang B (pins 1, 2, 3).

### 1.2a — Output Volume DC-Blocking Capacitor Inspection

This step determines whether the A10K pot substitution is safe. See the Pot Decisions section for a plain-language explanation of why this matters.

1. On the component side of the board, trace the signal path into VR3. Look for a capacitor in series immediately before the pot's input pin (pin 1 or pin 3 of each gang). This will be an electrolytic (cylindrical, with µF marking) or a film/ceramic capacitor.
2. Read the capacitor value.
3. Calculate the high-pass cutoff frequency for both the original and replacement pot:
   - f = 1 / (2π × R × C)
   - Example: 10µF + 20K → 0.8Hz; with 10K → 1.6Hz (no problem — both well below hearing)
   - Example: 4.7µF + 20K → 1.7Hz; with 10K → 3.4Hz (no problem)
   - Example: 1µF + 20K → 8Hz; with 10K → 16Hz (marginal for a drone instrument — you'd lose some sub-bass energy)
   - Example: 0.47µF + 20K → 17Hz; with 10K → 34Hz (problematic — audible bass loss)
4. **Decision gate:**
   - If the capacitor is ≥4.7µF: the A10K is safe. Proceed.
   - If the capacitor is 1–4.7µF: the A10K is marginal. Proceed but listen carefully during testing.
   - If the capacitor is <1µF: source a dual-gang A20K or A25K pot before proceeding. Do not wire the A10K.

**Record the capacitor value and your calculated cutoff frequencies.**

### 1.3 — Pot (Single-Gang: VR2)

5 solder points: 3 electrical, 2 structural.

1. Identify structural pins.
2. Turn fully CCW, find near-zero pair (pin 1 + wiper). Remaining is pin 3.
3. Verify by turning fully CW.

**Record:** Annotated photo.

### 1.4 — RCA Outputs

Each has 2 solder points.

1. Touch probe to outer barrel of RCA jack (ground). Find which solder point has continuity to barrel (= ground pad). Other pad is signal.
2. Repeat for second jack.

**Record:** Label pads S (signal) and G (ground).

### 1.5 — Combo XLR/TRS Jack — Signal Pins

11 solder points total. This is the most complex and highest-risk connector.

**Step 1 — Identify structural pads:** Check continuity from each pad to the jack's metal housing. Structural pads will show near-zero resistance.

**Step 2 — Map XLR pins:** Insert a male XLR plug (or use a paperclip in each pin hole). Check continuity from XLR pin 1, 2, and 3 to the solder points. XLR pin numbers 2 and 3 are embossed on the ZWEE jack face.

**Step 3 — Map TRS contacts:** Insert a 1/4" TRS plug. Check continuity from tip, ring, and sleeve to solder points.

**Step 4 — Identify shared pads:** Note any solder points that serve both XLR and TRS functions (e.g., XLR pin 1 and TRS sleeve may share a pad).

**Record:** Create a complete table: pad position → XLR function / TRS function / structural.

### 1.6 — Combo XLR/TRS Jack — Switching Contacts (CRITICAL)

**This step determines whether the jack has internal switches that change the circuit when a 1/4" plug is inserted or removed.**

1. With NO plug inserted, measure continuity between ALL pairs of the 11 pads. Record every pair that shows continuity.
2. Insert a 1/4" TRS plug. Repeat ALL pair measurements. Record results.
3. Compare the two sets of results. **Any pair whose continuity changed (gained or lost connection) between states contains a switching contact.**
4. If switching contacts exist: determine what they do. Common behaviours:
   - A contact that opens on TRS insertion may disconnect the XLR preamp path
   - A contact that closes on TRS insertion may route signal through a pad/attenuator
   - A contact may change the input impedance

**Mapping to the replacement jack:** The Neutrik NCJ6FI-S has a TRS normalling (TN) contact. Consult the NCJ6FI-S datasheet for its pin layout and switching behaviour. Create an explicit mapping table:

| UM2 PCB Pad | Original Function | NCJ6FI-S Pin |
|-------------|-------------------|--------------|
| (position) | XLR Pin 1 | (pin number) |
| (position) | XLR Pin 2 (hot) | (pin number) |
| (position) | XLR Pin 3 (cold) | (pin number) |
| (position) | TRS Tip | (pin number) |
| (position) | TRS Ring | (pin number) |
| (position) | TRS Sleeve | (pin number) |
| (position) | Switching contact | TN or (solution) |
| (position) | Shell/ground | (pin number) |

**If the original jack has switching behaviour that the NCJ6FI-S cannot replicate:** options include hard-wiring the circuit to one input mode, or adding a manual toggle switch. Determine this before proceeding to desoldering.

**Ground path warning:** XLR pin 1, connector shell, TRS sleeve, and board ground are NOT necessarily interchangeable. Verify the original grounding scheme (which of these are connected together on the PCB) and preserve it intentionally in your wiring. If phantom power is routed incorrectly to TRS contacts or the shell due to a ground path error, it could send 48V DC into equipment connected to the 1/4" input.

### Part Selection Warning

The Neutrik **NCJ6FI-S** (currently purchased) is a combo jack **without switching contacts**. It physically lacks the pins required to replicate normalling behaviour.

If Phase 1.6 reveals that the UM2's circuit relies on switching contacts (contacts that change state when a 1/4" plug is inserted or removed), the NCJ6FI-S **cannot replicate this behaviour**. In that case, you will need to purchase the **Neutrik NCJ9FI-S** instead, which includes 3 dedicated switching contacts.

Check CPC availability for the NCJ9FI-S before beginning desoldering, so that if switching is detected, you can order the replacement without delaying the project.

Before starting Phase 1, pull the actual datasheets for both the NCJ6FI-S and NCJ9FI-S and confirm the switching contact situation directly. This claim is load-bearing — the entire combo jack wiring plan depends on it.

If the original jack has NO switching behaviour (all contacts remain static regardless of plug insertion), the NCJ6FI-S is correct as purchased.

### 1.7 — Instrument Input Jack

5 solder points.

1. Identify structural pads (continuity to housing).
2. Insert a 1/4" TS plug. Check continuity from tip and sleeve to pads.
3. **Check for switch contact:** Measure continuity between all pads with NO plug inserted, then WITH a plug inserted. If any connection changes, there is a switching contact.
4. Common behaviour: a normally-closed switch that shorts the tip (input) to sleeve (ground) when no plug is inserted, preventing a floating high-impedance input from picking up noise.

**If a switch contact is found:** The Neutrik NJ3FP6C-BAG has a tip-shunt (switching) contact. Wire the PCB's switch pad to the NJ3FP6C-BAG's switch contact so the grounding behaviour is preserved.

**Record:** Label pads as T (tip), S (sleeve), SW (switch), structural.

### 1.8 — Headphone Output Jack (CRITICAL)

5 solder points.

**Step 1 — Map signal pins:** Insert a 1/4" TRS plug. Check continuity from tip (left), ring (right), sleeve (ground) to pads.

**Step 2 — Check for switching contacts:** Measure continuity between ALL pads with no plug, then with a plug. Note any changes.

**Step 3 — Determine if switching contacts are in the main output path:** This relates to Test 0.3. If the RCA outputs muted when headphones were plugged in, the headphone jack has normally-closed contacts routing audio to the RCA outputs. These contacts are closed (connected) when no plug is present, and open when headphones are inserted.

**If switching contacts exist in the main output path:** After removing the headphone jack, you MUST bridge the normally-closed contact pads with a short wire jumper soldered across them on the PCB. This permanently simulates "no headphone plug inserted" — keeping the main output path intact.

**If no switching contacts affect the main outputs:** The pads can be left unwired after jack removal.

**Step 4 — Note for later:** The headphone amp circuit remains powered even after jack removal. During Phase 3 testing, listen specifically for noise, hum, or oscillation that could be attributed to the unloaded headphone amp. If instability or noise appears to originate from the unloaded headphone output stage, investigate the original jack topology and output network before adding any dummy load. A temporary resistive load can be used as a diagnostic tool, but should not be made permanent without confirming the appropriate value for the specific circuit. The headphone driver's design determines whether a load is needed and what impedance is safe.

**Record:** Label all pads. Note switching behaviour and the bridge decision.

### 1.9 — LEDs

Each LED housing contains two LEDs (clip/red and signal/green).

**Step 1 — Check for shared pins:** Measure continuity between all LED pads within one housing. If any two pads are connected, the package uses a common anode or common cathode arrangement (3 functional pads for 2 LEDs, not 4).

**Step 2 — Identify polarity:** Use multimeter in diode mode. Red probe on suspected anode, black on suspected cathode. A correct reading shows a voltage drop (1.5–2.5V for red, 1.8–3.5V for green) and may dimly illuminate the LED. Reversed polarity shows OL/no reading.

**Step 3 — Measure forward voltage:** Record the exact forward voltage reading for each LED. This is critical for selecting compatible replacements.

**Step 4 — Record LED colours and positions:** Which pad pair drives the red (clip) LED and which drives the green (signal) LED for each input channel.

**LED replacement note:** Do NOT assume generic 5mm LEDs are electrically compatible with the originals. The original LEDs may be designed for lower current and different forward-voltage characteristics than generic 5mm replacements. Generic 5mm LEDs vary widely in forward voltage and brightness efficiency. Some may be dim at the current available from the UM2's existing resistor network, while others may be excessively bright or draw more current than intended. Do not assume the original and replacement LEDs share the same forward voltage or brightness curve just because both are through-hole packages.

If the measured forward voltages of the originals are similar to standard 5mm LEDs (~1.8V red, ~2.0V green), standard 5mm parts will work but may be dim (less current than rated). Use **high-efficiency or ultra-bright** 5mm LEDs, which reach full perceived brightness at 2–5mA.

If the forward voltages differ significantly (especially if the original green is 3.0–3.5V indicating InGaN type), using a standard 2.0V green LED will draw more current through the board's resistors than designed. Either match the Vf range or calculate and add an inline series resistor.

### 1.10 — USB-B Socket

6 solder points.

1. Identify structural/shield pads (continuity to socket housing).
2. Remaining 4 pads are USB pins. Standard USB-B pinout (viewed from behind the board):
   - Pin 1: +5V (power)
   - Pin 2: D− (data)
   - Pin 3: D+ (data)
   - Pin 4: Ground
3. Pin 4 (ground) likely shows continuity to the housing.

**Do not rely on assumed orientation alone.** "Viewed from behind the board" can be ambiguous depending on how you're holding the board. After labelling the pads by position, confirm +5V and ground by continuity to the board's power and ground network: the +5V pad should show continuity to other +5V points on the board, and the ground pad should show continuity to known ground points (e.g., RCA jack barrels, USB housing). This prevents wiring errors from a mirrored assumption.

**Record:** Annotate photo with pin numbers. Getting D+ and D− swapped means the device won't enumerate.

### 1.11 — Compile Wiring Reference Sheet

Before proceeding to Phase 2, compile:

- Annotated photo for every component showing every pad's function
- Mapping table for each component: "PCB pad → replacement component pin"
- Switching contact decisions (what to bridge, what to wire)
- Standoff hardware decision (nylon vs metal, per Step 1.0)
- DC-blocking capacitor value and A10K pot decision (per Step 1.2a)
- Wire colour assignments:
  - Red = audio signal / hot
  - Black = ground
  - White = cold (for balanced connections)
  - Blue = left channel
  - Green = right channel
  - Yellow = switched/control lines
  - Orange = power (+5V, +48V)

---

## Phase 2: Incremental Desoldering & Rewiring

This phase combines what were previously separate desoldering and wiring phases. Each subsystem is removed, its replacement is wired, and the result is tested before moving on. The USB-B socket stays in place throughout, providing a test connection to a computer (or the Pi).

### General Desoldering Technique

**Tools:** Temperature-adjustable soldering iron (350–400°C), desoldering braid/wick (Chemtronics 80-3-5), solder sucker (spring-loaded desoldering pump), flux, helping hands or PCB holder, isopropyl alcohol and small brush, magnifying glass or phone camera zoom, eye protection.

1. Apply flux to joints on the back of the board.
2. Heat the joint (2–3 seconds for full melt).
3. Remove solder with braid or solder sucker, one pin at a time.
4. When all pins seem clear, gently wiggle the component. If it doesn't move freely, find and clear the remaining connection.
5. Never force anything.

**For large metal connectors (combo jack, RCA jacks):** These sink heat. Use higher temperature (up to 400°C), longer dwell time, and add fresh leaded solder to stubborn joints to lower melting point.

**For pot structural tabs:** Use solder sucker rather than braid — the wide tabs hold more solder than braid can absorb.

**After each removal:** Inspect pads with magnification. Clean flux residue with IPA. Photograph both sides of the board.

### General Wiring Principles

- **Stranded hookup wire** for all connections (not solid core).
- **Keep wires short.** Measure the distance from PCB to panel component, add 2–3cm slack.
- **Consistent wire colours** per scheme from Phase 1.11.
- **Tin wire ends and PCB pads** separately before joining.
- **Solder to PCB pads from the back** of the board.
- **Strain relief on all wired connections.** After wiring, secure cables to the PCB or enclosure with cable ties or hot glue to prevent mechanical stress on solder joints during transport and performance.

### Wiring by Signal Type

**For the XLR mic input path (combo jack XLR pins 2 and 3):** Use **twisted pair** wire. Keep the hot (pin 2) and cold (pin 3) wires twisted together for the entire run between PCB and panel jack. This ensures both wires pick up the same interference, maximising the balanced input's common-mode rejection. If available, use shielded twisted pair — connect the shield to ground at the PCB end only. This is the most sensitive analogue path in the system (mic-level signals can be as low as 1mV) and it shares an enclosure with a Pi 5, switchmode PSU, touchscreen, and digital control wiring.

**For all other audio connections (TRS portion of combo jack, instrument input, L/R outputs, LED wires, pot wires, switch wires):** Standard stranded hookup wire is acceptable for short runs.

**Route analogue and digital wiring separately** wherever the enclosure layout permits. Keep mic input wires and phantom power lines away from the Pi, display, USB cables, and encoder wiring.

In particular, do not run the internal USB cable tightly bundled alongside the XLR mic-input twisted pair. USB carries high-frequency digital traffic that can couple into adjacent mic-level analogue wiring.

### Incremental Order

The subsystems are ordered from lowest-risk to highest-risk. Outputs are done early so you can hear test results for everything that follows. The combo XLR/TRS jack — the hardest and riskiest removal — is done second-to-last, when everything else is already proven working. The USB socket is done last.

---

### Step 2.1 — LEDs (Warm-Up)

**Desolder:** Remove Input 2 LEDs (DS2/DS3), then Input 1 LEDs (DS1). Inspect and clean pads after each removal.

**Quick check:** Connect UM2 via USB. Verify USB recognition. Send signal to both inputs — LEDs won't illuminate (expected), but verify both inputs still work. This confirms no nearby traces were damaged.

**Wire:** For each LED (4 total: 2 per input channel):
1. Solder wires to anode (+) and cathode (−) pads on PCB (per Phase 1.9).
2. Run wires to 5mm LED in panel-mount bezel on faceplate.
3. Connect with correct polarity.

If LED pads share a common pin (3 pads for 2 LEDs), wire accordingly — the common wire goes to one panel LED's anode/cathode AND the other panel LED's anode/cathode.

**Test 2.1:** Send signal to each input:
- Input 1 green LED lights with normal signal
- Input 1 red LED lights with loud/clipping signal
- Input 2 green LED lights with normal signal
- Input 2 red LED lights with loud/clipping signal

If LEDs are too dim: replace with high-efficiency/ultra-bright 5mm LEDs that reach full brightness at low current (preferred and simplest fix). If brightness is still insufficient, you would need to reduce the total series resistance in the LED circuit (replace the board's existing current-limiting resistor with a lower value). Calculate the new value to keep current within the replacement LED's absolute maximum rating: R = (Vcc − Vf) / I_desired. Do not reduce resistance without calculating first.

If LEDs are too bright or drawing more current than intended, add or increase series resistance.

---

### Step 2.2 — RCA Outputs → 1/4" Output Jacks

Wiring the outputs early means you can listen to test results for every subsequent step.

**Desolder:** Remove both RCA output jacks (4 pins total). Inspect and clean pads.

**Wire:** For each RCA pad pair → Neutrik NJ3FP6C-BAG:
- Signal pad → jack tip contact (consult NJ3FP6C-BAG datasheet for lug identification)
- Ground pad → jack sleeve contact

**Test 2.2:** Play audio through the UM2 via USB. Verify sound from both output jacks. Verify stereo separation (left only from left, right only from right).

---

### Step 2.3 — Headphone Output Jack (Remove Only)

**Desolder:** Remove the headphone output jack (5 pins). Inspect and clean pads.

**If switching contacts were identified in Phase 1.8 that route audio to the main outputs:** Bridge the normally-closed contact pads NOW. Solder a short wire jumper across the identified pads on the PCB. This permanently simulates "no headphone plug inserted" — keeping the main output path intact.

**Test 2.3:** Play audio from the computer via USB. Verify audio still appears at both 1/4" output jacks (wired in Step 2.2). If the RCA outputs previously muted when headphones were plugged in, this test confirms your switching contact bridge is working. If audio is missing from one or both outputs, the bridge is incorrect or incomplete.

---

### Step 2.4 — Instrument Input + Instrument Gain Pot

These two components form a single signal path and are done together so you can test the complete instrument input chain.

**Desolder instrument input jack:** 5 pins. Inspect and clean pads.

**Wire instrument input:** Pads → Neutrik NJ3FP6C-BAG:
- Tip pad → jack tip contact
- Sleeve pad → jack sleeve contact
- Switch pad → jack switch (tip-shunt) contact (if switch contact was identified in Phase 1.7)

If the original jack had a normally-closed switch grounding the input when no plug is present, preserving this behaviour prevents a floating high-impedance input from picking up hum and noise when nothing is plugged in.

**Note on TS vs TRS:** The NJ3FP6C-BAG is a 3-pole (TRS) jack being used for a mono (TS) instrument input. The ring contact on the replacement jack will be unused. Verify against the NJ3FP6C-BAG datasheet that the unused ring contact does not need to be grounded or left open according to the jack's internal topology. In most cases, leaving it unwired is correct for a TS application, but confirm this.

**Desolder instrument gain pot (VR2):** 5 pins. Inspect and clean pads.

**Wire instrument gain pot:** 3 electrical connections per Phase 1.3 mapping.

**Test 2.4a:** Plug an instrument or line source into the new 1/4" input jack. Verify signal reaches the computer via USB.

**Test 2.4b:** Verify smooth gain control across full rotation. No crackling, dropouts, or sudden jumps.

**Test 2.4c:** Unplug the instrument. Verify no excessive noise from the now-empty input (if switch contact is wired correctly, input should be grounded and quiet).

---

### Step 2.5 — Output Volume Pot (Dual-Gang A10K — PROVISIONAL)

**Desolder output volume pot (VR3):** 8 pins. Inspect and clean pads.

**Wire:** 6 electrical connections. Match gangs per Phase 1.2 mapping.

**Test 2.5 — Extended validation (because this is a substituted value):**
1. Verify smooth volume control.
2. Verify stereo balance (both channels attenuate equally).
3. **Listen critically to bass response.** Play content with strong sub-bass through the instrument. Compare to your memory of the original UM2's output (or any other reference). If the low end sounds thin or rolled off, the A10K pot's interaction with the DC-blocking capacitor is shifting the high-pass cutoff too high.
4. **Check usable rotation range.** Does volume go from silent to loud in a reasonable portion of the knob travel, or is it all compressed into the first 30%?
5. If bass is thin or the taper is badly compressed: source a dual-gang A20K or A25K pot and replace.

---

### Step 2.6 — Mic/Line Path: Phantom Power Switch + Gain Pot + Combo Jack

This is the most complex and highest-risk group. The combo XLR/TRS jack is the hardest component to desolder (11 pins, lots of heat sinking, surrounded by traces), and the phantom power circuit carries 48V DC that can damage equipment if miswired. These three components are grouped because they all serve the mic/line input and need each other for meaningful testing.

The order within this group is: phantom power switch first (smaller, lower risk, good warm-up), then the mic/line gain pot, then the combo jack last.

#### Step 2.6a — Phantom Power Switch

**Desolder phantom power switch (S1):** 8 pins. Inspect and clean pads.

**Wire:** 6 functional pads → Multicomp 1MD1T1B1M1QE toggle switch lugs:
- Pad 1 → lug 1
- Pad 2 → lug 2 (common)
- Pad 3 → lug 3
- Pad 4 → lug 4
- Pad 5 → lug 5 (common)
- Pad 6 → lug 6

**Verify lug positions on the 1MD1T1B1M1QE against its datasheet.** The physical arrangement may differ from the original S1 switch. The mapping must preserve the same switching logic: in one position, commons connect to one side (phantom off); in the other position, commons connect to the other side (phantom on).

**Toggle orientation:** Before final panel mounting, verify which physical toggle position activates phantom power (measure 48V at the combo jack in each toggle position). The mechanical throw of a toggle switch is typically inverted relative to its internal contacts — when the bat is flipped "up," the common lugs connect to the bottom row of pins. Label or orient the toggle on the backplate so that the "up" or marked position corresponds to phantom ON, matching user expectations and any panel markings.

**Test 2.6a (limited — full phantom testing requires the combo jack):** Connect USB. Toggle the switch. Verify that the phantom power LED on the board illuminates in one position and goes dark in the other. This confirms the switch is wired correctly to the phantom power circuit. Full voltage verification happens after the combo jack is wired.

#### Step 2.6b — Mic/Line Gain Pot

**Desolder mic/line gain pot (VR1):** 8 pins. Inspect and clean pads.

**Wire:** 6 electrical connections (gang A: pins 1, 2, 3; gang B: pins 1, 2, 3). Match gangs exactly per Phase 1.2 mapping.

**Test 2.6b (limited — full testing requires the combo jack):** Connect USB. Verify USB recognition. The gain pot controls the mic/line preamp, which you can't test without the combo jack connected. This step just confirms the board survived desoldering.

#### Step 2.6c — Combo XLR/TRS Jack (CRITICAL — Highest Risk)

**This is the hardest desoldering job. Budget 15–20 minutes. Take a photo before starting.**

**Desolder combo XLR/TRS jack:** 11 pins. After removal: inspect every pad with magnification. This connector is surrounded by traces and components — check for collateral damage. Clean thoroughly.

**Wire:** All connections per your mapping table from Phases 1.5 and 1.6.

**Critical connections:**
- XLR Pin 1 → NCJ6FI-S (or NCJ9FI-S) corresponding pin
- XLR Pin 2 (hot) → corresponding pin **(use twisted pair with pin 3 wire)**
- XLR Pin 3 (cold) → corresponding pin **(use twisted pair with pin 2 wire)**
- TRS Tip → corresponding tip contact
- TRS Ring → corresponding ring contact
- TRS Sleeve → corresponding sleeve contact
- Switching contact(s) → TN pin or switching contacts (if NCJ9FI-S — or other solution per Phase 1.6)
- Shell/ground → ground contact (preserve original grounding scheme)

Structural pads left unwired.

**Pre-power verification (MANDATORY):** Before connecting USB, use your multimeter to verify:
1. Continuity from XLR pin 1/2/3 pads on PCB to the correct XLR contacts on the panel jack.
2. Continuity from TRS tip/ring/sleeve pads on PCB to the correct TRS contacts on the panel jack.
3. No unintended shorts between XLR and TRS paths that shouldn't be connected.
4. Shell/ground path matches the original scheme.

**Test 2.6c-1 — Phantom safety (MANDATORY, before connecting any microphone):**
1. Connect USB. Engage phantom power via the toggle switch (wired in Step 2.6a).
2. **With nothing plugged into the combo jack,** measure DC voltage:
   - Between XLR pin 2 contact and XLR pin 1 contact: should read ~48V DC
   - Between XLR pin 3 contact and XLR pin 1 contact: should read ~48V DC
   - Between XLR pin 2 and XLR pin 3: should read ~0V DC (phantom is balanced)
   - Between any TRS contact and ground: should read ~0V DC (phantom must NOT appear on TRS)
3. If phantom appears on TRS contacts, STOP. There is a wiring error that could damage equipment.
4. Disengage phantom power. Verify all voltages return to 0V.

**If only one pin shows ~48V relative to pin 1:** One pole of the DPDT switch is miswired. Check your pad-to-lug mapping for the affected pole (pads 1-2-3 or pads 4-5-6).

**Test 2.6c-2 (XLR functional):** Plug a dynamic microphone into XLR. Verify signal reaches the computer. Verify the mic/line gain pot (wired in Step 2.6b) provides smooth gain control.

**Test 2.6c-3 (TRS functional):** Plug a line source into 1/4" TRS. Verify signal reaches the computer. If no signal, check switching contact wiring.

**Test 2.6c-4 (Phantom with condenser):** Engage phantom power. Plug in a condenser mic. Verify it works. Disengage — verify it stops.

**Test 2.6c-5 (Dynamic mic safety):** With phantom ON, plug in a dynamic mic. Verify the dynamic mic works normally. This provides an additional functional sanity check, but the true confirmation of balanced phantom is the voltage measurement in Test 2.6c-1 above.

---

### Step 2.7 — USB-B Socket → Internal USB Cable (LAST)

Everything is now wired and tested except the USB connection itself. This is the final step.

**Desolder USB-B socket:** 6 pins. Inspect and clean pads.

**Wire internal USB cable:** Cut a USB cable. Strip and identify the 4 signal wires plus the shield/drain wire:
- Red = +5V → PCB pin 1 pad
- White = D− → PCB pin 2 pad
- Green = D+ → PCB pin 3 pad
- Black = GND → PCB pin 4 pad
- Shield/drain wire → clip flush at UM2 end, heat-shrink to insulate (grounded at Pi end via USB-A plug shell)

**Cable shield/drain wire:** USB cables contain a shield conductor (foil or braid with a drain wire) in addition to the 4 signal wires. Standard practice for internal interconnects is to ground the shield at the host end only. The USB-A plug's metal shell is already bonded to the Pi's ground, so the shield is grounded at that end. At the UM2 end, clip the drain wire/foil flush and cover with heat shrink so it doesn't short against anything. Do not connect the shield to the UM2 structural pads — doing so creates a parallel ground path that can induce high-frequency noise.

**Cable type:** You need a USB cable with a USB-A plug on the Pi end and bare wires on the UM2 end. Cut an existing USB-A to USB-B cable, discard the USB-B connector end, and strip the wires. Alternatively, use a USB-A plug/breakout with bare wire tails.

**USB signal integrity:** Keep the stripped/untwisted portion of D+ and D− as short as physically possible (under 5mm). Keep the green and white wires twisted together right up to the solder points. USB 2.0 relies on 90Ω differential impedance; long unshielded stubs cause enumeration failures and audio dropouts.

**Strain relief:** Anchor the USB cable to the PCB or a nearby tie point so that cable movement does not load the D+/D− pads.

The USB-A end connects to one of the Pi's USB ports.

**Test 2.7:** Connect to the Pi (or a computer). Verify the UM2 is recognised as a USB audio device. If not: check for swapped D+/D−, cold joints, or shorts.

If recognised: run a quick audio test through all inputs and outputs to confirm nothing was disturbed during the USB socket removal. This is the final confirmation that the board survived the complete teardown and rewiring process.

---

## Phase 3: Final Integration Test

With all components wired and individually tested, perform a complete system test in the final enclosure context.

1. Connect the UM2 internally to the Raspberry Pi 5.
2. Boot the Pi. Verify the UM2 appears as an ALSA audio device.
3. Run the JUCE DronemakerClone application.
4. Test the complete signal path:
   - XLR input → verify signal
   - Instrument input → verify signal
   - Mic/line gain → smooth control
   - Instrument gain → smooth control
   - Phantom power on/off → correct behaviour
   - Output via both 1/4" jacks → verify sound, stereo balance
   - Output volume → smooth stereo control (re-evaluate bass response and taper range — see Step 2.5)
   - All 4 LEDs → correct response
5. **Sustained test:** Run the full signal chain (input → drone engine → loops → effects → output) for at least 15 minutes. Listen for:
   - Intermittent audio (loose connections)
   - Hum or buzz (ground loop, shielding issue, floating input)
   - Noise increase over time (thermal issue)
   - Any behaviour change when touching the enclosure or cables (grounding problem)
   - Oscillation or strange sounds from the headphone amp area (floating output issue — investigate per Phase 1.8 Step 4)

---

## Phase 4: Independent Headphone Output (Deferred)

Separate sub-system. Not connected to the UM2 board.

### Why This Is Needed

The UM2 has only one stereo output pair. The DronemakerClone needs an independent headphone output for monitoring loops, sub-mixes, or click tracks during performance — audio that shouldn't go to the main outputs. The UM2 can't provide this because it has a single stereo bus; its headphone output is just a copy of the main output.

### Signal Path

Pi 5 → I2S DAC HAT (InnoMaker PCM5122) → pre-built headphone amp PCB → Alpha 9mm dual-gang A10K volume pot → Neutrik NJ3FP6C-BAG panel-mount 1/4" TRS jack

### Headphone Amp Approach

Rather than designing a headphone amp circuit from scratch, this build will use a pre-built headphone amp PCB. There are many inexpensive options available (a few pounds) that accept line-level input and provide sufficient drive for headphones. Select one based on supply voltage compatibility (the Pi 5's 5V rail or the system's existing power supply), output impedance (lower is better for modern headphones — aim for <10Ω), and physical size (must fit inside the Hammond enclosure alongside everything else).

### Software Routing

The JUCE DronemakerClone application manages the UM2 directly as its audio device. The headphone DAC is a second audio device on the Pi. The application will need to be updated to open both devices — JUCE supports this, and the routing logic (which audio buses go to which output) is a software concern that doesn't affect the hardware build. Alternatively, ALSA or JACK can be used as an intermediary to route audio between devices, though this adds a layer of complexity and potentially some latency.

### Status

Dual-gang A10K pot and Neutrik jack purchased. DAC HAT purchased. Headphone amp PCB not yet sourced.

### Open Questions

- Headphone amp PCB selection (supply voltage, output impedance, form factor)
- JUCE dual-device implementation or ALSA/JACK routing configuration
- DAC HAT vs Pi onboard audio quality comparison
- Physical mounting of DAC HAT and headphone amp PCB inside the enclosure

---

## Grounding Strategy

The Hammond 1456KK4BKBK is a metal (aluminium) enclosure. Without deliberate planning, multiple ground paths can form between the UM2's audio ground, the Pi's ground, the enclosure chassis, and external equipment — creating ground loops that manifest as 50Hz mains hum.

### Key Facts About the Components

- The **Neutrik NJ3FP6C-BAG** jacks come with their sleeve contacts **isolated from the panel** by default (a red insulator on the lateral screw). The sleeve/shell does NOT bond to the chassis unless you deliberately remove this insulator.
- The **Neutrik NCJ6FI-S** (or NCJ9FI-S) combo jack also has a separate ground contact and does not automatically bridge to the panel.
- The **UM2 PCB mounting holes** may or may not be connected to the board's ground network (checked in Phase 1 Step 1.0).

### Recommended Strategy: Isolated Connectors with Star Ground

1. **Leave the red isolation washers installed** on all NJ3FP6C-BAG jacks. This keeps audio ground isolated from the enclosure chassis.
2. **Use nylon standoffs** (or nylon washers) for PCB mounting if the mounting holes are grounded (per Step 1.0). This prevents the PCB ground from bonding to the chassis through the standoffs.
3. **Run a dedicated ground wire** from each panel-mount jack's ground lug to the UM2 PCB's ground network (a single known ground point on the board). This ensures all audio grounds return to the same point without passing through the chassis.
4. **Create one intentional chassis bond:** Connect the enclosure chassis to the audio ground at ONE point only. This can be done by running a wire from the PCB's ground to one of the enclosure's standoff mounting points, or to a dedicated chassis lug.
5. **Route analogue and digital ground returns separately** where practical. The Pi's USB ground connection to the UM2 carries both audio ground and digital ground — this is unavoidable, but keeping other ground paths clean reduces the chance of noise.

### During Phase 3 Testing

Listen for 50Hz hum. If present:
- Verify no accidental chassis bonds exist (check standoffs, check that red washers are in place on all jacks).
- Try removing the single intentional chassis bond to see if hum reduces.
- Consider a ground-lift adapter on the Pi's power supply if mains earth is the noise path.
- A USB isolator between Pi and UM2 is a last resort (adds latency).

---

## Backplate Components (Final)

1. Neutrik NCJ6FI-S (or NCJ9FI-S if switching required) — combo XLR/TRS input (D-shaped cutout, 31 × 26mm)
2. Neutrik NJ3FP6C-BAG — instrument input 1/4" (D-shaped cutout)
3. Neutrik NJ3FP6C-BAG — left output 1/4" (D-shaped cutout)
4. Neutrik NJ3FP6C-BAG — right output 1/4" (D-shaped cutout)
5. Neutrik NJ3FP6C-BAG — independent headphone output 1/4" TRS (D-shaped cutout, Phase 4)
6. Multicomp Pro 1MD1T1B1M1QE — phantom power toggle (6.2mm / 1/4" round hole)

## Faceplate Components (from UM2)

1. Alpha 9mm dual-gang C50K — mic/line gain (7mm round hole, round shaft 6.35mm)
2. Alpha 9mm single-gang A50K — instrument gain (7mm round hole, round shaft 6.35mm)
3. Alpha 9mm dual-gang A10K — output volume (7mm round hole, round shaft 6.35mm) — PROVISIONAL
4. Alpha 9mm dual-gang A10K — headphone volume (7mm round hole, round shaft 6.35mm, Phase 4)
5. 2 × 5mm green LEDs in 5mm bezels — signal indicators
6. 2 × 5mm red LEDs in 5mm bezels — clip indicators

**Bezel note:** Must use 5mm bezels. Current stock of 3mm bezels is not compatible with 5mm LEDs.

---

## Risk Register

| Risk | Impact | Mitigation |
|------|--------|------------|
| Lifted pad during desoldering | High — broken trace, hard to repair on single-layer PCB | Go slow. Never force. Use flux. Photo both sides before each removal. If lifted, trace the connection and bridge with wire. |
| Combo XLR/TRS jack switching contacts not mapped | Critical — could permanently short XLR and TRS paths, or route phantom power to TRS contacts | Phase 1.6 mandatory switching contact check with/without plug. Map before desoldering. |
| Combo jack part (NCJ6FI-S) lacks switching contacts | Critical — if original jack has switching behaviour, the NCJ6FI-S cannot replicate it | Check NCJ9FI-S availability before desoldering. See Part Selection Warning in Phase 1.6. |
| Headphone jack has switching contacts in main output path | Critical — removing jack breaks main outputs permanently | Test 0.3 checks this. Phase 1.8 maps contacts. Bridge normally-closed contacts after removal if needed. |
| Phantom power on TRS contacts due to wiring error | Critical — 48V DC could damage connected equipment | Phase 1.6 ground path verification. Test 2.6c-1 mandatory phantom safety check before connecting any equipment. |
| Ground loop hum in metal enclosure | High — audible 50Hz hum in outputs | Grounding strategy section. Isolated connectors with star ground. Leave red washers on Neutrik jacks. Test during Phase 3. |
| PCB mounting standoffs create unintended ground path | High — silently creates the ground loop the grounding strategy prevents | Phase 1 Step 1.0 checks mounting hole ground connectivity. Use nylon standoffs if grounded. |
| Dual-gang pot wired with gangs crossed | Medium — incorrect gain, noise | Label gangs clearly during Phase 1. Verify with multimeter after wiring. |
| A10K pot substitution causes bass loss or taper compression | Medium — thin sound or unusable volume control | Phase 1 Step 1.2a inspects DC-blocking cap to resolve before wiring. Validate during Step 2.5. Source A20K/A25K dual-gang if problematic. |
| LED current mismatch — too dim or overdrive | Medium — invisible indicators or shortened LED life | Measure original LED Vf before desoldering. Select high-efficiency 5mm LEDs. Test brightness. |
| XLR mic-level signals pick up EMI from Pi/digital circuits | Medium — noise in mic input | Use twisted pair for XLR hot/cold. Route analogue away from digital. |
| Instrument input floating when unplugged (switch contact not wired) | Medium — hum/noise from open input | Wire NJ3FP6C-BAG tip-shunt contact to PCB switch pad. |
| USB D+ and D− swapped | Medium — device won't enumerate | Verify pin mapping by continuity to known power/ground points. If device fails, try swapping. |
| USB signal integrity — long unshielded stubs | Medium — dropouts, enumeration failures | Keep stripped wire under 5mm. Keep D+/D− twisted. Connect cable shield. Add strain relief. |
| Phantom power not discharged before rework | Medium — electrical shock risk, component damage | Safety rule: disconnect USB, wait 30s, verify <1V on phantom pads before touching. |
| Pi 5 USB power budget exceeded | Medium — intermittent USB disconnections that mimic bad solder joints | The Pi 5 provides up to 1.6A across all USB ports with the official 5.1V/5A PSU (600mA with lesser supplies). The UM2 draws ~200–300mA, plus the Teensy ~100mA. Verify the Pi uses a supply rated for full USB current. If intermittent USB disconnections occur during Phase 3 sustained testing, check PSU capacity before investigating solder joints. |
| Direct monitor switch bumped during servicing | Low — unexpected routing change | Secured with hot glue. Position documented. |

---

## Component Reference

### Purchased and Available

| Component | Source | Part Number | Qty | Status |
|-----------|--------|-------------|-----|--------|
| Alpha 9mm dual-gang A10K (round shaft 6.35mm) | Tayda | RD902F-40-15R1-A10K | 2 | Arrived |
| Alpha 9mm dual-gang C50K (round shaft 6.35mm) | Tayda | RD902F-40-15R1-C50K | 1 | Arrived |
| Alpha 9mm single-gang A50K (round shaft 6.35mm) | Thonk | RD901F-40-15R1-A50K | 1 | Arrived |
| Neutrik NCJ6FI-S combo XLR/TRS panel jack | CPC | AV11164 | 1 | Arrived (see Part Selection Warning — NCJ9FI-S may be needed) |
| Neutrik NJ3FP6C-BAG 1/4" locking panel jack | CPC | CN02851 | 4 | Arrived |
| Multicomp Pro 1MD1T1B1M1QE DPDT toggle | CPC | SW02853 | 1 | Arrived |
| Chemtronics 80-3-5 desoldering braid | CPC | SD00333 | 1 | In stock |
| 5mm LED kit (red, green, etc.) | Pi Hut | — | 1 kit | In stock |
| Hookup wire (stranded, multiple colours) | — | — | Various | In stock |
| Flux | — | — | — | In stock |
| Behringer UM2 | — | — | 1 | Teardown-ready |
| Hammond 1456KK4BKBK enclosure | Mouser | — | 1 | Waiting on arrival |
| InnoMaker PCM5122 DAC HAT | — | — | 1 | Available (shared with other project) |

### Still Needed

| Component | Notes |
|-----------|-------|
| 5mm LED panel-mount bezels | Current stock is 3mm — need 5mm |
| Pre-built headphone amp PCB | Pending Phase 4 — select for 5V supply compatibility and low output impedance |
| Short USB cable (for internal UM2-to-Pi connection) | Cut and strip an existing USB-A to USB-B cable |
| Twisted pair or shielded wire (~30cm) | For XLR hot/cold mic input path |
| Dual-gang A20K or A25K pot (contingency) | Only if A10K fails validation in Phase 1.2a / Step 2.5 |
| Nylon standoffs or nylon washers (contingency) | Only if PCB mounting holes are grounded (Phase 1 Step 1.0) |
| Neutrik NCJ9FI-S combo jack (contingency) | Only if Phase 1.6 reveals switching contacts that the NCJ6FI-S cannot replicate |

### Superseded (Not Used for This Project)

| Component | Source | Reason |
|-----------|--------|--------|
| Alpha 9mm single-gang C50K (round shaft) | Thonk | Mic/line gain needs dual-gang |
| Alpha 9mm single-gang A25K (D-shaft) | Thonk | Output volume needs dual-gang |
| Alpha 9mm single-gang A10K (D-shaft) | Thonk | Headphone volume needs dual-gang |
| Brass pot adapters (6mm→6.35mm) | Thonk | No longer using D-shaft pots |
| 3mm LED bezels (×50) | Amazon | LEDs are 5mm — wrong size |
