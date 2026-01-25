#pragma once
#include <JuceHeader.h>
#include <map>
#include "FFTProcessor.h"
#include "LoopRecorder.h"
#include "LoopAutomationEditor.h"
#include "Effects/EffectsChain.h"
#include "Modulation/ModulationManager.h"
#include "Modulation/ModulationPanel.h"

//==============================================================================
// Minimal, instrument-like look & feel (appearance only)
class MinimalLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MinimalLookAndFeel()
    {
        // Global palette
        const auto bg      = juce::Colour (0xff101114);
        const auto panel   = juce::Colour (0xff17181d);
        const auto panel2  = juce::Colour (0xff1d1f26);
        const auto text    = juce::Colour (0xffe9e9ea);
        const auto textDim = juce::Colour (0xffa9a9ad);
        const auto accent  = juce::Colour (0xff5dd6c6); // restrained mint accent

        setColour (juce::ResizableWindow::backgroundColourId, bg);

        setColour (juce::Label::textColourId, textDim);
        setColour (juce::TextButton::textColourOffId, text.withAlpha (0.85f));
        setColour (juce::TextButton::textColourOnId,  text);

        setColour (juce::TextButton::buttonColourId, panel2);
        setColour (juce::TextButton::buttonOnColourId, accent.withAlpha (0.18f));

        setColour (juce::ComboBox::backgroundColourId, panel2);
        setColour (juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha (0.08f));
        setColour (juce::ComboBox::textColourId, text.withAlpha (0.85f));
        setColour (juce::ComboBox::arrowColourId, textDim);

        setColour (juce::Slider::rotarySliderFillColourId, accent);
        setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::white.withAlpha (0.07f));
        setColour (juce::Slider::thumbColourId, text);
        setColour (juce::Slider::textBoxBackgroundColourId, panel);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::white.withAlpha (0.08f));
        setColour (juce::Slider::textBoxTextColourId, text.withAlpha (0.85f));

        setColour (juce::ToggleButton::textColourId, textDim);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override
    {
        return juce::Font (juce::Font::getDefaultSansSerifFontName(),
                           juce::jlimit (11.0f, 14.0f, (float) buttonHeight * 0.38f),
                           juce::Font::plain);
    }

    void drawButtonBackground (juce::Graphics& g,
                               juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool isMouseOverButton,
                               bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);

        const float corner = 9.0f;
        const auto base = backgroundColour
                            .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.45f);

        auto fill = base;
        if (isButtonDown)      fill = fill.brighter (0.10f);
        else if (isMouseOverButton) fill = fill.brighter (0.06f);

        g.setColour (fill);
        g.fillRoundedRectangle (bounds, corner);

        // Subtle border
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawRoundedRectangle (bounds, corner, 1.0f);
    }

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height)
                          .reduced (4.0f);

        const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Track
        const float trackThickness = juce::jlimit (2.0f, 4.0f, radius * 0.18f);
        juce::Path track;
        track.addCentredArc (centre.x, centre.y,
                             radius - trackThickness * 0.5f,
                             radius - trackThickness * 0.5f,
                             0.0f,
                             rotaryStartAngle,
                             rotaryEndAngle,
                             true);

        g.setColour (slider.findColour (juce::Slider::rotarySliderOutlineColourId));
        g.strokePath (track, juce::PathStrokeType (trackThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Value arc
        juce::Path value;
        value.addCentredArc (centre.x, centre.y,
                             radius - trackThickness * 0.5f,
                             radius - trackThickness * 0.5f,
                             0.0f,
                             rotaryStartAngle,
                             angle,
                             true);

        g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId).withAlpha (slider.isEnabled() ? 0.95f : 0.45f));
        g.strokePath (value, juce::PathStrokeType (trackThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Pointer
        juce::Path p;
        const float pointerLen = radius * 0.72f;
        const float pointerTh  = juce::jlimit (1.5f, 2.6f, radius * 0.10f);
        p.addRoundedRectangle (-pointerTh * 0.5f, -pointerLen, pointerTh, pointerLen, pointerTh * 0.5f);

        g.setColour (slider.findColour (juce::Slider::thumbColourId).withAlpha (slider.isEnabled() ? 0.85f : 0.35f));
        g.fillPath (p, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
    }

    void drawTickBox (juce::Graphics& g,
                      juce::Component& component,
                      float x, float y, float w, float h,
                      bool ticked,
                      bool isEnabled,
                      bool isMouseOverButton,
                      bool isButtonDown) override
    {
        juce::ignoreUnused (component, isMouseOverButton, isButtonDown);

        auto r = juce::Rectangle<float> (x, y, w, h).reduced (1.0f);
        const float corner = 4.0f;

        g.setColour (juce::Colours::white.withAlpha (isEnabled ? 0.10f : 0.05f));
        g.fillRoundedRectangle (r, corner);

        g.setColour (juce::Colours::white.withAlpha (isEnabled ? 0.10f : 0.05f));
        g.drawRoundedRectangle (r, corner, 1.0f);

        if (ticked)
        {
            const auto accent = findColour (juce::Slider::rotarySliderFillColourId);
            g.setColour (accent.withAlpha (isEnabled ? 0.90f : 0.35f));

            juce::Path tick;
            tick.startNewSubPath (r.getX() + r.getWidth() * 0.22f, r.getCentreY());
            tick.lineTo          (r.getX() + r.getWidth() * 0.42f, r.getBottom() - r.getHeight() * 0.22f);
            tick.lineTo          (r.getRight() - r.getWidth() * 0.20f, r.getY() + r.getHeight() * 0.22f);

            g.strokePath (tick, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    void drawComboBox (juce::Graphics& g, int width, int height,
                       bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.5f);
        const float corner = 8.0f;

        g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle (bounds, corner);

        g.setColour (box.findColour (juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle (bounds, corner, 1.0f);

        // Arrow
        auto arrowZone = bounds.removeFromRight (18.0f).reduced (4.0f, 6.0f);
        juce::Path arrow;
        arrow.startNewSubPath (arrowZone.getX(), arrowZone.getY());
        arrow.lineTo (arrowZone.getCentreX(), arrowZone.getBottom());
        arrow.lineTo (arrowZone.getRight(), arrowZone.getY());

        g.setColour (box.findColour (juce::ComboBox::arrowColourId));
        g.strokePath (arrow, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (1, 1, box.getWidth() - 20, box.getHeight() - 2);
        label.setFont (juce::Font (juce::Font::getDefaultSansSerifFontName(), 12.0f, juce::Font::plain));
    }
};

//==============================================================================
// Loop info display showing waveform, length, and playhead position
class LoopInfoDisplay : public juce::Component, public juce::Timer
{
public:
    LoopInfoDisplay (LoopRecorder& recorder, int slotIndex)
        : loopRecorder (recorder), slot (slotIndex)
    {
        setSize (300, 150);
        startTimerHz (30);  // Update at 30 fps for smooth playhead
    }

    ~LoopInfoDisplay() override { stopTimer(); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff17181d));

        auto bounds = getLocalBounds().reduced (10);

        // Title
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.setFont (juce::Font (14.0f, juce::Font::bold));
        g.drawText ("Loop " + juce::String (slot + 1) + " Info", bounds.removeFromTop (20), juce::Justification::centred);

        bounds.removeFromTop (5);

        // Check if slot has content
        if (!loopRecorder.isSlotActive (slot))
        {
            g.setColour (juce::Colours::grey);
            g.setFont (12.0f);
            g.drawText ("No recorded content", bounds, juce::Justification::centred);
            return;
        }

        // Info text
        auto infoArea = bounds.removeFromTop (20);
        int length = loopRecorder.getSlotLength (slot);
        float progress = loopRecorder.getSlotProgress (slot);
        double sampleRate = loopRecorder.getSampleRate();

        float lengthSeconds = length / (float) sampleRate;
        float currentPos = progress * lengthSeconds;

        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.setFont (11.0f);
        g.drawText ("Length: " + juce::String (lengthSeconds, 2) + "s  |  Position: " +
                    juce::String (currentPos, 2) + "s", infoArea, juce::Justification::centred);

        bounds.removeFromTop (5);

        // Waveform display
        auto waveformArea = bounds.reduced (5);

        // Background
        g.setColour (juce::Colour (0xff1d1f26));
        g.fillRoundedRectangle (waveformArea.toFloat(), 4.0f);

        // Draw waveform
        if (length > 0)
        {
            g.setColour (juce::Colour (0xff5dd6c6).withAlpha (0.8f));

            const int width = waveformArea.getWidth();
            const float centerY = waveformArea.getCentreY();
            const float halfHeight = waveformArea.getHeight() * 0.4f;

            juce::Path waveform;
            bool firstPoint = true;

            for (int x = 0; x < width; ++x)
            {
                // Sample the buffer at this position
                int sampleIndex = (x * length) / width;
                float sample = getSampleFromBuffer (sampleIndex);

                float y = centerY - (sample * halfHeight);

                if (firstPoint)
                {
                    waveform.startNewSubPath ((float) (waveformArea.getX() + x), y);
                    firstPoint = false;
                }
                else
                {
                    waveform.lineTo ((float) (waveformArea.getX() + x), y);
                }
            }

            g.strokePath (waveform, juce::PathStrokeType (1.0f));

            // Draw playhead
            float playheadX = waveformArea.getX() + (progress * waveformArea.getWidth());
            g.setColour (juce::Colours::white);
            g.drawLine (playheadX, (float) waveformArea.getY(),
                        playheadX, (float) waveformArea.getBottom(), 2.0f);
        }
    }

    void timerCallback() override
    {
        repaint();
    }

private:
    LoopRecorder& loopRecorder;
    int slot;

    float getSampleFromBuffer (int index) const
    {
        // Access the loop buffer - need to add an accessor to LoopRecorder
        // For now return a simplified version
        return loopRecorder.getSampleAtIndex (slot, index);
    }
};

//==============================================================================
// Clickable progress bar for loop playhead position
class LoopProgressBar : public juce::Component
{
public:
    LoopProgressBar (LoopRecorder& recorder, int slotIndex)
        : loopRecorder (recorder), slot (slotIndex) {}

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background
        g.setColour (juce::Colour (0xff1d1f26));
        g.fillRoundedRectangle (bounds, 2.0f);

        // Progress fill
        if (loopRecorder.isSlotActive (slot))
        {
            float progress = loopRecorder.getSlotProgress (slot);
            auto fillBounds = bounds;
            fillBounds.setWidth (bounds.getWidth() * progress);

            g.setColour (juce::Colour (0xff5dd6c6).withAlpha (0.7f));
            g.fillRoundedRectangle (fillBounds, 2.0f);
        }

        // Border
        g.setColour (juce::Colours::white.withAlpha (0.1f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 2.0f, 1.0f);
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        // Open waveform dialog when clicked
        auto* info = new LoopInfoDisplay (loopRecorder, slot);
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned (info);
        options.dialogTitle = "Loop " + juce::String (slot + 1) + " Waveform";
        options.dialogBackgroundColour = juce::Colour (0xff17181d);
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        options.resizable = false;
        options.launchAsync();
    }

private:
    LoopRecorder& loopRecorder;
    int slot;
};

//==============================================================================
// Settings dialog for audio device configuration
class SettingsDialog : public juce::Component
{
public:
    SettingsDialog (juce::AudioDeviceManager& dm) : deviceManager (dm)
    {
        deviceSelector = std::make_unique<juce::AudioDeviceSelectorComponent>(
            deviceManager,
            0, 1,        // input channels min/max
            0, 2,        // output channels min/max
            true,        // show MIDI input (enabled for MIDI mapping)
            false,       // show MIDI output
            true,        // show sample rate
            true         // show buffer size
        );
        addAndMakeVisible (*deviceSelector);
        setSize (500, 450);
    }

    void resized() override
    {
        deviceSelector->setBounds (getLocalBounds().reduced (10));
    }

private:
    juce::AudioDeviceManager& deviceManager;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;
};

//==============================================================================
// System resources monitor dialog
class ResourceMonitor : public juce::Component, public juce::Timer
{
public:
    ResourceMonitor (std::atomic<float>& cpuLoadRef) : cpuLoadPtr (&cpuLoadRef)
    {
        setSize (300, 150);
        startTimerHz (4);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff1a1a2e));
        g.setColour (juce::Colours::white);
        g.setFont (14.0f);

        int y = 20;
        const int lineHeight = 28;

        g.drawText ("Audio Load:", 20, y, 120, 20, juce::Justification::left);
        g.setColour (juce::Colours::darkgrey);
        g.fillRect (130, y + 2, 140, 16);
        g.setColour (audioLoad > 80 ? juce::Colours::red : (audioLoad > 50 ? juce::Colours::orange : juce::Colours::limegreen));
        g.fillRect (130, y + 2, (int)(140.0f * audioLoad / 100.0f), 16);
        g.setColour (juce::Colours::white);
        g.drawText (juce::String (audioLoad, 1) + " %", 130, y + 2, 140, 16, juce::Justification::centred);

        y += lineHeight;
        g.drawText ("FFT Buffers:", 20, y, 120, 20, juce::Justification::left);
        g.drawText (formatBytes (processMemory), 150, y, 120, 20, juce::Justification::left);

        y += lineHeight;
        g.drawText ("FFT Size:", 20, y, 120, 20, juce::Justification::left);
        g.drawText ("32768 pts x2 (stereo)", 150, y, 140, 20, juce::Justification::left);

        y += lineHeight;
        g.drawText ("History:", 20, y, 120, 20, juce::Justification::left);
        g.drawText ("56 frames (~10s)", 150, y, 120, 20, juce::Justification::left);
    }

    void timerCallback() override
    {
        if (cpuLoadPtr != nullptr)
            audioLoad = cpuLoadPtr->load();
        updateStats();
        repaint();
    }

private:
    std::atomic<float>* cpuLoadPtr = nullptr;
    float cpuUsage = 0.0f;
    int64_t memoryUsed = 0;
    int64_t processMemory = 0;
    float audioLoad = 0.0f;

    void updateStats()
    {
        cpuUsage = audioLoad;
        int64_t perProcessor = (32768 * 4) + (32768 * 4) + (32768 * 2 * 4) + (56 * 16385 * 4) + (16385 * 4);
        processMemory = perProcessor * 2;
    }

    juce::String formatBytes (int64_t bytes)
    {
        if (bytes > 1024 * 1024 * 1024)
            return juce::String (bytes / (1024.0 * 1024.0 * 1024.0), 2) + " GB";
        if (bytes > 1024 * 1024)
            return juce::String (bytes / (1024.0 * 1024.0), 1) + " MB";
        if (bytes > 1024)
            return juce::String (bytes / 1024.0, 1) + " KB";
        return juce::String (bytes) + " B";
    }
};

//==============================================================================
class MainComponent  : public juce::Component,
                       public juce::AudioIODeviceCallback,
                       public juce::Timer,
                       public juce::MidiInputCallback
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Audio callbacks
    void audioDeviceAboutToStart (juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const juce::AudioIODeviceCallbackContext&) override;

    // MIDI input callback
    void handleIncomingMidiMessage (juce::MidiInput* source,
                                    const juce::MidiMessage& message) override;

private:
    // Helper to create a rotary knob with label
    void setupKnob (juce::Slider& knob, juce::Label& label, const juce::String& name,
                    double min, double max, double step, double initial,
                    const juce::String& suffix = "");

    // Helper to update effect button colors based on current chain order
    void updateEffectButtonColors();

    // Update the effect parameter knobs for selected effect
    void updateEffectParameterKnobs();

    // Update effect parameter knob values/enabled state for modulation
    void updateEffectParameterModulation();

    // Audio
    juce::AudioDeviceManager deviceManager;

    // Look & Feel (appearance only)
    std::unique_ptr<MinimalLookAndFeel> lookAndFeel;


    // Header buttons
    juce::TextButton settingsButton { "Settings" };
    juce::TextButton resourcesButton { "Resources" };
    juce::TextButton bypassButton { "Bypass" };
    juce::TextButton midiLearnButton { "MIDI Learn" };

    // Tab buttons for Drone/Loops/Modulation sections
    juce::TextButton droneTabButton { "Drone" };
    juce::TextButton loopsTabButton { "Loops" };
    juce::TextButton modulationTabButton { "Mod" };
    int activeTab = 1;  // 0 = Drone, 1 = Loops, 2 = Modulation (default to Loops)

    // Master controls
    juce::Slider masterVolumeKnob;
    juce::Label masterVolumeLabel;
    std::atomic<float> masterVolume { 1.0f };
    std::atomic<bool> masterBypass { true };  // Bypass enabled by default

    // MIDI mapping
    struct MidiMapping
    {
        enum TargetType { None, LoopButton, Knob, Button };
        TargetType type = None;
        int targetIndex = -1;  // Loop button 0-3, or button index
        juce::Slider* knobPtr = nullptr;  // For knob mappings
        juce::Button* buttonPtr = nullptr;  // For button mappings
    };
    std::map<int, MidiMapping> midiCCMappings;  // CC number -> mapping
    std::map<int, MidiMapping> midiNoteMappings;  // Note number -> mapping (for buttons)
    std::atomic<bool> midiLearnActive { false };
    MidiMapping midiLearnSelected;  // Currently selected control for learning
    std::atomic<bool> midiLearnHasSelection { false };
    void toggleMidiLearnMode();
    void exitMidiLearnMode();
    void selectControlForMidiLearn (juce::Slider* knob);
    void selectLoopButtonForMidiLearn (int loopIndex);
    void selectButtonForMidiLearn (juce::Button* button);
    void clearMidiLearnSelection();
    void processMidiLearn (int ccOrNote, bool isNote);  // Called from message thread

    // Resource monitor
    std::unique_ptr<ResourceMonitor> resourceMonitor;

    // ===== DRONE & PITCH SECTION =====
    juce::Slider dryWetKnob;       juce::Label dryWetLabel;
    juce::Slider smoothingKnob;   juce::Label smoothingLabel;
    juce::Slider thresholdKnob;   juce::Label thresholdLabel;
    juce::Slider tiltKnob;        juce::Label tiltLabel;
    juce::Slider delayKnob;       juce::Label delayLabel;
    juce::Slider decayKnob;       juce::Label decayLabel;
    juce::Slider historyKnob;     juce::Label historyLabel;
    juce::Slider stereoWidthKnob; juce::Label stereoWidthLabel;
    juce::ToggleButton peakToggle { "Peak Hold" };
    juce::ToggleButton phaseToggle { "Random Phase" };
    juce::Slider pitchKnob;       juce::Label pitchLabel;
    juce::Slider octaveKnob;      juce::Label octaveLabel;

    // ===== LOOPS SECTION =====
    LoopRecorder loopRecorder;
    juce::Slider loopMixKnob;     juce::Label loopMixLabel;  // 0 = all live, 1 = all loops
    std::array<juce::TextButton, 8> loopButtons;
    juce::TextButton clearLoopsButton { "Clear All" };
    std::atomic<float> loopMix { 0.0f };  // 0 = live only, 1 = loops only

    // Per-loop controls
    std::array<juce::Slider, 8> loopVolumeSliders;
    std::array<juce::Label, 8> loopVolumeLabels;
    std::array<juce::Slider, 8> loopHPSliders;
    std::array<juce::Label, 8> loopHPLabels;
    std::array<juce::Slider, 8> loopLPSliders;
    std::array<juce::Label, 8> loopLPLabels;
    std::array<juce::ComboBox, 8> loopPitchCombos;
    std::array<juce::Label, 8> loopPitchLabels;
    std::array<juce::TextButton, 8> loopAutoButtons;  // "Auto" button per loop
    std::array<std::unique_ptr<LoopProgressBar>, 8> loopProgressBars;  // Progress bar per loop

    // Metering
    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };
    std::atomic<float> cpuLoad { 0.0f };
    double currentSampleRate = 44100.0;
    int currentBufferSize = 512;
    void timerCallback() override;

    // Scratch mono buffer
    juce::AudioBuffer<float> monoBuffer;

    // STFT processors (one per channel for stereo decorrelation)
    FFTProcessor fftProcessorL;
    FFTProcessor fftProcessorR;
    std::atomic<float> stereoWidth { 1.0f };

    // ===== EFFECTS CHAIN =====
    EffectsChain effectsChain;

    // ===== MODULATION =====
    ModulationManager modulationManager;
    std::unique_ptr<ModulationPanel> modulationPanel;

    // Effects chain UI
    std::array<juce::TextButton, 6> effectButtons;
    juce::TextButton moveLeftButton { "<" };
    juce::TextButton moveRightButton { ">" };
    int selectedEffectSlot = 0;  // Currently selected slot (0 = Filter by default)

    // Effect parameter knobs (displayed in bottom row)
    static constexpr int maxParamKnobs = 10;
    std::array<juce::Slider, maxParamKnobs> paramKnobs;
    std::array<juce::Label, maxParamKnobs> paramLabels;
    std::array<juce::ComboBox, 4> paramCombos;
    std::array<juce::Label, 4> paramComboLabels;
    std::array<juce::ToggleButton, 4> paramToggles;
    int numActiveKnobs = 0;
    int numActiveCombos = 0;
    int numActiveToggles = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
