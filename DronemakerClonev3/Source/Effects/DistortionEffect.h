#pragma once

#include "EffectBase.h"

/**
 * Distortion effect with multiple algorithms.
 * Includes DC blocking and post-distortion tone control.
 * Uses parameter smoothing to prevent clicks when adjusting.
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
    void setDrive (float d) { driveSmooth.setTargetValue (juce::jlimit (1.0f, 20.0f, d)); }
    void setTone (float t) { toneSmooth.setTargetValue (juce::jlimit (0.0f, 1.0f, t)); }
    void setDryWet (float dw) { dryWetSmooth.setTargetValue (juce::jlimit (0.0f, 1.0f, dw)); }
    void setBitDepth (float bits) { bitDepthSmooth.setTargetValue (juce::jlimit (1.0f, 16.0f, bits)); }
    void setSampleRateReduction (float factor) { srReductionSmooth.setTargetValue (juce::jlimit (1.0f, 64.0f, factor)); }

    int getAlgorithm() const { return algorithm; }
    float getDrive() const { return driveSmooth.getTargetValue(); }
    float getTone() const { return toneSmooth.getTargetValue(); }
    float getDryWet() const { return dryWetSmooth.getTargetValue(); }
    float getBitDepth() const { return bitDepthSmooth.getTargetValue(); }
    float getSampleRateReduction() const { return srReductionSmooth.getTargetValue(); }

private:
    int algorithm = 0;

    // Smoothed parameters
    SmoothedParam driveSmooth { 1.0f };
    SmoothedParam toneSmooth { 0.5f };
    SmoothedParam dryWetSmooth { 0.0f };
    SmoothedParam bitDepthSmooth { 8.0f };
    SmoothedParam srReductionSmooth { 1.0f };

    // For bitcrush
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

    float processSoftClip (float sample, float drive);
    float processHardClip (float sample, float drive);
    float processWavefold (float sample, float drive);
    float processBitcrush (float sample, float& hold, int& counter, float bitDepth, float srReduction);
    float processDCBlock (float sample, float& state, float& prev);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DistortionEffect)
};
