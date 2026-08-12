#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "../Params.h"

class GateScope : public juce::Component
{
public:
    explicit GateScope (juce::AudioProcessorValueTreeState& apvtsRef)
        : apvts (apvtsRef)
    {
        setInterceptsMouseClicks (true, false);
        setRepaintsOnMouseActivity (true);
        inBuffer.fill (-60.0f);
        outBuffer.fill (-60.0f);
    }

    void push (float inDb, float outDb)
    {
        const auto idx = static_cast<size_t> (writeIndex);
        inBuffer[idx] = inDb;
        outBuffer[idx] = outDb;
        writeIndex = (writeIndex + 1) % N;
    }

    void paint (juce::Graphics& g) override
    {
        const auto area = getLocalBounds().toFloat();
        const float thrDb = getThresholdDb();
        const float thrY = dbToY (thrDb, area);

        static const auto colPanel      = juce::Colour (0xff181b20);
        static const auto colTrack      = juce::Colour (0xff333844);
        static const auto colAccent     = juce::Colour (0xffff7043);
        static const auto colTextDim    = juce::Colour (0xff9aa0ab);
        static const auto colTextBright = juce::Colour (0xffe8eaf0);

        g.setColour (colPanel);
        g.fillRoundedRectangle (area, 4.0f);
        g.setColour (colTrack);
        g.drawRoundedRectangle (area, 4.0f, 1.0f);

        {
            g.setColour (colTrack.withAlpha (0.35f));
            for (float db : { -48.0f, -36.0f, -24.0f, -12.0f })
            {
                const float y = dbToY (db, area);
                g.drawLine (area.getX() + 1.0f, y, area.getRight() - 1.0f, y, 0.5f);
            }
        }

        {
            const float refCenterDb = getRefCenterDb();
            constexpr float refHalfDb = 3.0f;
            const float refTop = dbToY (refCenterDb - refHalfDb, area);
            const float refBot = dbToY (refCenterDb + refHalfDb, area);
            g.setColour (colTextDim.withAlpha (0.07f));
            g.fillRect (area.getX(), refTop, area.getWidth(), refBot - refTop);

            const float refMid = dbToY (refCenterDb, area);
            g.setColour (colTextDim.withAlpha (0.18f));
            g.drawLine (area.getX() + 1.0f, refMid, area.getRight() - 1.0f, refMid, 0.5f);

            g.setColour (colTextDim.withAlpha (0.45f));
            g.setFont (juce::Font (juce::FontOptions (7.0f)));
            g.drawText ("REF", static_cast<int> (area.getRight()) - 32,
                        static_cast<int> (refMid) - 6, 28, 8,
                        juce::Justification::centredRight);
        }

        {
            juce::Path outPath;
            for (int i = 1; i <= N; ++i)
            {
                const size_t idx = bufIdx (i);
                const float x = area.getX() + area.getWidth() * static_cast<float> (i - 1) / static_cast<float> (N - 1);
                const float y = dbToY (outBuffer[idx], area);

                if (i == 1)
                    outPath.startNewSubPath (x, y);
                else
                    outPath.lineTo (x, y);
            }
            g.setColour (colTextDim.withAlpha (0.4f));
            g.strokePath (outPath, juce::PathStrokeType (0.8f));
        }

        const auto thrPx = static_cast<int> (area.getRight() + 0.5f);
        for (int px = static_cast<int> (area.getX()); px < thrPx; ++px)
        {
            const float normX = (static_cast<float> (px) - area.getX()) / area.getWidth();
            const int i = static_cast<int> (normX * static_cast<float> (N - 1));
            const size_t idx = bufIdx (1 + i);
            const float sampleDb = inBuffer[idx];
            if (sampleDb > thrDb)
            {
                const float top = dbToY (sampleDb, area);
                g.setColour (colAccent.withAlpha (0.18f));
                g.fillRect (static_cast<float> (px), top,
                           1.0f, juce::jmax (0.0f, thrY - top));
            }
        }

        {
            juce::Path inPath;
            for (int i = 1; i <= N; ++i)
            {
                const size_t idx = bufIdx (i);
                const float x = area.getX() + area.getWidth() * static_cast<float> (i - 1) / static_cast<float> (N - 1);
                const float y = dbToY (inBuffer[idx], area);

                if (i == 1)
                    inPath.startNewSubPath (x, y);
                else
                    inPath.lineTo (x, y);
            }
            g.setColour (colAccent);
            g.strokePath (inPath, juce::PathStrokeType (1.5f));
        }

        {
            juce::Path thrPath;
            const float dashLen = 5.0f;
            for (float x = area.getX(); x < area.getRight(); x += dashLen * 2.0f)
            {
                const auto seg = juce::Line<float> (x, thrY,
                                                    juce::jmin (x + dashLen, area.getRight()), thrY);
                thrPath.addLineSegment (seg, 1.0f);
            }
            g.setColour (colTextBright);
            g.strokePath (thrPath, juce::PathStrokeType (1.0f));

            const float hx = area.getRight() - 8.0f;
            juce::Path handle;
            handle.addTriangle (hx - 4.5f, thrY - 5.0f,
                                hx + 4.5f, thrY - 5.0f,
                                hx,       thrY + 1.0f);
            g.fillPath (handle);

            g.setColour (colTextDim);
            g.setFont (juce::Font (juce::FontOptions (8.0f)));
            g.drawText (juce::String (static_cast<int> (thrDb)) + " dB",
                        static_cast<int> (area.getRight()) - 44,
                        static_cast<int> (thrY) - 10, 40, 10,
                        juce::Justification::centredRight);
        }

        {
            const float lastIn = inBuffer[bufIdx (N - 1)];
            const float lastOut = outBuffer[bufIdx (N - 1)];

            g.setColour (colTextDim);
            g.setFont (juce::Font (juce::FontOptions (8.0f)));

            g.drawText ("IN  " + juce::String (static_cast<int> (lastIn)) + " dB",
                        static_cast<int> (area.getX()) + 4,
                        static_cast<int> (area.getY()) + 2,
                        56, 12, juce::Justification::left);

            g.drawText ("OUT " + juce::String (static_cast<int> (lastOut)) + " dB",
                        static_cast<int> (area.getX()) + 4,
                        static_cast<int> (area.getBottom()) - 14,
                        56, 12, juce::Justification::left);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const auto area = getLocalBounds().toFloat();
        const float thrY = dbToY (getThresholdDb(), area);

        if (std::abs (e.position.y - thrY) < 7.0f)
        {
            dragging = true;
            if (auto* param = apvts.getParameter (Params::noiseGateThr))
                param->beginChangeGesture();
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! dragging)
            return;

        const auto area = getLocalBounds().toFloat();
        const float newDb = juce::jlimit (minDb, maxDb, yToDb (e.position.y, area));

        if (auto* param = apvts.getParameter (Params::noiseGateThr))
            param->setValueNotifyingHost (param->convertTo0to1 (newDb));

        repaint();
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (! dragging)
            return;

        dragging = false;
        if (auto* param = apvts.getParameter (Params::noiseGateThr))
            param->endChangeGesture();
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const auto area = getLocalBounds().toFloat();
        const float thrY = dbToY (getThresholdDb(), area);

        if (std::abs (e.position.y - thrY) < 7.0f)
            setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        else
            setMouseCursor (juce::MouseCursor::NormalCursor);
    }

private:
    static constexpr int N = 360;
    static constexpr float minDb = -60.0f;
    static constexpr float maxDb = 0.0f;

    juce::AudioProcessorValueTreeState& apvts;
    std::array<float, N> inBuffer;
    std::array<float, N> outBuffer;
    int writeIndex = 0;
    bool dragging = false;

    float getThresholdDb() const
    {
        return apvts.getRawParameterValue (Params::noiseGateThr)->load();
    }

    float getRefCenterDb() const
    {
        constexpr float internalRefDb = -18.0f;
        const bool padOn = apvts.getRawParameterValue (Params::calPad)->load() > 0.5f;
        return padOn ? internalRefDb - Params::calibrationTrimDb : internalRefDb;
    }

    size_t bufIdx (int offset) const
    {
        return static_cast<size_t> ((writeIndex + offset) % N);
    }

    float dbToY (float db, const juce::Rectangle<float>& area) const
    {
        return juce::jmap (juce::jlimit (minDb, maxDb, db), minDb, maxDb, area.getBottom(), area.getY());
    }

    float yToDb (float y, const juce::Rectangle<float>& area) const
    {
        return juce::jmap (y, area.getBottom(), area.getY(), minDb, maxDb);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GateScope)
};
