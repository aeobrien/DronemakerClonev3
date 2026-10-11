#include "ModulationManager.h"
#include "../LoopRecorder.h"
#include "../Effects/EffectsChain.h"
#include "../Effects/FilterEffect.h"
#include "../Effects/DelayEffect.h"
#include "../Effects/DistortionEffect.h"
#include "../Effects/TapeEffect.h"
#include "../Effects/TremoloEffect.h"

ModulationManager::ModulationManager()
{
    // Initialize with different default rates for variety
    lfos[0].setRate (0.5f);
    lfos[1].setRate (1.0f);
    lfos[2].setRate (2.0f);
    lfos[3].setRate (0.25f);

    // Different waveforms by default
    lfos[0].setWaveform (LFOSource::Waveform::Sine);
    lfos[1].setWaveform (LFOSource::Waveform::Triangle);
    lfos[2].setWaveform (LFOSource::Waveform::Sine);
    lfos[3].setWaveform (LFOSource::Waveform::SawUp);

    // Set default randomizer rates
    randomizers[0].setRate (0.3f);
    randomizers[0].setSmoothness (0.7f);
    randomizers[1].setRate (0.8f);
    randomizers[1].setSmoothness (0.4f);
}

void ModulationManager::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    for (auto& lfo : lfos)
        lfo.prepareToPlay (sampleRate, samplesPerBlock);

    envelopeFollower.prepareToPlay (sampleRate, samplesPerBlock);

    for (auto& randomizer : randomizers)
        randomizer.prepareToPlay (sampleRate, samplesPerBlock);
}

void ModulationManager::reset()
{
    for (auto& lfo : lfos)
        lfo.reset();

    envelopeFollower.reset();

    for (auto& randomizer : randomizers)
        randomizer.reset();

    accumulator.reset();
}

void ModulationManager::processSample (float inputL, float inputR)
{
    // Reset accumulator for this sample
    accumulator.reset();

    // Process all sources and accumulate their contributions
    for (auto& lfo : lfos)
    {
        if (lfo.isEnabled())
        {
            lfo.processSample();
            // LFO outputs -1 to +1, map to 0 to 1
            float value = (lfo.getCurrentValue() + 1.0f) * 0.5f;

            for (int t = 0; t < ModulationSource::maxTargets; ++t)
            {
                const auto& target = lfo.getTarget (t);
                if (target.isActive())
                    accumulateTarget (target, value);
            }
        }
    }

    // Envelope follower needs audio input
    if (envelopeFollower.isEnabled())
    {
        envelopeFollower.processSample (inputL, inputR);
        // Envelope outputs 0 to 1 already
        float value = envelopeFollower.getCurrentValue();

        for (int t = 0; t < ModulationSource::maxTargets; ++t)
        {
            const auto& target = envelopeFollower.getTarget (t);
            if (target.isActive())
                accumulateTarget (target, value);
        }
    }

    for (auto& randomizer : randomizers)
    {
        if (randomizer.isEnabled())
        {
            randomizer.processSample();
            // Randomizer outputs -1 to +1, map to 0 to 1
            float value = (randomizer.getCurrentValue() + 1.0f) * 0.5f;

            for (int t = 0; t < ModulationSource::maxTargets; ++t)
            {
                const auto& target = randomizer.getTarget (t);
                if (target.isActive())
                    accumulateTarget (target, value);
            }
        }
    }
}

void ModulationManager::accumulateTarget (const ModulationTarget& target, float normalizedValue)
{
    // normalizedValue is 0-1, map to rangeMin-rangeMax
    float modValue = target.rangeMin + normalizedValue * (target.rangeMax - target.rangeMin);

    switch (target.type)
    {
        case ModulationTarget::Type::LoopVolume:
            if (target.index >= 0 && target.index < 8)
            {
                accumulator.loopVolumes[target.index] = modValue;
                accumulator.loopVolumeActive[target.index] = true;
            }
            break;

        case ModulationTarget::Type::LoopFilterHP:
            if (target.index >= 0 && target.index < 8)
            {
                accumulator.loopFilterHP[target.index] = modValue;
                accumulator.loopFilterHPActive[target.index] = true;
            }
            break;

        case ModulationTarget::Type::LoopFilterLP:
            if (target.index >= 0 && target.index < 8)
            {
                accumulator.loopFilterLP[target.index] = modValue;
                accumulator.loopFilterLPActive[target.index] = true;
            }
            break;

        case ModulationTarget::Type::DelayTime:
            accumulator.delayTime = modValue;
            accumulator.delayTimeActive = true;
            break;

        case ModulationTarget::Type::DelayFeedback:
            accumulator.delayFeedback = modValue;
            accumulator.delayFeedbackActive = true;
            break;

        case ModulationTarget::Type::DelayDryWet:
            accumulator.delayDryWet = modValue;
            accumulator.delayDryWetActive = true;
            break;

        case ModulationTarget::Type::DistortionDrive:
            accumulator.distortionDrive = modValue;
            accumulator.distortionDriveActive = true;
            break;

        case ModulationTarget::Type::DistortionTone:
            accumulator.distortionTone = modValue;
            accumulator.distortionToneActive = true;
            break;

        case ModulationTarget::Type::DistortionDryWet:
            accumulator.distortionDryWet = modValue;
            accumulator.distortionDryWetActive = true;
            break;

        case ModulationTarget::Type::TapeSaturation:
            accumulator.tapeSaturation = modValue;
            accumulator.tapeSaturationActive = true;
            break;

        case ModulationTarget::Type::TapeBias:
            accumulator.tapeBias = modValue;
            accumulator.tapeBiasActive = true;
            break;

        case ModulationTarget::Type::TapeWowDepth:
            accumulator.tapeWowDepth = modValue;
            accumulator.tapeWowDepthActive = true;
            break;

        case ModulationTarget::Type::TapeFlutterDepth:
            accumulator.tapeFlutterDepth = modValue;
            accumulator.tapeFlutterDepthActive = true;
            break;

        case ModulationTarget::Type::TapeDryWet:
            accumulator.tapeDryWet = modValue;
            accumulator.tapeDryWetActive = true;
            break;

        case ModulationTarget::Type::FilterHP:
            accumulator.filterHP = modValue;
            accumulator.filterHPActive = true;
            break;

        case ModulationTarget::Type::FilterLP:
            accumulator.filterLP = modValue;
            accumulator.filterLPActive = true;
            break;

        case ModulationTarget::Type::FilterHarmonicIntensity:
            accumulator.filterHarmonicIntensity = modValue;
            accumulator.filterHarmonicIntensityActive = true;
            break;

        case ModulationTarget::Type::TremoloRate:
            accumulator.tremoloRate = modValue;
            accumulator.tremoloRateActive = true;
            break;

        case ModulationTarget::Type::TremoloDepth:
            accumulator.tremoloDepth = modValue;
            accumulator.tremoloDepthActive = true;
            break;

        case ModulationTarget::Type::MasterVolume:
            accumulator.masterVolume = modValue;
            accumulator.masterVolumeActive = true;
            break;

        case ModulationTarget::Type::LoopMix:
            accumulator.loopMix = modValue;
            accumulator.loopMixActive = true;
            break;

        case ModulationTarget::Type::None:
        default:
            break;
    }
}

void ModulationManager::applyModulation (LoopRecorder& loopRecorder, EffectsChain& effectsChain,
                                         float& loopMixMod, float& masterVolumeMod)
{
    // Apply loop modulation - set volume/filter directly if modulated
    for (int i = 0; i < 8; ++i)
    {
        if (accumulator.loopVolumeActive[i])
            loopRecorder.setSlotVolume (i, accumulator.loopVolumes[i]);

        if (accumulator.loopFilterHPActive[i])
            loopRecorder.setSlotHighPass (i, accumulator.loopFilterHP[i]);

        if (accumulator.loopFilterLPActive[i])
            loopRecorder.setSlotLowPass (i, accumulator.loopFilterLP[i]);
    }

    // Apply effect modulation
    auto* delay = effectsChain.getDelay();
    if (delay)
    {
        if (accumulator.delayTimeActive)
            delay->setDelayTimeMs (accumulator.delayTime);
        if (accumulator.delayFeedbackActive)
            delay->setFeedback (accumulator.delayFeedback);
        if (accumulator.delayDryWetActive)
            delay->setDryWet (accumulator.delayDryWet);
    }

    auto* distortion = effectsChain.getDistortion();
    if (distortion)
    {
        if (accumulator.distortionDriveActive)
            distortion->setDrive (accumulator.distortionDrive);
        if (accumulator.distortionToneActive)
            distortion->setTone (accumulator.distortionTone);
        if (accumulator.distortionDryWetActive)
            distortion->setDryWet (accumulator.distortionDryWet);
    }

    auto* tape = effectsChain.getTape();
    if (tape)
    {
        if (accumulator.tapeSaturationActive)
            tape->setSaturation (accumulator.tapeSaturation);
        if (accumulator.tapeBiasActive)
            tape->setBias (accumulator.tapeBias);
        if (accumulator.tapeWowDepthActive)
            tape->setWowDepth (accumulator.tapeWowDepth);
        if (accumulator.tapeFlutterDepthActive)
            tape->setFlutterDepth (accumulator.tapeFlutterDepth);
        if (accumulator.tapeDryWetActive)
            tape->setDryWet (accumulator.tapeDryWet);
    }

    auto* filter = effectsChain.getFilter();
    if (filter)
    {
        if (accumulator.filterHPActive)
            filter->setHighPassFreq (accumulator.filterHP);
        if (accumulator.filterLPActive)
            filter->setLowPassFreq (accumulator.filterLP);
        if (accumulator.filterHarmonicIntensityActive)
            filter->setHarmonicIntensity (accumulator.filterHarmonicIntensity);
    }

    auto* tremolo = effectsChain.getTremolo();
    if (tremolo)
    {
        if (accumulator.tremoloRateActive)
            tremolo->setRate (accumulator.tremoloRate);
        if (accumulator.tremoloDepthActive)
            tremolo->setDepth (accumulator.tremoloDepth);
    }

    // Return master modulation values (caller applies these)
    loopMixMod = accumulator.loopMixActive ? accumulator.loopMix : -1.0f;  // -1 = not modulated
    masterVolumeMod = accumulator.masterVolumeActive ? accumulator.masterVolume : -1.0f;
}

ModulationSource* ModulationManager::getSource (int index)
{
    if (index < numLFOs)
        return &lfos[index];

    index -= numLFOs;
    if (index < numEnvelopes)
        return &envelopeFollower;

    index -= numEnvelopes;
    if (index < numRandomizers)
        return &randomizers[index];

    return nullptr;
}

juce::String ModulationManager::getTargetTypeName (ModulationTarget::Type type)
{
    switch (type)
    {
        case ModulationTarget::Type::None:                    return "None";
        case ModulationTarget::Type::LoopVolume:              return "Loop Volume";
        case ModulationTarget::Type::LoopFilterHP:            return "Loop HP Filter";
        case ModulationTarget::Type::LoopFilterLP:            return "Loop LP Filter";
        case ModulationTarget::Type::DelayTime:               return "Delay Time";
        case ModulationTarget::Type::DelayFeedback:           return "Delay Feedback";
        case ModulationTarget::Type::DelayDryWet:             return "Delay Dry/Wet";
        case ModulationTarget::Type::DistortionDrive:         return "Distortion Drive";
        case ModulationTarget::Type::DistortionTone:          return "Distortion Tone";
        case ModulationTarget::Type::DistortionDryWet:        return "Distortion Dry/Wet";
        case ModulationTarget::Type::TapeSaturation:          return "Tape Saturation";
        case ModulationTarget::Type::TapeBias:                return "Tape Bias";
        case ModulationTarget::Type::TapeWowDepth:            return "Tape Wow";
        case ModulationTarget::Type::TapeFlutterDepth:        return "Tape Flutter";
        case ModulationTarget::Type::TapeDryWet:              return "Tape Dry/Wet";
        case ModulationTarget::Type::FilterHP:                return "Filter HP";
        case ModulationTarget::Type::FilterLP:                return "Filter LP";
        case ModulationTarget::Type::FilterHarmonicIntensity: return "Harmonic Intensity";
        case ModulationTarget::Type::TremoloRate:             return "Tremolo Rate";
        case ModulationTarget::Type::TremoloDepth:            return "Tremolo Depth";
        case ModulationTarget::Type::MasterVolume:            return "Master Volume";
        case ModulationTarget::Type::LoopMix:                 return "Loop Mix";
        default:                                              return "Unknown";
    }
}

bool ModulationManager::isTargetModulated (ModulationTarget::Type type, int index) const
{
    switch (type)
    {
        case ModulationTarget::Type::LoopVolume:
            return index >= 0 && index < 8 && accumulator.loopVolumeActive[index];
        case ModulationTarget::Type::LoopFilterHP:
            return index >= 0 && index < 8 && accumulator.loopFilterHPActive[index];
        case ModulationTarget::Type::LoopFilterLP:
            return index >= 0 && index < 8 && accumulator.loopFilterLPActive[index];
        case ModulationTarget::Type::DelayTime:
            return accumulator.delayTimeActive;
        case ModulationTarget::Type::DelayFeedback:
            return accumulator.delayFeedbackActive;
        case ModulationTarget::Type::DelayDryWet:
            return accumulator.delayDryWetActive;
        case ModulationTarget::Type::DistortionDrive:
            return accumulator.distortionDriveActive;
        case ModulationTarget::Type::DistortionTone:
            return accumulator.distortionToneActive;
        case ModulationTarget::Type::DistortionDryWet:
            return accumulator.distortionDryWetActive;
        case ModulationTarget::Type::TapeSaturation:
            return accumulator.tapeSaturationActive;
        case ModulationTarget::Type::TapeBias:
            return accumulator.tapeBiasActive;
        case ModulationTarget::Type::TapeWowDepth:
            return accumulator.tapeWowDepthActive;
        case ModulationTarget::Type::TapeFlutterDepth:
            return accumulator.tapeFlutterDepthActive;
        case ModulationTarget::Type::TapeDryWet:
            return accumulator.tapeDryWetActive;
        case ModulationTarget::Type::FilterHP:
            return accumulator.filterHPActive;
        case ModulationTarget::Type::FilterLP:
            return accumulator.filterLPActive;
        case ModulationTarget::Type::FilterHarmonicIntensity:
            return accumulator.filterHarmonicIntensityActive;
        case ModulationTarget::Type::TremoloRate:
            return accumulator.tremoloRateActive;
        case ModulationTarget::Type::TremoloDepth:
            return accumulator.tremoloDepthActive;
        case ModulationTarget::Type::MasterVolume:
            return accumulator.masterVolumeActive;
        case ModulationTarget::Type::LoopMix:
            return accumulator.loopMixActive;
        default:
            return false;
    }
}

float ModulationManager::getModulatedValue (ModulationTarget::Type type, int index) const
{
    switch (type)
    {
        case ModulationTarget::Type::LoopVolume:
            return (index >= 0 && index < 8 && accumulator.loopVolumeActive[index]) ? accumulator.loopVolumes[index] : -1.0f;
        case ModulationTarget::Type::LoopFilterHP:
            return (index >= 0 && index < 8 && accumulator.loopFilterHPActive[index]) ? accumulator.loopFilterHP[index] : -1.0f;
        case ModulationTarget::Type::LoopFilterLP:
            return (index >= 0 && index < 8 && accumulator.loopFilterLPActive[index]) ? accumulator.loopFilterLP[index] : -1.0f;
        case ModulationTarget::Type::DelayTime:
            return accumulator.delayTimeActive ? accumulator.delayTime : -1.0f;
        case ModulationTarget::Type::DelayFeedback:
            return accumulator.delayFeedbackActive ? accumulator.delayFeedback : -1.0f;
        case ModulationTarget::Type::DelayDryWet:
            return accumulator.delayDryWetActive ? accumulator.delayDryWet : -1.0f;
        case ModulationTarget::Type::DistortionDrive:
            return accumulator.distortionDriveActive ? accumulator.distortionDrive : -1.0f;
        case ModulationTarget::Type::DistortionTone:
            return accumulator.distortionToneActive ? accumulator.distortionTone : -1.0f;
        case ModulationTarget::Type::DistortionDryWet:
            return accumulator.distortionDryWetActive ? accumulator.distortionDryWet : -1.0f;
        case ModulationTarget::Type::TapeSaturation:
            return accumulator.tapeSaturationActive ? accumulator.tapeSaturation : -1.0f;
        case ModulationTarget::Type::TapeBias:
            return accumulator.tapeBiasActive ? accumulator.tapeBias : -1.0f;
        case ModulationTarget::Type::TapeWowDepth:
            return accumulator.tapeWowDepthActive ? accumulator.tapeWowDepth : -1.0f;
        case ModulationTarget::Type::TapeFlutterDepth:
            return accumulator.tapeFlutterDepthActive ? accumulator.tapeFlutterDepth : -1.0f;
        case ModulationTarget::Type::TapeDryWet:
            return accumulator.tapeDryWetActive ? accumulator.tapeDryWet : -1.0f;
        case ModulationTarget::Type::FilterHP:
            return accumulator.filterHPActive ? accumulator.filterHP : -1.0f;
        case ModulationTarget::Type::FilterLP:
            return accumulator.filterLPActive ? accumulator.filterLP : -1.0f;
        case ModulationTarget::Type::FilterHarmonicIntensity:
            return accumulator.filterHarmonicIntensityActive ? accumulator.filterHarmonicIntensity : -1.0f;
        case ModulationTarget::Type::TremoloRate:
            return accumulator.tremoloRateActive ? accumulator.tremoloRate : -1.0f;
        case ModulationTarget::Type::TremoloDepth:
            return accumulator.tremoloDepthActive ? accumulator.tremoloDepth : -1.0f;
        case ModulationTarget::Type::MasterVolume:
            return accumulator.masterVolumeActive ? accumulator.masterVolume : -1.0f;
        case ModulationTarget::Type::LoopMix:
            return accumulator.loopMixActive ? accumulator.loopMix : -1.0f;
        default:
            return -1.0f;
    }
}
