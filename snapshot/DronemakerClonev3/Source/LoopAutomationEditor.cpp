#include "LoopAutomationEditor.h"

// Custom ListBoxModel for command list
class CommandListModel : public juce::ListBoxModel
{
public:
    CommandListModel (LoopAutomationEditor& owner, Sequence& seq)
        : editor (owner), sequence (seq) {}

    int getNumRows() override
    {
        return static_cast<int> (sequence.commands.size());
    }

    void paintListBoxItem (int rowNumber, juce::Graphics& g,
                          int width, int height, bool rowIsSelected) override
    {
        if (rowNumber < 0 || rowNumber >= (int) sequence.commands.size())
            return;

        if (rowIsSelected)
            g.fillAll (juce::Colour (0xff5dd6c6).withAlpha (0.3f));
        else if (rowNumber % 2 == 0)
            g.fillAll (juce::Colour (0xff1d1f26));
        else
            g.fillAll (juce::Colour (0xff17181d));

        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.setFont (12.0f);

        juce::String text = juce::String (rowNumber + 1) + ". " + getCommandDescription (sequence.commands[rowNumber]);
        g.drawText (text, 8, 0, width - 16, height, juce::Justification::centredLeft);
    }

    void selectedRowsChanged (int lastRowSelected) override
    {
        editor.onCommandSelected (lastRowSelected);
    }

private:
    LoopAutomationEditor& editor;
    Sequence& sequence;

    juce::String getCommandDescription (const Command& cmd) const
    {
        if (std::holds_alternative<StartPlayback> (cmd))
            return "Start Playback";
        else if (std::holds_alternative<StopPlayback> (cmd))
            return "Stop Playback";
        else if (std::holds_alternative<Wait> (cmd))
        {
            auto& w = std::get<Wait> (cmd);
            return "Wait " + juce::String (w.durationSeconds, 2) + "s";
        }
        else if (std::holds_alternative<SetLevel> (cmd))
        {
            auto& s = std::get<SetLevel> (cmd);
            return "Set Level " + juce::String (static_cast<int> (s.targetLevel * 100)) + "%";
        }
        else if (std::holds_alternative<RampLevel> (cmd))
        {
            auto& r = std::get<RampLevel> (cmd);
            juce::String curve = r.curve == RampCurve::EqualPower ? " (EP)" : "";
            return "Ramp to " + juce::String (static_cast<int> (r.targetLevel * 100)) + "% over " +
                   juce::String (r.durationSeconds, 2) + "s" + curve;
        }
        return "Unknown";
    }

    friend class LoopAutomationEditor;
};

LoopAutomationEditor::LoopAutomationEditor (LoopRecorder& recorder, int slotIndex)
    : loopRecorder (recorder), slot (slotIndex)
{
    // Load current settings
    settings = loopRecorder.getSlotSettings (slot);

    // Max length slider
    maxLengthSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    maxLengthSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 20);
    maxLengthSlider.setRange (5.0, 300.0, 1.0);
    maxLengthSlider.setValue (settings.maxRecordLengthSeconds);
    maxLengthSlider.setTextValueSuffix (" s");
    maxLengthSlider.onValueChange = [this] {
        settings.maxRecordLengthSeconds = static_cast<float> (maxLengthSlider.getValue());
    };
    addAndMakeVisible (maxLengthSlider);

    maxLengthLabel.setText ("Max Record Length:", juce::dontSendNotification);
    maxLengthLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (maxLengthLabel);

    // Command list
    commandListModel = std::make_unique<CommandListModel> (*this, settings.postRecordSequence);
    commandList.setModel (commandListModel.get());
    commandList.setColour (juce::ListBox::backgroundColourId, juce::Colour (0xff17181d));
    commandList.setRowHeight (24);
    addAndMakeVisible (commandList);

    // Command type combo
    commandTypeCombo.addItem ("Start Playback", 1);
    commandTypeCombo.addItem ("Stop Playback", 2);
    commandTypeCombo.addItem ("Wait", 3);
    commandTypeCombo.addItem ("Set Level", 4);
    commandTypeCombo.addItem ("Ramp Level", 5);
    commandTypeCombo.onChange = [this] { updateCommandFromEditor(); };
    addAndMakeVisible (commandTypeCombo);

    // Duration slider
    durationSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    durationSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
    durationSlider.setRange (0.1, 60.0, 0.1);
    durationSlider.setValue (1.0);
    durationSlider.setTextValueSuffix (" s");
    durationSlider.onValueChange = [this] { updateCommandFromEditor(); };
    addAndMakeVisible (durationSlider);

    durationLabel.setText ("Duration:", juce::dontSendNotification);
    durationLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (durationLabel);

    // Target level slider
    targetLevelSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    targetLevelSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
    targetLevelSlider.setRange (0.0, 1.0, 0.01);
    targetLevelSlider.setValue (1.0);
    targetLevelSlider.onValueChange = [this] { updateCommandFromEditor(); };
    addAndMakeVisible (targetLevelSlider);

    targetLevelLabel.setText ("Level:", juce::dontSendNotification);
    targetLevelLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (targetLevelLabel);

    // Curve type combo
    curveTypeCombo.addItem ("Linear", 1);
    curveTypeCombo.addItem ("Equal Power", 2);
    curveTypeCombo.setSelectedId (1);
    curveTypeCombo.onChange = [this] { updateCommandFromEditor(); };
    addAndMakeVisible (curveTypeCombo);

    curveTypeLabel.setText ("Curve:", juce::dontSendNotification);
    curveTypeLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (curveTypeLabel);

    // Loop toggle
    loopToggle.setToggleState (settings.postRecordSequence.loopSequence, juce::dontSendNotification);
    loopToggle.onClick = [this] {
        settings.postRecordSequence.loopSequence = loopToggle.getToggleState();
    };
    addAndMakeVisible (loopToggle);

    // Buttons
    addButton.onClick = [this] { addCommand(); };
    addAndMakeVisible (addButton);

    removeButton.onClick = [this] { removeCommand(); };
    addAndMakeVisible (removeButton);

    moveUpButton.onClick = [this] { moveCommandUp(); };
    addAndMakeVisible (moveUpButton);

    moveDownButton.onClick = [this] { moveCommandDown(); };
    addAndMakeVisible (moveDownButton);

    previewButton.onClick = [this] {
        if (loopRecorder.isPreviewRunning (slot))
        {
            loopRecorder.stopPreview (slot);
            previewButton.setButtonText ("Preview");
        }
        else
        {
            // Apply settings before preview
            loopRecorder.setSlotSettings (slot, settings);
            loopRecorder.startPreview (slot);
            previewButton.setButtonText ("Stop");
        }
    };
    addAndMakeVisible (previewButton);

    applyButton.onClick = [this] { applySettings(); };
    addAndMakeVisible (applyButton);

    resetButton.onClick = [this] {
        settings.reset();
        maxLengthSlider.setValue (settings.maxRecordLengthSeconds);
        loopToggle.setToggleState (settings.postRecordSequence.loopSequence, juce::dontSendNotification);
        updateCommandList();
        selectedCommandIndex = -1;
        updateCommandEditor();
    };
    addAndMakeVisible (resetButton);

    savePresetButton.onClick = [this] { savePreset(); };
    addAndMakeVisible (savePresetButton);

    loadPresetButton.onClick = [this] { loadPreset(); };
    addAndMakeVisible (loadPresetButton);

    // Initial state
    updateCommandList();
    updateCommandEditor();

    setSize (450, 400);
    startTimerHz (10);
}

LoopAutomationEditor::~LoopAutomationEditor()
{
    stopTimer();
    // Stop preview if running
    if (loopRecorder.isPreviewRunning (slot))
        loopRecorder.stopPreview (slot);
}

void LoopAutomationEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff17181d));

    // Section titles
    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.setFont (juce::Font (14.0f, juce::Font::bold));
    g.drawText ("Recording Settings", 10, 10, 200, 20, juce::Justification::left);
    g.drawText ("Post-Record Sequence", 10, 70, 200, 20, juce::Justification::left);
    g.drawText ("Command Editor", 240, 70, 200, 20, juce::Justification::left);
}

void LoopAutomationEditor::resized()
{
    auto bounds = getLocalBounds().reduced (10);

    // Top section - max length
    auto topRow = bounds.removeFromTop (50);
    topRow.removeFromTop (25);  // Title space
    maxLengthLabel.setBounds (topRow.removeFromLeft (120));
    maxLengthSlider.setBounds (topRow.reduced (5, 0));

    bounds.removeFromTop (10);

    // Middle section - command list and editor side by side
    auto middleSection = bounds.removeFromTop (220);
    middleSection.removeFromTop (25);  // Title space

    // Left side - command list
    auto leftSide = middleSection.removeFromLeft (220);
    commandList.setBounds (leftSide.removeFromTop (130));

    leftSide.removeFromTop (5);
    auto listButtons = leftSide.removeFromTop (28);
    addButton.setBounds (listButtons.removeFromLeft (50).reduced (2));
    removeButton.setBounds (listButtons.removeFromLeft (60).reduced (2));
    moveUpButton.setBounds (listButtons.removeFromLeft (35).reduced (2));
    moveDownButton.setBounds (listButtons.removeFromLeft (50).reduced (2));

    leftSide.removeFromTop (8);
    loopToggle.setBounds (leftSide.removeFromTop (24));

    middleSection.removeFromLeft (10);

    // Right side - command editor
    auto rightSide = middleSection;
    auto row1 = rightSide.removeFromTop (28);
    commandTypeCombo.setBounds (row1.reduced (2));

    auto row2 = rightSide.removeFromTop (28);
    durationLabel.setBounds (row2.removeFromLeft (60));
    durationSlider.setBounds (row2.reduced (2));

    auto row3 = rightSide.removeFromTop (28);
    targetLevelLabel.setBounds (row3.removeFromLeft (60));
    targetLevelSlider.setBounds (row3.reduced (2));

    auto row4 = rightSide.removeFromTop (28);
    curveTypeLabel.setBounds (row4.removeFromLeft (60));
    curveTypeCombo.setBounds (row4.reduced (2));

    bounds.removeFromTop (10);

    // Bottom section - preview and apply buttons
    auto bottomRow = bounds.removeFromTop (35);
    previewButton.setBounds (bottomRow.removeFromLeft (70).reduced (2));
    bottomRow.removeFromLeft (5);
    resetButton.setBounds (bottomRow.removeFromLeft (60).reduced (2));
    bottomRow.removeFromLeft (5);
    savePresetButton.setBounds (bottomRow.removeFromLeft (55).reduced (2));
    loadPresetButton.setBounds (bottomRow.removeFromLeft (55).reduced (2));
    applyButton.setBounds (bottomRow.removeFromRight (70).reduced (2));
}

void LoopAutomationEditor::timerCallback()
{
    // Update preview button state
    if (loopRecorder.isPreviewRunning (slot))
    {
        previewButton.setButtonText ("Stop");
        previewButton.setColour (juce::TextButton::buttonColourId, juce::Colours::orange);
    }
    else
    {
        previewButton.setButtonText ("Preview");
        previewButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2a2b31));
    }
}

void LoopAutomationEditor::updateCommandList()
{
    commandList.updateContent();
    commandList.repaint();
}

void LoopAutomationEditor::updateCommandEditor()
{
    bool hasSelection = selectedCommandIndex >= 0 &&
                       selectedCommandIndex < (int) settings.postRecordSequence.commands.size();

    commandTypeCombo.setEnabled (hasSelection);

    // Hide all optional controls by default
    durationSlider.setVisible (false);
    durationLabel.setVisible (false);
    targetLevelSlider.setVisible (false);
    targetLevelLabel.setVisible (false);
    curveTypeCombo.setVisible (false);
    curveTypeLabel.setVisible (false);

    if (!hasSelection)
    {
        commandTypeCombo.setSelectedId (0, juce::dontSendNotification);
        return;
    }

    const auto& cmd = settings.postRecordSequence.commands[selectedCommandIndex];

    if (std::holds_alternative<StartPlayback> (cmd))
    {
        commandTypeCombo.setSelectedId (1, juce::dontSendNotification);
        // No additional controls needed
    }
    else if (std::holds_alternative<StopPlayback> (cmd))
    {
        commandTypeCombo.setSelectedId (2, juce::dontSendNotification);
        // No additional controls needed
    }
    else if (std::holds_alternative<Wait> (cmd))
    {
        commandTypeCombo.setSelectedId (3, juce::dontSendNotification);
        auto& w = std::get<Wait> (cmd);
        durationSlider.setValue (w.durationSeconds, juce::dontSendNotification);
        durationSlider.setVisible (true);
        durationLabel.setVisible (true);
    }
    else if (std::holds_alternative<SetLevel> (cmd))
    {
        commandTypeCombo.setSelectedId (4, juce::dontSendNotification);
        auto& s = std::get<SetLevel> (cmd);
        targetLevelSlider.setValue (s.targetLevel, juce::dontSendNotification);
        targetLevelSlider.setVisible (true);
        targetLevelLabel.setVisible (true);
    }
    else if (std::holds_alternative<RampLevel> (cmd))
    {
        commandTypeCombo.setSelectedId (5, juce::dontSendNotification);
        auto& r = std::get<RampLevel> (cmd);
        durationSlider.setValue (r.durationSeconds, juce::dontSendNotification);
        targetLevelSlider.setValue (r.targetLevel, juce::dontSendNotification);
        curveTypeCombo.setSelectedId (r.curve == RampCurve::EqualPower ? 2 : 1, juce::dontSendNotification);
        durationSlider.setVisible (true);
        durationLabel.setVisible (true);
        targetLevelSlider.setVisible (true);
        targetLevelLabel.setVisible (true);
        curveTypeCombo.setVisible (true);
        curveTypeLabel.setVisible (true);
    }
}

void LoopAutomationEditor::applySettings()
{
    loopRecorder.setSlotSettings (slot, settings);

    // Close the dialog by finding and closing the parent window
    if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
        dw->closeButtonPressed();
}

void LoopAutomationEditor::addCommand()
{
    // Add a new StartPlayback command at the end
    settings.postRecordSequence.commands.push_back (StartPlayback{});
    updateCommandList();

    // Select the new command
    selectedCommandIndex = static_cast<int> (settings.postRecordSequence.commands.size()) - 1;
    commandList.selectRow (selectedCommandIndex);
    updateCommandEditor();
}

void LoopAutomationEditor::removeCommand()
{
    if (selectedCommandIndex < 0 ||
        selectedCommandIndex >= (int) settings.postRecordSequence.commands.size())
        return;

    // Don't allow removing the last command
    if (settings.postRecordSequence.commands.size() <= 1)
        return;

    settings.postRecordSequence.commands.erase (
        settings.postRecordSequence.commands.begin() + selectedCommandIndex);

    if (selectedCommandIndex >= (int) settings.postRecordSequence.commands.size())
        selectedCommandIndex = static_cast<int> (settings.postRecordSequence.commands.size()) - 1;

    updateCommandList();
    commandList.selectRow (selectedCommandIndex);
    updateCommandEditor();
}

void LoopAutomationEditor::moveCommandUp()
{
    if (selectedCommandIndex <= 0 ||
        selectedCommandIndex >= (int) settings.postRecordSequence.commands.size())
        return;

    std::swap (settings.postRecordSequence.commands[selectedCommandIndex],
               settings.postRecordSequence.commands[selectedCommandIndex - 1]);
    selectedCommandIndex--;

    updateCommandList();
    commandList.selectRow (selectedCommandIndex);
}

void LoopAutomationEditor::moveCommandDown()
{
    if (selectedCommandIndex < 0 ||
        selectedCommandIndex >= (int) settings.postRecordSequence.commands.size() - 1)
        return;

    std::swap (settings.postRecordSequence.commands[selectedCommandIndex],
               settings.postRecordSequence.commands[selectedCommandIndex + 1]);
    selectedCommandIndex++;

    updateCommandList();
    commandList.selectRow (selectedCommandIndex);
}

void LoopAutomationEditor::onCommandSelected (int index)
{
    selectedCommandIndex = index;
    updateCommandEditor();
}

void LoopAutomationEditor::updateCommandFromEditor()
{
    if (selectedCommandIndex < 0 ||
        selectedCommandIndex >= (int) settings.postRecordSequence.commands.size())
        return;

    int typeId = commandTypeCombo.getSelectedId();
    Command newCmd;

    // Hide all optional controls first
    durationSlider.setVisible (false);
    durationLabel.setVisible (false);
    targetLevelSlider.setVisible (false);
    targetLevelLabel.setVisible (false);
    curveTypeCombo.setVisible (false);
    curveTypeLabel.setVisible (false);

    switch (typeId)
    {
        case 1: // StartPlayback
            newCmd = StartPlayback{};
            break;

        case 2: // StopPlayback
            newCmd = StopPlayback{};
            break;

        case 3: // Wait
            newCmd = Wait { static_cast<float> (durationSlider.getValue()) };
            durationSlider.setVisible (true);
            durationLabel.setVisible (true);
            break;

        case 4: // SetLevel
            newCmd = SetLevel { static_cast<float> (targetLevelSlider.getValue()) };
            targetLevelSlider.setVisible (true);
            targetLevelLabel.setVisible (true);
            break;

        case 5: // RampLevel
            newCmd = RampLevel {
                static_cast<float> (durationSlider.getValue()),
                static_cast<float> (targetLevelSlider.getValue()),
                curveTypeCombo.getSelectedId() == 2 ? RampCurve::EqualPower : RampCurve::Linear
            };
            durationSlider.setVisible (true);
            durationLabel.setVisible (true);
            targetLevelSlider.setVisible (true);
            targetLevelLabel.setVisible (true);
            curveTypeCombo.setVisible (true);
            curveTypeLabel.setVisible (true);
            break;

        default:
            return;
    }

    settings.postRecordSequence.commands[selectedCommandIndex] = newCmd;
    updateCommandList();
}

juce::String LoopAutomationEditor::getCommandDescription (const Command& cmd) const
{
    if (std::holds_alternative<StartPlayback> (cmd))
        return "Start Playback";
    else if (std::holds_alternative<StopPlayback> (cmd))
        return "Stop Playback";
    else if (std::holds_alternative<Wait> (cmd))
    {
        auto& w = std::get<Wait> (cmd);
        return "Wait " + juce::String (w.durationSeconds, 2) + "s";
    }
    else if (std::holds_alternative<SetLevel> (cmd))
    {
        auto& s = std::get<SetLevel> (cmd);
        return "Set Level " + juce::String (static_cast<int> (s.targetLevel * 100)) + "%";
    }
    else if (std::holds_alternative<RampLevel> (cmd))
    {
        auto& r = std::get<RampLevel> (cmd);
        return "Ramp to " + juce::String (static_cast<int> (r.targetLevel * 100)) + "% over " +
               juce::String (r.durationSeconds, 2) + "s";
    }
    return "Unknown";
}

juce::var LoopAutomationEditor::settingsToJson() const
{
    auto* obj = new juce::DynamicObject();

    obj->setProperty ("maxRecordLength", settings.maxRecordLengthSeconds);
    obj->setProperty ("loopSequence", settings.postRecordSequence.loopSequence);

    juce::Array<juce::var> commandsArray;
    for (const auto& cmd : settings.postRecordSequence.commands)
    {
        auto* cmdObj = new juce::DynamicObject();

        if (std::holds_alternative<StartPlayback> (cmd))
        {
            cmdObj->setProperty ("type", "StartPlayback");
        }
        else if (std::holds_alternative<StopPlayback> (cmd))
        {
            cmdObj->setProperty ("type", "StopPlayback");
        }
        else if (std::holds_alternative<Wait> (cmd))
        {
            auto& w = std::get<Wait> (cmd);
            cmdObj->setProperty ("type", "Wait");
            cmdObj->setProperty ("duration", w.durationSeconds);
        }
        else if (std::holds_alternative<SetLevel> (cmd))
        {
            auto& s = std::get<SetLevel> (cmd);
            cmdObj->setProperty ("type", "SetLevel");
            cmdObj->setProperty ("level", s.targetLevel);
        }
        else if (std::holds_alternative<RampLevel> (cmd))
        {
            auto& r = std::get<RampLevel> (cmd);
            cmdObj->setProperty ("type", "RampLevel");
            cmdObj->setProperty ("duration", r.durationSeconds);
            cmdObj->setProperty ("level", r.targetLevel);
            cmdObj->setProperty ("curve", r.curve == RampCurve::EqualPower ? "EqualPower" : "Linear");
        }

        commandsArray.add (juce::var (cmdObj));
    }

    obj->setProperty ("commands", commandsArray);
    return juce::var (obj);
}

void LoopAutomationEditor::jsonToSettings (const juce::var& json)
{
    if (auto* obj = json.getDynamicObject())
    {
        if (obj->hasProperty ("maxRecordLength"))
            settings.maxRecordLengthSeconds = static_cast<float> ((double) obj->getProperty ("maxRecordLength"));

        if (obj->hasProperty ("loopSequence"))
            settings.postRecordSequence.loopSequence = (bool) obj->getProperty ("loopSequence");

        if (obj->hasProperty ("commands"))
        {
            settings.postRecordSequence.commands.clear();
            auto* commands = obj->getProperty ("commands").getArray();

            if (commands != nullptr)
            {
                for (const auto& cmdVar : *commands)
                {
                    if (auto* cmdObj = cmdVar.getDynamicObject())
                    {
                        juce::String type = cmdObj->getProperty ("type").toString();

                        if (type == "StartPlayback")
                        {
                            settings.postRecordSequence.commands.push_back (StartPlayback{});
                        }
                        else if (type == "StopPlayback")
                        {
                            settings.postRecordSequence.commands.push_back (StopPlayback{});
                        }
                        else if (type == "Wait")
                        {
                            float duration = static_cast<float> ((double) cmdObj->getProperty ("duration"));
                            settings.postRecordSequence.commands.push_back (Wait { duration });
                        }
                        else if (type == "SetLevel")
                        {
                            float level = static_cast<float> ((double) cmdObj->getProperty ("level"));
                            settings.postRecordSequence.commands.push_back (SetLevel { level });
                        }
                        else if (type == "RampLevel")
                        {
                            float duration = static_cast<float> ((double) cmdObj->getProperty ("duration"));
                            float level = static_cast<float> ((double) cmdObj->getProperty ("level"));
                            juce::String curveStr = cmdObj->getProperty ("curve").toString();
                            RampCurve curve = curveStr == "EqualPower" ? RampCurve::EqualPower : RampCurve::Linear;
                            settings.postRecordSequence.commands.push_back (RampLevel { duration, level, curve });
                        }
                    }
                }
            }
        }
    }
}

void LoopAutomationEditor::savePreset()
{
    auto chooser = std::make_unique<juce::FileChooser> (
        "Save Automation Preset",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
        "*.json");

    auto chooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync (chooserFlags, [this, chooserPtr = chooser.release()] (const juce::FileChooser& fc)
    {
        std::unique_ptr<juce::FileChooser> chooserGuard (chooserPtr);

        auto file = fc.getResult();
        if (file == juce::File{})
            return;

        // Ensure .json extension
        if (!file.hasFileExtension (".json"))
            file = file.withFileExtension (".json");

        auto json = settingsToJson();
        auto jsonStr = juce::JSON::toString (json);

        file.replaceWithText (jsonStr);
    });
}

void LoopAutomationEditor::loadPreset()
{
    auto chooser = std::make_unique<juce::FileChooser> (
        "Load Automation Preset",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
        "*.json");

    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync (chooserFlags, [this, chooserPtr = chooser.release()] (const juce::FileChooser& fc)
    {
        std::unique_ptr<juce::FileChooser> chooserGuard (chooserPtr);

        auto file = fc.getResult();
        if (file == juce::File{} || !file.existsAsFile())
            return;

        auto jsonStr = file.loadFileAsString();
        auto json = juce::JSON::parse (jsonStr);

        if (json.isVoid())
            return;

        jsonToSettings (json);

        // Update UI
        maxLengthSlider.setValue (settings.maxRecordLengthSeconds);
        loopToggle.setToggleState (settings.postRecordSequence.loopSequence, juce::dontSendNotification);
        updateCommandList();
        selectedCommandIndex = settings.postRecordSequence.commands.empty() ? -1 : 0;
        if (selectedCommandIndex >= 0)
            commandList.selectRow (selectedCommandIndex);
        updateCommandEditor();
    });
}
