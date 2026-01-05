#pragma once
#include <JuceHeader.h>
#include "FFTProcessor.h"
#include "LoopRecorder.h"
#include "Effects/EffectsChain.h"

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
            false,       // show MIDI input (disabled)
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
                       public juce::Timer
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

private:
    // Helper to create a rotary knob with label
    void setupKnob (juce::Slider& knob, juce::Label& label, const juce::String& name,
                    double min, double max, double step, double initial,
                    const juce::String& suffix = "");

    // Helper to update effect button colors based on current chain order
    void updateEffectButtonColors();

    // Update the effect parameter knobs for selected effect
    void updateEffectParameterKnobs();

    // Audio
    juce::AudioDeviceManager deviceManager;

    // Header buttons
    juce::TextButton settingsButton { "Settings" };
    juce::TextButton resourcesButton { "Resources" };

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
    juce::Slider liveLevelKnob;   juce::Label liveLevelLabel;
    juce::Slider loopLevelKnob;   juce::Label loopLevelLabel;
    std::array<juce::TextButton, 4> loopButtons;
    juce::TextButton clearLoopsButton { "Clear All" };
    std::atomic<float> liveLevel { 1.0f };
    std::atomic<float> loopLevel { 0.5f };

    // Per-loop controls
    std::array<juce::Slider, 4> loopVolumeSliders;
    std::array<juce::Label, 4> loopVolumeLabels;
    std::array<juce::Slider, 4> loopHPSliders;
    std::array<juce::Label, 4> loopHPLabels;
    std::array<juce::Slider, 4> loopLPSliders;
    std::array<juce::Label, 4> loopLPLabels;
    std::array<juce::ComboBox, 4> loopPitchCombos;
    std::array<juce::Label, 4> loopPitchLabels;

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
