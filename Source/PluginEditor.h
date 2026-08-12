#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>

#include "PluginProcessor.h"
#include "UI/GateScope.h"
#include "UI/LevelMeter.h"

namespace Colours
{
    const juce::Colour bg         { 0xff101215 };
    const juce::Colour panel      { 0xff181b20 };
    const juce::Colour track      { 0xff333844 };
    const juce::Colour accent     { 0xffff7043 };
    const juce::Colour accentDim  { 0xff8f3e26 };
    const juce::Colour textBright { 0xffe8eaf0 };
    const juce::Colour textDim    { 0xff9aa0ab };
}

class VibeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VibeLookAndFeel()
    {
        setColour (juce::Slider::rotarySliderFillColourId, Colours::accent);
        setColour (juce::ComboBox::backgroundColourId, Colours::panel);
        setColour (juce::ComboBox::textColourId, Colours::textBright);
        setColour (juce::ComboBox::arrowColourId, Colours::accent);
        setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
        setColour (juce::PopupMenu::backgroundColourId, Colours::panel);
        setColour (juce::PopupMenu::textColourId, Colours::textBright);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, Colours::accentDim);
        setColour (juce::PopupMenu::highlightedTextColourId, Colours::textBright);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override
    {
        const auto bounds = juce::Rectangle<int> (x, y, width, height).reduced (4).toFloat();
        const auto centre = bounds.getCentre();
        const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        const auto lineW = juce::jmin (2.0f, radius * 0.4f);
        const auto arcRadius = radius - lineW * 0.5f;

        {
            juce::Path backgroundArc;
            backgroundArc.addCentredArc (centre.getX(), centre.getY(), arcRadius, arcRadius,
                                         0.0f, rotaryStartAngle, rotaryEndAngle, true);
            g.setColour (Colours::track);
            g.strokePath (backgroundArc, juce::PathStrokeType (lineW));
        }

        {
            juce::Path valueArc;
            valueArc.addCentredArc (centre.getX(), centre.getY(), arcRadius, arcRadius,
                                    0.0f, rotaryStartAngle, toAngle, true);
            g.setColour (Colours::accent);
            g.strokePath (valueArc, juce::PathStrokeType (lineW));
        }

        const auto thumbRadius = lineW * 1.3f;
        juce::Point<float> thumbPoint (centre.getX() + arcRadius * std::cos (toAngle - juce::MathConstants<float>::halfPi),
                                       centre.getY() + arcRadius * std::sin (toAngle - juce::MathConstants<float>::halfPi));
        g.setColour (Colours::accent);
        g.fillEllipse (juce::Rectangle<float> (thumbRadius * 2.0f, thumbRadius * 2.0f).withCentre (thumbPoint));

        g.setColour (Colours::textBright);
        g.setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold")));
        g.drawText (slider.getTextFromValue (slider.getValue()),
                    bounds.reduced (0.0f, radius * 0.25f),
                    juce::Justification::centredBottom, true);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                const juce::Colour&, bool highlighted, bool isDown) override
    {
        const auto b = button.getLocalBounds().toFloat().reduced (0.5f);
        const auto r = 4.0f;

        if (button.getToggleState())
        {
            g.setColour (Colours::accent);
            g.fillRoundedRectangle (b, r);
        }
        else if (isDown)
        {
            g.setColour (Colours::accentDim);
            g.fillRoundedRectangle (b, r);
        }
        else if (highlighted)
        {
            g.setColour (Colours::accentDim.withAlpha (0.45f));
            g.fillRoundedRectangle (b, r);
        }
        else
        {
            g.setColour (Colours::panel);
            g.fillRoundedRectangle (b, r);
            g.setColour (Colours::track);
            g.drawRoundedRectangle (b, r, 1.0f);
        }
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                          bool, bool) override
    {
        g.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
        g.setColour (button.getToggleState() ? Colours::textBright : Colours::textDim);
        g.drawText (button.getButtonText(), button.getLocalBounds(),
                    juce::Justification::centred, true);
    }

    void drawComboBox (juce::Graphics& g, int width, int height,
                        bool isButtonDown, int, int, int, int, juce::ComboBox&) override
    {
        const auto b = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
        const auto r = 4.0f;

        g.setColour (Colours::panel);
        g.fillRoundedRectangle (b, r);
        g.setColour (Colours::track);
        g.drawRoundedRectangle (b, r, 1.0f);

        if (isButtonDown)
        {
            g.setColour (Colours::accentDim.withAlpha (0.3f));
            g.fillRoundedRectangle (b, r);
        }

        const auto arrowWidth = (float) height * 0.35f;
        const auto arrowHeight = (float) height * 0.2f;
        const auto arrowX = (float) width - (float) height * 0.55f - arrowWidth * 0.5f;
        const auto arrowY = ((float) height - arrowHeight) * 0.5f;
        juce::Path arrow;
        arrow.addTriangle (arrowX, arrowY,
                           arrowX + arrowWidth, arrowY,
                           arrowX + arrowWidth * 0.5f, arrowY + arrowHeight);
        g.setColour (Colours::accent);
        g.fillPath (arrow);
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (8, 0, box.getWidth() - 32, box.getHeight());
        label.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
    }
};

class VibeFinisherAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        public juce::Timer
{
public:
    explicit VibeFinisherAudioProcessorEditor (VibeFinisherAudioProcessor& p);
    ~VibeFinisherAudioProcessorEditor() override;

    void resized() override;
    void paint (juce::Graphics&) override;
    void timerCallback() override;
    void updateSize();
    void updateRefTick();

private:
    VibeFinisherAudioProcessor& processorRef;
    VibeLookAndFeel laf;

    juce::Slider vibeSlider;
    juce::Slider inputSlider, driveSlider, blendSlider, noiseSlider, gateThrSlider, gateDecaySlider;
    juce::Slider outputSlider, mixSlider;
    juce::ComboBox noiseTypeBox;

    juce::Label vibeLabel;
    juce::Label inputLabel;
    juce::Label driveLabel, blendLabel, noiseLabel, gateThrLabel, gateDecayLabel;
    juce::Label outputLabel, mixLabel;

    juce::TextButton advancedButton;
    juce::TextButton padButton;
    juce::Slider driveTrimSlider, tapeTrimSlider, vinylTrimSlider, noiseTrimSlider;
    juce::Label driveTrimLabel, tapeTrimLabel, vinylTrimLabel, noiseTrimLabel;
    bool advancedVisible = false;
    bool padState = true;

    std::unique_ptr<GateScope> scope;
    LevelMeter inMeter, outMeter;

    float inMeterLevel = -60.0f;
    float outMeterLevel = -60.0f;
    float inPeak = -60.0f;
    float outPeak = -60.0f;

    static constexpr float peakHoldTime = 1.5f;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> vibeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> blendAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noiseAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gateThrAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gateDecayAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> noiseTypeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> advancedAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> padAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveTrimAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tapeTrimAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> vinylTrimAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noiseTrimAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibeFinisherAudioProcessorEditor)
};
