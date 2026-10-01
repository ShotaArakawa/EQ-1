/*
  ==============================================================================

    EQGraphComponent.h
    FabFilter Pro-Q 3のような、グラフ上のノードを直接ドラッグして操作する
    インタラクティブなEQカーブ表示。dB/Hzグリッド、スペクトラムアナライザー、
    合成後のEQカーブ、バンドごとのドラッグ可能なノードを1つのコンポーネントにまとめる。

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "FlatLookAndFeel.h"
#include <functional>
#include <vector>

namespace eq
{

class EQGraphComponent  : public juce::Component,
                           private juce::Timer
{
public:
    explicit EQGraphComponent (EQ1AudioProcessor& processorToUse);
    ~EQGraphComponent() override;

    void paint (juce::Graphics& g) override;

    void mouseMove (const juce::MouseEvent& e) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseExit (const juce::MouseEvent& e) override;
    bool keyPressed (const juce::KeyPress& key) override;

    // 選択中バンドが変わるたびに呼ばれる(BandControlStripとの同期用)。-1は未選択
    std::function<void (int)> onBandSelected;

    // 外部(PluginEditorの初期化時など)からバンドを選択状態にする。onBandSelectedも発火する
    void selectBand (int bandIndex);

private:
    void timerCallback() override;

    void drawGrid (juce::Graphics& g, juce::Rectangle<float> bounds) const;
    void drawAnalyzer (juce::Graphics& g, juce::Rectangle<float> bounds) const;
    void drawCurve (juce::Graphics& g, juce::Rectangle<float> bounds) const;
    void drawNodes (juce::Graphics& g) const;
    void drawInfoPanel (juce::Graphics& g, juce::Rectangle<float> bounds) const;

    float freqToX (float freq) const;
    float xToFreq (float x) const;
    float gainToY (float gainDb) const;
    float yToGain (float y) const;

    int findNodeAt (juce::Point<float> position) const;
    void updateHoverInfo (int bandIndex);
    void showBandContextMenu (int bandIndex);

    void setFloatParam (const juce::String& paramID, float rawValue);
    void setBoolParam (const juce::String& paramID, bool value);
    void setChoiceParam (const juce::String& paramID, int index);

    EQ1AudioProcessor& audioProcessor;

    static constexpr float minFreq = 20.0f;
    static constexpr float maxFreq = 20000.0f;
    static constexpr float minDb = -24.0f;
    static constexpr float maxDb = 24.0f;

    int selectedBandIndex = -1;
    int hoveredBandIndex = -1;
    int draggingBandIndex = -1;
    int infoBandIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQGraphComponent)
};

} // namespace eq
