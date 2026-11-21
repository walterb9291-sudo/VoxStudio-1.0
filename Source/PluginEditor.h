#pragma once

#include "PluginProcessor.h"

class RackLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RackLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;
};

class VoxStudioEditor : public juce::AudioProcessorEditor
{
public:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::SliderParameterAttachment> attachment;
    };

    explicit VoxStudioEditor(VoxStudioProcessor&);
    ~VoxStudioEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void setupKnob(Knob& k, const juce::String& paramID, const juce::String& text);

    VoxStudioProcessor& processor;

    RackLookAndFeel lnf;

    juce::Label title;
    juce::Label subtitle;

    juce::ComboBox presetBox;
    std::unique_ptr<juce::ComboBoxParameterAttachment> presetAttachment;

    Knob compThreshold, compRatio, compAttack, compRelease, compMakeup;
    Knob revRoom, revDamp, revMix;
    Knob gateThreshold, gateRelease;
    Knob sfxDrive, sfxDelay, sfxDelayTime;
    Knob outGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoxStudioEditor)
};
