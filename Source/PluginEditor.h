/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class ConvolutionPluginAudioProcessorEditor : public juce::AudioProcessorEditor,
                                              private juce::Slider::Listener,
                                              private juce::ToggleButton::Listener
{
public:
    ConvolutionPluginAudioProcessorEditor (ConvolutionPluginAudioProcessor&);
    ~ConvolutionPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ConvolutionPluginAudioProcessor& audioProcessor;
    
    juce::Slider midiVolume;
    juce::ToggleButton reverbButton { "Reverb" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConvolutionPluginAudioProcessorEditor)
};
