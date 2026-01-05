#pragma once

#include <JuceHeader.h>

/**
 * Abstract base class for all effects in the master effects chain.
 * Effects process stereo audio in-place, one sample at a time.
 */
class EffectBase
{
public:
    EffectBase() = default;
    virtual ~EffectBase() = default;

    // Called when audio playback starts
    virtual void prepareToPlay (double sampleRate, int samplesPerBlock) = 0;

    // Process one stereo sample in-place
    virtual void processSample (float& left, float& right) = 0;

    // Reset all internal state (delay lines, filter states, etc.)
    virtual void reset() = 0;

    // Get the effect name for UI display
    virtual juce::String getName() const = 0;

    // Enable/disable the effect (bypassed when disabled)
    bool isEnabled() const { return enabled; }
    void setEnabled (bool b) { enabled = b; }

protected:
    bool enabled = true;
    double sampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectBase)
};
