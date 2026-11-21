#pragma once

#include "PluginProcessor.h"

class VoxStudioEditor : public juce::AudioProcessorEditor
{
public:
    explicit VoxStudioEditor(VoxStudioProcessor&);
    ~VoxStudioEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    VoxStudioProcessor& processor;

    struct KnobWithLabel
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<juce::SliderParameterAttachment> attachment;
    };

    void setupKnob(KnobWithLabel& k, const juce::String& paramID, const juce::String& text);
    void drawPanel(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& name, juce::Colour accent);

    juce::Label title;
    juce::Label subtitle;
    juce::ComboBox presetBox;
    std::unique_ptr<juce::ComboBoxParameterAttachment> presetAttachment;

    KnobWithLabel compThreshold, compRatio, compAttack, compRelease, compMakeup;
    KnobWithLabel revRoom, revDamp, revMix;
    KnobWithLabel gateThreshold, gateRelease;
    KnobWithLabel sfxDrive, sfxDelay, sfxDelayTime;
    KnobWithLabel outGain;

    juce::LookAndFeel_V4 lnf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoxStudioEditor)
};
