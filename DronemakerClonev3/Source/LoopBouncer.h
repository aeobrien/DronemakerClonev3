#pragma once

#include <JuceHeader.h>
#include "FFTProcessor.h"
#include <vector>
#include <atomic>
#include <functional>

/**
 * Snapshot of all drone/FFT parameters at bounce time.
 * Used to configure fresh offline FFTProcessor instances.
 */
struct DroneParameterSnapshot
{
    float sampleRate = 44100.0f;
    float interpFactor = 0.2f;
    float droneDelay = 0.0f;
    float smoothingFactor = 0.05f;
    float threshold = 0.001f;
    float spectralTilt = 0.0f;
    bool randomizePhases = true;
    bool usePeakAmplitudes = true;
    float pitchShiftSemitones = 0.0f;
    float pitchShiftOctaves = 0.0f;
    float highPassFreq = 0.0f;
    float lowPassFreq = 20000.0f;
    float highPassSlope = 1.0f;
    float lowPassSlope = 1.0f;
    float decayRate = 1.0f;
    float historySeconds = 10.0f;
    bool harmonicFilterEnabled = false;
    int harmonicRootNote = 0;
    int harmonicScaleType = 0;
    float harmonicIntensity = 1.0f;

    /** Capture a snapshot from a live FFTProcessor. */
    static DroneParameterSnapshot fromProcessor (const FFTProcessor& proc)
    {
        DroneParameterSnapshot s;
        s.sampleRate = proc.getSampleRate();
        s.interpFactor = proc.getInterpFactor();
        s.droneDelay = proc.getDroneDelay();
        s.smoothingFactor = proc.getSmoothingFactor();
        s.threshold = proc.getThreshold();
        s.spectralTilt = proc.getSpectralTilt();
        s.randomizePhases = proc.getRandomizePhases();
        s.usePeakAmplitudes = proc.getUsePeakAmplitudes();
        s.pitchShiftSemitones = proc.getPitchShiftSemitones();
        s.pitchShiftOctaves = proc.getPitchShiftOctaves();
        s.highPassFreq = proc.getHighPassFreq();
        s.lowPassFreq = proc.getLowPassFreq();
        s.highPassSlope = proc.getHighPassSlope();
        s.lowPassSlope = proc.getLowPassSlope();
        s.decayRate = proc.getDecayRate();
        s.historySeconds = proc.getHistorySeconds();
        s.harmonicFilterEnabled = proc.getHarmonicFilterEnabled();
        s.harmonicRootNote = proc.getHarmonicRootNote();
        s.harmonicScaleType = proc.getHarmonicScaleType();
        s.harmonicIntensity = proc.getHarmonicIntensity();
        return s;
    }

    /** Apply snapshot to a fresh FFTProcessor instance. */
    void applyTo (FFTProcessor& proc) const
    {
        proc.setSampleRate (sampleRate);
        proc.setInterpFactor (interpFactor);
        proc.setDroneDelay (droneDelay);
        proc.setSmoothingFactor (smoothingFactor);
        proc.setThreshold (threshold);
        proc.setSpectralTilt (spectralTilt);
        proc.setRandomizePhases (randomizePhases);
        proc.setUsePeakAmplitudes (usePeakAmplitudes);
        proc.setPitchShiftSemitones (pitchShiftSemitones);
        proc.setPitchShiftOctaves (pitchShiftOctaves);
        proc.setHighPassFreq (highPassFreq);
        proc.setLowPassFreq (lowPassFreq);
        proc.setHighPassSlope (highPassSlope);
        proc.setLowPassSlope (lowPassSlope);
        proc.setDecayRate (decayRate);
        proc.setHistorySeconds (historySeconds);
        proc.setHarmonicFilterEnabled (harmonicFilterEnabled);
        proc.setHarmonicRootNote (harmonicRootNote);
        proc.setHarmonicScaleType (harmonicScaleType);
        proc.setHarmonicIntensity (harmonicIntensity);
        proc.reset();
    }
};

/**
 * Background thread that bounces a loop through offline FFT processing.
 * Creates two fresh FFTProcessor instances (L/R stereo), processes the loop
 * for 2x its length, and extracts the second half (skipping FFT startup transient).
 *
 * Only one bounce can run at a time.
 */
class LoopBouncer : public juce::Thread
{
public:
    LoopBouncer() : juce::Thread ("LoopBouncer") {}
    ~LoopBouncer() override { stopThread (5000); }

    /**
     * Start a bounce for the given slot.
     * @param slot           Slot index
     * @param loopBuffer     Raw mono loop samples (copy of the buffer)
     * @param loopLength     Number of valid samples
     * @param trimStart      Normalised trim start (0-1)
     * @param trimEnd        Normalised trim end (0-1)
     * @param pitchOctave    Pitch shift in octaves (-2 to +2)
     * @param params         Snapshot of current drone parameters
     * @param onComplete     Callback (on message thread) when bounce finishes
     */
    void startBounce (int slot,
                      std::vector<float> loopBuffer,
                      int loopLength,
                      float trimStart,
                      float trimEnd,
                      int pitchOctave,
                      const DroneParameterSnapshot& params,
                      std::function<void (int, std::vector<float>&&, std::vector<float>&&, int)> onComplete)
    {
        // Cancel any existing bounce
        if (isThreadRunning())
        {
            signalThreadShouldExit();
            waitForThreadToExit (3000);
        }

        activeSlot.store (slot);
        progress.store (0.0f);
        inputBuffer = std::move (loopBuffer);
        inputLength = loopLength;
        inputTrimStart = trimStart;
        inputTrimEnd = trimEnd;
        inputPitchOctave = pitchOctave;
        snapshot = params;
        completionCallback = std::move (onComplete);

        startThread (juce::Thread::Priority::low);
    }

    void run() override
    {
        // Calculate trim region
        int startSample = static_cast<int> (inputTrimStart * inputLength);
        int endSample = static_cast<int> (inputTrimEnd * inputLength);
        int effectiveLen = juce::jmax (1, endSample - startSample);

        // Total processing: 2x the effective loop length
        int totalSamples = effectiveLen * 2;

        // Create fresh FFT processors
        auto fftL = std::make_unique<FFTProcessor>();
        auto fftR = std::make_unique<FFTProcessor>();
        snapshot.applyTo (*fftL);
        snapshot.applyTo (*fftR);

        // Output buffers for full 2x processing
        std::vector<float> fullL (totalSamples, 0.0f);
        std::vector<float> fullR (totalSamples, 0.0f);

        // Playback rate
        double playbackRate = std::pow (2.0, inputPitchOctave);
        double readPos = static_cast<double> (startSample);

        // Process in batches
        constexpr int batchSize = 512;

        for (int i = 0; i < totalSamples; i += batchSize)
        {
            if (threadShouldExit())
                return;

            int end = juce::jmin (i + batchSize, totalSamples);

            for (int s = i; s < end; ++s)
            {
                // Read from loop with interpolation (unity gain, no filters)
                int idx0 = static_cast<int> (readPos);
                float frac = static_cast<float> (readPos - idx0);

                // Wrap within trim region
                while (idx0 >= endSample)
                    idx0 -= effectiveLen;
                while (idx0 < startSample)
                    idx0 += effectiveLen;

                int idx1 = idx0 + 1;
                if (idx1 >= endSample)
                    idx1 = startSample;

                float sample = inputBuffer[idx0] * (1.0f - frac) + inputBuffer[idx1] * frac;

                // Process through both FFT processors
                fullL[s] = fftL->processSample (sample, false);
                fullR[s] = fftR->processSample (sample, false);

                // Advance read position within trim region
                readPos += playbackRate;
                if (readPos >= endSample)
                    readPos = startSample + std::fmod (readPos - startSample, static_cast<double> (effectiveLen));
            }

            // Update progress
            progress.store (static_cast<float> (end) / static_cast<float> (totalSamples));

            // Yield to audio thread
            juce::Thread::wait (1);
        }

        // Extract second half (skip FFT startup transient)
        std::vector<float> resultL (effectiveLen);
        std::vector<float> resultR (effectiveLen);
        std::copy (fullL.begin() + effectiveLen, fullL.end(), resultL.begin());
        std::copy (fullR.begin() + effectiveLen, fullR.end(), resultR.begin());

        // Diagnostic: check RMS of bounced output
        {
            float sumSq = 0.0f;
            for (int s = 0; s < effectiveLen; ++s)
                sumSq += resultL[s] * resultL[s] + resultR[s] * resultR[s];
            float rms = std::sqrt (sumSq / (effectiveLen * 2));
            std::cerr << "[Bounce] Complete: slot=" << activeSlot.load()
                      << " len=" << effectiveLen
                      << " rmsL+R=" << rms
                      << " peak=" << *std::max_element (resultL.begin(), resultL.end())
                      << std::endl;
        }

        // Free the large temp buffers and FFT processors immediately
        fullL.clear();
        fullR.clear();
        fftL.reset();
        fftR.reset();

        // Post result to message thread
        int resultSlot = activeSlot.load();
        int resultLen = effectiveLen;
        auto callback = completionCallback;

        juce::MessageManager::callAsync ([callback, resultSlot,
                                          movedL = std::move (resultL),
                                          movedR = std::move (resultR),
                                          resultLen]() mutable {
            if (callback)
                callback (resultSlot, std::move (movedL), std::move (movedR), resultLen);
        });

        progress.store (1.0f);
    }

    std::atomic<float> progress { 0.0f };
    std::atomic<int> activeSlot { -1 };

private:
    std::vector<float> inputBuffer;
    int inputLength = 0;
    float inputTrimStart = 0.0f;
    float inputTrimEnd = 1.0f;
    int inputPitchOctave = 0;
    DroneParameterSnapshot snapshot;
    std::function<void (int, std::vector<float>&&, std::vector<float>&&, int)> completionCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoopBouncer)
};
