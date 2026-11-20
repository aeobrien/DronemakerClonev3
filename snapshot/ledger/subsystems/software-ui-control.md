# Software — UI & Control

## Overview

The user interface and control input layer. Dual layout system (desktop and Pi), virtual encoder bank for mapping physical encoders to contextual parameters, MIDI learn/mapping, and touchscreen interaction. The Pi layout is the primary target — warm coral/amber colour scheme, tap-to-select loops, long-press for actions, unified 8-knob row shared between loop and effect modes.

## Status

**Current state:** Working — encoder bank, modulation page, push-button mapping all implemented. MIDI Learn fixed (CC change detection + priority over encoder bank). Pi layout now default on all platforms.
**Last updated:** 2026-04-08

## Components

| Component | Role | Part Number | Ref |
|-----------|------|-------------|-----|
| MainComponent | Central JUCE component, owns audio + GUI | — | Source/MainComponent.h/cpp |
| PiLayout | Touchscreen-optimised layout for Pi | — | Source/PiLayout.h/cpp |
| VirtualEncoderBank | Contextual encoder-to-parameter mapping (CC 20-27, Notes 36-43/44-51) | — | Source/VirtualEncoderBank.h |
| ModulationPanel | Full modulation UI — 4 LFOs, envelope follower, random generators | — | Source/Modulation/ModulationPanel.h |
| MinimalLookAndFeel | Dark theme with mint accent (desktop) | — | Source/MinimalLookAndFeel.h |

## Connections

| Connects To | Interface | Notes |
|-------------|-----------|-------|
| Software — DSP Engine | Parameter callbacks, atomic values | UI reads audio state via atomic loads |
| MIDI Controller | USB MIDI (CC and Note) | CC 20-27 → encoder slots, Notes 36-43 → push buttons, Notes 44-51 → loop toggles |
| Raspberry Pi Platform | Waveshare touchscreen (HID) | Touch events as standard mouse input via X11 |

## Design Notes

**Dual layout:** `usePiLayout` flag defaults to `true` on all platforms (changed 2026-04-06, previously Linux-only). Overridable with `--pi-layout` / `--desktop-layout` CLI flags. Desktop layout unchanged.

**MIDI Learn:** CC change detection filter — requires seeing a CC value change before accepting, filtering out idle/background traffic from the controller. Learned mappings take priority over VirtualEncoderBank dispatch (Decision #29), so CCs in the encoder range (20-27) can be overridden by MIDI Learn.

**Pi layout:** Mutual exclusion between loop mode and effect mode. 8 rotary knobs contextually bound to active mode. Detail strip: oscilloscope visualiser + waveform overview with trim markers + playhead.

**Virtual encoder bank:** Header-only implementation. 8 encoder slots with `EncoderBinding` (name, range, step, suffix, get/set callbacks). 8 push-button slots with press/release callbacks. `clearAll()` + re-bind on mode switch. 25+ binding sites in MainComponent.

**Modulation page:** Tab-based (Drone / Loops / Modulation). Full panel with 4 LFOs, envelope follower, random generators.

**Font:** Avenir Next Condensed (macOS), Liberation Sans Narrow fallback (Linux).

## Open Questions

- UI polish once tested on actual hardware with encoders
- PFL (pre-fader listen) button for headphone monitoring — needs to be added to touchscreen UI
- Display rotation for inverted screen mount — `display_rotate=2` in `/boot/config.txt` (preferred, handles both framebuffer + touch) or JUCE-level AffineTransform + libinput coordinate matrix
