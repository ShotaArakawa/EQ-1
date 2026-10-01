/*
  ==============================================================================

    BandControlStrip.cpp

  ==============================================================================
*/

#include "BandControlStrip.h"

namespace eq
{

BandControlStrip::BandControlStrip (EQ1AudioProcessor& processorToUse)
    : audioProcessor (processorToUse)
{
    addAndMakeVisible (bandLabel);
    bandLabel.setFont (juce::Font (14.0f, juce::Font::bold));

    addAndMakeVisible (enabledButton);

    typeBox.addItemList (eq::getFilterTypeNames(), 1);
    addAndMakeVisible (typeBox);

    slopeBox.addItemList (eq::getSlopeNames(), 1);
    addAndMakeVisible (slopeBox);

    auto setupSlider = [this] (juce::Slider& slider, juce::Label& label, const juce::String& name)
    {
        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 56, 20);
        addAndMakeVisible (slider);

        label.setText (name, juce::dontSendNotification);
        label.setFont (juce::Font (11.0f));
        label.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.5f));
        addAndMakeVisible (label);
    };

    setupSlider (freqSlider, freqLabel, "Freq");
    setupSlider (gainSlider, gainLabel, "Gain");
    setupSlider (qSlider, qLabel, "Q");

    rebuildAttachments();
    startTimerHz (15);
}

BandControlStrip::~BandControlStrip()
{
    stopTimer();
}

void BandControlStrip::setSelectedBand (int bandIndex)
{
    if (currentBandIndex == bandIndex)
        return;

    currentBandIndex = bandIndex;
    rebuildAttachments();
}

void BandControlStrip::timerCallback()
{
    if (currentBandIndex < 0)
        return;

    auto type = audioProcessor.getBandSettings (currentBandIndex).type;

    if (type != lastKnownType)
    {
        lastKnownType = type;
        resized();
    }
}

void BandControlStrip::rebuildAttachments()
{
    freqAttachment.reset();
    gainAttachment.reset();
    qAttachment.reset();
    enabledAttachment.reset();
    typeAttachment.reset();
    slopeAttachment.reset();

    const bool hasSelection = currentBandIndex >= 0;

    bandLabel.setText (hasSelection ? ("Band " + juce::String (currentBandIndex + 1)) : "No band selected",
                        juce::dontSendNotification);
    bandLabel.setColour (juce::Label::textColourId,
                          hasSelection ? FlatLookAndFeel::getBandColour (currentBandIndex)
                                       : juce::Colours::white.withAlpha (0.5f));

    enabledButton.setEnabled (hasSelection);
    typeBox.setEnabled (hasSelection);
    slopeBox.setEnabled (hasSelection);
    freqSlider.setEnabled (hasSelection);
    gainSlider.setEnabled (hasSelection);
    qSlider.setEnabled (hasSelection);

    if (! hasSelection)
    {
        lastKnownType = eq::FilterType::Bell;
        resized();
        return;
    }

    enabledAttachment = std::make_unique<ButtonAttachment> (
        audioProcessor.apvts, EQ1AudioProcessor::bandParamID (currentBandIndex, "enabled"), enabledButton);
    freqAttachment = std::make_unique<SliderAttachment> (
        audioProcessor.apvts, EQ1AudioProcessor::bandParamID (currentBandIndex, "freq"), freqSlider);
    gainAttachment = std::make_unique<SliderAttachment> (
        audioProcessor.apvts, EQ1AudioProcessor::bandParamID (currentBandIndex, "gain"), gainSlider);
    qAttachment = std::make_unique<SliderAttachment> (
        audioProcessor.apvts, EQ1AudioProcessor::bandParamID (currentBandIndex, "q"), qSlider);
    typeAttachment = std::make_unique<ComboBoxAttachment> (
        audioProcessor.apvts, EQ1AudioProcessor::bandParamID (currentBandIndex, "type"), typeBox);
    slopeAttachment = std::make_unique<ComboBoxAttachment> (
        audioProcessor.apvts, EQ1AudioProcessor::bandParamID (currentBandIndex, "slope"), slopeBox);

    lastKnownType = audioProcessor.getBandSettings (currentBandIndex).type;
    resized();
}

void BandControlStrip::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1c1e22));
    g.setColour (juce::Colour (0xff35373c));
    g.drawLine (0.0f, 0.0f, (float) getWidth(), 0.0f, 1.0f);
}

void BandControlStrip::resized()
{
    auto area = getLocalBounds().reduced (10, 6);

    bandLabel.setBounds (area.removeFromLeft (100));

    // スイッチ(高さ22 × 1.8 ≒ 40px)の右に"On"のラベルが収まる幅を確保する
    enabledButton.setBounds (area.removeFromLeft (80).withSizeKeepingCentre (72, 22));
    area.removeFromLeft (10);

    typeBox.setBounds (area.removeFromLeft (120).withSizeKeepingCentre (120, 24));
    area.removeFromLeft (8);

    const bool showSlope = currentBandIndex >= 0
        && (lastKnownType == eq::FilterType::LowPass || lastKnownType == eq::FilterType::HighPass);
    slopeBox.setVisible (showSlope);

    if (showSlope)
    {
        slopeBox.setBounds (area.removeFromLeft (110).withSizeKeepingCentre (110, 24));
        area.removeFromLeft (8);
    }

    auto sliderWidth = area.getWidth() / 3;

    auto layoutSlider = [&] (juce::Slider& slider, juce::Label& label, int width)
    {
        auto col = area.removeFromLeft (width);
        label.setBounds (col.removeFromTop (14));
        slider.setBounds (col);
    };

    layoutSlider (freqSlider, freqLabel, sliderWidth);
    layoutSlider (gainSlider, gainLabel, sliderWidth);
    layoutSlider (qSlider, qLabel, area.getWidth());
}

} // namespace eq
