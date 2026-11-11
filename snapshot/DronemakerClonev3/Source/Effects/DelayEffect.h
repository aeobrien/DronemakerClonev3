#pragma once

#include "EffectBase.h"
#include <vector>

/**
 * Stereo delay with ping-pong option.
 * Uses parameter smoothing to prevent clicks when adjusting.
 */
class DelayEffect : public EffectBase
{
public:
    DelayEffect();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void processSample (float& left, float& right) override;
    void reset() override;
    juce::String getName() const override { return "Delay"; }

    void setDelayTimeMs (float ms) { delayTimeMsSmooth.setTargetValue (juce::jlimit (1.0f, 2000.0f, ms)); }
    void setFeedback (float fb) { feedbackSmooth.setTargetValue (juce::jlimit (0.0f, 0.95f, fb)); }
    void setDryWet (float dw) { dryWetSmooth.setTargetValue (juce::jlimit (0.0f, 1.0f, dw)); }
    void setPingPong (bool pp) { pingPong = pp; }

    float getDelayTimeMs() const { return delayTimeMsSmooth.getTargetValue(); }
    float getFeedback() const { return feedbackSmooth.getTargetValue(); }
    float getDryWet() const { return dryWetSmooth.getTargetValue(); }
    bool isPingPong() const { return pingPong; }

private:
    static constexpr int maxDelaySeconds = 2;

    std::vector<float> delayLineL;
    std::vector<float> delayLineR;
    int writePos = 0;
    int maxDelaySamples = 0;

    // Smoothed parameters
    SmoothedParam delayTimeMsSmooth { 250.0f };
    SmoothedParam feedbackSmooth { 0.3f };
    SmoothedParam dryWetSmooth { 0.0f };
    bool pingPong = false;

    // Feedback storage for ping-pong
    float feedbackL = 0.0f;
    float feedbackR = 0.0f;

    // Helper for interpolated delay reading
    float readDelayInterpolated (const std::vector<float>& line, float delaySamples);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayEffect)
};
