#include "PluginEditor.h"

VibeFinisherAudioProcessorEditor::VibeFinisherAudioProcessorEditor (VibeFinisherAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&laf);
    setSize (520, 460);

    const auto initKnob = [this] (juce::Slider& s)
    {
        addAndMakeVisible (s);
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    };

    const auto initLabel = [this] (juce::Label& l, const juce::String& text)
    {
        addAndMakeVisible (l);
        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (juce::Font (juce::FontOptions (9.0f).withStyle ("Bold")));
        l.setColour (juce::Label::textColourId, Colours::textDim);
    };

    initKnob (vibeSlider);
    vibeSlider.setLookAndFeel (&laf);
    initLabel (vibeLabel, "VIBE");

    // Row 1: creative controls
    initKnob (driveSlider); initLabel (driveLabel, "DRIVE");
    initKnob (blendSlider); initLabel (blendLabel, "TP/VNL");
    initKnob (noiseSlider); initLabel (noiseLabel, "NOISE");
    initKnob (gateThrSlider); initLabel (gateThrLabel, "GATE");

    // Row 2: utility controls
    initKnob (inputSlider);  initLabel (inputLabel, "INPUT");
    initKnob (outputSlider); initLabel (outputLabel, "OUTPUT");
    initKnob (gateDecaySlider); initLabel (gateDecayLabel, "DECAY");

    initKnob (mixSlider);   initLabel (mixLabel, "MIX");

    addAndMakeVisible (noiseTypeBox);
    noiseTypeBox.addItemList ({ "Tape Hiss", "Vinyl", "Console", "Digital" }, 1);
    noiseTypeBox.setLookAndFeel (&laf);

    addAndMakeVisible (advancedButton);
    advancedButton.setButtonText ("ADV");
    advancedButton.setClickingTogglesState (true);
    advancedButton.setLookAndFeel (&laf);
    advancedButton.onClick = [this]
    {
        advancedVisible = advancedButton.getToggleState();
        resized();
    };

    // Advanced trims
    initKnob (driveTrimSlider); initLabel (driveTrimLabel, "DRV TRIM");
    initKnob (tapeTrimSlider);  initLabel (tapeTrimLabel, "TAPE TRIM");
    initKnob (vinylTrimSlider); initLabel (vinylTrimLabel, "VNL TRIM");
    initKnob (noiseTrimSlider); initLabel (noiseTrimLabel, "NOI TRIM");

    vibeAttach       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processorRef.apvts, Params::vibe, vibeSlider);
    inputAttach      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processorRef.apvts, Params::inputGain, inputSlider);
    driveAttach      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processorRef.apvts, Params::drive, driveSlider);
    blendAttach      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processorRef.apvts, Params::blend, blendSlider);
    noiseAttach      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processorRef.apvts, Params::noiseLevel, noiseSlider);
    gateThrAttach    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processorRef.apvts, Params::noiseGateThr, gateThrSlider);
    gateDecayAttach  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processorRef.apvts, Params::noiseGateDecay, gateDecaySlider);
    outputAttach     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processorRef.apvts, Params::outputGain, outputSlider);
    mixAttach        = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processorRef.apvts, Params::mix, mixSlider);
    noiseTypeAttach  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processorRef.apvts, Params::noiseType, noiseTypeBox);
    advancedAttach   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processorRef.apvts, Params::advanced, advancedButton);
    driveTrimAttach  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processorRef.apvts, Params::driveTrim, driveTrimSlider);
    tapeTrimAttach   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processorRef.apvts, Params::tapeTrim, tapeTrimSlider);
    vinylTrimAttach  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processorRef.apvts, Params::vinylTrim, vinylTrimSlider);
    noiseTrimAttach  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processorRef.apvts, Params::noiseTrim, noiseTrimSlider);

    advancedVisible = processorRef.apvts.getRawParameterValue (Params::advanced)->load() > 0.5f;
    advancedButton.setToggleState (advancedVisible, juce::dontSendNotification);

    resized();
    startTimerHz (30);
}

VibeFinisherAudioProcessorEditor::~VibeFinisherAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
    vibeSlider.setLookAndFeel (nullptr);
    noiseTypeBox.setLookAndFeel (nullptr);
    advancedButton.setLookAndFeel (nullptr);
}

void VibeFinisherAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (Colours::bg);

    g.setColour (Colours::panel);
    g.fillRoundedRectangle (12.0f, 12.0f, (float) getWidth() - 24.0f, (float) getHeight() - 24.0f, 8.0f);

    g.setColour (Colours::accent);
    g.setFont (juce::Font (juce::FontOptions (12.0f).withStyle ("Bold")));
    g.drawText ("VIBEFINISHER", 24, 10, 200, 16, juce::Justification::centredLeft);

    const auto drawBar = [&] (juce::Rectangle<float> barRect, float levelDb, float peakDb,
                               const juce::String& label, bool isInput)
    {
        g.setColour (Colours::track);
        g.fillRoundedRectangle (barRect, 2.0f);

        juce::ColourGradient grad;
        grad.addColour (0.0f, juce::Colour (0xff4caf50));
        grad.addColour (0.6f, juce::Colour (0xfff4d03f));
        grad.addColour (0.85f, juce::Colour (0xffff7043));
        grad.addColour (1.0f, juce::Colour (0xffe74c3c));
        grad.point1 = barRect.getTopLeft();
        grad.point2 = barRect.getTopRight();

        const auto levelFrac = juce::jlimit (0.0f, 1.0f, (levelDb - meterMinDb) / (meterMaxDb - meterMinDb));
        if (levelFrac > 0.001f)
        {
            g.setGradientFill (grad);
            g.fillRoundedRectangle (barRect.withRight (barRect.getX() + barRect.getWidth() * levelFrac), 2.0f);
        }

        const auto peakFrac = juce::jlimit (0.0f, 1.0f, (peakDb - meterMinDb) / (meterMaxDb - meterMinDb));
        if (peakFrac > 0.01f && peakFrac > levelFrac)
        {
            const float px = barRect.getX() + barRect.getWidth() * peakFrac;
            g.setColour (Colours::textBright);
            g.drawLine (px, barRect.getY(), px, barRect.getBottom(), 1.5f);
        }

        if (isInput)
        {
            const auto gateThrDb = processorRef.apvts.getRawParameterValue (Params::noiseGateThr)->load();
            const auto gateFrac = (gateThrDb - meterMinDb) / (meterMaxDb - meterMinDb);
            const float gateX = barRect.getX() + barRect.getWidth() * gateFrac;
            g.setColour (Colours::textBright);
            g.drawLine (gateX, barRect.getBottom(), gateX, barRect.getBottom() + 2.0f, 1.5f);
        }

        g.setColour (Colours::textDim);
        g.setFont (juce::Font (juce::FontOptions (8.0f)));
        g.drawText (label, (int) barRect.getX() - 18, (int) barRect.getY() - 1, 18, (int) barRect.getHeight(),
                    juce::Justification::centredLeft);

        g.drawText (juce::String (static_cast<int> (levelDb)) + " dB",
                    (int) barRect.getRight() + 2, (int) barRect.getY() - 1, 46, (int) barRect.getHeight(),
                    juce::Justification::centredLeft);
    };

    const auto b = meterBounds;
    const float barH = 6.0f;
    const float gap = 2.0f;
    drawBar (juce::Rectangle<float> (b.getX(), b.getY(), b.getWidth(), barH), inMeterLevel, inPeak, "IN", true);
    drawBar (juce::Rectangle<float> (b.getX(), b.getY() + barH + gap, b.getWidth(), barH), outMeterLevel, outPeak, "OUT", false);
}

void VibeFinisherAudioProcessorEditor::timerCallback()
{
    const auto dt = 1.0f / 30.0f;
    const auto releaseCoeff = std::exp (-dt / meterReleaseTime);
    const auto peakReleaseCoeff = std::exp (-dt / peakHoldTime);

    const auto update = [&] (float& level, float& peak, float newDb)
    {
        if (newDb > level)
            level = newDb;
        else
            level += releaseCoeff * (newDb - level);

        if (newDb > peak)
            peak = newDb;
        else
            peak += peakReleaseCoeff * (newDb - peak);

        level = juce::jmax (level, meterMinDb);
        peak = juce::jmax (peak, meterMinDb);
    };

    update (inMeterLevel, inPeak, processorRef.inputLevel.load (std::memory_order_relaxed));
    update (outMeterLevel, outPeak, processorRef.outputLevel.load (std::memory_order_relaxed));

    repaint();
}

void VibeFinisherAudioProcessorEditor::resized()
{
    constexpr int knobW = 68;
    constexpr int knobH = 68;
    constexpr int labelH = 14;
    constexpr int labelW = 80;
    constexpr int margin = 20;
    const int totalW = getWidth() - margin * 2;

    meterBounds = juce::Rectangle<float> (static_cast<float> (margin), 28.0f,
                                           static_cast<float> (totalW), 14.0f);

    // --- Vibe knob (big, centered) ---
    constexpr int bigKnobSize = 96;
    const int vibeY = 46;
    vibeSlider.setBounds ((getWidth() - bigKnobSize) / 2, vibeY, bigKnobSize, bigKnobSize);
    vibeLabel.setBounds ((getWidth() - labelW) / 2, vibeY + bigKnobSize, labelW, labelH);

    // --- Row 1: creative controls (4 knobs + ADV button) ---
    const int numRow1 = 5;
    const int row1Y = vibeY + bigKnobSize + labelH + 10;
    const int row1Spacing = (totalW - numRow1 * knobW) / (numRow1 + 1);

    juce::Slider* row1Knobs[] = { &driveSlider, &blendSlider, &noiseSlider, &gateThrSlider, nullptr };
    juce::Label* row1Labels[] = { &driveLabel, &blendLabel, &noiseLabel, &gateThrLabel, nullptr };

    for (int i = 0; i < 4; ++i)
    {
        const int x = margin + row1Spacing + i * (knobW + row1Spacing);
        row1Knobs[i]->setBounds (x, row1Y, knobW, knobH);
        row1Labels[i]->setBounds (x - (labelW - knobW) / 2, row1Y + knobH, labelW, labelH);
    }

    {
        const int x = margin + row1Spacing + 4 * (knobW + row1Spacing);
        const int advW = 62;
        advancedButton.setBounds (x + (knobW - advW) / 2, row1Y + 18, advW, 24);
    }

    // --- Row 2: utility controls (3 knobs + combo + MIX) ---
    const int numRow2 = 5;
    const int row2Y = row1Y + knobH + labelH + 11;
    const int row2Spacing = (totalW - numRow2 * knobW) / (numRow2 + 1);

    juce::Slider* row2Knobs[] = { &inputSlider, &outputSlider, &gateDecaySlider, nullptr, &mixSlider };
    juce::Label* row2Labels[] = { &inputLabel, &outputLabel, &gateDecayLabel, nullptr, &mixLabel };

    for (int i = 0; i < 3; ++i)
    {
        const int x = margin + row2Spacing + i * (knobW + row2Spacing);
        row2Knobs[i]->setBounds (x, row2Y, knobW, knobH);
        row2Labels[i]->setBounds (x - (labelW - knobW) / 2, row2Y + knobH, labelW, labelH);
    }

    const int mixSlotX = margin + row2Spacing + 4 * (knobW + row2Spacing);
    row2Knobs[4]->setBounds (mixSlotX, row2Y, knobW, knobH);
    row2Labels[4]->setBounds (mixSlotX - (labelW - knobW) / 2, row2Y + knobH, labelW, labelH);

    const int comboSlotX = margin + row2Spacing + 3 * (knobW + row2Spacing);
    const int comboW = 100;
    noiseTypeBox.setBounds (comboSlotX + (knobW - comboW) / 2, row2Y + 18, comboW, 24);

    // --- Advanced panel ---
    const int advRowY = row2Y + knobH + labelH + 6;
    const bool show = advancedVisible;

    const int numAdv = 4;
    const int advSpacing = (totalW - numAdv * knobW) / (numAdv + 1);

    juce::Slider* advKnobs[] = { &driveTrimSlider, &tapeTrimSlider, &vinylTrimSlider, &noiseTrimSlider };
    juce::Label* advLabels[] = { &driveTrimLabel, &tapeTrimLabel, &vinylTrimLabel, &noiseTrimLabel };

    for (int i = 0; i < numAdv; ++i)
    {
        advKnobs[i]->setVisible (show);
        advLabels[i]->setVisible (show);
        if (show)
        {
            const int x = margin + advSpacing + i * (knobW + advSpacing);
            advKnobs[i]->setBounds (x, advRowY, knobW, knobH);
            advLabels[i]->setBounds (x - (labelW - knobW) / 2, advRowY + knobH, labelW, labelH);
        }
    }
}
