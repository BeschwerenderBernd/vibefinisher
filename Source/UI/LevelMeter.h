#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../Params.h"

class LevelMeter : public juce::Component
{
public:
    LevelMeter()
    {
        setInterceptsMouseClicks (false, false);
    }

    void setLevels (float levelDb, float peakDb)
    {
        level = juce::jlimit (minDb, maxDb, levelDb);
        peak  = juce::jlimit (minDb, maxDb, peakDb);
        repaint();
    }

    void setShowRefTick (float refDb)
    {
        showRef = true;
        refDb_ = refDb;
    }

    void paint (juce::Graphics& g) override
    {
        static const auto colTrack  = juce::Colour (0xff333844);
        static const auto colBright = juce::Colour (0xffe8eaf0);
        static const auto colDim    = juce::Colour (0xff9aa0ab);

        const auto area = getLocalBounds().toFloat();
        const float barW = juce::jmin (8.0f, area.getWidth());
        const auto barRect = juce::Rectangle<float> (
            area.getX() + (area.getWidth() - barW) * 0.5f,
            area.getY() + 1.0f,
            barW,
            area.getHeight() - 2.0f);

        g.setColour (colTrack);
        g.fillRoundedRectangle (barRect, 1.5f);

        const float frac = juce::jmap (level, minDb, maxDb, 0.0f, 1.0f);
        if (frac > 0.003f)
        {
            juce::ColourGradient grad;
            grad.addColour (0.0f,  juce::Colour (0xff4caf50));
            grad.addColour (0.6f,  juce::Colour (0xfff4d03f));
            grad.addColour (0.85f, juce::Colour (0xffff7043));
            grad.addColour (1.0f,  juce::Colour (0xffe74c3c));
            grad.point1 = barRect.getBottomLeft();
            grad.point2 = barRect.getTopLeft();

            const float fillH = barRect.getHeight() * frac;
            const auto fillRect = barRect.withTop (barRect.getBottom() - fillH);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (fillRect, 1.5f);
        }

        const float peakFrac = juce::jmap (peak, minDb, maxDb, 0.0f, 1.0f);
        if (peakFrac > 0.005f)
        {
            const float peakY = barRect.getBottom() - barRect.getHeight() * peakFrac;
            g.setColour (colBright);
            g.drawLine (barRect.getX(), peakY, barRect.getRight(), peakY, 1.5f);
        }

        if (showRef)
        {
            const float refY = juce::jmap (juce::jlimit (minDb, maxDb, refDb_),
                                           minDb, maxDb,
                                           barRect.getBottom(), barRect.getY());
            g.setColour (colDim.withAlpha (0.5f));
            g.drawLine (barRect.getRight(), refY, barRect.getRight() + 3.0f, refY, 1.0f);
        }
    }

private:
    static constexpr float minDb = -60.0f;
    static constexpr float maxDb = 0.0f;

    float level = minDb;
    float peak = minDb;
    float refDb_ = -12.0f;
    bool showRef = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeter)
};
