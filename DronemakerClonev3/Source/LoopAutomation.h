#pragma once

#include <JuceHeader.h>
#include <vector>
#include <variant>

/**
 * Data structures for per-loop recording settings and post-record automation.
 * Allows configurable recording lengths and automated actions after recording stops.
 */

// Ramp curve types for level changes
enum class RampCurve
{
    Linear,      // Linear interpolation
    EqualPower   // sqrt-based curve for perceptually even fades
};

// Command types for automation sequences
struct StartPlayback {};
struct StopPlayback {};
struct Wait { float durationSeconds = 1.0f; };
struct SetLevel { float targetLevel = 1.0f; };  // 0.0-1.0
struct RampLevel
{
    float durationSeconds = 1.0f;
    float targetLevel = 1.0f;
    RampCurve curve = RampCurve::Linear;
};

// Command variant type - can be any of the above
using Command = std::variant<StartPlayback, StopPlayback, Wait, SetLevel, RampLevel>;

// A sequence of commands to execute
struct Sequence
{
    std::vector<Command> commands;
    bool loopSequence = false;  // true = repeat continuously after completing

    // Default constructor creates a simple "start playback" sequence
    Sequence()
    {
        commands.push_back (StartPlayback{});
    }

    // Clear and reset to default
    void reset()
    {
        commands.clear();
        commands.push_back (StartPlayback{});
        loopSequence = false;
    }

    // Check if this is the default sequence (just StartPlayback)
    bool isDefault() const
    {
        return commands.size() == 1 &&
               std::holds_alternative<StartPlayback> (commands[0]) &&
               !loopSequence;
    }
};

// Per-loop settings including max recording length and automation
struct LoopSettings
{
    float maxRecordLengthSeconds = 60.0f;  // Range: 5.0 to 300.0
    Sequence postRecordSequence;           // Default: just StartPlayback

    // Reset to defaults
    void reset()
    {
        maxRecordLengthSeconds = 60.0f;
        postRecordSequence.reset();
    }
};
