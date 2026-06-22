# DronemakerClone — Vision Statement

## Intent

DronemakerClone is a standalone electronic instrument for performing and improvising ambient music. It is built around a spectral drone engine — reverse-engineered from Michael Norris' Spectral DroneMaker — that analyses incoming audio and generates evolving drones derived from the pitch and timbre of the input. The instrument extends this core with a multi-layer loop recorder, a chain of audio effects, and a modulation system, enabling the performer to build long, sprawling, ambient soundscapes in real time.

The software runs as a JUCE application on a Raspberry Pi, housed in a physical enclosure with a capacitive touchscreen and rotary encoders. The goal is a self-contained, performance-ready device that brings the depth and richness of studio ambient production into a live, improvisational context.

## Motivation

This project exists because the original Spectral DroneMaker plugin only runs inside a DAW, which limits how it can be used in live performance. A DAW-based workflow introduces friction and rigidity that work against the improvisational, free-time nature of the music this instrument is designed to create. By reverse-engineering the effect and building it into a standalone application, the instrument gains independence from the DAW and opens up possibilities — looping, effects processing, modulation routing — that the original plugin was never designed to support.

More broadly, this is about creating a bespoke tool shaped around a specific creative practice. Commercial instruments and plugins offer general-purpose functionality, but this instrument is purpose-built for one thing: generating rich, evolving ambient soundscapes from live input, in real time, with direct physical control.

## Audience

This is a personal instrument, built for the creator's own use. There is no commercial intent. The sole user is the builder, and the design decisions reflect their specific workflow, preferences, and creative goals.

## Scope

### Version One — The Instrument Exists

Version one is defined by the instrument having a physical form. The current software runs on a Raspberry Pi 5 with a capacitive touchscreen, rotary encoders for hands-on control, and audio input/output via a USB audio interface (Scarlett Solo). At this point, the instrument is playable and usable for live performance, even if the software continues to evolve.

Hardware for version one:
- Raspberry Pi 5 in an enclosure
- Capacitive LED touchscreen
- Nine rotary encoders (eight mapped to the active effect's parameters, one as a general-purpose selector knob)
- USB audio interface for mono input and stereo output
- Enclosure with front panel accommodating screen and encoders

Software at version one includes the current feature set: spectral drone engine, multi-layer loop recorder with configurable loop lengths, six post-drone effects (filter, delay, granular, tremolo, distortion, tape), and LFO modulation routing.

### Beyond Version One — Software Evolution

These are planned software enhancements to be pursued after (or alongside) the hardware build:

- **Repositionable drone maker** — placing the spectral drone engine at any point in the effects chain rather than always first
- **Gain compensation** — automatic level management across effects to keep input and output levels comparable
- **Per-loop dry/wet control** — independent control over how much of each loop's dry signal versus drone-processed signal is heard
- **Clickless looping** — ensuring completely seamless loop points for audible loops
- **Offset playback synchronisation** — delaying loop output so that changes in the loop and the resulting drone changes are heard simultaneously, compensating for the drone engine's inherent build-up time
- **MIDI mapping** — external MIDI controller support for parameter control

### Beyond Version One — Hardware Evolution

- Replacing the external USB audio interface with an integrated audio input/output solution
- Additional physical controls (buttons for loop record start/stop, etc.) as needs become clear through use

### Explicitly Out of Scope

- Plugin format (VST/AU) — this is a standalone instrument
- Multi-user or commercial considerations
- Mobile platform support
- Polished consumer-grade industrial design — the enclosure needs to be functional, not beautiful

## Design Principles

**Live-first.** Every design decision should favour real-time, hands-on usability. If something works well in a studio workflow but is awkward to operate on stage with your hands on the controls, it's the wrong solution.

**Free time, not clock time.** The instrument operates outside of tempo and meter. Loops are defined by duration, not bars. There is no transport, no metronome, no quantisation. This is fundamental to the instrument's identity and should not be compromised.

**Direct physical control.** Parameters should be controllable without navigating menus or nested screens. The touchscreen-plus-encoders interface exists to keep the most important controls immediately accessible. The dedicated encoder-per-parameter approach for effects and the tap-to-select approach for other parameters prioritise speed and directness.

**Build, then refine.** Especially on the hardware side, favour getting something working over getting it perfect. The software can always be updated. Hardware decisions that turn out to be wrong can be revisited, but only after learning from actual use.

**Graceful signal flow.** Audio levels should remain manageable throughout the signal chain. Effects that add gain should be compensated for. The performer should be able to focus on the music, not on managing levels to prevent clipping or signal loss.

## Definition of Done

**Version one is complete when:**
- The JUCE application runs reliably on the Raspberry Pi 5
- Audio input and output work via the USB audio interface
- The touchscreen displays the UI and responds to touch input
- The nine rotary encoders are connected and controlling parameters (eight for the active effect, one as selector)
- The instrument can be used to perform a live ambient set: input audio, generate drones, record and layer loops, apply and modulate effects, all from the physical controls
- The whole system is housed in an enclosure that can sit on a table or stage

**The broader project is "done" in the sense of reaching maturity when** the performer can reliably use it as their primary instrument for live ambient performance without regularly hitting limitations that interrupt the creative flow. This is subjective and will evolve with use.

## Mental Model

This is a studio-in-a-box for live ambient performance. In the studio, you'd build an ambient piece by layering tracks, processing them through chains of effects, automating parameters over time, and carefully sculpting the spectral content. This instrument compresses that workflow into something you can do with your hands in real time — the loops are your tracks, the effects chain is your processing, the modulation system is your automation, and the spectral drone engine is the core transformation that turns ordinary input into something otherworldly.

## Ethical Considerations

The spectral drone engine is reverse-engineered from Michael Norris' Spectral DroneMaker. This is a personal instrument with no commercial intent, and the reimplementation is original code rather than decompiled or copied source. However, the creative debt to Norris' work should be acknowledged, and if the project were ever to become public or commercial, the relationship to the original would need to be addressed transparently.