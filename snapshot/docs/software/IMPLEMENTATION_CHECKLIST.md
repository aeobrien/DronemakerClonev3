# DronemakerClonev3 Implementation Checklist

## Overview
Real-time spectral drone processor cloning Michael Norris' "Spectral DroneMaker".

---

## Phase B: STFT Engine

### Step 1: Create FFTProcessor Skeleton
- [ ] Create `Source/FFTProcessor.h` with constexpr parameters (fftOrder=11, 2048 samples)
- [ ] Create `Source/FFTProcessor.cpp` with stub processSample() returning input unchanged
- [ ] Add files to .jucer project
- **Test**: Build succeeds, audio passes through unchanged

### Step 2: Integrate into MainComponent
- [ ] Include FFTProcessor.h, add member variable
- [ ] Call reset() in audioDeviceAboutToStart()
- [ ] Replace passthrough with processSample() loop
- **Test**: Audio passes through, no clicks/pops

### Step 3: Input Ring Buffer
- [ ] Implement sample accumulation in processSample()
- [ ] Trigger processFrame() when count reaches hopSize
- **Test**: processFrame() fires every 512 samples

### Step 4: Windowing + Overlap-Add (No FFT)
- [ ] Copy from circular inputFifo to linear fftData
- [ ] Apply Hann window (analysis + synthesis)
- [ ] Overlap-add into outputFifo
- **Test**: Audio passes through with correct amplitude

### Step 5: Forward FFT
- [ ] Add fft.performRealOnlyForwardTransform()
- **Test**: Audio silent/distorted (expected)

### Step 6: Inverse FFT - IDENTITY COMPLETE
- [ ] Add fft.performRealOnlyInverseTransform()
- **Test**: CRITICAL - Perfect passthrough reconstruction

### Step 7: processSpectrum() Hook
- [ ] Extract magnitude/phase using std::abs/std::arg
- [ ] Reconstruct with std::polar
- **Test**: Audio unchanged (identity)

### Step 8: Spectral Test Effect
- [ ] Implement simple low-pass (zero bins >1kHz)
- **Test**: Audio muffled, then revert to identity

---

## Phase C: DroneMaker Core

### Step 9: Add Per-Bin State Arrays
- [ ] Add heldMag[], heldPhase[] arrays
- [ ] Add decayRate, threshold parameters
- **Test**: Compiles, no audio change

### Step 10: Peak-Hold Logic
- [ ] Capture magnitude/phase when input exceeds held
- [ ] Output held values
- **Test**: Sounds freeze and sustain indefinitely

### Step 11: Decay Logic
- [ ] Apply heldMag[i] *= decayRate each frame
- **Test**: Sounds fade out slowly

### Step 12: Threshold Gating
- [ ] Only capture above threshold
- [ ] Zero output below threshold
- **Test**: Silent input = silent output, clean fade

### Step 13: Phase Evolution
- [ ] Calculate per-bin phase increment
- [ ] Accumulate and wrap phase
- **Test**: Natural-sounding drone

---

## Phase D: Polish

### Step 14: Basic UI Sliders
- [ ] Decay slider (0.9 - 0.9999)
- [ ] Threshold slider (0.001 - 0.1)
- [ ] Dry/Wet mix slider (0.0 - 1.0)
- **Test**: Real-time control works

---

## Files

**Create:**
- `Source/FFTProcessor.h`
- `Source/FFTProcessor.cpp`

**Modify:**
- `Source/MainComponent.h`
- `Source/MainComponent.cpp`
- `DronemakerClonev3.jucer`

**Reference:**
- `Example Code/fft-juce-main/Source/FFTProcessor.h/.cpp`
