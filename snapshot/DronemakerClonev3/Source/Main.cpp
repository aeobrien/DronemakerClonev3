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
        bool kioskMode = getCommandLineParameterArray().contains ("--kiosk");

        mainWindow.reset (new MainWindow ("DronemakerClone", new MainComponent(), *this, kioskMode));
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
        MainWindow (juce::String name, juce::Component* c, JUCEApplication& a, bool kiosk)
            : DocumentWindow (name,
                              juce::Desktop::getInstance().getDefaultLookAndFeel()
                                  .findColour (ResizableWindow::backgroundColourId),
                              kiosk ? 0 : DocumentWindow::allButtons),
              app (a)
        {
            setContentOwned (c, true);

            if (kiosk)
            {
                // Kiosk mode: no title bar, fullscreen, no decorations
                setUsingNativeTitleBar (false);
                setTitleBarHeight (0);
                setResizable (false, false);
                auto displayArea = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
                setBounds (displayArea);
                setVisible (true);

                // Hide mouse cursor (touch-only)
                setMouseCursor (juce::MouseCursor::NoCursor);

                std::cerr << "Kiosk mode: fullscreen " << displayArea.getWidth()
                          << "x" << displayArea.getHeight() << std::endl;
            }
            else
            {
                // Normal windowed mode
                setUsingNativeTitleBar (true);
                centreWithSize (getWidth(), getHeight());
                setResizable (true, true);
                setVisible (true);
            }
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
