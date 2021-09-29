/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ConvolutionPluginAudioProcessorEditor::ConvolutionPluginAudioProcessorEditor (ConvolutionPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (200, 200);
    
    // define parameters of the slider
    midiVolume.setSliderStyle(juce::Slider::LinearBarVertical);
    midiVolume.setRange(0.0, 20.0, 0.1);
    midiVolume.setTextBoxStyle(juce::Slider::NoTextBox, false, 90, 0);
    midiVolume.setPopupDisplayEnabled(true, false, this);
    midiVolume.setTextValueSuffix(" Volume");
    midiVolume.setValue(1.0);
    
    // add slider to the editor
    addAndMakeVisible(&midiVolume);
    
    // check box
    addAndMakeVisible(&reverbButton);
    
    // add the listener to the slider
    midiVolume.addListener(this);
    
    // add the listener to the button
    reverbButton.addListener(this);
}

ConvolutionPluginAudioProcessorEditor::~ConvolutionPluginAudioProcessorEditor()
{
}

//==============================================================================
void ConvolutionPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Output Volume", 0, 0, getWidth(), 30, juce::Justification::centred, 1);
}

void ConvolutionPluginAudioProcessorEditor::resized()
{
    // set position and size of the slider
    midiVolume.setBounds(40, 30, 20, getHeight() - 60);
    
    reverbButton.setBounds(100, 50, 60, 20);
    
}

void ConvolutionPluginAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    audioProcessor.outputVol = midiVolume.getValue();
}

void ConvolutionPluginAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    audioProcessor.reverbOn = reverbButton.getToggleState();
}
