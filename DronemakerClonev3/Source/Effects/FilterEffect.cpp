#include "FilterEffect.h"
#include <cmath>

FilterEffect::FilterEffect()
{
    updateHarmonicFrequencies();
}

void FilterEffect::prepareToPlay (double sr, int /*samplesPerBlock*/)
{
    sampleRate = sr;

    // Set up parameter smoothing
    hpFreqSmooth.setSmoothingTime (sr, 20.0f);
    lpFreqSmooth.setSmoothingTime (sr, 20.0f);
    harmonicIntensitySmooth.setSmoothingTime (sr, 30.0f);
    crossfadeSmooth.setSmoothingTime (sr, 100.0f);  // 100ms crossfade for scale/root changes

    reset();
    updateResonatorCoefficients();
}

void FilterEffect::setRootNote (int note)
{
    int newRoot = juce::jlimit (0, 11, note);
    if (newRoot != rootNote)
    {
        // Copy current resonators to old bank for cross-fading
        resonatorsOld = resonators;
        numResonatorsOld = numResonators;
        needsCrossfade = true;
        crossfadeSmooth.setCurrentAndTarget (0.0f);
        crossfadeSmooth.setTargetValue (1.0f);

        rootNote = newRoot;
        updateHarmonicFrequencies();
    }
}

void FilterEffect::setScaleType (int type)
{
    int newType = juce::jlimit (0, 10, type);
    if (newType != scaleType)
    {
        // Copy current resonators to old bank for cross-fading
        resonatorsOld = resonators;
        numResonatorsOld = numResonators;
        needsCrossfade = true;
        crossfadeSmooth.setCurrentAndTarget (0.0f);
        crossfadeSmooth.setTargetValue (1.0f);

        scaleType = newType;
        updateHarmonicFrequencies();
    }
}

void FilterEffect::reset()
{
    hpStateL.fill (0.0f);
    hpStateR.fill (0.0f);
    lpStateL.fill (0.0f);
    lpStateR.fill (0.0f);

    for (auto& r : resonators)
    {
        r.z1L = r.z2L = 0.0f;
        r.z1R = r.z2R = 0.0f;
    }

    for (auto& r : resonatorsOld)
    {
        r.z1L = r.z2L = 0.0f;
        r.z1R = r.z2R = 0.0f;
    }

    needsCrossfade = false;
}

float FilterEffect::processResonatorBank (std::array<Resonator, maxResonators>& bank, int count, float left, float right, float& outR)
{
    float harmonicL = 0.0f;
    float harmonicR = 0.0f;

    for (int i = 0; i < count; ++i)
    {
        auto& r = bank[i];

        // Process left channel through biquad bandpass
        float inL = left;
        float outL = r.b0 * inL + r.b1 * r.z1L + r.b2 * r.z2L - r.a1 * r.z1L - r.a2 * r.z2L;
        r.z2L = r.z1L;
        r.z1L = outL;
        harmonicL += outL;

        // Process right channel through biquad bandpass
        float inR = right;
        float outRVal = r.b0 * inR + r.b1 * r.z1R + r.b2 * r.z2R - r.a1 * r.z1R - r.a2 * r.z2R;
        r.z2R = r.z1R;
        r.z1R = outRVal;
        harmonicR += outRVal;
    }

    // Scale output (resonators have gain based on Q)
    if (count > 0)
    {
        float scale = 2.0f / std::sqrt (static_cast<float> (count));
        harmonicL *= scale;
        harmonicR *= scale;
    }

    outR = harmonicR;
    return harmonicL;
}

void FilterEffect::processSample (float& left, float& right)
{
    if (! enabled)
        return;

    const float pi = juce::MathConstants<float>::pi;

    // Get smoothed parameter values
    float hpFreq = hpFreqSmooth.getNextValue();
    float lpFreq = lpFreqSmooth.getNextValue();
    float harmonicIntensity = harmonicIntensitySmooth.getNextValue();
    float crossfade = crossfadeSmooth.getNextValue();

    // High-pass filter (cascaded one-pole)
    float hpCoeff = 1.0f - std::exp (-2.0f * pi * hpFreq / static_cast<float> (sampleRate));

    float hpL = left;
    float hpR = right;

    for (int p = 0; p < hpPoles; ++p)
    {
        hpStateL[p] += hpCoeff * (hpL - hpStateL[p]);
        hpL = hpL - hpStateL[p];  // HP = input - LP

        hpStateR[p] += hpCoeff * (hpR - hpStateR[p]);
        hpR = hpR - hpStateR[p];
    }

    // Low-pass filter (cascaded one-pole)
    float lpCoeff = 1.0f - std::exp (-2.0f * pi * lpFreq / static_cast<float> (sampleRate));

    float lpL = hpL;
    float lpR = hpR;

    for (int p = 0; p < lpPoles; ++p)
    {
        lpStateL[p] += lpCoeff * (lpL - lpStateL[p]);
        lpL = lpStateL[p];

        lpStateR[p] += lpCoeff * (lpR - lpStateR[p]);
        lpR = lpStateR[p];
    }

    left = lpL;
    right = lpR;

    // Harmonic filter (if enabled)
    if (harmonicEnabled && harmonicIntensity > 0.0f)
    {
        float harmonicL, harmonicR;

        if (needsCrossfade && crossfade < 0.999f)
        {
            // Cross-fade between old and new resonator banks
            float oldR, newR;
            float oldL = processResonatorBank (resonatorsOld, numResonatorsOld, left, right, oldR);
            float newL = processResonatorBank (resonators, numResonators, left, right, newR);

            // Equal-power crossfade
            float oldGain = std::sqrt (1.0f - crossfade);
            float newGain = std::sqrt (crossfade);

            harmonicL = oldL * oldGain + newL * newGain;
            harmonicR = oldR * oldGain + newR * newGain;

            // Check if crossfade is complete
            if (crossfade >= 0.999f)
                needsCrossfade = false;
        }
        else
        {
            // Normal processing with just the current bank
            harmonicL = processResonatorBank (resonators, numResonators, left, right, harmonicR);
            needsCrossfade = false;
        }

        // Blend based on intensity
        left = left * (1.0f - harmonicIntensity) + harmonicL * harmonicIntensity;
        right = right * (1.0f - harmonicIntensity) + harmonicR * harmonicIntensity;
    }
}

void FilterEffect::updateHarmonicFrequencies()
{
    // Scale intervals (semitones from root)
    static const std::vector<std::vector<int>> scaleIntervals = {
        { 0 },                          // 0: Octaves only
        { 0, 7 },                       // 1: Fifths (root + fifth)
        { 0, 2, 4, 5, 7, 9, 11 },        // 2: Ionian (Major)
        { 0, 2, 3, 5, 7, 9, 10 },        // 3: Dorian
        { 0, 1, 3, 5, 7, 8, 10 },        // 4: Phrygian
        { 0, 2, 4, 6, 7, 9, 11 },        // 5: Lydian
        { 0, 2, 4, 5, 7, 9, 10 },        // 6: Mixolydian
        { 0, 2, 3, 5, 7, 8, 10 },        // 7: Aeolian (Natural Minor)
        { 0, 1, 3, 5, 6, 8, 10 },        // 8: Locrian
        { 0, 2, 4, 7, 9 },               // 9: Pentatonic Major
        { 0, 3, 5, 7, 10 }               // 10: Pentatonic Minor
    };

    const auto& intervals = scaleIntervals[scaleType];
    numResonators = 0;

    // Generate frequencies across audible range
    for (int octave = 0; octave < 8 && numResonators < maxResonators; ++octave)
    {
        for (int interval : intervals)
        {
            if (numResonators >= maxResonators)
                break;

            // MIDI note = root + octave * 12 + interval, starting from octave 1
            int midiNote = rootNote + (octave + 1) * 12 + interval;
            float freq = 440.0f * std::pow (2.0f, (midiNote - 69) / 12.0f);

            // Only include audible frequencies
            if (freq >= 20.0f && freq <= 20000.0f)
            {
                resonators[numResonators].freq = freq;
                resonators[numResonators].z1L = 0.0f;
                resonators[numResonators].z2L = 0.0f;
                resonators[numResonators].z1R = 0.0f;
                resonators[numResonators].z2R = 0.0f;
                numResonators++;
            }
        }
    }

    // Update coefficients if we have a valid sample rate
    if (sampleRate > 0)
        updateResonatorCoefficients();
}

void FilterEffect::updateResonatorCoefficients()
{
    const float pi = juce::MathConstants<float>::pi;

    // Q factor determines bandwidth (higher Q = narrower bandwidth)
    // Q of 20-40 gives good resonant character while still allowing some bandwidth
    const float Q = 25.0f;

    for (int i = 0; i < numResonators; ++i)
    {
        auto& r = resonators[i];

        // Calculate biquad bandpass coefficients
        float w0 = 2.0f * pi * r.freq / static_cast<float> (sampleRate);
        float cosW0 = std::cos (w0);
        float sinW0 = std::sin (w0);
        float alpha = sinW0 / (2.0f * Q);

        // Bandpass filter coefficients (constant 0 dB peak gain)
        float a0 = 1.0f + alpha;
        r.b0 = alpha / a0;
        r.b1 = 0.0f;
        r.b2 = -alpha / a0;
        r.a1 = -2.0f * cosW0 / a0;
        r.a2 = (1.0f - alpha) / a0;
    }
}
