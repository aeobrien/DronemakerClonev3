#include "MainComponent.h"

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
        auto* monitor = new ResourceMonitor();
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
    scaleTypeCombo.setSelectedId (1);
    scaleTypeCombo.onChange = [this] {
        int v = scaleTypeCombo.getSelectedId() - 1;
        fftProcessorL.setHarmonicScaleType (v);
        fftProcessorR.setHarmonicScaleType (v);
    };
    addAndMakeVisible (scaleTypeCombo);

    startTimerHz (30);
    setSize (750, 520);
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

    const int sectionWidth = (bounds.getWidth() - 20) / 3;

    // Drone section
    drawSection (bounds.removeFromLeft (sectionWidth).reduced (5), "DRONE");
    bounds.removeFromLeft (10);

    // Pitch section
    drawSection (bounds.removeFromLeft (sectionWidth).reduced (5), "PITCH");
    bounds.removeFromLeft (10);

    // Filter section
    drawSection (bounds.reduced (5), "FILTERS");

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
    resourcesButton.setBounds (header.removeFromRight (80));
    header.removeFromRight (5);
    settingsButton.setBounds (header.removeFromRight (70));

    bounds.removeFromTop (10);

    const int sectionWidth = (bounds.getWidth() - 20) / 3;
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

    auto droneRow2 = droneArea.removeFromTop (knobSize + labelHeight + 5);
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
    auto filterArea = bounds.reduced (5);
    filterArea.removeFromTop (28);

    // HP and LP row
    auto filterRow1 = filterArea.removeFromTop (knobSize + labelHeight + 5);
    layoutKnobWithLabel (highPassKnob, highPassLabel, filterRow1);
    layoutKnobWithLabel (highPassSlopeKnob, highPassSlopeLabel, filterRow1);
    filterRow1.removeFromLeft (8);
    layoutKnobWithLabel (lowPassKnob, lowPassLabel, filterRow1);
    layoutKnobWithLabel (lowPassSlopeKnob, lowPassSlopeLabel, filterRow1);

    filterArea.removeFromTop (5);

    // Harmonic filter row
    auto harmRow1 = filterArea.removeFromTop (20);
    harmonicToggle.setBounds (harmRow1.removeFromLeft (100));

    auto harmRow2 = filterArea.removeFromTop (knobSize + labelHeight + 5);
    layoutKnobWithLabel (harmonicIntensityKnob, harmonicIntensityLabel, harmRow2);

    filterArea.removeFromTop (2);
    auto comboRow = filterArea.removeFromTop (24);
    rootNoteCombo.setBounds (comboRow.removeFromLeft (55));
    comboRow.removeFromLeft (5);
    scaleTypeCombo.setBounds (comboRow.removeFromLeft (120));
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

    float sr = static_cast<float>(device->getCurrentSampleRate());
    fftProcessorL.setSampleRate (sr);
    fftProcessorL.reset();
    fftProcessorR.setSampleRate (sr);
    fftProcessorR.reset();
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

    if (activeInputs > 0)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            // Process through both FFT processors (each has its own random phase generator)
            float processedL = fftProcessorL.processSample (mono[i], false);
            float processedR = fftProcessorR.processSample (mono[i], false);

            // Stereo width: 0 = mono (average), 1 = full stereo (independent)
            float mid = (processedL + processedR) * 0.5f;
            float outL = mid + (processedL - mid) * width;
            float outR = mid + (processedR - mid) * width;

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
}

void MainComponent::timerCallback()
{
    repaint();
}
