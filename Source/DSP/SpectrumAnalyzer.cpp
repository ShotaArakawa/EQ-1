/*
  ==============================================================================

    SpectrumAnalyzer.cpp

  ==============================================================================
*/

#include "SpectrumAnalyzer.h"

namespace eq
{

namespace
{
    constexpr float minDisplayDb = -100.0f;
    constexpr float maxDisplayDb = 6.0f;
    constexpr float decayPerUpdateDb = 3.0f; // 1回のupdateIfReady呼び出し当たりの減衰量
}

SpectrumAnalyzer::SpectrumAnalyzer()
{
}

void SpectrumAnalyzer::pushSample (float sample) noexcept
{
    int start1, size1, start2, size2;
    abstractFifo.prepareToWrite (1, start1, size1, start2, size2);

    if (size1 > 0)
        fifoBuffer[(size_t) start1] = sample;
    else if (size2 > 0)
        fifoBuffer[(size_t) start2] = sample;

    abstractFifo.finishedWrite (size1 + size2);
}

bool SpectrumAnalyzer::updateIfReady()
{
    if (abstractFifo.getNumReady() < fftSize)
        return false;

    int start1, size1, start2, size2;
    abstractFifo.prepareToRead (fftSize, start1, size1, start2, size2);

    std::fill (fftWorkingBuffer.begin(), fftWorkingBuffer.end(), 0.0f);

    if (size1 > 0)
        std::copy (fifoBuffer.begin() + start1, fifoBuffer.begin() + start1 + size1, fftWorkingBuffer.begin());

    if (size2 > 0)
        std::copy (fifoBuffer.begin() + start2, fifoBuffer.begin() + start2 + size2, fftWorkingBuffer.begin() + size1);

    abstractFifo.finishedRead (size1 + size2);

    window.multiplyWithWindowingTable (fftWorkingBuffer.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftWorkingBuffer.data(), true);

    // FFTの生の振幅はfftSizeに比例し、Hann窓のコヒーレントゲイン(0.5)と片側スペクトラム(×2)を
    // 考慮するとフルスケールの正弦波でfftSize/4になる。これで割ってdBFS相当の値に正規化する
    constexpr float magnitudeScale = 4.0f / (float) fftSize;

    for (int i = 0; i < numBins; ++i)
    {
        auto db = juce::Decibels::gainToDecibels (fftWorkingBuffer[(size_t) i] * magnitudeScale, minDisplayDb);
        db = juce::jlimit (minDisplayDb, maxDisplayDb, db);

        auto& smoothed = smoothedMagnitudesDb[(size_t) i];
        smoothed = (db > smoothed) ? db : juce::jmax (db, smoothed - decayPerUpdateDb);
    }

    return true;
}

} // namespace eq
