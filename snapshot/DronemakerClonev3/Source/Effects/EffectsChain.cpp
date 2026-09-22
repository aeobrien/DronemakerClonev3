#include "EffectsChain.h"
#include <cmath>
#include "FilterEffect.h"
#include "DelayEffect.h"
#include "GranularEffect.h"
#include "TremoloEffect.h"
#include "DistortionEffect.h"
#include "TapeEffect.h"

EffectsChain::EffectsChain()
{
    // Create all effects
    effects[Filter] = std::make_unique<FilterEffect>();
    effects[Delay] = std::make_unique<DelayEffect>();
    effects[Granular] = std::make_unique<GranularEffect>();
    effects[Tremolo] = std::make_unique<TremoloEffect>();
    effects[Distortion] = std::make_unique<DistortionEffect>();
    effects[Tape] = std::make_unique<TapeEffect>();

    // Default order: Filter → Delay → Granular → Tremolo → Distortion → Tape
    for (int i = 0; i < numEffects; ++i)
        routingOrder[i].store (i);
}

EffectsChain::~EffectsChain()
{
}

void EffectsChain::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    for (auto& effect : effects)
        if (effect)
            effect->prepareToPlay (sampleRate, samplesPerBlock);
}

void EffectsChain::processSample (float& left, float& right)
{
    // Process in routing order
    for (int i = 0; i < numEffects; ++i)
    {
        int effectIdx = routingOrder[i].load();
        if (effectIdx >= 0 && effectIdx < numEffects && effects[effectIdx])
        {
            auto* effect = effects[effectIdx].get();
            if (effect->isEnabled())
            {
                effect->processSample (left, right);

                // Apply gain compensation
                float gainFactor = 1.0f;
                if (effect->getAutoGainComp())
                    gainFactor *= effect->getAutoGainFactor();
                float manualDB = effect->getGainCompensation();
                if (std::abs (manualDB) > 0.01f)
                    gainFactor *= std::pow (10.0f, manualDB / 20.0f);
                if (std::abs (gainFactor - 1.0f) > 0.001f)
                {
                    left *= gainFactor;
                    right *= gainFactor;
                }
            }
        }
    }
}

void EffectsChain::reset()
{
    for (auto& effect : effects)
        if (effect)
            effect->reset();
}

void EffectsChain::setOrder (const std::array<int, numEffects>& newOrder)
{
    for (int i = 0; i < numEffects; ++i)
        routingOrder[i].store (newOrder[i]);
}

std::array<int, EffectsChain::numEffects> EffectsChain::getOrder() const
{
    std::array<int, numEffects> order;
    for (int i = 0; i < numEffects; ++i)
        order[i] = routingOrder[i].load();
    return order;
}

void EffectsChain::swapPositions (int posA, int posB)
{
    if (posA < 0 || posA >= numEffects || posB < 0 || posB >= numEffects)
        return;

    int a = routingOrder[posA].load();
    int b = routingOrder[posB].load();
    routingOrder[posA].store (b);
    routingOrder[posB].store (a);
}

EffectBase* EffectsChain::getEffect (int effectType)
{
    if (effectType < 0 || effectType >= numEffects)
        return nullptr;
    return effects[effectType].get();
}

FilterEffect* EffectsChain::getFilter()
{
    return static_cast<FilterEffect*> (effects[Filter].get());
}

DelayEffect* EffectsChain::getDelay()
{
    return static_cast<DelayEffect*> (effects[Delay].get());
}

GranularEffect* EffectsChain::getGranular()
{
    return static_cast<GranularEffect*> (effects[Granular].get());
}

TremoloEffect* EffectsChain::getTremolo()
{
    return static_cast<TremoloEffect*> (effects[Tremolo].get());
}

DistortionEffect* EffectsChain::getDistortion()
{
    return static_cast<DistortionEffect*> (effects[Distortion].get());
}

TapeEffect* EffectsChain::getTape()
{
    return static_cast<TapeEffect*> (effects[Tape].get());
}

void EffectsChain::setModulation (float delayTime, float delayFeedback, float delayDryWet,
                                   float distortionDrive, float distortionTone, float distortionDryWet,
                                   float tapeSaturation, float tapeBias, float tapeWowDepth, float tapeFlutterDepth, float tapeDryWet,
                                   float filterHP, float filterLP, float filterHarmonicIntensity)
{
    // Modulation is applied through parameter offsets
    // The UI stores base values and applies base + modulation when updating parameters
    // This method is a placeholder - actual modulation is applied in MainComponent::applyModulation
    juce::ignoreUnused (delayTime, delayFeedback, delayDryWet,
                        distortionDrive, distortionTone, distortionDryWet,
                        tapeSaturation, tapeBias, tapeWowDepth, tapeFlutterDepth, tapeDryWet,
                        filterHP, filterLP, filterHarmonicIntensity);
}
