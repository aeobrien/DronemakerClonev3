#include "DistortionEffect.h"
#include <cmath>

DistortionEffect::DistortionEffect()
{
}

void DistortionEffect::prepareToPlay (double sr, int /*samplesPerBlock*/)
{
    sampleRate = sr;
    reset();
}

void DistortionEffect::reset()
{
    dcStateL = dcStateR = 0.0f;
    dcPrevL = dcPrevR = 0.0f;
    toneStateL = toneStateR = 0.0f;
    srCounter = 0;
    holdL = holdR = 0.0f;
}

float DistortionEffect::processSoftClip (float sample)
{
    // Soft clipping using tanh - smooth, tube-like saturation
    // Apply drive as gain before saturation
    sample *= drive;

    // Tanh gives gradual saturation
    return std::tanh (sample);
}

float DistortionEffect::processHardClip (float sample)
{
    // Hard clipping - harsh, transistor-like distortion
    // Apply drive as gain
    sample *= drive;

    // Asymmetric hard clipping for more character
    if (sample > 0.7f)
        sample = 0.7f + (sample - 0.7f) * 0.1f;  // Compress positive peaks
    if (sample < -0.7f)
        sample = -0.7f + (sample + 0.7f) * 0.1f; // Compress negative peaks

    // Final hard limit
    return juce::jlimit (-1.0f, 1.0f, sample);
}

float DistortionEffect::processWavefold (float sample)
{
    // Wavefolder - creates complex harmonics by folding the waveform
    // Apply drive as gain
    sample *= drive;

    // Multi-stage wavefolder for more complex harmonics
    for (int stage = 0; stage < 4; ++stage)
    {
        if (sample > 1.0f)
            sample = 2.0f - sample;
        else if (sample < -1.0f)
            sample = -2.0f - sample;
    }

    return sample;
}

float DistortionEffect::processBitcrush (float sample, float& hold, int& counter)
{
    // Sample rate reduction
    counter++;
    if (counter >= static_cast<int> (srReduction))
    {
        counter = 0;
        hold = sample;
    }

    // Bit depth reduction
    float levels = std::pow (2.0f, bitDepth);
    float quantized = std::round (hold * levels) / levels;

    return quantized;
}

float DistortionEffect::processDCBlock (float sample, float& state, float& prev)
{
    // Simple DC blocker: y[n] = x[n] - x[n-1] + 0.995 * y[n-1]
    float output = sample - prev + 0.995f * state;
    prev = sample;
    state = output;
    return output;
}

void DistortionEffect::processSample (float& left, float& right)
{
    if (! enabled)
        return;

    float dryL = left;
    float dryR = right;

    // Apply distortion algorithm
    float wetL, wetR;

    switch (algorithm)
    {
        case SoftClip:
            wetL = processSoftClip (left);
            wetR = processSoftClip (right);
            break;

        case HardClip:
            wetL = processHardClip (left);
            wetR = processHardClip (right);
            break;

        case Wavefold:
            wetL = processWavefold (left);
            wetR = processWavefold (right);
            break;

        case Bitcrush:
            wetL = processBitcrush (left, holdL, srCounter);
            wetR = processBitcrush (right, holdR, srCounter);
            break;

        default:
            wetL = left;
            wetR = right;
            break;
    }

    // DC blocking
    wetL = processDCBlock (wetL, dcStateL, dcPrevL);
    wetR = processDCBlock (wetR, dcStateR, dcPrevR);

    // Tone filter (one-pole LP, controlled by tone parameter)
    // tone = 0 → dark (low cutoff), tone = 1 → bright (high cutoff)
    float toneFreq = 500.0f + tone * 19500.0f;  // 500Hz to 20kHz
    float toneCoeff = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * toneFreq / static_cast<float> (sampleRate));

    toneStateL += toneCoeff * (wetL - toneStateL);
    toneStateR += toneCoeff * (wetR - toneStateR);

    wetL = toneStateL;
    wetR = toneStateR;

    // Dry/wet mix
    left = dryL * (1.0f - dryWet) + wetL * dryWet;
    right = dryR * (1.0f - dryWet) + wetR * dryWet;
}
