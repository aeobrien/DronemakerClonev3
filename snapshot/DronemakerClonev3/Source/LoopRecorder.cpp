#include "LoopRecorder.h"

LoopRecorder::LoopRecorder()
{
}

void LoopRecorder::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    maxLoopSamples = static_cast<int> (sampleRate * maxLoopSeconds);

    // History buffer: ~6 seconds at sample rate per slot
    int historySize = static_cast<int> (sampleRate * 6.0);

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
        // History buffer for delayed dry playback
        slot.historyBuffer.resize (historySize, 0.0f);
        slot.historyBufferSize = historySize;
        slot.historyWritePos = 0;
        slot.historyPrimed = false;
        slot.historySamplesFed = 0;
        // Calculate per-slot max samples from settings
        slot.maxSamplesForSlot = static_cast<int> (sampleRate * slot.settings.maxRecordLengthSeconds);
        // Initialize cached coefficients
        updateSlotCoefficients (slot);
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
    s.trimStart = 0.0f;
    s.trimEnd = 1.0f;
    s.automationLevel = 1.0f;
    s.executor.cancel();
    // Reset history buffer for new recording
    std::fill (s.historyBuffer.begin(), s.historyBuffer.end(), 0.0f);
    s.historyWritePos = 0;
    s.historyPrimed = false;
    s.historySamplesFed = 0;
    // Recalculate max samples in case settings changed
    s.maxSamplesForSlot = static_cast<int> (currentSampleRate * s.settings.maxRecordLengthSeconds);
    // Pre-load the post-record automation so stopRecording() doesn't allocate
    s.executor.preloadSequence (s.settings.postRecordSequence);
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
            // Activate pre-loaded automation (no allocation on audio thread)
            slot.executor.activatePreloaded (currentSampleRate);
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
    std::fill (s.historyBuffer.begin(), s.historyBuffer.end(), 0.0f);
    s.historyWritePos = 0;
    s.historyPrimed = false;
    s.historySamplesFed = 0;
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
        std::fill (slot.historyBuffer.begin(), slot.historyBuffer.end(), 0.0f);
        slot.historyWritePos = 0;
        slot.historyPrimed = false;
        slot.historySamplesFed = 0;
    }
}

void LoopRecorder::recordSample (float sample)
{
    if (activeRecordSlot < 0 || activeRecordSlot >= numSlots)
    {
        recordingDroneSend = 0.0f;
        return;
    }

    auto& slot = slots[activeRecordSlot];

    // Check against per-slot max length (use global max as hard limit)
    int effectiveMax = std::min (slot.maxSamplesForSlot, maxLoopSamples);

    // Only record if we haven't hit the max length
    if (slot.length < effectiveMax)
    {
        slot.buffer[slot.length] = sample;
        slot.length++;

        // Also feed the history buffer during recording so dry output primes
        // Apply filters and volume using pre-computed coefficients
        slot.hpState += slot.cachedHpCoeff * (sample - slot.hpState);
        float hpOutput = sample - slot.hpState;

        slot.lpState += slot.cachedLpCoeff * (hpOutput - slot.lpState);
        float filtered = slot.lpState;

        float processed = filtered * slot.cachedModulatedVolume;

        // Write to history buffer
        if (slot.historyBufferSize > 0)
        {
            slot.historyBuffer[slot.historyWritePos] = processed;
            slot.historyWritePos = (slot.historyWritePos + 1) % slot.historyBufferSize;
            slot.historySamplesFed++;

            int offsetSamples = static_cast<int> (droneOffsetSeconds * currentSampleRate);
            if (! slot.historyPrimed && slot.historySamplesFed >= offsetSamples)
                slot.historyPrimed = true;
        }

        // Store for visualiser and expose as drone send
        slot.lastFilteredSample = processed;
        recordingDroneSend = processed * slot.wetSend;
    }
    else
    {
        // Max length reached, auto-stop recording
        stopRecording();
    }
}

void LoopRecorder::setSlotWetSend (int slot, float level)
{
    if (slot >= 0 && slot < numSlots)
        slots[slot].wetSend = juce::jlimit (0.0f, 1.0f, level);
}

void LoopRecorder::setSlotDrySend (int slot, float level)
{
    if (slot >= 0 && slot < numSlots)
        slots[slot].drySend = juce::jlimit (0.0f, 1.0f, level);
}

float LoopRecorder::getSlotWetSend (int slot) const
{
    if (slot >= 0 && slot < numSlots) return slots[slot].wetSend;
    return 1.0f;
}

float LoopRecorder::getSlotDrySend (int slot) const
{
    if (slot >= 0 && slot < numSlots) return slots[slot].drySend;
    return 1.0f;
}

void LoopRecorder::setSlotVolume (int slot, float vol)
{
    if (slot >= 0 && slot < numSlots)
    {
        slots[slot].volume = juce::jlimit (0.0f, 2.0f, vol);
        updateSlotCoefficients (slots[slot]);
    }
}

void LoopRecorder::setSlotHighPass (int slot, float freqHz)
{
    if (slot >= 0 && slot < numSlots)
    {
        slots[slot].hpFreq = juce::jlimit (20.0f, 5000.0f, freqHz);
        updateSlotCoefficients (slots[slot]);
    }
}

void LoopRecorder::setSlotLowPass (int slot, float freqHz)
{
    if (slot >= 0 && slot < numSlots)
    {
        slots[slot].lpFreq = juce::jlimit (200.0f, 20000.0f, freqHz);
        updateSlotCoefficients (slots[slot]);
    }
}

void LoopRecorder::setSlotPitchOctave (int slot, int octave)
{
    if (slot >= 0 && slot < numSlots)
    {
        slots[slot].pitchOctave = juce::jlimit (-2, 2, octave);
        updateSlotCoefficients (slots[slot]);
    }
}

void LoopRecorder::updateSlotCoefficients (LoopSlot& slot)
{
    float sr = static_cast<float> (currentSampleRate);

    float modulatedHpFreq = slot.hpFreq * std::pow (2.0f, slot.hpFreqMod * 4.0f);
    float modulatedLpFreq = slot.lpFreq * std::pow (2.0f, slot.lpFreqMod * 4.0f);
    modulatedHpFreq = juce::jlimit (20.0f, 5000.0f, modulatedHpFreq);
    modulatedLpFreq = juce::jlimit (200.0f, 20000.0f, modulatedLpFreq);

    slot.cachedHpCoeff = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * modulatedHpFreq / sr);
    slot.cachedLpCoeff = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * modulatedLpFreq / sr);
    slot.cachedModulatedVolume = juce::jlimit (0.0f, 2.0f, slot.volume + slot.volumeMod);
    slot.cachedPlaybackRate = std::pow (2.0, slot.pitchOctave);
}

void LoopRecorder::setSlotTrimStart (int slot, float normalised)
{
    if (slot >= 0 && slot < numSlots)
    {
        auto& s = slots[slot];
        s.trimStart = juce::jlimit (0.0f, s.trimEnd - 0.01f, normalised);
    }
}

void LoopRecorder::setSlotTrimEnd (int slot, float normalised)
{
    if (slot >= 0 && slot < numSlots)
    {
        auto& s = slots[slot];
        s.trimEnd = juce::jlimit (s.trimStart + 0.01f, 1.0f, normalised);
    }
}

float LoopRecorder::getSlotTrimStart (int slot) const
{
    if (slot >= 0 && slot < numSlots)
        return slots[slot].trimStart;
    return 0.0f;
}

float LoopRecorder::getSlotTrimEnd (int slot) const
{
    if (slot >= 0 && slot < numSlots)
        return slots[slot].trimEnd;
    return 1.0f;
}

int LoopRecorder::getSlotEffectiveLength (int slot) const
{
    if (slot < 0 || slot >= numSlots)
        return 0;
    const auto& s = slots[slot];
    if (s.length <= 0) return 0;
    int startSample = (int) (s.trimStart * s.length);
    int endSample   = (int) (s.trimEnd * s.length);
    return juce::jmax (1, endSample - startSample);
}

void LoopRecorder::setSlotVolumeMod (int slot, float mod)
{
    if (slot >= 0 && slot < numSlots)
    {
        slots[slot].volumeMod = juce::jlimit (-1.0f, 1.0f, mod);
        updateSlotCoefficients (slots[slot]);
    }
}

void LoopRecorder::setSlotFilterHPMod (int slot, float mod)
{
    if (slot >= 0 && slot < numSlots)
    {
        slots[slot].hpFreqMod = juce::jlimit (-1.0f, 1.0f, mod);
        updateSlotCoefficients (slots[slot]);
    }
}

void LoopRecorder::setSlotFilterLPMod (int slot, float mod)
{
    if (slot >= 0 && slot < numSlots)
    {
        slots[slot].lpFreqMod = juce::jlimit (-1.0f, 1.0f, mod);
        updateSlotCoefficients (slots[slot]);
    }
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
            float rawLevel = slot.executor.processSample (currentSampleRate);
            // Map through automation range modifier
            slot.automationLevel = slot.automationRangeMin + rawLevel * (slot.automationRangeMax - slot.automationRangeMin);
        }
        // Always update isPlaying from executor state (it persists after sequence ends)
        slot.isPlaying = slot.executor.isPlaybackEnabled();

        // Only mix if hasContent AND isPlaying
        if (slot.hasContent && slot.isPlaying && slot.length > 0)
        {
            // Trim region
            int startSample = (int) (slot.trimStart * slot.length);
            int endSample   = (int) (slot.trimEnd * slot.length);
            int effectiveLen = juce::jmax (1, endSample - startSample);

            // Read sample with interpolation
            float sample = getInterpolatedSample (slot, slot.playPosition);

            // Apply filters using pre-computed coefficients
            slot.hpState += slot.cachedHpCoeff * (sample - slot.hpState);
            float hpOutput = sample - slot.hpState;

            slot.lpState += slot.cachedLpCoeff * (hpOutput - slot.lpState);
            float filtered = slot.lpState;

            // Apply volume with automation level
            float slotOutput = filtered * slot.cachedModulatedVolume * slot.automationLevel;
            slot.lastFilteredSample = slotOutput;
            mix += slotOutput;
            activeCount++;

            // Advance play position, wrapping within trim region
            slot.playPosition += slot.cachedPlaybackRate;
            if (slot.playPosition >= endSample)
                slot.playPosition = startSample + std::fmod (slot.playPosition - startSample, (double) effectiveLen);
            if (slot.playPosition < startSample)
                slot.playPosition = startSample;
        }
        else
        {
            slot.lastFilteredSample = 0.0f;
        }
    }

    // Normalize to prevent clipping when multiple loops are active
    if (activeCount > 1)
        mix /= std::sqrt (static_cast<float> (activeCount));

    return mix;
}

LoopRecorder::LoopMixOutput LoopRecorder::getLoopMixSplit()
{
    float droneSend = 0.0f;
    float dryOutputL = 0.0f;
    float dryOutputR = 0.0f;
    int droneCount = 0;
    int dryCount = 0;

    int offsetSamples = static_cast<int> (droneOffsetSeconds * currentSampleRate);

    for (auto& slot : slots)
    {
        // Process executor every sample
        if (slot.executor.isRunning())
        {
            float rawLevel = slot.executor.processSample (currentSampleRate);
            slot.automationLevel = slot.automationRangeMin + rawLevel * (slot.automationRangeMax - slot.automationRangeMin);
        }
        slot.isPlaying = slot.executor.isPlaybackEnabled();

        // Only process if hasContent AND isPlaying
        if (! (slot.hasContent && slot.isPlaying && slot.length > 0))
        {
            slot.lastFilteredSample = 0.0f;
            continue;
        }

        auto bounceState = static_cast<LoopSlot::BounceState> (slot.bounceState.load());

        // === Bounced slot: crossfaded dual-voice stereo playback ===
        if (bounceState == LoopSlot::BounceState::Bounced && slot.bouncedLength > 0
            && ! slot.transitioning)
        {
            // Hann crossfade between two voices offset by half the bounced length
            double normA = slot.bounceHeadA / static_cast<double> (slot.bouncedLength);
            double normB = slot.bounceHeadB / static_cast<double> (slot.bouncedLength);
            float gainA = 0.5f * (1.0f + std::cos (2.0f * juce::MathConstants<float>::pi * static_cast<float> (normA)));
            float gainB = 0.5f * (1.0f + std::cos (2.0f * juce::MathConstants<float>::pi * static_cast<float> (normB)));

            // Read from bounced stereo buffers with interpolation
            auto readBounced = [](const std::vector<float>& buf, double pos, int len) -> float {
                int idx0 = static_cast<int> (pos);
                int idx1 = (idx0 + 1) % len;
                float frac = static_cast<float> (pos - idx0);
                return buf[idx0] * (1.0f - frac) + buf[idx1] * frac;
            };

            float sampleAL = readBounced (slot.bouncedL, slot.bounceHeadA, slot.bouncedLength);
            float sampleAR = readBounced (slot.bouncedR, slot.bounceHeadA, slot.bouncedLength);
            float sampleBL = readBounced (slot.bouncedL, slot.bounceHeadB, slot.bouncedLength);
            float sampleBR = readBounced (slot.bouncedR, slot.bounceHeadB, slot.bouncedLength);

            float outL = sampleAL * gainA + sampleBL * gainB;
            float outR = sampleAR * gainA + sampleBR * gainB;

            // Mono mix for lastFilteredSample (visualiser)
            slot.lastFilteredSample = (outL + outR) * 0.5f;

            // Bounced audio goes to dry output only (not to drone)
            if (slot.drySend > 0.001f)
            {
                dryOutputL += outL * slot.drySend;
                dryOutputR += outR * slot.drySend;
                dryCount++;
            }

            // Advance bounce heads
            slot.bounceHeadA += 1.0;
            if (slot.bounceHeadA >= slot.bouncedLength)
                slot.bounceHeadA -= slot.bouncedLength;
            slot.bounceHeadB += 1.0;
            if (slot.bounceHeadB >= slot.bouncedLength)
                slot.bounceHeadB -= slot.bouncedLength;

            continue;
        }

        // === Live slot (normal or transitioning) ===
        {
            // Trim region
            int startSample = (int) (slot.trimStart * slot.length);
            int endSample   = (int) (slot.trimEnd * slot.length);
            int effectiveLen = juce::jmax (1, endSample - startSample);

            // Read sample with interpolation
            float sample = getInterpolatedSample (slot, slot.playPosition);

            // Apply filters using pre-computed coefficients
            slot.hpState += slot.cachedHpCoeff * (sample - slot.hpState);
            float hpOutput = sample - slot.hpState;

            slot.lpState += slot.cachedLpCoeff * (hpOutput - slot.lpState);
            float filtered = slot.lpState;

            // Apply volume with automation level
            float processed = filtered * slot.cachedModulatedVolume * slot.automationLevel;
            slot.lastFilteredSample = processed;

            // Write processed sample to history buffer
            if (slot.historyBufferSize > 0)
            {
                slot.historyBuffer[slot.historyWritePos] = processed;
                slot.historyWritePos = (slot.historyWritePos + 1) % slot.historyBufferSize;
                slot.historySamplesFed++;

                // Check if primed (enough samples written to fill offset)
                if (! slot.historyPrimed && slot.historySamplesFed >= offsetSamples)
                    slot.historyPrimed = true;
            }

            // Wet fade gain (1.0 normally, fades to 0 during transition)
            float effectiveWetSend = slot.wetSend * slot.wetFadeGain;

            // Drone send: immediate processed audio * wet send level
            if (effectiveWetSend > 0.001f)
            {
                droneSend += processed * effectiveWetSend;
                droneCount++;
            }

            // Dry output: read from history buffer N seconds behind
            if (slot.historyPrimed && slot.drySend > 0.001f && slot.historyBufferSize > 0)
            {
                int readPos = slot.historyWritePos - offsetSamples;
                if (readPos < 0)
                    readPos += slot.historyBufferSize;

                float delayedSample = slot.historyBuffer[readPos];
                float monoOut = delayedSample * slot.drySend;
                dryOutputL += monoOut;
                dryOutputR += monoOut;
                dryCount++;
            }

            // During transition: also play bounced audio fading in
            if (slot.transitioning && bounceState == LoopSlot::BounceState::Bounced
                && slot.bouncedLength > 0)
            {
                // Crossfaded dual-voice bounce playback
                double normA = slot.bounceHeadA / static_cast<double> (slot.bouncedLength);
                double normB = slot.bounceHeadB / static_cast<double> (slot.bouncedLength);
                float gainA = 0.5f * (1.0f + std::cos (2.0f * juce::MathConstants<float>::pi * static_cast<float> (normA)));
                float gainB = 0.5f * (1.0f + std::cos (2.0f * juce::MathConstants<float>::pi * static_cast<float> (normB)));

                auto readBounced = [](const std::vector<float>& buf, double pos, int len) -> float {
                    int idx0 = static_cast<int> (pos);
                    int idx1 = (idx0 + 1) % len;
                    float frac = static_cast<float> (pos - idx0);
                    return buf[idx0] * (1.0f - frac) + buf[idx1] * frac;
                };

                float bL = readBounced (slot.bouncedL, slot.bounceHeadA, slot.bouncedLength) * gainA
                         + readBounced (slot.bouncedL, slot.bounceHeadB, slot.bouncedLength) * gainB;
                float bR = readBounced (slot.bouncedR, slot.bounceHeadA, slot.bouncedLength) * gainA
                         + readBounced (slot.bouncedR, slot.bounceHeadB, slot.bouncedLength) * gainB;

                dryOutputL += bL * slot.bounceFadeGain * slot.drySend;
                dryOutputR += bR * slot.bounceFadeGain * slot.drySend;

                // Advance bounce heads
                slot.bounceHeadA += 1.0;
                if (slot.bounceHeadA >= slot.bouncedLength) slot.bounceHeadA -= slot.bouncedLength;
                slot.bounceHeadB += 1.0;
                if (slot.bounceHeadB >= slot.bouncedLength) slot.bounceHeadB -= slot.bouncedLength;

                // Advance fade (~3 seconds)
                float fadeStep = 1.0f / (3.0f * static_cast<float> (currentSampleRate));
                slot.wetFadeGain = juce::jmax (0.0f, slot.wetFadeGain - fadeStep);
                slot.bounceFadeGain = juce::jmin (1.0f, slot.bounceFadeGain + fadeStep);

                // Transition complete
                if (slot.wetFadeGain <= 0.0f && slot.bounceFadeGain >= 1.0f)
                    slot.transitioning = false;
            }

            // Advance play position
            slot.playPosition += slot.cachedPlaybackRate;
            if (slot.playPosition >= endSample)
                slot.playPosition = startSample + std::fmod (slot.playPosition - startSample, (double) effectiveLen);
            if (slot.playPosition < startSample)
                slot.playPosition = startSample;
        }
    }

    // Normalize
    if (droneCount > 1)
        droneSend /= std::sqrt (static_cast<float> (droneCount));
    if (dryCount > 1)
    {
        float norm = 1.0f / std::sqrt (static_cast<float> (dryCount));
        dryOutputL *= norm;
        dryOutputR *= norm;
    }

    return { droneSend, dryOutputL, dryOutputR };
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

    // Report progress within trim region
    int startSample = (int) (s.trimStart * s.length);
    int endSample   = (int) (s.trimEnd * s.length);
    int effectiveLen = juce::jmax (1, endSample - startSample);
    float pos = (float) (s.playPosition - startSample) / (float) effectiveLen;
    return juce::jlimit (0.0f, 1.0f, pos);
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

void LoopRecorder::setSlotAutomationRange (int slot, float rangeMin, float rangeMax)
{
    if (slot >= 0 && slot < numSlots)
    {
        slots[slot].automationRangeMin = juce::jlimit (0.0f, 1.0f, rangeMin);
        slots[slot].automationRangeMax = juce::jlimit (0.0f, 1.0f, rangeMax);
    }
}

float LoopRecorder::getSlotAutomationRangeMin (int slot) const
{
    if (slot >= 0 && slot < numSlots) return slots[slot].automationRangeMin;
    return 0.0f;
}

float LoopRecorder::getSlotAutomationRangeMax (int slot) const
{
    if (slot >= 0 && slot < numSlots) return slots[slot].automationRangeMax;
    return 1.0f;
}

float LoopRecorder::getSlotLastFilteredSample (int slot) const
{
    if (slot < 0 || slot >= numSlots)
        return 0.0f;
    return slots[slot].lastFilteredSample;
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

float LoopRecorder::getSlotVolume (int slot) const
{
    if (slot < 0 || slot >= numSlots) return 1.0f;
    return slots[slot].volume;
}

float LoopRecorder::getSlotHighPass (int slot) const
{
    if (slot < 0 || slot >= numSlots) return 20.0f;
    return slots[slot].hpFreq;
}

float LoopRecorder::getSlotLowPass (int slot) const
{
    if (slot < 0 || slot >= numSlots) return 20000.0f;
    return slots[slot].lpFreq;
}

float LoopRecorder::getSlotVolumeMod (int slot) const
{
    if (slot < 0 || slot >= numSlots) return 0.0f;
    return slots[slot].volumeMod;
}

float LoopRecorder::getSlotHPMod (int slot) const
{
    if (slot < 0 || slot >= numSlots) return 0.0f;
    return slots[slot].hpFreqMod;
}

float LoopRecorder::getSlotLPMod (int slot) const
{
    if (slot < 0 || slot >= numSlots) return 0.0f;
    return slots[slot].lpFreqMod;
}

bool LoopRecorder::isSlotBounced (int slot) const
{
    if (slot < 0 || slot >= numSlots) return false;
    return static_cast<LoopSlot::BounceState> (slots[slot].bounceState.load()) == LoopSlot::BounceState::Bounced;
}

bool LoopRecorder::isSlotBouncing (int slot) const
{
    if (slot < 0 || slot >= numSlots) return false;
    return static_cast<LoopSlot::BounceState> (slots[slot].bounceState.load()) == LoopSlot::BounceState::Bouncing;
}

void LoopRecorder::setSlotBouncing (int slot)
{
    if (slot >= 0 && slot < numSlots)
        slots[slot].bounceState.store (static_cast<int> (LoopSlot::BounceState::Bouncing));
}

void LoopRecorder::clearSlotBounceState (int slot)
{
    if (slot >= 0 && slot < numSlots)
    {
        auto& s = slots[slot];
        s.bounceState.store (static_cast<int> (LoopSlot::BounceState::None));
        s.transitioning = false;
        s.wetFadeGain = 1.0f;
        s.bounceFadeGain = 0.0f;
    }
}

void LoopRecorder::setSlotBounceResult (int slot, std::vector<float>&& left, std::vector<float>&& right, int length)
{
    if (slot < 0 || slot >= numSlots) return;

    auto& s = slots[slot];
    s.bouncedL = std::move (left);
    s.bouncedR = std::move (right);
    s.bouncedLength = length;
    s.bounceState.store (static_cast<int> (LoopSlot::BounceState::Bounced));
}

void LoopRecorder::unbounceSlot (int slot)
{
    if (slot < 0 || slot >= numSlots) return;

    auto& s = slots[slot];
    s.bounceState.store (static_cast<int> (LoopSlot::BounceState::None));
    s.transitioning = false;
    s.wetFadeGain = 1.0f;
    s.bounceFadeGain = 0.0f;
    // Keep bouncedL/R around (could be re-used), they'll be freed on clearSlot
}

void LoopRecorder::startBounceTransition (int slot, double currentPlayPos)
{
    if (slot < 0 || slot >= numSlots) return;

    auto& s = slots[slot];
    if (s.bouncedLength <= 0) return;

    // Align bounce heads with current playback position
    int startSample = static_cast<int> (s.trimStart * s.length);
    double posInLoop = currentPlayPos - startSample;
    if (posInLoop < 0) posInLoop = 0;

    s.bounceHeadA = std::fmod (posInLoop, static_cast<double> (s.bouncedLength));
    s.bounceHeadB = std::fmod (s.bounceHeadA + s.bouncedLength / 2.0, static_cast<double> (s.bouncedLength));

    s.wetFadeGain = 1.0f;
    s.bounceFadeGain = 0.0f;
    s.transitioning = true;
}
