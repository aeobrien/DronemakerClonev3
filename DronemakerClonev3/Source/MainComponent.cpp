#include "MainComponent.h"
#include "Effects/FilterEffect.h"
#include "Effects/DelayEffect.h"
#include "Effects/GranularEffect.h"
#include "Effects/TremoloEffect.h"
#include "Effects/DistortionEffect.h"
#include "Effects/TapeEffect.h"

MainComponent::MainComponent()
{
    // Initialize audio device
    auto err = deviceManager.initialise (1, 2, nullptr, true);
    if (err.isNotEmpty())
        DBG ("AudioDeviceManager init error: " + err);

    // Set default buffer size to 2048
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup (setup);
    setup.bufferSize = 2048;
    deviceManager.setAudioDeviceSetup (setup, true);

    deviceManager.addAudioCallback (this);

    // Settings button
    settingsButton.onClick = [this] {
        auto* dialog = new SettingsDialog (deviceManager);
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned (dialog);
        options.dialogTitle = "Audio Settings";
        options.dialogBackgroundColour = juce::Colours::darkgrey;
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

    // ===== DRONE SECTION =====
    setupKnob (dryWetKnob, dryWetLabel, "Dry/Wet", 0.0, 1.0, 0.01, 0.2);
    dryWetKnob.onValueChange = [this] {
        float v = (float) dryWetKnob.getValue();
        fftProcessorL.setInterpFactor (v);
        fftProcessorR.setInterpFactor (v);
    };

    setupKnob (smoothingKnob, smoothingLabel, "Smooth", 0.01, 0.5, 0.01, 0.05);
    smoothingKnob.onValueChange = [this] {
        float v = (float) smoothingKnob.getValue();
        fftProcessorL.setSmoothingFactor (v);
        fftProcessorR.setSmoothingFactor (v);
    };

    setupKnob (thresholdKnob, thresholdLabel, "Thresh", 0.0, 0.01, 0.0001, 0.001);
    thresholdKnob.onValueChange = [this] {
        float v = (float) thresholdKnob.getValue();
        fftProcessorL.setThreshold (v);
        fftProcessorR.setThreshold (v);
    };

    setupKnob (tiltKnob, tiltLabel, "Tilt", -6.0, 6.0, 0.1, 0.0, " dB/oct");
    tiltKnob.onValueChange = [this] {
        float v = (float) tiltKnob.getValue();
        fftProcessorL.setSpectralTilt (v);
        fftProcessorR.setSpectralTilt (v);
    };

    setupKnob (delayKnob, delayLabel, "Delay", 0.0, 1.0, 0.01, 0.0);
    delayKnob.onValueChange = [this] {
        float v = (float) delayKnob.getValue();
        fftProcessorL.setDroneDelay (v);
        fftProcessorR.setDroneDelay (v);
    };

    setupKnob (decayKnob, decayLabel, "Decay", 0.99, 1.0, 0.0001, 1.0);
    decayKnob.onValueChange = [this] {
        float v = (float) decayKnob.getValue();
        fftProcessorL.setDecayRate (v);
        fftProcessorR.setDecayRate (v);
    };

    setupKnob (historyKnob, historyLabel, "History", 0.5, 10.0, 0.1, 4.5, " s");
    historyKnob.onValueChange = [this] {
        float v = (float) historyKnob.getValue();
        fftProcessorL.setHistorySeconds (v);
        fftProcessorR.setHistorySeconds (v);
    };

    setupKnob (stereoWidthKnob, stereoWidthLabel, "Stereo", 0.0, 1.0, 0.01, 1.0);
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
    setupKnob (pitchKnob, pitchLabel, "Semitones", -24.0, 24.0, 0.1, 0.0, " st");
    pitchKnob.onValueChange = [this] {
        float v = (float) pitchKnob.getValue();
        fftProcessorL.setPitchShiftSemitones (v);
        fftProcessorR.setPitchShiftSemitones (v);
    };

    setupKnob (octaveKnob, octaveLabel, "Octaves", -4.0, 4.0, 1.0, 0.0, " oct");
    octaveKnob.onValueChange = [this] {
        float v = (float) octaveKnob.getValue();
        fftProcessorL.setPitchShiftOctaves (v);
        fftProcessorR.setPitchShiftOctaves (v);
    };

    // ===== LOOPS SECTION =====
    setupKnob (liveLevelKnob, liveLevelLabel, "Live", 0.0, 1.0, 0.01, 1.0);
    liveLevelKnob.onValueChange = [this] {
        liveLevel.store ((float) liveLevelKnob.getValue());
    };

    setupKnob (loopLevelKnob, loopLevelLabel, "Loop", 0.0, 1.0, 0.01, 0.5);
    loopLevelKnob.onValueChange = [this] {
        loopLevel.store ((float) loopLevelKnob.getValue());
    };

    // Per-loop controls
    for (int i = 0; i < 4; ++i)
    {
        // Volume slider (rotary knob)
        loopVolumeSliders[i].setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        loopVolumeSliders[i].setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        loopVolumeSliders[i].setRange (0.0, 1.0, 0.01);
        loopVolumeSliders[i].setValue (1.0);
        loopVolumeSliders[i].onValueChange = [this, i] {
            loopRecorder.setSlotVolume (i, (float) loopVolumeSliders[i].getValue());
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
        addAndMakeVisible (loopLPSliders[i]);

        loopLPLabels[i].setText ("20k", juce::dontSendNotification);
        loopLPLabels[i].setJustificationType (juce::Justification::centred);
        loopLPLabels[i].setFont (juce::Font (9.0f));
        loopLPLabels[i].setColour (juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible (loopLPLabels[i]);

        // Pitch selector
        loopPitchCombos[i].addItem ("-1 oct", 1);
        loopPitchCombos[i].addItem ("Normal", 2);
        loopPitchCombos[i].addItem ("+1 oct", 3);
        loopPitchCombos[i].setSelectedId (2);
        loopPitchCombos[i].onChange = [this, i] {
            int octave = loopPitchCombos[i].getSelectedId() - 2;
            loopRecorder.setSlotPitchOctave (i, octave);
        };
        addAndMakeVisible (loopPitchCombos[i]);

        loopPitchLabels[i].setText ("Pitch", juce::dontSendNotification);
        loopPitchLabels[i].setJustificationType (juce::Justification::centred);
        loopPitchLabels[i].setFont (juce::Font (9.0f));
        loopPitchLabels[i].setColour (juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible (loopPitchLabels[i]);

        // Loop button
        loopButtons[i].setButtonText ("Loop " + juce::String (i + 1));
        loopButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colours::darkgrey);
        loopButtons[i].onClick = [this, i] {
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
        effectButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colours::darkgrey);
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
    setSize (1000, 600);

    // Initialize with Filter selected
    updateEffectButtonColors();
    updateEffectParameterKnobs();
}

MainComponent::~MainComponent()
{
    deviceManager.removeAudioCallback (this);
}

void MainComponent::setupKnob (juce::Slider& knob, juce::Label& label, const juce::String& name,
                                double min, double max, double step, double initial,
                                const juce::String& suffix)
{
    knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 55, 15);
    knob.setRange (min, max, step);
    knob.setValue (initial);
    if (suffix.isNotEmpty())
        knob.setTextValueSuffix (suffix);
    addAndMakeVisible (knob);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (11.0f));
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
            effectButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colours::dodgerblue);
        else if (enabled)
            effectButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colours::green.darker());
        else
            effectButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colours::darkgrey);
    }
}

void MainComponent::updateEffectParameterKnobs()
{
    // Hide all existing param controls
    for (int i = 0; i < maxParamKnobs; ++i)
    {
        paramKnobs[i].setVisible (false);
        paramKnobs[i].onValueChange = nullptr;
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

    if (selectedEffectSlot < 0)
        return;

    auto order = effectsChain.getOrder();
    int effectType = order[selectedEffectSlot];

    auto addKnob = [this](const juce::String& name, double min, double max, double step,
                          double value, std::function<void(float)> onChange, const juce::String& suffix = "") {
        if (numActiveKnobs >= maxParamKnobs) return;
        auto& knob = paramKnobs[numActiveKnobs];
        auto& label = paramLabels[numActiveKnobs];

        knob.setRange (min, max, step);
        knob.setValue (value, juce::dontSendNotification);
        knob.setTextValueSuffix (suffix);
        knob.onValueChange = [&knob, onChange] { onChange ((float) knob.getValue()); };
        knob.setVisible (true);

        label.setText (name, juce::dontSendNotification);
        label.setVisible (true);

        numActiveKnobs++;
    };

    auto addCombo = [this](const juce::String& name, juce::StringArray items, int selectedIdx,
                           std::function<void(int)> onChange) {
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

    auto addToggle = [this](const juce::String& name, bool value, std::function<void(bool)> onChange) {
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
                addToggle ("Harmonic On", fx->isHarmonicEnabled(),
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
                addToggle ("Ping-Pong", fx->isPingPong(),
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

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a2e));

    auto drawSection = [&](juce::Rectangle<int> area, const juce::String& title) {
        g.setColour (juce::Colour (0xff16213e));
        g.fillRoundedRectangle (area.toFloat(), 8.0f);
        g.setColour (juce::Colour (0xff0f3460));
        g.drawRoundedRectangle (area.toFloat(), 8.0f, 1.5f);

        g.setColour (juce::Colours::white);
        g.setFont (juce::Font (14.0f).boldened());
        g.drawText (title, area.removeFromTop (25), juce::Justification::centred);
    };

    auto bounds = getLocalBounds().reduced (10);
    bounds.removeFromTop (40);  // Header area

    // Effect parameters section at bottom
    auto paramArea = bounds.removeFromBottom (100).reduced (5);
    drawSection (paramArea, "EFFECT PARAMETERS");

    bounds.removeFromBottom (5);

    // Effects chain row
    auto effectsArea = bounds.removeFromBottom (55).reduced (5);
    drawSection (effectsArea, "EFFECTS CHAIN");

    bounds.removeFromBottom (5);

    // Two main columns
    int columnWidth = (bounds.getWidth() - 10) / 2;

    // Drone & Pitch section (left column)
    drawSection (bounds.removeFromLeft (columnWidth).reduced (5), "DRONE & PITCH");
    bounds.removeFromLeft (10);

    // Loops section (right column)
    drawSection (bounds.reduced (5), "LOOPS");

    // Input/Output meters
    int meterY = 10;
    int meterHeight = 12;

    g.setColour (juce::Colours::grey.darker());
    g.fillRoundedRectangle (15.0f, (float)meterY, 150.0f, (float)meterHeight, 3.0f);
    g.setColour (juce::Colours::limegreen);
    const float inLevel = juce::jlimit (0.0f, 1.0f, inputLevel.load());
    g.fillRoundedRectangle (15.0f, (float)meterY, 150.0f * inLevel, (float)meterHeight, 3.0f);
    g.setColour (juce::Colours::white);
    g.setFont (10.0f);
    g.drawText ("IN", 15, meterY, 150, meterHeight, juce::Justification::centred);

    meterY += meterHeight + 3;
    g.setColour (juce::Colours::grey.darker());
    g.fillRoundedRectangle (15.0f, (float)meterY, 150.0f, (float)meterHeight, 3.0f);
    g.setColour (juce::Colours::cyan);
    const float outLevel = juce::jlimit (0.0f, 1.0f, outputLevel.load());
    g.fillRoundedRectangle (15.0f, (float)meterY, 150.0f * outLevel, (float)meterHeight, 3.0f);
    g.setColour (juce::Colours::white);
    g.drawText ("OUT", 15, meterY, 150, meterHeight, juce::Justification::centred);
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced (10);

    // Header row
    auto header = bounds.removeFromTop (35);
    header.removeFromLeft (170);  // Space for meters
    resourcesButton.setBounds (header.removeFromRight (80));
    header.removeFromRight (5);
    settingsButton.setBounds (header.removeFromRight (70));

    bounds.removeFromTop (5);

    // Effect parameters section at bottom
    auto paramArea = bounds.removeFromBottom (100).reduced (5);
    paramArea.removeFromTop (25);  // Title space

    // Layout parameter knobs horizontally
    const int paramKnobSize = 55;
    const int paramLabelHeight = 14;
    int totalParamWidth = numActiveKnobs * (paramKnobSize + 5) + numActiveCombos * 70 + numActiveToggles * 80;
    int paramStartX = paramArea.getX() + (paramArea.getWidth() - totalParamWidth) / 2;

    int paramX = paramStartX;
    for (int i = 0; i < numActiveKnobs; ++i)
    {
        paramLabels[i].setBounds (paramX, paramArea.getY(), paramKnobSize, paramLabelHeight);
        paramKnobs[i].setBounds (paramX, paramArea.getY() + paramLabelHeight, paramKnobSize, paramArea.getHeight() - paramLabelHeight - 5);
        paramX += paramKnobSize + 5;
    }

    for (int i = 0; i < numActiveCombos; ++i)
    {
        paramComboLabels[i].setBounds (paramX, paramArea.getY(), 65, paramLabelHeight);
        paramCombos[i].setBounds (paramX, paramArea.getY() + paramLabelHeight + 20, 65, 22);
        paramX += 70;
    }

    for (int i = 0; i < numActiveToggles; ++i)
    {
        paramToggles[i].setBounds (paramX, paramArea.getY() + paramLabelHeight + 20, 75, 22);
        paramX += 80;
    }

    bounds.removeFromBottom (5);

    // Effects chain row
    auto effectsArea = bounds.removeFromBottom (55).reduced (5);
    effectsArea.removeFromTop (25);  // Title space

    // Move buttons on left - same size
    const int moveButtonSize = 25;
    moveLeftButton.setBounds (effectsArea.removeFromLeft (moveButtonSize).reduced (2));
    effectsArea.removeFromLeft (3);
    moveRightButton.setBounds (effectsArea.removeFromLeft (moveButtonSize).reduced (2));
    effectsArea.removeFromLeft (10);

    // Effect buttons
    const int effectWidth = (effectsArea.getWidth() - 10) / 6;
    for (int i = 0; i < 6; ++i)
    {
        effectButtons[i].setBounds (effectsArea.removeFromLeft (effectWidth).reduced (2));
    }

    bounds.removeFromBottom (5);

    // Two main columns
    int columnWidth = (bounds.getWidth() - 10) / 2;
    const int knobSize = 55;
    const int labelHeight = 14;

    auto layoutKnobWithLabel = [&](juce::Slider& knob, juce::Label& label, juce::Rectangle<int>& area) {
        auto knobArea = area.removeFromLeft (knobSize + 4);
        label.setBounds (knobArea.removeFromTop (labelHeight));
        knob.setBounds (knobArea.reduced (2));
    };

    // ===== DRONE & PITCH SECTION (left column) =====
    auto droneArea = bounds.removeFromLeft (columnWidth).reduced (5);
    droneArea.removeFromTop (25);  // Title space

    auto droneRow1 = droneArea.removeFromTop (knobSize + labelHeight + 5);
    layoutKnobWithLabel (dryWetKnob, dryWetLabel, droneRow1);
    layoutKnobWithLabel (smoothingKnob, smoothingLabel, droneRow1);
    layoutKnobWithLabel (thresholdKnob, thresholdLabel, droneRow1);
    layoutKnobWithLabel (tiltKnob, tiltLabel, droneRow1);

    auto droneRow2 = droneArea.removeFromTop (knobSize + labelHeight + 5);
    layoutKnobWithLabel (delayKnob, delayLabel, droneRow2);
    layoutKnobWithLabel (decayKnob, decayLabel, droneRow2);
    layoutKnobWithLabel (historyKnob, historyLabel, droneRow2);
    layoutKnobWithLabel (stereoWidthKnob, stereoWidthLabel, droneRow2);

    droneArea.removeFromTop (3);
    peakToggle.setBounds (droneArea.removeFromTop (18).removeFromLeft (100));
    droneArea.removeFromTop (2);
    phaseToggle.setBounds (droneArea.removeFromTop (18).removeFromLeft (120));

    // Pitch controls below drone
    droneArea.removeFromTop (5);
    auto pitchRow = droneArea.removeFromTop (knobSize + labelHeight + 5);
    layoutKnobWithLabel (pitchKnob, pitchLabel, pitchRow);
    layoutKnobWithLabel (octaveKnob, octaveLabel, pitchRow);

    bounds.removeFromLeft (10);

    // ===== LOOPS SECTION (right column) =====
    auto loopsArea = bounds.reduced (5);
    loopsArea.removeFromTop (25);  // Title space

    // Live/Loop level knobs centered at top
    auto loopsRow1 = loopsArea.removeFromTop (knobSize + labelHeight + 5);
    int mixKnobsWidth = (knobSize + 4) * 2;
    loopsRow1.removeFromLeft ((loopsRow1.getWidth() - mixKnobsWidth) / 2);
    layoutKnobWithLabel (liveLevelKnob, liveLevelLabel, loopsRow1);
    layoutKnobWithLabel (loopLevelKnob, loopLevelLabel, loopsRow1);

    loopsArea.removeFromTop (10);

    // Per-loop controls - 4 horizontal strips
    const int loopStripWidth = loopsArea.getWidth() / 4;
    const int smallKnob = 40;

    for (int i = 0; i < 4; ++i)
    {
        auto strip = loopsArea.removeFromLeft (loopStripWidth).reduced (5, 0);
        int stripTop = strip.getY();

        // Loop button at top
        loopButtons[i].setBounds (strip.removeFromTop (28).reduced (2, 0));
        strip.removeFromTop (5);

        // Volume knob
        loopVolumeLabels[i].setBounds (strip.removeFromTop (12));
        loopVolumeSliders[i].setBounds (strip.removeFromTop (smallKnob).reduced (4, 0));
        strip.removeFromTop (2);

        // HP knob
        loopHPLabels[i].setBounds (strip.removeFromTop (12));
        loopHPSliders[i].setBounds (strip.removeFromTop (smallKnob).reduced (4, 0));
        strip.removeFromTop (2);

        // LP knob
        loopLPLabels[i].setBounds (strip.removeFromTop (12));
        loopLPSliders[i].setBounds (strip.removeFromTop (smallKnob).reduced (4, 0));
        strip.removeFromTop (2);

        // Pitch selector
        loopPitchLabels[i].setBounds (strip.removeFromTop (12));
        loopPitchCombos[i].setBounds (strip.removeFromTop (20).reduced (2, 0));
    }

    // Clear button centered at bottom
    auto clearArea = loopsArea.removeFromBottom (30);
    clearLoopsButton.setBounds (clearArea.withSizeKeepingCentre (80, 25));
}

void MainComponent::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    const int blockSize = device->getCurrentBufferSizeSamples();
    monoBuffer.setSize (1, blockSize, false, false, true);
    monoBuffer.clear();

    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup (setup);
    setup.inputChannels.setBit (0);
    deviceManager.setAudioDeviceSetup (setup, true);

    currentSampleRate = device->getCurrentSampleRate();
    currentBufferSize = blockSize;

    float sr = static_cast<float>(currentSampleRate);
    fftProcessorL.setSampleRate (sr);
    fftProcessorL.reset();
    fftProcessorR.setSampleRate (sr);
    fftProcessorR.reset();

    loopRecorder.prepareToPlay (currentSampleRate, blockSize);
    effectsChain.prepareToPlay (currentSampleRate, blockSize);
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
    const float liveLvl = liveLevel.load();
    const float loopLvl = loopLevel.load();

    if (activeInputs > 0 || hasLoops)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float fftInput = 0.0f;

            if (activeInputs > 0)
            {
                float liveSample = mono[i];
                if (isRecording)
                    loopRecorder.recordSample (liveSample);
                fftInput += liveSample * liveLvl;
            }

            if (hasLoops)
                fftInput += loopRecorder.getLoopMix() * loopLvl;

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
    for (int i = 0; i < 4; ++i)
    {
        juce::Colour buttonColour;

        if (loopRecorder.isRecording() && loopRecorder.getRecordingSlot() == i)
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
            buttonColour = juce::Colours::darkgrey;
        }

        loopButtons[i].setColour (juce::TextButton::buttonColourId, buttonColour);
    }

    repaint();
}
