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

    // Test button
    testButton.onClick = [this] {
        auto* dev = deviceManager.getCurrentAudioDevice();
        const double sr = (dev != nullptr ? dev->getCurrentSampleRate() : 48000.0);
        testPhase = 0.0;
        testSamplesRemaining = (int) (0.25 * sr);
        testToneActive.store (true);
    };
    addAndMakeVisible (testButton);

    // Buffer size selector
    // Note: Sizes above 2048 can cause issues with FFT overlap-add (hopSize is 8192)
    bufferSizeCombo.addItem ("256", 256);
    bufferSizeCombo.addItem ("512", 512);
    bufferSizeCombo.addItem ("1024", 1024);
    bufferSizeCombo.addItem ("2048", 2048);
    bufferSizeCombo.setSelectedId (2048);
    bufferSizeCombo.onChange = [this] {
        int bufSize = bufferSizeCombo.getSelectedId();
        juce::AudioDeviceManager::AudioDeviceSetup setup;
        deviceManager.getAudioDeviceSetup (setup);
        setup.bufferSize = bufSize;
        deviceManager.setAudioDeviceSetup (setup, true);
    };
    addAndMakeVisible (bufferSizeCombo);

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

    // ===== PITCH SECTION =====
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

    // ===== FILTER SECTION =====
    setupKnob (highPassKnob, highPassLabel, "HP Freq", 0.0, 5000.0, 1.0, 0.0, " Hz");
    highPassKnob.setSkewFactorFromMidPoint (500.0);
    highPassKnob.onValueChange = [this] {
        float v = (float) highPassKnob.getValue();
        fftProcessorL.setHighPassFreq (v);
        fftProcessorR.setHighPassFreq (v);
    };

    setupKnob (highPassSlopeKnob, highPassSlopeLabel, "HP Slope", 1.0, 8.0, 0.5, 1.0);
    highPassSlopeKnob.onValueChange = [this] {
        float v = (float) highPassSlopeKnob.getValue();
        fftProcessorL.setHighPassSlope (v);
        fftProcessorR.setHighPassSlope (v);
    };

    setupKnob (lowPassKnob, lowPassLabel, "LP Freq", 200.0, 20000.0, 1.0, 20000.0, " Hz");
    lowPassKnob.setSkewFactorFromMidPoint (2000.0);
    lowPassKnob.onValueChange = [this] {
        float v = (float) lowPassKnob.getValue();
        fftProcessorL.setLowPassFreq (v);
        fftProcessorR.setLowPassFreq (v);
    };

    setupKnob (lowPassSlopeKnob, lowPassSlopeLabel, "LP Slope", 1.0, 8.0, 0.5, 1.0);
    lowPassSlopeKnob.onValueChange = [this] {
        float v = (float) lowPassSlopeKnob.getValue();
        fftProcessorL.setLowPassSlope (v);
        fftProcessorR.setLowPassSlope (v);
    };

    // Harmonic filter
    harmonicToggle.setToggleState (false, juce::dontSendNotification);
    harmonicToggle.onClick = [this] {
        bool v = harmonicToggle.getToggleState();
        fftProcessorL.setHarmonicFilterEnabled (v);
        fftProcessorR.setHarmonicFilterEnabled (v);
    };
    addAndMakeVisible (harmonicToggle);

    setupKnob (harmonicIntensityKnob, harmonicIntensityLabel, "Intensity", 0.0, 1.0, 0.01, 1.0);
    harmonicIntensityKnob.onValueChange = [this] {
        float v = (float) harmonicIntensityKnob.getValue();
        fftProcessorL.setHarmonicIntensity (v);
        fftProcessorR.setHarmonicIntensity (v);
    };

    // Root note combo
    const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    for (int i = 0; i < 12; ++i)
        rootNoteCombo.addItem (noteNames[i], i + 1);
    rootNoteCombo.setSelectedId (1);
    rootNoteCombo.onChange = [this] {
        int v = rootNoteCombo.getSelectedId() - 1;
        fftProcessorL.setHarmonicRootNote (v);
        fftProcessorR.setHarmonicRootNote (v);
    };
    addAndMakeVisible (rootNoteCombo);

    // Scale type combo
    scaleTypeCombo.addItem ("Octaves", 1);
    scaleTypeCombo.addItem ("Fifths", 2);
    scaleTypeCombo.addItem ("Ionian (Major)", 3);
    scaleTypeCombo.addItem ("Dorian", 4);
    scaleTypeCombo.addItem ("Phrygian", 5);
    scaleTypeCombo.addItem ("Lydian", 6);
    scaleTypeCombo.addItem ("Mixolydian", 7);
    scaleTypeCombo.addItem ("Aeolian (Minor)", 8);
    scaleTypeCombo.addItem ("Locrian", 9);
    scaleTypeCombo.addItem ("Pentatonic Maj", 10);
    scaleTypeCombo.addItem ("Pentatonic Min", 11);
    scaleTypeCombo.setSelectedId (1);
    scaleTypeCombo.onChange = [this] {
        int v = scaleTypeCombo.getSelectedId() - 1;
        fftProcessorL.setHarmonicScaleType (v);
        fftProcessorR.setHarmonicScaleType (v);
    };
    addAndMakeVisible (scaleTypeCombo);

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
        // Volume slider (vertical)
        loopVolumeSliders[i].setSliderStyle (juce::Slider::LinearVertical);
        loopVolumeSliders[i].setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        loopVolumeSliders[i].setRange (0.0, 1.0, 0.01);
        loopVolumeSliders[i].setValue (1.0);
        loopVolumeSliders[i].onValueChange = [this, i] {
            loopRecorder.setSlotVolume (i, (float) loopVolumeSliders[i].getValue());
        };
        addAndMakeVisible (loopVolumeSliders[i]);

        // High-pass slider
        loopHPSliders[i].setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        loopHPSliders[i].setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        loopHPSliders[i].setRange (20.0, 2000.0, 1.0);
        loopHPSliders[i].setSkewFactorFromMidPoint (200.0);
        loopHPSliders[i].setValue (20.0);
        loopHPSliders[i].onValueChange = [this, i] {
            loopRecorder.setSlotHighPass (i, (float) loopHPSliders[i].getValue());
        };
        addAndMakeVisible (loopHPSliders[i]);

        // Low-pass slider
        loopLPSliders[i].setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        loopLPSliders[i].setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        loopLPSliders[i].setRange (500.0, 20000.0, 1.0);
        loopLPSliders[i].setSkewFactorFromMidPoint (4000.0);
        loopLPSliders[i].setValue (20000.0);
        loopLPSliders[i].onValueChange = [this, i] {
            loopRecorder.setSlotLowPass (i, (float) loopLPSliders[i].getValue());
        };
        addAndMakeVisible (loopLPSliders[i]);

        // Pitch selector
        loopPitchCombos[i].addItem ("-1 oct", 1);
        loopPitchCombos[i].addItem ("Normal", 2);
        loopPitchCombos[i].addItem ("+1 oct", 3);
        loopPitchCombos[i].setSelectedId (2);
        loopPitchCombos[i].onChange = [this, i] {
            int octave = loopPitchCombos[i].getSelectedId() - 2;  // -1, 0, or +1
            loopRecorder.setSlotPitchOctave (i, octave);
        };
        addAndMakeVisible (loopPitchCombos[i]);

        // Loop button
        loopButtons[i].setButtonText ("L" + juce::String (i + 1));
        loopButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colours::darkgrey);
        loopButtons[i].onClick = [this, i] {
            if (loopRecorder.isRecording() && loopRecorder.getRecordingSlot() == i)
            {
                // Stop recording
                loopRecorder.stopRecording();
            }
            else if (loopRecorder.isSlotActive (i))
            {
                // Slot has content - clear it on click
                loopRecorder.clearSlot (i);
            }
            else
            {
                // Start recording to this slot
                loopRecorder.startRecording (i);
            }
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
            static juce::int64 lastClickTime = 0;
            static int lastClickSlot = -1;

            juce::int64 now = juce::Time::currentTimeMillis();

            // Double-click detection (within 400ms on same slot)
            if (lastClickSlot == i && (now - lastClickTime) < 400)
            {
                // Double-click: open editor
                auto order = effectsChain.getOrder();
                showEffectEditor (order[i]);
                lastClickTime = 0;
                lastClickSlot = -1;
            }
            else
            {
                // Single click: toggle selection for reordering
                if (selectedEffectSlot == i)
                    selectedEffectSlot = -1;  // Deselect
                else
                    selectedEffectSlot = i;
                updateEffectButtonColors();
                lastClickTime = now;
                lastClickSlot = i;
            }
        };
        addAndMakeVisible (effectButtons[i]);

        effectEnableToggles[i].setToggleState (true, juce::dontSendNotification);
        effectEnableToggles[i].onClick = [this, i] {
            auto order = effectsChain.getOrder();
            int effectType = order[i];
            auto* effect = effectsChain.getEffect (effectType);
            if (effect)
                effect->setEnabled (effectEnableToggles[i].getToggleState());
            updateEffectButtonColors();
        };
        addAndMakeVisible (effectEnableToggles[i]);
    }

    moveLeftButton.onClick = [this] {
        if (selectedEffectSlot > 0)
        {
            effectsChain.swapPositions (selectedEffectSlot, selectedEffectSlot - 1);
            selectedEffectSlot--;
            updateEffectButtonColors();
        }
    };
    addAndMakeVisible (moveLeftButton);

    moveRightButton.onClick = [this] {
        if (selectedEffectSlot >= 0 && selectedEffectSlot < 5)
        {
            effectsChain.swapPositions (selectedEffectSlot, selectedEffectSlot + 1);
            selectedEffectSlot++;
            updateEffectButtonColors();
        }
    };
    addAndMakeVisible (moveRightButton);

    startTimerHz (30);
    setSize (1000, 720);  // Increased height for effects section
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
        effectButtons[i].setButtonText (effectNames[effectType]);

        auto* effect = effectsChain.getEffect (effectType);
        bool enabled = effect ? effect->isEnabled() : true;
        effectEnableToggles[i].setToggleState (enabled, juce::dontSendNotification);

        if (i == selectedEffectSlot)
            effectButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colours::dodgerblue);
        else if (enabled)
            effectButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colours::green.darker());
        else
            effectButtons[i].setColour (juce::TextButton::buttonColourId, juce::Colours::darkgrey);
    }
}

//==============================================================================
// Effect Editor Dialog Component
class EffectEditorContent : public juce::Component
{
public:
    EffectEditorContent (EffectsChain& chain, int effectType)
        : effectsChain (chain), type (effectType)
    {
        setSize (350, 300);
        buildControls();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (10);
        bounds.removeFromTop (10);

        for (auto& slider : sliders)
        {
            auto row = bounds.removeFromTop (50);
            labels[&slider - sliders.data()]->setBounds (row.removeFromTop (16));
            slider->setBounds (row.reduced (0, 2));
        }

        for (auto& combo : combos)
        {
            auto row = bounds.removeFromTop (40);
            comboLabels[&combo - combos.data()]->setBounds (row.removeFromTop (16));
            combo->setBounds (row.removeFromTop (22).reduced (0, 2));
        }

        for (auto& toggle : toggles)
        {
            bounds.removeFromTop (5);
            toggle->setBounds (bounds.removeFromTop (22));
        }
    }

private:
    EffectsChain& effectsChain;
    int type;

    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<juce::ComboBox>> combos;
    std::vector<std::unique_ptr<juce::Label>> comboLabels;
    std::vector<std::unique_ptr<juce::ToggleButton>> toggles;

    void addSlider (const juce::String& name, double min, double max, double step, double value,
                    std::function<void(float)> onChange, const juce::String& suffix = "")
    {
        auto label = std::make_unique<juce::Label>();
        label->setText (name, juce::dontSendNotification);
        label->setFont (juce::Font (12.0f));
        addAndMakeVisible (*label);
        labels.push_back (std::move (label));

        auto slider = std::make_unique<juce::Slider>();
        slider->setSliderStyle (juce::Slider::LinearHorizontal);
        slider->setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 20);
        slider->setRange (min, max, step);
        slider->setValue (value);
        if (suffix.isNotEmpty())
            slider->setTextValueSuffix (suffix);
        slider->onValueChange = [slider = slider.get(), onChange] {
            onChange (static_cast<float> (slider->getValue()));
        };
        addAndMakeVisible (*slider);
        sliders.push_back (std::move (slider));
    }

    void addCombo (const juce::String& name, juce::StringArray items, int selectedIdx,
                   std::function<void(int)> onChange)
    {
        auto label = std::make_unique<juce::Label>();
        label->setText (name, juce::dontSendNotification);
        label->setFont (juce::Font (12.0f));
        addAndMakeVisible (*label);
        comboLabels.push_back (std::move (label));

        auto combo = std::make_unique<juce::ComboBox>();
        for (int i = 0; i < items.size(); ++i)
            combo->addItem (items[i], i + 1);
        combo->setSelectedId (selectedIdx + 1);
        combo->onChange = [combo = combo.get(), onChange] {
            onChange (combo->getSelectedId() - 1);
        };
        addAndMakeVisible (*combo);
        combos.push_back (std::move (combo));
    }

    void addToggle (const juce::String& name, bool value, std::function<void(bool)> onChange)
    {
        auto toggle = std::make_unique<juce::ToggleButton> (name);
        toggle->setToggleState (value, juce::dontSendNotification);
        toggle->onClick = [toggle = toggle.get(), onChange] {
            onChange (toggle->getToggleState());
        };
        addAndMakeVisible (*toggle);
        toggles.push_back (std::move (toggle));
    }

    void buildControls()
    {
        switch (type)
        {
            case EffectsChain::Filter:
                if (auto* fx = effectsChain.getFilter())
                {
                    addSlider ("High-Pass Freq", 20, 5000, 1, fx->getHighPassFreq(),
                               [fx](float v) { fx->setHighPassFreq (v); }, " Hz");
                    addSlider ("HP Poles", 1, 8, 1, fx->getHighPassPoles(),
                               [fx](float v) { fx->setHighPassPoles ((int) v); });
                    addSlider ("Low-Pass Freq", 200, 20000, 1, fx->getLowPassFreq(),
                               [fx](float v) { fx->setLowPassFreq (v); }, " Hz");
                    addSlider ("LP Poles", 1, 8, 1, fx->getLowPassPoles(),
                               [fx](float v) { fx->setLowPassPoles ((int) v); });
                    addToggle ("Harmonic Filter", fx->isHarmonicEnabled(),
                               [fx](bool v) { fx->setHarmonicEnabled (v); });
                    addSlider ("Harmonic Intensity", 0, 1, 0.01, fx->getHarmonicIntensity(),
                               [fx](float v) { fx->setHarmonicIntensity (v); });
                }
                break;

            case EffectsChain::Delay:
                if (auto* fx = effectsChain.getDelay())
                {
                    addSlider ("Delay Time", 1, 2000, 1, fx->getDelayTimeMs(),
                               [fx](float v) { fx->setDelayTimeMs (v); }, " ms");
                    addSlider ("Feedback", 0, 0.95, 0.01, fx->getFeedback(),
                               [fx](float v) { fx->setFeedback (v); });
                    addSlider ("Dry/Wet", 0, 1, 0.01, fx->getDryWet(),
                               [fx](float v) { fx->setDryWet (v); });
                    addToggle ("Ping-Pong", fx->isPingPong(),
                               [fx](bool v) { fx->setPingPong (v); });
                }
                break;

            case EffectsChain::Granular:
                if (auto* fx = effectsChain.getGranular())
                {
                    addSlider ("Grain Size", 10, 500, 1, fx->getGrainSize(),
                               [fx](float v) { fx->setGrainSize (v); }, " ms");
                    addSlider ("Pitch", -24, 24, 0.1, fx->getPitch(),
                               [fx](float v) { fx->setPitch (v); }, " st");
                    addSlider ("Density", 0.1, 4, 0.1, fx->getDensity(),
                               [fx](float v) { fx->setDensity (v); });
                    addSlider ("Spread", 0, 1, 0.01, fx->getSpread(),
                               [fx](float v) { fx->setSpread (v); });
                    addSlider ("Dry/Wet", 0, 1, 0.01, fx->getDryWet(),
                               [fx](float v) { fx->setDryWet (v); });
                }
                break;

            case EffectsChain::Tremolo:
                if (auto* fx = effectsChain.getTremolo())
                {
                    addSlider ("Rate", 0.1, 20, 0.1, fx->getRate(),
                               [fx](float v) { fx->setRate (v); }, " Hz");
                    addSlider ("Depth", 0, 1, 0.01, fx->getDepth(),
                               [fx](float v) { fx->setDepth (v); });
                    addCombo ("Waveform", { "Sine", "Triangle", "Square" }, fx->getWaveform(),
                              [fx](int v) { fx->setWaveform (v); });
                    addToggle ("Stereo", fx->isStereo(),
                               [fx](bool v) { fx->setStereo (v); });
                }
                break;

            case EffectsChain::Distortion:
                if (auto* fx = effectsChain.getDistortion())
                {
                    addCombo ("Algorithm", { "Soft Clip", "Hard Clip", "Wavefold", "Bitcrush" },
                              fx->getAlgorithm(), [fx](int v) { fx->setAlgorithm (v); });
                    addSlider ("Drive", 1, 20, 0.1, fx->getDrive(),
                               [fx](float v) { fx->setDrive (v); });
                    addSlider ("Tone", 0, 1, 0.01, fx->getTone(),
                               [fx](float v) { fx->setTone (v); });
                    addSlider ("Dry/Wet", 0, 1, 0.01, fx->getDryWet(),
                               [fx](float v) { fx->setDryWet (v); });
                    addSlider ("Bit Depth", 1, 16, 0.5, fx->getBitDepth(),
                               [fx](float v) { fx->setBitDepth (v); }, " bits");
                }
                break;

            case EffectsChain::Tape:
                if (auto* fx = effectsChain.getTape())
                {
                    addSlider ("Saturation", 0, 1, 0.01, fx->getSaturation(),
                               [fx](float v) { fx->setSaturation (v); });
                    addSlider ("Bias", 0, 1, 0.01, fx->getBias(),
                               [fx](float v) { fx->setBias (v); });
                    addSlider ("Wow", 0, 1, 0.01, fx->getWowDepth(),
                               [fx](float v) { fx->setWowDepth (v); });
                    addSlider ("Flutter", 0, 1, 0.01, fx->getFlutterDepth(),
                               [fx](float v) { fx->setFlutterDepth (v); });
                    addSlider ("HF Loss", 0, 1, 0.01, fx->getHfLoss(),
                               [fx](float v) { fx->setHfLoss (v); });
                    addSlider ("Dry/Wet", 0, 1, 0.01, fx->getDryWet(),
                               [fx](float v) { fx->setDryWet (v); });
                }
                break;
        }
    }
};

void MainComponent::showEffectEditor (int effectType)
{
    const char* effectNames[] = { "Filter", "Delay", "Granular", "Tremolo", "Distortion", "Tape" };

    auto* editor = new EffectEditorContent (effectsChain, effectType);

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned (editor);
    options.dialogTitle = juce::String (effectNames[effectType]) + " Settings";
    options.dialogBackgroundColour = juce::Colour (0xff1a1a2e);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.launchAsync();
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a2e));

    // Section backgrounds
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
    bounds.removeFromTop (55); // Header area with meters

    // Effects section at bottom
    auto effectsArea = bounds.removeFromBottom (80).reduced (5);
    drawSection (effectsArea, "EFFECTS CHAIN");

    bounds.removeFromBottom (10);

    const int sectionWidth = (bounds.getWidth() - 30) / 4;

    // Drone section
    drawSection (bounds.removeFromLeft (sectionWidth).reduced (5), "DRONE");
    bounds.removeFromLeft (10);

    // Pitch section
    drawSection (bounds.removeFromLeft (sectionWidth).reduced (5), "PITCH");
    bounds.removeFromLeft (10);

    // Filter section
    drawSection (bounds.removeFromLeft (sectionWidth).reduced (5), "FILTERS");
    bounds.removeFromLeft (10);

    // Loops section
    drawSection (bounds.reduced (5), "LOOPS");

    // Input level meter
    auto meterBounds = getLocalBounds().reduced (10);
    int meterY = 10;
    int meterHeight = 14;

    g.setColour (juce::Colours::grey.darker());
    g.fillRoundedRectangle (15.0f, (float)meterY, 180.0f, (float)meterHeight, 3.0f);
    g.setColour (juce::Colours::limegreen);
    const float inLevel = juce::jlimit (0.0f, 1.0f, inputLevel.load());
    g.fillRoundedRectangle (15.0f, (float)meterY, 180.0f * inLevel, (float)meterHeight, 3.0f);
    g.setColour (juce::Colours::white);
    g.setFont (10.0f);
    g.drawText ("IN", 15, meterY, 180, meterHeight, juce::Justification::centred);

    // Output level meter
    meterY += meterHeight + 4;
    g.setColour (juce::Colours::grey.darker());
    g.fillRoundedRectangle (15.0f, (float)meterY, 180.0f, (float)meterHeight, 3.0f);
    g.setColour (juce::Colours::cyan);
    const float outLevel = juce::jlimit (0.0f, 1.0f, outputLevel.load());
    g.fillRoundedRectangle (15.0f, (float)meterY, 180.0f * outLevel, (float)meterHeight, 3.0f);
    g.setColour (juce::Colours::white);
    g.drawText ("OUT", 15, meterY, 180, meterHeight, juce::Justification::centred);
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced (10);

    // Header row with buttons
    auto header = bounds.removeFromTop (45);
    header.removeFromLeft (200); // Space for meters
    testButton.setBounds (header.removeFromLeft (50));
    header.removeFromLeft (5);
    bufferSizeCombo.setBounds (header.removeFromLeft (80));
    header.removeFromLeft (5);
    resourcesButton.setBounds (header.removeFromRight (80));
    header.removeFromRight (5);
    settingsButton.setBounds (header.removeFromRight (70));

    bounds.removeFromTop (10);

    // Effects chain section at bottom
    auto effectsArea = bounds.removeFromBottom (80).reduced (5);
    effectsArea.removeFromTop (28);  // Title space

    // Move buttons on the left
    auto moveArea = effectsArea.removeFromLeft (60);
    moveLeftButton.setBounds (moveArea.removeFromTop (25).reduced (2));
    moveArea.removeFromTop (5);
    moveRightButton.setBounds (moveArea.removeFromTop (25).reduced (2));

    // Effect buttons and toggles
    const int effectWidth = (effectsArea.getWidth() - 20) / 6;
    effectsArea.removeFromLeft (10);

    for (int i = 0; i < 6; ++i)
    {
        auto effectSlot = effectsArea.removeFromLeft (effectWidth);
        effectButtons[i].setBounds (effectSlot.removeFromTop (28).reduced (2));
        effectEnableToggles[i].setBounds (effectSlot.removeFromTop (20).reduced (4, 0));
    }

    bounds.removeFromBottom (10);

    const int sectionWidth = (bounds.getWidth() - 30) / 4;
    const int knobSize = 60;
    const int labelHeight = 16;

    auto layoutKnobWithLabel = [&](juce::Slider& knob, juce::Label& label, juce::Rectangle<int>& area) {
        auto knobArea = area.removeFromLeft (knobSize + 4);
        label.setBounds (knobArea.removeFromTop (labelHeight));
        knob.setBounds (knobArea.reduced (2));
    };

    // ===== DRONE SECTION =====
    auto droneArea = bounds.removeFromLeft (sectionWidth).reduced (5);
    droneArea.removeFromTop (28); // Title space

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

    droneArea.removeFromTop (5);
    peakToggle.setBounds (droneArea.removeFromTop (20));
    droneArea.removeFromTop (2);
    phaseToggle.setBounds (droneArea.removeFromTop (20));

    bounds.removeFromLeft (10);

    // ===== PITCH SECTION =====
    auto pitchArea = bounds.removeFromLeft (sectionWidth).reduced (5);
    pitchArea.removeFromTop (28);

    auto pitchRow = pitchArea.removeFromTop (knobSize + labelHeight + 5);
    layoutKnobWithLabel (pitchKnob, pitchLabel, pitchRow);
    layoutKnobWithLabel (octaveKnob, octaveLabel, pitchRow);

    bounds.removeFromLeft (10);

    // ===== FILTER SECTION =====
    auto filterArea = bounds.removeFromLeft (sectionWidth).reduced (5);
    filterArea.removeFromTop (28);

    // HP and LP row
    auto filterRow1 = filterArea.removeFromTop (knobSize + labelHeight + 5);
    layoutKnobWithLabel (highPassKnob, highPassLabel, filterRow1);
    layoutKnobWithLabel (highPassSlopeKnob, highPassSlopeLabel, filterRow1);

    auto filterRow2 = filterArea.removeFromTop (knobSize + labelHeight + 5);
    layoutKnobWithLabel (lowPassKnob, lowPassLabel, filterRow2);
    layoutKnobWithLabel (lowPassSlopeKnob, lowPassSlopeLabel, filterRow2);

    filterArea.removeFromTop (5);

    // Harmonic filter row
    harmonicToggle.setBounds (filterArea.removeFromTop (20).removeFromLeft (100));

    auto harmRow2 = filterArea.removeFromTop (knobSize + labelHeight + 5);
    layoutKnobWithLabel (harmonicIntensityKnob, harmonicIntensityLabel, harmRow2);

    filterArea.removeFromTop (2);
    auto comboRow = filterArea.removeFromTop (24);
    rootNoteCombo.setBounds (comboRow.removeFromLeft (55));
    comboRow.removeFromLeft (5);
    scaleTypeCombo.setBounds (comboRow.removeFromLeft (120));

    bounds.removeFromLeft (10);

    // ===== LOOPS SECTION =====
    auto loopsArea = bounds.reduced (5);
    loopsArea.removeFromTop (28);

    // Live/Loop level knobs at top
    auto loopsRow1 = loopsArea.removeFromTop (knobSize + labelHeight + 5);
    layoutKnobWithLabel (liveLevelKnob, liveLevelLabel, loopsRow1);
    layoutKnobWithLabel (loopLevelKnob, loopLevelLabel, loopsRow1);

    loopsArea.removeFromTop (5);

    // Per-loop controls - 4 vertical strips
    auto loopStripsArea = loopsArea.removeFromTop (200);
    const int stripWidth = loopStripsArea.getWidth() / 4;
    const int smallKnob = 40;

    for (int i = 0; i < 4; ++i)
    {
        auto strip = loopStripsArea.removeFromLeft (stripWidth).reduced (2);

        // Volume slider (vertical, at top)
        loopVolumeSliders[i].setBounds (strip.removeFromTop (60).reduced (8, 0));

        // HP knob
        loopHPSliders[i].setBounds (strip.removeFromTop (smallKnob).reduced (4, 0));

        // LP knob
        loopLPSliders[i].setBounds (strip.removeFromTop (smallKnob).reduced (4, 0));

        // Pitch selector
        loopPitchCombos[i].setBounds (strip.removeFromTop (22).reduced (2, 0));

        strip.removeFromTop (4);

        // Loop button at bottom
        loopButtons[i].setBounds (strip.removeFromTop (28).reduced (4, 0));
    }

    loopsArea.removeFromTop (5);
    clearLoopsButton.setBounds (loopsArea.removeFromTop (25).removeFromLeft (80));
}

void MainComponent::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    testPhase = 0.0;
    testToneActive.store (false);
    testSamplesRemaining = 0;

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

    // Initialize loop recorder
    loopRecorder.prepareToPlay (currentSampleRate, blockSize);

    // Initialize effects chain
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
    // Start timing for CPU load measurement
    const auto startTime = juce::Time::getHighResolutionTicks();

    for (int ch = 0; ch < numOutputChannels; ++ch)
        if (auto* out = outputChannelData[ch])
            juce::FloatVectorOperations::clear (out, numSamples);

    if (numSamples <= 0)
        return;

    if (monoBuffer.getNumSamples() < numSamples)
        return;

    float* mono = monoBuffer.getWritePointer (0);
    juce::FloatVectorOperations::clear (mono, numSamples);

    int activeInputs = 0;
    for (int ch = 0; ch < numInputChannels; ++ch)
    {
        const float* in = inputChannelData[ch];
        if (in == nullptr)
            continue;

        juce::FloatVectorOperations::add (mono, in, numSamples);
        ++activeInputs;
    }

    if (activeInputs > 0)
        juce::FloatVectorOperations::multiply (mono, 1.0f / (float) activeInputs, numSamples);

    // Input meter (RMS)
    if (activeInputs > 0)
    {
        float sumSquares = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            sumSquares += mono[i] * mono[i];

        const float rms = std::sqrt (sumSquares / (float) numSamples);
        inputLevel.store (rms * 3.0f);  // Scale for visibility
    }
    else
    {
        inputLevel.store (0.0f);
    }

    // Process audio with stereo decorrelation
    float outputSum = 0.0f;
    const float width = stereoWidth.load();

    // Check loop state once per buffer (not per sample)
    const bool isRecording = loopRecorder.isRecording();
    const bool hasLoops = loopRecorder.hasAnyContent();
    const float liveLvl = liveLevel.load();
    const float loopLvl = loopLevel.load();

    if (activeInputs > 0 || hasLoops)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float fftInput = 0.0f;

            // Live input contribution
            if (activeInputs > 0)
            {
                float liveSample = mono[i];

                // Record to loop if recording
                if (isRecording)
                    loopRecorder.recordSample (liveSample);

                fftInput += liveSample * liveLvl;
            }

            // Loop contribution
            if (hasLoops)
                fftInput += loopRecorder.getLoopMix() * loopLvl;

            // Process through FFT processors
            // Optimization: when stereo width is 0, only process one FFT (halves CPU load)
            float outL, outR;
            if (width < 0.01f)
            {
                // Mono mode - only use left processor
                float processed = fftProcessorL.processSample (fftInput, false);
                outL = outR = processed;
            }
            else
            {
                // Stereo mode - process through both FFT processors
                float processedL = fftProcessorL.processSample (fftInput, false);
                float processedR = fftProcessorR.processSample (fftInput, false);

                // Stereo width: 0 = mono (average), 1 = full stereo (independent)
                float mid = (processedL + processedR) * 0.5f;
                outL = mid + (processedL - mid) * width;
                outR = mid + (processedR - mid) * width;
            }

            // Process through effects chain
            effectsChain.processSample (outL, outR);

            outputSum += outL * outL + outR * outR;

            // Output to channels
            if (numOutputChannels >= 1 && outputChannelData[0] != nullptr)
                outputChannelData[0][i] += outL;
            if (numOutputChannels >= 2 && outputChannelData[1] != nullptr)
                outputChannelData[1][i] += outR;
        }
    }

    // Output meter (RMS) - divide by 2*numSamples since we summed L+R
    const float outRms = std::sqrt (outputSum / (float) (numSamples * 2));
    outputLevel.store (outRms * 3.0f);  // Scale for visibility

    // Test tone
    if (testToneActive.load())
    {
        auto* dev = deviceManager.getCurrentAudioDevice();
        const double sampleRate = (dev != nullptr ? dev->getCurrentSampleRate() : 48000.0);
        const double phaseInc = juce::MathConstants<double>::twoPi * 440.0 / sampleRate;

        for (int i = 0; i < numSamples; ++i)
        {
            if (--testSamplesRemaining <= 0)
            {
                testToneActive.store (false);
                break;
            }

            const float s = std::sin (testPhase) * 0.2f;
            testPhase += phaseInc;

            for (int ch = 0; ch < numOutputChannels; ++ch)
            {
                float* out = outputChannelData[ch];
                if (out != nullptr)
                    out[i] += s;
            }
        }
    }

    // Calculate CPU load: time used vs time available
    const auto endTime = juce::Time::getHighResolutionTicks();
    const double elapsedSeconds = juce::Time::highResolutionTicksToSeconds (endTime - startTime);
    const double availableSeconds = (double) numSamples / currentSampleRate;
    const float load = (float) (elapsedSeconds / availableSeconds * 100.0);

    // Smooth the CPU load reading (exponential moving average)
    const float smoothing = 0.1f;
    cpuLoad.store (cpuLoad.load() * (1.0f - smoothing) + load * smoothing);
}

void MainComponent::timerCallback()
{
    // Update loop button colors based on state
    for (int i = 0; i < 4; ++i)
    {
        juce::Colour buttonColour;

        if (loopRecorder.isRecording() && loopRecorder.getRecordingSlot() == i)
        {
            // Recording - red (pulse effect by alternating shades)
            static int pulseCounter = 0;
            pulseCounter = (pulseCounter + 1) % 15;
            float pulse = 0.7f + 0.3f * std::sin (pulseCounter * 0.4f);
            buttonColour = juce::Colour::fromFloatRGBA (pulse, 0.1f, 0.1f, 1.0f);
        }
        else if (loopRecorder.isSlotActive (i))
        {
            // Has content - green
            buttonColour = juce::Colours::green;
        }
        else
        {
            // Empty - dark grey
            buttonColour = juce::Colours::darkgrey;
        }

        loopButtons[i].setColour (juce::TextButton::buttonColourId, buttonColour);
    }

    repaint();
}
