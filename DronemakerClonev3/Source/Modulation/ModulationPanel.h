#pragma once

#include <JuceHeader.h>
#include "ModulationManager.h"

/**
 * Helper to format rate values - uses time for slow rates (<1Hz)
 */
inline juce::String formatRateValue (float hz)
{
    if (hz >= 1.0f)
        return juce::String (hz, 2) + " Hz";

    // Convert to time period
    float seconds = 1.0f / hz;
    int totalSeconds = juce::roundToInt (seconds);

    if (totalSeconds >= 60)
    {
        int minutes = totalSeconds / 60;
        int secs = totalSeconds % 60;
        if (secs == 0)
            return juce::String (minutes) + "m";
        return juce::String (minutes) + "m" + juce::String (secs) + "s";
    }
    return juce::String (totalSeconds) + "s";
}

/**
 * Helper to format parameter values with appropriate units
 */
inline juce::String formatParamValue (float value, ModulationTarget::Type type)
{
    switch (type)
    {
        case ModulationTarget::Type::LoopFilterHP:
        case ModulationTarget::Type::LoopFilterLP:
        case ModulationTarget::Type::FilterHP:
        case ModulationTarget::Type::FilterLP:
            if (value >= 1000.0f)
                return juce::String (value / 1000.0f, 1) + "kHz";
            return juce::String (juce::roundToInt (value)) + "Hz";

        case ModulationTarget::Type::DelayTime:
            if (value >= 1000.0f)
                return juce::String (value / 1000.0f, 2) + "s";
            return juce::String (juce::roundToInt (value)) + "ms";

        case ModulationTarget::Type::DistortionDrive:
        case ModulationTarget::Type::TapeSaturation:
            return juce::String (value, 1) + "x";

        case ModulationTarget::Type::TremoloRate:
            return juce::String (value, 1) + "Hz";

        default:
            // For 0-1 parameters, show as percentage
            if (value <= 1.0f)
                return juce::String (juce::roundToInt (value * 100.0f)) + "%";
            return juce::String (value, 2);
    }
}

/**
 * Target selection button with popup menu containing submenus
 */
class TargetSelectorButton : public juce::Component
{
public:
    TargetSelectorButton (ModulationTarget& target, std::function<void()> onChangeCallback)
        : targetRef (target), onChange (onChangeCallback)
    {
        updateText();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (1.0f);

        // Background
        g.setColour (juce::Colour (0xff1d1f26));
        g.fillRoundedRectangle (bounds, 4.0f);

        // Border
        g.setColour (juce::Colour (0xff3a3d45));
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

        // Text
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.setFont (10.0f);
        g.drawText (displayText, bounds.reduced (4, 0), juce::Justification::centredLeft);

        // Arrow
        auto arrowArea = bounds.removeFromRight (12.0f).reduced (2.0f, 5.0f);
        juce::Path arrow;
        arrow.startNewSubPath (arrowArea.getX(), arrowArea.getY());
        arrow.lineTo (arrowArea.getCentreX(), arrowArea.getBottom());
        arrow.lineTo (arrowArea.getRight(), arrowArea.getY());
        g.strokePath (arrow, juce::PathStrokeType (1.2f));
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        showPopupMenu();
    }

    void updateText()
    {
        if (targetRef.type == ModulationTarget::Type::None)
        {
            displayText = "None";
        }
        else
        {
            displayText = ModulationManager::getTargetTypeName (targetRef.type);
            if (targetRef.type == ModulationTarget::Type::LoopVolume ||
                targetRef.type == ModulationTarget::Type::LoopFilterHP ||
                targetRef.type == ModulationTarget::Type::LoopFilterLP)
            {
                displayText = "L" + juce::String (targetRef.index + 1) + " " +
                              displayText.fromLastOccurrenceOf ("Loop ", false, true);
            }
        }
        repaint();
    }

private:
    void showPopupMenu()
    {
        juce::PopupMenu menu;

        menu.addItem (1, "None", true, targetRef.type == ModulationTarget::Type::None);

        // Loop submenus (1-8)
        for (int loop = 0; loop < 8; ++loop)
        {
            juce::PopupMenu loopMenu;
            int baseId = 100 + loop * 10;
            loopMenu.addItem (baseId + 1, "Volume", true,
                              targetRef.type == ModulationTarget::Type::LoopVolume && targetRef.index == loop);
            loopMenu.addItem (baseId + 2, "High Pass", true,
                              targetRef.type == ModulationTarget::Type::LoopFilterHP && targetRef.index == loop);
            loopMenu.addItem (baseId + 3, "Low Pass", true,
                              targetRef.type == ModulationTarget::Type::LoopFilterLP && targetRef.index == loop);
            menu.addSubMenu ("Loop " + juce::String (loop + 1), loopMenu);
        }

        // Delay submenu
        juce::PopupMenu delayMenu;
        delayMenu.addItem (200, "Time", true, targetRef.type == ModulationTarget::Type::DelayTime);
        delayMenu.addItem (201, "Feedback", true, targetRef.type == ModulationTarget::Type::DelayFeedback);
        delayMenu.addItem (202, "Dry/Wet", true, targetRef.type == ModulationTarget::Type::DelayDryWet);
        menu.addSubMenu ("Delay", delayMenu);

        // Distortion submenu
        juce::PopupMenu distMenu;
        distMenu.addItem (210, "Drive", true, targetRef.type == ModulationTarget::Type::DistortionDrive);
        distMenu.addItem (211, "Tone", true, targetRef.type == ModulationTarget::Type::DistortionTone);
        distMenu.addItem (212, "Dry/Wet", true, targetRef.type == ModulationTarget::Type::DistortionDryWet);
        menu.addSubMenu ("Distortion", distMenu);

        // Tape submenu
        juce::PopupMenu tapeMenu;
        tapeMenu.addItem (220, "Saturation", true, targetRef.type == ModulationTarget::Type::TapeSaturation);
        tapeMenu.addItem (221, "Bias", true, targetRef.type == ModulationTarget::Type::TapeBias);
        tapeMenu.addItem (222, "Wow", true, targetRef.type == ModulationTarget::Type::TapeWowDepth);
        tapeMenu.addItem (223, "Flutter", true, targetRef.type == ModulationTarget::Type::TapeFlutterDepth);
        tapeMenu.addItem (224, "Dry/Wet", true, targetRef.type == ModulationTarget::Type::TapeDryWet);
        menu.addSubMenu ("Tape", tapeMenu);

        // Filter submenu
        juce::PopupMenu filterMenu;
        filterMenu.addItem (230, "High Pass", true, targetRef.type == ModulationTarget::Type::FilterHP);
        filterMenu.addItem (231, "Low Pass", true, targetRef.type == ModulationTarget::Type::FilterLP);
        filterMenu.addItem (232, "Harmonic Intensity", true, targetRef.type == ModulationTarget::Type::FilterHarmonicIntensity);
        menu.addSubMenu ("Filter", filterMenu);

        // Tremolo submenu
        juce::PopupMenu tremMenu;
        tremMenu.addItem (240, "Rate", true, targetRef.type == ModulationTarget::Type::TremoloRate);
        tremMenu.addItem (241, "Depth", true, targetRef.type == ModulationTarget::Type::TremoloDepth);
        menu.addSubMenu ("Tremolo", tremMenu);

        // Master submenu
        juce::PopupMenu masterMenu;
        masterMenu.addItem (250, "Volume", true, targetRef.type == ModulationTarget::Type::MasterVolume);
        masterMenu.addItem (251, "Loop Mix", true, targetRef.type == ModulationTarget::Type::LoopMix);
        menu.addSubMenu ("Master", masterMenu);

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                            [this] (int result) { handleMenuResult (result); });
    }

    void handleMenuResult (int result)
    {
        if (result == 0)
            return;

        ModulationTarget::Type newType = ModulationTarget::Type::None;
        int newIndex = 0;

        if (result == 1)
        {
            newType = ModulationTarget::Type::None;
        }
        else if (result >= 100 && result < 180)
        {
            int loop = (result - 100) / 10;
            int param = (result - 100) % 10;
            newIndex = loop;
            switch (param)
            {
                case 1: newType = ModulationTarget::Type::LoopVolume; break;
                case 2: newType = ModulationTarget::Type::LoopFilterHP; break;
                case 3: newType = ModulationTarget::Type::LoopFilterLP; break;
            }
        }
        else
        {
            switch (result)
            {
                case 200: newType = ModulationTarget::Type::DelayTime; break;
                case 201: newType = ModulationTarget::Type::DelayFeedback; break;
                case 202: newType = ModulationTarget::Type::DelayDryWet; break;
                case 210: newType = ModulationTarget::Type::DistortionDrive; break;
                case 211: newType = ModulationTarget::Type::DistortionTone; break;
                case 212: newType = ModulationTarget::Type::DistortionDryWet; break;
                case 220: newType = ModulationTarget::Type::TapeSaturation; break;
                case 221: newType = ModulationTarget::Type::TapeBias; break;
                case 222: newType = ModulationTarget::Type::TapeWowDepth; break;
                case 223: newType = ModulationTarget::Type::TapeFlutterDepth; break;
                case 224: newType = ModulationTarget::Type::TapeDryWet; break;
                case 230: newType = ModulationTarget::Type::FilterHP; break;
                case 231: newType = ModulationTarget::Type::FilterLP; break;
                case 232: newType = ModulationTarget::Type::FilterHarmonicIntensity; break;
                case 240: newType = ModulationTarget::Type::TremoloRate; break;
                case 241: newType = ModulationTarget::Type::TremoloDepth; break;
                case 250: newType = ModulationTarget::Type::MasterVolume; break;
                case 251: newType = ModulationTarget::Type::LoopMix; break;
            }
        }

        targetRef.type = newType;
        targetRef.index = newIndex;
        ModulationTarget::getDefaultRange (newType, targetRef.rangeMin, targetRef.rangeMax);
        updateText();
        if (onChange)
            onChange();
    }

    ModulationTarget& targetRef;
    std::function<void()> onChange;
    juce::String displayText { "None" };
};

/**
 * UI component for a single modulation target slot with min/max range sliders and labels
 * Redesigned with stacked layout for more slider room
 */
class ModulationTargetSlot : public juce::Component
{
public:
    ModulationTargetSlot (ModulationTarget& target, int slotIndex)
        : targetRef (target), slotIdx (slotIndex),
          targetSelector (target, [this] { updateSliderRanges(); repaint(); })
    {
        addAndMakeVisible (targetSelector);

        // Min slider
        minSlider.setSliderStyle (juce::Slider::LinearHorizontal);
        minSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        minSlider.onValueChange = [this] {
            float newMin = static_cast<float> (minSlider.getValue());
            // Clamp to not exceed max
            if (newMin > targetRef.rangeMax - 0.001f)
                newMin = targetRef.rangeMax - 0.001f;
            targetRef.rangeMin = newMin;
            minSlider.setValue (newMin, juce::dontSendNotification);
            repaint();
        };
        addAndMakeVisible (minSlider);

        // Max slider
        maxSlider.setSliderStyle (juce::Slider::LinearHorizontal);
        maxSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        maxSlider.onValueChange = [this] {
            float newMax = static_cast<float> (maxSlider.getValue());
            // Clamp to not go below min
            if (newMax < targetRef.rangeMin + 0.001f)
                newMax = targetRef.rangeMin + 0.001f;
            targetRef.rangeMax = newMax;
            maxSlider.setValue (newMax, juce::dontSendNotification);
            repaint();
        };
        addAndMakeVisible (maxSlider);

        updateSliderRanges();
    }

    void paint (juce::Graphics& g) override
    {
        // Draw min/max value labels
        if (targetRef.type != ModulationTarget::Type::None)
        {
            g.setColour (juce::Colours::white.withAlpha (0.7f));
            g.setFont (9.0f);

            juce::String minText = "Min: " + formatParamValue (targetRef.rangeMin, targetRef.type);
            juce::String maxText = "Max: " + formatParamValue (targetRef.rangeMax, targetRef.type);

            g.drawText (minText, minLabelArea, juce::Justification::centredLeft);
            g.drawText (maxText, maxLabelArea, juce::Justification::centredLeft);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // Target selector button at top
        targetSelector.setBounds (bounds.removeFromTop (20));
        bounds.removeFromTop (2);

        // Min row: label and slider
        auto minRow = bounds.removeFromTop (18);
        minLabelArea = minRow.removeFromLeft (70);
        minSlider.setBounds (minRow);
        bounds.removeFromTop (2);

        // Max row: label and slider
        auto maxRow = bounds.removeFromTop (18);
        maxLabelArea = maxRow.removeFromLeft (70);
        maxSlider.setBounds (maxRow);
    }

    void updateFromTarget()
    {
        targetSelector.updateText();
        updateSliderRanges();
    }

private:
    void updateSliderRanges()
    {
        float defaultMin, defaultMax;
        ModulationTarget::getDefaultRange (targetRef.type, defaultMin, defaultMax);

        double step = (defaultMax - defaultMin) / 200.0;
        if (step < 0.0001)
            step = 0.0001;

        // Ensure the target values are within valid range
        targetRef.rangeMin = juce::jlimit (defaultMin, defaultMax, targetRef.rangeMin);
        targetRef.rangeMax = juce::jlimit (defaultMin, defaultMax, targetRef.rangeMax);

        // Ensure min <= max
        if (targetRef.rangeMin > targetRef.rangeMax)
            std::swap (targetRef.rangeMin, targetRef.rangeMax);

        minSlider.setRange (defaultMin, defaultMax, step);
        maxSlider.setRange (defaultMin, defaultMax, step);

        minSlider.setValue (targetRef.rangeMin, juce::dontSendNotification);
        maxSlider.setValue (targetRef.rangeMax, juce::dontSendNotification);
    }

    ModulationTarget& targetRef;
    int slotIdx;
    TargetSelectorButton targetSelector;
    juce::Slider minSlider;
    juce::Slider maxSlider;
    juce::Rectangle<int> minLabelArea;
    juce::Rectangle<int> maxLabelArea;
};

/**
 * Visual indicator showing current modulation value
 */
class ModulationIndicator : public juce::Component
{
public:
    ModulationIndicator() = default;

    void setValue (float v)
    {
        currentValue = v;
        repaint();
    }

    void setUnipolar (bool isUnipolar) { unipolar = isUnipolar; }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (2.0f);

        g.setColour (juce::Colour (0xff1a1b1f));
        g.fillRoundedRectangle (bounds, 3.0f);

        if (unipolar)
        {
            float barWidth = currentValue * bounds.getWidth();
            g.setColour (juce::Colour (0xff4a9eff));
            g.fillRoundedRectangle (bounds.getX(), bounds.getY(),
                                    barWidth, bounds.getHeight(), 3.0f);
        }
        else
        {
            float centerX = bounds.getCentreX();
            float barWidth = std::abs (currentValue) * bounds.getWidth() * 0.5f;

            g.setColour (juce::Colour (0xff4a9eff));
            if (currentValue >= 0)
                g.fillRoundedRectangle (centerX, bounds.getY(), barWidth, bounds.getHeight(), 3.0f);
            else
                g.fillRoundedRectangle (centerX - barWidth, bounds.getY(), barWidth, bounds.getHeight(), 3.0f);

            g.setColour (juce::Colour (0xff3a3d45));
            g.drawLine (centerX, bounds.getY(), centerX, bounds.getBottom(), 1.0f);
        }

        g.setColour (juce::Colour (0xff3a3d45));
        g.drawRoundedRectangle (bounds, 3.0f, 1.0f);
    }

private:
    float currentValue = 0.0f;
    bool unipolar = false;
};

/**
 * UI component for a single modulation source (LFO, Envelope, Randomizer)
 * Redesigned with wider layout for 2-per-row display
 */
class ModulationSourcePanel : public juce::Component, public juce::Timer
{
public:
    ModulationSourcePanel (ModulationSource& source, const juce::String& name)
        : sourceRef (source), sourceName (name)
    {
        for (int i = 0; i < ModulationSource::maxTargets; ++i)
        {
            auto slot = std::make_unique<ModulationTargetSlot> (source.getTarget (i), i);
            addAndMakeVisible (slot.get());
            targetSlots.push_back (std::move (slot));
        }

        addAndMakeVisible (indicator);
        startTimerHz (30);
    }

    ~ModulationSourcePanel() override { stopTimer(); }

    virtual void createControls() = 0;

    void timerCallback() override
    {
        indicator.setValue (sourceRef.getCurrentValue());
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().reduced (2);

        g.setColour (juce::Colour (0xff252830));
        g.fillRoundedRectangle (bounds.toFloat(), 6.0f);

        g.setColour (juce::Colour (0xff3a3d45));
        g.drawRoundedRectangle (bounds.toFloat(), 6.0f, 1.0f);

        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (13.0f).withStyle ("Bold"));
        g.drawText (sourceName, bounds.removeFromTop (22).reduced (8, 0), juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (6);
        bounds.removeFromTop (22);

        // Controls area at top
        controlsArea = bounds.removeFromTop (50);

        bounds.removeFromTop (4);

        // Indicator
        indicatorArea = bounds.removeFromTop (14);
        indicator.setBounds (indicatorArea);

        bounds.removeFromTop (6);

        // Target slots - each takes 60px height
        for (auto& slot : targetSlots)
        {
            slot->setBounds (bounds.removeFromTop (60));
            bounds.removeFromTop (4);
        }
    }

    // Get the height needed for this panel
    static int getRequiredHeight()
    {
        return 22 + 50 + 4 + 14 + 6 + (3 * 64) + 10;  // ~288px
    }

protected:
    ModulationSource& sourceRef;
    juce::String sourceName;
    juce::Rectangle<int> controlsArea;
    juce::Rectangle<int> indicatorArea;
    std::vector<std::unique_ptr<ModulationTargetSlot>> targetSlots;
    ModulationIndicator indicator;
};

/**
 * LFO panel with rate (time-based display for slow rates) and waveform controls
 */
class LFOPanel : public ModulationSourcePanel
{
public:
    LFOPanel (LFOSource& lfo, int index)
        : ModulationSourcePanel (lfo, "LFO " + juce::String (index + 1)), lfoRef (lfo)
    {
        createControls();
    }

    void createControls() override
    {
        rateSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        rateSlider.setRange (0.002, 10.0, 0.001);
        rateSlider.setSkewFactorFromMidPoint (0.5);
        rateSlider.setValue (lfoRef.getRate());
        rateSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 14);
        rateSlider.textFromValueFunction = [] (double val) { return formatRateValue (static_cast<float> (val)); };
        rateSlider.onValueChange = [this] {
            lfoRef.setRate (static_cast<float> (rateSlider.getValue()));
        };
        addAndMakeVisible (rateSlider);

        waveformCombo.addItem ("Sine", 1);
        waveformCombo.addItem ("Triangle", 2);
        waveformCombo.addItem ("Square", 3);
        waveformCombo.addItem ("Saw Up", 4);
        waveformCombo.addItem ("Saw Down", 5);
        waveformCombo.addItem ("S&H", 6);
        waveformCombo.setSelectedId (static_cast<int> (lfoRef.getWaveform()) + 1);
        waveformCombo.onChange = [this] {
            lfoRef.setWaveform (static_cast<LFOSource::Waveform> (waveformCombo.getSelectedId() - 1));
        };
        addAndMakeVisible (waveformCombo);
    }

    void resized() override
    {
        ModulationSourcePanel::resized();

        auto controls = controlsArea;
        rateSlider.setBounds (controls.removeFromLeft (60));
        controls.removeFromLeft (8);
        waveformCombo.setBounds (controls.removeFromLeft (90).withHeight (22).withY (controls.getCentreY() - 11));
    }

private:
    LFOSource& lfoRef;
    juce::Slider rateSlider;
    juce::ComboBox waveformCombo;
};

/**
 * Envelope follower panel with attack/release controls
 */
class EnvelopePanel : public ModulationSourcePanel
{
public:
    EnvelopePanel (EnvelopeFollowerSource& env)
        : ModulationSourcePanel (env, "Envelope Follower"), envRef (env)
    {
        indicator.setUnipolar (true);
        createControls();
    }

    void createControls() override
    {
        attackSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        attackSlider.setRange (0.1, 500.0, 0.1);
        attackSlider.setValue (envRef.getAttackMs());
        attackSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 14);
        attackSlider.setTextValueSuffix (" ms");
        attackSlider.onValueChange = [this] {
            envRef.setAttackMs (static_cast<float> (attackSlider.getValue()));
        };
        addAndMakeVisible (attackSlider);

        releaseSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        releaseSlider.setRange (1.0, 2000.0, 1.0);
        releaseSlider.setValue (envRef.getReleaseMs());
        releaseSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 14);
        releaseSlider.setTextValueSuffix (" ms");
        releaseSlider.onValueChange = [this] {
            envRef.setReleaseMs (static_cast<float> (releaseSlider.getValue()));
        };
        addAndMakeVisible (releaseSlider);

        sensitivitySlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        sensitivitySlider.setRange (0.1, 10.0, 0.1);
        sensitivitySlider.setValue (envRef.getSensitivity());
        sensitivitySlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 14);
        sensitivitySlider.onValueChange = [this] {
            envRef.setSensitivity (static_cast<float> (sensitivitySlider.getValue()));
        };
        addAndMakeVisible (sensitivitySlider);
    }

    void resized() override
    {
        ModulationSourcePanel::resized();

        auto controls = controlsArea;
        attackSlider.setBounds (controls.removeFromLeft (60));
        controls.removeFromLeft (4);
        releaseSlider.setBounds (controls.removeFromLeft (60));
        controls.removeFromLeft (4);
        sensitivitySlider.setBounds (controls.removeFromLeft (60));
    }

private:
    EnvelopeFollowerSource& envRef;
    juce::Slider attackSlider;
    juce::Slider releaseSlider;
    juce::Slider sensitivitySlider;
};

/**
 * Randomizer panel with rate and smoothness controls
 */
class RandomizerPanel : public ModulationSourcePanel
{
public:
    RandomizerPanel (RandomizerSource& rand, int index)
        : ModulationSourcePanel (rand, "Random " + juce::String (index + 1)), randRef (rand)
    {
        createControls();
    }

    void createControls() override
    {
        rateSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        rateSlider.setRange (0.002, 10.0, 0.001);
        rateSlider.setSkewFactorFromMidPoint (0.5);
        rateSlider.setValue (randRef.getRate());
        rateSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 14);
        rateSlider.textFromValueFunction = [] (double val) { return formatRateValue (static_cast<float> (val)); };
        rateSlider.onValueChange = [this] {
            randRef.setRate (static_cast<float> (rateSlider.getValue()));
        };
        addAndMakeVisible (rateSlider);

        smoothSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        smoothSlider.setRange (0.0, 1.0, 0.01);
        smoothSlider.setValue (randRef.getSmoothness());
        smoothSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 14);
        smoothSlider.onValueChange = [this] {
            randRef.setSmoothness (static_cast<float> (smoothSlider.getValue()));
        };
        addAndMakeVisible (smoothSlider);
    }

    void resized() override
    {
        ModulationSourcePanel::resized();

        auto controls = controlsArea;
        rateSlider.setBounds (controls.removeFromLeft (60));
        controls.removeFromLeft (8);
        smoothSlider.setBounds (controls.removeFromLeft (60));
    }

private:
    RandomizerSource& randRef;
    juce::Slider rateSlider;
    juce::Slider smoothSlider;
};

/**
 * Content component for the scrollable viewport
 */
class ModulationPanelContent : public juce::Component
{
public:
    ModulationPanelContent (ModulationManager& manager)
        : modManager (manager)
    {
        for (int i = 0; i < ModulationManager::numLFOs; ++i)
        {
            auto panel = std::make_unique<LFOPanel> (manager.getLFO (i), i);
            addAndMakeVisible (panel.get());
            lfoPanels.push_back (std::move (panel));
        }

        envelopePanel = std::make_unique<EnvelopePanel> (manager.getEnvelopeFollower());
        addAndMakeVisible (envelopePanel.get());

        for (int i = 0; i < ModulationManager::numRandomizers; ++i)
        {
            auto panel = std::make_unique<RandomizerPanel> (manager.getRandomizer (i), i);
            addAndMakeVisible (panel.get());
            randomizerPanels.push_back (std::move (panel));
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (4);

        const int panelHeight = ModulationSourcePanel::getRequiredHeight();
        const int gap = 8;

        // Calculate panel width for 2 per row
        int availableWidth = bounds.getWidth();
        int panelWidth = (availableWidth - gap) / 2;

        int y = bounds.getY();

        // Row 1: LFO 1 and LFO 2
        lfoPanels[0]->setBounds (bounds.getX(), y, panelWidth, panelHeight);
        lfoPanels[1]->setBounds (bounds.getX() + panelWidth + gap, y, panelWidth, panelHeight);
        y += panelHeight + gap;

        // Row 2: LFO 3 and LFO 4
        lfoPanels[2]->setBounds (bounds.getX(), y, panelWidth, panelHeight);
        lfoPanels[3]->setBounds (bounds.getX() + panelWidth + gap, y, panelWidth, panelHeight);
        y += panelHeight + gap;

        // Row 3: Envelope and Random 1
        envelopePanel->setBounds (bounds.getX(), y, panelWidth, panelHeight);
        randomizerPanels[0]->setBounds (bounds.getX() + panelWidth + gap, y, panelWidth, panelHeight);
        y += panelHeight + gap;

        // Row 4: Random 2 (centered or left-aligned)
        randomizerPanels[1]->setBounds (bounds.getX(), y, panelWidth, panelHeight);
    }

    int getRequiredHeight() const
    {
        const int panelHeight = ModulationSourcePanel::getRequiredHeight();
        const int gap = 8;
        return 4 * (panelHeight + gap) + 8;  // 4 rows
    }

private:
    ModulationManager& modManager;
    std::vector<std::unique_ptr<LFOPanel>> lfoPanels;
    std::unique_ptr<EnvelopePanel> envelopePanel;
    std::vector<std::unique_ptr<RandomizerPanel>> randomizerPanels;
};

/**
 * Main modulation panel with scrolling viewport
 */
class ModulationPanel : public juce::Component
{
public:
    ModulationPanel (ModulationManager& manager)
        : modManager (manager)
    {
        content = std::make_unique<ModulationPanelContent> (manager);
        viewport.setViewedComponent (content.get(), false);
        viewport.setScrollBarsShown (true, false);
        addAndMakeVisible (viewport);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff17181d));
    }

    void resized() override
    {
        viewport.setBounds (getLocalBounds());

        // Set content size - width matches viewport, height is as needed
        int contentHeight = content->getRequiredHeight();
        content->setSize (viewport.getWidth() - viewport.getScrollBarThickness() - 4, contentHeight);
    }

private:
    ModulationManager& modManager;
    juce::Viewport viewport;
    std::unique_ptr<ModulationPanelContent> content;
};
