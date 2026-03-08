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
- Keep existing modulation plumbing/wiring intact.
- New interface layer for assigning modulators to parameters using the 8-encoder system.
- Need to design the interaction flow for quick assignment on Pi touchscreen + encoders.

## Effect Slot Param Expansion (use empty knob slots)
- **Delay**: Add HP/LP filter knobs (currently 4 params, 4 slots free). Explore different delay modes.
- **Distortion**: 4-6 params depending on mode, 2-4 slots free. Explore additional shaping options.

## Loop Knobs 4 & 5 (currently placeholders)
- User thinking about modulation or automation control per loop.
- Possibilities: modulation depth, automation level, playback speed fine-tune, pan, reverse toggle.
