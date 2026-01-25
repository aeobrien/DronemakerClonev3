#include "LoopSequenceExecutor.h"

LoopSequenceExecutor::LoopSequenceExecutor()
{
}

void LoopSequenceExecutor::startSequence (const Sequence& seq, double sampleRate)
{
    // Copy the sequence for thread-safe execution
    activeSequence = seq;
    currentCommandIndex = 0;
    commandProgress = 0.0;
    commandDuration = 0.0;
    rampStartLevel = currentLevel.load();

    // Default state at start of sequence
    playbackEnabled.store (false);
    currentLevel.store (1.0f);

    running.store (true);

    // Process initial immediate commands
    while (currentCommandIndex < (int) activeSequence.commands.size())
    {
        const auto& cmd = activeSequence.commands[currentCommandIndex];
        if (executeImmediateCommand (cmd))
        {
            currentCommandIndex++;
        }
        else
        {
            // Hit a timed command, set it up
            beginTimedCommand (cmd, sampleRate);
            break;
        }
    }

    // If we've run through all commands and none were timed, we're done
    if (currentCommandIndex >= (int) activeSequence.commands.size())
    {
        if (activeSequence.loopSequence)
        {
            currentCommandIndex = 0;
            // Process immediate commands at start of loop
            while (currentCommandIndex < (int) activeSequence.commands.size())
            {
                const auto& cmd = activeSequence.commands[currentCommandIndex];
                if (executeImmediateCommand (cmd))
                    currentCommandIndex++;
                else
                {
                    beginTimedCommand (cmd, sampleRate);
                    break;
                }
            }
        }
        else
        {
            running.store (false);
        }
    }
}

float LoopSequenceExecutor::processSample (double sampleRate)
{
    if (!running.load())
        return currentLevel.load();

    // If we're processing a timed command
    if (commandDuration > 0.0)
    {
        commandProgress += 1.0;

        // Check if we're on a RampLevel command
        if (currentCommandIndex < (int) activeSequence.commands.size())
        {
            const auto& cmd = activeSequence.commands[currentCommandIndex];
            if (std::holds_alternative<RampLevel> (cmd))
            {
                const auto& ramp = std::get<RampLevel> (cmd);
                float progress = static_cast<float> (commandProgress / commandDuration);
                progress = std::min (1.0f, std::max (0.0f, progress));

                float newLevel;
                if (ramp.curve == RampCurve::EqualPower)
                {
                    // Determine if fade in or fade out
                    bool fadeIn = ramp.targetLevel > rampStartLevel;
                    newLevel = rampStartLevel + (ramp.targetLevel - rampStartLevel) * equalPowerCurve (progress, fadeIn);
                }
                else
                {
                    // Linear interpolation
                    newLevel = rampStartLevel + (ramp.targetLevel - rampStartLevel) * progress;
                }
                currentLevel.store (newLevel);
            }
        }

        // Check if command is complete
        if (commandProgress >= commandDuration)
        {
            advanceToNextCommand (sampleRate);
        }
    }

    return currentLevel.load();
}

void LoopSequenceExecutor::cancel()
{
    running.store (false);
    playbackEnabled.store (false);
    currentLevel.store (1.0f);
    currentCommandIndex = 0;
    commandProgress = 0.0;
    commandDuration = 0.0;
}

void LoopSequenceExecutor::advanceToNextCommand (double sampleRate)
{
    currentCommandIndex++;
    commandProgress = 0.0;
    commandDuration = 0.0;

    // Check if sequence is complete
    if (currentCommandIndex >= (int) activeSequence.commands.size())
    {
        if (activeSequence.loopSequence)
        {
            currentCommandIndex = 0;
        }
        else
        {
            running.store (false);
            return;
        }
    }

    // Process commands until we hit a timed one
    while (currentCommandIndex < (int) activeSequence.commands.size())
    {
        const auto& cmd = activeSequence.commands[currentCommandIndex];
        if (executeImmediateCommand (cmd))
        {
            currentCommandIndex++;
            if (currentCommandIndex >= (int) activeSequence.commands.size())
            {
                if (activeSequence.loopSequence)
                    currentCommandIndex = 0;
                else
                {
                    running.store (false);
                    return;
                }
            }
        }
        else
        {
            beginTimedCommand (cmd, sampleRate);
            break;
        }
    }
}

bool LoopSequenceExecutor::executeImmediateCommand (const Command& cmd)
{
    if (std::holds_alternative<StartPlayback> (cmd))
    {
        playbackEnabled.store (true);
        return true;
    }
    else if (std::holds_alternative<StopPlayback> (cmd))
    {
        playbackEnabled.store (false);
        return true;
    }
    else if (std::holds_alternative<SetLevel> (cmd))
    {
        const auto& setLevel = std::get<SetLevel> (cmd);
        currentLevel.store (setLevel.targetLevel);
        return true;
    }

    // Wait and RampLevel are timed commands
    return false;
}

void LoopSequenceExecutor::beginTimedCommand (const Command& cmd, double sampleRate)
{
    commandProgress = 0.0;

    if (std::holds_alternative<Wait> (cmd))
    {
        const auto& wait = std::get<Wait> (cmd);
        commandDuration = wait.durationSeconds * sampleRate;
    }
    else if (std::holds_alternative<RampLevel> (cmd))
    {
        const auto& ramp = std::get<RampLevel> (cmd);
        commandDuration = ramp.durationSeconds * sampleRate;
        rampStartLevel = currentLevel.load();
    }
}

float LoopSequenceExecutor::equalPowerCurve (float progress, bool fadeIn)
{
    // Equal power curve using sqrt
    // For fade-in: sqrt(progress) - starts slow, accelerates
    // For fade-out: sqrt(1-progress) inverted - starts fast, decelerates
    if (fadeIn)
    {
        return std::sqrt (progress);
    }
    else
    {
        // For fade out, we want to decelerate
        // 1 - sqrt(1 - progress) gives us a curve that starts fast and slows down
        return 1.0f - std::sqrt (1.0f - progress);
    }
}
