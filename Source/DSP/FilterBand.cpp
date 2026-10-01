/*
  ==============================================================================

    FilterBand.cpp

  ==============================================================================
*/

#include "FilterBand.h"

namespace eq
{

namespace
{
    constexpr float butterworthQ = 0.70710678f;

    // LowPass/HighPassのスロープ選択肢(6/12/18/24/36/48 dB/oct)を
    // フィルタ次数(6dB/oct = 1次)に変換したテーブル
    constexpr int totalOrderForSlopeIndex[6] = { 1, 2, 3, 4, 6, 8 };

    // 合計次数を、1次・2次ステージの組み合わせ(最大maxStagesPerBand段)に分解する。
    // 例: 次数3 -> [2次, 1次] / 次数8 -> [2次, 2次, 2次, 2次]
    int decomposeOrderIntoStages (int totalOrder, std::array<int, maxStagesPerBand>& outStageOrders)
    {
        int remaining = totalOrder;
        int numStages = 0;

        while (remaining > 0 && numStages < maxStagesPerBand)
        {
            int stageOrder = juce::jmin (2, remaining);
            outStageOrders[(size_t) numStages] = stageOrder;
            remaining -= stageOrder;
            ++numStages;
        }

        return numStages;
    }

    int computeLowHighPassCoefficients (bool isLowPass, const BandParameterValues& s, double sampleRate,
                                         std::array<Coefficients::Ptr, maxStagesPerBand>& out)
    {
        const int slopeIdx = juce::jlimit (0, 5, s.slopeIndex);
        const int totalOrder = totalOrderForSlopeIndex[slopeIdx];

        std::array<int, maxStagesPerBand> stageOrders {};
        const int numStages = decomposeOrderIntoStages (totalOrder, stageOrders);

        for (int i = 0; i < numStages; ++i)
        {
            const float stageQ = (i == 0) ? s.q : butterworthQ;

            if (stageOrders[(size_t) i] == 1)
            {
                out[(size_t) i] = isLowPass ? Coefficients::makeFirstOrderLowPass (sampleRate, s.freq)
                                             : Coefficients::makeFirstOrderHighPass (sampleRate, s.freq);
            }
            else
            {
                out[(size_t) i] = isLowPass ? Coefficients::makeLowPass (sampleRate, s.freq, stageQ)
                                             : Coefficients::makeHighPass (sampleRate, s.freq, stageQ);
            }
        }

        return numStages;
    }
}

int computeBandCoefficients (const BandParameterValues& s, double sampleRate,
                              std::array<Coefficients::Ptr, maxStagesPerBand>& out)
{
    const float gainLin = juce::Decibels::decibelsToGain (s.gain);
    const float freq = juce::jlimit (20.0f, (float) (sampleRate * 0.49), s.freq);

    BandParameterValues clamped = s;
    clamped.freq = freq;

    switch (s.type)
    {
        case FilterType::Bell:
            out[0] = Coefficients::makePeakFilter (sampleRate, freq, s.q, gainLin);
            return 1;

        case FilterType::LowShelf:
            out[0] = Coefficients::makeLowShelf (sampleRate, freq, s.q, gainLin);
            return 1;

        case FilterType::HighShelf:
            out[0] = Coefficients::makeHighShelf (sampleRate, freq, s.q, gainLin);
            return 1;

        case FilterType::Notch:
            out[0] = Coefficients::makeNotch (sampleRate, freq, s.q);
            return 1;

        case FilterType::BandPass:
            out[0] = Coefficients::makeBandPass (sampleRate, freq, s.q);
            return 1;

        case FilterType::TiltShelf:
        {
            const float halfGainDb = s.gain * 0.5f;
            out[0] = Coefficients::makeLowShelf  (sampleRate, freq, s.q, juce::Decibels::decibelsToGain (-halfGainDb));
            out[1] = Coefficients::makeHighShelf (sampleRate, freq, s.q, juce::Decibels::decibelsToGain ( halfGainDb));
            return 2;
        }

        case FilterType::LowPass:
            return computeLowHighPassCoefficients (true, clamped, sampleRate, out);

        case FilterType::HighPass:
            return computeLowHighPassCoefficients (false, clamped, sampleRate, out);

        default:
            jassertfalse;
            out[0] = Coefficients::makePeakFilter (sampleRate, freq, s.q, 1.0f);
            return 1;
    }
}

} // namespace eq
