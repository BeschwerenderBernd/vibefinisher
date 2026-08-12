#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace Params
{

inline constexpr const char* inputGain     = "inputGain";
inline constexpr const char* vibe          = "vibe";
inline constexpr const char* drive         = "drive";
inline constexpr const char* blend         = "blend";
inline constexpr const char* noiseLevel    = "noiseLevel";
inline constexpr const char* noiseGateThr  = "noiseGateThr";
inline constexpr const char* noiseGateDecay = "noiseGateDecay";
inline constexpr const char* noiseType     = "noiseType";
inline constexpr const char* outputGain    = "outputGain";
inline constexpr const char* mix           = "mix";
inline constexpr const char* calPad        = "calPad";
inline constexpr const char* advanced      = "advanced";
inline constexpr const char* driveTrim     = "driveTrim";
inline constexpr const char* tapeTrim      = "tapeTrim";
inline constexpr const char* vinylTrim     = "vinylTrim";
inline constexpr const char* noiseTrim     = "noiseTrim";

inline constexpr int noiseTapeHiss  = 0;
inline constexpr int noiseVinyl     = 1;
inline constexpr int noiseConsole   = 2;
inline constexpr int noiseDigital   = 3;

inline constexpr float calibrationTrimDb = -6.0f;

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { inputGain, 1 }, "Input",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { vibe, 1 }, "Vibe",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { drive, 1 }, "Drive",
        juce::NormalisableRange<float> (0.0f, 18.0f, 0.1f), 5.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { blend, 1 }, "Blend",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { noiseLevel, 1 }, "Noise",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 10.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { noiseGateThr, 1 }, "Noise Gate",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -40.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { noiseGateDecay, 1 }, "Gate Decay",
        juce::NormalisableRange<float> (50.0f, 5000.0f, 1.0f, 0.3f), 500.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { noiseType, 1 }, "Noise Type",
        juce::StringArray { "Tape Hiss", "Vinyl", "Console", "Digital" }, noiseTapeHiss));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { outputGain, 1 }, "Output",
        juce::NormalisableRange<float> (-24.0f, 6.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { mix, 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { calPad, 1 }, "Pad", true));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { advanced, 1 }, "Advanced", false));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { driveTrim, 1 }, "Drive Trim",
        juce::NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { tapeTrim, 1 }, "Tape Trim",
        juce::NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { vinylTrim, 1 }, "Vinyl Trim",
        juce::NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { noiseTrim, 1 }, "Noise Trim",
        juce::NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    return layout;
}

} // namespace Params
