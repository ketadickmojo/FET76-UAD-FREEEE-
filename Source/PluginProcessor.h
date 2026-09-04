#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "CompressorDSP.h"

class FET76AudioProcessor : public juce::AudioProcessor
{
public:
    FET76AudioProcessor();
    ~FET76AudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "FET76"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    juce::AudioProcessorValueTreeState apvts;

    // текущая гейн-редакция для отображения на UI (в дБ, среднее по каналам)
    std::atomic<float> currentGrDb { 0.0f };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    FET76Compressor compL, compR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FET76AudioProcessor)
};
