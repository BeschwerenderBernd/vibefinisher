#include "PluginProcessor.h"
#include "PluginEditor.h"

VibeFinisherAudioProcessor::VibeFinisherAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", Params::createParameterLayout())
{
    inputGainParam     = apvts.getRawParameterValue (Params::inputGain);
    vibeParam          = apvts.getRawParameterValue (Params::vibe);
    driveParam         = apvts.getRawParameterValue (Params::drive);
    blendParam         = apvts.getRawParameterValue (Params::blend);
    noiseLevelParam    = apvts.getRawParameterValue (Params::noiseLevel);
    noiseGateThrParam  = apvts.getRawParameterValue (Params::noiseGateThr);
    noiseGateDecayParam = apvts.getRawParameterValue (Params::noiseGateDecay);
    noiseTypeParam     = apvts.getRawParameterValue (Params::noiseType);
    outputGainParam    = apvts.getRawParameterValue (Params::outputGain);
    mixParam           = apvts.getRawParameterValue (Params::mix);
    advancedParam      = apvts.getRawParameterValue (Params::advanced);
    driveTrimParam     = apvts.getRawParameterValue (Params::driveTrim);
    tapeTrimParam      = apvts.getRawParameterValue (Params::tapeTrim);
    vinylTrimParam     = apvts.getRawParameterValue (Params::vinylTrim);
    noiseTrimParam     = apvts.getRawParameterValue (Params::noiseTrim);
}

VibeFinisherAudioProcessor::~VibeFinisherAudioProcessor() = default;

const juce::String VibeFinisherAudioProcessor::getName() const { return "VibeFinisher"; }
bool VibeFinisherAudioProcessor::acceptsMidi() const { return false; }
bool VibeFinisherAudioProcessor::producesMidi() const { return false; }
double VibeFinisherAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int VibeFinisherAudioProcessor::getNumPrograms() { return 1; }
int VibeFinisherAudioProcessor::getCurrentProgram() { return 0; }
void VibeFinisherAudioProcessor::setCurrentProgram (int) {}
const juce::String VibeFinisherAudioProcessor::getProgramName (int) { return {}; }
void VibeFinisherAudioProcessor::changeProgramName (int, const juce::String&) {}

void VibeFinisherAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (juce::jmax (1, getTotalNumInputChannels()));

    overdrive.prepare (spec);
    tape.prepare (spec);
    vinyl.prepare (spec);
    noise.prepare (spec);

    const auto latency = overdrive.getLatencyInSamples();
    setLatencySamples (latency);

    dryDelaySampleRate = sampleRate;
    dryDelay.prepare ({ sampleRate, spec.maximumBlockSize, spec.numChannels });
    dryDelay.setMaximumDelayInSamples (latency + 1);
    dryDelay.setDelay (static_cast<float> (latency));

    vinylBuffer.setSize (getTotalNumInputChannels(), samplesPerBlock);

    constexpr auto rampSec = 0.005;
    driveDbSmoother.reset (sampleRate, rampSec);
    tapeDepthSmoother.reset (sampleRate, rampSec);
    vinylDepthSmoother.reset (sampleRate, rampSec);
    blendSmoother.reset (sampleRate, rampSec);
    noiseLevelSmoother.reset (sampleRate, rampSec);
    mixSmoother.reset (sampleRate, rampSec);
}

void VibeFinisherAudioProcessor::releaseResources()
{
}

bool VibeFinisherAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    if (output != juce::AudioChannelSet::mono() && output != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == output;
}

void VibeFinisherAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    if (numSamples == 0 || numChannels == 0)
        return;

    const auto inputGainVal = inputGainParam->load();
    const auto vibe = vibeParam->load() * 0.01f;
    const auto driveVal = driveParam->load();
    const auto blendVal = blendParam->load() * 0.01f;
    const auto noiseLevelVal = noiseLevelParam->load();
    const auto gateThrVal = noiseGateThrParam->load();
    const auto gateDecayVal = noiseGateDecayParam->load();
    const auto noiseTypeVal = static_cast<int> (noiseTypeParam->load());
    const auto outputGainVal = outputGainParam->load();
    const auto mixVal = mixParam->load() * 0.01f;
    const auto driveTrimVal = driveTrimParam->load() * 0.01f;
    const auto tapeTrimVal = tapeTrimParam->load() * 0.01f;
    const auto vinylTrimVal = vinylTrimParam->load() * 0.01f;
    const auto noiseTrimVal = noiseTrimParam->load() * 0.01f;

    {
        const auto gain = juce::Decibels::decibelsToGain (inputGainVal);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                data[i] *= gain;
        }

        float maxRms = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float sum = 0.0f;
            const auto* data = buffer.getReadPointer (ch);
            for (int i = 0; i < numSamples; ++i)
                sum += data[i] * data[i];
            maxRms = juce::jmax (maxRms, std::sqrt (sum / static_cast<float> (numSamples)));
        }
        inputLevel.store (juce::Decibels::gainToDecibels (maxRms + 1e-20f), std::memory_order_relaxed);
    }

    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf (buffer);

    driveDbSmoother.setTargetValue (driveVal * vibe * (1.0f + driveTrimVal));
    overdrive.process (buffer, driveDbSmoother);

    tapeDepthSmoother.setTargetValue (vibe * (1.0f + tapeTrimVal));
    vinylDepthSmoother.setTargetValue (vibe * (1.0f + vinylTrimVal));

    if (vinylBuffer.getNumSamples() < numSamples)
        vinylBuffer.setSize (numChannels, juce::jmax (numSamples, 512), false, false, true);
    for (int ch = 0; ch < numChannels; ++ch)
        vinylBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    tape.process (buffer, tapeDepthSmoother);
    vinyl.process (vinylBuffer, vinylDepthSmoother);

    blendSmoother.setTargetValue (blendVal);
    dspx::parallelBlend (buffer, vinylBuffer, blendSmoother);

    noiseLevelSmoother.setTargetValue (noiseLevelVal * vibe * (1.0f + noiseTrimVal));
    noise.process (buffer, noiseLevelSmoother, gateThrVal, gateDecayVal, noiseTypeVal);

    mixSmoother.setTargetValue (mixVal);
    for (int i = 0; i < numSamples; ++i)
    {
        const auto mix = mixSmoother.getNextValue();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            dryDelay.pushSample (ch, dryBuffer.getReadPointer (ch)[i]);
            const auto delayedDry = dryDelay.popSample (ch);
            buffer.getWritePointer (ch)[i] = buffer.getReadPointer (ch)[i] * mix
                                           + delayedDry * (1.0f - mix);
        }
    }

    {
        const auto gain = juce::Decibels::decibelsToGain (outputGainVal);
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                data[i] *= gain;
        }
    }

    {
        float maxRms = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float sum = 0.0f;
            const auto* data = buffer.getReadPointer (ch);
            for (int i = 0; i < numSamples; ++i)
                sum += data[i] * data[i];
            maxRms = juce::jmax (maxRms, std::sqrt (sum / static_cast<float> (numSamples)));
        }
        outputLevel.store (juce::Decibels::gainToDecibels (maxRms + 1e-20f), std::memory_order_relaxed);
    }
}

juce::AudioProcessorEditor* VibeFinisherAudioProcessor::createEditor()
{
    return new VibeFinisherAudioProcessorEditor (*this);
}

bool VibeFinisherAudioProcessor::hasEditor() const
{
    return true;
}

void VibeFinisherAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VibeFinisherAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VibeFinisherAudioProcessor();
}
