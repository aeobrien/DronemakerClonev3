#include <JuceHeader.h>
#include "MainComponent.h"

//==============================================================================
class DronemakerCloneApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override       { return "DronemakerClone"; }
    const juce::String getApplicationVersion() override    { return "0.1"; }
    bool moreThanOneInstanceAllowed() override              { return true; }

    void initialise (const juce::String&) override
    {
        mainWindow.reset (new MainWindow ("DronemakerClone", new MainComponent(), *this));
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const juce::String&) override {}

    //==============================================================================
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow (juce::String name, juce::Component* c, JUCEApplication& a)
            : DocumentWindow (name,
                              juce::Desktop::getInstance().getDefaultLookAndFeel()
                                  .findColour (ResizableWindow::backgroundColourId),
                              DocumentWindow::allButtons),
              app (a)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (c, true);
            centreWithSize (getWidth(), getHeight());
            setResizable (true, true);
            setVisible (true);
        }

        void closeButtonPressed() override
        {
            app.systemRequestedQuit();
        }

    private:
        JUCEApplication& app;
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (DronemakerCloneApplication)
