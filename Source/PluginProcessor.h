#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "Params.h"
#include "DSP/OverdriveStage.h"
#include "DSP/TapeStage.h"
#include "DSP/VinylStage.h"
#include "DSP/NoiseStage.h"
#include "DSP/ParallelBlend.h"

class VibeFinisherAudioProcessor : public juce::AudioProcessor
{
public:
    VibeFinisherAudioProcessor();
    ~VibeFinisherAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override;

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    bool isBusesLayoutSupported (const BusesLayout&) const override;

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float> inputLevel { -60.0f };
    std::atomic<float> outputLevel { -60.0f };

private:
    using Smoother = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;

    dspx::OverdriveStage overdrive;
    dspx::TapeStage tape;
    dspx::VinylStage vinyl;
    dspx::NoiseStage noise;

    juce::AudioBuffer<float> vinylBuffer;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay;
    double dryDelaySampleRate = 44100.0;

    Smoother driveDbSmoother;
    Smoother tapeDepthSmoother;
    Smoother vinylDepthSmoother;
    Smoother blendSmoother;
    Smoother noiseLevelSmoother;
    Smoother mixSmoother;
    Smoother calGainSmoother;
    Smoother calMakeupSmoother;

    std::atomic<float>* inputGainParam = nullptr;
    std::atomic<float>* vibeParam = nullptr;
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* blendParam = nullptr;
    std::atomic<float>* noiseLevelParam = nullptr;
    std::atomic<float>* noiseGateThrParam = nullptr;
    std::atomic<float>* noiseGateDecayParam = nullptr;
    std::atomic<float>* noiseTypeParam = nullptr;
    std::atomic<float>* outputGainParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* calPadParam = nullptr;
    std::atomic<float>* advancedParam = nullptr;
    std::atomic<float>* driveTrimParam = nullptr;
    std::atomic<float>* tapeTrimParam = nullptr;
    std::atomic<float>* vinylTrimParam = nullptr;
    std::atomic<float>* noiseTrimParam = nullptr;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibeFinisherAudioProcessor)
};
