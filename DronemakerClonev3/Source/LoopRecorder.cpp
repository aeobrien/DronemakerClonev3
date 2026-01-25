#include "LoopRecorder.h"

LoopRecorder::LoopRecorder()
{
}

void LoopRecorder::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    maxLoopSamples = static_cast<int> (sampleRate * maxLoopSeconds);

    // Pre-allocate buffers for all slots
    for (auto& slot : slots)
    {
        slot.buffer.resize (maxLoopSamples, 0.0f);
        slot.length = 0;
        slot.playPosition = 0.0;
        slot.hasContent = false;
        slot.isPlaying = false;
        slot.hpState = 0.0f;
        slot.lpState = 0.0f;
        slot.automationLevel = 1.0f;
        // Calculate per-slot max samples from settings
        slot.maxSamplesForSlot = static_cast<int> (sampleRate * slot.settings.maxRecordLengthSeconds);
    }

    activeRecordSlot = -1;
}

void LoopRecorder::reset()
{
    clearAll();
}

void LoopRecorder::startRecording (int slot)
{
    if (slot < 0 || slot >= numSlots)
        return;

    // Stop any existing recording first
    stopRecording();

    // Clear the target slot and start recording
    auto& s = slots[slot];
    s.length = 0;
    s.playPosition = 0.0;
    s.hasContent = false;
    s.isPlaying = false;
    s.hpState = 0.0f;
    s.lpState = 0.0f;
    s.automationLevel = 1.0f;
    s.executor.cancel();
    // Recalculate max samples in case settings changed
    s.maxSamplesForSlot = static_cast<int> (currentSampleRate * s.settings.maxRecordLengthSeconds);
    activeRecordSlot = slot;
}

void LoopRecorder::stopRecording()
{
    if (activeRecordSlot >= 0 && activeRecordSlot < numSlots)
    {
        auto& slot = slots[activeRecordSlot];

        // Check minimum recording length (0.75 seconds)
        float recordedSeconds = static_cast<float> (slot.length) / static_cast<float> (currentSampleRate);

        if (recordedSeconds >= 0.75f)
        {
            // Valid recording - keep it and start automation
            slot.hasContent = true;
            slot.playPosition = 0.0;
            slot.automationLevel = 1.0f;
            // Start the post-record automation sequence
            slot.executor.startSequence (slot.settings.postRecordSequence, currentSampleRate);
        }
        else
        {
            // Recording too short - discard it (keep previous content if any)
            slot.length = 0;
            // Don't change hasContent - keep previous loop if there was one
        }
    }
    activeRecordSlot = -1;
}

void LoopRecorder::clearSlot (int slot)
{
    if (slot < 0 || slot >= numSlots)
        return;

    // If we're recording to this slot, stop first
    if (activeRecordSlot == slot)
        activeRecordSlot = -1;

    auto& s = slots[slot];
    s.length = 0;
    s.playPosition = 0.0;
    s.hasContent = false;
    s.isPlaying = false;
    s.hpState = 0.0f;
    s.lpState = 0.0f;
    s.automationLevel = 1.0f;
    s.executor.cancel();
}

void LoopRecorder::clearAll()
{
    activeRecordSlot = -1;
    for (auto& slot : slots)
    {
        slot.length = 0;
        slot.playPosition = 0.0;
        slot.hasContent = false;
        slot.isPlaying = false;
        slot.hpState = 0.0f;
        slot.lpState = 0.0f;
        slot.automationLevel = 1.0f;
        slot.executor.cancel();
    }
}

void LoopRecorder::recordSample (float sample)
{
    if (activeRecordSlot < 0 || activeRecordSlot >= numSlots)
        return;

    auto& slot = slots[activeRecordSlot];

    // Check against per-slot max length (use global max as hard limit)
    int effectiveMax = std::min (slot.maxSamplesForSlot, maxLoopSamples);

    // Only record if we haven't hit the max length
    if (slot.length < effectiveMax)
    {
        slot.buffer[slot.length] = sample;
        slot.length++;
    }
    else
    {
        // Max length reached, auto-stop recording
        stopRecording();
    }
}

void LoopRecorder::setSlotVolume (int slot, float vol)
{
    if (slot >= 0 && slot < numSlots)
        slots[slot].volume = juce::jlimit (0.0f, 1.0f, vol);
}

void LoopRecorder::setSlotHighPass (int slot, float freqHz)
{
    if (slot >= 0 && slot < numSlots)
        slots[slot].hpFreq = juce::jlimit (20.0f, 5000.0f, freqHz);
}

void LoopRecorder::setSlotLowPass (int slot, float freqHz)
{
    if (slot >= 0 && slot < numSlots)
        slots[slot].lpFreq = juce::jlimit (200.0f, 20000.0f, freqHz);
}

void LoopRecorder::setSlotPitchOctave (int slot, int octave)
{
    if (slot >= 0 && slot < numSlots)
        slots[slot].pitchOctave = juce::jlimit (-2, 2, octave);
}

float LoopRecorder::getInterpolatedSample (const LoopSlot& slot, double position) const
{
    if (slot.length <= 0)
        return 0.0f;

    // Wrap position to loop length
    while (position >= slot.length)
        position -= slot.length;
    while (position < 0)
        position += slot.length;

    // Linear interpolation
    int idx0 = static_cast<int> (position);
    int idx1 = (idx0 + 1) % slot.length;
    float frac = static_cast<float> (position - idx0);

    return slot.buffer[idx0] * (1.0f - frac) + slot.buffer[idx1] * frac;
}

float LoopRecorder::getLoopMix()
{
    float mix = 0.0f;
    int activeCount = 0;

    for (auto& slot : slots)
    {
        // Process executor every sample (even if not playing, to keep automation running)
        if (slot.executor.isRunning())
        {
            slot.automationLevel = slot.executor.processSample (currentSampleRate);
        }
        // Always update isPlaying from executor state (it persists after sequence ends)
        slot.isPlaying = slot.executor.isPlaybackEnabled();

        // Only mix if hasContent AND isPlaying
        if (slot.hasContent && slot.isPlaying && slot.length > 0)
        {
            // Get playback rate based on pitch octave
            // Each octave doubles/halves the playback rate
            double playbackRate = std::pow (2.0, slot.pitchOctave);

            // Read sample with interpolation
            float sample = getInterpolatedSample (slot, slot.playPosition);

            // Apply high-pass filter (simple one-pole)
            // HP: output = input - lowpass(input)
            float hpCoeff = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * slot.hpFreq / static_cast<float> (currentSampleRate));
            slot.hpState += hpCoeff * (sample - slot.hpState);
            float hpOutput = sample - slot.hpState;

            // Apply low-pass filter (simple one-pole)
            float lpCoeff = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * slot.lpFreq / static_cast<float> (currentSampleRate));
            slot.lpState += lpCoeff * (hpOutput - slot.lpState);
            float filtered = slot.lpState;

            // Apply volume AND automation level
            mix += filtered * slot.volume * slot.automationLevel;
            activeCount++;

            // Advance play position based on pitch
            slot.playPosition += playbackRate;
            if (slot.playPosition >= slot.length)
                slot.playPosition -= slot.length;
        }
    }

    // Normalize to prevent clipping when multiple loops are active
    if (activeCount > 1)
        mix /= std::sqrt (static_cast<float> (activeCount));

    return mix;
}

bool LoopRecorder::isSlotActive (int slot) const
{
    if (slot < 0 || slot >= numSlots)
        return false;
    return slots[slot].hasContent;
}

bool LoopRecorder::hasAnyContent() const
{
    for (const auto& slot : slots)
        if (slot.hasContent)
            return true;
    return false;
}

int LoopRecorder::getSlotLength (int slot) const
{
    if (slot < 0 || slot >= numSlots)
        return 0;
    return slots[slot].length;
}

float LoopRecorder::getSlotProgress (int slot) const
{
    if (slot < 0 || slot >= numSlots)
        return 0.0f;

    const auto& s = slots[slot];
    if (!s.hasContent || s.length == 0)
        return 0.0f;

    return static_cast<float> (s.playPosition) / static_cast<float> (s.length);
}

void LoopRecorder::setSlotSettings (int slot, const LoopSettings& settings)
{
    if (slot < 0 || slot >= numSlots)
        return;

    slots[slot].settings = settings;
    // Update max samples for this slot
    slots[slot].maxSamplesForSlot = static_cast<int> (currentSampleRate * settings.maxRecordLengthSeconds);
}

LoopSettings LoopRecorder::getSlotSettings (int slot) const
{
    if (slot < 0 || slot >= numSlots)
        return LoopSettings();

    return slots[slot].settings;
}

void LoopRecorder::setSlotPlaying (int slot, bool playing)
{
    if (slot < 0 || slot >= numSlots)
        return;

    slots[slot].isPlaying = playing;
}

bool LoopRecorder::isSlotPlaying (int slot) const
{
    if (slot < 0 || slot >= numSlots)
        return false;

    return slots[slot].isPlaying;
}

void LoopRecorder::startPreview (int slot)
{
    if (slot < 0 || slot >= numSlots)
        return;

    auto& s = slots[slot];
    // Start the automation sequence for preview (requires content)
    if (s.hasContent)
    {
        s.automationLevel = 1.0f;
        s.executor.startSequence (s.settings.postRecordSequence, currentSampleRate);
    }
}

void LoopRecorder::stopPreview (int slot)
{
    if (slot < 0 || slot >= numSlots)
        return;

    auto& s = slots[slot];
    s.executor.cancel();
    s.isPlaying = false;
    s.automationLevel = 1.0f;
}

bool LoopRecorder::isPreviewRunning (int slot) const
{
    if (slot < 0 || slot >= numSlots)
        return false;

    return slots[slot].executor.isRunning();
}

float LoopRecorder::getSlotAutomationLevel (int slot) const
{
    if (slot < 0 || slot >= numSlots)
        return 1.0f;

    return slots[slot].automationLevel;
}

bool LoopRecorder::slotHasLevelAutomation (int slot) const
{
    if (slot < 0 || slot >= numSlots)
        return false;

    const auto& sequence = slots[slot].settings.postRecordSequence;
    for (const auto& cmd : sequence.commands)
    {
        if (std::holds_alternative<SetLevel> (cmd) || std::holds_alternative<RampLevel> (cmd))
            return true;
    }
    return false;
}

float LoopRecorder::getSampleAtIndex (int slot, int index) const
{
    if (slot < 0 || slot >= numSlots)
        return 0.0f;

    const auto& s = slots[slot];
    if (!s.hasContent || index < 0 || index >= s.length)
        return 0.0f;

    return s.buffer[index];
}
