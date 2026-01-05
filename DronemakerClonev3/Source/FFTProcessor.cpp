#include "FFTProcessor.h"
#include <complex>
#include <vector>
#include <cmath>

FFTProcessor::FFTProcessor() :
    fft(fftOrder),
    window(fftSize + 1, juce::dsp::WindowingFunction<float>::WindowingMethod::hann, false)
{
    // Window is size fftSize+1 because JUCE windows are symmetrical.
    // For overlap-add we need periodic, so we use 2049 but only first 2048 samples.
}

void FFTProcessor::reset()
{
    count = 0;
    pos = 0;

    std::fill(inputFifo.begin(), inputFifo.end(), 0.0f);
    std::fill(outputFifo.begin(), outputFifo.end(), 0.0f);

    // Reset spectral history ring buffer
    for (auto& frame : magHistory)
        std::fill(frame.begin(), frame.end(), 0.0f);
    historyWritePos = 0;

    // Reset EMA smoothing state
    std::fill(smoothedMag.begin(), smoothedMag.end(), 0.0f);
}

void FFTProcessor::setHistorySeconds(float sec)
{
    // Calculate how many frames fit in the requested duration
    // frames = seconds * sampleRate / hopSize
    float framesNeeded = sec * sampleRate / float(hopSize);
    activeHistoryFrames = juce::jlimit(1, historyFrames, (int) framesNeeded);
}

bool FFTProcessor::isFrequencyInScale(float freq) const
{
    if (freq < 20.0f) return true;  // Allow very low frequencies through

    // Scale intervals (semitones from root that are included)
    // 0=Octaves, 1=Fifths, 2=Ionian, 3=Dorian, 4=Phrygian, 5=Lydian, 6=Mixolydian, 7=Aeolian, 8=Locrian, 9=Pent Maj, 10=Pent Min
    static const std::vector<std::vector<int>> scaleIntervals = {
        {0},                            // Octaves (root only)
        {0, 7},                         // Fifths (root + fifth)
        {0, 2, 4, 5, 7, 9, 11},        // Ionian (Major)
        {0, 2, 3, 5, 7, 9, 10},        // Dorian
        {0, 1, 3, 5, 7, 8, 10},        // Phrygian
        {0, 2, 4, 6, 7, 9, 11},        // Lydian
        {0, 2, 4, 5, 7, 9, 10},        // Mixolydian
        {0, 2, 3, 5, 7, 8, 10},        // Aeolian (Natural Minor)
        {0, 1, 3, 5, 6, 8, 10},        // Locrian
        {0, 2, 4, 7, 9},               // Pentatonic Major
        {0, 3, 5, 7, 10}               // Pentatonic Minor
    };

    // Convert frequency to MIDI note number
    // MIDI note 69 = A4 = 440 Hz
    float midiNoteF = 69.0f + 12.0f * std::log2(freq / 440.0f);
    int midiNote = (int) std::round(midiNoteF);

    // Get pitch class (0-11, where 0=C, 1=C#, ..., 9=A, 10=A#, 11=B)
    int pitchClass = ((midiNote % 12) + 12) % 12;

    // Calculate interval from root note
    int intervalFromRoot = ((pitchClass - harmonicRootNote) + 12) % 12;

    // Check if this interval is in the current scale
    const auto& intervals = scaleIntervals[harmonicScaleType];
    for (int interval : intervals)
    {
        if (intervalFromRoot == interval)
            return true;
    }

    return false;
}

float FFTProcessor::processSample(float sample, bool bypassed)
{
    // Push the new sample into the input FIFO
    inputFifo[pos] = sample;

    // Read the output sample from the output FIFO
    // (Initially zeros, then delayed/processed audio once frames are processed)
    float outputSample = outputFifo[pos];

    // Clear this position for the next overlap-add cycle
    outputFifo[pos] = 0.0f;

    // Advance the FIFO position (circular buffer)
    pos += 1;
    if (pos == fftSize)
        pos = 0;

    // Process an FFT frame every hopSize samples
    count += 1;
    if (count == hopSize)
    {
        count = 0;
        processFrame(bypassed);
    }

    return outputSample;
}

void FFTProcessor::processFrame(bool bypassed)
{
    juce::ignoreUnused(bypassed);

    const float* inputPtr = inputFifo.data();
    float* fftPtr = fftData.data();

    // Copy from circular inputFifo to linear fftData
    // The oldest sample is at position 'pos', wrapping around
    std::memcpy(fftPtr, inputPtr + pos, (fftSize - pos) * sizeof(float));
    if (pos > 0)
        std::memcpy(fftPtr + fftSize - pos, inputPtr, pos * sizeof(float));

    // Apply analysis window (Hann) to reduce spectral leakage
    window.multiplyWithWindowingTable(fftPtr, fftSize);

    // Forward FFT: time domain -> frequency domain
    // Result is interleaved complex: [re0, im0, re1, im1, ...]
    fft.performRealOnlyForwardTransform(fftPtr, true);

    // Spectral processing (magnitude/phase manipulation)
    processSpectrum(fftPtr, numBins);

    // Inverse FFT: frequency domain -> time domain
    fft.performRealOnlyInverseTransform(fftPtr);

    // Apply synthesis window (Hann) for reconstruction
    window.multiplyWithWindowingTable(fftPtr, fftSize);

    // Scale for overlap-add (compensates for overlapping Hann windows)
    for (int i = 0; i < fftSize; ++i)
        fftPtr[i] *= windowCorrection;

    // Overlap-add: accumulate into output FIFO
    // First part: from current pos to end of buffer
    for (int i = 0; i < fftSize - pos; ++i)
        outputFifo[pos + i] += fftData[i];

    // Second part: wrap around to beginning of buffer
    for (int i = 0; i < pos; ++i)
        outputFifo[i] += fftData[fftSize - pos + i];
}

void FFTProcessor::processSpectrum(float* data, int numBinsParam)
{
    juce::ignoreUnused(numBinsParam);

    // Treat interleaved [re, im, re, im, ...] as complex numbers
    auto* cdata = reinterpret_cast<std::complex<float>*>(data);

    // Calculate filter bin indices
    const int highPassBin = (int)(highPassFreq * float(fftSize) / sampleRate);
    const int lowPassBin = (int)(lowPassFreq * float(fftSize) / sampleRate);

    // First pass: store current magnitudes in history
    for (int i = 0; i < numBins; ++i)
    {
        float magnitude = std::abs(cdata[i]);
        magHistory[historyWritePos][i] = magnitude;
    }

    // Second pass: generate output with pitch shifting
    for (int i = 0; i < numBins; ++i)
    {
        float binFreq = float(i) * sampleRate / float(fftSize);

        // -------- High/Low Pass Filtering with variable slope --------
        float filterGain = 1.0f;

        // High pass: attenuate below cutoff frequency
        if (highPassFreq > 0.0f && binFreq < highPassFreq)
        {
            float ratio = binFreq / highPassFreq;
            filterGain *= std::pow(ratio, highPassSlope);
        }

        // Low pass: attenuate above cutoff frequency
        if (binFreq > lowPassFreq)
        {
            float ratio = lowPassFreq / binFreq;
            filterGain *= std::pow(ratio, lowPassSlope);
        }

        // Hard cutoff for extreme cases
        if (filterGain < 0.001f)
        {
            cdata[i] = std::complex<float>(0.0f, 0.0f);
            continue;
        }

        // -------- Harmonic Comb Filter with intensity --------
        float harmonicGain = 1.0f;
        if (harmonicFilterEnabled)
        {
            if (!isFrequencyInScale(binFreq))
            {
                // Blend based on intensity: 0 = no filtering, 1 = full filtering
                harmonicGain = 1.0f - harmonicIntensity;
            }
        }

        float totalFilterGain = filterGain * harmonicGain;

        // -------- Pitch Shifting: read from shifted source bin --------
        // To shift UP, read from LOWER bins; to shift DOWN, read from HIGHER bins
        float sourceBinF = float(i) / pitchShiftRatio;
        int sourceBin = (int)sourceBinF;
        float frac = sourceBinF - float(sourceBin);

        // Get current phase for this output bin
        float phase = std::arg(cdata[i]);

        // Interpolate magnitude from history at shifted position
        float oldMag = 0.0f;

        if (sourceBin >= 0 && sourceBin < numBins - 1)
        {
            if (usePeakAmplitudes)
            {
                // Find peak magnitude across active history window (with interpolation)
                float peak0 = 0.0f, peak1 = 0.0f;
                for (int f = 0; f < activeHistoryFrames; ++f)
                {
                    int frameIdx = (historyWritePos - f + historyFrames) % historyFrames;
                    peak0 = std::max(peak0, magHistory[frameIdx][sourceBin]);
                    peak1 = std::max(peak1, magHistory[frameIdx][sourceBin + 1]);
                }
                oldMag = peak0 * (1.0f - frac) + peak1 * frac;
            }
            else
            {
                // Use oldest frame in active window
                int oldFrameIdx = (historyWritePos - activeHistoryFrames + 1 + historyFrames) % historyFrames;
                float mag0 = magHistory[oldFrameIdx][sourceBin];
                float mag1 = magHistory[oldFrameIdx][sourceBin + 1];
                oldMag = mag0 * (1.0f - frac) + mag1 * frac;
            }
        }

        // Current magnitude at this bin (no pitch shift for input)
        float newMag = std::abs(cdata[i]);

        // Blend between old (sustained drone) and new (current input)
        // droneDelay controls how much we favor pure-delayed vs blended output
        float blendedMag = oldMag * (1.0f - interpFactor) + newMag * interpFactor;
        float pureMag = oldMag;  // Pure delayed signal (true DroneMaker style)
        float interpMag = blendedMag * (1.0f - droneDelay) + pureMag * droneDelay;

        // -------- Exponential Moving Average smoothing --------
        smoothedMag[i] = smoothedMag[i] * (1.0f - smoothingFactor) + interpMag * smoothingFactor;

        // -------- Decay: gradually reduce magnitude when input is quiet --------
        smoothedMag[i] *= decayRate;

        float smoothMag = smoothedMag[i];

        // -------- Phase randomization --------
        float outputPhase;
        if (randomizePhases)
            outputPhase = random.nextFloat() * juce::MathConstants<float>::twoPi - juce::MathConstants<float>::pi;
        else
            outputPhase = phase;

        // -------- Threshold gating with spectral tilt --------
        float effectiveThreshold = threshold;
        if (std::abs(spectralTilt) > 0.001f && binFreq > 20.0f)
        {
            // Reference frequency (middle of audible range)
            const float refFreq = 1000.0f;
            // Calculate octaves from reference
            float octavesFromRef = std::log2(binFreq / refFreq);
            // Convert dB/octave tilt to linear multiplier
            float tiltMultiplier = std::pow(10.0f, (spectralTilt * octavesFromRef) / 20.0f);
            effectiveThreshold = threshold * tiltMultiplier;
        }
        float finalMag = (smoothMag > effectiveThreshold) ? smoothMag : 0.0f;

        // Apply filter gains
        finalMag *= totalFilterGain;

        cdata[i] = std::polar(finalMag, outputPhase);
    }

    // Advance history write position (ring buffer)
    historyWritePos = (historyWritePos + 1) % historyFrames;
}
