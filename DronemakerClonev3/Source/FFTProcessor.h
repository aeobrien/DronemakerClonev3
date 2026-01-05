#pragma once

#include <JuceHeader.h>

/**
  STFT analysis and resynthesis for DroneMaker effect.

  Each channel should have its own FFTProcessor.
 */
class FFTProcessor
{
public:
    FFTProcessor();

    int getLatencyInSamples() const { return fftSize; }

    void reset();
    void setSampleRate(float sr) { sampleRate = sr; }
    float processSample(float sample, bool bypassed);

    // Parameter setters for UI control
    void setInterpFactor(float f) { interpFactor = juce::jlimit(0.0f, 1.0f, f); }
    void setDroneDelay(float d) { droneDelay = juce::jlimit(0.0f, 1.0f, d); }
    void setSmoothingFactor(float f) { smoothingFactor = juce::jlimit(0.01f, 0.5f, f); }
    void setThreshold(float t) { threshold = juce::jlimit(0.0f, 0.1f, t); }
    void setSpectralTilt(float dbPerOctave) { spectralTilt = juce::jlimit(-6.0f, 6.0f, dbPerOctave); }
    void setRandomizePhases(bool b) { randomizePhases = b; }
    void setUsePeakAmplitudes(bool b) { usePeakAmplitudes = b; }

    // New parameters
    void setPitchShiftSemitones(float semitones) { pitchShiftSemitones = semitones; updatePitchRatio(); }
    void setPitchShiftOctaves(float octaves) { pitchShiftOctaves = octaves; updatePitchRatio(); }
    void setHighPassFreq(float hz) { highPassFreq = juce::jlimit(0.0f, 20000.0f, hz); }
    void setLowPassFreq(float hz) { lowPassFreq = juce::jlimit(20.0f, 20000.0f, hz); }
    void setHighPassSlope(float poles) { highPassSlope = juce::jlimit(1.0f, 8.0f, poles); }
    void setLowPassSlope(float poles) { lowPassSlope = juce::jlimit(1.0f, 8.0f, poles); }
    void setDecayRate(float r) { decayRate = juce::jlimit(0.9f, 1.0f, r); }
    void setHistorySeconds(float sec);

    // Harmonic comb filter
    void setHarmonicFilterEnabled(bool b) { harmonicFilterEnabled = b; }
    void setHarmonicRootNote(int note) { harmonicRootNote = juce::jlimit(0, 11, note); }
    void setHarmonicScaleType(int type) { harmonicScaleType = juce::jlimit(0, 8, type); }
    void setHarmonicIntensity(float i) { harmonicIntensity = juce::jlimit(0.0f, 1.0f, i); }

private:
    void processFrame(bool bypassed);
    void processSpectrum(float* data, int numBins);
    void updatePitchRatio() { pitchShiftRatio = std::pow(2.0f, (pitchShiftSemitones + pitchShiftOctaves * 12.0f) / 12.0f); }
    bool isFrequencyInScale(float freq) const;

    // FFT configuration: 2^15 = 32768 points, 16385 bins
    static constexpr int fftOrder = 15;
    static constexpr int fftSize = 1 << fftOrder;      // 32768 samples
    static constexpr int numBins = fftSize / 2 + 1;    // 16385 bins
    static constexpr int overlap = 4;                  // 75% overlap
    static constexpr int hopSize = fftSize / overlap;  // 8192 samples

    // Gain correction for Hann window with 75% overlap
    static constexpr float windowCorrection = 2.0f / 3.0f;

    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;

    // Counts up until the next hop
    int count = 0;

    // Write position in input FIFO and read position in output FIFO
    int pos = 0;

    // Circular buffers for incoming and outgoing audio data
    std::array<float, fftSize> inputFifo;
    std::array<float, fftSize> outputFifo;

    // FFT working space - contains interleaved complex numbers [re, im, re, im, ...]
    std::array<float, fftSize * 2> fftData;

    // -------- DroneMaker v2: Spectral History --------
    // Number of frames to store (~10 seconds at 44.1kHz with hopSize 8192)
    // 10.0 * 44100 / 8192 ≈ 54 frames, using 56 for margin
    static constexpr int historyFrames = 56;

    // Ring buffer: [frame][bin] magnitudes
    std::array<std::array<float, numBins>, historyFrames> magHistory;
    int historyWritePos = 0;

    // EMA smoothing state (per-bin smoothed magnitudes)
    std::array<float, numBins> smoothedMag;
    float smoothingFactor = 0.05f;  // Lower = smoother (0.01-0.5 range)

    // Interpolation factor: 0.0 = fully old (droney), 1.0 = fully new (dry)
    float interpFactor = 0.2f;

    // Drone delay: 0.0 = blend mode (responsive), 1.0 = pure delay mode (true DroneMaker)
    float droneDelay = 0.0f;

    // Sample rate (needed for future calculations)
    float sampleRate = 44100.0f;

    // Threshold below which bins are zeroed (noise gate)
    float threshold = 0.001f;

    // Spectral tilt: frequency-dependent threshold adjustment (dB/octave)
    // Negative = high frequencies decay faster, Positive = low frequencies decay faster
    float spectralTilt = 0.0f;

    // Phase randomization for ethereal quality
    juce::Random random;
    bool randomizePhases = true;

    // Peak tracking: use peak magnitude in window instead of oldest
    bool usePeakAmplitudes = true;

    // Pitch shifting
    float pitchShiftSemitones = 0.0f;
    float pitchShiftOctaves = 0.0f;
    float pitchShiftRatio = 1.0f;  // Computed from semitones + octaves

    // Spectral filtering
    float highPassFreq = 0.0f;      // Hz (0 = disabled)
    float lowPassFreq = 20000.0f;   // Hz
    float highPassSlope = 1.0f;     // Filter steepness (1-8 poles)
    float lowPassSlope = 1.0f;

    // Decay rate applied to smoothed magnitudes (1.0 = no decay, 0.99 = fast decay)
    float decayRate = 1.0f;

    // Active history length (can be less than historyFrames)
    int activeHistoryFrames = historyFrames;

    // Harmonic comb filter
    bool harmonicFilterEnabled = false;
    int harmonicRootNote = 0;   // 0=C, 1=C#, 2=D, ..., 11=B
    int harmonicScaleType = 0;  // 0=Octaves, 1=Fifths, 2=Ionian, 3=Dorian, etc.
    float harmonicIntensity = 1.0f;  // 0 = no filtering, 1 = full filtering

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FFTProcessor)
};
