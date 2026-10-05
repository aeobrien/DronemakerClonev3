# Software — DSP Engine

## Overview

The core audio processing pipeline: spectral drone engine (STFT analysis/resynthesis with per-bin peak-hold and decay), multi-layer loop recorder (8 slots with trim, bounce-in-place, delayed dry playback), effects chain (filter, delay, granular, tremolo, distortion, tape), and LFO modulation routing. All processing runs in a single-threaded JUCE audio callback.

## Status

**Current state:** Working — gain compensation implemented, JUCE best-practices optimisation pass done (untested on Pi)
**Last updated:** 2026-03-30

## Components

| Component | Role | Part Number | Ref |
|-----------|------|-------------|-----|
| FFTProcessor | Per-channel STFT engine (analysis, bin processing, resynthesis) | — | Source/FFTProcessor.h/cpp |
| LoopRecorder | 8-slot loop capture with configurable length, trim, bounce | — | Source/LoopRecorder.h/cpp |
| LoopSequenceExecutor | Sample-accurate automation playback for loops | — | Source/LoopSequenceExecutor.h/cpp |
| EffectsChain | Routes audio through 6 effects in series, applies gain compensation | — | Source/EffectsChain.h/cpp |
| ModulationManager | LFO-to-parameter routing | — | Source/Modulation/ModulationManager.h/cpp |
| EffectBase | Base class with SmoothedParam + gain compensation (manual dB + auto factor) | — | Source/Effects/EffectBase.h |

## Connections

| Connects To | Interface | Notes |
|-------------|-----------|-------|
| Software — UI & Control | Atomic values, parameter callbacks | Audio thread ↔ UI thread via std::atomic |
| Audio Interface (UM2 Mod) | ALSA/USB audio | Mono in, stereo out via JUCE AudioDeviceManager |
| Raspberry Pi Platform | CPU budget | Single-core audio callback ~11.6ms at 512 samples/44.1kHz |

## Design Notes

Signal flow: Live Input → FFT Analysis → Spectral Processing (peak-hold, decay, threshold) → IFFT Resynthesis → Loop Recorder → Effects Chain → Modulation → Output.

**Gain compensation:** Implemented in EffectBase + EffectsChain. Each effect has manual gain comp (dB) and auto gain factor. DistortionEffect calculates auto gain as `1/sqrt(drive)`. Applied per-effect to both L/R channels.

The spectral engine's inherent build-up time (several seconds for changes to manifest) is a defining characteristic. The "delayed dry playback" system compensates by delaying the loop's dry signal so changes and drone results are heard simultaneously.

No state persistence between sessions — the instrument starts fresh each time.

A JUCE best-practices optimisation pass has been completed but not yet tested on Pi hardware. Further optimisation passes planned.

## Open Questions

- Repositionable drone engine (anywhere in effects chain) — planned post-v1, not urgent
- Performance headroom on Pi 5 with all loops and effects active — needs testing after optimisation
