# 8-Encoder Control System — Implementation Roadmap

This roadmap covers the complete transition to an 8-encoder-driven interface:
virtual encoder abstraction, contextual MIDI mapping, modulation page,
automation presets, and kiosk deployment on Raspberry Pi.

---

## Hardware Context

- **Target**: 8 rotary encoders with push buttons (Teensy-based custom controller)
- **Interim**: Any MIDI controller with 8 CCs mapped to the virtual encoder slots
- **Display**: Waveshare 7" touchscreen (1024×600) on Raspberry Pi 5
- **Audio**: Focusrite Scarlett Solo USB
- **MIDI**: USB MIDI (MIDI Mix, Teensy, or any class-compliant controller)

---

## Phase 1 — Virtual Encoder Abstraction & Default CC Map

**Goal:** Replace the current direct-pointer MIDI mapping with a virtual encoder
layer. 8 CC inputs always control whichever 8 parameters are currently displayed,
regardless of mode (loop/effect/drone/modulation).

### 1.1 — Define VirtualEncoder Abstraction

- Create `VirtualEncoderBank` class (new file: `Source/VirtualEncoderBank.h/cpp`)
- 8 virtual encoder slots (indices 0–7)
- Each slot holds a **binding** — a callback `std::function<void(float)>` plus metadata:
  - `name` (juce::String) — current parameter name for display
  - `currentValue` (float, 0–1 normalised) — for UI sync
  - `getValue` callback — read current value from the parameter
  - `setValue` callback — write normalised value to the parameter
  - `suffix` (juce::String) — display unit
  - `steps` (int) — number of discrete steps (0 = continuous)
  - `displayMapper` (optional) — maps normalised to display value (e.g., 0–1 → 20–5000 Hz)
- 8 push-button slots (indices 0–7), each holding:
  - `onPress` callback `std::function<void()>`
  - `onRelease` callback `std::function<void()>` (optional, for held actions)
- `rebindAll()` method to swap all 8 bindings at once (called on mode/selection change)
- `handleEncoder(int slot, float normalisedValue)` — resolves to current binding
- `handlePush(int slot, bool pressed)` — resolves to current push binding

### 1.2 — Default CC-to-Encoder Mapping

- Define default CC map: **CC 20–27 → Encoder slots 0–7**
  - CCs 20–27 are in the MIDI "undefined" range, no conflicts
  - Easily changeable — single constexpr array
- Define default push-button map: **Notes 36–43 → Push slots 0–7**
  - Notes 36–43 (C2–G2) are commonly available on pad controllers
  - NoteOn = press, NoteOff = release
- Store defaults in a `MidiDefaults` namespace or constexpr struct
- On startup, auto-create these mappings (no learn required)
- Existing MIDI Learn system still works for overriding/adding custom mappings

### 1.3 — Rearchitect MIDI Input Handling

- Current flow: `CC received → midiCCMappings[cc] → raw slider pointer → set value`
- New flow: `CC received → check if it maps to a virtual encoder → VirtualEncoderBank::handleEncoder(slot, value) → current binding callback`
- Non-encoder CCs (e.g., loop button toggles via MIDI Learn) continue to work as before
- Virtual encoder mappings take priority over legacy direct mappings
- Thread safety: encoder callbacks post to message thread (same pattern as current MIDI handling)

### 1.4 — Wire Existing Modes to VirtualEncoderBank

- **Loop mode** (`updateLoopParameterKnobs`): Rebind encoders to loop selector, trim start, trim end, [slot 4 TBD], [slot 5 TBD], HP, LP, volume
- **Effect mode** (`updateEffectParameterKnobs`): Rebind encoders to the current effect's parameters (already 8 knobs)
- **Drone mode** (`updateDroneParameterKnobs`): Rebind encoders to drone params (already 8 knobs)
- Each mode's `update*Knobs()` function calls `encoderBank.rebindAll(...)` with the appropriate bindings
- The Pi layout knob row should reflect the virtual encoder state (bidirectional sync)

### 1.5 — Bidirectional Sync (MIDI → UI, UI → MIDI)

- When a CC moves an encoder, the on-screen knob must update
- When the user touches an on-screen knob, the value is stored (no MIDI output needed yet)
- Avoid feedback loops: flag when a value change originates from MIDI vs. UI touch

### 1.6 — Preserve Legacy MIDI Learn

- MIDI Learn still works for mapping arbitrary CCs/notes to specific controls
- If a CC is both a virtual encoder AND has a legacy mapping, virtual encoder wins
- "Clear" in MIDI Learn can remove a virtual encoder override to restore default

### 1.7 — Testing & Validation

- Verify: plug in MIDI controller, send CCs 20–27, see the 8 on-screen knobs respond
- Verify: switch from loop mode to effect mode, same CCs now control effect params
- Verify: legacy MIDI Learn still works for non-encoder controls
- Verify: on-screen touch still works independently of MIDI

**Done when:** 8 CCs contextually control whichever 8 parameters are on screen.

---

## Phase 2 — Modulation Page (8-Encoder Control)

**Goal:** Full control of all 7 modulators and their target assignments using only
the 8 encoders.

### 2.1 — Modulation Tab Integration

- Add "Mod" tab to Pi layout tab bar (already exists in header: `modulationTabButton`)
- When Mod tab is active, `activeTab = 2`, rebind encoders to modulation controls
- Main display area shows modulation status (active modulator, targets, value indicator)

### 2.2 — Encoder Layout for Modulation

| Encoder | Function | Type |
|---------|----------|------|
| 1 | **Modulator selector** | Discrete: LFO1, LFO2, LFO3, LFO4, EnvFollow, Rand1, Rand2 (7 positions) |
| 2 | **Param 1** | Continuous: Rate (LFO/Random), Attack (EnvFollow) |
| 3 | **Param 2** | Discrete/Continuous: Waveform (LFO, 6 shapes), Release (EnvFollow), Smoothness (Random) |
| 4 | **Target slot** | Discrete: Slot 1, 2, 3 (which of the 3 target assignments) |
| 5 | **Target parent** | Discrete: None, Loop1–8, Delay, Distortion, Tape, Filter, Tremolo, Master (15 positions) |
| 6 | **Target child** | Discrete: depends on parent — e.g., Volume/HP/LP for loops, Time/Feedback/DryWet for Delay |
| 7 | **Mod range min** | Continuous: 0–1 normalised (mapped to target's actual range) |
| 8 | **Mod range max** | Continuous: 0–1 normalised (mapped to target's actual range) |

### 2.3 — Push Button Actions (Modulation)

| Push | Function |
|------|----------|
| 1 | Toggle modulator enabled/disabled |
| 2 | (EnvFollow only) Toggle to Sensitivity param on encoder 2 |
| 3 | — |
| 4 | Clear current target slot (set to None) |
| 5 | — |
| 6 | — |
| 7 | Reset min to target default |
| 8 | Reset max to target default |

### 2.4 — Dynamic Rebinding on Selection Change

- Turning encoder 1 (modulator selector) triggers rebind of encoders 2–3 to that modulator's params
- Turning encoder 4 (target slot) triggers rebind of encoders 5–8 to that slot's current assignment
- Turning encoder 5 (target parent) updates encoder 6's available options
- Each change calls a partial rebind (not full 8-encoder rebind, just affected subset)

### 2.5 — Modulation Display (Pi Layout)

- Show currently selected modulator name and type (top of display area)
- Real-time modulator output indicator (visual bar, same as desktop)
- Show all 3 target slots with their assignments, highlight the selected slot
- Show min/max range visually on encoders 7–8

### 2.6 — Testing

- Verify: can fully configure any modulator and all 3 targets without touching the screen
- Verify: modulation applies correctly to all target types (loops, effects, master)
- Verify: switching between mod tab and loop/effect tab preserves modulation state

**Done when:** All modulation parameters are controllable via 8 encoders + push buttons.

---

## Phase 3 — Automation Presets (Loop Knobs 4 & 5)

**Goal:** Make loop automation controllable from the encoder interface using presets
and a time-scale parameter.

### 3.1 — Built-in Automation Preset Library

Create a set of hardcoded preset sequences in `LoopAutomation.h` or a new
`AutomationPresets.h`:

| Index | Name | Sequence | Default Duration |
|-------|------|----------|-----------------|
| 0 | Off | (no automation) | — |
| 1 | Fade In | StartPlayback → RampLevel 0→1 | 10s |
| 2 | Fade Out | RampLevel 1→0 → StopPlayback | 10s |
| 3 | Swell | StartPlayback → RampLevel 0→1 → Wait → RampLevel 1→0 (looped) | 30s cycle |
| 4 | Pulse | StartPlayback → RampLevel 0→1 → RampLevel 1→0 (looped) | 4s cycle |
| 5 | Delayed Entry | Wait → StartPlayback → RampLevel 0→1 | 15s total |
| 6 | Fade In & Hold | StartPlayback → RampLevel 0→1 (one-shot) | 10s |
| 7 | Fade Out & Stop | RampLevel 1→0 → StopPlayback (one-shot) | 10s |
| 8 | Long Swell | StartPlayback → RampLevel 0→1 → Wait → RampLevel 1→0 (looped) | 120s cycle |
| 9 | Stutter | StartPlayback → Wait → StopPlayback → Wait (looped) | 2s cycle |

### 3.2 — Time Scale Parameter

- Add `timeScale` field to `LoopSettings` (float, default 1.0)
- `LoopSequenceExecutor` multiplies all command durations by `timeScale`
- Range: 0.1× to 10.0× (10s fade becomes 1s–100s)
- Applied at execution time, not stored in preset (so same preset can be fast or slow)

### 3.3 — Wire to Loop Mode Encoders 4 & 5

| Encoder | Function |
|---------|----------|
| 4 | **Automation preset** — discrete selector (Off, Fade In, Fade Out, Swell, etc.) |
| 5 | **Time scale** — continuous, 0.1×–10.0× with centre detent at 1.0× |

### 3.4 — Push Button Actions (Loop Automation)

| Push | Function |
|------|----------|
| 4 | **Arm/disarm** automation for selected loop (automation will engage when recording stops) |
| 5 | **Preview** automation on selected loop (uses existing preview system) |

### 3.5 — Automation State Display

- Show automation preset name on the loop detail strip
- Show armed/active indicator per loop button (e.g., small "A" badge)
- Show time scale value near encoder 5 label

### 3.6 — Desktop Automation Editor Compatibility

- Preset selection from encoders creates the equivalent sequence in `LoopSettings`
- Desktop editor can still open and show/modify the generated sequence
- Custom sequences edited on desktop don't map to a preset name (shown as "Custom")

### 3.7 — Max Record Length

- Move max record length control to a **long-press on encoder 3** (Trim End)
  - Normal turn: trim end position
  - Push + turn (or push to toggle mode): max record length (5–300s)
- Alternative: settings page accessible via push on a dedicated button
- Not a performance-critical control, doesn't need a dedicated encoder

### 3.8 — Testing

- Verify: selecting a preset and time scale, then recording a loop, triggers correct automation
- Verify: preview works for all presets
- Verify: time scale correctly stretches/compresses durations
- Verify: desktop editor shows the generated sequence correctly

**Done when:** Automation presets selectable from encoder 4, time scale on encoder 5, arm/preview via push buttons.

---

## Phase 4 — Kiosk Mode on Raspberry Pi

**Goal:** Pi boots directly into the app fullscreen. Recoverable via button combo
or SSH. Software updatable via git pull + rebuild.

### 4.1 — Auto-Login Configuration

- Configure `getty` for auto-login on tty1:
  ```
  sudo systemctl edit getty@tty1
  # ExecStart=-/sbin/agetty --autologin <user> --noclear %I $TERM
  ```
- No desktop environment needed — login goes straight to shell

### 4.2 — App Launch via systemd Service

- Create `/etc/systemd/system/dronemaker.service`:
  ```ini
  [Unit]
  Description=DronemakerClonev3
  After=graphical.target sound.target
  Wants=sound.target

  [Service]
  Type=simple
  User=<pi-user>
  Environment=DISPLAY=:0
  ExecStartPre=/usr/bin/xinit /bin/sleep infinity -- :0 &
  ExecStart=/home/<user>/Dev/DronemakerClonev3/build/DronemakerClonev3_artefacts/DronemakerClonev3
  Restart=always
  RestartSec=3

  [Install]
  WantedBy=multi-user.target
  ```
- Alternative: use `cage` (Wayland single-app compositor) instead of xinit:
  ```
  ExecStart=/usr/bin/cage -- /path/to/DronemakerClonev3
  ```
- Watchdog: `Restart=always` ensures auto-restart on crash

### 4.3 — Fullscreen / No Window Decorations

- Add `--kiosk` CLI flag to the app
- When `--kiosk` is active:
  - Window is set to fullscreen, no title bar, no decorations
  - `setUsingNativeTitleBar(false)` + `setFullScreen(true)` on the DocumentWindow
  - Disable screen blanking via `xset s off` / `xset -dpms` in the service pre-start
  - Hide mouse cursor (optional, since it's touch)

### 4.4 — Skip-Kiosk Button Combination

- On boot, check for a physical key/button hold:
  - **Option A**: GPIO button — wire a momentary switch to a GPIO pin, read it in a boot script
  - **Option B**: USB keyboard — hold a specific key (e.g., Escape) during first 5 seconds
  - **Option C**: MIDI note — hold a specific MIDI button during startup
- If skip condition detected:
  - Don't start the kiosk service
  - Boot to normal desktop (start the desktop environment instead)
  - Display a message: "Kiosk mode skipped — booting to desktop"
- Implementation: a small shell script (`/usr/local/bin/kiosk-check.sh`) that runs before the service, checks the condition, and either starts the kiosk or the desktop

### 4.5 — Software Update Workflow

Standard workflow (via SSH):
```bash
ssh pi@dronemaker.local
sudo systemctl stop dronemaker
cd ~/Dev/DronemakerClonev3
git pull
cmake --build build/ -j4
sudo systemctl start dronemaker
```

Or reboot after build:
```bash
# ... git pull + build ...
sudo reboot  # kiosk auto-starts with new binary
```

Optional convenience script (`~/update.sh`):
```bash
#!/bin/bash
sudo systemctl stop dronemaker
cd ~/Dev/DronemakerClonev3
git pull
cmake --build build/ -j4
if [ $? -eq 0 ]; then
    echo "Build succeeded, restarting..."
    sudo systemctl start dronemaker
else
    echo "BUILD FAILED — not restarting"
fi
```

### 4.6 — Audio Device Pinning

- Use ALSA udev rules to pin the Scarlett Solo to a consistent device name
- Or: app auto-selects first USB audio device on startup (add logic to MainComponent)
- Store last-used device name and auto-reconnect

### 4.7 — Screen Blanking Prevention

- Disable DPMS and screensaver:
  ```bash
  xset s off
  xset -dpms
  xset s noblank
  ```
- Run these in the service ExecStartPre or in `.xinitrc`

### 4.8 — Read-Only Filesystem (Optional, Phase 6 territory)

- Use `overlayfs` to make the root filesystem read-only
- Writes go to a RAM-backed overlay
- Prevents SD card corruption on hard power-off
- Temporarily remount read-write for updates:
  ```bash
  sudo mount -o remount,rw /
  # do updates
  sudo mount -o remount,ro /
  ```
- **Defer this to Phase 6** — it adds complexity and isn't needed for initial kiosk

### 4.9 — Testing

- Verify: cold boot → app appears fullscreen within 15–30 seconds
- Verify: audio works immediately (Scarlett Solo auto-selected)
- Verify: MIDI controller detected and default CCs work
- Verify: skip button combo boots to desktop
- Verify: SSH update workflow works (stop → pull → build → start)
- Verify: app auto-restarts after simulated crash (`kill -9`)
- Verify: normal reboot returns to kiosk mode

**Done when:** Power on → fullscreen app. SSH for updates. Button combo for desktop escape.

---

## Phase Summary & Dependencies

```
Phase 1: Virtual Encoder + Default CCs
   ↓
Phase 2: Modulation Page ←─── depends on Phase 1 (uses VirtualEncoderBank)
   ↓
Phase 3: Automation Presets ←─ depends on Phase 1 (uses encoder slots 4–5)
   ↓
Phase 4: Kiosk Mode ←──────── independent, can run in parallel with 2–3
```

Phases 2 and 3 both depend on Phase 1 but are independent of each other.
Phase 4 is entirely independent and can be done at any time.

---

## Default MIDI CC / Note Assignments

### Encoders (Continuous)
| CC | Encoder | Notes |
|----|---------|-------|
| 20 | Encoder 1 | Selector (loop/mod/effect-specific) |
| 21 | Encoder 2 | |
| 22 | Encoder 3 | |
| 23 | Encoder 4 | |
| 24 | Encoder 5 | |
| 25 | Encoder 6 | |
| 26 | Encoder 7 | |
| 27 | Encoder 8 | |

### Push Buttons (Momentary)
| Note | Push | Notes |
|------|------|-------|
| 36 (C2) | Push 1 | |
| 37 (C#2) | Push 2 | |
| 38 (D2) | Push 3 | |
| 39 (D#2) | Push 4 | |
| 40 (E2) | Push 5 | |
| 41 (F2) | Push 6 | |
| 42 (F#2) | Push 7 | |
| 43 (G2) | Push 8 | |

### Loop Buttons (existing MIDI Learn, unchanged)
Mapped via MIDI Learn as before — typically notes or CCs above the encoder range.

---

## Notes

- All encoder values are **normalised 0–1** at the virtual layer. Mapping to actual parameter ranges happens in the binding callbacks.
- The Teensy firmware will eventually hardcode these CC/note numbers. Until then, any MIDI controller can be configured to send them.
- Desktop layout is **unaffected** — virtual encoder bank only activates in Pi layout mode.
- Existing MIDI Learn continues to work for non-encoder mappings (loop button toggles, etc.).
