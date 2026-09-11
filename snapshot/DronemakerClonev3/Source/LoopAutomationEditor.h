#pragma once

#include <JuceHeader.h>
#include "LoopRecorder.h"

/**
 * Modal dialog for editing per-loop automation settings.
 * Contains:
 * - Max record length slider (5-300s)
 * - Command list with add/remove/reorder
 * - Command parameter editors (duration, target level, curve type)
 * - Loop sequence toggle
 * - Preview/Stop Preview buttons
 */
class LoopAutomationEditor : public juce::Component,
                              public juce::Timer
{
public:
    LoopAutomationEditor (LoopRecorder& recorder, int slotIndex);
    ~LoopAutomationEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // Allow selection callback from list model
    void onCommandSelected (int index);

private:
    friend class CommandListModel;

    LoopRecorder& loopRecorder;
    int slot;
    LoopSettings settings;

    // Max record length
    juce::Slider maxLengthSlider;
    juce::Label maxLengthLabel;

    // Command list
    juce::ListBox commandList;
    std::unique_ptr<juce::ListBoxModel> commandListModel;

    // Command editing
    juce::ComboBox commandTypeCombo;
    juce::Slider durationSlider;
    juce::Label durationLabel;
    juce::Slider targetLevelSlider;
    juce::Label targetLevelLabel;
    juce::ComboBox curveTypeCombo;
    juce::Label curveTypeLabel;

    // Loop toggle
    juce::ToggleButton loopToggle { "Loop Sequence" };

    // Buttons
    juce::TextButton addButton { "Add" };
    juce::TextButton removeButton { "Remove" };
    juce::TextButton moveUpButton { "Up" };
    juce::TextButton moveDownButton { "Down" };
    juce::TextButton previewButton { "Preview" };
    juce::TextButton applyButton { "Apply" };
    juce::TextButton resetButton { "Reset" };
    juce::TextButton savePresetButton { "Save" };
    juce::TextButton loadPresetButton { "Load" };

    // State
    int selectedCommandIndex = -1;

    void updateCommandList();
    void updateCommandEditor();
    void applySettings();
    void addCommand();
    void removeCommand();
    void moveCommandUp();
    void moveCommandDown();
    void updateCommandFromEditor();
    juce::String getCommandDescription (const Command& cmd) const;
    void savePreset();
    void loadPreset();
    juce::var settingsToJson() const;
    void jsonToSettings (const juce::var& json);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoopAutomationEditor)
};
