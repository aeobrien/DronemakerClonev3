#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>
#include <atomic>

/**
 * Multi-slot loop recorder for feeding pre-recorded audio into the drone processor.
 * Each slot can have a different loop length and loops independently.
 * The dry loop audio is never heard - only its effect on the drone.
 */
class LoopRecorder
{
public:
    static constexpr int numSlots = 4;
    static constexpr int maxLoopSeconds = 30;

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

    // Per-slot parameter setters
    void setSlotVolume (int slot, float vol);
    void setSlotHighPass (int slot, float freqHz);
    void setSlotLowPass (int slot, float freqHz);
    void setSlotPitchOctave (int slot, int octave);  // -1, 0, or +1

    // State queries
    bool isSlotActive (int slot) const;
    bool isRecording() const { return activeRecordSlot >= 0; }
    int getRecordingSlot() const { return activeRecordSlot; }
    bool hasAnyContent() const;
    int getSlotLength (int slot) const;
    float getSlotProgress (int slot) const;  // 0.0 to 1.0 playback position

private:
    struct LoopSlot
    {
        std::vector<float> buffer;
        int length = 0;              // Actual recorded length in samples
        double playPosition = 0.0;   // Current playback position (float for pitch shifting)
        bool hasContent = false;

        // Per-slot controls
        float volume = 1.0f;
        float hpFreq = 20.0f;        // High-pass cutoff Hz
        float lpFreq = 20000.0f;     // Low-pass cutoff Hz
        int pitchOctave = 0;         // -1, 0, or +1

        // Filter states (simple one-pole filters)
        float hpState = 0.0f;
        float lpState = 0.0f;
    };

    std::array<LoopSlot, numSlots> slots;
    int activeRecordSlot = -1;    // Which slot is currently recording (-1 = none)
    int maxLoopSamples = 0;       // Calculated from sample rate
    double currentSampleRate = 44100.0;

    // Helper to get sample with linear interpolation
    float getInterpolatedSample (const LoopSlot& slot, double position) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoopRecorder)
};
