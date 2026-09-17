# Headphone Monitor

## Overview

Independent headphone output, separate from the UM2's main stereo output. The UM2's headphone jack was linked to the main output and has been removed. A Pi audio HAT provides a second audio output, fed through a headphone amplifier to a panel-mount TRS socket. This enables an independent monitor bus with PFL (pre-fader listen) capability during performance.

## Status

**Current state:** Architecture decided — HAT has built-in headphone amp (TPA6133A2), just needs pot + jack wiring. Software dual-output untested.
**Last updated:** 2026-03-31

## Components

| Component | Role | Part Number | Ref |
|-----------|------|-------------|-----|
| DAC HAT | Second audio output from Pi (I2S) + headphone amp | InnoMaker HiFi DAC HAT (PCM5122 + TPA6133A2) | Already owned |
| Headphone volume pot | Passive attenuator on amp output | Dual A10K audio taper (Alpha 9mm spare) or dual A1K (ideal, to source) | Spare in hand, upgrade optional |
| TRS socket | Panel-mount headphone jack (backplate) | Neutrik NJ3FP6C-BAG | Same as output jacks |

## Connections

| Connects To | Interface | Notes |
|-------------|-----------|-------|
| Raspberry Pi Platform | I2S (HAT stacked on Pi GPIO) | Second audio device in ALSA |
| Software — DSP Engine | Second audio output stream | Needs JUCE multi-device output or ALSA multi-device config |
| Enclosure & Faceplate | Panel-mount TRS socket | Location TBD |

## Design Notes

**Default behaviour:** Headphone output mirrors L/R main output.

**PFL mode:** A push-button (on MIDI controller or touchscreen) will solo a selected channel/loop to headphones only. This is a key performance feature — allows previewing loops before bringing them into the main mix.

**Headphone amp — resolved:** The InnoMaker HAT includes a TPA6133A2 headphone amplifier IC (TI). No separate amp needed.
- 138mW into 16Ω, ~40-70mW into 32Ω, ~15-25mW into 150Ω, ~10-15mW into 250Ω
- Supply: 2.5–5.5V (runs from Pi rail)
- Output impedance: ~4Ω
- Quiescent current: 4.2mA
- Adequate for 32–150Ω headphones. Marginal for inefficient 250Ω cans but usable.

**Volume control:** Passive dual-gang audio taper pot as voltage divider between TPA6133A2 output and panel-mount TRS jack. Using dual A10K (spare in hand) with **1KΩ taper-shaping resistor per channel** from wiper to ground. This spreads the usable volume sweep across the full rotation instead of compressing it into the last 20%. Wiring per channel: amp output → pot pin 3, pot wiper → TRS tip/ring AND 1KΩ resistor to ground, pot pin 1 → ground. Software volume via PCM5122 digital register available as secondary trim.

**Software challenge:** JUCE's AudioDeviceManager typically handles one audio device. Options:
1. ALSA multi-device config (combine UM2 + HAT into one virtual device with extra channels)
2. JACK routing (more flexible but adds complexity)
3. Direct ALSA output from the app for the second device (bypassing JUCE for headphone output)

The InnoMaker HAT uses PCM5122 DAC over I2S — well-supported on Pi, should appear as a second ALSA device automatically.

**Physical:** HAT stacks on Pi GPIO header. Need to confirm clearance inside Hammond enclosure with all other components.

## Open Questions

- Dual audio output in JUCE — needs testing and architecture decision
- Physical clearance for HAT + Pi stack in enclosure (dry fit needed)
- PFL implementation in software (UI button + routing logic)
- Source 2× 1KΩ resistors for pot taper shaping (may already be in stock)
