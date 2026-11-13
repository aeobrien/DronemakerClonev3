# Roadmap

## Next Up
<!-- Highest priority incomplete tasks across all phases -->

| Task | Milestone | Phase | Status | Effort |
|------|-----------|-------|--------|--------|
| 1.2.7 Undo U2 pin-lift, test U1 in isolation | 1.2 Firmware Testing | 1: Hardware Build | Todo | Physical |
| 1.2.8 Redo U2 pin-lift with solid cross-wire | 1.2 Firmware Testing | 1: Hardware Build | Todo | Physical |
| 1.2.2 Run 5 test firmwares on assembled boards | 1.2 Firmware Testing | 1: Hardware Build | In Progress | Physical |
| 1.2.4 Complete LED bit mapping and update firmware | 1.2 Firmware Testing | 1: Hardware Build | In Progress | Physical |
| 1.2.5 Fix 4 button looms — diagnose and repair crimping | 1.2 Firmware Testing | 1: Hardware Build | Todo | Physical |
| 2.2.2 Get panels CNC-machined | 2.2 CNC Fabrication | 2: Enclosure | Todo | Physical |

---

## Phase 1: Hardware Build
**Status:** In Progress
**Definition of Done:** All 3 PCBs assembled, tested with firmware, and confirmed working over USB MIDI.
**Deadline:** Mid-April 2026

### 1.1 — PCB Assembly
**Status:** In Progress
**Priority:** High
**Definition of Done:** All PCBs soldered with all components, no shorts, power-on test passed.

| # | Task | Status | Effort | Deadline | Notes |
|---|------|--------|--------|----------|-------|
| 1.1.1 | Solder channel strip PCBs (3x) | Done | Physical | | Completed 2026-04-10 |
| 1.1.2 | Solder master strip PCB | Done | Physical | | Completed 2026-04-10 |
| 1.1.3 | Solder brain board PCB | Done | Physical | | Completed 2026-04-10. Teensy 4.1 + shift registers inserted. |
| 1.1.4 | Measure LED bezels — 6.2mm hole, 7.8mm flange | Done | Physical | | Completed 2026-04-02 |

### 1.2 — Firmware Testing
**Status:** In Progress
**Priority:** High
**Definition of Done:** All 5 test firmwares pass, production MIDI firmware verified sending correct CCs.

| # | Task | Status | Effort | Deadline | Notes |
|---|------|--------|--------|----------|-------|
| 1.2.1 | Write build checklist + 5 test firmwares + production MIDI firmware | Done | Deep Focus | | Written 2026-04-07 |
| 1.2.2 | Run 5 test firmwares on assembled boards | In Progress | Physical | | Tests 1-3 passed. Test 4 (LED): root cause identified as PCB layout bug (SRCLK/RCLK swap on U1/U3), not solder joints. Firmware swap applied. Per-pin diagnostic test (04e) written. Initial results after U2 pin-lift inconclusive — needs rework. |
| 1.2.3 | Verify production MIDI firmware end-to-end with JUCE app | Todo | Deep Focus | | |
| 1.2.4 | Complete LED bit mapping (04c test) and update firmware with correct mapping | In Progress | Physical | | Full green+blue mapping done (J9-J16). Firmware updated: byte order corrected (red first/farthest, blue middle, green last/nearest), bit offset +1, C++ byte inversion bug fixed. Mapping not yet verified due to solder joints. |
| 1.2.5 | Fix 4 button looms — diagnose and repair crimping | Todo | Physical | | Wiring consistent across looms. Likely systematic crimping error. 1 button also has loose blue wire. |
| 1.2.6 | Reflow all shift register socket joints (U1, U2, U3) + nearby resistors | Done | Physical | | Completed 2026-04-11. Reflow done; LED issues traced to PCB layout bug (SRCLK/RCLK swap), not solder joints. |
| 1.2.7 | Undo U2 pin-lift, test U1 outputs in isolation to verify firmware swap | Todo | Physical | | U1 gets data directly from Teensy — if firmware swap is correct, all 8 outputs should read properly without any physical mod. |
| 1.2.8 | Redo U2 pin-lift with solid cross-wire connections (pins 11↔12) | Todo | Physical | | Depends on 1.2.7 confirming firmware swap works on U1. |

### 1.3 — Component Sourcing
**Status:** In Progress
**Priority:** High
**Definition of Done:** All components in hand, ready for assembly.

| # | Task | Status | Effort | Deadline | Notes |
|---|------|--------|--------|----------|-------|
| 1.3.1 | Order resistors + caps | Done | Administrative | | Arriving Apr 8 |
| 1.3.2 | Download all datasheets for design brief | Done | Administrative | | 8 component PDFs + Neutrik D-size cutout template (2026-04-02) |
| 1.3.3 | Verify TUK keystone mounting — snap-in, no screw holes needed | Done | Quick Win | | Confirmed 2026-04-04 |
| 1.3.4 | Resolve combo jack part — NCJ6FI-S vs NCJ9FI-S | Todo | Deep Focus | | Pin compatibility with phantom power |
| 1.3.5 | Verify 5mm replacement LED forward voltage/current vs original UM2 LEDs | Todo | Quick Win | | |

### 1.4 — Audio Interface (UM2 Mod)
**Status:** In Progress
**Priority:** Normal
**Definition of Done:** UM2 fully modified — XLR/TRS jack fitted, LEDs replaced, re-wired with heat shrink.

| # | Task | Status | Effort | Deadline | Notes |
|---|------|--------|--------|----------|-------|
| 1.4.1 | Fit XLR/TRS jack + LEDs | Todo | Physical | | |
| 1.4.2 | Re-wire with heat shrink | Todo | Physical | | |

---

## Phase 2: Enclosure
**Status:** In Progress
**Definition of Done:** Fully fabricated and painted enclosure panels with all cutouts, ready for component mounting.
**Deadline:** Apr 24 (soft — Cornwall retreat)

### 2.1 — Design Finalisation
**Status:** In Progress
**Priority:** High
**Definition of Done:** Final CAD files approved, all cutout dimensions verified, ready for fabrication.

| # | Task | Status | Effort | Deadline | Notes |
|---|------|--------|--------|----------|-------|
| 2.1.1 | Write design brief v2 — trimmed, SVGs as guides | Done | Administrative | | Completed 2026-04-02 |
| 2.1.2 | Resolve screen cutout issues with designer | Done | Communication | | Revision 1 received 2026-04-09. Full dimension comparison: all within spec. Standoff-encoder clearance ~0.5mm. |
| 2.1.3 | Final SVG tweaks in Inkscape — slope, front, back panel layout guides | Done | Creative | | Superseded by designer's CAD files. |
| 2.1.4 | Encoder holes: reduce from 9.6mm to 9.3mm (datasheet spec) + add 8 × 3.3mm locator holes (9mm above each encoder centre) | Done | Communication | | Confirmed at 9.4mm in designer files. 8 locator holes present at 3.3mm/9.0mm. |
| 2.1.4 | Obtain full Hammond 1456KK4BKBK enclosure STEP | Done | Administrative | | Completed 2026-04-03 |
| 2.1.5 | Post design brief on Fiverr — commission professional CAD designer | Done | Communication | | Posted 2026-04-04 |

### 2.2 — CNC Fabrication
**Status:** Todo
**Priority:** High
**Definition of Done:** Panels CNC-machined with all cutouts to spec.

| # | Task | Status | Effort | Deadline | Notes |
|---|------|--------|--------|----------|-------|
| 2.2.1 | Send hackspace email for CNC fabrication advice | Done | Communication | | Sent 2026-04-10. Visited open evening Apr 9. Requested Standard + Plus One membership. |
| 2.2.2 | Get panels CNC-machined | Todo | Physical | | Depends on 2.1 completion. Panel is ONE piece (faceplate + backplate = single bent aluminium) |

### 2.3 — Enclosure Assembly
**Status:** Todo
**Priority:** Normal
**Definition of Done:** All components dry-fitted and mounted in enclosure.

| # | Task | Status | Effort | Deadline | Notes |
|---|------|--------|--------|----------|-------|
| 2.3.1 | Dry fit all boards in enclosure | Todo | Physical | | Internal PCB mounting: 4mm clips for UM2, VHB tape or clips for Pi + MIDI controller |
| 2.3.2 | Final component mounting and wiring | Todo | Physical | | |

---

## Phase 3: Software Polish
**Status:** In Progress
**Definition of Done:** JUCE app stable with all core features working, MIDI Learn reliable, Pi layout confirmed.

### 3.1 — JUCE App Fixes & Features
**Status:** In Progress
**Priority:** Normal
**Definition of Done:** All known bugs fixed, MIDI Learn working, Pi layout default.

| # | Task | Status | Effort | Deadline | Notes |
|---|------|--------|--------|----------|-------|
| 3.1.1 | Fix MIDI Learn — CC change detection + priority over encoder bank | Done | Deep Focus | | Fixed 2026-04-06 |
| 3.1.2 | Fix UTF-8 em-dash assertion in modulation page placeholder knobs | Done | Quick Win | | Fixed 2026-04-06 |
| 3.1.3 | Add PiLayout.cpp/VirtualEncoderBank.h to .jucer file — linker fix | Done | Quick Win | | Fixed 2026-04-06 |
| 3.1.4 | Make Pi layout default on all platforms | Done | Quick Win | | Done 2026-04-06 |
| 3.1.5 | Software level meters on touchscreen | Todo | Deep Focus | | |
| 3.1.6 | Headphone PFL routing | Todo | Deep Focus | | |
| 3.1.7 | 14-bit MIDI CC for fine-resolution parameters | Todo | Deep Focus | | Decision pending |
| 3.1.8 | Fix volume persistence bug — knobs reading from UI sliders instead of loopRecorder | Done | Deep Focus | | Fixed 2026-04-09. Vol, HP, LP now read from loopRecorder.getSlotVolume/HighPass/LowPass. |

---

## Phase 4: Pi Deployment
**Status:** Todo
**Definition of Done:** Raspberry Pi 5 boots into kiosk mode, runs JUCE app full-screen on 7" touchscreen, audio output via UM2 + DAC HAT confirmed.

### 4.1 — Kiosk Mode & Display
**Status:** Todo
**Priority:** Normal
**Definition of Done:** Pi boots straight into JUCE app on 7" touchscreen, no desktop visible.

| # | Task | Status | Effort | Deadline | Notes |
|---|------|--------|--------|----------|-------|
| 4.1.1 | Configure kiosk mode auto-launch | Todo | Deep Focus | | |
| 4.1.2 | Verify 7" touchscreen display and touch input | Todo | Physical | | |
| 4.1.3 | Configure display rotation for inverted screen mount | Todo | Quick Win | | `display_rotate=2` in `/boot/config.txt` preferred — rotates both framebuffer and touch. Decision #31 |

### 4.2 — Audio & Performance
**Status:** Todo
**Priority:** Normal
**Definition of Done:** Dual audio output working (UM2 + DAC HAT), real-time scheduling configured, no glitches.

| # | Task | Status | Effort | Deadline | Notes |
|---|------|--------|--------|----------|-------|
| 4.2.1 | Test dual audio output in JUCE (UM2 + DAC HAT via ALSA) | Todo | Deep Focus | | |
| 4.2.2 | Configure real-time scheduling | Todo | Deep Focus | | |
| 4.2.3 | Address PSU noise and ventilation | Todo | Physical | | |

---

## Phase 5: Integration & Testing
**Status:** Todo
**Definition of Done:** Complete instrument working as a standalone unit — MIDI controller talks to JUCE app on Pi, audio routes through UM2 mod, headphones work, everything housed in enclosure.
**Deadline:** Late April / May 2026

### 5.1 — End-to-End Testing
**Status:** Todo
**Priority:** High
**Definition of Done:** Full signal chain verified: MIDI controller -> Pi -> JUCE -> UM2 -> audio out + headphones.

| # | Task | Status | Effort | Deadline | Notes |
|---|------|--------|--------|----------|-------|
| 5.1.1 | Hardware walkthrough — verify all connections before Cornwall | Todo | Physical | | Pre-Cornwall prep task |
| 5.1.2 | Full signal chain test | Todo | Physical | | |
| 5.1.3 | Headphone monitor test (TPA6133A2 on HAT, A10K pot) | Todo | Physical | | Software untested |
| 5.1.4 | Stress test — extended play session for stability | Todo | Physical | | |

---

## Dependencies

| Item | Depends On | Status |
|------|-----------|--------|
| 1.2.2 Run test firmwares | 1.1 PCB Assembly (complete) | Unmet |
| 2.2.2 CNC machining | 2.1 Design Finalisation (complete) | Unmet |
| 2.3.1 Dry fit | 1.1 + 2.2 (complete) | Unmet |
| 5.1.2 Full signal chain | 1, 2, 3, 4 (all complete) | Unmet |

---

## Reference

### Status Values
| Status | Meaning |
|--------|---------|
| Todo | Not yet started |
| In Progress | Actively being worked on |
| Blocked: [reason] | Cannot proceed — reason is one of: poorly-defined, too-large, missing-info, missing-resource, decision-required |
| Waiting | User's part done, waiting on external input |
| Done | Complete |
| Dropped | Deliberately abandoned |

### Effort Types
| Type | Description |
|------|-------------|
| Deep Focus | Sustained concentration, problem-solving, design work |
| Creative | Open-ended, generative, exploratory |
| Administrative | Organising, documenting, updating, filing |
| Communication | Discussions, reviews, feedback |
| Physical | Hands-on work, building, soldering |
| Quick Win | Small, low-effort, momentum-building |

### Priority
High / Normal / Low — milestones only. Tasks inherit from their milestone unless overridden.
