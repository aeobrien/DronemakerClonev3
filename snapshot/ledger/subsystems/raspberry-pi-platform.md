# Raspberry Pi Platform

## Overview

The compute platform for the instrument. A Raspberry Pi 5 running Raspberry Pi OS, connected to a Waveshare 7" capacitive touchscreen (1024×600) via HDMI + USB, with USB audio (UM2) and USB MIDI (Teensy controller). Runs the JUCE application in kiosk mode — boots directly into the instrument UI with no desktop environment visible.

## Status

**Current state:** Working — app runs on Pi, kiosk mode configured
**Last updated:** 2026-03-30

## Components

| Component | Role | Part Number | Ref |
|-----------|------|-------------|-----|
| Raspberry Pi 5 | Main compute | Raspberry Pi 5 (4GB or 8GB) | |
| Waveshare 7" LCD | Display + touch input | Waveshare 7inch HDMI LCD (C) | 1024×600, capacitive |
| Power supply | Pi power | Official Pi 5 27W USB-C PSU | Switched-mode — noise source |
| MicroSD card | Boot + OS storage | TBD | |

## Connections

| Connects To | Interface | Notes |
|-------------|-----------|-------|
| Audio Interface (UM2 Mod) | USB audio | ALSA class-compliant |
| MIDI Controller | USB MIDI | Class-compliant |
| Headphone Monitor | I2S (HAT) or USB | TBD |
| Waveshare touchscreen | HDMI (display) + USB (touch) | Touch appears as HID mouse input |
| Enclosure & Faceplate | Internal mounting | Standoffs or bracket |

## Design Notes

**OS:** Raspberry Pi OS (Debian-based). JUCE 8.0.12 installed at `~/JUCE`.

**Build:** CMake build system. `CMakeLists.txt` at project root, defaults to `~/JUCE` on Linux.

**Kiosk mode:** Boots directly into the JUCE app. Scripts in `pi-kiosk/`. No desktop environment visible during normal operation.

**JUCE patches applied on Pi:**
- LevelMeter null channel crash: patched `~/JUCE` (null check in updateLevel)
- Combo box popups hang on X11/touch: avoided in Pi layout (direct-select buttons instead)

**Runtime fixes:**
- MIDI devices not found at startup: periodic rescan every ~2s in timerCallback
- DBG doesn't print in release builds: use std::cerr for Pi diagnostics

**Performance:** Single-core audio callback. Pi 5 quad-core Cortex-A76 is capable but only one core does audio. Buffer size 512 samples at 44.1kHz = ~11.6ms budget.

## Open Questions

- Performance headroom with all loops + effects active
- Real-time scheduling permissions (rtkit or manual setup)
- Power supply noise — switched-mode PSU sharing enclosure with analogue audio
- GPIO HAT for headphone output — physical stacking with other connections
