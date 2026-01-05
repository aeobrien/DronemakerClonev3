#include "TremoloEffect.h"
#include <cmath>

TremoloEffect::TremoloEffect()
{
}

void TremoloEffect::prepareToPlay (double sr, int /*samplesPerBlock*/)
{
    sampleRate = sr;
    reset();
}

void TremoloEffect::reset()
{
    phase = 0.0f;
}

float TremoloEffect::getLfoValue (float p) const
{
    // Normalize phase to 0-1
    float normPhase = p - std::floor (p);

    switch (waveform)
    {
        case 0: // Sine
            return 0.5f + 0.5f * std::sin (normPhase * juce::MathConstants<float>::twoPi);

        case 1: // Triangle
            return normPhase < 0.5f ? (normPhase * 2.0f) : (2.0f - normPhase * 2.0f);

        case 2: // Square
            return normPhase < 0.5f ? 1.0f : 0.0f;

        default:
            return 0.5f;
    }
}

void TremoloEffect::processSample (float& left, float& right)
{
    if (! enabled)
        return;

    // Get LFO values
    float lfoL = getLfoValue (phase);
    float lfoR = stereo ? getLfoValue (phase + 0.5f) : lfoL;  // 180 degree offset for stereo

    // Apply depth: 1.0 = full volume, (1-depth) = minimum volume
    float modL = 1.0f - depth * (1.0f - lfoL);
    float modR = 1.0f - depth * (1.0f - lfoR);

    left *= modL;
    right *= modR;

    // Advance phase
    phase += static_cast<float> (rate / sampleRate);
    if (phase >= 1.0f)
        phase -= 1.0f;
}
