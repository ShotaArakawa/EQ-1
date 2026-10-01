/*
  ==============================================================================

    BandControlStrip.h
    現在選択中のバンドを正確な数値で編集するための下部パネル。
    ドラッグでは大まかにしか合わせられないFreq/Gain/Q、および
    グラフ上では選びにくいフィルタ種別・スロープをここで細かく編集する。
    選択バンドが変わるたびにAttachmentを張り替える。

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "../DSP/FilterBand.h"
#include "FlatLookAndFeel.h"

namespace eq
{

class BandControlStrip  : public juce::Component,
                           private juce::Timer
{
public:
    explicit BandControlStrip (EQ1AudioProcessor& processorToUse);
    ~BandControlStrip() override;

    // -1を渡すと「未選択」状態になり、全コントロールが無効化される
    void setSelectedBand (int bandIndex);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void rebuildAttachments();

    EQ1AudioProcessor& audioProcessor;

    int currentBandIndex = -1;
    eq::FilterType lastKnownType = eq::FilterType::Bell;

    juce::Label bandLabel;
    juce::ToggleButton enabledButton { "On" };
    juce::ComboBox typeBox;
    juce::ComboBox slopeBox;
    juce::Slider freqSlider, gainSlider, qSlider;
    juce::Label freqLabel, gainLabel, qLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> freqAttachment, gainAttachment, qAttachment;
    std::unique_ptr<ButtonAttachment> enabledAttachment;
    std::unique_ptr<ComboBoxAttachment> typeAttachment, slopeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BandControlStrip)
};

} // namespace eq
