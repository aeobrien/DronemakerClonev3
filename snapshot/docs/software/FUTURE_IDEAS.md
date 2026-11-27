# Future Ideas & Enhancements

## Flexible Effects Chain Architecture
- **Variable drone position**: Move the drone maker's position within the effects chain. Currently drone processes first, then all effects. Want to be able to place it anywhere (e.g., filter → distortion → drone → tape → filter).
- **User-assignable effect slots**: 6 effect slots should be empty by default. User chooses what to add and in what order. Could have 3 filters + 3 distortions, or any combination. This replaces the current fixed effect layout.
- **Gain compensation**: All effects need gain makeup/compensation to maintain consistent signal levels. Distortion in particular causes large volume jumps.

## Bounce-in-Place for Loops
- Process a loop through the drone maker (record 2x through for seamless crossfade), bounce the resulting drone audio to a new buffer.
- Swap out the original loop → drone processing with simple playback of the bounced drone audio.
- Frees up processing power — 8 bounced loops just play back without needing the drone processor.
- Allows independent control of each drone element after bouncing.
- Logistics TBD — buffer management, crossfade strategy, UI for triggering bounce.

## Tremolo / Gain Shaper Dual Mode
- **Mode 1**: Standard tremolo (current).
- **Mode 2**: Gain shaper / LFO tool — preset drawn shapes (rhythmic patterns, swells in/out, etc.). Cycle through presets and apply instead of straight tremolo.
- Switchable between the two modes.

## Modulation System on 8-Encoder UI
- **DESIGNED** — see `ENCODER_SYSTEM_ROADMAP.md` Phase 2
- Enc1=modulator selector, Enc2-3=params, Enc4=target slot(1/2/3), Enc5=target parent, Enc6=target child, Enc7=min, Enc8=max
- Push buttons for enable/disable, clear target, reset ranges

## Loop Knobs 4 & 5 — Automation Presets
- **DESIGNED** — see `ENCODER_SYSTEM_ROADMAP.md` Phase 3
- Knob 4: automation preset selector (Off, Fade In, Fade Out, Swell, Pulse, etc.)
- Knob 5: time scale multiplier (0.1×–10.0×)
- Push 4: arm/disarm automation, Push 5: preview

## Loop State Visibility & Control Conflicts (flagged during Phase 2)
Three related UX problems to solve:

1. **No per-loop volume/filter indicators**: With the old desktop layout, per-loop volume/HP/LP sliders were always visible and showed modulation/automation movement. Now those are only visible when a loop is selected on the 8 encoders — you can't see all 8 loops' states at once.

2. **Manual vs automated control conflict**: If volume is being automated (ramping up/down) and the user grabs the encoder to set volume manually, what should happen? Options to explore:
   - Manual override temporarily suspends automation for that param
   - Manual input sets a new "base" that automation modulates around
   - A "takeover" mode where manual control only engages once the knob crosses the current automated value
   - Need to decide behaviour and make it feel predictable in performance

3. **Always-visible loop status indicators**: Want some compact visual indicator on the loop buttons (1–8 row, always visible) showing volume level, filter state, and/or modulation activity at a glance. Ideas:
   - Small level meter bars on each loop button
   - Colour-coded brightness based on current volume
   - Thin HP/LP indicator lines
   - Modulation activity dot/pulse

4. **Automation time scale needs visual feedback**: The time scale encoder (0.1x–10.0x) is meaningless without knowing the base duration. Could show actual cycle time on the label, but really this is solved by point 3 — if loop buttons show live volume indicators, you'd see the automation shape playing out in real time and intuitively understand the speed. Points 3 and 4 converge on the same solution.

These are secondary to core system work but important for live usability.

## Effect Slot Param Expansion (use empty knob slots)
- **Delay**: Add HP/LP filter knobs (currently 4 params, 4 slots free). Explore different delay modes.
- **Distortion**: 4-6 params depending on mode, 2-4 slots free. Explore additional shaping options.
