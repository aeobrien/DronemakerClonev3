#pragma once

#include "EffectBase.h"
#include <vector>

/**
 * Stereo delay with ping-pong option.
 */
class DelayEffect : public EffectBase
{
public:
    DelayEffect();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void processSample (float& left, float& right) override;
    void reset() override;
    juce::String getName() const override { return "Delay"; }

    void setDelayTimeMs (float ms) { delayTimeMs = juce::jlimit (1.0f, 2000.0f, ms); }
    void setFeedback (float fb) { feedback = juce::jlimit (0.0f, 0.95f, fb); }
    void setDryWet (float dw) { dryWet = juce::jlimit (0.0f, 1.0f, dw); }
    void setPingPong (bool pp) { pingPong = pp; }

    float getDelayTimeMs() const { return delayTimeMs; }
    float getFeedback() const { return feedback; }
    float getDryWet() const { return dryWet; }
    bool isPingPong() const { return pingPong; }

private:
    static constexpr int maxDelaySeconds = 2;

    std::vector<float> delayLineL;
    std::vector<float> delayLineR;
    int writePos = 0;
    int maxDelaySamples = 0;

    float delayTimeMs = 250.0f;
    float feedback = 0.3f;
    float dryWet = 0.5f;
    bool pingPong = false;

    // Feedback storage for ping-pong
    float feedbackL = 0.0f;
    float feedbackR = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayEffect)
};
