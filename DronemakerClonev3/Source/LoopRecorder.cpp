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
        slot.playPosition = 0;
        slot.hasContent = false;
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
    slots[slot].playPosition = 0;
    slots[slot].hasContent = false;
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
            slot.playPosition = 0;  // Start playback from beginning
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
    slots[slot].playPosition = 0;
    slots[slot].hasContent = false;
}

void LoopRecorder::clearAll()
{
    activeRecordSlot = -1;
    for (auto& slot : slots)
    {
        slot.length = 0;
        slot.playPosition = 0;
        slot.hasContent = false;
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

float LoopRecorder::getLoopMix()
{
    float mix = 0.0f;
    int activeCount = 0;

    for (auto& slot : slots)
    {
        if (slot.hasContent && slot.length > 0)
        {
            // Read from current position
            mix += slot.buffer[slot.playPosition];
            activeCount++;

            // Advance play position (wrap around)
            slot.playPosition++;
            if (slot.playPosition >= slot.length)
                slot.playPosition = 0;
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
