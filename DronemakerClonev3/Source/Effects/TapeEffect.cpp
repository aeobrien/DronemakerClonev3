#include "TapeEffect.h"
#include <cmath>

TapeEffect::TapeEffect()
{
}

void TapeEffect::prepareToPlay (double sr, int /*samplesPerBlock*/)
{
    sampleRate = sr;
    reset();
}

void TapeEffect::reset()
{
    delayLineL.fill (0.0f);
    delayLineR.fill (0.0f);
    writePos = 0;
    wowPhase = 0.0f;
    flutterPhase = 0.0f;
    hfStateL = 0.0f;
    hfStateR = 0.0f;
}

float TapeEffect::processSaturation (float sample)
{
    // Tape saturation using soft clipping with bias
    // Bias affects the asymmetry of the saturation curve
    float biasOffset = (bias - 0.5f) * 0.3f;
    sample += biasOffset;

    // Soft saturation using tanh with adjustable intensity
    // Increased intensity range for more extreme saturation
    float intensity = 1.0f + saturation * 8.0f;  // More extreme range
    sample = std::tanh (sample * intensity) / std::tanh (intensity);

    sample -= biasOffset * 0.5f;  // Partially compensate for bias DC offset

    return sample;
}

float TapeEffect::getInterpolatedDelay (const std::array<float, delayLineSize>& buffer, float delay) const
{
    float readPos = static_cast<float> (writePos) - delay;
    while (readPos < 0)
        readPos += delayLineSize;

    int idx0 = static_cast<int> (readPos) % delayLineSize;
    int idx1 = (idx0 + 1) % delayLineSize;
    float frac = readPos - std::floor (readPos);

    return buffer[idx0] * (1.0f - frac) + buffer[idx1] * frac;
}

void TapeEffect::processSample (float& left, float& right)
{
    if (! enabled)
        return;

    float dryL = left;
    float dryR = right;

    // Apply saturation first
    float wetL = processSaturation (left);
    float wetR = processSaturation (right);

    // Write to delay line
    delayLineL[writePos] = wetL;
    delayLineR[writePos] = wetR;

    // Calculate wow/flutter modulation
    float wowMod = std::sin (wowPhase * juce::MathConstants<float>::twoPi);
    float flutterMod = std::sin (flutterPhase * juce::MathConstants<float>::twoPi);

    // Advance LFO phases using separate rates
    wowPhase += static_cast<float> (wowRate / sampleRate);
    if (wowPhase >= 1.0f) wowPhase -= 1.0f;

    flutterPhase += static_cast<float> (flutterRate / sampleRate);
    if (flutterPhase >= 1.0f) flutterPhase -= 1.0f;

    // Calculate total delay modulation (in samples)
    // Wow: up to ~15ms variation at full depth
    // Flutter: up to ~2ms variation at full depth
    float wowDelaySamples = wowMod * wowDepth * static_cast<float> (sampleRate) * 0.015f;
    float flutterDelaySamples = flutterMod * flutterDepth * static_cast<float> (sampleRate) * 0.002f;
    float totalDelay = 100.0f + wowDelaySamples + flutterDelaySamples;  // Base delay + modulation

    // Clamp delay to valid range
    totalDelay = juce::jlimit (1.0f, static_cast<float> (delayLineSize - 1), totalDelay);

    // Read from delay line with interpolation
    wetL = getInterpolatedDelay (delayLineL, totalDelay);
    wetR = getInterpolatedDelay (delayLineR, totalDelay);

    // Advance write position
    writePos = (writePos + 1) % delayLineSize;

    // Apply HF loss (simple one-pole LP filter)
    // Higher hfLoss = more high frequency attenuation (down to 1kHz at full)
    float hfCutoff = 20000.0f - hfLoss * 19000.0f;  // 20kHz to 1kHz
    float hfCoeff = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * hfCutoff / static_cast<float> (sampleRate));

    hfStateL += hfCoeff * (wetL - hfStateL);
    hfStateR += hfCoeff * (wetR - hfStateR);

    wetL = hfStateL;
    wetR = hfStateR;

    // Dry/wet mix
    left = dryL * (1.0f - dryWet) + wetL * dryWet;
    right = dryR * (1.0f - dryWet) + wetR * dryWet;
}
