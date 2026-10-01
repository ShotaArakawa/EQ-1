/*
  ==============================================================================

    TopBarComponent.cpp

  ==============================================================================
*/

#include "TopBarComponent.h"

namespace eq
{

TopBarComponent::TopBarComponent (EQ1AudioProcessor& processorToUse)
    : audioProcessor (processorToUse)
{
    titleLabel.setText ("EQ-1", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (20.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (titleLabel);

    addAndMakeVisible (bypassButton);
    bypassAttachment = std::make_unique<ButtonAttachment> (audioProcessor.apvts, "masterBypass", bypassButton);

    analyzerLabel.setText ("Analyzer", juce::dontSendNotification);
    analyzerLabel.setJustificationType (juce::Justification::centredRight);
    analyzerLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.6f));
    addAndMakeVisible (analyzerLabel);

    analyzerModeBox.addItemList ({ "Off", "Pre", "Post", "Pre+Post" }, 1);
    addAndMakeVisible (analyzerModeBox);
    analyzerModeAttachment = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, "analyzerMode", analyzerModeBox);
}

void TopBarComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1c1e22));
    g.setColour (juce::Colour (0xff35373c));
    g.drawLine (0.0f, (float) getHeight() - 1.0f, (float) getWidth(), (float) getHeight() - 1.0f, 1.0f);
}

void TopBarComponent::resized()
{
    auto area = getLocalBounds().reduced (10, 6);

    titleLabel.setBounds (area.removeFromLeft (100));

    auto rightArea = area.removeFromRight (230);
    analyzerModeBox.setBounds (rightArea.removeFromRight (110).withSizeKeepingCentre (110, 24));
    analyzerLabel.setBounds (rightArea);

    bypassButton.setBounds (area.removeFromLeft (130).withSizeKeepingCentre (120, 24));
}

} // namespace eq
