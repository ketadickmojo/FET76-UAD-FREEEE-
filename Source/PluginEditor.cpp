#include "PluginEditor.h"

namespace
{
    void setupRotary (juce::Slider& s, juce::Label& l, juce::Component& parent, const juce::String& text)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
        parent.addAndMakeVisible (s);

        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        parent.addAndMakeVisible (l);
    }
}

FET76AudioProcessorEditor::FET76AudioProcessorEditor (FET76AudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setupRotary (inputSlider,   inputLabel,   *this, "Input");
    setupRotary (outputSlider,  outputLabel,  *this, "Output");
    setupRotary (attackSlider,  attackLabel,  *this, "Attack");
    setupRotary (releaseSlider, releaseLabel, *this, "Release");
    setupRotary (mixSlider,     mixLabel,     *this, "Mix");

    ratioBox.addItemList ({ "4:1", "8:1", "12:1", "20:1", "All Buttons In" }, 1);
    addAndMakeVisible (ratioBox);
    ratioLabel.setText ("Ratio", juce::dontSendNotification);
    ratioLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (ratioLabel);

    auto& apvts = processor.apvts;
    inputAttach   = std::make_unique<SliderAttachment> (apvts, "inputGain",  inputSlider);
    outputAttach  = std::make_unique<SliderAttachment> (apvts, "outputGain", outputSlider);
    attackAttach  = std::make_unique<SliderAttachment> (apvts, "attack",     attackSlider);
    releaseAttach = std::make_unique<SliderAttachment> (apvts, "release",    releaseSlider);
    mixAttach     = std::make_unique<SliderAttachment> (apvts, "mix",        mixSlider);
    ratioAttach   = std::make_unique<ComboAttachment>  (apvts, "ratio",      ratioBox);

    setSize (520, 260);
    startTimerHz (30);
}

void FET76AudioProcessorEditor::timerCallback()
{
    float target = processor.currentGrDb.load();
    displayedGrDb += (target - displayedGrDb) * 0.3f; // сглаживание для VU
    repaint();
}

void FET76AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff2b2420)); // тёплый тёмный "железный" фон

    g.setColour (juce::Colour (0xffd9c9a3));
    g.setFont (juce::Font (20.0f, juce::Font::bold));
    g.drawText ("FET76 -- Digital 1176-Style Compressor", getLocalBounds().removeFromTop (30),
                juce::Justification::centred);

    // VU-метр гейн-редакции
    auto meterArea = getLocalBounds().removeFromBottom (40).reduced (20, 5);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (meterArea.toFloat(), 4.0f);

    float grNorm = juce::jlimit (0.0f, 1.0f, displayedGrDb / 20.0f); // 0-20 дБ GR
    auto fill = meterArea.reduced (2).withWidth ((int) (meterArea.getWidth() * grNorm));
    g.setColour (juce::Colours::orangered);
    g.fillRoundedRectangle (fill.toFloat(), 3.0f);

    g.setColour (juce::Colours::white);
    g.setFont (12.0f);
    g.drawText (juce::String (displayedGrDb, 1) + " dB GR", meterArea,
                juce::Justification::centred);
}

void FET76AudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (30);   // заголовок
    area.removeFromBottom (50); // метр

    auto knobWidth = area.getWidth() / 6;

    auto place = [&] (juce::Slider& s, juce::Label& l)
    {
        auto col = area.removeFromLeft (knobWidth);
        l.setBounds (col.removeFromTop (18));
        s.setBounds (col.reduced (4));
    };

    place (inputSlider, inputLabel);
    place (attackSlider, attackLabel);
    place (releaseSlider, releaseLabel);
    place (outputSlider, outputLabel);
    place (mixSlider, mixLabel);

    auto ratioCol = area;
    ratioLabel.setBounds (ratioCol.removeFromTop (18));
    ratioBox.setBounds (ratioCol.reduced (4).removeFromTop (30));
}
