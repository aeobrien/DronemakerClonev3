#include "DelayEffect.h"

DelayEffect::DelayEffect()
{
}

void DelayEffect::prepareToPlay (double sr, int /*samplesPerBlock*/)
{
    sampleRate = sr;
    maxDelaySamples = static_cast<int> (sr * maxDelaySeconds);

    delayLineL.resize (maxDelaySamples, 0.0f);
    delayLineR.resize (maxDelaySamples, 0.0f);

    // Set up parameter smoothing (20ms smoothing time)
    delayTimeMsSmooth.setSmoothingTime (sr, 20.0f);
    feedbackSmooth.setSmoothingTime (sr, 10.0f);
    dryWetSmooth.setSmoothingTime (sr, 10.0f);

    reset();
}

void DelayEffect::reset()
{
    std::fill (delayLineL.begin(), delayLineL.end(), 0.0f);
    std::fill (delayLineR.begin(), delayLineR.end(), 0.0f);
    writePos = 0;
    feedbackL = 0.0f;
    feedbackR = 0.0f;
}

float DelayEffect::readDelayInterpolated (const std::vector<float>& line, float delaySamples)
{
    float readPosF = static_cast<float> (writePos) - delaySamples;
    while (readPosF < 0)
        readPosF += maxDelaySamples;

    int idx0 = static_cast<int> (readPosF) % maxDelaySamples;
    int idx1 = (idx0 + 1) % maxDelaySamples;
    float frac = readPosF - std::floor (readPosF);

    return line[idx0] * (1.0f - frac) + line[idx1] * frac;
}

void DelayEffect::processSample (float& left, float& right)
{
    if (! enabled || maxDelaySamples == 0)
        return;

    // Get smoothed parameter values
    float delayTimeMs = delayTimeMsSmooth.getNextValue();
    float feedback = feedbackSmooth.getNextValue();
    float dryWet = dryWetSmooth.getNextValue();

    // Calculate delay in samples (use float for interpolation)
    float delaySamples = delayTimeMs * 0.001f * static_cast<float> (sampleRate);
    delaySamples = juce::jlimit (1.0f, static_cast<float> (maxDelaySamples - 1), delaySamples);

    // Read delayed samples with interpolation
    float delayedL = readDelayInterpolated (delayLineL, delaySamples);
    float delayedR = readDelayInterpolated (delayLineR, delaySamples);

    // Store dry signal
    float dryL = left;
    float dryR = right;

    if (pingPong)
    {
        // Ping-pong: left feedback goes to right, right feedback goes to left
        delayLineL[writePos] = left + feedbackR * feedback;
        delayLineR[writePos] = right + feedbackL * feedback;

        feedbackL = delayedL;
        feedbackR = delayedR;
    }
    else
    {
        // Normal stereo delay
        delayLineL[writePos] = left + delayedL * feedback;
        delayLineR[writePos] = right + delayedR * feedback;
    }

    // Advance write position
    writePos = (writePos + 1) % maxDelaySamples;

    // Mix dry/wet
    left = dryL * (1.0f - dryWet) + delayedL * dryWet;
    right = dryR * (1.0f - dryWet) + delayedR * dryWet;
}
