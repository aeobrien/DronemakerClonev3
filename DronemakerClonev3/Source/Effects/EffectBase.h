#pragma once

#include <JuceHeader.h>
#include <cmath>

/**
 * Simple parameter smoother using one-pole lowpass filter.
 * Prevents clicks when parameters change by smoothly interpolating.
 */
class SmoothedParam
{
public:
    SmoothedParam (float initialValue = 0.0f) : target (initialValue), current (initialValue) {}

    void setTargetValue (float newTarget) { target = newTarget; }
    float getTargetValue() const { return target; }

    void setCurrentAndTarget (float value) { current = target = value; }

    // Call once when sample rate is known
    void setSmoothingTime (double sampleRate, float timeMs = 10.0f)
    {
        // Calculate coefficient for given smoothing time
        // timeMs is the approximate time to reach 63% of target (one time constant)
        float smoothingHz = 1000.0f / timeMs;
        coeff = 1.0f - std::exp (-2.0f * 3.14159265f * smoothingHz / static_cast<float> (sampleRate));
    }

    // Call once per sample to get smoothed value
    float getNextValue()
    {
        current += coeff * (target - current);
        return current;
    }

    float getCurrentValue() const { return current; }

    // Check if we're close enough to target (for optimization)
    bool isSmoothing() const { return std::abs (target - current) > 1e-6f; }

private:
    float target = 0.0f;
    float current = 0.0f;
    float coeff = 0.1f;  // Default smoothing coefficient
};

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

    // Gain compensation (applied after effect processing)
    void setGainCompensation (float gainDB) { gainCompDB = gainDB; }
    float getGainCompensation() const { return gainCompDB; }
    void setAutoGainComp (bool b) { autoGainComp = b; }
    bool getAutoGainComp() const { return autoGainComp; }
    float getAutoGainFactor() const { return autoGainFactor; }

protected:
    bool enabled = true;
    double sampleRate = 44100.0;
    float gainCompDB = 0.0f;     // Manual gain compensation in dB
    bool autoGainComp = true;    // Auto gain compensation enabled by default

    // Subclasses set this to indicate how much gain the effect adds
    float autoGainFactor = 1.0f; // Multiplier for auto gain comp (calculated by effect)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectBase)
};
