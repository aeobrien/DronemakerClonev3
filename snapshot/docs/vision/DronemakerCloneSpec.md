# DronemakerClone (JUCE) — Spec & Reference Pack Brief

## 0) What this document is for

This is a **handoff spec** to accompany a folder of reference code. It’s intended to be given to a coding LLM (CLI) with one instruction: **implement a JUCE app that recreates the core behaviour of Michael Norris’ “Spectral DroneMaker” using explicit STFT spectral-bin processing**.

We already proved we can:

* build a fresh JUCE Audio App in Xcode,
* get reliable **input → output passthrough**,
* embed/link third-party frameworks successfully on macOS,
* and (briefly) integrate Bungee as an STFT engine (useful lessons, but we’re now pursuing the **bin-level approach** rather than Bungee’s black-box streaming API).

---

## 1) Goal

Build a **real-time spectral drone processor** (initially as a JUCE macOS app), that:

* takes **live input audio** (mic/line),
* converts to **STFT frames**,
* performs **DroneMaker-style per-bin stateful processing** (peak-hold / decay / threshold / interpolation behaviour),
* resynthesises via **ISTFT overlap-add**,
* outputs a drone-like transformation while maintaining **coherent timbre/pitch relationships** (unlike crude glitch/grain repetition).

This is meant to be “hardware-friendly” long term (eventually Raspberry Pi), but the immediate target is a stable macOS JUCE app.

---

## 2) Target effect family (what “DroneMaker-ish” means)

SoundMagic Spectral is a suite of real-time spectral plug-ins including **Spectral DroneMaker** and related effects like **Spectral Freezing** and **Spectral Gate and Hold**. ([michaelnorris.info][1])

The approach strongly implies **bin-level state** (watch bins over time, hold/decay peaks, gate bins, etc.). The “Spectral Gate and Hold” description is especially explicit about per-bin watching, freezing, hold durations, and decay behaviour. ([Scribd][2])

---

## 3) Functional spec (core DSP behaviour)

### 3.1 STFT engine requirements

Implement a streaming STFT/ISTFT pipeline:

* Configurable `fftSize` (start 2048/4096; later 8192/16384/32768)
* `hopSize` (typically `fftSize/4` for Hann; allow variation)
* Window function (Hann by default)
* Overlap-add synthesis with correct scaling
* Continuous streaming with a ring buffer:

  * audio in arrives in blocks (`bufferSize` from device)
  * accumulate into STFT analysis buffer
  * emit spectra at hop cadence
  * reconstruct to output FIFO, read out to audio device callback

This is the foundation; **bin processing happens between FFT and IFFT**.

### 3.2 DroneMaker-style “bin accumulation” behaviour

Implement a per-bin state machine, inspired by the “spectral accumulation / gate & hold / freeze factor” descriptions:

#### Per-bin state data (for each FFT bin `k`)

Maintain:

* `heldMag[k]` (current held magnitude)
* `heldPhase[k]` (phase strategy; see below)
* Optional: `timeSinceHold[k]` or `holdSamplesRemaining[k]` (if implementing “hold for duration” behaviour)
* Optional: `lastPeakMag[k]` (for peak tracking)
* Optional: RNG seed per bin (if adding per-bin randomisation)

#### Analysis values per frame

From FFT each frame:

* `mag[k]`, `phase[k]`

#### Core accumulation rule (peak-hold with decay)

A straightforward interpretation of “each bin watched until it reaches a peak… held until exceeded… Freeze Factor adds decay” (as documented for the suite’s related effects) is:

* If `mag[k] > heldMag[k]` and (if threshold enabled) `mag[k] >= threshold`:

  * `heldMag[k] = mag[k]`
* Else:

  * `heldMag[k] *= decayCoefficient`
    where `decayCoefficient` comes from “Freeze Factor” semantics: 100% ≈ no decay, lower values decay faster. ([Scribd][2])

Additionally, if “zero bins under threshold” behaviour is enabled (described in the manual excerpt):

* If `mag[k] < threshold` and the bin is not currently “frozen/held” (or a chosen rule):

  * output magnitude becomes 0. ([Scribd][2])

This produces evolving spectra that “accumulate” prominent partials over time rather than following the instantaneous input spectrum.

#### Phase strategy (important)

To avoid “buzzy/robotic/glitched” results, phase must be treated carefully. Options:

1. **Use input phase as-is** each frame, but apply held magnitude

   * Simple, tends to keep some naturalness, but can be unstable when magnitude is disconnected from input.
2. **Phase propagation / phase vocoder style**

   * Track phase increments per bin to maintain continuity.
3. **Randomised phase** (optionally very gentle)

   * More “smeared/noisy”, often drone-like, but can destroy pitch clarity.

Start with **(1)** for audibility and then add **(2)** as soon as possible if artefacts are strong. (Reference code below includes phase-coherent STFT examples.)

### 3.3 Interpolation / “evolution time” (DroneMaker feel)

To move towards DroneMaker’s slow evolution rather than “instant freeze”:

* Introduce a parameter: `evolutionTimeSeconds`
* Instead of instantly updating `heldMag[k]`, smoothly interpolate:

  * `heldMag[k] = lerp(heldMag[k], targetMag[k], alpha)`
    where `alpha` is derived from hop rate and desired time constant.

This is crucial: it changes the effect from “glitchy hold” to “slowly swelling drone”.

### 3.4 Additional DroneMaker-adjacent modules (later phases)

Keep these as modular add-ons (off by default initially):

* **Bin gating** (threshold + zeroing) as above ([Scribd][2])
* **Hold duration / hold variance** (from “Spectral Gate and Hold”) ([Scribd][2])
* **Comb/filterbank-like resonation** (if aligning with DroneMaker’s distinctive tone; implement as a post stage after ISTFT or as a spectral weighting curve)

---

## 4) Non-goals (for the first “working clone”)

* No GUI beyond minimal device selection + a few sliders/text readouts.
* No preset system.
* No plugin formats yet (AU/VST). (App first.)
* No “perfect SoundMagic parity” initially—aim for the **core perceptual behaviour**.

---

## 5) Real-time constraints

* **No allocations on the audio thread**
* All FFT buffers preallocated on `audioDeviceAboutToStart`
* Use lock-free FIFOs/ring buffers between audio callback and STFT processing if processing is moved off-thread
* Start single-threaded for correctness; move STFT work to a background thread only once stable

---

## 6) JUCE “getting started” lessons (what we learned the hard way)

### 6.1 Minimal audio app structure that actually works

* Use `AudioDeviceManager` + `AudioDeviceSelectorComponent` for choosing devices (input/output, sample rate, buffer size).
* Use `AudioIODeviceCallbackWithContext` and **always clear output buffers first** to avoid persistent tones.
* Mix input to mono (or implement per-channel later), and copy to all output channels for quick sanity checks.
* If the app “plays a tone forever” after a test sound, it’s often because the output buffer wasn’t cleared fully or a generator path wasn’t gated correctly.

### 6.2 macOS microphone permission loop

* Ensure the app bundle has microphone usage description keys and that signing/hardened runtime settings are coherent. (We saw repeated prompts until settings were consistent.)
* Verify permissions in System Settings → Privacy & Security → Microphone.

### 6.3 Framework integration on macOS (the Bungee saga, distilled)

We hit three classic failure modes and solved them:

1. **Header not found**

   * Framework headers are not automatically visible.
   * You must add a header search path that points at the framework’s Headers directory (or otherwise ensure include paths resolve).

2. **Build succeeds but app crashes at launch with dyld “Library not loaded @rpath/…”**

   * The framework must be embedded into the app bundle (`Contents/Frameworks`) and signed.
   * `@rpath` must include the correct runtime search path. The reliable fix was adding a Runpath Search Path like:

     * `@executable_path/../Frameworks`
   * When misconfigured, dyld tries to load the framework from build folders and fails.

3. **Intermittent breakage after cleaning**

   * Cleaning can wipe derived build artefacts; if the embed/sign/copy steps aren’t fully correct, the fresh build will regress.
   * Treat “Embed & Sign” + correct Runpath Search Paths as mandatory, not optional.

Even though we’re moving away from Bungee for the bin-level build, these macOS linking lessons are still useful for any third-party DSP libs.

### 6.4 “No global header file was included!” / AppConfig confusion

* JUCE expects project configuration headers / defines to be consistent. When macros or include ordering get broken, JUCE can throw errors like `"No global header file was included!"`.
* Fixing this required restoring sane Projucer/Xcode exporter defines and not polluting preprocessor macros with malformed entries.

---

## 7) Reference code to include in the “example code” folder (with why it matters)

### 7.1 JUCE + spectral processing (drone-adjacent)

* **PaulXStretch (JUCE + FFTW3)** — extreme time-stretch/drone family; great reference for long-window spectral workflows and practical engineering decisions. ([GitHub][3])
* **Epigene (JUCE spectral filter)** — explicitly “bin-wise attenuation” with overlap-add; clean small example of bins as first-class citizens in JUCE. ([GitHub][4])

### 7.2 Spectral freeze / morph concepts (very relevant)

* **HISSTools Freeze** — spectral freezing/morphing modes; useful conceptual and DSP reference even if not JUCE. ([GitHub][5])
* **FrameLib** — a framework built specifically for frame-based DSP, including variable frame timing; can inform architecture even if not adopted directly. ([GitHub][6])

### 7.3 Phase-aware STFT pipelines (bin processing that doesn’t sound broken)

* **jurihock/stftPitchShift** — includes a real-time STFT implementation plus a “core module” meant to embed into other pipelines. Good reference for phase handling and stable realtime processing. ([GitHub][7])
* **jurihock/stftPitchShiftPlugin (JUCE wrapper)** — shows how to wire that kind of pipeline into JUCE. ([GitHub][8])

### 7.4 Official/primary descriptions of the target suite

* **SoundMagic Spectral suite page** (effect list including Spectral DroneMaker) ([michaelnorris.info][1])
* **Spectral Freezing description** (how Norris describes “windows stored and replayed in random walk”) — not DroneMaker, but a core conceptual building block. ([michaelnorris.info][9])
* **Manual excerpt covering spectral accumulation + freeze factor + threshold + zeroing behaviour** (key per-bin spec cues). ([Scribd][2])
* **SoundMagic Spectral Manual intro** (general spectral-processing context) ([Scribd][10])

### 7.5 “Grab bag” map of other JUCE OSS

* **awesome-juce** — good directory for other JUCE DSP modules/examples you may want later. ([GitHub][11])

---

## 8) Implementation plan (what the coding LLM should do first)

### Phase A — Infrastructure (already done once; do again cleanly)

1. New JUCE Audio App project.
2. Device selector UI.
3. Verified passthrough (input audible on output).
4. Basic meters/logging for sanity.

### Phase B — STFT engine

1. Implement analysis/synthesis with overlap-add and ring buffers.
2. Confirm “identity” transform works:

   * FFT → IFFT without modification should sound like passthrough (minus latency).
3. Add a debug mode:

   * output only a single bin or band-limited spectrum (proves bins are correctly addressed).

### Phase C — DroneMaker core bins

1. Add per-bin `heldMag[]` (and chosen phase strategy).
2. Implement peak-hold + decay + threshold options as per spec cues. ([Scribd][2])
3. Add “evolutionTime” interpolation and verify it reduces chattering.

### Phase D — DroneMaker flavour

1. Add random-walk windowing ideas (from Spectral Freezing) as an optional variant. ([michaelnorris.info][9])
2. Add gate-and-hold style timed holds (optional). ([Scribd][2])
3. Add a resonant/filterbank tone-shaping stage if needed.

---

## 9) Verification checklist (acceptance tests)

* **No crashes** when changing buffer size/sample rate/devices.
* Identity STFT mode sounds like clean passthrough.
* Peak-hold mode:

  * sustained tones accumulate and persist with controllable decay.
  * threshold gating behaves predictably.
* CPU stable at moderate FFT sizes (start small, scale up).
* No allocations on audio thread (verify with instrumentation if possible).

