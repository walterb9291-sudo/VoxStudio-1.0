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
    void syncPresetBoxes(int globalIndex);
    void highlightCompMode(int mode);
    void setEqBandsVisible(int count);
    juce::Rectangle<float> eqArea() const;
    juce::Point<float> nodePosition(int band) const;
    void setBandFromPoint(int band, juce::Point<float> p);
    int nodeAt(juce::Point<float> p) const;

    VoxStudioProcessor& processor;
    NeonLookAndFeel lnf;

    juce::Label title;
    juce::Label subtitle;
    juce::Label producer;

    juce::ComboBox globalPresetBox;
    juce::ComboBox presetBox;
    juce::ComboBox eqPresetBox;
    juce::ComboBox dynPresetBox;
    juce::ComboBox delayPresetBox;
    juce::ComboBox distPresetBox;
    juce::ComboBox distTypeBox;

    juce::TextButton compModeBtn[4];

    juce::TextButton eqAddBtn { "+" };
    juce::TextButton eqRemoveBtn { "-" };
    int eqBandCount = 4;

    juce::Label bandTitle[VoxStudioProcessor::numEqBands];

    juce::TextButton bypassButton { "BYPASS" };
    std::unique_ptr<juce::ButtonParameterAttachment> bypassAttachment;

    Knob inGain, outGain;

    // Compresor, gate y saturador
    Knob compThreshold, compRatio, compAttack, compRelease, compMakeup;
    Knob dynGate, gateHyst;
    Knob satDrive, satWarm, satTone, satOut;

    // EQ
    Knob eqFreq[VoxStudioProcessor::numEqBands];
    Knob eqGain[VoxStudioProcessor::numEqBands];
    Knob eqQ[VoxStudioProcessor::numEqBands];

    // Reverb y delay
    Knob sfxDrive;
    Knob revRoom, revDecay, revDamp, revMix, revPre, revDiff, revEarly;
    Knob modRate, modDepth;
    Knob delayMix, delayDry, delayFb, delayDrv, delayDiv;

    // Dynamics y distorsión
    Knob dynDeess, dynFocus, dynColor, dynHpFreq, dynLpFreq, dynMix;
    Knob dynDrive, dynDistMix, dynDistTone, dynDistBias, dynDistOut;

    int draggingBand = -1;
    float inLevel = 0.0f;
    float outLevel = 0.0f;
    float grLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoxStudioEditor)
};
