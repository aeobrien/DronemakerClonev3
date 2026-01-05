#pragma once

#include "EffectBase.h"
#include <vector>
#include <array>

/**
 * Simplified granular delay with pitch shifting.
 * Creates clouds of grains from a delay buffer.
 */
class GranularEffect : public EffectBase
{
public:
    GranularEffect();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void processSample (float& left, float& right) override;
    void reset() override;
    juce::String getName() const override { return "Granular"; }

    void setGrainSize (float ms) { grainSizeMs = juce::jlimit (10.0f, 500.0f, ms); }
    void setPitch (float semitones) { pitch = juce::jlimit (-24.0f, 24.0f, semitones); updatePitchRatio(); }
    void setDensity (float d) { density = juce::jlimit (0.1f, 4.0f, d); }
    void setSpread (float s) { spread = juce::jlimit (0.0f, 1.0f, s); }
    void setDryWet (float dw) { dryWet = juce::jlimit (0.0f, 1.0f, dw); }

    float getGrainSize() const { return grainSizeMs; }
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

    float grainSizeMs = 100.0f;
    float pitch = 0.0f;
    float pitchRatio = 1.0f;
    float density = 1.0f;
    float spread = 0.5f;
    float dryWet = 0.5f;

    juce::Random random;

    void updatePitchRatio();
    void spawnGrain();
    float getWindowValue (double position, int length) const;
    float getInterpolatedSample (const std::vector<float>& buffer, double position) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularEffect)
};
