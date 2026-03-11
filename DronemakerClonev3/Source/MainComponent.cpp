#include "MainComponent.h"
#include <iostream>
#include "Effects/FilterEffect.h"
#include "Effects/DelayEffect.h"
#include "Effects/GranularEffect.h"
#include "Effects/TremoloEffect.h"
#include "Effects/DistortionEffect.h"
#include "Effects/TapeEffect.h"
#include "AutomationPresets.h"

MainComponent::MainComponent()
{
    // Appearance-only LookAndFeel (keeps all functionality unchanged)
    lookAndFeel = std::make_unique<MinimalLookAndFeel>();
    setLookAndFeel (lookAndFeel.get());

    // Initialize audio device
    auto err = deviceManager.initialise (1, 2, nullptr, true);
    if (err.isNotEmpty())
        DBG ("AudioDeviceManager init error: " + err);

    // Auto-select USB audio interface if available (for Pi kiosk mode)
    // Searches for Scarlett, USB, or any non-default device
    #if JUCE_LINUX
    {
        auto* currentType = deviceManager.getCurrentAudioDeviceType();
        if (currentType != nullptr)
        {
            auto deviceNames = currentType->getDeviceNames();
            juce::String bestDevice;
            for (const auto& name : deviceNames)
            {
                auto lower = name.toLowerCase();
                if (lower.contains ("scarlett"))
                {
                    bestDevice = name;
                    break;  // Scarlett is the top pick
                }
                else if (lower.contains ("usb") && bestDevice.isEmpty())
                {
                    bestDevice = name;
                }
            }
            if (bestDevice.isNotEmpty())
            {
                juce::AudioDeviceManager::AudioDeviceSetup autoSetup;
                deviceManager.getAudioDeviceSetup (autoSetup);
                autoSetup.outputDeviceName = bestDevice;
                autoSetup.inputDeviceName = bestDevice;
                deviceManager.setAudioDeviceSetup (autoSetup, true);
                std::cerr << "Auto-selected audio device: " << bestDevice << std::endl;
            }
        }
    }
    #endif

    // Set default buffer size and ensure input channel is enabled
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup (setup);
    setup.bufferSize = 2048;
    setup.inputChannels.setBit (0);
    deviceManager.setAudioDeviceSetup (setup, true);

    deviceManager.addAudioCallback (this);

    // Settings button
    settingsButton.onClick = [this] {
        auto* dialog = new SettingsDialog (deviceManager);
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned (dialog);
        options.dialogTitle = "Audio Settings";
        options.dialogBackgroundColour = juce::Colour (0xff2a2b31);
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        options.resizable = false;
        options.launchAsync();
    };
    addAndMakeVisible (settingsButton);

    // Resources button
    resourcesButton.onClick = [this] {
        auto* monitor = new ResourceMonitor (cpuLoad);
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned (monitor);
        options.dialogTitle = "System Resources";
        options.dialogBackgroundColour = juce::Colour (0xff1a1a2e);
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        options.resizable = false;
        options.launchAsync();
    };
    addAndMakeVisible (resourcesButton);

    // Master bypass button (starts active/red since bypass is on by default)
    bypassButton.setColour (juce::TextButton::buttonColourId, juce::Colours::red);
    bypassButton.onClick = [this] {
        bool newState = !masterBypass.load();
        masterBypass.store (newState);
        if (usePiLayout)
        {
            bypassButton.setButtonText (newState ? "OFF" : "ON");
            bypassButton.setColour (juce::TextButton::buttonColourId,
                                     newState ? PiColours::recording.withAlpha (0.3f) : PiColours::buttonBg);
            bypassButton.setColour (juce::TextButton::textColourOffId,
                                     newState ? PiColours::recording : PiColours::textNormal);
        }
        else
        {
            bypassButton.setColour (juce::TextButton::buttonColourId,
                                     newState ? juce::Colours::red : juce::Colour (0xff2a2b31));
        }
    };
    addAndMakeVisible (bypassButton);

    // MIDI Learn button
    midiLearnButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2a2b31));
    midiLearnButton.onClick = [this] {
        toggleMidiLearnMode();
    };
    addAndMakeVisible (midiLearnButton);

    // Tab buttons for Drone/Loops/Modulation sections
    auto updateTabColors = [this] {
        droneTabButton.setColour (juce::TextButton::buttonColourId,
            activeTab == 0 ? juce::Colour (0xff5dd6c6).withAlpha (0.3f) : juce::Colour (0xff2a2b31));
        loopsTabButton.setColour (juce::TextButton::buttonColourId,
            activeTab == 1 ? juce::Colour (0xff5dd6c6).withAlpha (0.3f) : juce::Colour (0xff2a2b31));
        modulationTabButton.setColour (juce::TextButton::buttonColourId,
            activeTab == 2 ? juce::Colour (0xff5dd6c6).withAlpha (0.3f) : juce::Colour (0xff2a2b31));
    };

    droneTabButton.onClick = [this, updateTabColors] {
        activeTab = 0;
        updateTabColors();
        resized();
    };
    addAndMakeVisible (droneTabButton);

    loopsTabButton.onClick = [this, updateTabColors] {
        activeTab = 1;
        updateTabColors();
        resized();
    };
    addAndMakeVisible (loopsTabButton);

    modulationTabButton.onClick = [this, updateTabColors] {
        activeTab = 2;
        updateTabColors();
        resized();
    };
    addAndMakeVisible (modulationTabButton);

    updateTabColors();  // Set initial colors

    // Create modulation panel
    modulationPanel = std::make_unique<ModulationPanel> (modulationManager);
    addAndMakeVisible (modulationPanel.get());

    // Master volume knob
    masterVolumeKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    masterVolumeKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    masterVolumeKnob.setRange (0.0, 1.5, 0.01);  // Allow boost up to 150%
    masterVolumeKnob.setValue (1.0);
    masterVolumeKnob.onValueChange = [this] {
        masterVolume.store ((float) masterVolumeKnob.getValue());
    };
    masterVolumeKnob.onDragStart = [this]() {
        if (midiLearnActive.load())
            selectControlForMidiLearn (&masterVolumeKnob);
    };
    addAndMakeVisible (masterVolumeKnob);
    masterVolumeLabel.setText ("Master", juce::dontSendNotification);
    masterVolumeLabel.setVisible (false);  // Will be drawn rotated

    // Enable MIDI input for all available devices (also re-scanned periodically in timerCallback)
    refreshMidiInputs();

    // ===== DRONE SECTION =====
    // Helper for knobs with rotated labels (no value display)
    // Labels will be drawn rotated in paint(), so we don't make them visible
    auto setupRotatedKnob = [this](juce::Slider& knob, juce::Label& label, const juce::String& name,
                                    double min, double max, double step, double initial) {
        knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        knob.setRange (min, max, step);
        knob.setValue (initial);

        // MIDI learn click support
        knob.onDragStart = [this, &knob]() {
            if (midiLearnActive.load())
                selectControlForMidiLearn (&knob);
        };

        addAndMakeVisible (knob);

        // Store label text but don't show it - we'll draw it rotated
        label.setText (name, juce::dontSendNotification);
        label.setVisible (false);
    };

    setupRotatedKnob (dryWetKnob, dryWetLabel, "Dry/Wet", 0.0, 1.0, 0.01, 0.2);
    dryWetKnob.onValueChange = [this] {
        float v = (float) dryWetKnob.getValue();
        fftProcessorL.setInterpFactor (v);
        fftProcessorR.setInterpFactor (v);
    };

    setupRotatedKnob (smoothingKnob, smoothingLabel, "Smooth", 0.01, 0.5, 0.01, 0.05);
    smoothingKnob.onValueChange = [this] {
        float v = (float) smoothingKnob.getValue();
        fftProcessorL.setSmoothingFactor (v);
        fftProcessorR.setSmoothingFactor (v);
    };

    setupRotatedKnob (thresholdKnob, thresholdLabel, "Thresh", 0.0, 0.01, 0.0001, 0.001);
    thresholdKnob.onValueChange = [this] {
        float v = (float) thresholdKnob.getValue();
        fftProcessorL.setThreshold (v);
        fftProcessorR.setThreshold (v);
    };

    setupRotatedKnob (tiltKnob, tiltLabel, "Tilt", -6.0, 6.0, 0.1, 0.0);
    tiltKnob.onValueChange = [this] {
        float v = (float) tiltKnob.getValue();
        fftProcessorL.setSpectralTilt (v);
        fftProcessorR.setSpectralTilt (v);
    };

    setupRotatedKnob (delayKnob, delayLabel, "Delay", 0.0, 1.0, 0.01, 0.0);
    delayKnob.onValueChange = [this] {
        float v = (float) delayKnob.getValue();
        fftProcessorL.setDroneDelay (v);
        fftProcessorR.setDroneDelay (v);
    };

    setupRotatedKnob (decayKnob, decayLabel, "Decay", 0.99, 1.0, 0.0001, 1.0);
    decayKnob.onValueChange = [this] {
        float v = (float) decayKnob.getValue();
        fftProcessorL.setDecayRate (v);
        fftProcessorR.setDecayRate (v);
    };

    setupRotatedKnob (historyKnob, historyLabel, "History", 0.5, 10.0, 0.1, 4.5);
    historyKnob.onValueChange = [this] {
        float v = (float) historyKnob.getValue();
        fftProcessorL.setHistorySeconds (v);
        fftProcessorR.setHistorySeconds (v);
    };

    setupRotatedKnob (stereoWidthKnob, stereoWidthLabel, "Stereo", 0.0, 1.0, 0.01, 1.0);
    stereoWidthKnob.onValueChange = [this] {
        stereoWidth.store ((float) stereoWidthKnob.getValue());
    };

    peakToggle.setToggleState (true, juce::dontSendNotification);
    peakToggle.onClick = [this] {
        bool v = peakToggle.getToggleState();
        fftProcessorL.setUsePeakAmplitudes (v);
        fftProcessorR.setUsePeakAmplitudes (v);
    };
    addAndMakeVisible (peakToggle);

    phaseToggle.setToggleState (true, juce::dontSendNotification);
    phaseToggle.onClick = [this] {
        bool v = phaseToggle.getToggleState();
        fftProcessorL.setRandomizePhases (v);
        fftProcessorR.setRandomizePhases (v);
    };
    addAndMakeVisible (phaseToggle);

    // ===== PITCH SECTION (now part of Drone column) =====
    // Pitch knobs with rotated labels and value display
    pitchKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    pitchKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 14);
    pitchKnob.setRange (-24.0, 24.0, 0.1);
    pitchKnob.setValue (0.0);
    pitchKnob.setTextValueSuffix (" st");
    pitchKnob.onValueChange = [this] {
        float v = (float) pitchKnob.getValue();
        fftProcessorL.setPitchShiftSemitones (v);
        fftProcessorR.setPitchShiftSemitones (v);
    };
    pitchKnob.onDragStart = [this]() {
        if (midiLearnActive.load())
            selectControlForMidiLearn (&pitchKnob);
    };
    addAndMakeVisible (pitchKnob);
    pitchLabel.setText ("Semitones", juce::dontSendNotification);
    pitchLabel.setVisible (false);  // Will be drawn rotated

    octaveKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    octaveKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 14);
    octaveKnob.setRange (-4.0, 4.0, 1.0);
    octaveKnob.setValue (0.0);
    octaveKnob.setTextValueSuffix (" oct");
    octaveKnob.onValueChange = [this] {
        float v = (float) octaveKnob.getValue();
        fftProcessorL.setPitchShiftOctaves (v);
        fftProcessorR.setPitchShiftOctaves (v);
    };
    octaveKnob.onDragStart = [this]() {
        if (midiLearnActive.load())
            selectControlForMidiLearn (&octaveKnob);
    };
    addAndMakeVisible (octaveKnob);
    octaveLabel.setText ("Octaves", juce::dontSendNotification);
    octaveLabel.setVisible (false);  // Will be drawn rotated

    // Live/Loop mix knob (in drone section, same row as pitch)
    loopMixKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    loopMixKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    loopMixKnob.setRange (0.0, 1.0, 0.01);
    loopMixKnob.setValue (0.0);
    loopMixKnob.onValueChange = [this] {
        loopMix.store ((float) loopMixKnob.getValue());
    };
    loopMixKnob.onDragStart = [this]() {
        if (midiLearnActive.load())
            selectControlForMidiLearn (&loopMixKnob);
    };
    addAndMakeVisible (loopMixKnob);
    loopMixLabel.setText ("Live/Loop", juce::dontSendNotification);
    loopMixLabel.setVisible (false);  // Will be drawn rotated

    // ===== LOOPS SECTION =====

    // Per-loop controls
    for (int i = 0; i < 8; ++i)
    {
        // Volume slider (vertical slider)
        loopVolumeSliders[i].setSliderStyle (juce::Slider::LinearVertical);
        loopVolumeSliders[i].setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        loopVolumeSliders[i].setRange (0.0, 1.0, 0.01);
        loopVolumeSliders[i].setValue (1.0);
        loopVolumeSliders[i].onValueChange = [this, i] {
            loopRecorder.setSlotVolume (i, (float) loopVolumeSliders[i].getValue());
        };
        loopVolumeSliders[i].onDragStart = [this, i]() {
            if (midiLearnActive.load())
                selectControlForMidiLearn (&loopVolumeSliders[i]);
        };
        addAndMakeVisible (loopVolumeSliders[i]);

        loopVolumeLabels[i].setText ("Vol", juce::dontSendNotification);
        loopVolumeLabels[i].setJustificationType (juce::Justification::centred);
        loopVolumeLabels[i].setFont (juce::Font (9.0f));
        loopVolumeLabels[i].setColour (juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible (loopVolumeLabels[i]);

        // High-pass knob with frequency display
        loopHPSliders[i].setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        loopHPSliders[i].setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        loopHPSliders[i].setRange (20.0, 2000.0, 1.0);
        loopHPSliders[i].setSkewFactorFromMidPoint (200.0);
        loopHPSliders[i].setValue (20.0);
        loopHPSliders[i].onValueChange = [this, i] {
            loopRecorder.setSlotHighPass (i, (float) loopHPSliders[i].getValue());
            int freq = (int) loopHPSliders[i].getValue();
            loopHPLabels[i].setText (juce::String (freq) + " Hz", juce::dontSendNotification);
        };
        loopHPSliders[i].onDragStart = [this, i]() {
            if (midiLearnActive.load())
                selectControlForMidiLearn (&loopHPSliders[i]);
        };
        addAndMakeVisible (loopHPSliders[i]);

        loopHPLabels[i].setText ("20 Hz", juce::dontSendNotification);
        loopHPLabels[i].setJustificationType (juce::Justification::centred);
        loopHPLabels[i].setFont (juce::Font (9.0f));
        loopHPLabels[i].setColour (juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible (loopHPLabels[i]);

        // Low-pass knob with frequency display
        loopLPSliders[i].setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        loopLPSliders[i].setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        loopLPSliders[i].setRange (500.0, 20000.0, 1.0);
        loopLPSliders[i].setSkewFactorFromMidPoint (4000.0);
        loopLPSliders[i].setValue (20000.0);
        loopLPSliders[i].onValueChange = [this, i] {
            loopRecorder.setSlotLowPass (i, (float) loopLPSliders[i].getValue());
            int freq = (int) loopLPSliders[i].getValue();
            if (freq >= 1000)
                loopLPLabels[i].setText (juce::String (freq / 1000.0f, 1) + "k", juce::dontSendNotification);
            else
                loopLPLabels[i].setText (juce::String (freq) + " Hz", juce::dontSendNotification);
        };
        loopLPSliders[i].onDragStart = [this, i]() {
            if (midiLearnActive.load())
                selectControlForMidiLearn (&loopLPSliders[i]);
        };
        addAndMakeVisible (loopLPSliders[i]);

        loopLPLabels[i].setText ("20k", juce::dontSendNotification);
        loopLPLabels[i].setJustificationType (juce::Justification::centred);
        loopLPLabels[i].setFont (juce::Font (9.0f));
        loopLPLabels[i].setColour (juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible (loopLPLabels[i]);

        // Pitch selector
        loopPitchCombos[i].addItem ("-2 oct", 1);
        loopPitchCombos[i].addItem ("-1 oct", 2);
        loopPitchCombos[i].addItem ("Normal", 3);
        loopPitchCombos[i].addItem ("+1 oct", 4);
        loopPitchCombos[i].addItem ("+2 oct", 5);
        loopPitchCombos[i].setSelectedId (3);
        loopPitchCombos[i].onChange = [this, i] {
            int octave = loopPitchCombos[i].getSelectedId() - 3;  // -2, -1, 0, +1, +2
            loopRecorder.setSlotPitchOctave (i, octave);
        };
        addAndMakeVisible (loopPitchCombos[i]);

        loopPitchLabels[i].setText ("Pitch", juce::dontSendNotification);
        loopPitchLabels[i].setJustificationType (juce::Justification::centred);
        loopPitchLabels[i].setFont (juce::Font (9.0f));
        loopPitchLabels[i].setColour (juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible (loopPitchLabels[i]);

        // Auto button for loop automation settings
        loopAutoButtons[i].setButtonText ("Auto");
        loopAutoButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2a2b31));
        loopAutoButtons[i].onClick = [this, i] {
            auto* editor = new LoopAutomationEditor (loopRecorder, i);
            juce::DialogWindow::LaunchOptions options;
            options.content.setOwned (editor);
            options.dialogTitle = "Loop " + juce::String (i + 1) + " Automation";
            options.dialogBackgroundColour = juce::Colour (0xff17181d);
            options.escapeKeyTriggersCloseButton = true;
            options.useNativeTitleBar = true;
            options.resizable = false;
            options.launchAsync();
        };
        addAndMakeVisible (loopAutoButtons[i]);

        // Progress bar for loop playhead position (click to show waveform)
        loopProgressBars[i] = std::make_unique<LoopProgressBar> (loopRecorder, i);
        addAndMakeVisible (*loopProgressBars[i]);

        // Loop button
        loopButtons[i].setButtonText ("Loop " + juce::String (i + 1));
        loopButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2a2b31));
        loopButtons[i].onClick = [this, i] {
            // In MIDI learn mode, select this button for mapping
            if (midiLearnActive.load())
            {
                selectLoopButtonForMidiLearn (i);
                return;
            }

            if (loopRecorder.isRecording() && loopRecorder.getRecordingSlot() == i)
                loopRecorder.stopRecording();
            else if (loopRecorder.isSlotActive (i))
                loopRecorder.clearSlot (i);
            else
                loopRecorder.startRecording (i);
        };
        addAndMakeVisible (loopButtons[i]);
    }

    clearLoopsButton.onClick = [this] {
        loopRecorder.clearAll();
    };
    addAndMakeVisible (clearLoopsButton);

    // ===== EFFECTS CHAIN SECTION =====
    const char* effectNames[] = { "Filter", "Delay", "Granular", "Tremolo", "Distort", "Tape" };

    for (int i = 0; i < 6; ++i)
    {
        effectButtons[i].setButtonText (effectNames[i]);
        effectButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2a2b31));
        effectButtons[i].onClick = [this, i] {
            // Single click: select this effect and show its parameters
            // Right-click or ctrl-click: toggle enable
            if (juce::ModifierKeys::currentModifiers.isRightButtonDown() ||
                juce::ModifierKeys::currentModifiers.isCtrlDown())
            {
                // Toggle enable
                auto order = effectsChain.getOrder();
                int effectType = order[i];
                auto* effect = effectsChain.getEffect (effectType);
                if (effect)
                    effect->setEnabled (!effect->isEnabled());
                updateEffectButtonColors();
            }
            else
            {
                // Select this slot
                selectedEffectSlot = i;
                updateEffectButtonColors();
                updateEffectParameterKnobs();
            }
        };
        addAndMakeVisible (effectButtons[i]);
    }

    moveLeftButton.onClick = [this] {
        if (selectedEffectSlot > 0)
        {
            effectsChain.swapPositions (selectedEffectSlot, selectedEffectSlot - 1);
            selectedEffectSlot--;
            updateEffectButtonColors();
            updateEffectParameterKnobs();
        }
    };
    addAndMakeVisible (moveLeftButton);

    moveRightButton.onClick = [this] {
        if (selectedEffectSlot >= 0 && selectedEffectSlot < 5)
        {
            effectsChain.swapPositions (selectedEffectSlot, selectedEffectSlot + 1);
            selectedEffectSlot++;
            updateEffectButtonColors();
            updateEffectParameterKnobs();
        }
    };
    addAndMakeVisible (moveRightButton);

    // ===== EFFECT PARAMETER KNOBS =====
    for (int i = 0; i < maxParamKnobs; ++i)
    {
        paramKnobs[i].setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        paramKnobs[i].setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 14);
        paramKnobs[i].setVisible (false);

        // Capture index for MIDI learn click handling
        paramKnobs[i].onDragStart = [this, i]() {
            if (midiLearnActive.load())
            {
                selectControlForMidiLearn (&paramKnobs[i]);
            }
        };

        addAndMakeVisible (paramKnobs[i]);

        paramLabels[i].setJustificationType (juce::Justification::centred);
        paramLabels[i].setFont (juce::Font (10.0f));
        paramLabels[i].setColour (juce::Label::textColourId, juce::Colours::lightgrey);
        paramLabels[i].setVisible (false);
        addAndMakeVisible (paramLabels[i]);
    }

    for (int i = 0; i < 4; ++i)
    {
        paramCombos[i].setVisible (false);
        addAndMakeVisible (paramCombos[i]);

        paramComboLabels[i].setJustificationType (juce::Justification::centred);
        paramComboLabels[i].setFont (juce::Font (10.0f));
        paramComboLabels[i].setColour (juce::Label::textColourId, juce::Colours::lightgrey);
        paramComboLabels[i].setVisible (false);
        addAndMakeVisible (paramComboLabels[i]);

        paramToggles[i].setVisible (false);
        addAndMakeVisible (paramToggles[i]);
    }

    startTimerHz (30);

    // Detect Pi layout: use on Linux, or override with command line
    #if JUCE_LINUX
        usePiLayout = true;
    #endif
    if (juce::JUCEApplication::getCommandLineParameterArray().contains ("--pi-layout"))
        usePiLayout = true;
    if (juce::JUCEApplication::getCommandLineParameterArray().contains ("--desktop-layout"))
        usePiLayout = false;

    if (usePiLayout)
    {
        initPiLayout();
        setSize (1024, 600);
    }
    else
    {
        setSize (1000, 700);
    }

    // Initialize with Filter selected
    updateEffectButtonColors();
    updateEffectParameterKnobs();
}

MainComponent::~MainComponent()
{
    setLookAndFeel (nullptr);
    lookAndFeel.reset();

    // Clean up MIDI callbacks for all devices we registered
    for (const auto& id : knownMidiInputIds)
        deviceManager.removeMidiInputDeviceCallback (id, this);

    deviceManager.removeAudioCallback (this);
}

void MainComponent::setupKnob (juce::Slider& knob, juce::Label& label, const juce::String& name,
                                double min, double max, double step, double initial,
                                const juce::String& suffix)
{
    knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 52, 14);
    knob.setRange (min, max, step);
    knob.setValue (initial);
    if (suffix.isNotEmpty())
        knob.setTextValueSuffix (suffix);
    addAndMakeVisible (knob);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (10.5f, juce::Font::plain));
    label.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible (label);
}

void MainComponent::updateEffectButtonColors()
{
    const char* effectNames[] = { "Filter", "Delay", "Granular", "Tremolo", "Distort", "Tape" };
    auto order = effectsChain.getOrder();

    for (int i = 0; i < 6; ++i)
    {
        int effectType = order[i];
        auto* effect = effectsChain.getEffect (effectType);
        bool enabled = effect ? effect->isEnabled() : true;

        // Show effect name with enable indicator
        juce::String buttonText = effectNames[effectType];
        if (!enabled)
            buttonText = "[" + buttonText + "]";

        effectButtons[i].setButtonText (buttonText);

        if (i == selectedEffectSlot)
        {
            effectButtons[i].setColour (juce::TextButton::buttonColourId,
                usePiLayout ? PiColours::accent.withAlpha (0.25f) : juce::Colour (0xff5dd6c6));
            effectButtons[i].setColour (juce::TextButton::textColourOffId,
                usePiLayout ? PiColours::accentBright : juce::Colours::white);
        }
        else if (enabled)
        {
            effectButtons[i].setColour (juce::TextButton::buttonColourId,
                usePiLayout ? PiColours::buttonBg : juce::Colour (0xff1f2a27));
            effectButtons[i].setColour (juce::TextButton::textColourOffId,
                usePiLayout ? PiColours::textNormal : juce::Colours::white);
        }
        else
        {
            effectButtons[i].setColour (juce::TextButton::buttonColourId,
                usePiLayout ? juce::Colour (0xff14161c) : juce::Colour (0xff2a2b31));
            effectButtons[i].setColour (juce::TextButton::textColourOffId,
                usePiLayout ? PiColours::textDim : juce::Colours::white.withAlpha (0.5f));
        }
    }
}

void MainComponent::updateEffectParameterKnobs()
{
    // If we're in Pi loop mode, delegate to loop knobs instead
    if (usePiLayout && piLoopMode)
    {
        updateLoopParameterKnobs();
        return;
    }

    // Hide all existing param controls
    for (int i = 0; i < maxParamKnobs; ++i)
    {
        paramKnobs[i].setVisible (false);
        paramKnobs[i].onValueChange = nullptr;
        paramKnobs[i].textFromValueFunction = nullptr;
        paramLabels[i].setVisible (false);
    }
    for (int i = 0; i < 4; ++i)
    {
        paramCombos[i].setVisible (false);
        paramCombos[i].onChange = nullptr;
        paramCombos[i].clear();
        paramComboLabels[i].setVisible (false);
        paramToggles[i].setVisible (false);
        paramToggles[i].onClick = nullptr;
    }

    numActiveKnobs = 0;
    numActiveCombos = 0;
    numActiveToggles = 0;
    encoderBank.clearAll();

    if (selectedEffectSlot < 0)
        return;

    auto order = effectsChain.getOrder();
    int effectType = order[selectedEffectSlot];

    auto addKnob = [this](const juce::String& name, double min, double max, double step,
                          double value, std::function<void(float)> onChange, const juce::String& suffix = "") {
        if (numActiveKnobs >= maxParamKnobs) return;
        int knobIndex = numActiveKnobs;
        auto& knob = paramKnobs[knobIndex];
        auto& label = paramLabels[knobIndex];

        knob.setRange (min, max, step);
        knob.setValue (value, juce::dontSendNotification);
        knob.setTextValueSuffix (suffix);
        knob.onValueChange = [&knob, onChange] { onChange ((float) knob.getValue()); };
        knob.setVisible (true);

        label.setText (name, juce::dontSendNotification);
        label.setVisible (true);

        // Bind to virtual encoder bank for MIDI CC control
        if (knobIndex < VirtualEncoderBank::numEncoders)
        {
            encoderBank.bindEncoder (knobIndex, name, min, max, step,
                [this, knobIndex, onChange](float v) {
                    paramKnobs[knobIndex].setValue (v, juce::dontSendNotification);
                    onChange (v);
                },
                [this, knobIndex]() -> float {
                    return (float) paramKnobs[knobIndex].getValue();
                },
                suffix);
        }

        numActiveKnobs++;
    };

    // On Pi, everything is a knob. On desktop, keep combos/toggles.
    auto addCombo = [this, &addKnob](const juce::String& name, juce::StringArray items, int selectedIdx,
                           std::function<void(int)> onChange) {
        if (usePiLayout)
        {
            // Render as a discrete knob (0 to N-1), showing item name in text box
            int knobIdx = numActiveKnobs;  // capture before addKnob increments it
            addKnob (name, 0, items.size() - 1, 1, selectedIdx,
                     [onChange](float v) { onChange ((int) v); });
            paramKnobs[knobIdx].textFromValueFunction = [items](double v) {
                int idx = juce::jlimit (0, items.size() - 1, (int) v);
                return items[idx];
            };
            paramKnobs[knobIdx].updateText();
            return;
        }
        if (numActiveCombos >= 4) return;
        auto& combo = paramCombos[numActiveCombos];
        auto& label = paramComboLabels[numActiveCombos];

        combo.clear();
        for (int i = 0; i < items.size(); ++i)
            combo.addItem (items[i], i + 1);
        combo.setSelectedId (selectedIdx + 1, juce::dontSendNotification);
        combo.onChange = [&combo, onChange] { onChange (combo.getSelectedId() - 1); };
        combo.setVisible (true);

        label.setText (name, juce::dontSendNotification);
        label.setVisible (true);

        numActiveCombos++;
    };

    auto addToggle = [this, &addKnob](const juce::String& name, bool value, std::function<void(bool)> onChange) {
        if (usePiLayout)
        {
            // Render as a knob: 0 = Off, 1 = On, showing state name in text box
            int knobIdx = numActiveKnobs;
            addKnob (name, 0, 1, 1, value ? 1.0 : 0.0,
                     [onChange](float v) { onChange (v >= 0.5f); });
            paramKnobs[knobIdx].textFromValueFunction = [](double v) {
                return juce::String (v >= 0.5 ? "On" : "Off");
            };
            paramKnobs[knobIdx].updateText();
            return;
        }
        if (numActiveToggles >= 4) return;
        auto& toggle = paramToggles[numActiveToggles];

        toggle.setButtonText (name);
        toggle.setToggleState (value, juce::dontSendNotification);
        toggle.onClick = [&toggle, onChange] { onChange (toggle.getToggleState()); };
        toggle.setVisible (true);

        numActiveToggles++;
    };

    switch (effectType)
    {
        case EffectsChain::Filter:
            if (auto* fx = effectsChain.getFilter())
            {
                addKnob ("HP Freq", 20, 5000, 1, fx->getHighPassFreq(),
                         [fx](float v) { fx->setHighPassFreq (v); }, " Hz");
                addKnob ("HP Poles", 1, 8, 1, fx->getHighPassPoles(),
                         [fx](float v) { fx->setHighPassPoles ((int) v); });
                addKnob ("LP Freq", 200, 20000, 1, fx->getLowPassFreq(),
                         [fx](float v) { fx->setLowPassFreq (v); }, " Hz");
                addKnob ("LP Poles", 1, 8, 1, fx->getLowPassPoles(),
                         [fx](float v) { fx->setLowPassPoles ((int) v); });
                addKnob ("Harmonic", 0, 1, 0.01, fx->getHarmonicIntensity(),
                         [fx](float v) { fx->setHarmonicIntensity (v); });
                addToggle ("Harm On", fx->isHarmonicEnabled(),
                           [fx](bool v) { fx->setHarmonicEnabled (v); });
                addCombo ("Root", { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" },
                          fx->getRootNote(), [fx](int v) { fx->setRootNote (v); });
                addCombo ("Scale", { "Oct", "5th", "Ion", "Dor", "Phr", "Lyd", "Mix", "Aeo", "Loc", "PMaj", "PMin" },
                          fx->getScaleType(), [fx](int v) { fx->setScaleType (v); });
            }
            break;

        case EffectsChain::Delay:
            if (auto* fx = effectsChain.getDelay())
            {
                addKnob ("Time", 1, 2000, 1, fx->getDelayTimeMs(),
                         [fx](float v) { fx->setDelayTimeMs (v); }, " ms");
                addKnob ("Feedback", 0, 0.95, 0.01, fx->getFeedback(),
                         [fx](float v) { fx->setFeedback (v); });
                addKnob ("Dry/Wet", 0, 1, 0.01, fx->getDryWet(),
                         [fx](float v) { fx->setDryWet (v); });
                addToggle ("Ping Pong", fx->isPingPong(),
                           [fx](bool v) { fx->setPingPong (v); });
            }
            break;

        case EffectsChain::Granular:
            if (auto* fx = effectsChain.getGranular())
            {
                addKnob ("Grain Min", 10, 2000, 1, fx->getGrainSizeMin(),
                         [fx](float v) { fx->setGrainSizeMin (v); }, " ms");
                addKnob ("Grain Max", 10, 2000, 1, fx->getGrainSizeMax(),
                         [fx](float v) { fx->setGrainSizeMax (v); }, " ms");
                addKnob ("Delay Min", 10, 4000, 1, fx->getDelayMin(),
                         [fx](float v) { fx->setDelayMin (v); }, " ms");
                addKnob ("Delay Max", 10, 4000, 1, fx->getDelayMax(),
                         [fx](float v) { fx->setDelayMax (v); }, " ms");
                addKnob ("Pitch", -24, 24, 0.1, fx->getPitch(),
                         [fx](float v) { fx->setPitch (v); }, " st");
                addKnob ("Density", 0.1, 4, 0.1, fx->getDensity(),
                         [fx](float v) { fx->setDensity (v); });
                addKnob ("Spread", 0, 1, 0.01, fx->getSpread(),
                         [fx](float v) { fx->setSpread (v); });
                addKnob ("Dry/Wet", 0, 1, 0.01, fx->getDryWet(),
                         [fx](float v) { fx->setDryWet (v); });
            }
            break;

        case EffectsChain::Tremolo:
            if (auto* fx = effectsChain.getTremolo())
            {
                addKnob ("Rate", 0.1, 20, 0.1, fx->getRate(),
                         [fx](float v) { fx->setRate (v); }, " Hz");
                addKnob ("Depth", 0, 1, 0.01, fx->getDepth(),
                         [fx](float v) { fx->setDepth (v); });
                addCombo ("Wave", { "Sine", "Triangle", "Square" }, fx->getWaveform(),
                          [fx](int v) { fx->setWaveform (v); });
                addToggle ("Stereo", fx->isStereo(),
                           [fx](bool v) { fx->setStereo (v); });
            }
            break;

        case EffectsChain::Distortion:
            if (auto* fx = effectsChain.getDistortion())
            {
                addCombo ("Type", { "Soft", "Hard", "Fold", "Crush" }, fx->getAlgorithm(),
                          [fx](int v) { fx->setAlgorithm (v); });
                addKnob ("Drive", 1, 20, 0.1, fx->getDrive(),
                         [fx](float v) { fx->setDrive (v); });
                addKnob ("Tone", 0, 1, 0.01, fx->getTone(),
                         [fx](float v) { fx->setTone (v); });
                addKnob ("Dry/Wet", 0, 1, 0.01, fx->getDryWet(),
                         [fx](float v) { fx->setDryWet (v); });
                if (fx->getAlgorithm() == DistortionEffect::Bitcrush)
                {
                    addKnob ("Bits", 1, 16, 0.5, fx->getBitDepth(),
                             [fx](float v) { fx->setBitDepth (v); });
                    addKnob ("SR Reduce", 1, 64, 1, fx->getSampleRateReduction(),
                             [fx](float v) { fx->setSampleRateReduction (v); }, "x");
                }
            }
            break;

        case EffectsChain::Tape:
            if (auto* fx = effectsChain.getTape())
            {
                addKnob ("Saturation", 0, 2, 0.01, fx->getSaturation(),
                         [fx](float v) { fx->setSaturation (v); });
                addKnob ("Bias", 0, 1, 0.01, fx->getBias(),
                         [fx](float v) { fx->setBias (v); });
                addKnob ("Wow Rate", 0.1, 2, 0.01, fx->getWowRate(),
                         [fx](float v) { fx->setWowRate (v); }, " Hz");
                addKnob ("Wow Depth", 0, 1, 0.01, fx->getWowDepth(),
                         [fx](float v) { fx->setWowDepth (v); });
                addKnob ("Flutter Rate", 2, 15, 0.1, fx->getFlutterRate(),
                         [fx](float v) { fx->setFlutterRate (v); }, " Hz");
                addKnob ("Flutter Depth", 0, 1, 0.01, fx->getFlutterDepth(),
                         [fx](float v) { fx->setFlutterDepth (v); });
                addKnob ("HF Loss", 0, 1, 0.01, fx->getHfLoss(),
                         [fx](float v) { fx->setHfLoss (v); });
                addKnob ("Dry/Wet", 0, 1, 0.01, fx->getDryWet(),
                         [fx](float v) { fx->setDryWet (v); });
            }
            break;
    }

    resized();
}

void MainComponent::updateEffectParameterModulation()
{
    if (selectedEffectSlot < 0)
        return;

    auto order = effectsChain.getOrder();
    int effectType = order[selectedEffectSlot];

    // Define which knobs map to which modulation targets for each effect
    // The order must match the order knobs are added in updateEffectParameterKnobs()
    switch (effectType)
    {
        case EffectsChain::Filter:
        {
            // HP Freq, HP Poles, LP Freq, LP Poles, Harmonic
            if (numActiveKnobs > 0)
            {
                bool hpMod = modulationManager.isTargetModulated (ModulationTarget::Type::FilterHP);
                if (hpMod)
                {
                    float val = modulationManager.getModulatedValue (ModulationTarget::Type::FilterHP);
                    if (val >= 0.0f)
                        paramKnobs[0].setValue (val, juce::dontSendNotification);
                }
                paramKnobs[0].setEnabled (!hpMod);
            }
            if (numActiveKnobs > 2)
            {
                bool lpMod = modulationManager.isTargetModulated (ModulationTarget::Type::FilterLP);
                if (lpMod)
                {
                    float val = modulationManager.getModulatedValue (ModulationTarget::Type::FilterLP);
                    if (val >= 0.0f)
                        paramKnobs[2].setValue (val, juce::dontSendNotification);
                }
                paramKnobs[2].setEnabled (!lpMod);
            }
            if (numActiveKnobs > 4)
            {
                bool harmMod = modulationManager.isTargetModulated (ModulationTarget::Type::FilterHarmonicIntensity);
                if (harmMod)
                {
                    float val = modulationManager.getModulatedValue (ModulationTarget::Type::FilterHarmonicIntensity);
                    if (val >= 0.0f)
                        paramKnobs[4].setValue (val, juce::dontSendNotification);
                }
                paramKnobs[4].setEnabled (!harmMod);
            }
            break;
        }

        case EffectsChain::Delay:
        {
            // Time, Feedback, Dry/Wet
            if (numActiveKnobs > 0)
            {
                bool timeMod = modulationManager.isTargetModulated (ModulationTarget::Type::DelayTime);
                if (timeMod)
                {
                    float val = modulationManager.getModulatedValue (ModulationTarget::Type::DelayTime);
                    if (val >= 0.0f)
                        paramKnobs[0].setValue (val, juce::dontSendNotification);
                }
                paramKnobs[0].setEnabled (!timeMod);
            }
            if (numActiveKnobs > 1)
            {
                bool fbMod = modulationManager.isTargetModulated (ModulationTarget::Type::DelayFeedback);
                if (fbMod)
                {
                    float val = modulationManager.getModulatedValue (ModulationTarget::Type::DelayFeedback);
                    if (val >= 0.0f)
                        paramKnobs[1].setValue (val, juce::dontSendNotification);
                }
                paramKnobs[1].setEnabled (!fbMod);
            }
            if (numActiveKnobs > 2)
            {
                bool dwMod = modulationManager.isTargetModulated (ModulationTarget::Type::DelayDryWet);
                if (dwMod)
                {
                    float val = modulationManager.getModulatedValue (ModulationTarget::Type::DelayDryWet);
                    if (val >= 0.0f)
                        paramKnobs[2].setValue (val, juce::dontSendNotification);
                }
                paramKnobs[2].setEnabled (!dwMod);
            }
            break;
        }

        case EffectsChain::Tremolo:
        {
            // Rate, Depth
            if (numActiveKnobs > 0)
            {
                bool rateMod = modulationManager.isTargetModulated (ModulationTarget::Type::TremoloRate);
                if (rateMod)
                {
                    float val = modulationManager.getModulatedValue (ModulationTarget::Type::TremoloRate);
                    if (val >= 0.0f)
                        paramKnobs[0].setValue (val, juce::dontSendNotification);
                }
                paramKnobs[0].setEnabled (!rateMod);
            }
            if (numActiveKnobs > 1)
            {
                bool depthMod = modulationManager.isTargetModulated (ModulationTarget::Type::TremoloDepth);
                if (depthMod)
                {
                    float val = modulationManager.getModulatedValue (ModulationTarget::Type::TremoloDepth);
                    if (val >= 0.0f)
                        paramKnobs[1].setValue (val, juce::dontSendNotification);
                }
                paramKnobs[1].setEnabled (!depthMod);
            }
            break;
        }

        case EffectsChain::Distortion:
        {
            // Type (combo), Drive, Tone, Dry/Wet
            if (numActiveKnobs > 0)
            {
                bool driveMod = modulationManager.isTargetModulated (ModulationTarget::Type::DistortionDrive);
                if (driveMod)
                {
                    float val = modulationManager.getModulatedValue (ModulationTarget::Type::DistortionDrive);
                    if (val >= 0.0f)
                        paramKnobs[0].setValue (val, juce::dontSendNotification);
                }
                paramKnobs[0].setEnabled (!driveMod);
            }
            if (numActiveKnobs > 1)
            {
                bool toneMod = modulationManager.isTargetModulated (ModulationTarget::Type::DistortionTone);
                if (toneMod)
                {
                    float val = modulationManager.getModulatedValue (ModulationTarget::Type::DistortionTone);
                    if (val >= 0.0f)
                        paramKnobs[1].setValue (val, juce::dontSendNotification);
                }
                paramKnobs[1].setEnabled (!toneMod);
            }
            if (numActiveKnobs > 2)
            {
                bool dwMod = modulationManager.isTargetModulated (ModulationTarget::Type::DistortionDryWet);
                if (dwMod)
                {
                    float val = modulationManager.getModulatedValue (ModulationTarget::Type::DistortionDryWet);
                    if (val >= 0.0f)
                        paramKnobs[2].setValue (val, juce::dontSendNotification);
                }
                paramKnobs[2].setEnabled (!dwMod);
            }
            break;
        }

        case EffectsChain::Tape:
        {
            // Saturation, Bias, Wow Rate, Wow Depth, Flutter Rate, Flutter Depth, HF Loss, Dry/Wet
            if (numActiveKnobs > 0)
            {
                bool satMod = modulationManager.isTargetModulated (ModulationTarget::Type::TapeSaturation);
                if (satMod)
                {
                    float val = modulationManager.getModulatedValue (ModulationTarget::Type::TapeSaturation);
                    if (val >= 0.0f)
                        paramKnobs[0].setValue (val, juce::dontSendNotification);
                }
                paramKnobs[0].setEnabled (!satMod);
            }
            if (numActiveKnobs > 1)
            {
                bool biasMod = modulationManager.isTargetModulated (ModulationTarget::Type::TapeBias);
                if (biasMod)
                {
                    float val = modulationManager.getModulatedValue (ModulationTarget::Type::TapeBias);
                    if (val >= 0.0f)
                        paramKnobs[1].setValue (val, juce::dontSendNotification);
                }
                paramKnobs[1].setEnabled (!biasMod);
            }
            if (numActiveKnobs > 3)
            {
                bool wowMod = modulationManager.isTargetModulated (ModulationTarget::Type::TapeWowDepth);
                if (wowMod)
                {
                    float val = modulationManager.getModulatedValue (ModulationTarget::Type::TapeWowDepth);
                    if (val >= 0.0f)
                        paramKnobs[3].setValue (val, juce::dontSendNotification);
                }
                paramKnobs[3].setEnabled (!wowMod);
            }
            if (numActiveKnobs > 5)
            {
                bool flutMod = modulationManager.isTargetModulated (ModulationTarget::Type::TapeFlutterDepth);
                if (flutMod)
                {
                    float val = modulationManager.getModulatedValue (ModulationTarget::Type::TapeFlutterDepth);
                    if (val >= 0.0f)
                        paramKnobs[5].setValue (val, juce::dontSendNotification);
                }
                paramKnobs[5].setEnabled (!flutMod);
            }
            if (numActiveKnobs > 7)
            {
                bool dwMod = modulationManager.isTargetModulated (ModulationTarget::Type::TapeDryWet);
                if (dwMod)
                {
                    float val = modulationManager.getModulatedValue (ModulationTarget::Type::TapeDryWet);
                    if (val >= 0.0f)
                        paramKnobs[7].setValue (val, juce::dontSendNotification);
                }
                paramKnobs[7].setEnabled (!dwMod);
            }
            break;
        }

        default:
            break;
    }
}

void MainComponent::paint (juce::Graphics& g)
{
    if (usePiLayout) { paintPi (g); return; }

    // Background
    g.fillAll (juce::Colour (0xff101114));

    // Shared styling
    const auto panelFill   = juce::Colour (0xff17181d);
    const auto panelFill2  = juce::Colour (0xff1d1f26);
    const auto border      = juce::Colours::white.withAlpha (0.06f);
    const auto titleColour = juce::Colours::white.withAlpha (0.78f);
    const auto dimText     = juce::Colours::white.withAlpha (0.40f);

    auto drawPanel = [&](juce::Rectangle<int> area, const juce::String& title, bool showTitle = true)
    {
        // Keep the title strip inside the panel, but do not modify the caller's rectangle
        auto r = area;

        g.setColour (panelFill);
        g.fillRoundedRectangle (r.toFloat(), 12.0f);

        g.setColour (border);
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 12.0f, 1.0f);

        if (showTitle && title.isNotEmpty())
        {
            // Title - centered
            auto titleArea = r.removeFromTop (24);
            g.setColour (titleColour);
            g.setFont (juce::Font (12.0f, juce::Font::bold));
            g.drawText (title.toUpperCase(), titleArea, juce::Justification::centred, false);

            // Subtle divider under title
            g.setColour (juce::Colours::white.withAlpha (0.04f));
            g.drawLine ((float) area.getX() + 10.0f,
                        (float) area.getY() + 24.0f,
                        (float) area.getRight() - 10.0f,
                        (float) area.getY() + 24.0f,
                        1.0f);
        }
    };

    // Layout mirrors resized(): header row + two columns + chain row + param row
    auto bounds = getLocalBounds().reduced (10);

    // Header visual area (buttons are laid out in resized())
    auto header = bounds.removeFromTop (40);

    // Header background strip
    {
        auto headerBg = header.toFloat();
        g.setColour (panelFill2);
        g.fillRoundedRectangle (headerBg, 12.0f);

        g.setColour (border);
        g.drawRoundedRectangle (headerBg.reduced (0.5f), 12.0f, 1.0f);
    }

    bounds.removeFromTop (5);

    // Effect parameters section at bottom (no title)
    auto paramArea = bounds.removeFromBottom (100).reduced (5);
    drawPanel (paramArea, "", false);
    bounds.removeFromBottom (5);

    // Effects chain row (no title, taller buttons)
    auto effectsArea = bounds.removeFromBottom (70).reduced (5);
    drawPanel (effectsArea, "", false);
    bounds.removeFromBottom (5);

    // Single full-width panel for active tab
    auto mainArea = bounds.reduced (5);
    if (activeTab == 0)
        drawPanel (mainArea, "Drone");
    else
        drawPanel (mainArea, "Loops");

    // Draw rotated labels for drone knobs
    g.setColour (juce::Colours::lightgrey);
    g.setFont (juce::Font (10.0f));

    auto drawRotatedLabel = [&g](const juce::Label& label) {
        if (label.getText().isEmpty()) return;
        auto bounds = label.getBounds();

        // Save graphics state
        g.saveState();

        // Move to center of label area, rotate, then draw
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();

        g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, cx, cy));

        // Draw text centered in the rotated space
        g.drawText (label.getText(),
                    bounds.getX() - (bounds.getHeight() - bounds.getWidth()) / 2,
                    bounds.getY() + (bounds.getHeight() - bounds.getWidth()) / 2,
                    bounds.getHeight(), bounds.getWidth(),
                    juce::Justification::centred, false);

        g.restoreState();
    };

    // Drone section rotated labels (only when drone tab is active)
    if (activeTab == 0)
    {
        drawRotatedLabel (dryWetLabel);
        drawRotatedLabel (smoothingLabel);
        drawRotatedLabel (thresholdLabel);
        drawRotatedLabel (tiltLabel);
        drawRotatedLabel (delayLabel);
        drawRotatedLabel (decayLabel);
        drawRotatedLabel (historyLabel);
        drawRotatedLabel (stereoWidthLabel);
        drawRotatedLabel (pitchLabel);
        drawRotatedLabel (octaveLabel);
    }

    // Live/Loop mix label (always visible in header)
    drawRotatedLabel (loopMixLabel);

    // Master volume label (also rotated)
    drawRotatedLabel (masterVolumeLabel);

    // Input / Output meters inside the header strip (left side)
    {
        const int meterX = 16;
        int meterY = 14;
        const int meterW = 140;
        const int meterH = 10;
        const int gap = 6;

        auto drawMeter = [&](const juce::String& label, float value, juce::Colour colour)
        {
            auto bg = juce::Rectangle<float> ((float) meterX, (float) meterY, (float) meterW, (float) meterH);
            g.setColour (juce::Colours::white.withAlpha (0.06f));
            g.fillRoundedRectangle (bg, 4.0f);

            const float v = juce::jlimit (0.0f, 1.0f, value);
            auto fg = bg.withWidth (bg.getWidth() * v);

            g.setColour (colour.withAlpha (0.85f));
            g.fillRoundedRectangle (fg, 4.0f);

            g.setColour (juce::Colours::white.withAlpha (0.60f));
            g.setFont (juce::Font (10.0f, juce::Font::bold));
            g.drawText (label, meterX, meterY - 1, 30, meterH + 2, juce::Justification::centredLeft, false);

            meterY += meterH + gap;
        };

        drawMeter ("IN",  inputLevel.load(),  juce::Colour (0xff5dd6c6));
        drawMeter ("OUT", outputLevel.load(), juce::Colour (0xff7aa7ff));
    }
}

void MainComponent::resized()
{
    if (usePiLayout) { resizedPi(); return; }

    auto bounds = getLocalBounds().reduced (10);

    // Header row
    auto header = bounds.removeFromTop (35);
    header.removeFromLeft (170);  // Space for meters

    // Tab buttons (center-ish)
    droneTabButton.setBounds (header.removeFromLeft (60).reduced (2));
    header.removeFromLeft (3);
    loopsTabButton.setBounds (header.removeFromLeft (60).reduced (2));
    header.removeFromLeft (3);
    modulationTabButton.setBounds (header.removeFromLeft (50).reduced (2));
    header.removeFromLeft (15);

    // Live/Loop mix knob in header
    const int mixKnobSize = 32;
    const int mixLabelWidth = 50;
    loopMixLabel.setBounds (header.removeFromLeft (mixLabelWidth).reduced (0, 4));
    loopMixLabel.setVisible (true);  // Make visible in header
    loopMixKnob.setBounds (header.removeFromLeft (mixKnobSize).reduced (0, 2));
    header.removeFromLeft (10);

    // Right side buttons
    resourcesButton.setBounds (header.removeFromRight (80));
    header.removeFromRight (5);
    settingsButton.setBounds (header.removeFromRight (70));
    header.removeFromRight (5);
    midiLearnButton.setBounds (header.removeFromRight (85));
    header.removeFromRight (5);
    bypassButton.setBounds (header.removeFromRight (60));
    header.removeFromRight (10);

    // Master volume knob (with rotated label space to the left)
    const int masterKnobSize = 32;
    const int masterLabelWidth = 45;
    int masterX = header.getRight() - masterKnobSize - masterLabelWidth - 5;
    masterVolumeLabel.setBounds (masterX, header.getY(), masterLabelWidth, masterKnobSize);
    masterVolumeKnob.setBounds (masterX + masterLabelWidth, header.getY() + 2, masterKnobSize, masterKnobSize);

    bounds.removeFromTop (5);

    // Effect parameters section at bottom - 8 fixed slots
    auto paramArea = bounds.removeFromBottom (100).reduced (5);
    paramArea.removeFromTop (8);  // Small top padding

    // Always divide into 8 equal slots for physical knob mapping
    const int numSlots = 8;
    const int slotWidth = paramArea.getWidth() / numSlots;
    const int knobSize = 55;
    const int labelHeight = 14;

    // Position knobs in their fixed slots
    for (int i = 0; i < numActiveKnobs && i < numSlots; ++i)
    {
        int slotX = paramArea.getX() + i * slotWidth + (slotWidth - knobSize) / 2;
        paramLabels[i].setBounds (slotX, paramArea.getY(), knobSize, labelHeight);
        paramKnobs[i].setBounds (slotX, paramArea.getY() + labelHeight, knobSize, paramArea.getHeight() - labelHeight - 5);
    }

    // Position combos after knobs in their slots
    for (int i = 0; i < numActiveCombos && (numActiveKnobs + i) < numSlots; ++i)
    {
        int slot = numActiveKnobs + i;
        int slotX = paramArea.getX() + slot * slotWidth + (slotWidth - 65) / 2;
        paramComboLabels[i].setBounds (slotX, paramArea.getY(), 65, labelHeight);
        paramCombos[i].setBounds (slotX, paramArea.getY() + labelHeight + 15, 65, 22);
    }

    // Position toggles after combos in their slots
    for (int i = 0; i < numActiveToggles && (numActiveKnobs + numActiveCombos + i) < numSlots; ++i)
    {
        int slot = numActiveKnobs + numActiveCombos + i;
        int slotX = paramArea.getX() + slot * slotWidth + (slotWidth - 75) / 2;
        paramToggles[i].setBounds (slotX, paramArea.getY() + labelHeight + 15, 75, 22);
    }

    bounds.removeFromBottom (5);

    // Effects chain row (no title, taller buttons)
    auto effectsArea = bounds.removeFromBottom (70).reduced (5);
    effectsArea.removeFromTop (8);  // Small top padding

    // Move buttons on left - same size
    const int moveButtonSize = 30;
    moveLeftButton.setBounds (effectsArea.removeFromLeft (moveButtonSize).reduced (2));
    effectsArea.removeFromLeft (3);
    moveRightButton.setBounds (effectsArea.removeFromLeft (moveButtonSize).reduced (2));
    effectsArea.removeFromLeft (10);

    // Effect buttons (taller - about double height)
    const int effectWidth = (effectsArea.getWidth() - 10) / 6;
    for (int i = 0; i < 6; ++i)
    {
        effectButtons[i].setBounds (effectsArea.removeFromLeft (effectWidth).reduced (2));
    }

    bounds.removeFromBottom (5);

    // Show/hide drone controls based on active tab
    bool showDrone = (activeTab == 0);
    dryWetKnob.setVisible (showDrone);
    smoothingKnob.setVisible (showDrone);
    thresholdKnob.setVisible (showDrone);
    tiltKnob.setVisible (showDrone);
    delayKnob.setVisible (showDrone);
    decayKnob.setVisible (showDrone);
    historyKnob.setVisible (showDrone);
    stereoWidthKnob.setVisible (showDrone);
    peakToggle.setVisible (showDrone);
    phaseToggle.setVisible (showDrone);
    pitchKnob.setVisible (showDrone);
    octaveKnob.setVisible (showDrone);

    // Show/hide loop controls based on active tab
    bool showLoops = (activeTab == 1);
    for (int i = 0; i < 8; ++i)
    {
        loopButtons[i].setVisible (showLoops);
        loopAutoButtons[i].setVisible (showLoops);
        loopPitchCombos[i].setVisible (showLoops);
        loopHPSliders[i].setVisible (showLoops);
        loopHPLabels[i].setVisible (showLoops);
        loopLPSliders[i].setVisible (showLoops);
        loopLPLabels[i].setVisible (showLoops);
        loopVolumeSliders[i].setVisible (showLoops);
        loopVolumeLabels[i].setVisible (showLoops);
        if (loopProgressBars[i])
            loopProgressBars[i]->setVisible (showLoops);
    }
    clearLoopsButton.setVisible (showLoops);

    // Show/hide modulation panel based on active tab
    bool showModulation = (activeTab == 2);
    if (modulationPanel)
        modulationPanel->setVisible (showModulation);

    // Full width for active tab
    auto layoutKnobWithLabel = [&](juce::Slider& knob, juce::Label& label, juce::Rectangle<int>& area) {
        auto knobArea = area.removeFromLeft (knobSize + 4);
        label.setBounds (knobArea.removeFromTop (labelHeight));
        knob.setBounds (knobArea.reduced (2));
    };

    if (activeTab == 0)
    {
        // ===== DRONE SECTION (full width) =====
        auto droneArea = bounds.reduced (5);
        droneArea.removeFromTop (25);  // Title space

        // Layout with rotated labels to the left of knobs
        const int rotatedLabelWidth = 50;  // Width for rotated label area
        const int knobWithLabel = rotatedLabelWidth + knobSize;

        // Calculate equal spacing for 4 knobs per row
        const int numKnobsPerRow = 4;
        int droneSpacing = (droneArea.getWidth() - (numKnobsPerRow * knobWithLabel)) / (numKnobsPerRow + 1);

        auto layoutRotatedKnobs = [&](juce::Rectangle<int>& rowArea, std::initializer_list<std::pair<juce::Slider*, juce::Label*>> knobs) {
            int x = rowArea.getX() + droneSpacing;
            for (auto& [knob, label] : knobs)
            {
                // Label to the left of knob (will be drawn rotated in paint)
                label->setBounds (x, rowArea.getY(), rotatedLabelWidth, knobSize);
                knob->setBounds (x + rotatedLabelWidth, rowArea.getY(), knobSize, knobSize);
                x += knobWithLabel + droneSpacing;
            }
        };

        auto droneRow1 = droneArea.removeFromTop (knobSize + 5);
        layoutRotatedKnobs (droneRow1, {{&dryWetKnob, &dryWetLabel}, {&smoothingKnob, &smoothingLabel},
                                         {&thresholdKnob, &thresholdLabel}, {&tiltKnob, &tiltLabel}});

        auto droneRow2 = droneArea.removeFromTop (knobSize + 5);
        layoutRotatedKnobs (droneRow2, {{&delayKnob, &delayLabel}, {&decayKnob, &decayLabel},
                                         {&historyKnob, &historyLabel}, {&stereoWidthKnob, &stereoWidthLabel}});

        droneArea.removeFromTop (3);
        auto toggleRow = droneArea.removeFromTop (20);
        int toggleSpacing = (droneArea.getWidth() - 220) / 3;
        peakToggle.setBounds (toggleRow.getX() + toggleSpacing, toggleRow.getY(), 100, 18);
        phaseToggle.setBounds (toggleRow.getX() + toggleSpacing + 110, toggleRow.getY(), 120, 18);

        // Pitch controls row (Live/Loop mix is now in header)
        droneArea.removeFromTop (5);
        auto pitchRow = droneArea.removeFromTop (knobSize + labelHeight + 18);  // Extra for value display
        int pitchSpacing = (droneArea.getWidth() - 2 * knobWithLabel) / 3;
        int pitchX = pitchRow.getX() + pitchSpacing;

        // Semitones (with value display)
        pitchLabel.setBounds (pitchX, pitchRow.getY(), rotatedLabelWidth, knobSize);
        pitchKnob.setBounds (pitchX + rotatedLabelWidth, pitchRow.getY(), knobSize, knobSize + labelHeight + 4);
        pitchX += knobWithLabel + pitchSpacing;

        // Octaves (with value display)
        octaveLabel.setBounds (pitchX, pitchRow.getY(), rotatedLabelWidth, knobSize);
        octaveKnob.setBounds (pitchX + rotatedLabelWidth, pitchRow.getY(), knobSize, knobSize + labelHeight + 4);
    }
    else if (activeTab == 1)
    {
        // ===== LOOPS SECTION (full width) =====
        auto loopsArea = bounds.reduced (5);

        // Title row with Clear All button on right
        auto loopsTitleRow = loopsArea.removeFromTop (25);
        clearLoopsButton.setBounds (loopsTitleRow.removeFromRight (70).reduced (2, 3));

        loopsArea.removeFromTop (5);

        // Per-loop controls - all 8 loops in a single row
        const int loopStripWidth = loopsArea.getWidth() / 8;
        const int smallKnob = 32;

        for (int i = 0; i < 8; ++i)
        {
            auto strip = loopsArea.removeFromLeft (loopStripWidth).reduced (2, 0);

            // Progress bar at top (thin bar showing playhead position)
            loopProgressBars[i]->setBounds (strip.removeFromTop (6).reduced (1, 0));
            strip.removeFromTop (1);

            // Loop button below progress bar
            loopButtons[i].setBounds (strip.removeFromTop (22).reduced (1, 0));
            strip.removeFromTop (1);

            // Auto button directly under loop button
            loopAutoButtons[i].setBounds (strip.removeFromTop (16).reduced (2, 0));
            strip.removeFromTop (1);

            // Pitch selector directly under auto button (no label)
            loopPitchLabels[i].setVisible (false);
            loopPitchCombos[i].setBounds (strip.removeFromTop (20).reduced (2, 0));
            strip.removeFromTop (2);

            // HP knob
            loopHPLabels[i].setBounds (strip.removeFromTop (10));
            loopHPSliders[i].setBounds (strip.removeFromTop (smallKnob).reduced (4, 0));
            strip.removeFromTop (1);

            // LP knob
            loopLPLabels[i].setBounds (strip.removeFromTop (10));
            loopLPSliders[i].setBounds (strip.removeFromTop (smallKnob).reduced (4, 0));
            strip.removeFromTop (2);

            // Volume slider (vertical) - takes remaining space
            loopVolumeLabels[i].setBounds (strip.removeFromTop (10));
            loopVolumeSliders[i].setBounds (strip.reduced (8, 0));
        }
    }
    else if (activeTab == 2)
    {
        // ===== MODULATION SECTION (full width) =====
        if (modulationPanel)
            modulationPanel->setBounds (bounds.reduced (5));
    }
}

void MainComponent::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    const int blockSize = device->getCurrentBufferSizeSamples();
    monoBuffer.setSize (1, blockSize, false, false, true);
    monoBuffer.clear();

    currentSampleRate = device->getCurrentSampleRate();
    currentBufferSize = blockSize;

    float sr = static_cast<float>(currentSampleRate);
    fftProcessorL.setSampleRate (sr);
    fftProcessorL.reset();
    fftProcessorR.setSampleRate (sr);
    fftProcessorR.reset();

    loopRecorder.prepareToPlay (currentSampleRate, blockSize);
    effectsChain.prepareToPlay (currentSampleRate, blockSize);
    modulationManager.prepareToPlay (currentSampleRate, blockSize);
}

void MainComponent::audioDeviceStopped()
{
}

void MainComponent::audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                                     int numInputChannels,
                                                     float* const* outputChannelData,
                                                     int numOutputChannels,
                                                     int numSamples,
                                                     const juce::AudioIODeviceCallbackContext&)
{
    const auto startTime = juce::Time::getHighResolutionTicks();

    for (int ch = 0; ch < numOutputChannels; ++ch)
        if (auto* out = outputChannelData[ch])
            juce::FloatVectorOperations::clear (out, numSamples);

    if (numSamples <= 0 || monoBuffer.getNumSamples() < numSamples)
        return;

    float* mono = monoBuffer.getWritePointer (0);
    juce::FloatVectorOperations::clear (mono, numSamples);

    int activeInputs = 0;
    for (int ch = 0; ch < numInputChannels; ++ch)
    {
        const float* in = inputChannelData[ch];
        if (in == nullptr) continue;
        juce::FloatVectorOperations::add (mono, in, numSamples);
        ++activeInputs;
    }

    if (activeInputs > 0)
        juce::FloatVectorOperations::multiply (mono, 1.0f / (float) activeInputs, numSamples);

    if (activeInputs > 0)
    {
        float sumSquares = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            sumSquares += mono[i] * mono[i];
        inputLevel.store (std::sqrt (sumSquares / (float) numSamples) * 3.0f);
    }
    else
    {
        inputLevel.store (0.0f);
    }

    float outputSum = 0.0f;
    const float width = stereoWidth.load();
    const bool isRecording = loopRecorder.isRecording();
    const bool hasLoops = loopRecorder.hasAnyContent();
    const float mix = loopMix.load();  // 0 = live only, 1 = loops only
    const bool bypassed = masterBypass.load();
    const float volume = masterVolume.load();

    // Master bypass - just pass through silence (or could pass through dry input)
    if (bypassed)
    {
        // Still update loop recording if active
        if (isRecording && activeInputs > 0)
        {
            for (int i = 0; i < numSamples; ++i)
                loopRecorder.recordSample (mono[i]);
        }
        outputLevel.store (0.0f);
        return;
    }

    if (activeInputs > 0 || hasLoops)
    {
        // Modulation mix value (may be overridden by modulation)
        float currentMix = mix;
        float currentVolume = volume;

        for (int i = 0; i < numSamples; ++i)
        {
            float fftInput = 0.0f;
            float liveSample = 0.0f;

            if (activeInputs > 0)
            {
                liveSample = mono[i];
                if (isRecording)
                    loopRecorder.recordSample (liveSample);
            }

            float loopSample = hasLoops ? loopRecorder.getLoopMix() : 0.0f;

            // Feed spectrum visualiser on Pi — per-loop (post filter) or master output
            if (usePiLayout && piLoopDetail)
            {
                if (piLoopMode)
                {
                    int sel = TouchLoopButton::selectedSlot;
                    piLoopDetail->pushVisSample (loopRecorder.getSlotLastFilteredSample (sel));
                }
                // else: we'll push master output after processing below
            }

            // Crossfade between live and loops based on mix
            fftInput = liveSample * (1.0f - currentMix) + loopSample * currentMix;

            float outL, outR;
            if (width < 0.01f)
            {
                float processed = fftProcessorL.processSample (fftInput, false);
                outL = outR = processed;
            }
            else
            {
                float processedL = fftProcessorL.processSample (fftInput, false);
                float processedR = fftProcessorR.processSample (fftInput, false);
                float mid = (processedL + processedR) * 0.5f;
                outL = mid + (processedL - mid) * width;
                outR = mid + (processedR - mid) * width;
            }

            effectsChain.processSample (outL, outR);

            // Process modulation sources (envelope follower uses raw microphone input)
            modulationManager.processSample (liveSample, liveSample);

            // Apply modulation to targets
            float loopMixMod, masterVolumeMod;
            modulationManager.applyModulation (loopRecorder, effectsChain, loopMixMod, masterVolumeMod);

            // Update mix and volume if modulated (-1 means not modulated)
            if (loopMixMod >= 0.0f)
                currentMix = loopMixMod;
            else
                currentMix = mix;

            if (masterVolumeMod >= 0.0f)
                currentVolume = masterVolumeMod * volume;  // Multiply with manual volume
            else
                currentVolume = volume;

            // Apply master volume
            outL *= currentVolume;
            outR *= currentVolume;

            // Feed master output to spectrum visualiser in effect mode
            if (usePiLayout && piLoopDetail && !piLoopMode)
                piLoopDetail->pushVisSample ((outL + outR) * 0.5f);

            outputSum += outL * outL + outR * outR;

            if (numOutputChannels >= 1 && outputChannelData[0] != nullptr)
                outputChannelData[0][i] += outL;
            if (numOutputChannels >= 2 && outputChannelData[1] != nullptr)
                outputChannelData[1][i] += outR;
        }
    }

    outputLevel.store (std::sqrt (outputSum / (float) (numSamples * 2)) * 3.0f);

    const auto endTime = juce::Time::getHighResolutionTicks();
    const double elapsedSeconds = juce::Time::highResolutionTicksToSeconds (endTime - startTime);
    const double availableSeconds = (double) numSamples / currentSampleRate;
    const float load = (float) (elapsedSeconds / availableSeconds * 100.0);
    cpuLoad.store (cpuLoad.load() * 0.9f + load * 0.1f);
}

void MainComponent::timerCallback()
{
    // Re-scan for MIDI devices every ~2 seconds (every 60 timer ticks at 30Hz)
    if (++midiRescanCounter >= 60)
    {
        midiRescanCounter = 0;
        refreshMidiInputs();
    }

    bool learnMode = midiLearnActive.load();
    bool hasSelection = midiLearnHasSelection.load();

    for (int i = 0; i < 8; ++i)
    {
        juce::Colour buttonColour;

        // In MIDI learn mode with this button selected
        if (learnMode && hasSelection && midiLearnSelected.type == MidiMapping::LoopButton && midiLearnSelected.targetIndex == i)
        {
            buttonColour = juce::Colour (0xffff9900);  // Orange for selected
        }
        else if (loopRecorder.isRecording() && loopRecorder.getRecordingSlot() == i)
        {
            static int pulseCounter = 0;
            pulseCounter = (pulseCounter + 1) % 15;
            float pulse = 0.7f + 0.3f * std::sin (pulseCounter * 0.4f);
            buttonColour = juce::Colour::fromFloatRGBA (pulse, 0.1f, 0.1f, 1.0f);
        }
        else if (loopRecorder.isSlotActive (i))
        {
            buttonColour = juce::Colours::green;
        }
        else
        {
            buttonColour = juce::Colour (0xff2a2b31);
        }

        loopButtons[i].setColour (juce::TextButton::buttonColourId, buttonColour);

        // Repaint progress bar for smooth playhead animation
        if (loopProgressBars[i])
            loopProgressBars[i]->repaint();

        // Handle volume slider automation and modulation
        bool hasLevelAutomation = loopRecorder.slotHasLevelAutomation (i);
        bool isAutomationRunning = loopRecorder.isPreviewRunning (i);
        bool volumeModulated = modulationManager.isTargetModulated (ModulationTarget::Type::LoopVolume, i);

        // If slot has level automation or modulation, move slider to show current level
        if (hasLevelAutomation || isAutomationRunning)
        {
            float autoLevel = loopRecorder.getSlotAutomationLevel (i);
            if (std::abs (loopVolumeSliders[i].getValue() - autoLevel) > 0.01)
                loopVolumeSliders[i].setValue (autoLevel, juce::dontSendNotification);
        }
        else if (volumeModulated)
        {
            float modValue = modulationManager.getModulatedValue (ModulationTarget::Type::LoopVolume, i);
            if (modValue >= 0.0f && std::abs (loopVolumeSliders[i].getValue() - modValue) > 0.01)
                loopVolumeSliders[i].setValue (modValue, juce::dontSendNotification);
        }

        // Disable manual control if slot has level automation or modulation
        loopVolumeSliders[i].setEnabled (!hasLevelAutomation && !volumeModulated);

        // Handle HP slider modulation
        bool hpModulated = modulationManager.isTargetModulated (ModulationTarget::Type::LoopFilterHP, i);
        if (hpModulated)
        {
            float modValue = modulationManager.getModulatedValue (ModulationTarget::Type::LoopFilterHP, i);
            if (modValue >= 0.0f && std::abs (loopHPSliders[i].getValue() - modValue) > 1.0)
            {
                loopHPSliders[i].setValue (modValue, juce::dontSendNotification);
                int freq = (int) modValue;
                loopHPLabels[i].setText (juce::String (freq) + " Hz", juce::dontSendNotification);
            }
        }
        loopHPSliders[i].setEnabled (!hpModulated);

        // Handle LP slider modulation
        bool lpModulated = modulationManager.isTargetModulated (ModulationTarget::Type::LoopFilterLP, i);
        if (lpModulated)
        {
            float modValue = modulationManager.getModulatedValue (ModulationTarget::Type::LoopFilterLP, i);
            if (modValue >= 0.0f && std::abs (loopLPSliders[i].getValue() - modValue) > 1.0)
            {
                loopLPSliders[i].setValue (modValue, juce::dontSendNotification);
                int freq = (int) modValue;
                if (freq >= 1000)
                    loopLPLabels[i].setText (juce::String (freq / 1000.0f, 1) + "k", juce::dontSendNotification);
                else
                    loopLPLabels[i].setText (juce::String (freq) + " Hz", juce::dontSendNotification);
            }
        }
        loopLPSliders[i].setEnabled (!lpModulated);

        // Populate indicator data for Pi layout components
        if (usePiLayout)
        {
            LoopIndicatorData ind;
            ind.active = loopRecorder.isSlotActive (i);
            ind.recording = loopRecorder.isRecording() && loopRecorder.getRecordingSlot() == i;
            ind.volume = loopRecorder.getSlotVolume (i);
            ind.hpFreq = loopRecorder.getSlotHighPass (i);
            ind.lpFreq = loopRecorder.getSlotLowPass (i);
            ind.volumeModulated = volumeModulated;
            ind.hpModulated = hpModulated;
            ind.lpModulated = lpModulated;
            ind.hasAutomation = hasLevelAutomation;
            ind.automationLevel = loopRecorder.getSlotAutomationLevel (i);

            if (piLoopButtons[i])
                piLoopButtons[i]->indicatorData = ind;
            if (piMiniMixer)
                piMiniMixer->loopData[(size_t) i] = ind;
            if (piMultiOverview)
                piMultiOverview->loopData[(size_t) i] = ind;
        }
    }

    // Handle Live/Loop mix modulation
    bool loopMixModulated = modulationManager.isTargetModulated (ModulationTarget::Type::LoopMix);
    if (loopMixModulated)
    {
        float modValue = modulationManager.getModulatedValue (ModulationTarget::Type::LoopMix);
        if (modValue >= 0.0f && std::abs (loopMixKnob.getValue() - modValue) > 0.01)
            loopMixKnob.setValue (modValue, juce::dontSendNotification);
    }
    loopMixKnob.setEnabled (!loopMixModulated);

    // Handle Master volume modulation
    bool masterVolumeModulated = modulationManager.isTargetModulated (ModulationTarget::Type::MasterVolume);
    if (masterVolumeModulated)
    {
        float modValue = modulationManager.getModulatedValue (ModulationTarget::Type::MasterVolume);
        if (modValue >= 0.0f && std::abs (masterVolumeKnob.getValue() - modValue) > 0.01)
            masterVolumeKnob.setValue (modValue, juce::dontSendNotification);
    }
    masterVolumeKnob.setEnabled (!masterVolumeModulated);

    // Update effect parameter knobs to reflect modulation
    updateEffectParameterModulation();

    // Update MIDI learn button color
    if (learnMode)
    {
        if (hasSelection)
            midiLearnButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xffff9900));  // Orange when has selection
        else
            midiLearnButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff5dd6c6));  // Accent color when active
    }
    else
    {
        midiLearnButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2a2b31));
    }

    repaint();
}

void MainComponent::handleIncomingMidiMessage (juce::MidiInput* source,
                                                const juce::MidiMessage& message)
{
    juce::ignoreUnused (source);

    std::cerr << "MIDI message received: " << message.getDescription() << std::endl;

    // All UI updates must happen on the message thread
    // Copy the relevant data we need
    const bool isCC = message.isController();
    const bool isNoteOn = message.isNoteOn();
    const bool isNoteOff = message.isNoteOff();
    const int ccNumber = isCC ? message.getControllerNumber() : 0;
    const int ccValue = isCC ? message.getControllerValue() : 0;
    const int noteNumber = (isNoteOn || isNoteOff) ? message.getNoteNumber() : 0;

    // Post to message thread for processing
    juce::MessageManager::callAsync ([this, isCC, isNoteOn, isNoteOff, ccNumber, ccValue, noteNumber]()
    {
        // Handle MIDI learn mode - only if we have a selection
        if (midiLearnActive.load() && midiLearnHasSelection.load())
        {
            if (isCC || isNoteOn)
            {
                processMidiLearn (isCC ? ccNumber : noteNumber, isNoteOn);
            }
            return;
        }

        // Virtual encoder bank — contextual dispatch (takes priority)
        bool handledByEncoder = false;
        if (isCC)
            handledByEncoder = encoderBank.handleCC (ccNumber, ccValue);
        if (isNoteOn && !handledByEncoder)
            handledByEncoder = encoderBank.handleNoteOn (noteNumber);

        // Default loop toggle notes: 44-51 (G#2-D#3) → toggle record/play/stop
        if (isNoteOn && !handledByEncoder)
        {
            int loopSlot = noteNumber - VirtualEncoderBank::loopToggleNoteBase;
            if (loopSlot >= 0 && loopSlot < 8)
            {
                if (usePiLayout)
                    piToggleLoop (loopSlot);
                else
                    loopButtons[loopSlot].triggerClick();
                handledByEncoder = true;
            }
        }

        // Handle CC messages for legacy mapped knobs (skip if encoder handled it)
        if (isCC && !handledByEncoder)
        {
            auto it = midiCCMappings.find (ccNumber);
            if (it != midiCCMappings.end())
            {
                if (it->second.type == MidiMapping::Knob && it->second.knobPtr != nullptr)
                {
                    float normValue = ccValue / 127.0f;
                    auto* knob = it->second.knobPtr;
                    double range = knob->getMaximum() - knob->getMinimum();
                    knob->setValue (knob->getMinimum() + normValue * range);
                }
                else if (it->second.type == MidiMapping::LoopButton)
                {
                    int loopIdx = it->second.targetIndex;
                    if (loopIdx >= 0 && loopIdx < 8 && ccValue > 63)
                    {
                        if (usePiLayout)
                            piToggleLoop (loopIdx);
                        else
                            loopButtons[loopIdx].triggerClick();
                    }
                }
                else if (it->second.type == MidiMapping::Button && it->second.buttonPtr != nullptr)
                {
                    if (ccValue > 63)
                        it->second.buttonPtr->triggerClick();
                }
            }
        }

        // Handle note messages — check encoder push buttons first, then legacy
        if (isNoteOn && !handledByEncoder)
        {
            auto it = midiNoteMappings.find (noteNumber);
            if (it != midiNoteMappings.end())
            {
                if (it->second.type == MidiMapping::LoopButton)
                {
                    int loopIdx = it->second.targetIndex;
                    if (loopIdx >= 0 && loopIdx < 8)
                    {
                        if (usePiLayout)
                            piToggleLoop (loopIdx);
                        else
                            loopButtons[loopIdx].triggerClick();
                    }
                }
                else if (it->second.type == MidiMapping::Button && it->second.buttonPtr != nullptr)
                {
                    it->second.buttonPtr->triggerClick();
                }
            }
        }

        // Handle NoteOff for encoder push button release
        if (isNoteOff)
            encoderBank.handleNoteOff (noteNumber);
    });
}

void MainComponent::refreshMidiInputs()
{
    auto midiInputs = juce::MidiInput::getAvailableDevices();
    for (const auto& input : midiInputs)
    {
        if (knownMidiInputIds.count (input.identifier) == 0)
        {
            std::cerr << "MIDI input connected: " << input.name << " [" << input.identifier << "]" << std::endl;
            deviceManager.setMidiInputDeviceEnabled (input.identifier, true);
            deviceManager.addMidiInputDeviceCallback (input.identifier, this);
            knownMidiInputIds.insert (input.identifier);
        }
    }
}

void MainComponent::toggleMidiLearnMode()
{
    if (midiLearnActive.load())
    {
        exitMidiLearnMode();
    }
    else
    {
        midiLearnActive.store (true);
        midiLearnHasSelection.store (false);
        midiLearnSelected = MidiMapping();
        midiLearnButton.setButtonText ("Select...");
    }
}

void MainComponent::exitMidiLearnMode()
{
    midiLearnActive.store (false);
    midiLearnHasSelection.store (false);
    midiLearnSelected = MidiMapping();
    midiLearnButton.setButtonText ("MIDI Learn");
}

void MainComponent::selectControlForMidiLearn (juce::Slider* knob)
{
    if (!midiLearnActive.load())
        return;

    clearMidiLearnSelection();

    midiLearnSelected.type = MidiMapping::Knob;
    midiLearnSelected.knobPtr = knob;
    midiLearnHasSelection.store (true);
    midiLearnButton.setButtonText ("Waiting...");
}

void MainComponent::selectLoopButtonForMidiLearn (int loopIndex)
{
    if (!midiLearnActive.load())
        return;

    clearMidiLearnSelection();

    midiLearnSelected.type = MidiMapping::LoopButton;
    midiLearnSelected.targetIndex = loopIndex;
    midiLearnHasSelection.store (true);
    midiLearnButton.setButtonText ("Loop " + juce::String (loopIndex + 1));
}

void MainComponent::selectButtonForMidiLearn (juce::Button* button)
{
    if (!midiLearnActive.load())
        return;

    clearMidiLearnSelection();

    midiLearnSelected.type = MidiMapping::Button;
    midiLearnSelected.buttonPtr = button;
    midiLearnHasSelection.store (true);
    midiLearnButton.setButtonText ("Waiting...");
}

void MainComponent::clearMidiLearnSelection()
{
    midiLearnSelected = MidiMapping();
    midiLearnHasSelection.store (false);
}

void MainComponent::processMidiLearn (int ccOrNote, bool isNote)
{
    // This is called on the message thread
    if (!midiLearnHasSelection.load())
        return;

    MidiMapping mapping = midiLearnSelected;

    if (isNote)
    {
        // Notes can map to buttons (loop buttons or other buttons)
        midiNoteMappings[ccOrNote] = mapping;
    }
    else
    {
        // CCs can map to anything
        midiCCMappings[ccOrNote] = mapping;
    }

    // Clear selection but stay in learn mode
    clearMidiLearnSelection();
    midiLearnButton.setButtonText ("Select...");
}

//==============================================================================
// PI LAYOUT
//==============================================================================

void MainComponent::initPiLayout()
{
    // Create touch loop buttons
    for (int i = 0; i < 8; ++i)
    {
        piLoopButtons[i] = std::make_unique<TouchLoopButton> (loopRecorder, i);
        piLoopButtons[i]->onSelect = [this](int slot) {
            // Short tap on a recording slot stops the recording
            if (loopRecorder.isRecording() && loopRecorder.getRecordingSlot() == slot)
                loopRecorder.stopRecording();
            piSelectLoop (slot);
        };
        piLoopButtons[i]->onToggle = [this](int slot) { piToggleLoop (slot); };
        addAndMakeVisible (*piLoopButtons[i]);
    }

    // Create detail strip
    piLoopDetail = std::make_unique<LoopDetailStrip> (loopRecorder);
    addAndMakeVisible (*piLoopDetail);

    // Wire detail strip callbacks
    piLoopDetail->octaveSelector.onChange = [this](int octave) {
        int slot = piLoopDetail->getSelectedLoop();
        loopRecorder.setSlotPitchOctave (slot, octave);
        loopPitchCombos[slot].setSelectedId (octave + 3, juce::dontSendNotification);
    };

    piLoopDetail->autoButton.onClick = [this]() {
        int slot = piLoopDetail->getSelectedLoop();
        auto* editor = new LoopAutomationEditor (loopRecorder, slot);
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned (editor);
        options.dialogTitle = "Loop " + juce::String (slot + 1) + " Automation";
        options.dialogBackgroundColour = juce::Colour (0xff17181d);
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = false;
        options.resizable = false;
        options.launchAsync();
    };

    piLoopDetail->clearButton.onClick = [this]() {
        int slot = piLoopDetail->getSelectedLoop();
        loopRecorder.clearSlot (slot);
    };

    // Override tab buttons for Pi layout
    auto piUpdateTabColors = [this] {
        droneTabButton.setColour (juce::TextButton::buttonColourId,
            activeTab == 0 ? PiColours::accent.withAlpha (0.25f) : PiColours::buttonBg);
        droneTabButton.setColour (juce::TextButton::textColourOffId,
            activeTab == 0 ? PiColours::accentBright : PiColours::textNormal);
        modulationTabButton.setColour (juce::TextButton::buttonColourId,
            activeTab == 2 ? PiColours::accent.withAlpha (0.25f) : PiColours::buttonBg);
        modulationTabButton.setColour (juce::TextButton::textColourOffId,
            activeTab == 2 ? PiColours::accentBright : PiColours::textNormal);
        settingsButton.setColour (juce::TextButton::buttonColourId, PiColours::buttonBg);
        settingsButton.setColour (juce::TextButton::textColourOffId, PiColours::textNormal);
        resourcesButton.setColour (juce::TextButton::buttonColourId, PiColours::buttonBg);
        resourcesButton.setColour (juce::TextButton::textColourOffId, PiColours::textNormal);
    };

    droneTabButton.onClick = [this, piUpdateTabColors] {
        if (activeTab == 0)
        {
            // Already on drone tab — toggle page
            piDroneKnobPage = (piDroneKnobPage + 1) % 2;
            updateDroneParameterKnobs();
            return;
        }
        activeTab = 0;
        piDroneKnobPage = 0;
        piLoopMode = false;
        selectedEffectSlot = -1;
        TouchLoopButton::selectedSlot = -1;
        piUpdateTabColors();
        updateEffectButtonColors();
        updateDroneParameterKnobs();
        if (modulationPanel)
            modulationPanel->setVisible (false);
        // Restore indicator mode layout via resized
        resized();
        for (auto& btn : piLoopButtons)
            if (btn) btn->repaint();
    };

    modulationTabButton.onClick = [this, piUpdateTabColors] {
        activeTab = 2;
        piLoopMode = false;
        selectedEffectSlot = -1;
        TouchLoopButton::selectedSlot = -1;
        piUpdateTabColors();
        updateEffectButtonColors();
        // Hide desktop modulation panel in Pi mode; resized() handles indicator layout
        if (modulationPanel)
            modulationPanel->setVisible (false);
        resized();
        // Populate bottom knobs with modulation encoder controls
        updateModulationParameterKnobs();
        for (auto& btn : piLoopButtons)
            if (btn) btn->repaint();
    };

    piUpdateTabColors();

    // Effect buttons: tap selects effect and enters effect mode
    for (int i = 0; i < 6; ++i)
    {
        effectButtons[i].onClick = [this, i]() {
            piSelectEffect (i);
        };
    }

    // Set Pi-specific button styling
    bypassButton.setButtonText (masterBypass.load() ? "OFF" : "ON");
    bypassButton.setColour (juce::TextButton::buttonColourId,
                             masterBypass.load() ? PiColours::recording.withAlpha (0.3f) : PiColours::buttonBg);
    bypassButton.setColour (juce::TextButton::textColourOffId,
                             masterBypass.load() ? PiColours::recording : PiColours::textNormal);
    midiLearnButton.setColour (juce::TextButton::buttonColourId, PiColours::buttonBg);
    midiLearnButton.setColour (juce::TextButton::textColourOffId, PiColours::textDim);

    // Create mini mixer strip (always visible except on mod tab)
    piMiniMixer = std::make_unique<MiniMixerStrip> (loopRecorder);
    addAndMakeVisible (*piMiniMixer);

    // Create multi-loop overview (visible on mod tab only)
    piMultiOverview = std::make_unique<MultiLoopOverview> (loopRecorder);
    addChildComponent (*piMultiOverview);

    // Dev push buttons — simulate encoder push buttons for testing without MIDI controller
    for (int i = 0; i < 8; ++i)
    {
        piPushButtons[i].setButtonText ("P");
        piPushButtons[i].setColour (juce::TextButton::buttonColourId, PiColours::panelRaised);
        piPushButtons[i].setColour (juce::TextButton::textColourOffId, PiColours::textDim);
        piPushButtons[i].onClick = [this, i] {
            // Trigger the encoder bank's push button for this slot
            encoderBank.handleNoteOn (VirtualEncoderBank::defaultNoteBase + i);
            // Also send NoteOff shortly after (fire-and-forget)
            juce::Timer::callAfterDelay (100, [this, i] {
                encoderBank.handleNoteOff (VirtualEncoderBank::defaultNoteBase + i);
            });
        };
        addAndMakeVisible (piPushButtons[i]);
    }

    // Start in loop mode with loop 0 selected
    piLoopMode = true;
    piSelectLoop (0);
}

void MainComponent::piSelectLoop (int slot)
{
    // Enter loop mode — deselect effects, return to loops view
    activeTab = 1;
    piLoopMode = true;
    selectedEffectSlot = -1;
    updateEffectButtonColors();

    // Restore indicator layout (may have been replaced by mod panel)
    if (modulationPanel)
        modulationPanel->setVisible (false);

    // Unhighlight drone/mod tab buttons
    droneTabButton.setColour (juce::TextButton::buttonColourId, PiColours::buttonBg);
    droneTabButton.setColour (juce::TextButton::textColourOffId, PiColours::textNormal);
    modulationTabButton.setColour (juce::TextButton::buttonColourId, PiColours::buttonBg);
    modulationTabButton.setColour (juce::TextButton::textColourOffId, PiColours::textNormal);

    TouchLoopButton::selectedSlot = slot;
    piLoopDetail->setSelectedLoop (slot);
    piSyncDetailToLoop (slot);
    updateLoopParameterKnobs();
    resized();

    for (auto& btn : piLoopButtons)
        if (btn) btn->repaint();
}

void MainComponent::piSelectEffect (int slot)
{
    // Enter effect mode — deselect loops, return to loops view
    activeTab = 1;
    piLoopMode = false;
    TouchLoopButton::selectedSlot = -1;
    selectedEffectSlot = slot;

    // Restore indicator layout
    if (modulationPanel)
        modulationPanel->setVisible (false);

    droneTabButton.setColour (juce::TextButton::buttonColourId, PiColours::buttonBg);
    droneTabButton.setColour (juce::TextButton::textColourOffId, PiColours::textNormal);
    modulationTabButton.setColour (juce::TextButton::buttonColourId, PiColours::buttonBg);
    modulationTabButton.setColour (juce::TextButton::textColourOffId, PiColours::textNormal);

    updateEffectButtonColors();
    updateEffectParameterKnobs();
    resized();

    for (auto& btn : piLoopButtons)
        if (btn) btn->repaint();
}

void MainComponent::piToggleLoop (int slot)
{
    // Same logic as the desktop loop button onClick
    if (midiLearnActive.load())
    {
        selectLoopButtonForMidiLearn (slot);
        return;
    }

    if (loopRecorder.isRecording() && loopRecorder.getRecordingSlot() == slot)
        loopRecorder.stopRecording();
    else if (loopRecorder.isSlotActive (slot))
        loopRecorder.clearSlot (slot);
    else
        loopRecorder.startRecording (slot);

    // Also select this loop for detail view
    piSelectLoop (slot);
}

void MainComponent::updateDroneParameterKnobs()
{
    // Hide all existing param controls
    for (int i = 0; i < maxParamKnobs; ++i)
    {
        paramKnobs[i].setVisible (false);
        paramKnobs[i].onValueChange = nullptr;
        paramKnobs[i].textFromValueFunction = nullptr;
        paramLabels[i].setVisible (false);
    }
    for (int i = 0; i < 4; ++i)
    {
        paramCombos[i].setVisible (false);
        paramCombos[i].onChange = nullptr;
        paramComboLabels[i].setVisible (false);
        paramToggles[i].setVisible (false);
        paramToggles[i].onClick = nullptr;
    }
    numActiveKnobs = 0;
    numActiveCombos = 0;
    numActiveToggles = 0;
    encoderBank.clearAll();

    auto addKnob = [this](const juce::String& name, juce::Slider& source) {
        if (numActiveKnobs >= maxParamKnobs) return;
        int knobIndex = numActiveKnobs;
        auto& knob = paramKnobs[knobIndex];
        auto& label = paramLabels[knobIndex];

        knob.setRange (source.getRange().getStart(), source.getRange().getEnd(), source.getInterval());
        knob.setValue (source.getValue(), juce::dontSendNotification);
        knob.setTextValueSuffix (source.getTextValueSuffix());
        knob.onValueChange = [&knob, &source] {
            source.setValue (knob.getValue(), juce::sendNotificationSync);
        };
        knob.setVisible (true);

        label.setText (name, juce::dontSendNotification);
        label.setVisible (true);

        // Bind to virtual encoder bank
        if (knobIndex < VirtualEncoderBank::numEncoders)
        {
            encoderBank.bindEncoder (knobIndex, name,
                source.getRange().getStart(), source.getRange().getEnd(), source.getInterval(),
                [this, knobIndex, &source](float v) {
                    paramKnobs[knobIndex].setValue (v, juce::dontSendNotification);
                    source.setValue (v, juce::sendNotificationSync);
                },
                [this, knobIndex]() -> float {
                    return (float) paramKnobs[knobIndex].getValue();
                },
                source.getTextValueSuffix());
        }

        numActiveKnobs++;
    };

    auto addToggleAsKnob = [this](const juce::String& name, juce::ToggleButton& source) {
        if (numActiveKnobs >= maxParamKnobs) return;
        int knobIndex = numActiveKnobs;
        auto& knob = paramKnobs[knobIndex];
        auto& label = paramLabels[knobIndex];

        knob.setRange (0, 1, 1);
        knob.setValue (source.getToggleState() ? 1.0 : 0.0, juce::dontSendNotification);
        knob.setTextValueSuffix ("");
        knob.onValueChange = [&knob, &source] {
            source.setToggleState (knob.getValue() >= 0.5, juce::sendNotificationSync);
        };
        knob.setVisible (true);

        label.setText (name, juce::dontSendNotification);
        label.setVisible (true);

        // Bind to virtual encoder bank
        if (knobIndex < VirtualEncoderBank::numEncoders)
        {
            encoderBank.bindEncoder (knobIndex, name, 0, 1, 1,
                [this, knobIndex, &source](float v) {
                    paramKnobs[knobIndex].setValue (v, juce::dontSendNotification);
                    source.setToggleState (v >= 0.5f, juce::sendNotificationSync);
                },
                [this, knobIndex]() -> float {
                    return (float) paramKnobs[knobIndex].getValue();
                });
        }

        numActiveKnobs++;
    };

    if (piDroneKnobPage == 0)
    {
        addKnob ("Dry/Wet", dryWetKnob);
        addKnob ("Smooth", smoothingKnob);
        addKnob ("Thresh", thresholdKnob);
        addKnob ("Tilt", tiltKnob);
        addKnob ("Delay", delayKnob);
        addKnob ("Decay", decayKnob);
        addKnob ("History", historyKnob);
        addKnob ("Width", stereoWidthKnob);
    }
    else
    {
        addKnob ("Pitch", pitchKnob);
        addKnob ("Octave", octaveKnob);
        addToggleAsKnob ("Peak", peakToggle);
        addToggleAsKnob ("Phase", phaseToggle);
        addKnob ("Live/Loop", loopMixKnob);
        addKnob ("Volume", masterVolumeKnob);
    }

    resized();
}

void MainComponent::piSyncDetailToLoop (int slot)
{
    piLoopDetail->octaveSelector.setSelectedOctave (loopPitchCombos[slot].getSelectedId() - 3);
}

// Apply an automation preset with time scaling to a sequence
static Sequence applyPresetWithTimeScale (int presetIdx, float timeScale)
{
    presetIdx = juce::jlimit (0, AutomationPresets::numPresets - 1, presetIdx);
    Sequence seq = AutomationPresets::presets[presetIdx].build();

    // Scale all durations
    if (std::abs (timeScale - 1.0f) > 0.001f)
    {
        for (auto& cmd : seq.commands)
        {
            if (auto* w = std::get_if<Wait> (&cmd))
                w->durationSeconds *= timeScale;
            else if (auto* r = std::get_if<RampLevel> (&cmd))
                r->durationSeconds *= timeScale;
        }
    }
    return seq;
}

void MainComponent::updateLoopParameterKnobs()
{
    // Hide all existing param controls
    for (int i = 0; i < maxParamKnobs; ++i)
    {
        paramKnobs[i].setVisible (false);
        paramKnobs[i].onValueChange = nullptr;
        paramKnobs[i].textFromValueFunction = nullptr;
        paramLabels[i].setVisible (false);
    }
    for (int i = 0; i < 4; ++i)
    {
        paramCombos[i].setVisible (false);
        paramCombos[i].onChange = nullptr;
        paramCombos[i].clear();
        paramComboLabels[i].setVisible (false);
        paramToggles[i].setVisible (false);
        paramToggles[i].onClick = nullptr;
    }
    numActiveKnobs = 0;
    numActiveCombos = 0;
    numActiveToggles = 0;
    encoderBank.clearAll();

    int slot = TouchLoopButton::selectedSlot;
    if (slot < 0 || slot >= 8) return;

    auto addKnob = [this](const juce::String& name, double min, double max, double step,
                          double value, std::function<void(float)> onChange, const juce::String& suffix = "") {
        if (numActiveKnobs >= maxParamKnobs) return;
        int knobIndex = numActiveKnobs;
        auto& knob = paramKnobs[knobIndex];
        auto& label = paramLabels[knobIndex];

        knob.setRange (min, max, step);
        knob.setValue (value, juce::dontSendNotification);
        knob.setTextValueSuffix (suffix);
        knob.onValueChange = [&knob, onChange] { onChange ((float) knob.getValue()); };
        knob.setVisible (true);

        label.setText (name, juce::dontSendNotification);
        label.setVisible (true);

        // Bind to virtual encoder bank for MIDI CC control
        if (knobIndex < VirtualEncoderBank::numEncoders)
        {
            encoderBank.bindEncoder (knobIndex, name, min, max, step,
                [this, knobIndex, onChange](float v) {
                    paramKnobs[knobIndex].setValue (v, juce::dontSendNotification);
                    onChange (v);
                },
                [this, knobIndex]() -> float {
                    return (float) paramKnobs[knobIndex].getValue();
                },
                suffix);
        }

        numActiveKnobs++;
    };

    // Knob 1: Loop selector (1-8)
    addKnob ("Loop", 1, 8, 1, slot + 1,
             [this](float v) {
                 int newSlot = juce::jlimit (0, 7, (int) v - 1);
                 if (newSlot != TouchLoopButton::selectedSlot)
                     piSelectLoop (newSlot);
             });

    // Knob 2: Trim start
    addKnob ("Start", 0, 1, 0.001, loopRecorder.getSlotTrimStart (slot),
             [this](float v) {
                 int s = TouchLoopButton::selectedSlot;
                 if (s >= 0) loopRecorder.setSlotTrimStart (s, v);
             });

    // Knob 3: Trim end
    addKnob ("End", 0, 1, 0.001, loopRecorder.getSlotTrimEnd (slot),
             [this](float v) {
                 int s = TouchLoopButton::selectedSlot;
                 if (s >= 0) loopRecorder.setSlotTrimEnd (s, v);
             });

    // Knob 4: Automation preset selector
    {
        auto settings = loopRecorder.getSlotSettings (slot);
        int autoKnobIdx = numActiveKnobs;
        addKnob ("Auto", 0, AutomationPresets::numPresets - 1, 1, settings.presetIndex,
                 [this](float v) {
                     int s = TouchLoopButton::selectedSlot;
                     if (s < 0) return;
                     int presetIdx = juce::jlimit (0, AutomationPresets::numPresets - 1, (int) v);
                     auto slotSettings = loopRecorder.getSlotSettings (s);
                     slotSettings.presetIndex = presetIdx;
                     slotSettings.postRecordSequence = applyPresetWithTimeScale (presetIdx, slotSettings.timeScale);
                     loopRecorder.setSlotSettings (s, slotSettings);
                 });
        paramKnobs[autoKnobIdx].textFromValueFunction = [](double v) {
            int idx = juce::jlimit (0, AutomationPresets::numPresets - 1, (int) v);
            return juce::String (AutomationPresets::presets[idx].name);
        };
        paramKnobs[autoKnobIdx].updateText();
    }

    // Knob 5: Time scale (0.1x to 10.0x)
    {
        auto settings = loopRecorder.getSlotSettings (slot);
        addKnob ("Time", 0.1, 10.0, 0.1, settings.timeScale,
                 [this](float v) {
                     int s = TouchLoopButton::selectedSlot;
                     if (s < 0) return;
                     auto slotSettings = loopRecorder.getSlotSettings (s);
                     slotSettings.timeScale = v;
                     slotSettings.postRecordSequence = applyPresetWithTimeScale (slotSettings.presetIndex, v);
                     loopRecorder.setSlotSettings (s, slotSettings);
                 }, "x");
    }

    // Knob 6: High pass (log-scaled: 0-1 normalized, mapped to 20-5000 Hz)
    {
        const double hpMin = 20.0, hpMax = 5000.0;
        double currentHP = loopHPSliders[slot].getValue();
        // Convert current Hz to normalized 0-1 log position
        double hpNorm = (std::log (currentHP) - std::log (hpMin)) / (std::log (hpMax) - std::log (hpMin));
        hpNorm = juce::jlimit (0.0, 1.0, hpNorm);

        addKnob ("HP", 0, 1, 0.005, hpNorm,
                 [this](float v) {
                     int s = TouchLoopButton::selectedSlot;
                     if (s >= 0) {
                         float hz = 20.0f * std::pow (5000.0f / 20.0f, v);
                         loopRecorder.setSlotHighPass (s, hz);
                         loopHPSliders[s].setValue (hz, juce::dontSendNotification);
                     }
                 });
        // Show Hz in the text box below the knob
        paramKnobs[numActiveKnobs - 1].textFromValueFunction = [](double v) {
            float hz = 20.0f * std::pow (5000.0f / 20.0f, (float) v);
            int freq = (int) hz;
            if (freq >= 1000) return juce::String (freq / 1000.0f, 1) + "k";
            return juce::String (freq) + " Hz";
        };
        paramKnobs[numActiveKnobs - 1].updateText();
    }

    // Knob 7: Low pass (log-scaled: 0-1 normalized, mapped to 200-20000 Hz)
    {
        const double lpMin = 200.0, lpMax = 20000.0;
        double currentLP = loopLPSliders[slot].getValue();
        // Convert current Hz to normalized 0-1 log position
        double lpNorm = (std::log (currentLP) - std::log (lpMin)) / (std::log (lpMax) - std::log (lpMin));
        lpNorm = juce::jlimit (0.0, 1.0, lpNorm);

        addKnob ("LP", 0, 1, 0.005, lpNorm,
                 [this](float v) {
                     int s = TouchLoopButton::selectedSlot;
                     if (s >= 0) {
                         float hz = 200.0f * std::pow (20000.0f / 200.0f, v);
                         loopRecorder.setSlotLowPass (s, hz);
                         loopLPSliders[s].setValue (hz, juce::dontSendNotification);
                     }
                 });
        // Show Hz in the text box below the knob
        paramKnobs[numActiveKnobs - 1].textFromValueFunction = [](double v) {
            float hz = 200.0f * std::pow (20000.0f / 200.0f, (float) v);
            int freq = (int) hz;
            if (freq >= 1000) return juce::String (freq / 1000.0f, 1) + "k";
            return juce::String (freq) + " Hz";
        };
        paramKnobs[numActiveKnobs - 1].updateText();
    }

    // Knob 8: Volume
    addKnob ("Vol", 0, 2, 0.01, loopVolumeSliders[slot].getValue(),
             [this](float v) {
                 int s = TouchLoopButton::selectedSlot;
                 if (s >= 0) {
                     loopRecorder.setSlotVolume (s, v);
                     loopVolumeSliders[s].setValue (v, juce::dontSendNotification);
                 }
             });

    // Push button 4: Preview automation on selected loop
    encoderBank.bindButton (3, [this] {
        int s = TouchLoopButton::selectedSlot;
        if (s < 0) return;
        if (loopRecorder.isPreviewRunning (s))
            loopRecorder.stopPreview (s);
        else
            loopRecorder.startPreview (s);
    });

    // Push button 5: (reserved for future use)

    // Grey out automation knobs (4 & 5) if this loop's volume is being modulated
    bool volumeModulated = modulationManager.isTargetModulated (ModulationTarget::Type::LoopVolume, slot);
    if (volumeModulated)
    {
        paramKnobs[3].setEnabled (false);  // Automation preset
        paramKnobs[4].setEnabled (false);  // Time scale
    }

    // Repurpose volume knob as automation range modifier when automation is active
    bool hasLevelAutomation = loopRecorder.slotHasLevelAutomation (slot);
    if (hasLevelAutomation)
    {
        auto& knob = paramKnobs[7];
        auto& label = paramLabels[7];

        knob.setRange (0, 1, 0.01);
        knob.setValue (0.5, juce::dontSendNotification);
        knob.setTextValueSuffix ("");
        knob.textFromValueFunction = [](double v) {
            if (std::abs (v - 0.5) < 0.01) return juce::String ("--");
            return juce::String ((v - 0.5) * 200.0, 0) + "%";
        };
        knob.updateText();
        knob.setEnabled (true);

        label.setText ("Vol Rng", juce::dontSendNotification);

        knob.onValueChange = [this, slot] {
            float v = (float) paramKnobs[7].getValue();
            if (v < 0.49f)
            {
                // Below center: reduce max. At 0, max = min (fully squashed)
                float t = v / 0.5f;  // 0 to 1
                loopRecorder.setSlotAutomationRange (slot, 0.0f, t);
            }
            else if (v > 0.51f)
            {
                // Above center: increase min. At 1, min = max (fully squashed at top)
                float t = (v - 0.5f) / 0.5f;  // 0 to 1
                loopRecorder.setSlotAutomationRange (slot, t, 1.0f);
            }
            else
            {
                // Center: full range
                loopRecorder.setSlotAutomationRange (slot, 0.0f, 1.0f);
            }
        };

        if (7 < VirtualEncoderBank::numEncoders)
        {
            encoderBank.bindEncoder (7, "Vol Rng", 0, 1, 0.01,
                [this](float v) {
                    paramKnobs[7].setValue (v, juce::dontSendNotification);
                    paramKnobs[7].onValueChange();
                },
                [this]() -> float {
                    return (float) paramKnobs[7].getValue();
                });
        }
    }

    // Repurpose modulated parameter knobs as range modifiers
    // Center (0.5) = no impact, below = reduce max, above = increase min
    auto setupModRangeKnob = [this, slot](int knobIndex, ModulationTarget::Type targetType, const juce::String& name) {
        auto& knob = paramKnobs[knobIndex];
        auto& label = paramLabels[knobIndex];

        // Find the current modulation offset (check if we already adjusted)
        float defMin, defMax;
        ModulationTarget::getDefaultRange (targetType, defMin, defMax);

        knob.setRange (0, 1, 0.01);
        knob.setValue (0.5, juce::dontSendNotification);
        knob.setTextValueSuffix ("");
        knob.textFromValueFunction = [](double v) {
            if (std::abs (v - 0.5) < 0.01) return juce::String ("--");
            return juce::String ((v - 0.5) * 200.0, 0) + "%";
        };
        knob.updateText();
        knob.setEnabled (true);

        label.setText (name + " Mod", juce::dontSendNotification);

        knob.onValueChange = [this, slot, targetType, knobIndex, defMin, defMax] {
            float v = (float) paramKnobs[knobIndex].getValue();
            // Find the modulation source targeting this parameter on this slot
            for (int si = 0; si < modulationManager.getNumSources(); ++si)
            {
                auto* src = modulationManager.getSource (si);
                if (!src) continue;
                for (int ti = 0; ti < 3; ++ti)
                {
                    auto& tgt = src->getTarget (ti);
                    if (tgt.type == targetType && tgt.index == slot)
                    {
                        if (v < 0.49f)
                        {
                            // Below center: reduce max. At 0, max = min (fully squashed)
                            float t = v / 0.5f;  // 0 to 1
                            tgt.rangeMax = defMin + t * (defMax - defMin);
                            tgt.rangeMin = defMin;
                        }
                        else if (v > 0.51f)
                        {
                            // Above center: increase min. At 1, min = max (fully squashed)
                            float t = (v - 0.5f) / 0.5f;  // 0 to 1
                            tgt.rangeMin = defMin + t * (defMax - defMin);
                            tgt.rangeMax = defMax;
                        }
                        else
                        {
                            // Center: full range
                            tgt.rangeMin = defMin;
                            tgt.rangeMax = defMax;
                        }
                    }
                }
            }
        };

        if (knobIndex < VirtualEncoderBank::numEncoders)
        {
            encoderBank.bindEncoder (knobIndex, name + " Mod", 0, 1, 0.01,
                [this, knobIndex](float v) {
                    paramKnobs[knobIndex].setValue (v, juce::dontSendNotification);
                    paramKnobs[knobIndex].onValueChange();
                },
                [this, knobIndex]() -> float {
                    return (float) paramKnobs[knobIndex].getValue();
                });
        }
    };

    if (volumeModulated)
        setupModRangeKnob (7, ModulationTarget::Type::LoopVolume, "Vol");

    bool hpModulated = modulationManager.isTargetModulated (ModulationTarget::Type::LoopFilterHP, slot);
    if (hpModulated)
        setupModRangeKnob (5, ModulationTarget::Type::LoopFilterHP, "HP");

    bool lpModulated = modulationManager.isTargetModulated (ModulationTarget::Type::LoopFilterLP, slot);
    if (lpModulated)
        setupModRangeKnob (6, ModulationTarget::Type::LoopFilterLP, "LP");

    resized();
}

// ======================================================================
// Modulation target parent/child lookup tables
// ======================================================================
namespace ModTargetLookup
{
    // Parent categories for encoder 5
    struct ParentInfo { juce::String name; };
    static const ParentInfo parents[] = {
        { "None" },        // 0
        { "Loop 1" },      // 1
        { "Loop 2" },      // 2
        { "Loop 3" },      // 3
        { "Loop 4" },      // 4
        { "Loop 5" },      // 5
        { "Loop 6" },      // 6
        { "Loop 7" },      // 7
        { "Loop 8" },      // 8
        { "Delay" },       // 9
        { "Distort" },     // 10
        { "Tape" },        // 11
        { "Filter" },      // 12
        { "Tremolo" },     // 13
        { "Master" },      // 14
    };
    static constexpr int numParents = 15;

    struct ChildInfo { juce::String name; ModulationTarget::Type type; };

    // Children for each parent
    static const ChildInfo loopChildren[] = {
        { "Volume", ModulationTarget::Type::LoopVolume },
        { "HP",     ModulationTarget::Type::LoopFilterHP },
        { "LP",     ModulationTarget::Type::LoopFilterLP },
    };
    static const ChildInfo delayChildren[] = {
        { "Time",     ModulationTarget::Type::DelayTime },
        { "Feedback", ModulationTarget::Type::DelayFeedback },
        { "Dry/Wet",  ModulationTarget::Type::DelayDryWet },
    };
    static const ChildInfo distChildren[] = {
        { "Drive",   ModulationTarget::Type::DistortionDrive },
        { "Tone",    ModulationTarget::Type::DistortionTone },
        { "Dry/Wet", ModulationTarget::Type::DistortionDryWet },
    };
    static const ChildInfo tapeChildren[] = {
        { "Satur.",  ModulationTarget::Type::TapeSaturation },
        { "Bias",    ModulationTarget::Type::TapeBias },
        { "Wow",     ModulationTarget::Type::TapeWowDepth },
        { "Flutter", ModulationTarget::Type::TapeFlutterDepth },
        { "Dry/Wet", ModulationTarget::Type::TapeDryWet },
    };
    static const ChildInfo filterChildren[] = {
        { "HP",       ModulationTarget::Type::FilterHP },
        { "LP",       ModulationTarget::Type::FilterLP },
        { "Harmonic", ModulationTarget::Type::FilterHarmonicIntensity },
    };
    static const ChildInfo tremoloChildren[] = {
        { "Rate",  ModulationTarget::Type::TremoloRate },
        { "Depth", ModulationTarget::Type::TremoloDepth },
    };
    static const ChildInfo masterChildren[] = {
        { "Volume",   ModulationTarget::Type::MasterVolume },
        { "Loop Mix", ModulationTarget::Type::LoopMix },
    };

    static void getChildren (int parentIndex, const ChildInfo*& out, int& count)
    {
        if (parentIndex >= 1 && parentIndex <= 8)
        { out = loopChildren; count = 3; }
        else if (parentIndex == 9)
        { out = delayChildren; count = 3; }
        else if (parentIndex == 10)
        { out = distChildren; count = 3; }
        else if (parentIndex == 11)
        { out = tapeChildren; count = 5; }
        else if (parentIndex == 12)
        { out = filterChildren; count = 3; }
        else if (parentIndex == 13)
        { out = tremoloChildren; count = 2; }
        else if (parentIndex == 14)
        { out = masterChildren; count = 2; }
        else
        { out = nullptr; count = 0; }
    }

    // Reverse-lookup: given a ModulationTarget, find the parent and child indices
    static void findIndices (const ModulationTarget& t, int& parentOut, int& childOut)
    {
        parentOut = 0;
        childOut = 0;
        if (t.type == ModulationTarget::Type::None)
            return;

        // Check loop types
        if (t.type == ModulationTarget::Type::LoopVolume ||
            t.type == ModulationTarget::Type::LoopFilterHP ||
            t.type == ModulationTarget::Type::LoopFilterLP)
        {
            parentOut = juce::jlimit (1, 8, t.index + 1);
            for (int i = 0; i < 3; ++i)
                if (loopChildren[i].type == t.type) { childOut = i; break; }
            return;
        }

        // Check each effect group
        struct Group { int parent; const ChildInfo* children; int count; };
        const Group groups[] = {
            { 9,  delayChildren,   3 },
            { 10, distChildren,    3 },
            { 11, tapeChildren,    5 },
            { 12, filterChildren,  3 },
            { 13, tremoloChildren, 2 },
            { 14, masterChildren,  2 },
        };
        for (auto& g : groups)
        {
            for (int i = 0; i < g.count; ++i)
            {
                if (g.children[i].type == t.type)
                {
                    parentOut = g.parent;
                    childOut = i;
                    return;
                }
            }
        }
    }
}

void MainComponent::updateModulationParameterKnobs()
{
    // Hide all existing param controls
    for (int i = 0; i < maxParamKnobs; ++i)
    {
        paramKnobs[i].setVisible (false);
        paramKnobs[i].onValueChange = nullptr;
        paramKnobs[i].textFromValueFunction = nullptr;
        paramLabels[i].setVisible (false);
    }
    for (int i = 0; i < 4; ++i)
    {
        paramCombos[i].setVisible (false);
        paramCombos[i].onChange = nullptr;
        paramCombos[i].clear();
        paramComboLabels[i].setVisible (false);
        paramToggles[i].setVisible (false);
        paramToggles[i].onClick = nullptr;
    }
    numActiveKnobs = 0;
    numActiveCombos = 0;
    numActiveToggles = 0;
    encoderBank.clearAll();

    // Clamp state
    piModSourceIndex = juce::jlimit (0, modulationManager.getNumSources() - 1, piModSourceIndex);
    piModTargetSlot  = juce::jlimit (0, 2, piModTargetSlot);

    auto* source = modulationManager.getSource (piModSourceIndex);
    if (source == nullptr) return;

    // Sync piModParentIndex / piModChildIndex from current target
    auto& target = source->getTarget (piModTargetSlot);
    ModTargetLookup::findIndices (target, piModParentIndex, piModChildIndex);

    // Get children for current parent, filtering out Volume if loop has automation
    const ModTargetLookup::ChildInfo* rawChildren = nullptr;
    int rawNumChildren = 0;
    ModTargetLookup::getChildren (piModParentIndex, rawChildren, rawNumChildren);

    // For loop parents, filter out Volume if that loop has level automation
    static ModTargetLookup::ChildInfo filteredChildren[8];
    const ModTargetLookup::ChildInfo* children = rawChildren;
    int numChildren = rawNumChildren;

    if (piModParentIndex >= 1 && piModParentIndex <= 8)
    {
        int loopIdx = piModParentIndex - 1;
        bool loopHasAutomation = loopRecorder.slotHasLevelAutomation (loopIdx);
        if (loopHasAutomation)
        {
            int filtered = 0;
            for (int i = 0; i < rawNumChildren; ++i)
            {
                if (rawChildren[i].type != ModulationTarget::Type::LoopVolume)
                    filteredChildren[filtered++] = rawChildren[i];
            }
            children = filteredChildren;
            numChildren = filtered;
        }
    }

    piModChildIndex = (numChildren > 0) ? juce::jlimit (0, numChildren - 1, piModChildIndex) : 0;

    // Helper: determine source type
    auto* lfo   = dynamic_cast<LFOSource*> (source);
    auto* env   = dynamic_cast<EnvelopeFollowerSource*> (source);
    auto* rnd   = dynamic_cast<RandomizerSource*> (source);

    auto addKnob = [this](const juce::String& name, double min, double max, double step,
                          double value, std::function<void(float)> onChange, const juce::String& suffix = "") {
        if (numActiveKnobs >= maxParamKnobs) return;
        int knobIndex = numActiveKnobs;
        auto& knob = paramKnobs[knobIndex];
        auto& label = paramLabels[knobIndex];

        knob.setRange (min, max, step);
        knob.setValue (value, juce::dontSendNotification);
        knob.setTextValueSuffix (suffix);
        knob.onValueChange = [&knob, onChange] { onChange ((float) knob.getValue()); };
        knob.setVisible (true);

        label.setText (name, juce::dontSendNotification);
        label.setVisible (true);

        if (knobIndex < VirtualEncoderBank::numEncoders)
        {
            encoderBank.bindEncoder (knobIndex, name, min, max, step,
                [this, knobIndex, onChange](float v) {
                    paramKnobs[knobIndex].setValue (v, juce::dontSendNotification);
                    onChange (v);
                },
                [this, knobIndex]() -> float {
                    return (float) paramKnobs[knobIndex].getValue();
                },
                suffix);
        }

        numActiveKnobs++;
    };

    // --- Source name labels for encoder 1 ---
    juce::StringArray sourceNames;
    for (int i = 0; i < modulationManager.getNumSources(); ++i)
    {
        if (i < ModulationManager::numLFOs)
            sourceNames.add ("LFO " + juce::String (i + 1));
        else if (i == ModulationManager::numLFOs)
            sourceNames.add ("Env Flw");
        else
            sourceNames.add ("Rand " + juce::String (i - ModulationManager::numLFOs));
    }

    // ===== ENCODER 1: Modulator selector =====
    {
        int srcKnobIdx = numActiveKnobs;
        addKnob ("Source", 0, modulationManager.getNumSources() - 1, 1,
                 piModSourceIndex,
                 [this, sourceNames](float v) {
                     int newIdx = juce::jlimit (0, modulationManager.getNumSources() - 1, (int) v);
                     if (newIdx != piModSourceIndex)
                     {
                         piModSourceIndex = newIdx;
                         updateModulationParameterKnobs();
                     }
                 });
        paramKnobs[srcKnobIdx].textFromValueFunction = [sourceNames](double v) {
            int idx = juce::jlimit (0, sourceNames.size() - 1, (int) v);
            return sourceNames[idx];
        };
        paramKnobs[srcKnobIdx].updateText();
    }

    // ===== ENCODER 2: Param 1 (Rate / Attack) =====
    if (lfo)
    {
        {
            // Log-scaled rate: 0-1 normalized, mapped to 0.001-3000 Hz
            const double rateMin = 0.001, rateMax = 3000.0;
            double currentRate = lfo->getRate();
            double rateNorm = (std::log (juce::jmax (rateMin, currentRate)) - std::log (rateMin))
                              / (std::log (rateMax) - std::log (rateMin));
            rateNorm = juce::jlimit (0.0, 1.0, rateNorm);
            int rateKnobIdx = numActiveKnobs;
            addKnob ("Rate", 0, 1, 0.002, rateNorm,
                     [this](float v) {
                         float hz = 0.001f * std::pow (3000.0f / 0.001f, v);
                         auto* l = dynamic_cast<LFOSource*> (modulationManager.getSource (piModSourceIndex));
                         if (l) l->setRate (hz);
                     });
            paramKnobs[rateKnobIdx].textFromValueFunction = [](double v) {
                float hz = 0.001f * std::pow (3000.0f / 0.001f, (float) v);
                if (hz >= 1000.0f)
                    return juce::String (hz / 1000.0f, 1) + "kHz";
                else if (hz >= 1.0f)
                    return juce::String (hz, 1) + "Hz";
                else
                    return juce::String (hz, 3) + "Hz";
            };
            paramKnobs[rateKnobIdx].updateText();
        }
    }
    else if (env)
    {
        addKnob ("Attack", 0.1, 500, 0.1, env->getAttackMs(),
                 [this](float v) {
                     modulationManager.getEnvelopeFollower().setAttackMs (v);
                 }, " ms");
    }
    else if (rnd)
    {
        {
            const double rateMin = 0.001, rateMax = 3000.0;
            double currentRate = rnd->getRate();
            double rateNorm = (std::log (juce::jmax (rateMin, currentRate)) - std::log (rateMin))
                              / (std::log (rateMax) - std::log (rateMin));
            rateNorm = juce::jlimit (0.0, 1.0, rateNorm);
            int rateKnobIdx = numActiveKnobs;
            addKnob ("Rate", 0, 1, 0.002, rateNorm,
                     [this](float v) {
                         float hz = 0.001f * std::pow (3000.0f / 0.001f, v);
                         int ri = piModSourceIndex - ModulationManager::numLFOs - ModulationManager::numEnvelopes;
                         if (ri >= 0 && ri < ModulationManager::numRandomizers)
                             modulationManager.getRandomizer (ri).setRate (hz);
                     });
            paramKnobs[rateKnobIdx].textFromValueFunction = [](double v) {
                float hz = 0.001f * std::pow (3000.0f / 0.001f, (float) v);
                if (hz >= 1000.0f)
                    return juce::String (hz / 1000.0f, 1) + "kHz";
                else if (hz >= 1.0f)
                    return juce::String (hz, 1) + "Hz";
                else
                    return juce::String (hz, 3) + "Hz";
            };
            paramKnobs[rateKnobIdx].updateText();
        }
    }

    // ===== ENCODER 3: Param 2 (Waveform / Release / Smoothness) =====
    if (lfo)
    {
        {
            int shapeIdx = (int) lfo->getWaveform();
            int knobIdx = numActiveKnobs;
            addKnob ("Shape", 0, 5, 1, shapeIdx,
                     [this](float v) {
                         int idx = juce::jlimit (0, 5, (int) v);
                         auto* l = dynamic_cast<LFOSource*> (modulationManager.getSource (piModSourceIndex));
                         if (l) l->setWaveform (static_cast<LFOSource::Waveform> (idx));
                     });
            paramKnobs[knobIdx].textFromValueFunction = [](double v) {
                static const char* names[] = { "Sine", "Tri", "Square", "SawUp", "SawDn", "S&H" };
                int idx = juce::jlimit (0, 5, (int) v);
                return juce::String (names[idx]);
            };
            paramKnobs[knobIdx].updateText();
        }
    }
    else if (env)
    {
        addKnob ("Release", 1, 2000, 1, env->getReleaseMs(),
                 [this](float v) {
                     modulationManager.getEnvelopeFollower().setReleaseMs (v);
                 }, " ms");
    }
    else if (rnd)
    {
        addKnob ("Smooth", 0, 1, 0.01, rnd->getSmoothness(),
                 [this](float v) {
                     int ri = piModSourceIndex - ModulationManager::numLFOs - ModulationManager::numEnvelopes;
                     if (ri >= 0 && ri < ModulationManager::numRandomizers)
                         modulationManager.getRandomizer (ri).setSmoothness (v);
                 });
    }

    // ===== ENCODER 4: Target slot selector (1/2/3) =====
    addKnob ("Slot", 1, 3, 1, piModTargetSlot + 1,
             [this](float v) {
                 int newSlot = juce::jlimit (0, 2, (int) v - 1);
                 if (newSlot != piModTargetSlot)
                 {
                     piModTargetSlot = newSlot;
                     updateModulationParameterKnobs();
                 }
             });

    // ===== ENCODER 5: Target parent =====
    {
        int parentKnobIdx = numActiveKnobs;
        addKnob ("Target", 0, ModTargetLookup::numParents - 1, 1, piModParentIndex,
                 [this](float v) {
                     int newParent = juce::jlimit (0, ModTargetLookup::numParents - 1, (int) v);
                     if (newParent != piModParentIndex)
                     {
                         piModParentIndex = newParent;
                         piModChildIndex = 0;
                         // Apply target change
                         auto* src = modulationManager.getSource (piModSourceIndex);
                         if (src)
                         {
                             if (piModParentIndex == 0)
                             {
                                 // None
                                 ModulationTarget t;
                                 src->setTarget (piModTargetSlot, t);
                             }
                             else
                             {
                                 const ModTargetLookup::ChildInfo* ch = nullptr;
                                 int nc = 0;
                                 ModTargetLookup::getChildren (piModParentIndex, ch, nc);
                                 if (nc > 0)
                                 {
                                     ModulationTarget t;
                                     t.type = ch[0].type;
                                     t.index = (piModParentIndex >= 1 && piModParentIndex <= 8) ? piModParentIndex - 1 : 0;
                                     ModulationTarget::getDefaultRange (t.type, t.rangeMin, t.rangeMax);
                                     src->setTarget (piModTargetSlot, t);
                                 }
                             }
                         }
                         updateModulationParameterKnobs();
                     }
                 });
        paramKnobs[parentKnobIdx].textFromValueFunction = [](double v) {
            int idx = juce::jlimit (0, ModTargetLookup::numParents - 1, (int) v);
            return juce::String (ModTargetLookup::parents[idx].name);
        };
        paramKnobs[parentKnobIdx].updateText();
    }

    // ===== ENCODER 6: Target child =====
    if (numChildren > 0)
    {
        int childKnobIdx = numActiveKnobs;
        addKnob ("Param", 0, numChildren - 1, 1, piModChildIndex,
                 [this, children, numChildren](float v) {
                     int newChild = juce::jlimit (0, numChildren - 1, (int) v);
                     if (newChild != piModChildIndex)
                     {
                         piModChildIndex = newChild;
                         auto* src = modulationManager.getSource (piModSourceIndex);
                         if (src)
                         {
                             ModulationTarget t;
                             t.type = children[newChild].type;
                             t.index = (piModParentIndex >= 1 && piModParentIndex <= 8) ? piModParentIndex - 1 : 0;
                             ModulationTarget::getDefaultRange (t.type, t.rangeMin, t.rangeMax);
                             // Preserve existing min/max if the type hasn't changed
                             auto& existing = src->getTarget (piModTargetSlot);
                             if (existing.type == t.type && existing.index == t.index)
                             {
                                 t.rangeMin = existing.rangeMin;
                                 t.rangeMax = existing.rangeMax;
                             }
                             src->setTarget (piModTargetSlot, t);
                         }
                         updateModulationParameterKnobs();
                     }
                 });
        paramKnobs[childKnobIdx].textFromValueFunction = [children, numChildren](double v) {
            int idx = juce::jlimit (0, numChildren - 1, (int) v);
            return juce::String (children[idx].name);
        };
        paramKnobs[childKnobIdx].updateText();
    }
    else
    {
        addKnob ("\xe2\x80\x94", 0, 1, 0.01, 0, [](float) {});
    }

    // ===== ENCODER 7: Min modulation range =====
    if (target.type != ModulationTarget::Type::None)
    {
        float defMin, defMax;
        ModulationTarget::getDefaultRange (target.type, defMin, defMax);
        addKnob ("Min", defMin, defMax, (defMax - defMin) / 200.0, target.rangeMin,
                 [this](float v) {
                     auto* src = modulationManager.getSource (piModSourceIndex);
                     if (src) src->getTarget (piModTargetSlot).rangeMin = v;
                 });
    }
    else
    {
        addKnob ("\xe2\x80\x94", 0, 1, 0.01, 0, [](float) {});
    }

    // ===== ENCODER 8: Max modulation range =====
    if (target.type != ModulationTarget::Type::None)
    {
        float defMin, defMax;
        ModulationTarget::getDefaultRange (target.type, defMin, defMax);
        addKnob ("Max", defMin, defMax, (defMax - defMin) / 200.0, target.rangeMax,
                 [this](float v) {
                     auto* src = modulationManager.getSource (piModSourceIndex);
                     if (src) src->getTarget (piModTargetSlot).rangeMax = v;
                 });
    }
    else
    {
        addKnob ("\xe2\x80\x94", 0, 1, 0.01, 0, [](float) {});
    }

    // Bind push buttons
    // Push 1: Toggle modulator enabled/disabled
    encoderBank.bindButton (0, [this] {
        auto* src = modulationManager.getSource (piModSourceIndex);
        if (src) src->setEnabled (!src->isEnabled());
    });
    // Push 2: (EnvFollow) toggle to sensitivity on encoder 2
    // For now, no-op for other types — can be extended later
    // Push 4: Clear current target slot
    encoderBank.bindButton (3, [this] {
        auto* src = modulationManager.getSource (piModSourceIndex);
        if (src)
        {
            ModulationTarget t;
            src->setTarget (piModTargetSlot, t);
            updateModulationParameterKnobs();
        }
    });
    // Push 7: Reset min to default
    encoderBank.bindButton (6, [this] {
        auto* src = modulationManager.getSource (piModSourceIndex);
        if (src)
        {
            auto& t = src->getTarget (piModTargetSlot);
            if (t.type != ModulationTarget::Type::None)
            {
                float defMin, defMax;
                ModulationTarget::getDefaultRange (t.type, defMin, defMax);
                t.rangeMin = defMin;
                updateModulationParameterKnobs();
            }
        }
    });
    // Push 8: Reset max to default
    encoderBank.bindButton (7, [this] {
        auto* src = modulationManager.getSource (piModSourceIndex);
        if (src)
        {
            auto& t = src->getTarget (piModTargetSlot);
            if (t.type != ModulationTarget::Type::None)
            {
                float defMin, defMax;
                ModulationTarget::getDefaultRange (t.type, defMin, defMax);
                t.rangeMax = defMax;
                updateModulationParameterKnobs();
            }
        }
    });

    resized();
}

void MainComponent::paintPi (juce::Graphics& g)
{
    g.fillAll (PiColours::bg);

    const float W = (float) getWidth();

    // Header background with subtle gradient
    {
        auto headerRect = juce::Rectangle<float> (0, 0, W, 34.0f);
        g.setColour (juce::Colour (0xff181c24));
        g.fillRect (headerRect);
        g.setColour (PiColours::panelBorder.withAlpha (0.5f));
        g.drawLine (0, 34, W, 34, 1.0f);
    }

    // Input / Output meters
    {
        const int meterX = 8;
        int meterY = 7;
        const int meterW = 90;
        const int meterH = 8;

        auto drawMeter = [&](const juce::String& label, float value, juce::Colour colour)
        {
            auto bgRect = juce::Rectangle<float> ((float) (meterX + 22), (float) meterY,
                                                   (float) meterW, (float) meterH);
            g.setColour (juce::Colour (0xff0a0c10));
            g.fillRoundedRectangle (bgRect, 3.0f);
            g.setColour (PiColours::panelBorder.withAlpha (0.3f));
            g.drawRoundedRectangle (bgRect, 3.0f, 0.5f);

            float v = juce::jlimit (0.0f, 1.0f, value);
            if (v > 0.001f)
            {
                auto fillRect = bgRect.withWidth (bgRect.getWidth() * v);
                g.setColour (colour.withAlpha (0.8f));
                g.fillRoundedRectangle (fillRect, 3.0f);
            }

            g.setColour (PiColours::textDim);
            g.setFont (PiColours::make (9.0f, true));
            g.drawText (label, meterX, meterY - 1, 20, meterH + 2, juce::Justification::centredLeft);

            meterY += meterH + 4;
        };

        drawMeter ("IN",  inputLevel.load(),  PiColours::accent);
        drawMeter ("OUT", outputLevel.load(), juce::Colour (0xff6699dd));
    }

    // Effects + params combined background (anchored to bottom)
    auto bottomBg = juce::Rectangle<float> (4, (float) piEffectsRowY,
                                             W - 8, (float) (getHeight() - piEffectsRowY - 2));
    g.setColour (PiColours::panelBg);
    g.fillRoundedRectangle (bottomBg, 10.0f);
    g.setColour (PiColours::panelBorder.withAlpha (0.4f));
    g.drawRoundedRectangle (bottomBg.reduced (0.5f), 10.0f, 1.0f);
}

void MainComponent::resizedPi()
{
    // Fixed layout heights — total = 600px
    const int headerH    = 34;
    const int loopBtnH   = 78;
    const int effectsH   = 40;
    const int paramH     = 110;
    const int gap        = 3;
    // Detail strip fills the space between loop buttons and effects row

    auto bounds = getLocalBounds();

    // === Header ===
    auto header = bounds.removeFromTop (headerH);
    header.removeFromLeft (120);  // Space for meters drawn in paintPi

    // Right corner: small bypass + MIDI learn
    auto cornerRight = header.removeFromRight (90);
    midiLearnButton.setVisible (true);
    midiLearnButton.setButtonText ("M");
    midiLearnButton.setBounds (cornerRight.removeFromRight (32).reduced (2, 5));
    cornerRight.removeFromRight (2);
    bypassButton.setVisible (true);
    bypassButton.setBounds (cornerRight.reduced (2, 5));
    piIndicatorModeButton.setVisible (false);

    header.removeFromRight (6);

    // Main header buttons: Drone, Mod, Settings, Resources — evenly spaced
    loopsTabButton.setVisible (false);
    int mainBtnW = header.getWidth() / 4;
    droneTabButton.setVisible (true);
    droneTabButton.setBounds (header.removeFromLeft (mainBtnW).reduced (3, 5));
    modulationTabButton.setVisible (true);
    modulationTabButton.setBounds (header.removeFromLeft (mainBtnW).reduced (3, 5));
    settingsButton.setBounds (header.removeFromLeft (mainBtnW).reduced (3, 5));
    resourcesButton.setBounds (header.reduced (3, 5));

    masterVolumeKnob.setVisible (false);
    masterVolumeLabel.setVisible (false);
    loopMixKnob.setVisible (false);
    loopMixLabel.setVisible (false);

    bounds.removeFromTop (gap);

    // === Loop buttons ===
    auto loopRow = bounds.removeFromTop (loopBtnH).reduced (4, 0);
    const int loopBtnWidth = loopRow.getWidth() / 8;
    for (int i = 0; i < 8; ++i)
        piLoopButtons[i]->setBounds (loopRow.removeFromLeft (loopBtnWidth).reduced (2, 1));

    bounds.removeFromTop (gap);

    // === Effects + params anchored to bottom ===
    auto bottomBounds = getLocalBounds();
    auto paramArea = bottomBounds.removeFromBottom (paramH).reduced (10, 4);
    bottomBounds.removeFromBottom (gap);
    auto effectsArea = bottomBounds.removeFromBottom (effectsH).reduced (10, 3);

    piEffectsRowY = effectsArea.getY() - 2;
    piEffectsRowH = effectsH + gap + paramH + 8;

    const int effectWidth = effectsArea.getWidth() / 6;
    for (int i = 0; i < 6; ++i)
    {
        effectButtons[i].setVisible (true);
        effectButtons[i].setBounds (effectsArea.removeFromLeft (effectWidth).reduced (3, 1));
    }

    // === Detail strip / indicator layout ===
    int detailTop = bounds.getY();
    int detailBottom = effectsArea.getY() - gap;
    auto detailArea = juce::Rectangle<int> (4, detailTop, getWidth() - 8, detailBottom - detailTop);

    if (activeTab == 2)
    {
        // Mod tab: show multi-loop overview, hide mini mixer and detail strip
        piMiniMixer->setVisible (false);
        piLoopDetail->setVisible (false);
        piMultiOverview->setVisible (true);
        piMultiOverview->setBounds (detailArea);
    }
    else
    {
        // All other tabs: mini mixer strip on top, detail strip below
        const int mixerH = 14;
        auto mixerArea = juce::Rectangle<int> (detailArea.getX(), detailArea.getY(),
                                                detailArea.getWidth(), mixerH);
        piMiniMixer->setVisible (true);
        piMiniMixer->setBounds (mixerArea);

        auto remainingDetail = juce::Rectangle<int> (detailArea.getX(),
                                                      detailArea.getY() + mixerH + gap,
                                                      detailArea.getWidth(),
                                                      detailArea.getHeight() - mixerH - gap);
        piLoopDetail->setVisible (true);
        piLoopDetail->setBounds (remainingDetail);
        piMultiOverview->setVisible (false);
    }

    // === Parameter knobs: 8 slots, all rotary ===
    const int numSlots = 8;
    const int slotWidth = paramArea.getWidth() / numSlots;
    const int knobSize = juce::jmin (slotWidth - 8, paramArea.getHeight() - 18);
    const int labelHeight = 13;

    for (int i = 0; i < maxParamKnobs; ++i)
    {
        if (i < numActiveKnobs)
        {
            int slotX = paramArea.getX() + i * slotWidth + (slotWidth - knobSize) / 2;
            paramLabels[i].setBounds (slotX, paramArea.getY(), knobSize, labelHeight);
            paramKnobs[i].setBounds (slotX, paramArea.getY() + labelHeight,
                                     knobSize, knobSize);
        }
    }

    // Dev push buttons — positioned to the right of each knob
    for (int i = 0; i < 8; ++i)
    {
        if (i < numActiveKnobs)
        {
            int slotX = paramArea.getX() + i * slotWidth + (slotWidth - knobSize) / 2;
            int btnX = slotX + knobSize - 2;
            int btnY = paramArea.getY() + labelHeight;
            piPushButtons[i].setBounds (btnX, btnY, 16, 16);
            piPushButtons[i].setVisible (true);
        }
        else
        {
            piPushButtons[i].setVisible (false);
        }
    }

    // Combos only visible on desktop layout
    if (!usePiLayout)
    {
        for (int i = 0; i < numActiveCombos && (numActiveKnobs + i) < numSlots; ++i)
        {
            int slot = numActiveKnobs + i;
            int slotX = paramArea.getX() + slot * slotWidth + (slotWidth - 80) / 2;
            paramComboLabels[i].setBounds (slotX, paramArea.getY(), 80, labelHeight);
            paramCombos[i].setBounds (slotX, paramArea.getY() + labelHeight + 8, 80, 26);
        }
    }

    for (int i = 0; i < numActiveToggles && (numActiveKnobs + numActiveCombos + i) < numSlots; ++i)
    {
        int slot = numActiveKnobs + numActiveCombos + i;
        int slotX = paramArea.getX() + slot * slotWidth + (slotWidth - 85) / 2;
        paramToggles[i].setBounds (slotX, paramArea.getY() + labelHeight + 8, 85, 26);
    }

    // === Hide desktop-only elements ===
    moveLeftButton.setVisible (false);
    moveRightButton.setVisible (false);
    clearLoopsButton.setVisible (false);

    for (int i = 0; i < 8; ++i)
    {
        loopButtons[i].setVisible (false);
        loopAutoButtons[i].setVisible (false);
        loopPitchCombos[i].setVisible (false);
        loopPitchLabels[i].setVisible (false);
        loopHPSliders[i].setVisible (false);
        loopHPLabels[i].setVisible (false);
        loopLPSliders[i].setVisible (false);
        loopLPLabels[i].setVisible (false);
        loopVolumeSliders[i].setVisible (false);
        loopVolumeLabels[i].setVisible (false);
        if (loopProgressBars[i])
            loopProgressBars[i]->setVisible (false);
    }

    dryWetKnob.setVisible (false);
    smoothingKnob.setVisible (false);
    thresholdKnob.setVisible (false);
    tiltKnob.setVisible (false);
    delayKnob.setVisible (false);
    decayKnob.setVisible (false);
    historyKnob.setVisible (false);
    stereoWidthKnob.setVisible (false);
    peakToggle.setVisible (false);
    phaseToggle.setVisible (false);
    pitchKnob.setVisible (false);
    octaveKnob.setVisible (false);

    if (modulationPanel)
        modulationPanel->setVisible (false);
}
