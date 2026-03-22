# Future Hardware Upgrade Paths: Multi-Drone Processing

## Current Architecture (Single Drone, Raspberry Pi 5)

The drone maker's FFT analysis and processing is the most resource-intensive part of the instrument. Currently, all eight loops are summed via `getLoopMix()` and fed into a single stereo drone maker instance to keep the workload manageable on a Raspberry Pi 5.

### Processing Pipeline Summary

| Parameter | Value |
|-----------|-------|
| FFT Size | 32,768 points |
| Hop Size | 8,192 samples (75% overlap) |
| Frequency Bins | 16,385 per frame |
| History Buffer | 56 frames (~10 seconds of spectral memory) |
| Stereo | 2 independent FFTProcessor instances (L + R) |
| Frame Rate | ~5.4 frames/sec at 44.1 kHz (one hop every ~186ms) |
| Algorithmic Latency | ~742ms (32,768 samples at 44.1 kHz) |

### Per-Frame Computation (Every ~186ms)

Each stereo drone pair requires per hop:

- **2x forward FFTs** (32,768-point)
- **2x inverse FFTs** (32,768-point)
- **2x spectral processing** across 16,385 bins:
  - Peak-tracking across up to 56 history frames per bin (~918K comparisons per processor)
  - EMA smoothing, decay, threshold gating, harmonic comb filtering, phase randomization
- **2x Hann window multiplication** (analysis + synthesis)
- **2x overlap-add** reconstruction

### Current Memory Usage (Single Stereo Drone)

| Buffer | Size |
|--------|------|
| inputFifo (x2) | 256 KB |
| outputFifo (x2) | 256 KB |
| fftData (x2) | 512 KB |
| magHistory (x2) | 7.3 MB |
| smoothedMag (x2) | 128 KB |
| **Total** | **~8.4 MB** |

### Current CPU Estimate (Pi 5)

On the Pi 5 (4x Cortex-A76 @ 2.4 GHz), the single stereo drone pair uses an estimated 15-25% of one core, with the remaining budget consumed by loop playback, effects chain, UI, and OS overhead.

---

## Scaled Architecture: 8 Independent Drones

Instead of summing 8 loops into one drone, each loop would feed its own stereo drone maker instance. This allows mixing the actual tonal output of 8 separate drones rather than just controlling loop volumes into a single shared drone.

### Resource Requirements

#### Memory

| Config | FFT Buffers | History Arrays | Total |
|--------|------------|----------------|-------|
| Current (1 drone, stereo) | ~1 MB | ~7.3 MB | ~8.4 MB |
| 8 drones, stereo | ~8 MB | ~58.5 MB | ~67 MB |

67 MB is trivial for any modern system with 4+ GB RAM.

#### CPU

Scaling to 8 stereo drones means:

- **32 large FFTs** (16 forward + 16 inverse) per hop (~186ms)
- **~14.7 million peak comparisons** per hop (16 processors x 16,385 bins x 56 history frames)
- Roughly **8x the current FFT workload**, requiring ~120-200% of a single Pi 5 core

#### Threading Implications

The current code processes both FFT instances sequentially in the audio callback. Scaling to 8 drones **requires** parallelising the FFT work:

- Run each drone's `processFrame()` on separate threads or via a thread pool
- Use lock-free design to feed samples in and mix results out
- The hop-based nature helps — processing only triggers every 8,192 samples, so drones can be staggered or batch-processed across cores

---

## Platform Assessment

| Platform | 8 Drones Viable? | Confidence |
|----------|-------------------|------------|
| Raspberry Pi 5 | No (not reliably real-time) | High |
| Intel N100 mini PC / SBC | Probably, with careful threading | Medium |
| Ryzen 5 / i5 mini PC or SBC | Yes, comfortably | High |
| Ryzen 7 / i7 mini PC or SBC | Yes, with plenty of headroom | High |
| Jetson Orin Nano (GPU FFT) | Yes, with code rewrite to cuFFT | High |

### Optimisation Tradeoffs

If a lower-powered platform is preferred, the FFT size could be reduced to 16,384 points:

- Halves compute per frame
- Coarser frequency resolution (2.7 Hz/bin instead of 1.35 Hz/bin)
- Reduces history buffer memory proportionally
- May slightly affect drone texture quality

---

## Hardware Options

### Option A: Mini PC (Easiest, Least Integration)

A standalone mini PC in its own chassis, connected via USB to the audio interface. Simple but bulky for a custom enclosure.

| Tier | Example | CPU | Price | Notes |
|------|---------|-----|-------|-------|
| Budget | Beelink Mini S12 Pro | Intel N100 (4C/4T) | ~$150 | Tight for 8 drones |
| Comfortable | Beelink SER5 / Minisforum UM560 | Ryzen 5 5560U (6C/12T) | $250-400 | Good headroom |
| Overkill | Higher-end Minisforum | Ryzen 7 / i7 (8C/16T) | $400-600 | Future-proof |

### Option B: Single Board Computer (Better for Enclosures)

Bare boards that mount with standoffs inside a custom enclosure.

| Board | CPU | Price | Notes |
|-------|-----|-------|-------|
| Lattepanda 3 Delta | Intel N100 | ~$200 | Pi-sized, marginal for 8 drones |
| UDOO Vision X5 | Intel Celeron | ~$200 | Arduino-compatible headers |
| Minisforum EM680 | Ryzen 7 6800U (8C/16T) | ~$350 | 60x90mm board, serious power |

### Option C: Compute-on-Module (Best for Custom Enclosures)

COM boards are the x86 equivalent of a Raspberry Pi Compute Module — a CPU module that plugs into a carrier board. You design or buy a carrier with only the I/O you need.

| Module | CPU | Price | Notes |
|--------|-----|-------|-------|
| Lattepanda Mu | Intel N100 | ~$110-170 | SO-DIMM form factor, closest Pi CM equivalent. Marginal for 8 drones |
| Lattepanda Sigma | Intel i5-1340P (12C) | ~$500 | COM form factor, 12 cores, handles 8 drones easily |

### Option D: ARM with GPU Acceleration (Highest Effort)

| Board | CPU / GPU | Price | Notes |
|-------|-----------|-------|-------|
| Jetson Orin Nano | 6-core A78AE + 1024-core GPU | ~$250 | Requires rewriting FFT pipeline to CUDA/cuFFT. Batch FFTs are exactly what GPUs excel at. High effort, highest efficiency |
| RK3588 boards (Khadas VIM4, Orange Pi 5 Plus) | 4x A76 + 4x A55 | ~$100-150 | ~2x Pi 5 multi-threaded. Might handle 4-5 drones, probably not 8 |
| Raspberry Pi Compute Module 5 | Same as Pi 5 | ~$45 | Solves form factor, not compute. Still can't do 8 drones |

---

## Recommendations

### Best Overall (Custom Enclosure)

**Lattepanda Sigma** (~$500) — COM module with i5-1340P (12 cores). Carrier board flexibility like a Pi Compute Module but with the x86 processing power to handle 8 stereo drones comfortably. Design a carrier board with only power, USB (for audio interface), and optionally a display connector.

### Best Value

**Ryzen 5/7 SBC** such as the Minisforum EM680 (~$350) — compact enough to mount inside an enclosure, powerful enough to run 8 drones with 50%+ CPU headroom remaining. 60x90mm board with standoff mounting.

### Budget Option

**Lattepanda Mu** (~$130) with a reduced FFT size (16,384 points) and careful multi-threaded implementation. The N100's 4 cores at ~3.4 GHz boost should handle the reduced workload, and the COM form factor integrates cleanly into a custom enclosure.

### Maximum Future-Proofing

**Jetson Orin Nano** (~$250) with a CUDA-based FFT pipeline. This requires significant code changes (replacing JUCE's CPU FFT with cuFFT, managing GPU memory) but would leave the CPU almost entirely free and could scale to far more than 8 drones. Only worthwhile if further scaling is anticipated.
