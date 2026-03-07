#pragma once
#include <JuceHeader.h>
#include "LoopRecorder.h"

//==============================================================================
// Colour palette for Pi layout
namespace PiColours
{
    const auto bg          = juce::Colour (0xff0e1015);
    const auto panelBg     = juce::Colour (0xff161920);
    const auto panelBorder = juce::Colour (0xff2a2e38);
    const auto accent      = juce::Colour (0xff4fc3b0);  // mint/teal
    const auto accentDim   = juce::Colour (0xff2a7568);
    const auto accentGlow  = juce::Colour (0xff5dd6c6);
    const auto textBright  = juce::Colour (0xfff0f0f2);
    const auto textNormal  = juce::Colour (0xffb8bac0);
    const auto textDim     = juce::Colour (0xff6b6e78);
    const auto red         = juce::Colour (0xffcc4444);
    const auto redGlow     = juce::Colour (0xffff5555);
    const auto buttonBg    = juce::Colour (0xff1e2230);
    const auto buttonHover = juce::Colour (0xff282d3c);
    const auto loopActive  = juce::Colour (0xff1a3d34);
    const auto loopEmpty   = juce::Colour (0xff1a1e28);
}

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

    std::function<void(int)> onSelect;
    std::function<void(int)> onToggle;

    void triggerToggle() { if (onToggle) onToggle (slot); }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (1.0f);
        const float corner = 10.0f;

        bool recording = loopRecorder.isRecording() && loopRecorder.getRecordingSlot() == slot;
        bool active = loopRecorder.isSlotActive (slot);
        bool isSelected = (slot == selectedSlot);

        // Background
        juce::Colour bg;
        if (recording)
        {
            float pulse = 0.6f + 0.4f * std::sin (juce::Time::getMillisecondCounter() * 0.008f);
            bg = PiColours::red.withAlpha (pulse);
        }
        else if (active)
            bg = PiColours::loopActive;
        else
            bg = PiColours::loopEmpty;

        if (isDown)
            bg = bg.brighter (0.2f);

        g.setColour (bg);
        g.fillRoundedRectangle (bounds, corner);

        // Progress bar fill (sweep from left)
        if (active && !recording)
        {
            float progress = loopRecorder.getSlotProgress (slot);
            auto progressBounds = bounds;
            progressBounds.setWidth (bounds.getWidth() * progress);

            g.setColour (PiColours::accent.withAlpha (0.2f));
            g.fillRoundedRectangle (progressBounds, corner);

            // Playhead line
            float playheadX = bounds.getX() + bounds.getWidth() * progress;
            g.setColour (PiColours::accentGlow.withAlpha (0.7f));
            g.drawLine (playheadX, bounds.getY() + 3, playheadX, bounds.getBottom() - 3, 2.0f);
        }

        // Border
        if (isSelected)
        {
            g.setColour (PiColours::accent);
            g.drawRoundedRectangle (bounds, corner, 2.5f);
        }
        else
        {
            g.setColour (PiColours::panelBorder);
            g.drawRoundedRectangle (bounds, corner, 1.0f);
        }

        // Slot number
        g.setFont (juce::Font (18.0f, juce::Font::bold));

        if (recording)
        {
            g.setColour (PiColours::textBright);
            g.drawText ("REC", bounds.removeFromTop (bounds.getHeight() * 0.55f),
                        juce::Justification::centredBottom);
            g.setFont (juce::Font (13.0f));
            g.drawText (juce::String (slot + 1), bounds, juce::Justification::centredTop);
        }
        else if (active)
        {
            g.setColour (PiColours::accentGlow);
            g.drawText (juce::String (slot + 1), bounds, juce::Justification::centred);
        }
        else
        {
            g.setColour (PiColours::textDim);
            g.drawText (juce::String (slot + 1), bounds, juce::Justification::centred);
        }
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

    static int selectedSlot;

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
            b->onClick = [this, i] {
                selectedIndex = i;
                updateColors();
                if (onChange) onChange (i - 2);
            };
            addAndMakeVisible (b);
        }
        selectedIndex = 2;
        updateColors();
    }

    void setSelectedOctave (int octave)
    {
        selectedIndex = juce::jlimit (0, 4, octave + 2);
        updateColors();
    }

    int getSelectedOctave() const { return selectedIndex - 2; }

    std::function<void(int)> onChange;

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
                sel ? PiColours::accent.withAlpha (0.35f) : PiColours::buttonBg);
            buttons[i].setColour (juce::TextButton::textColourOffId,
                sel ? PiColours::textBright : PiColours::textDim);
        }
    }
};

//==============================================================================
// Detail strip for the currently selected loop
class LoopDetailStrip : public juce::Component
{
public:
    LoopDetailStrip (LoopRecorder& recorder)
        : loopRecorder (recorder)
    {
        addAndMakeVisible (loopLabel);
        loopLabel.setFont (juce::Font (15.0f, juce::Font::bold));
        loopLabel.setColour (juce::Label::textColourId, PiColours::accentGlow);

        addAndMakeVisible (octaveLabel);
        octaveLabel.setText ("Oct", juce::dontSendNotification);
        octaveLabel.setFont (juce::Font (11.0f));
        octaveLabel.setColour (juce::Label::textColourId, PiColours::textDim);

        addAndMakeVisible (octaveSelector);

        addAndMakeVisible (hpSlider);
        hpSlider.setSliderStyle (juce::Slider::LinearHorizontal);
        hpSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 48, 20);
        hpSlider.setRange (20.0, 20000.0, 1.0);
        hpSlider.setSkewFactorFromMidPoint (1000.0);
        hpSlider.setValue (20.0);
        addAndMakeVisible (hpLabel);
        hpLabel.setText ("HP", juce::dontSendNotification);
        hpLabel.setFont (juce::Font (11.0f, juce::Font::bold));
        hpLabel.setColour (juce::Label::textColourId, PiColours::textDim);

        addAndMakeVisible (lpSlider);
        lpSlider.setSliderStyle (juce::Slider::LinearHorizontal);
        lpSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 48, 20);
        lpSlider.setRange (20.0, 20000.0, 1.0);
        lpSlider.setSkewFactorFromMidPoint (4000.0);
        lpSlider.setValue (20000.0);
        addAndMakeVisible (lpLabel);
        lpLabel.setText ("LP", juce::dontSendNotification);
        lpLabel.setFont (juce::Font (11.0f, juce::Font::bold));
        lpLabel.setColour (juce::Label::textColourId, PiColours::textDim);

        addAndMakeVisible (volSlider);
        volSlider.setSliderStyle (juce::Slider::LinearHorizontal);
        volSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 36, 20);
        volSlider.setRange (0.0, 2.0, 0.01);
        volSlider.setValue (1.0);
        addAndMakeVisible (volLabel);
        volLabel.setText ("Vol", juce::dontSendNotification);
        volLabel.setFont (juce::Font (11.0f, juce::Font::bold));
        volLabel.setColour (juce::Label::textColourId, PiColours::textDim);

        autoButton.setButtonText ("Auto");
        autoButton.setColour (juce::TextButton::buttonColourId, PiColours::buttonBg);
        autoButton.setColour (juce::TextButton::textColourOffId, PiColours::textNormal);
        addAndMakeVisible (autoButton);

        clearButton.setButtonText ("Clear");
        clearButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2a1520));
        clearButton.setColour (juce::TextButton::textColourOffId, PiColours::red);
        addAndMakeVisible (clearButton);
    }

    void setSelectedLoop (int loopIndex)
    {
        currentLoop = loopIndex;
        loopLabel.setText ("Loop " + juce::String (loopIndex + 1), juce::dontSendNotification);
        repaint();
    }

    int getSelectedLoop() const { return currentLoop; }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (8, 4);

        // Top row: label, octave label + selector, auto + clear buttons
        auto topRow = bounds.removeFromTop (bounds.getHeight() / 2);
        loopLabel.setBounds (topRow.removeFromLeft (65));
        topRow.removeFromLeft (3);

        clearButton.setBounds (topRow.removeFromRight (52).reduced (1));
        topRow.removeFromRight (3);
        autoButton.setBounds (topRow.removeFromRight (52).reduced (1));
        topRow.removeFromRight (8);

        octaveLabel.setBounds (topRow.removeFromLeft (28));
        octaveSelector.setBounds (topRow.reduced (1));

        bounds.removeFromTop (3);

        // Bottom row: HP, LP, Vol sliders
        auto bottomRow = bounds;
        int sliderWidth = (bottomRow.getWidth() - 80) / 3;

        hpLabel.setBounds (bottomRow.removeFromLeft (22));
        hpSlider.setBounds (bottomRow.removeFromLeft (sliderWidth).reduced (0, 1));
        bottomRow.removeFromLeft (5);

        lpLabel.setBounds (bottomRow.removeFromLeft (22));
        lpSlider.setBounds (bottomRow.removeFromLeft (sliderWidth).reduced (0, 1));
        bottomRow.removeFromLeft (5);

        volLabel.setBounds (bottomRow.removeFromLeft (24));
        volSlider.setBounds (bottomRow.reduced (0, 1));
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour (PiColours::panelBg);
        g.fillRoundedRectangle (bounds, 8.0f);
        g.setColour (PiColours::panelBorder);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 8.0f, 1.0f);
    }

    OctaveSelector octaveSelector;
    juce::Slider hpSlider, lpSlider, volSlider;
    juce::TextButton autoButton, clearButton;

private:
    LoopRecorder& loopRecorder;
    int currentLoop = 0;

    juce::Label loopLabel, octaveLabel;
    juce::Label hpLabel, lpLabel, volLabel;
};
