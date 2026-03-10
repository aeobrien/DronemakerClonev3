#pragma once
#include <JuceHeader.h>
#include <functional>
#include <array>

//==============================================================================
// Virtual encoder abstraction layer.
// Maps 8 physical encoder CCs + 8 push-button notes to contextual parameter
// bindings that change depending on the active mode (loop/effect/drone/mod).
//==============================================================================
class VirtualEncoderBank
{
public:
    static constexpr int numEncoders = 8;

    // Default MIDI assignments
    static constexpr int defaultCCBase   = 20;   // CC 20-27 for encoders
    static constexpr int defaultNoteBase = 36;    // Notes 36-43 (C2-G2) for push buttons
    static constexpr int loopToggleNoteBase = 44; // Notes 44-51 (G#2-D#3) for loop toggle

    struct EncoderBinding
    {
        juce::String name;
        std::function<void(float)> setValue;   // normalised 0-1 input from MIDI, or actual value from knob
        std::function<float()>     getValue;   // returns current actual value
        double rangeMin = 0.0;
        double rangeMax = 1.0;
        double step     = 0.01;
        juce::String suffix;
        bool bound = false;
    };

    struct ButtonBinding
    {
        std::function<void()> onPress;
        std::function<void()> onRelease;
        bool bound = false;
    };

    //--------------------------------------------------------------------------
    // Binding API — called by update*ParameterKnobs() functions
    //--------------------------------------------------------------------------

    void bindEncoder (int slot, const juce::String& name,
                      double rangeMin, double rangeMax, double step,
                      std::function<void(float)> setValue,
                      std::function<float()> getValue,
                      const juce::String& suffix = "")
    {
        jassert (slot >= 0 && slot < numEncoders);
        auto& enc = encoders[slot];
        enc.name     = name;
        enc.rangeMin = rangeMin;
        enc.rangeMax = rangeMax;
        enc.step     = step;
        enc.setValue  = std::move (setValue);
        enc.getValue = std::move (getValue);
        enc.suffix   = suffix;
        enc.bound    = true;
    }

    void bindButton (int slot,
                     std::function<void()> onPress,
                     std::function<void()> onRelease = nullptr)
    {
        jassert (slot >= 0 && slot < numEncoders);
        auto& btn = buttons[slot];
        btn.onPress   = std::move (onPress);
        btn.onRelease = std::move (onRelease);
        btn.bound     = true;
    }

    void clearAll()
    {
        for (auto& enc : encoders)
            enc = {};
        for (auto& btn : buttons)
            btn = {};
    }

    //--------------------------------------------------------------------------
    // MIDI dispatch — returns true if the message was handled
    //--------------------------------------------------------------------------

    // Handle a CC message. Returns true if it was dispatched to an encoder.
    bool handleCC (int ccNumber, int ccValue)
    {
        int slot = ccToSlot (ccNumber);
        if (slot < 0 || !encoders[slot].bound || !encoders[slot].setValue)
            return false;

        auto& enc = encoders[slot];
        float normValue = ccValue / 127.0f;
        float actual = (float) (enc.rangeMin + normValue * (enc.rangeMax - enc.rangeMin));

        // Snap to step if discrete
        if (enc.step >= 1.0)
            actual = std::round (actual / (float) enc.step) * (float) enc.step;

        actual = juce::jlimit ((float) enc.rangeMin, (float) enc.rangeMax, actual);
        enc.setValue (actual);
        return true;
    }

    // Handle a NoteOn message. Returns true if it was dispatched to a push button.
    bool handleNoteOn (int noteNumber)
    {
        int slot = noteToSlot (noteNumber);
        if (slot < 0 || !buttons[slot].bound || !buttons[slot].onPress)
            return false;

        buttons[slot].onPress();
        return true;
    }

    // Handle a NoteOff message. Returns true if it was dispatched to a push button.
    bool handleNoteOff (int noteNumber)
    {
        int slot = noteToSlot (noteNumber);
        if (slot < 0 || !buttons[slot].bound || !buttons[slot].onRelease)
            return false;

        buttons[slot].onRelease();
        return true;
    }

    //--------------------------------------------------------------------------
    // Query
    //--------------------------------------------------------------------------

    const EncoderBinding& getEncoder (int slot) const
    {
        jassert (slot >= 0 && slot < numEncoders);
        return encoders[slot];
    }

    const ButtonBinding& getButton (int slot) const
    {
        jassert (slot >= 0 && slot < numEncoders);
        return buttons[slot];
    }

    bool isEncoderCC (int ccNumber) const   { return ccToSlot (ccNumber) >= 0; }
    bool isButtonNote (int noteNumber) const { return noteToSlot (noteNumber) >= 0; }

private:
    std::array<EncoderBinding, numEncoders> encoders;
    std::array<ButtonBinding, numEncoders>  buttons;

    // Map CC/note to slot index, or -1 if not in range
    static int ccToSlot (int cc)
    {
        int slot = cc - defaultCCBase;
        return (slot >= 0 && slot < numEncoders) ? slot : -1;
    }

    static int noteToSlot (int note)
    {
        int slot = note - defaultNoteBase;
        return (slot >= 0 && slot < numEncoders) ? slot : -1;
    }
};
