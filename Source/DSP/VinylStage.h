#pragma once

#include <juce_dsp/juce_dsp.h>

namespace dspx
{

class VinylStage
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;

        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, 150.0f, 0.707f);
        sideLowpass.coefficients = coeffs;

        reset();
    }

    void reset()
    {
        sideLowpass.reset();
    }

    void process (juce::AudioBuffer<float>& buffer,
                  juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>& depthSmoother)
    {
        if (depthSmoother.getTargetValue() <= 0.001f && !depthSmoother.isSmoothing())
            return;

        const auto numSamples = buffer.getNumSamples();
        const auto numChannels = buffer.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            const auto depth = depthSmoother.getNextValue();
            const auto satGain = 1.0f + depth * 1.5f;
            const auto monoDepth = depth * 0.6f;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                const auto dry = buffer.getReadPointer (ch)[i];

                auto wet = std::tanh (dry * satGain);

                buffer.getWritePointer (ch)[i] = dry * (1.0f - depth) + wet * depth;
            }

            if (numChannels > 1)
            {
                const auto L = buffer.getReadPointer (0)[i];
                const auto R = buffer.getReadPointer (1)[i];
                const auto mid = (L + R) * 0.5f;
                const auto side = (L - R) * 0.5f;
                const auto sideLow = sideLowpass.processSample (side);
                const auto narrowedSide = side - sideLow * monoDepth;

                buffer.getWritePointer (0)[i] = mid + narrowedSide;
                buffer.getWritePointer (1)[i] = mid - narrowedSide;
            }
        }
    }

private:
    juce::dsp::IIR::Filter<float> sideLowpass;
    double sampleRate = 44100.0;
};

} // namespace dspx
