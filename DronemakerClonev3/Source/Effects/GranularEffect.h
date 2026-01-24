#pragma once

#include "EffectBase.h"
#include <vector>
#include <array>

/**
 * Simplified granular delay with pitch shifting.
 * Creates clouds of grains from a delay buffer.
 * Supports variable grain sizes and separate delay time control.
 */
class GranularEffect : public EffectBase
{
public:
    GranularEffect();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void processSample (float& left, float& right) override;
    void reset() override;
    juce::String getName() const override { return "Granular"; }

    // Grain size (randomized between min and max)
    void setGrainSizeMin (float ms) { grainSizeMinMs = juce::jlimit (10.0f, 2000.0f, ms); }
    void setGrainSizeMax (float ms) { grainSizeMaxMs = juce::jlimit (10.0f, 2000.0f, ms); }

    // Delay time (randomized between min and max) - how far back grains read from
    void setDelayMin (float ms) { delayMinMs = juce::jlimit (10.0f, 4000.0f, ms); }
    void setDelayMax (float ms) { delayMaxMs = juce::jlimit (10.0f, 4000.0f, ms); }

    void setPitch (float semitones) { pitch = juce::jlimit (-24.0f, 24.0f, semitones); updatePitchRatio(); }
    void setDensity (float d) { density = juce::jlimit (0.1f, 4.0f, d); }
    void setSpread (float s) { spread = juce::jlimit (0.0f, 1.0f, s); }
    void setDryWet (float dw) { dryWet = juce::jlimit (0.0f, 1.0f, dw); }

    float getGrainSizeMin() const { return grainSizeMinMs; }
    float getGrainSizeMax() const { return grainSizeMaxMs; }
    float getDelayMin() const { return delayMinMs; }
    float getDelayMax() const { return delayMaxMs; }
    float getPitch() const { return pitch; }
    float getDensity() const { return density; }
    float getSpread() const { return spread; }
    float getDryWet() const { return dryWet; }

private:
    static constexpr int maxGrains = 8;
    static constexpr int bufferSeconds = 4;

    struct Grain
    {
        double readPosition = 0.0;    // Position in the buffer
        double grainPosition = 0.0;   // Position within the grain (0 to grainLength)
        int grainLength = 0;          // Length of this grain in samples
        float pan = 0.5f;             // Stereo position (0 = left, 1 = right)
        bool active = false;
    };

    std::vector<float> bufferL;
    std::vector<float> bufferR;
    int writePos = 0;
    int bufferSize = 0;

    std::array<Grain, maxGrains> grains;
    int grainSpawnCounter = 0;
    int grainSpawnInterval = 1000;

    // Parameters
    float grainSizeMinMs = 50.0f;
    float grainSizeMaxMs = 200.0f;
    float delayMinMs = 100.0f;
    float delayMaxMs = 500.0f;
    float pitch = 0.0f;
    float pitchRatio = 1.0f;
    float density = 1.0f;
    float spread = 0.5f;
    float dryWet = 0.0f;  // Fully dry by default

    juce::Random random;

    void updatePitchRatio();
    void spawnGrain();
    float getWindowValue (double position, int length) const;
    float getInterpolatedSample (const std::vector<float>& buffer, double position) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularEffect)
};
