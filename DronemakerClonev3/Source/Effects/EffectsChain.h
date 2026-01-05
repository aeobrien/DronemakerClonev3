#pragma once

#include <JuceHeader.h>
#include "EffectBase.h"
#include <array>
#include <memory>
#include <atomic>

// Forward declarations
class FilterEffect;
class DelayEffect;
class GranularEffect;
class TremoloEffect;
class DistortionEffect;
class TapeEffect;

/**
 * Master effects chain with reorderable routing.
 * Manages 6 effects that can be reordered at runtime.
 */
class EffectsChain
{
public:
    static constexpr int numEffects = 6;

    // Effect type indices (for getEffect)
    enum EffectType
    {
        Filter = 0,
        Delay = 1,
        Granular = 2,
        Tremolo = 3,
        Distortion = 4,
        Tape = 5
    };

    EffectsChain();
    ~EffectsChain();

    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void processSample (float& left, float& right);
    void reset();

    // Reordering - sets the processing order
    // order[0] = first effect to process, order[5] = last effect
    void setOrder (const std::array<int, numEffects>& newOrder);
    std::array<int, numEffects> getOrder() const;
    void swapPositions (int posA, int posB);

    // Access individual effects for parameter control
    EffectBase* getEffect (int effectType);

    // Typed access for convenience
    FilterEffect* getFilter();
    DelayEffect* getDelay();
    GranularEffect* getGranular();
    TremoloEffect* getTremolo();
    DistortionEffect* getDistortion();
    TapeEffect* getTape();

private:
    // Effects storage (fixed order by type)
    std::array<std::unique_ptr<EffectBase>, numEffects> effects;

    // Routing order: order[i] = effect index to process at position i
    // Uses atomic for each element for thread-safe updates
    std::array<std::atomic<int>, numEffects> routingOrder;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectsChain)
};
