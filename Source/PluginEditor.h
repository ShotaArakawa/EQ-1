/*
  ==============================================================================

    EQ1AudioProcessorEditor
    FabFilter Pro-Q 3を意識した、グラフ中心のフラット/ダークなエディタ。
    上から TopBar(マスターバイパス/アナライザー切替) / EQGraphComponent(メイングラフ) /
    BandControlStrip(選択中バンドの数値編集)の3段構成。

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GUI/FlatLookAndFeel.h"
#include "GUI/TopBarComponent.h"
#include "GUI/EQGraphComponent.h"
#include "GUI/BandControlStrip.h"

//==============================================================================
class EQ1AudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit EQ1AudioProcessorEditor (EQ1AudioProcessor&);
    ~EQ1AudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    EQ1AudioProcessor& audioProcessor;

    eq::FlatLookAndFeel lookAndFeel;

    eq::TopBarComponent topBar;
    eq::EQGraphComponent graph;
    eq::BandControlStrip controlStrip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQ1AudioProcessorEditor)
};
