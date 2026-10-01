/*
  ==============================================================================

    SpectrumAnalyzer.h
    モノラルにサマリングした信号を受け取り、FFTで周波数スペクトラムを算出するクラス。
    pushSample()はオーディオスレッドから、updateIfReady()はUIスレッドのTimerから
    呼ぶことを想定している(juce::AbstractFifoによるロックフリーなスレッド間受け渡し)。

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

namespace eq
{

class SpectrumAnalyzer
{
public:
    SpectrumAnalyzer();

    // オーディオスレッドから1サンプルずつ書き込む(確保なし、ロックフリー)
    void pushSample (float sample) noexcept;

    // UIスレッドのTimerから定期的に呼ぶ。新しいFFTフレームを計算できればtrueを返す
    bool updateIfReady();

    // 表示用の、ピークホールド+減衰で平滑化されたdB値(ビンごと)
    const std::vector<float>& getMagnitudesDb() const noexcept { return smoothedMagnitudesDb; }

    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;   // 2048
    static constexpr int numBins = fftSize / 2;

private:
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann };

    static constexpr int fifoCapacity = fftSize * 4;
    juce::AbstractFifo abstractFifo { fifoCapacity };
    std::array<float, fifoCapacity> fifoBuffer {};

    std::array<float, fftSize * 2> fftWorkingBuffer {};
    std::vector<float> smoothedMagnitudesDb = std::vector<float> ((size_t) numBins, -100.0f);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumAnalyzer)
};

} // namespace eq
