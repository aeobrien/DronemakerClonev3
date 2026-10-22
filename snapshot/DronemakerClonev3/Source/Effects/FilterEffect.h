#pragma once

#include "EffectBase.h"
#include <array>

/**
 * High-pass and Low-pass filter with variable poles (1-8).
 * Also includes harmonic/scale filtering for musical applications.
 * Uses parameter smoothing to prevent clicks when adjusting.
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
    void setHighPassFreq (float hz) { hpFreqSmooth.setTargetValue (juce::jlimit (20.0f, 5000.0f, hz)); }
    void setLowPassFreq (float hz) { lpFreqSmooth.setTargetValue (juce::jlimit (200.0f, 20000.0f, hz)); }
    void setHighPassPoles (int poles) { hpPoles = juce::jlimit (1, maxPoles, poles); }
    void setLowPassPoles (int poles) { lpPoles = juce::jlimit (1, maxPoles, poles); }

    float getHighPassFreq() const { return hpFreqSmooth.getTargetValue(); }
    float getLowPassFreq() const { return lpFreqSmooth.getTargetValue(); }
    int getHighPassPoles() const { return hpPoles; }
    int getLowPassPoles() const { return lpPoles; }

    // Harmonic filter parameters
    void setHarmonicEnabled (bool b) { harmonicEnabled = b; }
    void setRootNote (int note);
    void setScaleType (int type);
    void setHarmonicIntensity (float i) { harmonicIntensitySmooth.setTargetValue (juce::jlimit (0.0f, 1.0f, i)); }

    bool isHarmonicEnabled() const { return harmonicEnabled; }
    int getRootNote() const { return rootNote; }
    int getScaleType() const { return scaleType; }
    float getHarmonicIntensity() const { return harmonicIntensitySmooth.getTargetValue(); }

private:
    // HP/LP filter
    static constexpr int maxPoles = 8;
    std::array<float, maxPoles> hpStateL {}, hpStateR {};
    std::array<float, maxPoles> lpStateL {}, lpStateR {};

    // Smoothed filter frequencies
    SmoothedParam hpFreqSmooth { 20.0f };
    SmoothedParam lpFreqSmooth { 20000.0f };
    int hpPoles = 1;
    int lpPoles = 1;

    // Harmonic filter (comb filter based on scale frequencies)
    bool harmonicEnabled = false;
    int rootNote = 0;         // 0=C, 1=C#, ..., 11=B
    int scaleType = 0;        // 0=Octaves, 1=Fifths, 2=Ionian, etc.
    SmoothedParam harmonicIntensitySmooth { 0.0f };  // Default to 0 (off)

    // Cross-fade support for smooth scale/root changes
    bool needsCrossfade = false;
    SmoothedParam crossfadeSmooth { 1.0f };

    // Pre-computed allowed frequencies for harmonic filter
    static constexpr int maxHarmonicFreqs = 128;
    std::array<float, maxHarmonicFreqs> harmonicFreqs {};
    int numHarmonicFreqs = 0;

    // Harmonic filter state (biquad bandpass for each allowed frequency)
    // Two banks for cross-fading during scale/root changes
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
    std::array<Resonator, maxResonators> resonatorsOld {};  // For cross-fading
    int numResonators = 0;
    int numResonatorsOld = 0;

    void updateHarmonicFrequencies();
    void updateResonatorCoefficients();
    float processResonatorBank (std::array<Resonator, maxResonators>& bank, int count, float left, float right, float& outR);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterEffect)
};
