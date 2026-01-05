#include "GranularEffect.h"
#include <cmath>

GranularEffect::GranularEffect()
{
}

void GranularEffect::prepareToPlay (double sr, int /*samplesPerBlock*/)
{
    sampleRate = sr;
    bufferSize = static_cast<int> (sr * bufferSeconds);

    bufferL.resize (bufferSize, 0.0f);
    bufferR.resize (bufferSize, 0.0f);

    reset();
}

void GranularEffect::reset()
{
    std::fill (bufferL.begin(), bufferL.end(), 0.0f);
    std::fill (bufferR.begin(), bufferR.end(), 0.0f);
    writePos = 0;
    grainSpawnCounter = 0;

    for (auto& grain : grains)
        grain.active = false;
}

void GranularEffect::updatePitchRatio()
{
    pitchRatio = std::pow (2.0f, pitch / 12.0f);
}

float GranularEffect::getWindowValue (double position, int length) const
{
    // Hann window
    if (length <= 0)
        return 0.0f;

    double normalized = position / static_cast<double> (length);
    if (normalized < 0.0 || normalized > 1.0)
        return 0.0f;

    return static_cast<float> (0.5 * (1.0 - std::cos (2.0 * juce::MathConstants<double>::pi * normalized)));
}

float GranularEffect::getInterpolatedSample (const std::vector<float>& buffer, double position) const
{
    if (bufferSize <= 0)
        return 0.0f;

    // Wrap position
    while (position < 0)
        position += bufferSize;
    while (position >= bufferSize)
        position -= bufferSize;

    int idx0 = static_cast<int> (position);
    int idx1 = (idx0 + 1) % bufferSize;
    float frac = static_cast<float> (position - idx0);

    return buffer[idx0] * (1.0f - frac) + buffer[idx1] * frac;
}

void GranularEffect::spawnGrain()
{
    // Find an inactive grain slot
    for (auto& grain : grains)
    {
        if (! grain.active)
        {
            // Calculate grain parameters
            int grainLengthSamples = static_cast<int> (grainSizeMs * 0.001 * sampleRate);

            // Start position: random offset from current write position based on spread
            int maxOffset = static_cast<int> (bufferSize * spread * 0.5);
            int offset = maxOffset > 0 ? random.nextInt (maxOffset * 2) - maxOffset : 0;

            grain.readPosition = static_cast<double> (writePos - grainLengthSamples - offset);
            if (grain.readPosition < 0)
                grain.readPosition += bufferSize;

            grain.grainPosition = 0.0;
            grain.grainLength = grainLengthSamples;
            grain.pan = 0.3f + random.nextFloat() * 0.4f;  // Random pan between 0.3 and 0.7
            grain.active = true;

            break;
        }
    }
}

void GranularEffect::processSample (float& left, float& right)
{
    if (! enabled || bufferSize == 0)
        return;

    // Store dry signal
    float dryL = left;
    float dryR = right;

    // Write to buffer
    bufferL[writePos] = left;
    bufferR[writePos] = right;
    writePos = (writePos + 1) % bufferSize;

    // Spawn new grains based on density
    int grainLengthSamples = static_cast<int> (grainSizeMs * 0.001 * sampleRate);
    grainSpawnInterval = static_cast<int> (grainLengthSamples / density);

    grainSpawnCounter++;
    if (grainSpawnCounter >= grainSpawnInterval)
    {
        grainSpawnCounter = 0;
        spawnGrain();
    }

    // Process all active grains
    float wetL = 0.0f;
    float wetR = 0.0f;
    int activeCount = 0;

    for (auto& grain : grains)
    {
        if (grain.active)
        {
            // Get windowed sample
            float window = getWindowValue (grain.grainPosition, grain.grainLength);
            float sampleL = getInterpolatedSample (bufferL, grain.readPosition) * window;
            float sampleR = getInterpolatedSample (bufferR, grain.readPosition) * window;

            // Apply pan
            wetL += sampleL * (1.0f - grain.pan);
            wetR += sampleR * grain.pan;
            activeCount++;

            // Advance positions
            grain.readPosition += pitchRatio;
            if (grain.readPosition >= bufferSize)
                grain.readPosition -= bufferSize;

            grain.grainPosition += 1.0;

            // Deactivate grain when it ends
            if (grain.grainPosition >= grain.grainLength)
                grain.active = false;
        }
    }

    // Normalize
    if (activeCount > 1)
    {
        float norm = 1.0f / std::sqrt (static_cast<float> (activeCount));
        wetL *= norm;
        wetR *= norm;
    }

    // Dry/wet mix
    left = dryL * (1.0f - dryWet) + wetL * dryWet;
    right = dryR * (1.0f - dryWet) + wetR * dryWet;
}
