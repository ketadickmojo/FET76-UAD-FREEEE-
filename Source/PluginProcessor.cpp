#include "PluginProcessor.h"
#include "PluginEditor.h"

FET76AudioProcessor::FET76AudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout FET76AudioProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "inputGain", 1 }, "Input",
        NormalisableRange<float> (-20.0f, 40.0f, 0.1f), 0.0f,
        AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "outputGain", 1 }, "Output",
        NormalisableRange<float> (-40.0f, 20.0f, 0.1f), 0.0f,
        AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "attack", 1 }, "Attack",
        NormalisableRange<float> (0.02f, 0.8f, 0.001f), 0.3f,
        AudioParameterFloatAttributes().withLabel ("ms")));

    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "release", 1 }, "Release",
        NormalisableRange<float> (50.0f, 1100.0f, 1.0f), 250.0f,
        AudioParameterFloatAttributes().withLabel ("ms")));

    params.push_back (std::make_unique<AudioParameterChoice>(
        ParameterID { "ratio", 1 }, "Ratio",
        StringArray { "4:1", "8:1", "12:1", "20:1", "All Buttons In" }, 0));

    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "mix", 1 }, "Mix",
        NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));

    return { params.begin(), params.end() };
}

void FET76AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    compL.prepare (sampleRate, samplesPerBlock);
    compR.prepare (sampleRate, samplesPerBlock);
}

bool FET76AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo();
}

void FET76AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const float inGain  = apvts.getRawParameterValue ("inputGain")->load();
    const float outGain = apvts.getRawParameterValue ("outputGain")->load();
    const float attack  = apvts.getRawParameterValue ("attack")->load();
    const float release = apvts.getRawParameterValue ("release")->load();
    const int   ratioIdx = (int) apvts.getRawParameterValue ("ratio")->load();
    const float mix      = apvts.getRawParameterValue ("mix")->load();

    auto ratio = static_cast<FET76Compressor::Ratio> (ratioIdx);

    for (auto* c : { &compL, &compR })
    {
        c->setInputGainDb (inGain);
        c->setOutputGainDb (outGain);
        c->setAttackMs (attack);
        c->setReleaseMs (release);
        c->setRatio (ratio);
        c->setMix (mix);
    }

    auto* left  = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    const int numSamples = buffer.getNumSamples();

    float grSum = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        left[i] = compL.processSample (left[i]);
        if (right != nullptr)
            right[i] = compR.processSample (right[i]);

        grSum += compL.getGainReductionDb();
    }

    currentGrDb.store (numSamples > 0 ? grSum / (float) numSamples : 0.0f);
}

juce::AudioProcessorEditor* FET76AudioProcessor::createEditor()
{
    return new FET76AudioProcessorEditor (*this);
}

void FET76AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        std::unique_ptr<juce::XmlElement> xml (state.createXml());
        copyXmlToBinary (*xml, destData);
    }
}

void FET76AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// Обязательная фабричная функция JUCE
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FET76AudioProcessor();
}
