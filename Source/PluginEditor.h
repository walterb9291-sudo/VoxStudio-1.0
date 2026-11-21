#pragma once

#include "PluginProcessor.h"

class NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeonLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;
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
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void timerCallback() override;
    void setupKnob(Knob& k, const juce::String& paramID, const juce::String& text);
    juce::Rectangle<float> eqArea() const;
    juce::Point<float> nodePosition(int band) const;
    void setBandFromPoint(int band, juce::Point<float> p);
    int nodeAt(juce::Point<float> p) const;

    VoxStudioProcessor& processor;
    NeonLookAndFeel lnf;

    juce::Label title;
    juce::Label subtitle;
    juce::Label producer;

    juce::ComboBox presetBox;
    std::unique_ptr<juce::ComboBoxParameterAttachment> presetAttachment;

    juce::TextButton bypassButton { "BYPASS" };
    bool bypassed = false;

    Knob gateThreshold, gateRelease;
    Knob compThreshold, compRatio, compAttack, compRelease, compMakeup;
    Knob eqFreq[VoxStudioProcessor::numEqBands];
    Knob eqGain[VoxStudioProcessor::numEqBands];
    Knob eqQ[VoxStudioProcessor::numEqBands];
    Knob sfxDrive, sfxDelay, sfxDelayTime;
    Knob revRoom, revDamp, revMix;
    Knob outGain;

    int draggingBand = -1;
    float inLevel = 0.0f;
    float outLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoxStudioEditor)
};
