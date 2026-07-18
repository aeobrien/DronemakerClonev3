# Raspberry Pi 5 Port — Roadmap

## Hardware
- Raspberry Pi 5
- Waveshare 7" capacitive touchscreen (1024x600)
- Focusrite Scarlett Solo (USB audio interface)
- USB MIDI controller (AKAI MIDI Mix)

---

## Phase 1A — CMake Build System Migration (on Mac) ✓

**Goal:** Replace Projucer-based build with CMake. Verify identical app on macOS.

- [x] 1A.1 — Create top-level `CMakeLists.txt` using `juce_add_gui_app`
- [x] 1A.2 — Build on macOS with CMake, verify it launches and works identically
- [x] 1A.3 — Keep `.jucer` file in repo as reference (don't delete it)

---

## Phase 1B — Raspberry Pi OS Setup ✓

**Goal:** Get the Pi 5 booted with an OS and the touchscreen working.

- [x] 1B.1 — Flash Raspberry Pi OS (64-bit) to SD card
- [x] 1B.2 — Boot the Pi, connect to network
- [x] 1B.3 — Connect the Waveshare 7" touchscreen
- [x] 1B.4 — Verify touch input works
- [x] 1B.5 — Plug in Scarlett Solo, verify it appears: `aplay -l`
- [x] 1B.6 — Plug in MIDI controller, verify it appears: `amidi -l`
- [x] 1B.7 — Install build dependencies
- [x] 1B.8 — Clone JUCE 8.0.12 onto the Pi

---

## Phase 2 — Build on the Raspberry Pi ✓

**Goal:** Get DronemakerClone compiling and launching on the Pi.

- [x] 2.1 — Clone repo onto the Pi
- [x] 2.2 — Build with CMake (Pi JUCE path auto-detected)
- [x] 2.3 — Launch the app on the desktop
- [x] 2.4 — Auto-select Scarlett Solo on startup (USB audio auto-detection)
- [x] 2.5 — Test basic audio passthrough and drone processing
- [x] 2.6 — Fix Linux-specific build issues (GCC warnings, API differences)

**Fixes applied:**
- JUCE LevelMeter null channel crash (patched `~/JUCE` on Pi)
- `getCurrentAudioDeviceType()` returns String, not pointer (GCC caught this)
- Unused variable and shadowed variable warnings (GCC stricter than Clang)

---

## Phase 3 — Touchscreen UI Adaptation ✓

**Goal:** Make the UI usable on a 1024x600 touch-only display.

- [x] 3.1 — Dual layout system: `usePiLayout` flag, auto-true on Linux
- [x] 3.2 — Pi layout: `PiLayout.h/cpp` with touch-friendly controls
- [x] 3.3 — Touch loop buttons: short tap = select, long press = toggle record/play/stop
- [x] 3.4 — Replaced combo boxes with direct-select buttons (combos hang on X11/touch)
- [x] 3.5 — Unified knob row (8 rotary knobs, shared between loop and effect modes)
- [x] 3.6 — Detail strip: oscilloscope + waveform overview with trim markers + playhead
- [x] 3.7 — Warm coral/amber color scheme (PiColours namespace)

---

## Phase 4 — MIDI Controller Integration on Pi ✓

**Goal:** Verify MIDI learn/mapping works on the Pi.

- [x] 4.1 — MIDI input working (auto-scans every ~2s)
- [x] 4.2 — MIDI Learn mode works with AKAI MIDI Mix
- [x] 4.3 — Virtual encoder bank for relative MIDI control
- [x] 4.4 — MIDI-mapped loop buttons always trigger toggle (same as long press)

---

## Phase 5 — Kiosk / Embedded Mode ✓

**Goal:** App launches automatically on boot, fullscreen, no desktop environment visible.

- [x] 5.1 — Fullscreen with no title bar or window decorations (`--kiosk` flag)
- [x] 5.2 — Auto-launch on boot via `.bash_profile` + `xinit` (not systemd — see notes)
- [x] 5.3 — Auto-select Scarlett Solo on startup (no manual Settings needed)
- [x] 5.4 — Cold boot → app ready workflow tested and working
- [x] 5.5 — Desktop-mode skip file (`/boot/firmware/desktop-mode`) for maintenance
- [ ] 5.6 — Disable screen blanking / power management
- [ ] 5.7 — Settings window trap: need a way to close it in kiosk mode

**Notes:** systemd service approach was attempted but had issues with TTY ownership and SIGHUP killing xinit. The `.bash_profile` approach on tty1 auto-login is simpler and reliable. See `pi-kiosk/SETUP.md` for full instructions.

---

## Phase 6 — Optimisation & Hardening

**Goal:** Ensure reliable, low-latency performance for live use.

- [ ] 6.1 — Profile CPU usage, especially FFT processing on ARM
- [ ] 6.2 — Tune ALSA buffer sizes for low latency
- [ ] 6.3 — Set real-time thread priority for audio
- [ ] 6.4 — Stress test: long runtime stability, no memory leaks
- [ ] 6.5 — Read-only filesystem or overlay FS to prevent SD card corruption
- [ ] 6.6 — Graceful shutdown on power loss (if needed)

**Done when:** Runs reliably for hours with acceptable latency.

---

## Notes

- The macOS standalone build is NOT affected by Pi work. Platform-specific UI is behind `usePiLayout` flag (auto-detected or CLI override: `--pi-layout` / `--desktop-layout`).
- JUCE version on Mac and Pi must match (currently 8.0.12).
- The Scarlett Solo is USB class-compliant, so no drivers needed on Linux.
- Binary name is `DronemakerClone` (not `DronemakerClonev3`). Located at `build/DronemakerClonev3_artefacts/DronemakerClone`.
