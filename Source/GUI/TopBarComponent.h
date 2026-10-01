/*
  ==============================================================================

    TopBarComponent.h
    マスターバイパスとアナライザーモード切替を持つ上部バー。

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"

namespace eq
{

class TopBarComponent  : public juce::Component
{
public:
    explicit TopBarComponent (EQ1AudioProcessor& processorToUse);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    EQ1AudioProcessor& audioProcessor;

    juce::Label titleLabel;
    juce::ToggleButton bypassButton { "Bypass" };
    juce::Label analyzerLabel;
    juce::ComboBox analyzerModeBox;

    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ButtonAttachment> bypassAttachment;
    std::unique_ptr<ComboBoxAttachment> analyzerModeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TopBarComponent)
};

} // namespace eq
