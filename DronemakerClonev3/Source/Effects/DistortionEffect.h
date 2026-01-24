#pragma once

#include "EffectBase.h"

/**
 * Distortion effect with multiple algorithms.
 * Includes DC blocking and post-distortion tone control.
 */
class DistortionEffect : public EffectBase
{
public:
    enum Algorithm
    {
        SoftClip = 0,
        HardClip = 1,
        Wavefold = 2,
        Bitcrush = 3
    };

    DistortionEffect();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void processSample (float& left, float& right) override;
    void reset() override;
    juce::String getName() const override { return "Distortion"; }

    void setAlgorithm (int alg) { algorithm = juce::jlimit (0, 3, alg); }
    void setDrive (float d) { drive = juce::jlimit (1.0f, 20.0f, d); }
    void setTone (float t) { tone = juce::jlimit (0.0f, 1.0f, t); }
    void setDryWet (float dw) { dryWet = juce::jlimit (0.0f, 1.0f, dw); }
    void setBitDepth (float bits) { bitDepth = juce::jlimit (1.0f, 16.0f, bits); }
    void setSampleRateReduction (float factor) { srReduction = juce::jlimit (1.0f, 64.0f, factor); }

    int getAlgorithm() const { return algorithm; }
    float getDrive() const { return drive; }
    float getTone() const { return tone; }
    float getDryWet() const { return dryWet; }
    float getBitDepth() const { return bitDepth; }
    float getSampleRateReduction() const { return srReduction; }

private:
    int algorithm = 0;
    float drive = 1.0f;
    float tone = 0.5f;
    float dryWet = 0.0f;  // Fully dry by default

    // For bitcrush
    float bitDepth = 8.0f;
    float srReduction = 1.0f;
    int srCounter = 0;
    float holdL = 0.0f;
    float holdR = 0.0f;

    // DC blocker state
    float dcStateL = 0.0f;
    float dcStateR = 0.0f;
    float dcPrevL = 0.0f;
    float dcPrevR = 0.0f;

    // Tone filter state
    float toneStateL = 0.0f;
    float toneStateR = 0.0f;

    float processSoftClip (float sample);
    float processHardClip (float sample);
    float processWavefold (float sample);
    float processBitcrush (float sample, float& hold, int& counter);
    float processDCBlock (float sample, float& state, float& prev);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DistortionEffect)
};
