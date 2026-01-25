#pragma once

#include "EffectBase.h"
#include <array>

/**
 * High-pass and Low-pass filter with variable poles (1-8).
 * Also includes harmonic/scale filtering for musical applications.
 */
class FilterEffect : public EffectBase
{
public:
    FilterEffect();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void processSample (float& left, float& right) override;
    void reset() override;
    juce::String getName() const override { return "Filter"; }

    // HP/LP filter parameters
    void setHighPassFreq (float hz) { hpFreq = juce::jlimit (20.0f, 5000.0f, hz); }
    void setLowPassFreq (float hz) { lpFreq = juce::jlimit (200.0f, 20000.0f, hz); }
    void setHighPassPoles (int poles) { hpPoles = juce::jlimit (1, maxPoles, poles); }
    void setLowPassPoles (int poles) { lpPoles = juce::jlimit (1, maxPoles, poles); }

    float getHighPassFreq() const { return hpFreq; }
    float getLowPassFreq() const { return lpFreq; }
    int getHighPassPoles() const { return hpPoles; }
    int getLowPassPoles() const { return lpPoles; }

    // Harmonic filter parameters
    void setHarmonicEnabled (bool b) { harmonicEnabled = b; }
    void setRootNote (int note) { rootNote = juce::jlimit (0, 11, note); updateHarmonicFrequencies(); }
    void setScaleType (int type) { scaleType = juce::jlimit (0, 10, type); updateHarmonicFrequencies(); }
    void setHarmonicIntensity (float i) { harmonicIntensity = juce::jlimit (0.0f, 1.0f, i); }

    bool isHarmonicEnabled() const { return harmonicEnabled; }
    int getRootNote() const { return rootNote; }
    int getScaleType() const { return scaleType; }
    float getHarmonicIntensity() const { return harmonicIntensity; }

private:
    // HP/LP filter
    static constexpr int maxPoles = 8;
    std::array<float, maxPoles> hpStateL {}, hpStateR {};
    std::array<float, maxPoles> lpStateL {}, lpStateR {};

    float hpFreq = 20.0f;
    float lpFreq = 20000.0f;
    int hpPoles = 1;
    int lpPoles = 1;

    // Harmonic filter (comb filter based on scale frequencies)
    bool harmonicEnabled = false;
    int rootNote = 0;         // 0=C, 1=C#, ..., 11=B
    int scaleType = 0;        // 0=Octaves, 1=Fifths, 2=Ionian, etc.
    float harmonicIntensity = 1.0f;

    // Pre-computed allowed frequencies for harmonic filter
    static constexpr int maxHarmonicFreqs = 128;
    std::array<float, maxHarmonicFreqs> harmonicFreqs {};
    int numHarmonicFreqs = 0;

    // Harmonic filter state (biquad bandpass for each allowed frequency)
    static constexpr int maxResonators = 32;
    struct Resonator
    {
        float freq = 440.0f;
        // Biquad state
        float z1L = 0.0f, z2L = 0.0f;
        float z1R = 0.0f, z2R = 0.0f;
        // Biquad coefficients
        float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
    };
    std::array<Resonator, maxResonators> resonators {};
    int numResonators = 0;

    void updateHarmonicFrequencies();
    void updateResonatorCoefficients();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterEffect)
};
