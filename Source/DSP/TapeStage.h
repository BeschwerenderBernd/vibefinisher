#pragma once

#include <juce_dsp/juce_dsp.h>

namespace dspx
{

class TapeStage
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;

        rmsDetector.prepare (spec);
        rmsDetector.setAttackTime (2.0f);
        rmsDetector.setReleaseTime (80.0f);

        auto lowShelfCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf (
            sampleRate, 80.0f, 0.5f, juce::Decibels::decibelsToGain (3.0f));
        auto highShelfCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
            sampleRate, 9000.0f, 0.5f, juce::Decibels::decibelsToGain (-2.5f));

        for (int ch = 0; ch < 2; ++ch)
        {
            lowShelf[ch].coefficients = lowShelfCoeffs;
            highShelf[ch].coefficients = highShelfCoeffs;
        }

        reset();
    }

    void reset()
    {
        rmsDetector.reset();
        for (int ch = 0; ch < 2; ++ch)
        {
            lowShelf[ch].reset();
            highShelf[ch].reset();
        }
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
            const auto saturationGain = 1.0f + depth * 2.0f;
            const auto compAmount = depth * 0.15f;

            float peak = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
                peak = juce::jmax (peak, std::abs (buffer.getReadPointer (ch)[i]));

            const auto env = rmsDetector.processSample (0, peak);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                const auto dry = buffer.getReadPointer (ch)[i];

                auto wet = std::atan (dry * saturationGain) * 2.0f / juce::MathConstants<float>::pi;

                if (env > 0.04f)
                {
                    const auto gr = juce::jlimit (0.0f, 0.25f, compAmount * env);
                    wet *= (1.0f - gr);
                }

                const auto chIdx = ch < 2 ? ch : 0;
                auto filtered = lowShelf[chIdx].processSample (wet);
                filtered = highShelf[chIdx].processSample (filtered);

                buffer.getWritePointer (ch)[i] = dry * (1.0f - depth) + filtered * depth;
            }
        }
    }

private:
    juce::dsp::IIR::Filter<float> lowShelf[2];
    juce::dsp::IIR::Filter<float> highShelf[2];
    juce::dsp::BallisticsFilter<float> rmsDetector;
    double sampleRate = 44100.0;
};

} // namespace dspx
