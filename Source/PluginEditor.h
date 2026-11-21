#pragma once

#include "PluginProcessor.h"

class NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeonLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;
};

class VoxStudioEditor : public juce::AudioProcessorEditor, private juce::Timer
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
    void timerCallback() override;
    void setupKnob(Knob& k, const juce::String& paramID, const juce::String& text);
    void drawEqCurve(juce::Graphics& g, juce::Rectangle<float> area);

    VoxStudioProcessor& processor;
    NeonLookAndFeel lnf;

    juce::Label title;
    juce::Label subtitle;
    juce::ComboBox presetBox;
    std::unique_ptr<juce::ComboBoxParameterAttachment> presetAttachment;

    Knob gateThreshold, gateRelease;
    Knob compThreshold, compRatio, compAttack, compRelease, compMakeup;
    Knob eqFreq[VoxStudioProcessor::numEqBands];
    Knob eqGain[VoxStudioProcessor::numEqBands];
    Knob eqQ[VoxStudioProcessor::numEqBands];
    Knob sfxDrive, sfxDelay, sfxDelayTime;
    Knob revRoom, revDamp, revMix;
    Knob outGain;

    float inLevel = 0.0f;
    float outLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoxStudioEditor)
};
