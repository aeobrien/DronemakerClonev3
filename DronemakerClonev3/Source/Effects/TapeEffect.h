#pragma once

#include "EffectBase.h"
#include <array>

/**
 * Analog tape simulation with saturation, wow/flutter, and HF loss.
 */
class TapeEffect : public EffectBase
{
public:
    TapeEffect();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void processSample (float& left, float& right) override;
    void reset() override;
    juce::String getName() const override { return "Tape"; }

    void setSaturation (float s) { saturation = juce::jlimit (0.0f, 1.0f, s); }
    void setBias (float b) { bias = juce::jlimit (0.0f, 1.0f, b); }
    void setWowDepth (float d) { wowDepth = juce::jlimit (0.0f, 1.0f, d); }
    void setFlutterDepth (float d) { flutterDepth = juce::jlimit (0.0f, 1.0f, d); }
    void setHfLoss (float l) { hfLoss = juce::jlimit (0.0f, 1.0f, l); }
    void setDryWet (float dw) { dryWet = juce::jlimit (0.0f, 1.0f, dw); }

    float getSaturation() const { return saturation; }
    float getBias() const { return bias; }
    float getWowDepth() const { return wowDepth; }
    float getFlutterDepth() const { return flutterDepth; }
    float getHfLoss() const { return hfLoss; }
    float getDryWet() const { return dryWet; }

private:
    // Parameters
    float saturation = 0.3f;
    float bias = 0.5f;
    float wowDepth = 0.0f;
    float flutterDepth = 0.0f;
    float hfLoss = 0.3f;
    float dryWet = 1.0f;

    // Wow/flutter LFOs
    float wowPhase = 0.0f;
    float flutterPhase = 0.0f;
    static constexpr float wowRate = 0.5f;      // Hz
    static constexpr float flutterRate = 6.0f;  // Hz

    // Variable delay line for wow/flutter
    static constexpr int delayLineSize = 4096;
    std::array<float, delayLineSize> delayLineL {};
    std::array<float, delayLineSize> delayLineR {};
    int writePos = 0;

    // HF loss filter state
    float hfStateL = 0.0f;
    float hfStateR = 0.0f;

    float processSaturation (float sample);
    float getInterpolatedDelay (const std::array<float, delayLineSize>& buffer, float delay) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeEffect)
};
