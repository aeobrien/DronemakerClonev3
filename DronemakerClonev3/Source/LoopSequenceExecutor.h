#pragma once

#include "LoopAutomation.h"
#include <atomic>
#include <cmath>

/**
 * Executes automation sequences sample-by-sample in the audio thread.
 * Manages playback enable/disable state and current level.
 * Thread-safe via atomics for state queries from UI thread.
 */
class LoopSequenceExecutor
{
public:
    LoopSequenceExecutor();

    // Start executing a sequence (copies the sequence for thread safety)
    void startSequence (const Sequence& seq, double sampleRate);

    // Process one sample, returns current automation level (0.0-1.0)
    float processSample (double sampleRate);

    // Stop and reset the executor
    void cancel();

    // Query current state (thread-safe)
    bool isRunning() const { return running.load(); }
    bool isPlaybackEnabled() const { return playbackEnabled.load(); }
    float getCurrentLevel() const { return currentLevel.load(); }

private:
    // Sequence state (only accessed from audio thread during execution)
    Sequence activeSequence;
    int currentCommandIndex = 0;
    double commandProgress = 0.0;     // Progress through current command (in samples)
    double commandDuration = 0.0;     // Total duration of current command (in samples)
    float rampStartLevel = 1.0f;      // Starting level for ramp commands

    // Thread-safe state
    std::atomic<bool> running { false };
    std::atomic<bool> playbackEnabled { false };
    std::atomic<float> currentLevel { 1.0f };

    // Advance to the next command
    void advanceToNextCommand (double sampleRate);

    // Execute immediate commands (StartPlayback, StopPlayback, SetLevel)
    // Returns true if the command was immediate and we should move to the next
    bool executeImmediateCommand (const Command& cmd);

    // Begin a timed command (Wait, RampLevel)
    void beginTimedCommand (const Command& cmd, double sampleRate);

    // Apply equal power curve
    static float equalPowerCurve (float progress, bool fadeIn);
};
