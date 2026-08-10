#pragma once

#include <juce_dsp/juce_dsp.h>

namespace dspx
{

class OverdriveStage
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        oversampling.initProcessing (static_cast<size_t> (spec.maximumBlockSize));
        latencySamples = static_cast<int> (oversampling.getLatencyInSamples());
        reset();
    }

    void reset()
    {
        oversampling.reset();
    }

    int getLatencyInSamples() const
    {
        return latencySamples;
    }

    void process (juce::AudioBuffer<float>& buffer,
                  juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>& driveDbSmoother)
    {
        auto oversampled = oversampling.processSamplesUp (buffer);

        const auto osSamples = static_cast<int> (oversampled.getNumSamples());
        const auto hasDrive = driveDbSmoother.getTargetValue() > 0.01f || driveDbSmoother.isSmoothing();

        if (hasDrive)
        {
            for (size_t ch = 0; ch < oversampled.getNumChannels(); ++ch)
            {
                auto* data = oversampled.getChannelPointer (ch);
                for (int i = 0; i < osSamples; ++i)
                {
                    const auto driveDb = driveDbSmoother.getNextValue();
                    if (driveDb > 0.01f)
                    {
                        const auto preGain = juce::Decibels::decibelsToGain (driveDb);
                        data[i] = std::tanh (data[i] * preGain);
                    }
                }
            }
        }

        auto outBlock = juce::dsp::AudioBlock<float> (buffer);
        oversampling.processSamplesDown (outBlock);
    }

private:
    juce::dsp::Oversampling<float> oversampling { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };
    double sampleRate = 44100.0;
    int latencySamples = 0;
};

} // namespace dspx
