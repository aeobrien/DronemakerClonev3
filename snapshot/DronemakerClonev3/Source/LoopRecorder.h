#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>
#include <atomic>
#include "LoopAutomation.h"
#include "LoopSequenceExecutor.h"

/**
 * Multi-slot loop recorder for feeding pre-recorded audio into the drone processor.
 * Each slot can have a different loop length and loops independently.
 * The dry loop audio is never heard - only its effect on the drone.
 * Supports per-loop recording settings and post-record automation.
 */
class LoopRecorder
{
public:
    static constexpr int numSlots = 8;
    static constexpr int maxLoopSeconds = 300;  // Maximum possible (5 minutes)

    LoopRecorder();

    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void reset();

    // Recording control
    void startRecording (int slot);
    void stopRecording();
    void clearSlot (int slot);
    void clearAll();

    // Call this for each input sample during audio callback
    void recordSample (float sample);

    // Get mixed output of all active loops (call once per sample)
    float getLoopMix();

    // Split output: droneSend (immediate) and dryOutput (delayed by droneOffset)
    struct LoopMixOutput
    {
        float droneSend = 0.0f;   // Processed audio for drone input (immediate, mono)
        float dryOutputL = 0.0f;  // Delayed/bounced audio for direct output (left)
        float dryOutputR = 0.0f;  // Delayed/bounced audio for direct output (right)
    };
    LoopMixOutput getLoopMixSplit();

    // Global drone offset (seconds of delay between drone input and dry output)
    void setDroneOffset (float seconds) { droneOffsetSeconds = juce::jlimit (0.5f, 8.0f, seconds); }
    float getDroneOffset() const { return droneOffsetSeconds; }

    // Get drone send from recording slot (fed during recording so drone warms up)
    float getRecordingDroneSend() const { return recordingDroneSend; }

    // Per-slot parameter setters
    void setSlotWetSend (int slot, float level);
    void setSlotDrySend (int slot, float level);
    float getSlotWetSend (int slot) const;
    float getSlotDrySend (int slot) const;
    void setSlotVolume (int slot, float vol);
    void setSlotHighPass (int slot, float freqHz);
    void setSlotLowPass (int slot, float freqHz);
    void setSlotPitchOctave (int slot, int octave);  // -1, 0, or +1
    void setSlotTrimStart (int slot, float normalised);  // 0.0–1.0
    void setSlotTrimEnd (int slot, float normalised);    // 0.0–1.0
    float getSlotTrimStart (int slot) const;
    float getSlotTrimEnd (int slot) const;
    int getSlotEffectiveLength (int slot) const;         // trimmed length in samples

    // Modulation offsets (added to base parameters)
    void setSlotVolumeMod (int slot, float mod);
    void setSlotFilterHPMod (int slot, float mod);
    void setSlotFilterLPMod (int slot, float mod);

    // State queries
    bool isSlotActive (int slot) const;
    bool isRecording() const { return activeRecordSlot >= 0; }
    int getRecordingSlot() const { return activeRecordSlot; }
    bool hasAnyContent() const;
    int getSlotLength (int slot) const;
    float getSlotProgress (int slot) const;  // 0.0 to 1.0 playback position

    // Per-slot settings
    void setSlotSettings (int slot, const LoopSettings& settings);
    LoopSettings getSlotSettings (int slot) const;

    // Playback control (separate from hasContent)
    void setSlotPlaying (int slot, bool playing);
    bool isSlotPlaying (int slot) const;

    // Preview automation sequence
    void startPreview (int slot);
    void stopPreview (int slot);
    bool isPreviewRunning (int slot) const;

    // Get automation level for a slot (for UI display)
    float getSlotAutomationLevel (int slot) const;

    // Check if a slot has level-affecting automation commands (SetLevel or RampLevel)
    bool slotHasLevelAutomation (int slot) const;

    // Automation range modifier (constrains automation output)
    void setSlotAutomationRange (int slot, float rangeMin, float rangeMax);
    float getSlotAutomationRangeMin (int slot) const;
    float getSlotAutomationRangeMax (int slot) const;

    // Bounce-in-place
    bool isSlotBounced (int slot) const;
    bool isSlotBouncing (int slot) const;
    void setSlotBounceResult (int slot, std::vector<float>&& left, std::vector<float>&& right, int length);
    void unbounceSlot (int slot);
    void setSlotBouncing (int slot);
    void clearSlotBounceState (int slot);
    void startBounceTransition (int slot, double currentPlayPos);

    // Per-slot parameter getters (for UI indicators)
    float getSlotVolume (int slot) const;
    float getSlotHighPass (int slot) const;
    float getSlotLowPass (int slot) const;
    float getSlotVolumeMod (int slot) const;
    float getSlotHPMod (int slot) const;
    float getSlotLPMod (int slot) const;

    // Get a sample at a specific index from a slot's buffer (for waveform display)
    float getSampleAtIndex (int slot, int index) const;

    // Get the last filtered output sample for a slot (post HP/LP, post volume, updated each getLoopMix call)
    float getSlotLastFilteredSample (int slot) const;

    // Get the current sample rate
    double getSampleRate() const { return currentSampleRate; }

private:
    struct LoopSlot
    {
        std::vector<float> buffer;
        int length = 0;              // Actual recorded length in samples
        double playPosition = 0.0;   // Current playback position (float for pitch shifting)
        bool hasContent = false;
        bool isPlaying = false;      // Separate from hasContent - controlled by automation

        // Per-slot controls
        float volume = 1.0f;
        float hpFreq = 20.0f;        // High-pass cutoff Hz
        float lpFreq = 20000.0f;     // Low-pass cutoff Hz
        int pitchOctave = 0;         // -1, 0, or +1
        float trimStart = 0.0f;      // 0.0–1.0 normalised start point
        float trimEnd   = 1.0f;      // 0.0–1.0 normalised end point

        // Modulation offsets
        float volumeMod = 0.0f;      // Added to volume
        float hpFreqMod = 0.0f;      // Added to hpFreq (in octaves, scaled)
        float lpFreqMod = 0.0f;      // Added to lpFreq (in octaves, scaled)

        // Filter states (simple one-pole filters)
        float hpState = 0.0f;
        float lpState = 0.0f;

        // Cached filter coefficients (updated when params change, not per-sample)
        float cachedHpCoeff = 0.0f;
        float cachedLpCoeff = 1.0f;
        float cachedModulatedVolume = 1.0f;
        double cachedPlaybackRate = 1.0;

        // Wet/dry send levels
        float wetSend = 1.0f;            // How much processed audio goes to drone (0-1)
        float drySend = 1.0f;            // How much delayed processed audio goes to output (0-1)

        // History ring buffer for delayed dry playback
        std::vector<float> historyBuffer;
        int historyWritePos = 0;
        int historyBufferSize = 0;
        bool historyPrimed = false;       // True once buffer has filled to offset length
        int historySamplesFed = 0;        // Samples written since recording started (for priming)

        // Bounce-in-place state
        enum class BounceState { None, Bouncing, Bounced };
        std::atomic<int> bounceState { 0 };  // cast to/from BounceState
        std::vector<float> bouncedL;         // Left channel of bounced drone output
        std::vector<float> bouncedR;         // Right channel
        int bouncedLength = 0;               // Length in samples
        double bounceHeadA = 0.0;            // Playback position for voice A
        double bounceHeadB = 0.0;            // Playback position for voice B
        float wetFadeGain = 1.0f;            // Fading out wet send during transition
        float bounceFadeGain = 0.0f;         // Fading in bounced audio during transition
        bool transitioning = false;

        // Automation
        float lastFilteredSample = 0.0f; // Post-filter output for visualiser
        LoopSettings settings;              // Per-loop recording settings
        LoopSequenceExecutor executor;      // Runtime automation executor
        float automationLevel = 1.0f;       // Current level from automation (0.0-1.0)
        float automationRangeMin = 0.0f;    // Min output level for automation range modifier
        float automationRangeMax = 1.0f;    // Max output level for automation range modifier
        int maxSamplesForSlot = 0;          // Calculated from settings.maxRecordLengthSeconds
    };

    std::array<LoopSlot, numSlots> slots;
    int activeRecordSlot = -1;    // Which slot is currently recording (-1 = none)
    int maxLoopSamples = 0;       // Calculated from sample rate
    double currentSampleRate = 44100.0;
    float droneOffsetSeconds = 4.0f;  // Global delay between drone input and dry output
    float recordingDroneSend = 0.0f;  // Drone send from currently recording slot

    // Helper to get sample with linear interpolation
    float getInterpolatedSample (const LoopSlot& slot, double position) const;

    // Recompute cached filter coefficients for a slot
    void updateSlotCoefficients (LoopSlot& slot);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoopRecorder)
};
