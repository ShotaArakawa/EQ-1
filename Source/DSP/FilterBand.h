/*
  ==============================================================================

    FilterBand.h
    EQの1バンド分のフィルタ種別・パラメータ値・係数計算をまとめた純粋なDSPロジック。
    オーディオスレッド(PluginProcessor)とUIスレッド(EQGraphComponentの曲線描画)の
    両方から同じ関数を呼べるよう、実際のIIRフィルタ実体は持たず係数計算のみを行う。

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>

namespace eq
{

//==============================================================================
enum class FilterType
{
    Bell = 0,
    LowShelf,
    HighShelf,
    LowPass,
    HighPass,
    Notch,
    BandPass,
    TiltShelf,
    numFilterTypes
};

inline juce::StringArray getFilterTypeNames()
{
    return { "Bell", "Low Shelf", "High Shelf", "Low Pass", "High Pass", "Notch", "Band Pass", "Tilt Shelf" };
}

// Low Pass / High Pass のみ使用する、6dB/oct刻みのスロープ選択肢
inline juce::StringArray getSlopeNames()
{
    return { "6 dB/oct", "12 dB/oct", "18 dB/oct", "24 dB/oct", "36 dB/oct", "48 dB/oct" };
}

// 1バンドが取り得る最大段数。LowPass/HighPassの48dB/oct(2次フィルタ4段)が最大要求段数
constexpr int maxStagesPerBand = 4;

using Coefficients = juce::dsp::IIR::Coefficients<float>;

//==============================================================================
// 1バンド分の「生の」パラメータ値。APVTSから読み出した値をまとめるスナップショット
struct BandParameterValues
{
    bool enabled = false;
    float freq = 1000.0f;
    float gain = 0.0f;
    float q = 0.70710678f;
    FilterType type = FilterType::Bell;
    int slopeIndex = 1; // getSlopeNames()のインデックス。デフォルトは"12 dB/oct"
};

//==============================================================================
// 指定バンドの現在値から、最大maxStagesPerBand個のIIR係数を算出する。
// 戻り値は実際に使用するステージ数(1 〜 maxStagesPerBand)。
// LowPass/HighPassはスロープに応じて複数段をカスケードし、
// Qはユーザー指定値を先頭ステージのみに適用し、残りはButterworth平坦(1/sqrt(2))とする。
// TiltShelfはLowShelf(-gain/2)とHighShelf(+gain/2)の2段合成で実現する。
int computeBandCoefficients (const BandParameterValues& settings, double sampleRate,
                              std::array<Coefficients::Ptr, maxStagesPerBand>& outCoefficients);

} // namespace eq
