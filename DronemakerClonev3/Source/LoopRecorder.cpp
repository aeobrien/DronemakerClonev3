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
        slot.hpState = 0.0f;
        slot.lpState = 0.0f;
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
    slots[slot].length = 0;
    slots[slot].playPosition = 0.0;
    slots[slot].hasContent = false;
    slots[slot].hpState = 0.0f;
    slots[slot].lpState = 0.0f;
    activeRecordSlot = slot;
}

void LoopRecorder::stopRecording()
{
    if (activeRecordSlot >= 0 && activeRecordSlot < numSlots)
    {
        auto& slot = slots[activeRecordSlot];
        if (slot.length > 0)
        {
            slot.hasContent = true;
            slot.playPosition = 0.0;  // Start playback from beginning
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

    slots[slot].length = 0;
    slots[slot].playPosition = 0.0;
    slots[slot].hasContent = false;
    slots[slot].hpState = 0.0f;
    slots[slot].lpState = 0.0f;
}

void LoopRecorder::clearAll()
{
    activeRecordSlot = -1;
    for (auto& slot : slots)
    {
        slot.length = 0;
        slot.playPosition = 0.0;
        slot.hasContent = false;
        slot.hpState = 0.0f;
        slot.lpState = 0.0f;
    }
}

void LoopRecorder::recordSample (float sample)
{
    if (activeRecordSlot < 0 || activeRecordSlot >= numSlots)
        return;

    auto& slot = slots[activeRecordSlot];

    // Only record if we haven't hit the max length
    if (slot.length < maxLoopSamples)
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
        slots[slot].pitchOctave = juce::jlimit (-1, 1, octave);
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
        if (slot.hasContent && slot.length > 0)
        {
            // Get playback rate based on pitch octave
            double playbackRate = 1.0;
            if (slot.pitchOctave == -1)
                playbackRate = 0.5;   // Octave down = half speed
            else if (slot.pitchOctave == 1)
                playbackRate = 2.0;   // Octave up = double speed

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

            // Apply volume
            mix += filtered * slot.volume;
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
