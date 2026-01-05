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

void DelayEffect::processSample (float& left, float& right)
{
    if (! enabled || maxDelaySamples == 0)
        return;

    // Calculate delay in samples
    int delaySamples = static_cast<int> (delayTimeMs * 0.001f * sampleRate);
    delaySamples = juce::jlimit (1, maxDelaySamples - 1, delaySamples);

    // Read position
    int readPos = writePos - delaySamples;
    if (readPos < 0)
        readPos += maxDelaySamples;

    // Read delayed samples
    float delayedL = delayLineL[readPos];
    float delayedR = delayLineR[readPos];

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
