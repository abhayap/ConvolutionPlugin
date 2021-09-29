/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include <thread>

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ConvolutionPluginAudioProcessor::ConvolutionPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::discreteChannels(ARRAY_MICROPHONES), true)
                       .withOutput ("Output", juce::AudioChannelSet::ambisonic(ARRAY_ORDER), true)
                       ),
#endif
    messageQueue(100)
{
    auto numImpulses = ARRAY_MICROPHONES * ARRAY_HARMONICS;
    for (auto i = 0; i < numImpulses; i++)
    {
        convolvers.emplace_back( new juce::dsp::Convolution(juce::dsp::Convolution::Latency {256}, messageQueue) );
    }
    
    auto dir = juce::File::getSpecialLocation(juce::File::userHomeDirectory);

    int numTries = 0;

    while (! dir.getChildFile("dev").exists() && numTries++ < 15)
        dir = dir.getParentDirectory();
    
    for (auto & convolver : convolvers)
    {
        convolver->loadImpulseResponse(dir.getChildFile("dev").getChildFile("resources").getChildFile("large_church.wav"), juce::dsp::Convolution::Stereo::no, juce::dsp::Convolution::Trim::no, 1024);
    }
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
    jassert(totalNumInputChannels >= 64);
    jassert(totalNumOutputChannels >= 36);

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    {
        buffer.clear (i, 0, buffer.getNumSamples());
    }
    
    auto outBuffer = juce::AudioBuffer<float> {buffer.getNumChannels(), buffer.getNumSamples()};
    outBuffer.clear();  // buffer data is not empty when initialized
    
    // create blocks for the buffers - no memory copied or created
    auto inBlock = juce::dsp::AudioBlock<float>(buffer);
    auto outBlock = juce::dsp::AudioBlock<float>(outBuffer);
    
    std::vector<std::thread> threads;
    
    if (reverbOn)
    {
        for (auto harm = 0; harm < ARRAY_HARMONICS; harm++)
        {
            threads.push_back(std::thread(&ConvolutionPluginAudioProcessor::processHarmonic, this, harm, std::ref(inBlock), std::ref(outBlock)));
        }
        
        for (auto &th : threads)
        {
          th.join();
        }
        
        //outBuffer.applyGain(1.0f/64.0f);
        buffer.makeCopyOf(outBuffer, true);
    }
    
    buffer.applyGain(outputVol);
}

void ConvolutionPluginAudioProcessor::processHarmonic(int harmonic, const juce::dsp::AudioBlock<float>& inBlock, juce::dsp::AudioBlock<float>& outBlock)
{
    auto tmpBuffer = juce::AudioBuffer<float> {1, static_cast<int> (inBlock.getNumSamples())};
    auto tmpBlock = juce::dsp::AudioBlock<float>(tmpBuffer);
    
    for (auto mic = 0; mic < ARRAY_MICROPHONES; mic++)
    {
        auto idx = (harmonic * ARRAY_MICROPHONES) + mic;
        auto singleInBlock = inBlock.getSingleChannelBlock(mic);
        auto context = juce::dsp::ProcessContextNonReplacing<float>(singleInBlock, tmpBlock);
        convolvers[idx]->process(context);
        outBlock.getSingleChannelBlock(harmonic).add(tmpBlock);
    }
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
