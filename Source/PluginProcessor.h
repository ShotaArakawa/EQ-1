/*
  ==============================================================================

    EQ1AudioProcessor
    可変バンド数のパラメトリックEQ本体。
    APVTSには最大maxNumBands個ぶんのバンドパラメータ(enabled/freq/gain/q/type/slope)を
    あらかじめ全て確保しておき、enabledフラグでグラフ上のバンドの出し入れを表現する
    (JUCEのAudioProcessorValueTreeStateは実行時にパラメータ数を変更できないため)。
    各バンドは最大eq::maxStagesPerBand段のIIRフィルタをカスケードし、
    フィルタ種別・スロープに応じて使用する段数を切り替える。

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "DSP/FilterBand.h"
#include "DSP/SpectrumAnalyzer.h"
#include <array>

//==============================================================================
class EQ1AudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    EQ1AudioProcessor();
    ~EQ1AudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // グラフ上に同時に置けるバンドの最大数。enabledフラグがfalseのバンドは処理もUIも非表示になる
    static constexpr int maxNumBands = 16;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts { *this, nullptr, "PARAMETERS", createParameterLayout() };

    // "band3_freq" のようなパラメータIDを一箇所で生成する
    static juce::String bandParamID (int bandIndex, const char* field);

    // 指定バンドの現在のパラメータ値(スムージング前の生値)を取得する。
    // オーディオスレッドのprocessBlock、UIスレッドのグラフ描画の両方から使用する
    eq::BandParameterValues getBandSettings (int bandIndex) const;

    // 20Hz〜20000Hzなど、指数的なスケールのNormalisableRangeを作る(パラメータ定義とUIの両方で使用)
    template <typename FloatType>
    static juce::NormalisableRange<FloatType> makeLogRange (FloatType minValue, FloatType maxValue)
    {
        return juce::NormalisableRange<FloatType> (minValue, maxValue,
            [] (FloatType start, FloatType end, FloatType normalised)
            { return start * std::pow (end / start, normalised); },
            [] (FloatType start, FloatType end, FloatType value)
            { return std::log (value / start) / std::log (end / start); });
    }

    // スペクトラムアナライザー。processBlockが書き込み、エディタのTimerが読み出す
    eq::SpectrumAnalyzer preAnalyzer, postAnalyzer;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQ1AudioProcessor)

    using Filter = juce::dsp::IIR::Filter<float>;
    using StereoFilter = juce::dsp::ProcessorDuplicator<Filter, eq::Coefficients>;

    struct BandStage
    {
        StereoFilter filter;
        bool active = false;
    };

    struct Band
    {
        std::array<BandStage, eq::maxStagesPerBand> stages;

        // ノブ操作時のジッパーノイズを防ぐためのパラメータスムージング(freq/gain/qのみ。
        // type/slopeの変更は構造が変わるため補間せずブロック境界で即座に切り替える)
        juce::SmoothedValue<float> freqSmoothed, gainSmoothed, qSmoothed;
    };

    std::array<Band, maxNumBands> bands;

    double currentSampleRate = 44100.0;

    // ブロック先頭で1回だけ、指定バンドのスムージング済みの値からフィルタ係数を再計算する
    void updateBand (int bandIndex, const eq::BandParameterValues& rawSettings, int numSamplesInBlock);
};
