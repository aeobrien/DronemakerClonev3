#include "FilterEffect.h"
#include <cmath>

FilterEffect::FilterEffect()
{
    updateHarmonicFrequencies();
}

void FilterEffect::prepareToPlay (double sr, int /*samplesPerBlock*/)
{
    sampleRate = sr;
    reset();
}

void FilterEffect::reset()
{
    hpStateL.fill (0.0f);
    hpStateR.fill (0.0f);
    lpStateL.fill (0.0f);
    lpStateR.fill (0.0f);

    for (auto& r : resonators)
    {
        r.stateL = 0.0f;
        r.stateR = 0.0f;
    }
}

void FilterEffect::processSample (float& left, float& right)
{
    if (! enabled)
        return;

    const float pi = juce::MathConstants<float>::pi;

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
        float harmonicL = 0.0f;
        float harmonicR = 0.0f;

        // Sum of resonant bandpass filters at scale frequencies
        for (int i = 0; i < numResonators; ++i)
        {
            auto& r = resonators[i];
            float bpCoeff = 1.0f - std::exp (-2.0f * pi * 50.0f / static_cast<float> (sampleRate));  // ~50Hz bandwidth

            // Simple resonator: bandpass around frequency
            float targetCoeff = 1.0f - std::exp (-2.0f * pi * r.freq / static_cast<float> (sampleRate));

            r.stateL += targetCoeff * (left - r.stateL);
            r.stateR += targetCoeff * (right - r.stateR);

            // Bandpass-ish output
            float bpL = (left - r.stateL) * bpCoeff + r.stateL;
            float bpR = (right - r.stateR) * bpCoeff + r.stateR;

            harmonicL += bpL;
            harmonicR += bpR;
        }

        // Normalize by number of resonators
        if (numResonators > 0)
        {
            harmonicL /= static_cast<float> (numResonators);
            harmonicR /= static_cast<float> (numResonators);
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
                resonators[numResonators].stateL = 0.0f;
                resonators[numResonators].stateR = 0.0f;
                numResonators++;
            }
        }
    }
}
