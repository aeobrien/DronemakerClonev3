#pragma once

#include "ModulationSource.h"
#include <array>
#include <memory>

// Forward declarations
class LoopRecorder;
class EffectsChain;

/**
 * Manages all modulation sources and applies them to their targets.
 * Central hub for the modulation system.
 */
class ModulationManager
{
public:
    static constexpr int numLFOs = 4;
    static constexpr int numEnvelopes = 1;
    static constexpr int numRandomizers = 2;

    ModulationManager();

    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void reset();

    // Process all modulation sources for one sample
    // inputL/inputR are for envelope follower
    void processSample (float inputL, float inputR);

    // Apply accumulated modulation to targets
    // Call this after processing audio to update parameters
    void applyModulation (LoopRecorder& loopRecorder, EffectsChain& effectsChain,
                          float& loopMixMod, float& masterVolumeMod);

    // Access to individual sources
    LFOSource& getLFO (int index) { return lfos[index]; }
    EnvelopeFollowerSource& getEnvelopeFollower() { return envelopeFollower; }
    RandomizerSource& getRandomizer (int index) { return randomizers[index]; }

    const LFOSource& getLFO (int index) const { return lfos[index]; }
    const EnvelopeFollowerSource& getEnvelopeFollower() const { return envelopeFollower; }
    const RandomizerSource& getRandomizer (int index) const { return randomizers[index]; }

    // Get all sources as a flat list for iteration
    ModulationSource* getSource (int index);
    int getNumSources() const { return numLFOs + numEnvelopes + numRandomizers; }

    // Get target name for display
    static juce::String getTargetTypeName (ModulationTarget::Type type);

    // Query modulation state for UI reflection
    // Returns true if the target is currently being modulated
    bool isTargetModulated (ModulationTarget::Type type, int index = 0) const;

    // Get the current modulated value for a target (for UI display)
    // Returns -1.0f if not modulated
    float getModulatedValue (ModulationTarget::Type type, int index = 0) const;

private:
    std::array<LFOSource, numLFOs> lfos;
    EnvelopeFollowerSource envelopeFollower;
    std::array<RandomizerSource, numRandomizers> randomizers;

    // Accumulated modulation values for each target type
    // These are set by active sources and applied to parameters
    struct ModulationAccumulator
    {
        std::array<float, 8> loopVolumes {};
        std::array<float, 8> loopFilterHP {};
        std::array<float, 8> loopFilterLP {};
        std::array<bool, 8> loopVolumeActive {};
        std::array<bool, 8> loopFilterHPActive {};
        std::array<bool, 8> loopFilterLPActive {};

        float delayTime = 0.0f;
        float delayFeedback = 0.0f;
        float delayDryWet = 0.0f;
        bool delayTimeActive = false;
        bool delayFeedbackActive = false;
        bool delayDryWetActive = false;

        float distortionDrive = 0.0f;
        float distortionTone = 0.0f;
        float distortionDryWet = 0.0f;
        bool distortionDriveActive = false;
        bool distortionToneActive = false;
        bool distortionDryWetActive = false;

        float tapeSaturation = 0.0f;
        float tapeBias = 0.0f;
        float tapeWowDepth = 0.0f;
        float tapeFlutterDepth = 0.0f;
        float tapeDryWet = 0.0f;
        bool tapeSaturationActive = false;
        bool tapeBiasActive = false;
        bool tapeWowDepthActive = false;
        bool tapeFlutterDepthActive = false;
        bool tapeDryWetActive = false;

        float filterHP = 0.0f;
        float filterLP = 0.0f;
        float filterHarmonicIntensity = 0.0f;
        bool filterHPActive = false;
        bool filterLPActive = false;
        bool filterHarmonicIntensityActive = false;

        float tremoloRate = 0.0f;
        float tremoloDepth = 0.0f;
        bool tremoloRateActive = false;
        bool tremoloDepthActive = false;

        float masterVolume = 0.0f;
        float loopMix = 0.0f;
        bool masterVolumeActive = false;
        bool loopMixActive = false;

        void reset()
        {
            loopVolumes.fill (0.0f);
            loopFilterHP.fill (0.0f);
            loopFilterLP.fill (0.0f);
            loopVolumeActive.fill (false);
            loopFilterHPActive.fill (false);
            loopFilterLPActive.fill (false);

            delayTime = delayFeedback = delayDryWet = 0.0f;
            delayTimeActive = delayFeedbackActive = delayDryWetActive = false;

            distortionDrive = distortionTone = distortionDryWet = 0.0f;
            distortionDriveActive = distortionToneActive = distortionDryWetActive = false;

            tapeSaturation = tapeBias = tapeWowDepth = tapeFlutterDepth = tapeDryWet = 0.0f;
            tapeSaturationActive = tapeBiasActive = tapeWowDepthActive = tapeFlutterDepthActive = tapeDryWetActive = false;

            filterHP = filterLP = filterHarmonicIntensity = 0.0f;
            filterHPActive = filterLPActive = filterHarmonicIntensityActive = false;

            tremoloRate = tremoloDepth = 0.0f;
            tremoloRateActive = tremoloDepthActive = false;

            masterVolume = loopMix = 0.0f;
            masterVolumeActive = loopMixActive = false;
        }
    };

    ModulationAccumulator accumulator;

    void accumulateTarget (const ModulationTarget& target, float value);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModulationManager)
};
