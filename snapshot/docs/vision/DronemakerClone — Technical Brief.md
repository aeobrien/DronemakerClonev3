# DronemakerClone — Technical Brief

## Technology Stack

**C++17** — the application language. Chosen because JUCE is a C++ framework and real-time audio processing demands the performance characteristics C++ provides. No alternative was seriously considered.

**JUCE** — the application framework, providing audio I/O, DSP primitives, GUI, and MIDI handling. JUCE is the standard framework for this kind of audio application and supports standalone builds on Linux (including Raspberry Pi). The project uses the following JUCE modules: juce_core, juce_audio_basics, juce_audio_devices, juce_audio_formats, juce_audio_processors, juce_audio_utils, juce_dsp, juce_gui_basics, juce_gui_extra, juce_graphics, juce_data_structures, juce_events.

**juce::dsp::FFT and juce::dsp::WindowingFunction** — used for the STFT analysis/resynthesis pipeline at the heart of the spectral drone engine. This is explicit bin-level processing using Hann windowing, not a black-box library.

**Raspberry Pi 5** — the target platform for the standalone instrument. Runs Linux (Raspberry Pi OS). Chosen for its combination of processing power, GPIO availability, small form factor, and broad community support.

**Teensy LC or Teensy 4.0** — microcontroller for reading rotary encoders and sending MIDI CC messages to the Pi over USB. Chosen for native USB MIDI support, small footprint, ample pin count, and well-documented encoder handling.

**Waveshare 7" LCD Touchscreen** — display and touch input. Connects via HDMI for display and micro USB for touch. Touch events appear as standard mouse input at the OS level, so JUCE's existing input handling should work without special adaptation.

**Focusrite Scarlett Solo** — USB audio interface for version one, providing mono input and stereo output. Chosen as a pragmatic starting point that avoids the complexity of building a custom audio input stage with variable impedance handling.

**Xcode** — current build system (macOS development). Will need to transition to a CMake or Makefile-based build for cross-compilation or native compilation on the Pi.

## Architecture

The application follows a standard JUCE AudioAppComponent pattern with a linear signal flow:

Live Audio Input → FFT Analysis → Spectral Processing (Drone Engine) → IFFT Resynthesis → Loop Recorder (multi-layer) → Effects Chain (6 effects) → Modulation System (LFOs → parameters) → Audio Output

**FFTProcessor** — the core of the instrument. Per-channel STFT engine that analyses incoming audio, holds and accumulates spectral bin magnitudes over a rolling time window, and resynthesises the result. This is what generates the drone character from arbitrary input. The inherent build-up time of this process (several seconds for spectral changes to fully manifest) is a defining characteristic of the instrument's sound and behaviour.

**LoopRecorder** — multi-layer loop capture with configurable maximum length per loop. Loops are defined by duration, not musical bars. Each loop's output feeds back into the drone engine, creating evolving, layered soundscapes. The LoopSequenceExecutor handles sample-accurate automation playback using atomics for thread-safe communication between audio and UI threads.

**EffectsChain** — owns and routes audio through six effects in series, all positioned after the drone engine: FilterEffect, DelayEffect, GranularEffect, TremoloEffect, DistortionEffect, TapeEffect. All effects inherit from EffectBase, which provides SmoothedParam (one-pole lowpass) for click-free parameter changes.

**ModulationManager** — routes LFO sources to effect parameters. Each LFO can target any parameter on any effect, enabling automated parameter movement during performance.

**MainComponent** — the central JUCE component that owns all of the above and manages both audio processing and the GUI. This is a monolithic component that handles a lot — a potential candidate for refactoring as the project grows, but functional for now.

### Hardware Integration Layer (Version One)

The hardware integration adds three external connections to the Pi:

- **USB Audio** (Scarlett Solo) — JUCE's audio device management handles this via ALSA on Linux
- **USB Touch** (Waveshare) — appears as a standard input device; JUCE handles touch-as-mouse natively
- **USB MIDI** (Teensy with encoders) — JUCE's MidiInput class receives CC messages; the application maps CCs to parameters

All three connections are USB, which keeps the integration simple and modular. The Pi sees standard device classes (audio, HID, MIDI) with no custom drivers required.

## Data Model

This application processes audio in real time and does not persist data between sessions. There is no database, no file storage, and no save/load system.

**Runtime data:**

- **FFT bin arrays** — per-channel arrays of magnitude and phase values, sized to the FFT window (the specific FFT size is set in FFTProcessor). These are the working data of the drone engine, updated every hop.
- **Loop buffers** — per-loop audio buffers storing recorded samples. Maximum length is configurable. Each loop tracks its record/playback position, state (recording, playing, stopped), and automation sequence.
- **Automation sequences** — per-loop command lists (stored as std::variant) defining post-record behaviour, with timing information for sample-accurate execution.
- **Effect parameters** — each effect maintains its own parameter set as SmoothedParam instances. Current values live in memory only.
- **Modulation routings** — LFO-to-parameter assignments maintained by ModulationManager. These define which parameters are being modulated, by which source, at what depth and rate.

**Thread safety model:** Audio thread and UI thread communicate via std::atomic values. The audio thread never allocates memory or blocks. UI reads of audio-thread state (loop positions, levels, etc.) go through atomic loads.

## Key Decisions

**Bin-level STFT rather than a library-based approach.** The project initially explored using the Bungee streaming API but pivoted to explicit FFT bin manipulation. This gives full control over the spectral processing behaviour — which bins to hold, how magnitudes accumulate, how phase is handled — and is essential for accurately reproducing and extending the DroneMaker algorithm.

**Standalone application, not a plugin.** The entire motivation is independence from the DAW. Building as a JUCE AudioAppComponent rather than an AudioProcessor means the application owns its own audio device management and lifecycle, which is what's needed for a standalone hardware instrument.

**Effects chain after the drone engine (for now).** Currently all six effects process the drone engine's output. The planned repositionable drone maker feature would allow the engine to sit at any point in the chain, but the current fixed ordering is simpler to implement and reason about.

**USB MIDI for encoder communication.** Rather than reading encoders directly from Pi GPIO (which introduces jitter from Linux userspace), a Teensy microcontroller handles encoder reading and debouncing, sending MIDI CC messages over USB. This is modular (the encoder board is independent of the Pi), leverages JUCE's built-in MIDI handling, and naturally extends to supporting external MIDI controllers.

**External USB audio interface for version one.** Building a custom audio input stage with appropriate impedance handling for microphones, instrument pickups, and line sources is a significant hardware challenge. Using a Scarlett Solo defers this complexity while providing reliable, low-latency audio I/O.

**No state persistence.** The instrument starts fresh each time. There is no preset system, no session save/load, no loop export. This is consistent with its identity as a live performance instrument — you build a soundscape in the moment, and when you're done, it's gone.

## Integration Points

**ALSA (via JUCE)** — audio device management on Linux. JUCE abstracts this, but the underlying connection to the Scarlett Solo goes through ALSA. JACK is an alternative if lower latency or more flexible routing is needed, and JUCE supports both.

**USB MIDI (via JUCE)** — receives CC messages from the Teensy encoder board. The same MIDI input path would handle external MIDI controllers, making MIDI mapping a natural extension.

**Linux input subsystem** — the Waveshare touchscreen's USB touch interface appears as a standard HID device. No direct integration needed; the OS and JUCE handle this.

**No network connections, no cloud services, no external APIs.** This is a fully self-contained instrument.

## Constraints

**Real-time audio processing on the Pi 5.** The STFT engine performs FFT analysis and resynthesis per audio callback, per channel. The Pi 5 is substantially more powerful than its predecessors, but real-time spectral processing with six additional effects and multiple loops is a non-trivial workload. Latency budget and CPU headroom are unknown until tested.

**Single-core audio thread.** JUCE's audio callback runs on a single thread. All processing — FFT, loop playback, effects chain, modulation — must complete within the audio buffer period. At 44.1kHz with a 512-sample buffer, that's roughly 11.6ms per callback.

**7-inch touchscreen resolution.** The current UI was designed for a desktop display. It will need adaptation to be usable at the Waveshare's resolution (likely 1024x600 or 800x480) and physical size. Touch targets need to be large enough for finger interaction rather than mouse clicks.

**Linux build toolchain.** The project currently builds via Xcode on macOS. Targeting the Pi requires either cross-compilation or native compilation on the Pi itself, and a corresponding build system (CMake or Makefile). JUCE supports this but it needs to be set up.

**Encoder resolution.** MIDI CC values are 0-127, which provides 128 steps of resolution per parameter. For most effect parameters this is adequate, but for parameters requiring fine control (filter cutoff frequency across a wide range, for example), 14-bit MIDI CC or a custom scaling approach may be needed.

**Enclosure design.** No experience in enclosure design or fabrication. The physical housing needs to accommodate the Pi, touchscreen, nine encoders, and USB cable routing, but how to design and build this is an open question requiring external guidance or learning.

## Implementation Order

The primary goal is version one: a physical instrument that works. Software enhancements are a parallel track that doesn't gate the hardware build.

**Track 1 — Hardware Build (critical path to version one)**

1. **Get the application running on the Pi 5.** Compile JUCE on the Pi (or cross-compile), verify audio I/O works with the Scarlett Solo, confirm basic functionality. This is the single most important validation step — if the app can't run acceptably on the Pi, everything else changes.
2. **Adapt the UI for the touchscreen.** Resize and reorganise the interface for 7-inch display at the Waveshare's native resolution. Ensure touch targets are usable.
3. **Build the encoder board.** Wire nine rotary encoders to a Teensy, program it to send MIDI CCs over USB. Test with the JUCE app on the Pi.
4. **Implement MIDI CC mapping in the application.** Receive MIDI input and route CCs to effect parameters (eight encoders for active effect) and the selector knob behaviour.
5. **Design and build the enclosure.** House the Pi, screen, encoder board, and cable routing in a functional enclosure.
6. **Integration and testing.** Verify everything works together as a playable instrument.

**Track 2 — Software Enhancements (ongoing, not gating version one)**

These can be pursued in any order based on creative priority and energy, but a logical sequence would be:

1. **Gain compensation** — relatively contained change, high usability impact
2. **Clickless looping** — important for the loops-you-can-hear use case
3. **Per-loop dry/wet control** — extends the looping system's expressiveness
4. **Repositionable drone maker** — significant architectural change to the signal flow
5. **Offset playback synchronisation** — complex feature, depends on solid looping fundamentals
6. **MIDI mapping UI** — if the MIDI CC infrastructure is built for the encoders, exposing user-configurable mapping is a natural extension

## Risks and Uncertainties

**Performance on the Pi 5.** This is the biggest unknown. Real-time STFT processing with multiple loops and six effects is computationally demanding. The Pi 5's quad-core Cortex-A76 is capable, but the single-threaded audio callback constraint means only one core does the heavy lifting. If performance is insufficient, options include reducing FFT size (trading spectral resolution for CPU), reducing the maximum number of simultaneous loops, or optimising the effects chain. This must be validated early — it's step one of the hardware track for a reason.

**UI adaptation effort.** The current UI was built for desktop. The scope of changes needed to make it usable on a 7-inch touchscreen is unknown. It could be minor layout adjustments or it could require significant rethinking of how information is displayed and how interaction works. This won't be clear until the app is actually running on the screen.

**Linux audio configuration.** Getting reliable low-latency audio on Linux can involve configuration challenges — ALSA vs JACK, buffer sizes, real-time scheduling permissions, USB audio device quirks. This is well-documented territory but can be fiddly.

**Enclosure design and fabrication.** This is a completely new domain. Options range from a simple off-the-shelf project box with hand-cut holes to 3D printing to laser-cut panels. The user has no experience here and will need to learn or seek guidance. This is unlikely to be technically blocking but could be a significant source of decision paralysis.

**Encoder feel and usability.** The choice of specific encoders (detented vs smooth, size, torque) will affect how the instrument feels to play. This is hard to evaluate without hands-on testing, and getting it wrong means resoldering and potentially redesigning the panel.

**Touch coordinate mapping.** While the Waveshare screen should work out of the box, there's a possibility of touch calibration issues — offset between where you touch and where the OS registers the input. Usually solvable but potentially annoying to debug.

**Build system transition.** Moving from Xcode to a Linux-compatible build system is a known task but could surface unexpected issues with JUCE module dependencies or platform-specific code. The codebase includes .mm (Objective-C++) files for macOS, which will need platform-conditional handling.