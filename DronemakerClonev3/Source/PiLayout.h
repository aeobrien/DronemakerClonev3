#pragma once
#include <JuceHeader.h>
#include "LoopRecorder.h"

//==============================================================================
// Large loop button with built-in progress bar.
// Short tap = select for detail editing. Long press = toggle record/play/stop.
class TouchLoopButton : public juce::Component, public juce::Timer
{
public:
    TouchLoopButton (LoopRecorder& recorder, int slotIndex)
        : loopRecorder (recorder), slot (slotIndex)
    {
        startTimerHz (30);
    }

    ~TouchLoopButton() override { stopTimer(); }

    std::function<void(int)> onSelect;       // short tap — passes slot index
    std::function<void(int)> onToggle;       // long press / MIDI — passes slot index

    // Called by MIDI mapping — always triggers toggle (record/play/stop)
    void triggerToggle() { if (onToggle) onToggle (slot); }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (1.0f);
        const float corner = 8.0f;

        bool recording = loopRecorder.isRecording() && loopRecorder.getRecordingSlot() == slot;
        bool active = loopRecorder.isSlotActive (slot);
        bool isSelected = (slot == selectedSlot);

        // Background
        juce::Colour bg;
        if (recording)
        {
            float pulse = 0.7f + 0.3f * std::sin (juce::Time::getMillisecondCounter() * 0.006f);
            bg = juce::Colour::fromFloatRGBA (pulse, 0.1f, 0.1f, 1.0f);
        }
        else if (active)
            bg = juce::Colour (0xff1a5c4a);
        else
            bg = juce::Colour (0xff2a2b31);

        // Highlight if pressed
        if (isDown)
            bg = bg.brighter (0.15f);

        g.setColour (bg);
        g.fillRoundedRectangle (bounds, corner);

        // Progress bar fill
        if (active && !recording)
        {
            float progress = loopRecorder.getSlotProgress (slot);
            auto progressBounds = bounds;
            progressBounds.setWidth (bounds.getWidth() * progress);
            g.setColour (juce::Colour (0xff5dd6c6).withAlpha (0.25f));
            g.fillRoundedRectangle (progressBounds, corner);
        }

        // Border — thicker if selected
        g.setColour (isSelected ? juce::Colour (0xff5dd6c6) : juce::Colours::white.withAlpha (0.10f));
        g.drawRoundedRectangle (bounds, corner, isSelected ? 2.0f : 1.0f);

        // Label
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.setFont (juce::Font (14.0f, juce::Font::bold));

        juce::String text;
        if (recording)
            text = "REC " + juce::String (slot + 1);
        else
            text = juce::String (slot + 1);

        g.drawText (text, bounds, juce::Justification::centred);
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        isDown = true;
        pressTime = juce::Time::getMillisecondCounter();
        repaint();
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        isDown = false;
        if (! e.mouseWasDraggedSinceMouseDown())
        {
            auto elapsed = juce::Time::getMillisecondCounter() - pressTime;
            if (elapsed >= longPressMs)
            {
                if (onToggle) onToggle (slot);
            }
            else
            {
                if (onSelect) onSelect (slot);
            }
        }
        repaint();
    }

    void timerCallback() override { repaint(); }

    static int selectedSlot;  // shared across all instances

private:
    LoopRecorder& loopRecorder;
    int slot;
    bool isDown = false;
    juce::uint32 pressTime = 0;
    static constexpr juce::uint32 longPressMs = 400;
};

//==============================================================================
// Row of direct-select buttons for octave: -2  -1  0  +1  +2
class OctaveSelector : public juce::Component
{
public:
    OctaveSelector()
    {
        for (int i = 0; i < 5; ++i)
        {
            auto* b = &buttons[i];
            const int octave = i - 2;
            juce::String label = (octave == 0) ? "0" : ((octave > 0 ? "+" : "") + juce::String (octave));
            b->setButtonText (label);
            b->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2a2b31));
            b->setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha (0.7f));
            b->onClick = [this, i] {
                selectedIndex = i;
                updateColors();
                if (onChange) onChange (i - 2);
            };
            addAndMakeVisible (b);
        }
        selectedIndex = 2;  // default = "0"
        updateColors();
    }

    void setSelectedOctave (int octave)
    {
        selectedIndex = juce::jlimit (0, 4, octave + 2);
        updateColors();
    }

    int getSelectedOctave() const { return selectedIndex - 2; }

    std::function<void(int)> onChange;  // passes octave value (-2 to +2)

    void resized() override
    {
        auto bounds = getLocalBounds();
        int w = bounds.getWidth() / 5;
        for (int i = 0; i < 5; ++i)
            buttons[i].setBounds (bounds.removeFromLeft (w).reduced (1));
    }

private:
    juce::TextButton buttons[5];
    int selectedIndex = 2;

    void updateColors()
    {
        for (int i = 0; i < 5; ++i)
        {
            bool sel = (i == selectedIndex);
            buttons[i].setColour (juce::TextButton::buttonColourId,
                sel ? juce::Colour (0xff5dd6c6).withAlpha (0.3f) : juce::Colour (0xff2a2b31));
            buttons[i].setColour (juce::TextButton::textColourOffId,
                sel ? juce::Colours::white : juce::Colours::white.withAlpha (0.5f));
        }
    }
};

//==============================================================================
// Detail strip for the currently selected loop:
// Shows octave selector, HP slider, LP slider, volume slider, Auto button, Clear button
class LoopDetailStrip : public juce::Component
{
public:
    LoopDetailStrip (LoopRecorder& recorder)
        : loopRecorder (recorder)
    {
        addAndMakeVisible (loopLabel);
        loopLabel.setFont (juce::Font (16.0f, juce::Font::bold));
        loopLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.9f));

        addAndMakeVisible (octaveSelector);

        addAndMakeVisible (hpSlider);
        hpSlider.setSliderStyle (juce::Slider::LinearHorizontal);
        hpSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
        hpSlider.setRange (20.0, 20000.0, 1.0);
        hpSlider.setSkewFactorFromMidPoint (1000.0);
        hpSlider.setValue (20.0);
        addAndMakeVisible (hpLabel);
        hpLabel.setText ("HP", juce::dontSendNotification);
        hpLabel.setFont (juce::Font (12.0f));
        hpLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.6f));

        addAndMakeVisible (lpSlider);
        lpSlider.setSliderStyle (juce::Slider::LinearHorizontal);
        lpSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
        lpSlider.setRange (20.0, 20000.0, 1.0);
        lpSlider.setSkewFactorFromMidPoint (4000.0);
        lpSlider.setValue (20000.0);
        addAndMakeVisible (lpLabel);
        lpLabel.setText ("LP", juce::dontSendNotification);
        lpLabel.setFont (juce::Font (12.0f));
        lpLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.6f));

        addAndMakeVisible (volSlider);
        volSlider.setSliderStyle (juce::Slider::LinearHorizontal);
        volSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 40, 20);
        volSlider.setRange (0.0, 2.0, 0.01);
        volSlider.setValue (1.0);
        addAndMakeVisible (volLabel);
        volLabel.setText ("Vol", juce::dontSendNotification);
        volLabel.setFont (juce::Font (12.0f));
        volLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.6f));

        autoButton.setButtonText ("Auto");
        autoButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2a2b31));
        addAndMakeVisible (autoButton);

        clearButton.setButtonText ("Clear");
        clearButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff3a1515));
        addAndMakeVisible (clearButton);
    }

    void setSelectedLoop (int loopIndex)
    {
        currentLoop = loopIndex;
        loopLabel.setText ("Loop " + juce::String (loopIndex + 1), juce::dontSendNotification);

        // Sync controls to this loop's current state
        // (The parent will call this after setting up callbacks)
        repaint();
    }

    int getSelectedLoop() const { return currentLoop; }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (6, 4);

        // Top row: label, octave selector, auto + clear buttons
        auto topRow = bounds.removeFromTop (bounds.getHeight() / 2);
        loopLabel.setBounds (topRow.removeFromLeft (70));
        topRow.removeFromLeft (5);

        clearButton.setBounds (topRow.removeFromRight (55).reduced (2));
        topRow.removeFromRight (3);
        autoButton.setBounds (topRow.removeFromRight (55).reduced (2));
        topRow.removeFromRight (10);

        octaveSelector.setBounds (topRow.reduced (2));

        bounds.removeFromTop (2);

        // Bottom row: HP, LP, Vol sliders
        auto bottomRow = bounds;
        int sliderWidth = (bottomRow.getWidth() - 90) / 3;  // 90px for labels

        hpLabel.setBounds (bottomRow.removeFromLeft (25));
        hpSlider.setBounds (bottomRow.removeFromLeft (sliderWidth).reduced (0, 1));
        bottomRow.removeFromLeft (5);

        lpLabel.setBounds (bottomRow.removeFromLeft (25));
        lpSlider.setBounds (bottomRow.removeFromLeft (sliderWidth).reduced (0, 1));
        bottomRow.removeFromLeft (5);

        volLabel.setBounds (bottomRow.removeFromLeft (25));
        volSlider.setBounds (bottomRow.reduced (0, 1));
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff17181d));
        g.fillRoundedRectangle (bounds, 6.0f);
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);
    }

    // Public so parent can wire callbacks
    OctaveSelector octaveSelector;
    juce::Slider hpSlider, lpSlider, volSlider;
    juce::TextButton autoButton, clearButton;

private:
    LoopRecorder& loopRecorder;
    int currentLoop = 0;

    juce::Label loopLabel;
    juce::Label hpLabel, lpLabel, volLabel;
};
