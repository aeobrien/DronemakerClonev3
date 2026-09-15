#pragma once

#include <JuceHeader.h>
#include "../Effects/EffectBase.h"
#include <array>
#include <functional>

/**
 * Modulation target descriptor - identifies what parameter to modulate
 */
struct ModulationTarget
{
    enum class Type
    {
        None = 0,
        // Loop parameters (index 0-7 for loops 1-8)
        LoopVolume,
        LoopFilterHP,
        LoopFilterLP,
        // Effect parameters
        DelayTime,
        DelayFeedback,
        DelayDryWet,
        DistortionDrive,
        DistortionTone,
        DistortionDryWet,
        TapeSaturation,
        TapeBias,
        TapeWowDepth,
        TapeFlutterDepth,
        TapeDryWet,
        FilterHP,
        FilterLP,
        FilterHarmonicIntensity,
        TremoloRate,
        TremoloDepth,
        // Master
        MasterVolume,
        LoopMix
    };

    Type type = Type::None;
    int index = 0;        // For loop parameters, which loop (0-3)
    float rangeMin = 0.0f;  // Lower bound of modulation range
    float rangeMax = 1.0f;  // Upper bound of modulation range

    bool isActive() const { return type != Type::None && std::abs (rangeMax - rangeMin) > 0.001f; }

    // Get the default min/max for a target type
    static void getDefaultRange (Type t, float& outMin, float& outMax)
    {
        switch (t)
        {
            case Type::LoopVolume:
                outMin = 0.0f; outMax = 2.0f; break;

            case Type::DelayDryWet:
            case Type::DistortionDryWet:
            case Type::TapeDryWet:
            case Type::FilterHarmonicIntensity:
            case Type::MasterVolume:
            case Type::LoopMix:
                outMin = 0.0f; outMax = 1.0f; break;

            case Type::LoopFilterHP:
            case Type::FilterHP:
                outMin = 20.0f; outMax = 5000.0f; break;

            case Type::LoopFilterLP:
            case Type::FilterLP:
                outMin = 200.0f; outMax = 20000.0f; break;

            case Type::DelayTime:
                outMin = 1.0f; outMax = 2000.0f; break;

            case Type::DelayFeedback:
                outMin = 0.0f; outMax = 0.95f; break;

            case Type::DistortionDrive:
                outMin = 1.0f; outMax = 20.0f; break;

            case Type::DistortionTone:
                outMin = 0.0f; outMax = 1.0f; break;

            case Type::TapeSaturation:
                outMin = 0.0f; outMax = 2.0f; break;

            case Type::TapeBias:
                outMin = 0.0f; outMax = 1.0f; break;

            case Type::TapeWowDepth:
            case Type::TapeFlutterDepth:
                outMin = 0.0f; outMax = 1.0f; break;

            case Type::TremoloRate:
                outMin = 0.1f; outMax = 20.0f; break;

            case Type::TremoloDepth:
                outMin = 0.0f; outMax = 1.0f; break;

            default:
                outMin = 0.0f; outMax = 1.0f; break;
        }
    }

    // Calculate the modulated value given a modulation input (0-1)
    float getModulatedValue (float modInput) const
    {
        // modInput is 0-1 for unipolar sources, map to range
        return rangeMin + modInput * (rangeMax - rangeMin);
    }
};

/**
 * Base class for all modulation sources (LFO, Envelope Follower, Randomizer)
 */
class ModulationSource
{
public:
    static constexpr int maxTargets = 3;

    ModulationSource() = default;
    virtual ~ModulationSource() = default;

    virtual void prepareToPlay (double sampleRate, int samplesPerBlock) = 0;
    virtual void processSample (float inputL = 0.0f, float inputR = 0.0f) = 0;
    virtual void reset() = 0;
    virtual juce::String getName() const = 0;

    // Get the current modulation value (-1 to +1 for bipolar, 0 to +1 for unipolar)
    float getCurrentValue() const { return currentValue; }

    // Target management
    void setTarget (int slot, ModulationTarget target) { if (slot >= 0 && slot < maxTargets) targets[slot] = target; }
    ModulationTarget& getTarget (int slot) { return targets[slot]; }
    const ModulationTarget& getTarget (int slot) const { return targets[slot]; }

    // Enable/disable
    bool isEnabled() const { return enabled; }
    void setEnabled (bool b) { enabled = b; }

protected:
    float currentValue = 0.0f;
    double sampleRate = 44100.0;
    bool enabled = true;
    std::array<ModulationTarget, maxTargets> targets {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModulationSource)
};

/**
 * LFO modulation source with multiple waveforms
 */
class LFOSource : public ModulationSource
{
public:
    enum class Waveform
    {
        Sine = 0,
        Triangle,
        Square,
        SawUp,
        SawDown,
        SampleAndHold
    };

    LFOSource() = default;

    void prepareToPlay (double sr, int /*samplesPerBlock*/) override
    {
        sampleRate = sr;
        phaseSmooth.setSmoothingTime (sr, 50.0f);
    }

    void processSample (float /*inputL*/ = 0.0f, float /*inputR*/ = 0.0f) override
    {
        if (! enabled)
        {
            currentValue = 0.0f;
            return;
        }

        // Advance phase
        phase += rate / static_cast<float> (sampleRate);
        if (phase >= 1.0f)
        {
            phase -= 1.0f;
            // Sample and hold: capture new random value at cycle start
            if (waveform == Waveform::SampleAndHold)
                sampleHoldValue = random.nextFloat() * 2.0f - 1.0f;
        }

        // Generate waveform
        float value = 0.0f;
        switch (waveform)
        {
            case Waveform::Sine:
                value = std::sin (phase * juce::MathConstants<float>::twoPi);
                break;

            case Waveform::Triangle:
                value = phase < 0.5f ? (phase * 4.0f - 1.0f) : (3.0f - phase * 4.0f);
                break;

            case Waveform::Square:
                value = phase < 0.5f ? 1.0f : -1.0f;
                break;

            case Waveform::SawUp:
                value = phase * 2.0f - 1.0f;
                break;

            case Waveform::SawDown:
                value = 1.0f - phase * 2.0f;
                break;

            case Waveform::SampleAndHold:
                value = sampleHoldValue;
                break;
        }

        currentValue = value;
    }

    void reset() override
    {
        phase = 0.0f;
        currentValue = 0.0f;
        sampleHoldValue = 0.0f;
    }

    juce::String getName() const override { return "LFO"; }

    // LFO parameters
    void setRate (float hz) { rate = juce::jlimit (0.002f, 10.0f, hz); }
    void setWaveform (Waveform wf) { waveform = wf; }

    float getRate() const { return rate; }
    Waveform getWaveform() const { return waveform; }

private:
    float phase = 0.0f;
    float rate = 1.0f;  // Hz
    Waveform waveform = Waveform::Sine;
    float sampleHoldValue = 0.0f;
    juce::Random random;
    SmoothedParam phaseSmooth { 0.0f };
};

/**
 * Envelope follower that tracks input audio amplitude
 */
class EnvelopeFollowerSource : public ModulationSource
{
public:
    EnvelopeFollowerSource() = default;

    void prepareToPlay (double sr, int /*samplesPerBlock*/) override
    {
        sampleRate = sr;
        updateCoefficients();
    }

    void processSample (float inputL = 0.0f, float inputR = 0.0f) override
    {
        if (! enabled)
        {
            currentValue = 0.0f;
            return;
        }

        // Get input amplitude (RMS-like)
        float inputLevel = std::sqrt (inputL * inputL + inputR * inputR) * 0.707f;

        // Apply sensitivity
        inputLevel *= sensitivity;

        // Envelope follower with separate attack/release
        if (inputLevel > envelope)
            envelope += attackCoeff * (inputLevel - envelope);
        else
            envelope += releaseCoeff * (inputLevel - envelope);

        // Output is 0 to 1 (unipolar)
        currentValue = juce::jlimit (0.0f, 1.0f, envelope);
    }

    void reset() override
    {
        envelope = 0.0f;
        currentValue = 0.0f;
    }

    juce::String getName() const override { return "Envelope"; }

    // Parameters
    void setAttackMs (float ms)
    {
        attackMs = juce::jlimit (0.1f, 500.0f, ms);
        updateCoefficients();
    }

    void setReleaseMs (float ms)
    {
        releaseMs = juce::jlimit (1.0f, 2000.0f, ms);
        updateCoefficients();
    }

    void setSensitivity (float s) { sensitivity = juce::jlimit (0.1f, 10.0f, s); }

    float getAttackMs() const { return attackMs; }
    float getReleaseMs() const { return releaseMs; }
    float getSensitivity() const { return sensitivity; }

private:
    void updateCoefficients()
    {
        if (sampleRate > 0)
        {
            attackCoeff = 1.0f - std::exp (-1.0f / (static_cast<float> (sampleRate) * attackMs * 0.001f));
            releaseCoeff = 1.0f - std::exp (-1.0f / (static_cast<float> (sampleRate) * releaseMs * 0.001f));
        }
    }

    float envelope = 0.0f;
    float attackMs = 10.0f;
    float releaseMs = 200.0f;
    float sensitivity = 1.0f;
    float attackCoeff = 0.1f;
    float releaseCoeff = 0.01f;
};

/**
 * Randomizer that smoothly drifts to random values
 */
class RandomizerSource : public ModulationSource
{
public:
    RandomizerSource() = default;

    void prepareToPlay (double sr, int /*samplesPerBlock*/) override
    {
        sampleRate = sr;
        updateSmoothingCoeff();
    }

    void processSample (float /*inputL*/ = 0.0f, float /*inputR*/ = 0.0f) override
    {
        if (! enabled)
        {
            currentValue = 0.0f;
            return;
        }

        // Count down to next target change
        samplesUntilChange--;
        if (samplesUntilChange <= 0)
        {
            // Pick new random target
            targetValue = random.nextFloat() * 2.0f - 1.0f;
            // Randomize time until next change (based on rate)
            float baseInterval = sampleRate / rate;
            samplesUntilChange = static_cast<int> (baseInterval * (0.5f + random.nextFloat()));
        }

        // Smooth towards target
        currentValue += smoothCoeff * (targetValue - currentValue);
    }

    void reset() override
    {
        currentValue = 0.0f;
        targetValue = 0.0f;
        samplesUntilChange = 0;
    }

    juce::String getName() const override { return "Random"; }

    // Parameters
    void setRate (float hz)
    {
        rate = juce::jlimit (0.002f, 10.0f, hz);
        updateSmoothingCoeff();
    }

    void setSmoothness (float s)
    {
        smoothness = juce::jlimit (0.0f, 1.0f, s);
        updateSmoothingCoeff();
    }

    float getRate() const { return rate; }
    float getSmoothness() const { return smoothness; }

private:
    void updateSmoothingCoeff()
    {
        if (sampleRate > 0)
        {
            // Smoothness affects how quickly we approach the target
            // Higher smoothness = slower approach = smoother result
            float smoothTimeMs = 10.0f + smoothness * 490.0f;  // 10ms to 500ms
            smoothCoeff = 1.0f - std::exp (-1.0f / (static_cast<float> (sampleRate) * smoothTimeMs * 0.001f));
        }
    }

    float rate = 0.5f;       // How often to pick new random targets (Hz)
    float smoothness = 0.5f; // How smooth the transitions are
    float targetValue = 0.0f;
    float smoothCoeff = 0.01f;
    int samplesUntilChange = 0;
    juce::Random random;
};
