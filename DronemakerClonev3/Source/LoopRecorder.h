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
        int length = 0;           // Actual recorded length in samples
        int playPosition = 0;     // Current playback position
        bool hasContent = false;
    };

    std::array<LoopSlot, numSlots> slots;
    int activeRecordSlot = -1;    // Which slot is currently recording (-1 = none)
    int maxLoopSamples = 0;       // Calculated from sample rate
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoopRecorder)
};
