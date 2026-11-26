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
    juce::Font getComboBoxFont(juce::ComboBox& box) override;
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
    void mouseDoubleClick(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void timerCallback() override;
    void setupKnob(Knob& k, const juce::String& paramID, const juce::String& text);
    void setupPending(Knob& k, const juce::String& text);
    void setDelayDivision(int index);
    juce::Rectangle<float> eqArea() const;
    juce::Point<float> nodePosition(int band) const;
    void setBandFromPoint(int band, juce::Point<float> p);
    int nodeAt(juce::Point<float> p) const;

    VoxStudioProcessor& processor;
    NeonLookAndFeel lnf;

    juce::Label title;
    juce::Label subtitle;
    juce::Label producer;

    juce::TextEditor searchBox;

    juce::ComboBox presetBox;
    std::unique_ptr<juce::ComboBoxParameterAttachment> presetAttachment;

    juce::ComboBox eqPresetBox;
    juce::ComboBox dynPresetBox;
    juce::ComboBox delayPresetBox;

    juce::TextButton bypassButton { "BYPASS" };
    std::unique_ptr<juce::ButtonParameterAttachment> bypassAttachment;

    Knob inGain, outGain;
    Knob compThreshold, compRatio, compAttack, compRelease, compMakeup;
    Knob gateHyst;
    Knob eqFreq[VoxStudioProcessor::numEqBands];
    Knob eqGain[VoxStudioProcessor::numEqBands];
    Knob eqQ[VoxStudioProcessor::numEqBands];
    Knob sfxDrive;
    Knob delayMix, delayDry, delayFb, delayDrv, delayDiv;
    Knob revRoom, revDecay, revDamp, revMix, revPre;
    Knob modRate, modDepth;
    Knob dynGate, dynDeess, dynFocus;
    Knob dynDrive, dynColor, dynHpFreq, dynLpFreq, dynMix;

    int draggingBand = -1;
    float inLevel = 0.0f;
    float outLevel = 0.0f;
    float grLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoxStudioEditor)
};
