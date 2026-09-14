#pragma once

#include "EffectBase.h"
#include <array>

/**
 * Analog tape simulation with saturation, wow/flutter, and HF loss.
 * Separate rate and depth controls for wow and flutter.
 * Uses parameter smoothing to prevent clicks when adjusting.
 */
class TapeEffect : public EffectBase
{
public:
    TapeEffect();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void processSample (float& left, float& right) override;
    void reset() override;
    juce::String getName() const override { return "Tape"; }

    // Saturation (0-2, higher values for extreme saturation)
    void setSaturation (float s) { saturationSmooth.setTargetValue (juce::jlimit (0.0f, 2.0f, s)); }
    void setBias (float b) { biasSmooth.setTargetValue (juce::jlimit (0.0f, 1.0f, b)); }

    // Wow (slow pitch variation) - separate rate and depth
    void setWowRate (float hz) { wowRateSmooth.setTargetValue (juce::jlimit (0.1f, 2.0f, hz)); }
    void setWowDepth (float d) { wowDepthSmooth.setTargetValue (juce::jlimit (0.0f, 1.0f, d)); }

    // Flutter (fast pitch variation) - separate rate and depth
    void setFlutterRate (float hz) { flutterRateSmooth.setTargetValue (juce::jlimit (2.0f, 15.0f, hz)); }
    void setFlutterDepth (float d) { flutterDepthSmooth.setTargetValue (juce::jlimit (0.0f, 1.0f, d)); }

    // HF loss (0-1, higher = more extreme HF attenuation)
    void setHfLoss (float l) { hfLossSmooth.setTargetValue (juce::jlimit (0.0f, 1.0f, l)); }
    void setDryWet (float dw) { dryWetSmooth.setTargetValue (juce::jlimit (0.0f, 1.0f, dw)); }

    float getSaturation() const { return saturationSmooth.getTargetValue(); }
    float getBias() const { return biasSmooth.getTargetValue(); }
    float getWowRate() const { return wowRateSmooth.getTargetValue(); }
    float getWowDepth() const { return wowDepthSmooth.getTargetValue(); }
    float getFlutterRate() const { return flutterRateSmooth.getTargetValue(); }
    float getFlutterDepth() const { return flutterDepthSmooth.getTargetValue(); }
    float getHfLoss() const { return hfLossSmooth.getTargetValue(); }
    float getDryWet() const { return dryWetSmooth.getTargetValue(); }

private:
    // Smoothed parameters
    SmoothedParam saturationSmooth { 0.3f };
    SmoothedParam biasSmooth { 0.5f };
    SmoothedParam wowRateSmooth { 0.5f };       // Hz (0.1-2)
    SmoothedParam wowDepthSmooth { 0.0f };
    SmoothedParam flutterRateSmooth { 6.0f };   // Hz (2-15)
    SmoothedParam flutterDepthSmooth { 0.0f };
    SmoothedParam hfLossSmooth { 0.3f };
    SmoothedParam dryWetSmooth { 0.0f };

    // Wow/flutter LFOs
    float wowPhase = 0.0f;
    float flutterPhase = 0.0f;

    // Variable delay line for wow/flutter
    static constexpr int delayLineSize = 4096;
    std::array<float, delayLineSize> delayLineL {};
    std::array<float, delayLineSize> delayLineR {};
    int writePos = 0;

    // HF loss filter state
    float hfStateL = 0.0f;
    float hfStateR = 0.0f;

    float processSaturation (float sample, float saturation, float bias);
    float getInterpolatedDelay (const std::array<float, delayLineSize>& buffer, float delay) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeEffect)
};
