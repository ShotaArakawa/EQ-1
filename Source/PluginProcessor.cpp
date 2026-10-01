/*
  ==============================================================================

    EQ1AudioProcessor

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float smoothingTimeSeconds = 0.02f;

    // 0=Off, 1=Pre, 2=Post, 3=Pre+Post (createParameterLayoutのanalyzerModeの選択肢順と対応)
    enum AnalyzerMode { AnalyzerOff = 0, AnalyzerPre, AnalyzerPost, AnalyzerPrePost };
}

//==============================================================================
EQ1AudioProcessor::EQ1AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

EQ1AudioProcessor::~EQ1AudioProcessor()
{
}

//==============================================================================
const juce::String EQ1AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool EQ1AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool EQ1AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool EQ1AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double EQ1AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int EQ1AudioProcessor::getNumPrograms()
{
    return 1;
}

int EQ1AudioProcessor::getCurrentProgram()
{
    return 0;
}

void EQ1AudioProcessor::setCurrentProgram (int)
{
}

const juce::String EQ1AudioProcessor::getProgramName (int)
{
    return {};
}

void EQ1AudioProcessor::changeProgramName (int, const juce::String&)
{
}

//==============================================================================
void EQ1AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = (juce::uint32) juce::jmax (getTotalNumOutputChannels(), 1);

    for (int n = 0; n < maxNumBands; ++n)
    {
        auto& band = bands[(size_t) n];

        for (auto& stage : band.stages)
        {
            stage.filter.prepare (spec);
            stage.filter.reset();
            stage.active = false;
        }

        auto raw = getBandSettings (n);
        band.freqSmoothed.reset (sampleRate, smoothingTimeSeconds);
        band.gainSmoothed.reset (sampleRate, smoothingTimeSeconds);
        band.qSmoothed.reset (sampleRate, smoothingTimeSeconds);
        band.freqSmoothed.setCurrentAndTargetValue (raw.freq);
        band.gainSmoothed.setCurrentAndTargetValue (raw.gain);
        band.qSmoothed.setCurrentAndTargetValue (raw.q);

        updateBand (n, raw, samplesPerBlock);
    }
}

void EQ1AudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool EQ1AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

juce::String EQ1AudioProcessor::bandParamID (int bandIndex, const char* field)
{
    return "band" + juce::String (bandIndex) + "_" + field;
}

eq::BandParameterValues EQ1AudioProcessor::getBandSettings (int bandIndex) const
{
    eq::BandParameterValues s;

    s.enabled    = apvts.getRawParameterValue (bandParamID (bandIndex, "enabled"))->load() >= 0.5f;
    s.freq       = apvts.getRawParameterValue (bandParamID (bandIndex, "freq"))->load();
    s.gain       = apvts.getRawParameterValue (bandParamID (bandIndex, "gain"))->load();
    s.q          = apvts.getRawParameterValue (bandParamID (bandIndex, "q"))->load();
    s.type       = (eq::FilterType) (int) apvts.getRawParameterValue (bandParamID (bandIndex, "type"))->load();
    s.slopeIndex = (int) apvts.getRawParameterValue (bandParamID (bandIndex, "slope"))->load();

    return s;
}

void EQ1AudioProcessor::updateBand (int bandIndex, const eq::BandParameterValues& raw, int numSamplesInBlock)
{
    auto& band = bands[(size_t) bandIndex];

    if (! raw.enabled)
    {
        for (auto& stage : band.stages)
            stage.active = false;

        return;
    }

    band.freqSmoothed.setTargetValue (raw.freq); band.freqSmoothed.skip (numSamplesInBlock);
    band.gainSmoothed.setTargetValue (raw.gain); band.gainSmoothed.skip (numSamplesInBlock);
    band.qSmoothed.setTargetValue (raw.q);       band.qSmoothed.skip (numSamplesInBlock);

    eq::BandParameterValues smoothed = raw;
    smoothed.freq = band.freqSmoothed.getCurrentValue();
    smoothed.gain = band.gainSmoothed.getCurrentValue();
    smoothed.q    = band.qSmoothed.getCurrentValue();

    std::array<eq::Coefficients::Ptr, eq::maxStagesPerBand> newCoeffs;
    const int numActiveStages = eq::computeBandCoefficients (smoothed, currentSampleRate, newCoeffs);

    for (int i = 0; i < eq::maxStagesPerBand; ++i)
    {
        auto& stage = band.stages[(size_t) i];

        if (i < numActiveStages)
        {
            *stage.filter.state = *newCoeffs[(size_t) i];
            stage.active = true;
        }
        else
        {
            stage.active = false;
        }
    }
}

//==============================================================================
void EQ1AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    const bool masterBypassed = apvts.getRawParameterValue ("masterBypass")->load() >= 0.5f;
    const int analyzerMode = (int) apvts.getRawParameterValue ("analyzerMode")->load();

    auto pushMonoSumTo = [&] (eq::SpectrumAnalyzer& analyzer)
    {
        for (int s = 0; s < numSamples; ++s)
        {
            float mono = 0.0f;

            for (int ch = 0; ch < numChannels; ++ch)
                mono += buffer.getSample (ch, s);

            if (numChannels > 0)
                mono /= (float) numChannels;

            analyzer.pushSample (mono);
        }
    };

    if (analyzerMode == AnalyzerPre || analyzerMode == AnalyzerPrePost)
        pushMonoSumTo (preAnalyzer);

    if (! masterBypassed)
    {
        for (int n = 0; n < maxNumBands; ++n)
            updateBand (n, getBandSettings (n), numSamples);

        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> context (block);

        for (auto& band : bands)
            for (auto& stage : band.stages)
                if (stage.active)
                    stage.filter.process (context);
    }

    if (analyzerMode == AnalyzerPost || analyzerMode == AnalyzerPrePost)
        pushMonoSumTo (postAnalyzer);
}

//==============================================================================
bool EQ1AudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* EQ1AudioProcessor::createEditor()
{
    return new EQ1AudioProcessorEditor (*this);
}

//==============================================================================
void EQ1AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void EQ1AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout EQ1AudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto freqRange = makeLogRange<float> (20.0f, 20000.0f);
    auto qRange    = makeLogRange<float> (0.025f, 40.0f);
    juce::NormalisableRange<float> gainRange (-30.0f, 30.0f, 0.1f);

    auto hzAttributes = juce::AudioParameterFloatAttributes().withLabel ("Hz")
        .withStringFromValueFunction ([] (float v, int) { return juce::String ((int) std::round (v)); });

    auto qAttributes = juce::AudioParameterFloatAttributes().withLabel ("Q")
        .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 2); });

    auto dbAttributes = juce::AudioParameterFloatAttributes().withLabel ("dB")
        .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1); });

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "masterBypass", 1 }, "Master Bypass", false));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "analyzerMode", 1 }, "Analyzer",
        juce::StringArray { "Off", "Pre", "Post", "Pre+Post" }, 3));

    for (int n = 0; n < maxNumBands; ++n)
    {
        const juce::String nameSuffix = " " + juce::String (n + 1);
        const bool defaultEnabled = (n == 0);

        float defaultFreq;

        if (n == 0)
        {
            defaultFreq = 1000.0f;
        }
        else
        {
            auto proportion = (float) n / (float) (maxNumBands - 1);
            defaultFreq = 20.0f * std::pow (20000.0f / 20.0f, proportion);
        }

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { bandParamID (n, "enabled"), 1 }, "Band" + nameSuffix + " Enabled", defaultEnabled));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { bandParamID (n, "freq"), 1 }, "Band" + nameSuffix + " Freq", freqRange, defaultFreq, hzAttributes));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { bandParamID (n, "gain"), 1 }, "Band" + nameSuffix + " Gain", gainRange, 0.0f, dbAttributes));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { bandParamID (n, "q"), 1 }, "Band" + nameSuffix + " Q", qRange, 0.70710678f, qAttributes));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { bandParamID (n, "type"), 1 }, "Band" + nameSuffix + " Type", eq::getFilterTypeNames(), 0));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { bandParamID (n, "slope"), 1 }, "Band" + nameSuffix + " Slope", eq::getSlopeNames(), 1));
    }

    return { params.begin(), params.end() };
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EQ1AudioProcessor();
}
