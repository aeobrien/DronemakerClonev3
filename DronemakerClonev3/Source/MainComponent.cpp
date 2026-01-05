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

    // Loop slot buttons
    for (int i = 0; i < 4; ++i)
    {
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
                // Slot has content - clear it on click (could also be right-click only)
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

    startTimerHz (30);
    setSize (850, 580);
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

    auto loopsRow1 = loopsArea.removeFromTop (knobSize + labelHeight + 5);
    layoutKnobWithLabel (liveLevelKnob, liveLevelLabel, loopsRow1);
    layoutKnobWithLabel (loopLevelKnob, loopLevelLabel, loopsRow1);

    loopsArea.removeFromTop (10);

    // Loop buttons row
    auto buttonRow = loopsArea.removeFromTop (30);
    const int buttonWidth = 40;
    for (int i = 0; i < 4; ++i)
    {
        loopButtons[i].setBounds (buttonRow.removeFromLeft (buttonWidth));
        buttonRow.removeFromLeft (5);
    }

    loopsArea.removeFromTop (10);
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
