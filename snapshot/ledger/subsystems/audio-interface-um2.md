# Audio Interface (UM2 Mod)

## Overview

A Behringer UM2 (board rev PCB302082REVJ_01) stripped of its original enclosure and user-facing components, with panel-mount replacements wired back to the PCB pads. Provides mono input (combo XLR/TRS + instrument), stereo output, phantom power, preamp gain, and USB audio — all the audio I/O the instrument needs. The PCB retains all active analogue circuitry, ADC/DAC, and USB audio controller.

## Status

**Current state:** Nearly complete — XLR/TRS jack and LEDs remaining. Original pots successfully removed and reattached with hookup wire (no replacement pots needed). Some board-mount components were left in place where removal wasn't necessary.
**Last updated:** 2026-03-30

## Components

| Component | Role | Part Number | Ref |
|-----------|------|-------------|-----|
| UM2 PCB | Core audio circuit (preamps, ADC/DAC, USB) | Behringer UM2 (PCB302082REVJ_01) | Board retained internally |
| Combo XLR/TRS jack | Mic/line input (panel-mount replacement) | Neutrik NCJ6FI-S (or NCJ9FI-S) | Still to be wired |
| Instrument jack | 1/4" TS input (panel-mount) | Neutrik NJ3FP6C-BAG | Wired and tested |
| Left output jack | 1/4" TRS (panel-mount, replaces RCA) | Neutrik NJ3FP6C-BAG | Wired and tested |
| Right output jack | 1/4" TRS (panel-mount, replaces RCA) | Neutrik NJ3FP6C-BAG | Wired and tested |
| Phantom power switch | DPDT on-on toggle (panel-mount) | Multicomp Pro 1MD1T1B1M1QE | Wired and tested |
| Mic/line gain pot | Original UM2 pot, reattached with hookup wire | 05C50K dual-gang | Working |
| Instrument gain pot | Original UM2 pot, reattached with hookup wire | W50K single-gang | Working |
| Output volume pot | Original UM2 pot, reattached with hookup wire | A20K dual-gang | Working |
| Clip/signal LEDs ×4 | Input level indicators (panel-mount) | 5mm red + green in 5mm bezels | NOT YET WIRED — see notes |

## Connections

| Connects To | Interface | Notes |
|-------------|-----------|-------|
| Raspberry Pi Platform | USB audio (internal cable) | ALSA class-compliant, no drivers |
| Software — DSP Engine | Audio I/O via JUCE AudioDeviceManager | Mono in, stereo out |
| Enclosure & Faceplate | Panel-mount connectors + pots | Backplate: audio jacks, phantom switch. Faceplate: pots, LEDs |

## Design Notes

**Pots:** Original UM2 pots cleanly removed and reattached via hookup wire. No value matching concerns — they are the originals.

**LEDs:** Original UM2 LEDs were 3mm, housed in a dual-LED plastic component. Removed LEDs cannot be reused in their original form. Replacement plan: individual 5mm LEDs in 5mm panel-mount bezels (bezels ordered, arriving ~2026-03-30). Need to verify the electrical specs of the originals to confirm 5mm replacements will work — likely just need matching forward voltage and current draw, which for indicator LEDs is usually not critical.

**Direct monitor switch:** Set to "off" (output only), secured with hot glue. Internal — not panel-mounted.

**Headphone jack:** Removed from board. Pads may need switching contacts bridged (the original headphone jack may have had normalling contacts that disconnect the main output when headphones are inserted — if so, those contacts need to be permanently bridged to keep the main output active). Headphone output handled by separate Headphone Monitor subsystem.

Full teardown procedure and photos in `docs/audio-interface/`. Note: some details in the teardown doc may not match final decisions — some components were left on the board rather than removed.

## Open Questions

- Combo jack part: NCJ6FI-S vs NCJ9FI-S (depends on pin compatibility with phantom power wiring)
- LED electrical specs: need to check original UM2 LED forward voltage/current to confirm 5mm replacements are compatible
- Headphone jack pad bridging — may have normalling/switching contacts in main output path that need bridging
