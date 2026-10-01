/*
  ==============================================================================

    EQGraphComponent.cpp

  ==============================================================================
*/

#include "EQGraphComponent.h"
#include "../DSP/SpectrumAnalyzer.h"
#include <array>

namespace eq
{

namespace
{
    juce::String formatFreqLabel (float freq)
    {
        if (freq >= 1000.0f)
        {
            auto khz = freq / 1000.0f;
            auto isWhole = std::abs (khz - std::round (khz)) < 0.01f;
            return (isWhole ? juce::String ((int) std::round (khz)) : juce::String (khz, 1)) + "k";
        }

        return juce::String ((int) freq);
    }
}

//==============================================================================
EQGraphComponent::EQGraphComponent (EQ1AudioProcessor& processorToUse)
    : audioProcessor (processorToUse)
{
    setWantsKeyboardFocus (true);
    setOpaque (true);
    startTimerHz (30);
}

EQGraphComponent::~EQGraphComponent()
{
    stopTimer();
}

//==============================================================================
void EQGraphComponent::timerCallback()
{
    audioProcessor.preAnalyzer.updateIfReady();
    audioProcessor.postAnalyzer.updateIfReady();
    repaint();
}

//==============================================================================
float EQGraphComponent::freqToX (float freq) const
{
    auto b = getLocalBounds().toFloat();
    auto proportion = std::log (freq / minFreq) / std::log (maxFreq / minFreq);
    return b.getX() + proportion * b.getWidth();
}

float EQGraphComponent::xToFreq (float x) const
{
    auto b = getLocalBounds().toFloat();
    auto proportion = juce::jlimit (0.0f, 1.0f, (x - b.getX()) / juce::jmax (1.0f, b.getWidth()));
    return minFreq * std::pow (maxFreq / minFreq, proportion);
}

float EQGraphComponent::gainToY (float gainDb) const
{
    auto b = getLocalBounds().toFloat();
    auto clamped = juce::jlimit (minDb, maxDb, gainDb);
    return juce::jmap (clamped, minDb, maxDb, b.getBottom(), b.getY());
}

float EQGraphComponent::yToGain (float y) const
{
    auto b = getLocalBounds().toFloat();
    auto clampedY = juce::jlimit (b.getY(), b.getBottom(), y);
    return juce::jmap (clampedY, b.getBottom(), b.getY(), minDb, maxDb);
}

//==============================================================================
void EQGraphComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.fillAll (juce::Colour (0xff1a1c20));

    drawGrid (g, bounds);
    drawAnalyzer (g, bounds);
    drawCurve (g, bounds);
    drawNodes (g);
    drawInfoPanel (g, bounds);
}

void EQGraphComponent::drawGrid (juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    static constexpr float freqTicks[] = { 20, 30, 50, 100, 200, 300, 500, 1000, 2000, 3000, 5000, 10000, 20000 };
    static constexpr float dbTicks[]   = { -24, -18, -12, -6, 0, 6, 12, 18, 24 };

    g.setFont (juce::Font (10.0f));

    for (auto freq : freqTicks)
    {
        auto x = freqToX (freq);
        bool isMajor = (freq == 100.0f || freq == 1000.0f || freq == 10000.0f);

        g.setColour (juce::Colours::white.withAlpha (isMajor ? 0.16f : 0.07f));
        g.drawVerticalLine ((int) x, bounds.getY(), bounds.getBottom());

        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.drawText (formatFreqLabel (freq),
                    juce::Rectangle<float> (x - 16.0f, bounds.getBottom() - 14.0f, 32.0f, 12.0f),
                    juce::Justification::centred);
    }

    for (auto db : dbTicks)
    {
        auto y = gainToY (db);

        g.setColour (juce::Colours::white.withAlpha (db == 0.0f ? 0.22f : 0.07f));
        g.drawHorizontalLine ((int) y, bounds.getX(), bounds.getRight());

        g.setColour (juce::Colours::white.withAlpha (0.4f));
        g.drawText (juce::String ((int) db),
                    juce::Rectangle<float> (bounds.getX() + 3.0f, y - 6.0f, 26.0f, 12.0f),
                    juce::Justification::centredLeft);
    }
}

void EQGraphComponent::drawAnalyzer (juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    auto sampleRate = audioProcessor.getSampleRate();
    if (sampleRate <= 0.0)
        sampleRate = 44100.0;

    auto mode = (int) audioProcessor.apvts.getRawParameterValue ("analyzerMode")->load();
    if (mode == 0)
        return;

    auto drawOne = [&] (const std::vector<float>& magnitudesDb, juce::Colour colour)
    {
        juce::Path path;
        bool started = false;

        // アナライザーのdBレンジ(-100〜+6dB)をEQカーブのゲイン軸(-24〜+24dB)に
        // マッピングして重ね描きする(絶対的なdB値ではなく相対的な山を見せる表示)
        for (int bin = 1; bin < SpectrumAnalyzer::numBins; ++bin)
        {
            auto freq = (float) (bin * sampleRate / (double) SpectrumAnalyzer::fftSize);
            if (freq < minFreq || freq > maxFreq)
                continue;

            auto mappedDb = juce::jmap (magnitudesDb[(size_t) bin], -100.0f, 6.0f, minDb, maxDb);
            auto x = freqToX (freq);
            auto y = gainToY (mappedDb);

            if (! started) { path.startNewSubPath (x, y); started = true; }
            else path.lineTo (x, y);
        }

        if (! started)
            return;

        g.setColour (colour.withAlpha (0.8f));
        g.strokePath (path, juce::PathStrokeType (1.0f));

        auto fillPath = path;
        fillPath.lineTo (bounds.getRight(), bounds.getBottom());
        fillPath.lineTo (bounds.getX(), bounds.getBottom());
        fillPath.closeSubPath();

        g.setColour (colour.withAlpha (0.10f));
        g.fillPath (fillPath);
    };

    if (mode == 1 || mode == 3)
        drawOne (audioProcessor.preAnalyzer.getMagnitudesDb(), juce::Colours::grey);

    if (mode == 2 || mode == 3)
        drawOne (audioProcessor.postAnalyzer.getMagnitudesDb(), juce::Colour (0xff4ca8ff));
}

void EQGraphComponent::drawCurve (juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    auto sampleRate = audioProcessor.getSampleRate();
    if (sampleRate <= 0.0)
        sampleRate = 44100.0;

    struct ActiveBand
    {
        std::array<eq::Coefficients::Ptr, eq::maxStagesPerBand> coeffs;
        int numStages = 0;
    };

    std::vector<ActiveBand> activeBands;
    activeBands.reserve ((size_t) EQ1AudioProcessor::maxNumBands);

    for (int i = 0; i < EQ1AudioProcessor::maxNumBands; ++i)
    {
        auto s = audioProcessor.getBandSettings (i);
        if (! s.enabled)
            continue;

        ActiveBand ab;
        ab.numStages = eq::computeBandCoefficients (s, sampleRate, ab.coeffs);
        activeBands.push_back (std::move (ab));
    }

    auto width = bounds.getWidth();
    auto numPoints = juce::jmax (2, (int) width);

    auto magnitudeDbAt = [&] (float freq) -> float
    {
        double magnitude = 1.0;

        for (auto& band : activeBands)
            for (int s = 0; s < band.numStages; ++s)
                magnitude *= band.coeffs[(size_t) s]->getMagnitudeForFrequency ((double) freq, sampleRate);

        return juce::Decibels::gainToDecibels ((float) magnitude, minDb);
    };

    juce::Path curvePath;

    for (int px = 0; px < numPoints; ++px)
    {
        auto proportion = (float) px / (float) juce::jmax (1, numPoints - 1);
        auto freq = minFreq * std::pow (maxFreq / minFreq, proportion);

        auto x = bounds.getX() + proportion * width;
        auto y = gainToY (magnitudeDbAt (freq));

        if (px == 0) curvePath.startNewSubPath (x, y);
        else curvePath.lineTo (x, y);
    }

    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.strokePath (curvePath, juce::PathStrokeType (2.0f));

    // 選択中バンドの個別カーブを破線で重ね描きする
    if (selectedBandIndex >= 0)
    {
        auto s = audioProcessor.getBandSettings (selectedBandIndex);

        if (s.enabled)
        {
            std::array<eq::Coefficients::Ptr, eq::maxStagesPerBand> coeffs;
            auto numStages = eq::computeBandCoefficients (s, sampleRate, coeffs);

            juce::Path bandPath;

            for (int px = 0; px < numPoints; ++px)
            {
                auto proportion = (float) px / (float) juce::jmax (1, numPoints - 1);
                auto freq = minFreq * std::pow (maxFreq / minFreq, proportion);

                double magnitude = 1.0;
                for (int st = 0; st < numStages; ++st)
                    magnitude *= coeffs[(size_t) st]->getMagnitudeForFrequency ((double) freq, sampleRate);

                auto db = juce::Decibels::gainToDecibels ((float) magnitude, minDb);
                auto x = bounds.getX() + proportion * width;
                auto y = gainToY (db);

                if (px == 0) bandPath.startNewSubPath (x, y);
                else bandPath.lineTo (x, y);
            }

            juce::Path dashed;
            float dashLengths[] = { 4.0f, 3.0f };
            juce::PathStrokeType (1.5f).createDashedStroke (dashed, bandPath, dashLengths, 2);

            g.setColour (FlatLookAndFeel::getBandColour (selectedBandIndex).withAlpha (0.85f));
            g.fillPath (dashed);
        }
    }
}

void EQGraphComponent::drawNodes (juce::Graphics& g) const
{
    for (int i = 0; i < EQ1AudioProcessor::maxNumBands; ++i)
    {
        auto s = audioProcessor.getBandSettings (i);
        if (! s.enabled)
            continue;

        auto colour = FlatLookAndFeel::getBandColour (i);
        auto pos = juce::Point<float> (freqToX (s.freq), gainToY (s.gain));

        float radius = 6.0f;
        if (i == hoveredBandIndex) radius = 7.0f;
        if (i == selectedBandIndex) radius = 8.0f;

        g.setColour (colour.withAlpha (0.25f));
        g.fillEllipse (juce::Rectangle<float> (radius * 2.6f, radius * 2.6f).withCentre (pos));

        g.setColour (colour);
        g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (pos));

        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (pos), 1.0f);

        if (i == selectedBandIndex)
        {
            g.setColour (juce::Colours::white);
            g.drawEllipse (juce::Rectangle<float> (radius * 2.0f + 4.0f, radius * 2.0f + 4.0f).withCentre (pos), 1.5f);
        }
    }
}

void EQGraphComponent::drawInfoPanel (juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    if (infoBandIndex < 0)
        return;

    auto s = audioProcessor.getBandSettings (infoBandIndex);
    if (! s.enabled)
        return;

    auto typeNames = eq::getFilterTypeNames();
    juce::String freqText = s.freq >= 1000.0f ? juce::String (s.freq / 1000.0f, 2) + " kHz"
                                               : juce::String ((int) s.freq) + " Hz";

    juce::String text = "Band " + juce::String (infoBandIndex + 1) + "  |  " + typeNames[(int) s.type]
                       + "  |  " + freqText
                       + "  |  " + juce::String (s.gain, 1) + " dB"
                       + "  |  Q " + juce::String (s.q, 2);

    juce::Font font (13.0f);
    auto textWidth = juce::GlyphArrangement::getStringWidth (font, text) + 16.0f;

    juce::Rectangle<float> panel (bounds.getX() + 8.0f, bounds.getY() + 8.0f, textWidth, 22.0f);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (panel, 4.0f);

    g.setColour (FlatLookAndFeel::getBandColour (infoBandIndex));
    g.setFont (font);
    g.drawText (text, panel.reduced (8.0f, 0.0f), juce::Justification::centredLeft);
}

//==============================================================================
int EQGraphComponent::findNodeAt (juce::Point<float> position) const
{
    constexpr float hitRadius = 12.0f;
    int best = -1;
    float bestDist = hitRadius;

    for (int i = 0; i < EQ1AudioProcessor::maxNumBands; ++i)
    {
        auto s = audioProcessor.getBandSettings (i);
        if (! s.enabled)
            continue;

        juce::Point<float> nodePos (freqToX (s.freq), gainToY (s.gain));
        auto dist = nodePos.getDistanceFrom (position);

        if (dist < bestDist)
        {
            bestDist = dist;
            best = i;
        }
    }

    return best;
}

void EQGraphComponent::selectBand (int bandIndex)
{
    selectedBandIndex = bandIndex;
    if (onBandSelected)
        onBandSelected (bandIndex);
}

void EQGraphComponent::updateHoverInfo (int bandIndex)
{
    infoBandIndex = bandIndex;
}

//==============================================================================
void EQGraphComponent::setFloatParam (const juce::String& paramID, float rawValue)
{
    if (auto* param = audioProcessor.apvts.getParameter (paramID))
        param->setValueNotifyingHost (param->convertTo0to1 (rawValue));
}

void EQGraphComponent::setBoolParam (const juce::String& paramID, bool value)
{
    if (auto* param = audioProcessor.apvts.getParameter (paramID))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (value ? 1.0f : 0.0f);
        param->endChangeGesture();
    }
}

void EQGraphComponent::setChoiceParam (const juce::String& paramID, int index)
{
    if (auto* param = audioProcessor.apvts.getParameter (paramID))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 ((float) index));
        param->endChangeGesture();
    }
}

//==============================================================================
void EQGraphComponent::mouseMove (const juce::MouseEvent& e)
{
    auto hit = findNodeAt (e.position);

    if (hit != hoveredBandIndex)
    {
        hoveredBandIndex = hit;
        repaint();
    }

    updateHoverInfo (hit >= 0 ? hit : selectedBandIndex);
}

void EQGraphComponent::mouseExit (const juce::MouseEvent&)
{
    if (hoveredBandIndex != -1)
    {
        hoveredBandIndex = -1;
        repaint();
    }

    updateHoverInfo (selectedBandIndex);
}

void EQGraphComponent::mouseDown (const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    auto hit = findNodeAt (e.position);

    if (e.mods.isPopupMenu())
    {
        if (hit >= 0)
        {
            selectBand (hit);
            showBandContextMenu (hit);
        }
        return;
    }

    if (hit >= 0)
    {
        selectBand (hit);
        draggingBandIndex = hit;

        if (auto* freqParam = audioProcessor.apvts.getParameter (EQ1AudioProcessor::bandParamID (hit, "freq")))
            freqParam->beginChangeGesture();
        if (auto* gainParam = audioProcessor.apvts.getParameter (EQ1AudioProcessor::bandParamID (hit, "gain")))
            gainParam->beginChangeGesture();

        repaint();
    }
}

void EQGraphComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (draggingBandIndex < 0)
        return;

    auto freq = xToFreq (e.position.x);
    auto gain = yToGain (e.position.y);

    setFloatParam (EQ1AudioProcessor::bandParamID (draggingBandIndex, "freq"), freq);
    setFloatParam (EQ1AudioProcessor::bandParamID (draggingBandIndex, "gain"), gain);

    updateHoverInfo (draggingBandIndex);
    repaint();
}

void EQGraphComponent::mouseUp (const juce::MouseEvent&)
{
    if (draggingBandIndex >= 0)
    {
        if (auto* freqParam = audioProcessor.apvts.getParameter (EQ1AudioProcessor::bandParamID (draggingBandIndex, "freq")))
            freqParam->endChangeGesture();
        if (auto* gainParam = audioProcessor.apvts.getParameter (EQ1AudioProcessor::bandParamID (draggingBandIndex, "gain")))
            gainParam->endChangeGesture();
    }

    draggingBandIndex = -1;
}

void EQGraphComponent::mouseDoubleClick (const juce::MouseEvent& e)
{
    auto hit = findNodeAt (e.position);

    if (hit >= 0)
    {
        setBoolParam (EQ1AudioProcessor::bandParamID (hit, "enabled"), false);

        if (selectedBandIndex == hit)
            selectBand (-1);

        updateHoverInfo (selectedBandIndex);
        repaint();
        return;
    }

    int freeIndex = -1;

    for (int i = 0; i < EQ1AudioProcessor::maxNumBands; ++i)
    {
        if (! audioProcessor.getBandSettings (i).enabled)
        {
            freeIndex = i;
            break;
        }
    }

    if (freeIndex < 0)
        return; // 空きバンドがない(maxNumBandsに到達済み)

    auto freq = xToFreq (e.position.x);
    auto gain = yToGain (e.position.y);

    setFloatParam (EQ1AudioProcessor::bandParamID (freeIndex, "freq"), freq);
    setFloatParam (EQ1AudioProcessor::bandParamID (freeIndex, "gain"), gain);
    setBoolParam  (EQ1AudioProcessor::bandParamID (freeIndex, "enabled"), true);

    selectBand (freeIndex);
    updateHoverInfo (freeIndex);
    repaint();
}

void EQGraphComponent::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    int target = hoveredBandIndex >= 0 ? hoveredBandIndex : selectedBandIndex;
    if (target < 0)
        return;

    auto paramID = EQ1AudioProcessor::bandParamID (target, "q");

    if (auto* param = audioProcessor.apvts.getParameter (paramID))
    {
        auto newNorm = juce::jlimit (0.0f, 1.0f, param->getValue() + wheel.deltaY * 0.05f);
        param->beginChangeGesture();
        param->setValueNotifyingHost (newNorm);
        param->endChangeGesture();
    }

    updateHoverInfo (target);
    repaint();
}

bool EQGraphComponent::keyPressed (const juce::KeyPress& key)
{
    if ((key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey) && selectedBandIndex >= 0)
    {
        setBoolParam (EQ1AudioProcessor::bandParamID (selectedBandIndex, "enabled"), false);
        selectBand (-1);
        updateHoverInfo (-1);
        repaint();
        return true;
    }

    return false;
}

void EQGraphComponent::showBandContextMenu (int bandIndex)
{
    auto settings = audioProcessor.getBandSettings (bandIndex);
    auto typeNames = eq::getFilterTypeNames();

    juce::PopupMenu menu;
    juce::PopupMenu typeMenu;

    for (int i = 0; i < typeNames.size(); ++i)
        typeMenu.addItem (100 + i, typeNames[i], true, (int) settings.type == i);

    menu.addSubMenu ("Filter Type", typeMenu);

    if (settings.type == eq::FilterType::LowPass || settings.type == eq::FilterType::HighPass)
    {
        juce::PopupMenu slopeMenu;
        auto slopeNames = eq::getSlopeNames();

        for (int i = 0; i < slopeNames.size(); ++i)
            slopeMenu.addItem (200 + i, slopeNames[i], true, settings.slopeIndex == i);

        menu.addSubMenu ("Slope", slopeMenu);
    }

    menu.addSeparator();
    menu.addItem (999, "Delete Band");

    menu.showMenuAsync (juce::PopupMenu::Options{}, [this, bandIndex] (int result)
    {
        if (result == 0)
            return;

        if (result == 999)
        {
            setBoolParam (EQ1AudioProcessor::bandParamID (bandIndex, "enabled"), false);

            if (selectedBandIndex == bandIndex)
                selectBand (-1);
        }
        else if (result >= 200)
        {
            setChoiceParam (EQ1AudioProcessor::bandParamID (bandIndex, "slope"), result - 200);
        }
        else if (result >= 100)
        {
            setChoiceParam (EQ1AudioProcessor::bandParamID (bandIndex, "type"), result - 100);
        }

        repaint();
    });
}

} // namespace eq
