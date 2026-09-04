#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class FET76AudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit FET76AudioProcessorEditor (FET76AudioProcessor&);
    ~FET76AudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    FET76AudioProcessor& processor;

    juce::Slider inputSlider, outputSlider, attackSlider, releaseSlider, mixSlider;
    juce::Label  inputLabel, outputLabel, attackLabel, releaseLabel, mixLabel, ratioLabel;
    juce::ComboBox ratioBox;

    // VU-style индикатор гейн-редакции
    float displayedGrDb = 0.0f;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SliderAttachment> inputAttach, outputAttach, attackAttach, releaseAttach, mixAttach;
    std::unique_ptr<ComboAttachment>  ratioAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FET76AudioProcessorEditor)
};
