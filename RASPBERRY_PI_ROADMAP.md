# Raspberry Pi 5 Port — Roadmap

## Hardware
- Raspberry Pi 5
- Waveshare 7" capacitive touchscreen (1024x600)
- Focusrite Scarlett Solo (USB audio interface)
- USB MIDI controller (generic, for mapping to controls)

---

## Phase 1A — CMake Build System Migration (on Mac)

**Goal:** Replace Projucer-based build with CMake. Verify identical app on macOS.

- [ ] 1A.1 — Create top-level `CMakeLists.txt` using `juce_add_gui_app`
- [ ] 1A.2 — Build on macOS with CMake, verify it launches and works identically
- [ ] 1A.3 — Keep `.jucer` file in repo as reference (don't delete it)

**Done when:** `cmake --build` produces a working macOS app matching current behaviour.

---

## Phase 1B — Raspberry Pi OS Setup (you, in parallel)

**Goal:** Get the Pi 5 booted with an OS and the touchscreen working.

- [ ] 1B.1 — Flash Raspberry Pi OS (64-bit, Desktop edition) to SD card
  - Use Raspberry Pi Imager from your Mac
  - Choose "Raspberry Pi OS (64-bit)" — the full desktop version
  - Set hostname, enable SSH, set username/password during imaging
- [ ] 1B.2 — Boot the Pi, connect to your network (ethernet or wifi)
- [ ] 1B.3 — Connect the Waveshare 7" touchscreen
  - Typically connects via DSI ribbon cable or HDMI + USB touch
  - May need config.txt edits depending on model — check Waveshare wiki for your exact model
- [ ] 1B.4 — Verify touch input works (open a browser, tap around)
- [ ] 1B.5 — Plug in Scarlett Solo, verify it appears: `aplay -l`
- [ ] 1B.6 — Plug in MIDI controller, verify it appears: `amidi -l`
- [ ] 1B.7 — Install build dependencies:
  ```bash
  sudo apt update && sudo apt upgrade -y
  sudo apt install -y build-essential cmake git \
    libasound2-dev libfreetype6-dev libx11-dev \
    libxrandr-dev libxinerama-dev libxcursor-dev \
    libgl1-mesa-dev libcurl4-openssl-dev \
    libwebkit2gtk-4.0-dev pkg-config
  ```
- [ ] 1B.8 — Clone JUCE onto the Pi:
  ```bash
  cd ~
  git clone https://github.com/juce-framework/JUCE.git
  cd JUCE && git checkout 8.0.4  # or whichever version matches your Mac
  ```

**Done when:** Pi boots to desktop, touchscreen works, Scarlett and MIDI show up, build tools installed.

---

## Phase 2 — Build on the Raspberry Pi

**Goal:** Get DronemakerClone compiling and launching on the Pi.

- [ ] 2.1 — Clone this repo onto the Pi (or push from Mac via SSH/git)
- [ ] 2.2 — Point CMake at the Pi's JUCE installation and build
- [ ] 2.3 — Launch the app — expect it to open a window on the desktop
- [ ] 2.4 — Configure audio: select Scarlett Solo as input/output via Settings dialog
- [ ] 2.5 — Test basic audio passthrough and drone processing
- [ ] 2.6 — Fix any Linux-specific build or runtime issues

**Done when:** App runs on Pi, processes audio through the Scarlett, UI is visible on touchscreen.

---

## Phase 3 — Touchscreen UI Adaptation

**Goal:** Make the UI usable on a 1024x600 touch-only display.

- [ ] 3.1 — Set initial window size to 1024x600, test current layout
- [ ] 3.2 — Identify controls that are too small for finger interaction
  - Minimum touch target: ~44x44 px (Apple HIG guideline)
  - Rotary knobs, buttons, combo boxes all need checking
- [ ] 3.3 — Adjust layout for 1024x600 (likely needs reorganisation)
  - May need scrollable panels or more tabs
  - Progress bars and waveform displays need resizing
- [ ] 3.4 — Remove/adapt hover-dependent interactions (no hover on touchscreen)
- [ ] 3.5 — Remove dialog windows that assume mouse (Settings, waveform popup)
  - Replace with inline panels or fullscreen overlays
- [ ] 3.6 — Test all interactions via touch only — no mouse, no keyboard
- [ ] 3.7 — Add on-screen controls for anything currently keyboard-only

**Done when:** Every feature is accessible and usable via touch alone at 1024x600.

---

## Phase 4 — MIDI Controller Integration on Pi

**Goal:** Verify MIDI learn/mapping works on the Pi.

- [ ] 4.1 — Verify MIDI input is received (check with `aseqdump`)
- [ ] 4.2 — Test MIDI Learn mode with your controller
- [ ] 4.3 — Fix any Linux MIDI issues (ALSA vs JUCE MIDI device enumeration)
- [ ] 4.4 — Test saving/loading MIDI mappings (persistence across restarts)

**Done when:** MIDI controller can map to and control all parameters.

---

## Phase 5 — Kiosk / Embedded Mode

**Goal:** App launches automatically on boot, fullscreen, no desktop environment visible.

- [ ] 5.1 — Make app launch fullscreen (no title bar, no window decorations)
- [ ] 5.2 — Create a systemd service to auto-launch the app on boot
- [ ] 5.3 — Disable screen blanking / power management
- [ ] 5.4 — Auto-select the Scarlett Solo on startup (no manual Settings needed)
- [ ] 5.5 — Test cold boot → app ready workflow
- [ ] 5.6 — Optional: strip down to minimal OS (remove desktop environment entirely)

**Done when:** Power on the Pi → app appears fullscreen on touchscreen → ready to use.

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

- The iOS/macOS standalone build is NOT affected by any of this work. The CMake migration builds the same source code with a different build system. Platform-specific UI tweaks will be `#if JUCE_LINUX` guarded where needed.
- JUCE version on Mac and Pi must match. Check your Mac's JUCE version before cloning on the Pi.
- The Scarlett Solo is USB class-compliant, so no drivers needed on Linux.
