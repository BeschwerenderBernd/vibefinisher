#include "PluginEditor.h"

static constexpr int knobW = 68;
static constexpr int knobH = 68;
static constexpr int vibeW = 96;
static constexpr int vibeH = 96;
static constexpr int labelH = 14;
static constexpr int margin = 12;
static constexpr int rowGap = 12;
static constexpr int scopeH = 68;

VibeFinisherAudioProcessorEditor::VibeFinisherAudioProcessorEditor (VibeFinisherAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&laf);

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

    scope = std::make_unique<GateScope> (processorRef.apvts);
    addAndMakeVisible (*scope);

    addAndMakeVisible (inMeter);
    padState = processorRef.apvts.getRawParameterValue (Params::calPad)->load() > 0.5f;
    updateRefTick();
    addAndMakeVisible (outMeter);

    initKnob (vibeSlider);
    vibeSlider.setLookAndFeel (&laf);
    initLabel (vibeLabel, "VIBE");

    initKnob (inputSlider);  initLabel (inputLabel, "INPUT");
    inputSlider.setTooltip ("Level hitting the saturation stages. Aim for the meter tick: around "
                            "-12 dBFS RMS with the pad on, around -18 dBFS with it off.");
    initKnob (outputSlider); initLabel (outputLabel, "OUTPUT");

    initKnob (driveSlider); initLabel (driveLabel, "DRIVE");
    initKnob (blendSlider); initLabel (blendLabel, "TP/VNL");

    initKnob (noiseSlider); initLabel (noiseLabel, "NOISE");
    initKnob (gateThrSlider); initLabel (gateThrLabel, "GATE");
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
        updateSize();
        resized();
    };

    addAndMakeVisible (padButton);
    padButton.setButtonText ("PAD");
    padButton.setClickingTogglesState (true);
    padButton.setLookAndFeel (&laf);
    padButton.setTooltip ("Headroom pad: attenuates the signal by 6 dB before the saturation "
                          "stages (made up afterwards) so they are not driven too hot. Disable to "
                          "drive them 6 dB hotter for more grit. Ideal input with pad on: around "
                          "-12 dBFS RMS (meter tick).");

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
    padAttach        = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processorRef.apvts, Params::calPad, padButton);
    driveTrimAttach  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processorRef.apvts, Params::driveTrim, driveTrimSlider);
    tapeTrimAttach   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processorRef.apvts, Params::tapeTrim, tapeTrimSlider);
    vinylTrimAttach  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processorRef.apvts, Params::vinylTrim, vinylTrimSlider);
    noiseTrimAttach  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processorRef.apvts, Params::noiseTrim, noiseTrimSlider);

    advancedVisible = processorRef.apvts.getRawParameterValue (Params::advanced)->load() > 0.5f;
    advancedButton.setToggleState (advancedVisible, juce::dontSendNotification);
    padButton.setToggleState (padState, juce::dontSendNotification);

    inputSlider.setDoubleClickReturnValue (true, 0.0);
    vibeSlider.setDoubleClickReturnValue (true, 50.0);
    driveSlider.setDoubleClickReturnValue (true, 5.0);
    blendSlider.setDoubleClickReturnValue (true, 50.0);
    noiseSlider.setDoubleClickReturnValue (true, 10.0);
    gateThrSlider.setDoubleClickReturnValue (true, -40.0);
    gateDecaySlider.setDoubleClickReturnValue (true, 500.0);
    outputSlider.setDoubleClickReturnValue (true, 0.0);
    mixSlider.setDoubleClickReturnValue (true, 100.0);
    driveTrimSlider.setDoubleClickReturnValue (true, 0.0);
    tapeTrimSlider.setDoubleClickReturnValue (true, 0.0);
    vinylTrimSlider.setDoubleClickReturnValue (true, 0.0);
    noiseTrimSlider.setDoubleClickReturnValue (true, 0.0);

    updateSize();
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
    padButton.setLookAndFeel (nullptr);
}

void VibeFinisherAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (Colours::bg);

    g.setColour (Colours::panel);
    g.fillRoundedRectangle ((float) margin, (float) margin,
                            (float) getWidth() - margin * 2.0f,
                            (float) getHeight() - margin * 2.0f, 8.0f);

    g.setColour (Colours::accent);
    g.setFont (juce::Font (juce::FontOptions (12.0f).withStyle ("Bold")));
    g.drawText ("VIBEFINISHER", margin + 12, 10, 200, 16, juce::Justification::centredLeft);
}

void VibeFinisherAudioProcessorEditor::timerCallback()
{
    const auto dt = 1.0f / 30.0f;
    const auto releaseCoeff = std::exp (-dt / 0.3f);
    const auto peakReleaseCoeff = std::exp (-dt / peakHoldTime);

    const auto update = [&] (float& level, float& pk, float newDb)
    {
        if (newDb > level)
            level = newDb;
        else
            level += releaseCoeff * (newDb - level);

        if (newDb > pk)
            pk = newDb;
        else
            pk += peakReleaseCoeff * (newDb - pk);

        level = juce::jmax (level, -60.0f);
        pk = juce::jmax (pk, -60.0f);
    };

    update (inMeterLevel, inPeak, processorRef.inputLevel.load (std::memory_order_relaxed));
    update (outMeterLevel, outPeak, processorRef.outputLevel.load (std::memory_order_relaxed));

    scope->push (inMeterLevel, outMeterLevel);
    scope->repaint();

    const auto padOn = processorRef.apvts.getRawParameterValue (Params::calPad)->load() > 0.5f;
    if (padOn != padState)
    {
        padState = padOn;
        updateRefTick();
    }

    inMeter.setLevels (inMeterLevel, inPeak);
    outMeter.setLevels (outMeterLevel, outPeak);
}

void VibeFinisherAudioProcessorEditor::updateRefTick()
{
    constexpr float internalRefDb = -18.0f;
    inMeter.setShowRefTick (padState ? internalRefDb - Params::calibrationTrimDb : internalRefDb);
}

void VibeFinisherAudioProcessorEditor::updateSize()
{
    constexpr int scopeRowY = 38;
    const int vibeRowY = scopeRowY + scopeH + labelH + rowGap;
    const int noiseRowY = vibeRowY + vibeH + labelH + rowGap;
    const int noiseRowBottom = noiseRowY + knobH + labelH;

    if (advancedVisible)
    {
        const int advRowY = noiseRowY + knobH + labelH + rowGap;
        setSize (520, advRowY + knobH + labelH + margin);
    }
    else
    {
        setSize (520, noiseRowBottom + margin);
    }
}

void VibeFinisherAudioProcessorEditor::resized()
{
    const int w = getWidth();
    const int innerW = w - margin * 2;

    // --- Header: title left, PAD + ADV buttons right ---
    constexpr int headerY = 14;
    constexpr int hdrBtnW = 46;
    constexpr int hdrGap = 6;
    advancedButton.setBounds (w - margin - hdrBtnW, headerY, hdrBtnW, 18);
    padButton.setBounds (w - margin - hdrBtnW * 2 - hdrGap, headerY, hdrBtnW, 18);

    // --- Scope row: INPUT | meter | scope | meter | OUTPUT ---
    constexpr int meterW = 8;
    constexpr int meterGap = 4;
    constexpr int scopeGap = 8;
    const int scopeRowY = 38;
    const int leftClusterW = knobW + meterGap + meterW + scopeGap;
    const int rightClusterW = scopeGap + meterW + meterGap + knobW;
    const int scopeX = margin + leftClusterW;
    const int scopeW = innerW - leftClusterW - rightClusterW;

    inputSlider.setBounds (margin, scopeRowY, knobW, knobH);
    inputLabel.setBounds (margin, scopeRowY + knobH, knobW, labelH);

    inMeter.setBounds (margin + knobW + meterGap, scopeRowY, meterW, knobH);

    scope->setBounds (scopeX, scopeRowY, scopeW, scopeH);

    const int outMeterX = scopeX + scopeW + scopeGap;
    outMeter.setBounds (outMeterX, scopeRowY, meterW, knobH);

    const int outputX = outMeterX + meterW + meterGap;
    outputSlider.setBounds (outputX, scopeRowY, knobW, knobH);
    outputLabel.setBounds (outputX, scopeRowY + knobH, knobW, labelH);

    // --- Vibe row: DRIVE | VIBE | TP/VNL (centered group) ---
    const int vibeRowY = scopeRowY + scopeH + labelH + rowGap;
    const int driveVibeGap = 12;
    const int groupWidth = knobW + driveVibeGap + vibeW + driveVibeGap + knobW;
    const int groupStartX = margin + (innerW - groupWidth) / 2;

    const int driveKnobY = vibeRowY + (vibeH - knobH) / 2;
    driveSlider.setBounds (groupStartX, driveKnobY, knobW, knobH);
    driveLabel.setBounds (groupStartX, vibeRowY + vibeH, knobW, labelH);

    const int vibeX = groupStartX + knobW + driveVibeGap;
    vibeSlider.setBounds (vibeX, vibeRowY, vibeW, vibeH);
    vibeLabel.setBounds (vibeX + (vibeW - knobW) / 2, vibeRowY + vibeH, knobW, labelH);

    const int blendX = vibeX + vibeW + driveVibeGap;
    blendSlider.setBounds (blendX, driveKnobY, knobW, knobH);
    blendLabel.setBounds (blendX, vibeRowY + vibeH, knobW, labelH);

    // --- Noise row: NOISE | [Type] | GATE | DECAY | MIX ---
    const int noiseRowY = vibeRowY + vibeH + labelH + rowGap;
    const int comboW = 88;
    const int comboH = 24;
    const int numNoiseSlots = 5;
    const int totalNoiseSlotW = knobW * 4 + comboW;
    const int noiseGap = (innerW - totalNoiseSlotW) / (numNoiseSlots + 1);

    int nx = margin + noiseGap;
    noiseSlider.setBounds (nx, noiseRowY, knobW, knobH);
    noiseLabel.setBounds (nx, noiseRowY + knobH, knobW, labelH);

    nx += knobW + noiseGap;
    noiseTypeBox.setBounds (nx, noiseRowY + (knobH - comboH) / 2, comboW, comboH);

    nx += comboW + noiseGap;
    gateThrSlider.setBounds (nx, noiseRowY, knobW, knobH);
    gateThrLabel.setBounds (nx, noiseRowY + knobH, knobW, labelH);

    nx += knobW + noiseGap;
    gateDecaySlider.setBounds (nx, noiseRowY, knobW, knobH);
    gateDecayLabel.setBounds (nx, noiseRowY + knobH, knobW, labelH);

    nx += knobW + noiseGap;
    mixSlider.setBounds (nx, noiseRowY, knobW, knobH);
    mixLabel.setBounds (nx, noiseRowY + knobH, knobW, labelH);

    // --- Advanced row ---
    const int advRowY = noiseRowY + knobH + labelH + rowGap;
    const bool show = advancedVisible;
    const int numAdv = 4;
    const int advGap = (innerW - numAdv * knobW) / (numAdv + 1);

    juce::Slider* advKnobs[] = { &driveTrimSlider, &tapeTrimSlider, &vinylTrimSlider, &noiseTrimSlider };
    juce::Label* advLabels[] = { &driveTrimLabel, &tapeTrimLabel, &vinylTrimLabel, &noiseTrimLabel };

    for (int i = 0; i < numAdv; ++i)
    {
        advKnobs[i]->setVisible (show);
        advLabels[i]->setVisible (show);
        if (show)
        {
            const int x = margin + advGap + i * (knobW + advGap);
            advKnobs[i]->setBounds (x, advRowY, knobW, knobH);
            advLabels[i]->setBounds (x, advRowY + knobH, knobW, labelH);
        }
    }
}
