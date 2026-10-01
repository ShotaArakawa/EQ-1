/*
  ==============================================================================

    FlatLookAndFeel.h
    FabFilter Pro-Q 3を意識したダーク・フラットなビジュアルテーマ。
    JUCE標準のLookAndFeel_V4ダークスキームをベースに、プラグイン全体で使う
    バンドごとの識別色(getBandColour)と、フラットなトグルスイッチの描画を追加する。

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace eq
{

class FlatLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FlatLookAndFeel();

    // バンドindexから識別用のアクセントカラーを返す(8色を周回させる)
    static juce::Colour getBandColour (int bandIndex);

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                            bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

} // namespace eq
