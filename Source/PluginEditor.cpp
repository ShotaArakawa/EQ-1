/*
  ==============================================================================

    EQ1AudioProcessorEditor

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EQ1AudioProcessorEditor::EQ1AudioProcessorEditor (EQ1AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      topBar (p), graph (p), controlStrip (p)
{
    setLookAndFeel (&lookAndFeel);

    graph.onBandSelected = [this] (int bandIndex)
    {
        controlStrip.setSelectedBand (bandIndex);
    };

    addAndMakeVisible (topBar);
    addAndMakeVisible (graph);
    addAndMakeVisible (controlStrip);

    // 起動時点で有効なバンドがあれば、最初の1つを選択状態にしてコントロールストリップに反映する
    for (int i = 0; i < EQ1AudioProcessor::maxNumBands; ++i)
    {
        if (audioProcessor.getBandSettings (i).enabled)
        {
            graph.selectBand (i);
            break;
        }
    }

    setResizable (true, true);
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio (1000.0 / 640.0);
    setResizeLimits (700, 448, 1600, 1024);
    setSize (1000, 640);
}

EQ1AudioProcessorEditor::~EQ1AudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

//==============================================================================
void EQ1AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff101113));
}

void EQ1AudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    topBar.setBounds (area.removeFromTop (48));
    controlStrip.setBounds (area.removeFromBottom (72));
    graph.setBounds (area.reduced (4));
}
