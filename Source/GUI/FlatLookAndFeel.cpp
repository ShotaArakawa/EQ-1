/*
  ==============================================================================

    FlatLookAndFeel.cpp

  ==============================================================================
*/

#include "FlatLookAndFeel.h"

namespace eq
{

FlatLookAndFeel::FlatLookAndFeel()
{
    setColourScheme (getDarkColourScheme());

    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xff15171a));
    setColour (juce::Slider::thumbColourId, juce::Colours::white);
    setColour (juce::Slider::trackColourId, juce::Colour (0xff3a3a3a));
    setColour (juce::Slider::backgroundColourId, juce::Colour (0xff2a2c30));
    setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff4ca8ff));
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff3a3a3a));
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2a2c30));
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff45484e));
    setColour (juce::ComboBox::textColourId, juce::Colours::white.withAlpha (0.85f));
    setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.85f));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff23252a));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff3a6ea5));
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2a2c30));
}

juce::Colour FlatLookAndFeel::getBandColour (int bandIndex)
{
    static const juce::Colour palette[] = {
        juce::Colour (0xffff5f5f),
        juce::Colour (0xffffb648),
        juce::Colour (0xffffe066),
        juce::Colour (0xff8cff66),
        juce::Colour (0xff4ce0d2),
        juce::Colour (0xff4ca8ff),
        juce::Colour (0xffb58cff),
        juce::Colour (0xffff6ec7),
    };

    return palette[(size_t) (((bandIndex % 8) + 8) % 8)];
}

void FlatLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                         bool shouldDrawButtonAsHighlighted, bool /*shouldDrawButtonAsDown*/)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    auto switchWidth = juce::jmin (bounds.getWidth(), bounds.getHeight() * 1.8f);
    auto switchBounds = bounds.withWidth (switchWidth);
    auto cornerRadius = switchBounds.getHeight() * 0.5f;

    const bool isOn = button.getToggleState();

    g.setColour (isOn ? getBandColour (0) : juce::Colour (0xff3a3a3a));
    g.fillRoundedRectangle (switchBounds, cornerRadius);

    if (shouldDrawButtonAsHighlighted)
    {
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.fillRoundedRectangle (switchBounds, cornerRadius);
    }

    auto knobDiameter = switchBounds.getHeight() - 4.0f;
    auto knobX = isOn ? switchBounds.getRight() - knobDiameter - 2.0f : switchBounds.getX() + 2.0f;
    juce::Rectangle<float> knobBounds (knobX, switchBounds.getY() + 2.0f, knobDiameter, knobDiameter);

    g.setColour (juce::Colours::white);
    g.fillEllipse (knobBounds);

    if (button.getButtonText().isNotEmpty())
    {
        auto textArea = button.getLocalBounds().withTrimmedLeft ((int) switchWidth + 8);
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.setFont (juce::Font (13.0f));
        g.drawText (button.getButtonText(), textArea, juce::Justification::centredLeft);
    }
}

} // namespace eq
