#pragma once
#include <JuceHeader.h>
#include "LoopRecorder.h"

//==============================================================================
// Colour palette for Pi layout — warm coral/amber on deep navy
namespace PiColours
{
    // Backgrounds
    const auto bg          = juce::Colour (0xff08091a);   // deep midnight
    const auto panelBg     = juce::Colour (0xff111425);   // dark navy panel
    const auto panelRaised = juce::Colour (0xff181c30);   // slightly raised surface
    const auto panelBorder = juce::Colour (0xff252a40);   // subtle edge

    // Accents
    const auto accent      = juce::Colour (0xffff6b4a);   // warm coral — primary
    const auto accentBright= juce::Colour (0xffff8866);   // lighter coral for glow
    const auto amber        = juce::Colour (0xffffa033);   // amber — secondary
    const auto amberDim    = juce::Colour (0xff8a6020);   // muted amber

    // States
    const auto recording   = juce::Colour (0xffff2244);   // vivid red
    const auto loopActive  = juce::Colour (0xff1a2210);   // dark warm tint for active
    const auto loopEmpty   = juce::Colour (0xff0e1020);   // very dark for empty

    // Text
    const auto textBright  = juce::Colour (0xfff2f0f5);   // near white
    const auto textNormal  = juce::Colour (0xffb0aec0);   // light grey-lavender
    const auto textDim     = juce::Colour (0xff585670);   // muted
    const auto textValue   = juce::Colour (0xffe0dce8);   // for numeric values

    // Buttons
    const auto buttonBg    = juce::Colour (0xff161a2e);   // dark button surface
    const auto buttonHover = juce::Colour (0xff1e2240);   // hover/press

    // Font helpers — condensed monospace look for synth UI
    inline juce::Font make (float height, bool bold = false)
    {
        return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain)
                               .withName ("Avenir Next Condensed")
                               .withFallbacks ({ "Liberation Sans Narrow", "DejaVu Sans" }));
    }

    inline juce::Font mono (float height, bool bold = false)
    {
        return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain)
                               .withName ("SF Mono")
                               .withFallbacks ({ "Liberation Mono", "DejaVu Sans Mono", "monospace" }));
    }
}

//==============================================================================
// Large loop button with built-in progress bar and waveform hint
class TouchLoopButton : public juce::Component, public juce::Timer
{
public:
    TouchLoopButton (LoopRecorder& recorder, int slotIndex)
        : loopRecorder (recorder), slot (slotIndex)
    {
        startTimerHz (30);
    }

    ~TouchLoopButton() override { stopTimer(); }

    std::function<void(int)> onSelect;
    std::function<void(int)> onToggle;

    void triggerToggle() { if (onToggle) onToggle (slot); }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (1.0f);
        const float corner = 10.0f;

        bool recording = loopRecorder.isRecording() && loopRecorder.getRecordingSlot() == slot;
        bool active = loopRecorder.isSlotActive (slot);
        bool isSelected = (slot == selectedSlot);

        // Background
        juce::Colour bg;
        if (recording)
        {
            float t = (float) juce::Time::getMillisecondCounter() * 0.005f;
            float pulse = 0.15f + 0.12f * std::sin (t);
            bg = PiColours::recording.withAlpha (pulse);
        }
        else if (active)
            bg = juce::Colour (0xff14201a);
        else
            bg = PiColours::loopEmpty;

        if (isDown)
            bg = bg.brighter (0.15f);

        g.setColour (bg);
        g.fillRoundedRectangle (bounds, corner);

        // Progress sweep
        if (active && !recording)
        {
            float progress = loopRecorder.getSlotProgress (slot);
            auto progressBounds = bounds;
            progressBounds.setWidth (bounds.getWidth() * progress);
            g.setColour (PiColours::amber.withAlpha (0.12f));
            g.fillRoundedRectangle (progressBounds, corner);

            float px = bounds.getX() + bounds.getWidth() * progress;
            g.setColour (PiColours::amber.withAlpha (0.8f));
            g.drawLine (px, bounds.getY() + 4, px, bounds.getBottom() - 4, 2.0f);
        }

        // Border
        if (isSelected)
        {
            g.setColour (PiColours::accent);
            g.drawRoundedRectangle (bounds, corner, 2.5f);

            auto barRect = juce::Rectangle<float> (bounds.getX() + 8, bounds.getBottom() - 4,
                                                    bounds.getWidth() - 16, 3.0f);
            g.setColour (PiColours::accent);
            g.fillRoundedRectangle (barRect, 1.5f);
        }
        else
        {
            g.setColour (active ? PiColours::panelBorder.brighter (0.1f) : PiColours::panelBorder);
            g.drawRoundedRectangle (bounds, corner, 1.0f);
        }

        // Number
        if (recording)
        {
            g.setFont (PiColours::make (12.0f, true));
            g.setColour (PiColours::recording);
            g.drawText ("REC", bounds.withTrimmedBottom (bounds.getHeight() * 0.4f),
                        juce::Justification::centredBottom);
            g.setFont (PiColours::make (20.0f, true));
            g.setColour (PiColours::textBright);
            g.drawText (juce::String (slot + 1), bounds.withTrimmedTop (bounds.getHeight() * 0.45f),
                        juce::Justification::centredTop);
        }
        else
        {
            g.setFont (PiColours::make (22.0f, true));
            g.setColour (active ? PiColours::amber : PiColours::textDim);
            g.drawText (juce::String (slot + 1), bounds, juce::Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        isDown = true;
        longPressFired = false;
        longPressTimer = 0;
        repaint();
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        isDown = false;
        if (!longPressFired)
        {
            // Short tap — select
            if (onSelect) onSelect (slot);
        }
        repaint();
    }

    void timerCallback() override
    {
        // Long press detection via timer (fires while held down)
        if (isDown && !longPressFired)
        {
            longPressTimer++;
            // At 30Hz, ~13 ticks = ~430ms
            if (longPressTimer >= 13)
            {
                longPressFired = true;
                if (onToggle) onToggle (slot);
            }
        }
        repaint();
    }

    static int selectedSlot;

private:
    LoopRecorder& loopRecorder;
    int slot;
    bool isDown = false;
    bool longPressFired = false;
    int longPressTimer = 0;
};

//==============================================================================
// Octave selector: row of direct-select buttons
class OctaveSelector : public juce::Component
{
public:
    OctaveSelector()
    {
        for (int i = 0; i < 5; ++i)
        {
            auto* b = &buttons[i];
            const int octave = i - 2;
            juce::String label = (octave == 0) ? "0" : ((octave > 0 ? "+" : "") + juce::String (octave));
            b->setButtonText (label);
            b->onClick = [this, i] {
                selectedIndex = i;
                updateColors();
                if (onChange) onChange (i - 2);
            };
            addAndMakeVisible (b);
        }
        selectedIndex = 2;
        updateColors();
    }

    void setSelectedOctave (int octave)
    {
        selectedIndex = juce::jlimit (0, 4, octave + 2);
        updateColors();
    }

    int getSelectedOctave() const { return selectedIndex - 2; }
    std::function<void(int)> onChange;

    void resized() override
    {
        auto bounds = getLocalBounds();
        int w = bounds.getWidth() / 5;
        for (int i = 0; i < 5; ++i)
            buttons[i].setBounds (bounds.removeFromLeft (w).reduced (1));
    }

private:
    juce::TextButton buttons[5];
    int selectedIndex = 2;

    void updateColors()
    {
        for (int i = 0; i < 5; ++i)
        {
            bool sel = (i == selectedIndex);
            buttons[i].setColour (juce::TextButton::buttonColourId,
                sel ? PiColours::accent.withAlpha (0.3f) : PiColours::buttonBg);
            buttons[i].setColour (juce::TextButton::textColourOffId,
                sel ? PiColours::accent : PiColours::textDim);
        }
    }
};

//==============================================================================
// Expanded detail strip: octave selector, spectrum analyser, waveform with trim
class LoopDetailStrip : public juce::Component, public juce::Timer
{
public:
    LoopDetailStrip (LoopRecorder& recorder)
        : loopRecorder (recorder), fft (fftOrder), window (fftSize, juce::dsp::WindowingFunction<float>::hann)
    {
        addAndMakeVisible (loopLabel);
        loopLabel.setFont (PiColours::make (16.0f, true));
        loopLabel.setColour (juce::Label::textColourId, PiColours::accent);

        addAndMakeVisible (octaveLabel);
        octaveLabel.setText ("OCT", juce::dontSendNotification);
        octaveLabel.setFont (PiColours::make (10.0f, true));
        octaveLabel.setColour (juce::Label::textColourId, PiColours::textDim);

        addAndMakeVisible (octaveSelector);

        autoButton.setButtonText ("Auto");
        autoButton.setColour (juce::TextButton::buttonColourId, PiColours::buttonBg);
        autoButton.setColour (juce::TextButton::textColourOffId, PiColours::textNormal);
        addAndMakeVisible (autoButton);

        clearButton.setButtonText ("Clear");
        clearButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1c1018));
        clearButton.setColour (juce::TextButton::textColourOffId, PiColours::recording.withAlpha (0.8f));
        addAndMakeVisible (clearButton);

        // FFT buffers
        fftInput.resize (fftSize, 0.0f);
        fftOutput.resize (fftSize * 2, 0.0f);
        smoothedBands.resize (numBands, 0.0f);

        startTimerHz (25);
    }

    ~LoopDetailStrip() override { stopTimer(); }

    void setSelectedLoop (int loopIndex)
    {
        currentLoop = loopIndex;
        loopLabel.setText ("Loop " + juce::String (loopIndex + 1), juce::dontSendNotification);
        repaint();
    }

    int getSelectedLoop() const { return currentLoop; }

    // Call from audio thread to feed the spectrum analyser
    void pushVisSample (float sample)
    {
        fftInput[fftWritePos] = sample;
        fftWritePos = (fftWritePos + 1) % fftSize;
        if (fftWritePos == 0)
            fftReady.store (true);
    }

    void timerCallback() override
    {
        if (fftReady.exchange (false))
            computeSpectrum();
        repaint();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (8, 5);

        // Top row: label, octave, auto, clear
        auto topRow = bounds.removeFromTop (28);
        loopLabel.setBounds (topRow.removeFromLeft (65));
        topRow.removeFromLeft (2);
        octaveLabel.setBounds (topRow.removeFromLeft (28).reduced (0, 4));

        clearButton.setBounds (topRow.removeFromRight (52).reduced (1, 2));
        topRow.removeFromRight (3);
        autoButton.setBounds (topRow.removeFromRight (52).reduced (1, 2));
        topRow.removeFromRight (8);
        octaveSelector.setBounds (topRow.reduced (1, 2));

        bounds.removeFromTop (3);

        // Split remaining: top = waveform overview, bottom = spectrum analyser
        int remaining = bounds.getHeight();
        int waveH = remaining * 2 / 5;  // waveform gets less space

        waveformBounds = bounds.removeFromTop (waveH).reduced (2, 1);
        bounds.removeFromTop (3);
        spectrumBounds = bounds.reduced (2, 1);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Panel background
        g.setColour (PiColours::panelBg);
        g.fillRoundedRectangle (bounds, 10.0f);
        g.setColour (PiColours::panelBorder);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 10.0f, 1.0f);

        // === Multi-band spectrum analyser ===
        paintSpectrum (g);

        // === Waveform overview with playhead + trim markers ===
        paintWaveform (g);
    }

    OctaveSelector octaveSelector;
    juce::TextButton autoButton, clearButton;

private:
    LoopRecorder& loopRecorder;
    int currentLoop = 0;
    juce::Rectangle<int> spectrumBounds;
    juce::Rectangle<int> waveformBounds;

    // FFT for spectrum analysis
    static constexpr int fftOrder = 12;           // 2^12 = 4096
    static constexpr int fftSize  = 1 << fftOrder; // 4096
    static constexpr int numBands = 128;           // display bands

    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::vector<float> fftInput;
    std::vector<float> fftOutput;
    std::vector<float> smoothedBands;
    int fftWritePos = 0;
    std::atomic<bool> fftReady { false };

    juce::Label loopLabel, octaveLabel;

    void computeSpectrum()
    {
        // Copy input in order (ring buffer to linear), apply window, run FFT
        std::vector<float> windowed (fftSize);
        for (int i = 0; i < fftSize; ++i)
            windowed[i] = fftInput[(fftWritePos + i) % fftSize];

        window.multiplyWithWindowingTable (windowed.data(), (size_t) fftSize);

        std::fill (fftOutput.begin(), fftOutput.end(), 0.0f);
        for (int i = 0; i < fftSize; ++i)
            fftOutput[i] = windowed[i];

        fft.performFrequencyOnlyForwardTransform (fftOutput.data());

        // Map FFT bins to logarithmic frequency bands using interpolation
        const float minFreq = 25.0f;
        const float maxFreq = 18000.0f;
        const float sampleRate = (float) loopRecorder.getSampleRate();
        const float binWidth = sampleRate / (float) fftSize;
        const int halfSize = fftSize / 2;

        for (int band = 0; band < numBands; ++band)
        {
            // Centre frequency for this band (log scale)
            float t = ((float) band + 0.5f) / (float) numBands;
            float freq = minFreq * std::pow (maxFreq / minFreq, t);

            // Fractional bin position — interpolate between adjacent bins
            float binPos = freq / binWidth;
            int bin0 = juce::jlimit (0, halfSize - 2, (int) binPos);
            float frac = binPos - (float) bin0;
            float mag = fftOutput[bin0] * (1.0f - frac) + fftOutput[bin0 + 1] * frac;

            // Also average a small neighbourhood to reduce noise
            int spread = juce::jmax (1, (int) (binPos * 0.05f));  // ~5% bandwidth
            int lo = juce::jmax (0, bin0 - spread);
            int hi = juce::jmin (halfSize - 1, bin0 + spread);
            float sum = 0.0f;
            for (int b = lo; b <= hi; ++b)
                sum += fftOutput[b];
            float avg = sum / (float) (hi - lo + 1);

            // Blend interpolated with neighbourhood average
            mag = mag * 0.6f + avg * 0.4f;

            // Convert to dB, normalise
            float db = juce::jlimit (-72.0f, 0.0f, 20.0f * std::log10 (mag + 1e-10f));
            float norm = (db + 72.0f) / 72.0f;

            // Smooth: fast attack, slower decay
            float current = smoothedBands[(size_t) band];
            float alpha = (norm > current) ? 0.5f : 0.15f;
            smoothedBands[(size_t) band] = current + alpha * (norm - current);
        }
    }

    void paintSpectrum (juce::Graphics& g) const
    {
        auto sf = spectrumBounds.toFloat();
        g.setColour (juce::Colour (0xff0a0c18));
        g.fillRoundedRectangle (sf, 5.0f);
        g.setColour (PiColours::panelBorder.withAlpha (0.4f));
        g.drawRoundedRectangle (sf.reduced (0.5f), 5.0f, 0.5f);

        if (smoothedBands.empty()) return;

        float barW = sf.getWidth() / (float) numBands;
        float maxH = sf.getHeight() - 2.0f;

        for (int i = 0; i < numBands; ++i)
        {
            float h = smoothedBands[(size_t) i] * maxH;
            if (h < 0.5f) continue;

            float x = sf.getX() + (float) i * barW;
            float y = sf.getBottom() - 1.0f - h;

            // Gradient: amber (low freq) → coral (high freq)
            float t = (float) i / (float) numBands;
            float mag = smoothedBands[(size_t) i];
            auto col = PiColours::amber.interpolatedWith (PiColours::accent, t).withAlpha (0.7f + 0.3f * mag);
            g.setColour (col);
            g.fillRect (x, y, juce::jmax (1.0f, barW - 0.5f), h);
        }
    }

    void paintWaveform (juce::Graphics& g) const
    {
        auto wf = waveformBounds.toFloat();
        g.setColour (juce::Colour (0xff0a0c18));
        g.fillRoundedRectangle (wf, 5.0f);
        g.setColour (PiColours::panelBorder.withAlpha (0.4f));
        g.drawRoundedRectangle (wf.reduced (0.5f), 5.0f, 0.5f);

        if (loopRecorder.isSlotActive (currentLoop))
        {
            int length = loopRecorder.getSlotLength (currentLoop);
            if (length > 0)
            {
                float trimStart = loopRecorder.getSlotTrimStart (currentLoop);
                float trimEnd   = loopRecorder.getSlotTrimEnd (currentLoop);

                // Dim trimmed-out regions
                if (trimStart > 0.001f)
                {
                    auto dimRect = wf.withWidth (wf.getWidth() * trimStart);
                    g.setColour (juce::Colour (0x40000000));
                    g.fillRoundedRectangle (dimRect, 5.0f);
                }
                if (trimEnd < 0.999f)
                {
                    float endX = wf.getX() + wf.getWidth() * trimEnd;
                    auto dimRect = juce::Rectangle<float> (endX, wf.getY(),
                                                            wf.getRight() - endX, wf.getHeight());
                    g.setColour (juce::Colour (0x40000000));
                    g.fillRoundedRectangle (dimRect, 5.0f);
                }

                // Draw waveform
                int width = (int) wf.getWidth();
                float centreY = wf.getCentreY();
                float halfH = wf.getHeight() * 0.4f;

                juce::Path wavePath;
                bool first = true;
                for (int x = 0; x < width; ++x)
                {
                    int sampleIdx = (x * length) / width;
                    float sample = loopRecorder.getSampleAtIndex (currentLoop, sampleIdx);
                    float y = centreY - (sample * halfH);
                    if (first) { wavePath.startNewSubPath (wf.getX() + x, y); first = false; }
                    else wavePath.lineTo (wf.getX() + x, y);
                }
                g.setColour (PiColours::amber.withAlpha (0.6f));
                g.strokePath (wavePath, juce::PathStrokeType (1.0f));

                // Trim markers
                if (trimStart > 0.001f)
                {
                    float tx = wf.getX() + wf.getWidth() * trimStart;
                    g.setColour (PiColours::textDim);
                    g.drawLine (tx, wf.getY() + 2, tx, wf.getBottom() - 2, 1.5f);
                }
                if (trimEnd < 0.999f)
                {
                    float tx = wf.getX() + wf.getWidth() * trimEnd;
                    g.setColour (PiColours::textDim);
                    g.drawLine (tx, wf.getY() + 2, tx, wf.getBottom() - 2, 1.5f);
                }

                // Playhead
                float progress = loopRecorder.getSlotProgress (currentLoop);
                float playNorm = trimStart + progress * (trimEnd - trimStart);
                float px = wf.getX() + wf.getWidth() * playNorm;
                g.setColour (PiColours::accent.withAlpha (0.9f));
                g.drawLine (px, wf.getY() + 2, px, wf.getBottom() - 2, 2.0f);

                // Centre line
                g.setColour (PiColours::panelBorder.withAlpha (0.2f));
                g.drawLine (wf.getX() + 3, centreY, wf.getRight() - 3, centreY, 0.5f);
            }
        }
        else
        {
            g.setColour (PiColours::textDim.withAlpha (0.4f));
            g.setFont (PiColours::make (12.0f));
            g.drawText ("Long press a loop button to record", wf, juce::Justification::centred);
        }
    }
};
