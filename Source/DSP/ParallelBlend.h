#pragma once

#include <juce_dsp/juce_dsp.h>

namespace dspx
{

inline void parallelBlend (juce::AudioBuffer<float>& tapeBuffer,
                           juce::AudioBuffer<float>& vinylBuffer,
                           juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>& blendSmoother)
{
    const auto numSamples = tapeBuffer.getNumSamples();
    const auto numChannels = tapeBuffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        const auto blend = blendSmoother.getNextValue();
        const auto tapeWeight = 1.0f - blend;
        const auto vinylWeight = blend;

        for (int ch = 0; ch < numChannels; ++ch)
            tapeBuffer.getWritePointer (ch)[i] = tapeBuffer.getReadPointer (ch)[i] * tapeWeight
                                               + vinylBuffer.getReadPointer (ch)[i] * vinylWeight;
    }
}

} // namespace dspx
