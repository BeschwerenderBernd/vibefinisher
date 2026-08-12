#pragma once

#include <juce_dsp/juce_dsp.h>
#include <random>

namespace dspx
{

class NoiseStage
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;

        auto tapeLpCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, 8000.0f, 0.7f);
        auto tapeHpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 2000.0f, 0.7f);
        auto digitalCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, 3000.0f, 1.0f);

        tapeLp.coefficients = tapeLpCoeffs;
        tapeHp.coefficients = tapeHpCoeffs;
        digitalLp.coefficients = digitalCoeffs;

        envelopeFollower.prepare (spec);
        envelopeFollower.setAttackTime (10.0f);
        envelopeFollower.setReleaseTime (50.0f);

        popDecayCoeff = std::exp (-1.0f / (static_cast<float> (sampleRate) * 0.0005f));
        gateAttackCoeff = std::exp (-1.0f / (static_cast<float> (sampleRate) * 0.002f));
        driftSmoothCoeff = std::exp (-1.0f / (static_cast<float> (sampleRate) * 0.1f));

        reset();
    }

    void reset()
    {
        tapeLp.reset();
        tapeHp.reset();
        digitalLp.reset();
        envelopeFollower.reset();
        gateEnvelope = 0.0f;
        pinkB0 = pinkB1 = pinkB2 = pinkB3 = pinkB4 = pinkB5 = pinkB6 = 0.0f;
        popEnv[0] = popEnv[1] = 0.0f;
        levelDriftValue = 1.0f;
        levelDriftTarget = 1.0f;
        samplesUntilNextDrift = 0;
        popRateValue = 13.0f;
        popRateTarget = 13.0f;
    }

    void process (juce::AudioBuffer<float>& buffer,
                  juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>& levelSmoother,
                  float gateThrDb, float gateDecayMs, int noiseType)
    {
        const auto numSamples = buffer.getNumSamples();
        const auto numChannels = buffer.getNumChannels();

        const auto gateReleaseCoeff = std::exp (-1.0f / (static_cast<float> (sampleRate) * gateDecayMs * 0.001f));
        const auto gateThreshold = juce::Decibels::decibelsToGain (gateThrDb);

        for (int i = 0; i < numSamples; ++i)
        {
            if (samplesUntilNextDrift <= 0)
            {
                std::uniform_real_distribution<float> levelDist (0.7f, 1.0f);
                levelDriftTarget = levelDist (gen);
                std::uniform_real_distribution<float> rateDist (6.5f, 19.5f);
                popRateTarget = rateDist (gen);
                std::uniform_int_distribution<int> intervalDist (
                    static_cast<int> (sampleRate * 0.15),
                    static_cast<int> (sampleRate * 0.6));
                samplesUntilNextDrift = intervalDist (gen);
            }
            levelDriftValue = driftSmoothCoeff * levelDriftValue + (1.0f - driftSmoothCoeff) * levelDriftTarget;
            popRateValue = driftSmoothCoeff * popRateValue + (1.0f - driftSmoothCoeff) * popRateTarget;
            --samplesUntilNextDrift;

            const auto noiseGain = levelSmoother.getNextValue() * 0.01f * 0.8f * levelDriftValue;

            float inputMeanAbs = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
                inputMeanAbs += std::abs (buffer.getReadPointer (ch)[i]);
            inputMeanAbs /= static_cast<float> (numChannels);

            const auto smoothedInput = envelopeFollower.processSample (0, inputMeanAbs);
            const auto gateTarget = smoothedInput > gateThreshold ? 1.0f : 0.0f;

            if (gateTarget > gateEnvelope)
                gateEnvelope = gateAttackCoeff * gateEnvelope + (1.0f - gateAttackCoeff) * gateTarget;
            else
                gateEnvelope = gateReleaseCoeff * gateEnvelope + (1.0f - gateReleaseCoeff) * gateTarget;

            if (gateEnvelope < 0.0001f)
                continue;

            if (noiseType == 1)
            {
                for (int ch = 0; ch < numChannels; ++ch)
                    buffer.getWritePointer (ch)[i] += generateVinylCrackle (ch) * noiseGain * gateEnvelope;
            }
            else
            {
                float noiseSample = 0.0f;
                switch (noiseType)
                {
                    case 0: noiseSample = generateTapeHiss(); break;
                    case 2: noiseSample = generateConsoleNoise(); break;
                    case 3: noiseSample = generateDigitalNoise(); break;
                }
                for (int ch = 0; ch < numChannels; ++ch)
                    buffer.getWritePointer (ch)[i] += noiseSample * noiseGain * gateEnvelope;
            }
        }
    }

private:
    float generateTapeHiss()
    {
        auto w = whiteNoise (gen);
        auto filtered = tapeLp.processSample (w);
        filtered = tapeHp.processSample (filtered);
        return filtered * 0.7f;
    }

    float generateVinylCrackle (int ch)
    {
        std::uniform_real_distribution<float> dist (0.0f, 1.0f);
        const auto r = dist (gen);
        const auto popProb = popRateValue / static_cast<float> (sampleRate);
        const auto textureProb = (popRateValue * 30.0f / 13.0f) / static_cast<float> (sampleRate);

        float result = 0.0f;

        if (r < popProb)
        {
            std::uniform_real_distribution<float> polarity (-1.0f, 1.0f);
            popEnv[ch] = polarity (gen) * 0.8f;
        }
        else if (r < textureProb)
        {
            result += whiteNoise (gen) * 0.08f;
        }

        if (std::abs (popEnv[ch]) > 0.0001f)
        {
            result += whiteNoise (gen) * popEnv[ch];
            popEnv[ch] *= popDecayCoeff;
        }

        return result;
    }

    float generateConsoleNoise()
    {
        auto w = whiteNoise (gen);
        pinkB0 = 0.99886f * pinkB0 + w * 0.0555179f;
        pinkB1 = 0.99332f * pinkB1 + w * 0.0750759f;
        pinkB2 = 0.96900f * pinkB2 + w * 0.1538520f;
        pinkB3 = 0.86650f * pinkB3 + w * 0.3104856f;
        pinkB4 = 0.55000f * pinkB4 + w * 0.5329522f;
        pinkB5 = -0.7616f * pinkB5 - w * 0.0168980f;
        auto pink = pinkB0 + pinkB1 + pinkB2 + pinkB3 + pinkB4 + pinkB5 + pinkB6 + w * 0.5362f;
        pinkB6 = w * 0.115926f;
        return pink * 0.15f;
    }

    float generateDigitalNoise()
    {
        auto w = whiteNoise (gen);
        return digitalLp.processSample (w) * 0.4f;
    }

    juce::dsp::IIR::Filter<float> tapeLp;
    juce::dsp::IIR::Filter<float> tapeHp;
    juce::dsp::IIR::Filter<float> digitalLp;
    double sampleRate = 44100.0;

    juce::dsp::BallisticsFilter<float> envelopeFollower;
    std::mt19937 gen { std::random_device{}() };
    std::normal_distribution<float> whiteNoise { 0.0f, 1.0f };
    float pinkB0 = 0.0f, pinkB1 = 0.0f, pinkB2 = 0.0f, pinkB3 = 0.0f;
    float pinkB4 = 0.0f, pinkB5 = 0.0f, pinkB6 = 0.0f;
    float popEnv[2] = { 0.0f, 0.0f };
    float popDecayCoeff = 0.99f;
    float gateAttackCoeff = 0.99f;
    float gateEnvelope = 0.0f;
    float levelDriftValue = 1.0f;
    float levelDriftTarget = 1.0f;
    int samplesUntilNextDrift = 0;
    float driftSmoothCoeff = 0.99f;
    float popRateValue = 13.0f;
    float popRateTarget = 13.0f;
};

} // namespace dspx
