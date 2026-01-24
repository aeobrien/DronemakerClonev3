#pragma once

#include "EffectBase.h"

/**
 * LFO-based tremolo effect with stereo phase offset option.
 */
class TremoloEffect : public EffectBase
{
public:
    TremoloEffect();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void processSample (float& left, float& right) override;
    void reset() override;
    juce::String getName() const override { return "Tremolo"; }

    void setRate (float hz) { rate = juce::jlimit (0.1f, 20.0f, hz); }
    void setDepth (float d) { depth = juce::jlimit (0.0f, 1.0f, d); }
    void setWaveform (int wf) { waveform = juce::jlimit (0, 2, wf); }
    void setStereo (bool s) { stereo = s; }

    float getRate() const { return rate; }
    float getDepth() const { return depth; }
    int getWaveform() const { return waveform; }
    bool isStereo() const { return stereo; }

private:
    float phase = 0.0f;

    float rate = 4.0f;      // Hz
    float depth = 0.0f;     // 0-1, no tremolo by default
    int waveform = 0;       // 0=sine, 1=triangle, 2=square
    bool stereo = true;     // Phase offset between L/R

    float getLfoValue (float p) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TremoloEffect)
};
