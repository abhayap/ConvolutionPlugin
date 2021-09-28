/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ConvolutionPluginAudioProcessor::ConvolutionPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::discreteChannels(ARRAY_MICROPHONES), true)
                       .withOutput ("Output", juce::AudioChannelSet::ambisonic(ARRAY_ORDER), true)
                       )
#endif
{
    auto numImpulses = ARRAY_MICROPHONES * ARRAY_HARMONICS;
    for (auto i = 0; i < numImpulses; i++)
    {
        convolvers.emplace_back(new juce::dsp::Convolution());
    }
    
    auto dir = juce::File::getSpecialLocation(juce::File::userHomeDirectory);

    int numTries = 0;

    while (! dir.getChildFile("dev").exists() && numTries++ < 15)
        dir = dir.getParentDirectory();
    
    for (auto & convolver : convolvers)
    {
        convolver->loadImpulseResponse(dir.getChildFile("dev").getChildFile("resources").getChildFile("large_church.wav"), juce::dsp::Convolution::Stereo::no, juce::dsp::Convolution::Trim::no, 1024);
    }
    
    convolverL.loadImpulseResponse(dir.getChildFile("dev").getChildFile("resources").getChildFile("large_church.wav"), juce::dsp::Convolution::Stereo::no, juce::dsp::Convolution::Trim::no, 0);
    convolverR.loadImpulseResponse(dir.getChildFile("dev").getChildFile("resources").getChildFile("large_church.wav"), juce::dsp::Convolution::Stereo::no, juce::dsp::Convolution::Trim::no, 0);
}

ConvolutionPluginAudioProcessor::~ConvolutionPluginAudioProcessor()
{
}

//==============================================================================
const juce::String ConvolutionPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ConvolutionPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ConvolutionPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ConvolutionPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ConvolutionPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ConvolutionPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ConvolutionPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ConvolutionPluginAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ConvolutionPluginAudioProcessor::getProgramName (int index)
{
    return {};
}

void ConvolutionPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ConvolutionPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    for (auto & convolver : convolvers)
    {
        convolver->prepare({ sampleRate, (juce::uint32) samplesPerBlock, 1 });
    }
    
    convolverL.prepare({ sampleRate, (juce::uint32) samplesPerBlock, 1 });
    convolverR.prepare({ sampleRate, (juce::uint32) samplesPerBlock, 1 });
}

void ConvolutionPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ConvolutionPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::discreteChannels(64)
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::ambisonic(5))
    {
        return false;
    }

    return true;
}
#endif

void ConvolutionPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    if (reverbOn) {
        auto block = juce::dsp::AudioBlock<float>(buffer);
        auto singleBlockL = block.getSingleChannelBlock(0);
        auto contextL = juce::dsp::ProcessContextReplacing<float>(singleBlockL);
//        convolverL.process(contextL);
        convolvers[0]->process(contextL);
        singleBlockL *= 6.0f;
        auto singleBlockR = block.getSingleChannelBlock(1);
        auto contextR = juce::dsp::ProcessContextReplacing<float>(singleBlockR);
//        convolverR.process(contextR);
        convolvers[1]->process(contextR);
        singleBlockR *= 6.0f;
    }
    
    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
//    for (int channel = 0; channel < totalNumInputChannels; ++channel)
//    {
//        auto* channelData = buffer.getWritePointer (channel);
//        auto* inputData = buffer.getReadPointer(channel);
//        for (auto samp = 0; samp < buffer.getNumSamples(); ++samp)
//        {
//            channelData[samp] = outputVol * inputData[samp];
//        }
//    }
}

//==============================================================================
bool ConvolutionPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ConvolutionPluginAudioProcessor::createEditor()
{
    return new ConvolutionPluginAudioProcessorEditor (*this);
}

//==============================================================================
void ConvolutionPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void ConvolutionPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ConvolutionPluginAudioProcessor();
}
