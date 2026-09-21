#pragma once

#include "LoopAutomation.h"
#include <vector>

/**
 * Built-in automation presets for quick selection via encoder.
 * Each preset generates a Sequence with default durations that can be
 * scaled by a time multiplier at execution time.
 */
namespace AutomationPresets
{
    struct Preset
    {
        const char* name;
        Sequence (*build)();
    };

    // 0: Off — no automation, just start playback
    inline Sequence buildOff()
    {
        Sequence s;
        s.commands.clear();
        s.commands.push_back (StartPlayback{});
        s.loopSequence = false;
        return s;
    }

    // 1: Fade In — start silent, ramp to full
    inline Sequence buildFadeIn()
    {
        Sequence s;
        s.commands.clear();
        s.commands.push_back (SetLevel { 0.0f });
        s.commands.push_back (StartPlayback{});
        s.commands.push_back (RampLevel { 10.0f, 1.0f, RampCurve::EqualPower });
        s.loopSequence = false;
        return s;
    }

    // 2: Fade Out — start full, ramp to zero, stop
    inline Sequence buildFadeOut()
    {
        Sequence s;
        s.commands.clear();
        s.commands.push_back (StartPlayback{});
        s.commands.push_back (RampLevel { 10.0f, 0.0f, RampCurve::EqualPower });
        s.commands.push_back (StopPlayback{});
        s.loopSequence = false;
        return s;
    }

    // 3: Swell — fade in, hold, fade out, hold (looped)
    inline Sequence buildSwell()
    {
        Sequence s;
        s.commands.clear();
        s.commands.push_back (SetLevel { 0.0f });
        s.commands.push_back (StartPlayback{});
        s.commands.push_back (RampLevel { 8.0f, 1.0f, RampCurve::EqualPower });
        s.commands.push_back (Wait { 7.0f });
        s.commands.push_back (RampLevel { 8.0f, 0.0f, RampCurve::EqualPower });
        s.commands.push_back (Wait { 7.0f });
        s.loopSequence = true;
        return s;
    }

    // 4: Pulse — short fade in/out (looped)
    inline Sequence buildPulse()
    {
        Sequence s;
        s.commands.clear();
        s.commands.push_back (SetLevel { 0.0f });
        s.commands.push_back (StartPlayback{});
        s.commands.push_back (RampLevel { 2.0f, 1.0f, RampCurve::Linear });
        s.commands.push_back (RampLevel { 2.0f, 0.0f, RampCurve::Linear });
        s.loopSequence = true;
        return s;
    }

    // 5: Delayed Entry — wait, then fade in
    inline Sequence buildDelayedEntry()
    {
        Sequence s;
        s.commands.clear();
        s.commands.push_back (SetLevel { 0.0f });
        s.commands.push_back (StartPlayback{});
        s.commands.push_back (Wait { 5.0f });
        s.commands.push_back (RampLevel { 10.0f, 1.0f, RampCurve::EqualPower });
        s.loopSequence = false;
        return s;
    }

    // 6: Fade In & Hold — fade in then stay (one-shot)
    inline Sequence buildFadeInHold()
    {
        Sequence s;
        s.commands.clear();
        s.commands.push_back (SetLevel { 0.0f });
        s.commands.push_back (StartPlayback{});
        s.commands.push_back (RampLevel { 10.0f, 1.0f, RampCurve::EqualPower });
        s.loopSequence = false;
        return s;
    }

    // 7: Fade Out & Stop — ramp down then stop (one-shot)
    inline Sequence buildFadeOutStop()
    {
        Sequence s;
        s.commands.clear();
        s.commands.push_back (StartPlayback{});
        s.commands.push_back (RampLevel { 10.0f, 0.0f, RampCurve::EqualPower });
        s.commands.push_back (StopPlayback{});
        s.loopSequence = false;
        return s;
    }

    // 8: Long Swell — very slow fade cycle (looped)
    inline Sequence buildLongSwell()
    {
        Sequence s;
        s.commands.clear();
        s.commands.push_back (SetLevel { 0.0f });
        s.commands.push_back (StartPlayback{});
        s.commands.push_back (RampLevel { 30.0f, 1.0f, RampCurve::EqualPower });
        s.commands.push_back (Wait { 30.0f });
        s.commands.push_back (RampLevel { 30.0f, 0.0f, RampCurve::EqualPower });
        s.commands.push_back (Wait { 30.0f });
        s.loopSequence = true;
        return s;
    }

    // 9: Stutter — quick on/off cycle (looped)
    inline Sequence buildStutter()
    {
        Sequence s;
        s.commands.clear();
        s.commands.push_back (StartPlayback{});
        s.commands.push_back (SetLevel { 1.0f });
        s.commands.push_back (Wait { 0.8f });
        s.commands.push_back (SetLevel { 0.0f });
        s.commands.push_back (Wait { 0.4f });
        s.loopSequence = true;
        return s;
    }

    inline const Preset presets[] = {
        { "Off",          buildOff },
        { "Fade In",      buildFadeIn },
        { "Fade Out",     buildFadeOut },
        { "Swell",        buildSwell },
        { "Pulse",        buildPulse },
        { "Delay Entry",  buildDelayedEntry },
        { "Fade+Hold",    buildFadeInHold },
        { "Fade+Stop",    buildFadeOutStop },
        { "Long Swell",   buildLongSwell },
        { "Stutter",      buildStutter },
    };

    inline constexpr int numPresets = 10;
}
