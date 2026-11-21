#include "PluginEditor.h"

namespace VoxColours
{
    const juce::Colour background { 0xff0a0814 };
    const juce::Colour panel      { 0xff15102a };
    const juce::Colour panelEdge  { 0xff2a1f4d };
    const juce::Colour violet     { 0xff9d5cff };
    const juce::Colour sky        { 0xff4fc3ff };
    const juce::Colour text       { 0xffe8e2ff };
    const juce::Colour muted      { 0xff7f7aa0 };
}

VoxStudioEditor::VoxStudioEditor(VoxStudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(920, 540);
    setResizable(false, false);

    title.setText("VOXSTUDIO", juce::dontSendNotification);
    title.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, VoxColours::sky);
    addAndMakeVisible(title);

    subtitle.setText("1.0  ·  Compressor  ·  Reverb  ·  Dynamics  ·  SFX", juce::dontSendNotification);
    subtitle.setFont(juce::FontOptions(12.0f));
    subtitle.setColour(juce::Label::textColourId, VoxColours::muted);
    addAndMakeVisible(subtitle);

    presetBox.addItemList(VoxStudioProcessor::reverbPresetNames, 1);
    presetBox.setColour(juce::ComboBox::backgroundColourId, VoxColours::panel);
    presetBox.setColour(juce::ComboBox::outlineColourId, VoxColours::violet.withAlpha(0.5f));
    presetBox.setColour(juce::ComboBox::textColourId, VoxColours::text);
    addAndMakeVisible(presetBox);
    presetAttachment = std::make_unique<juce::ComboBoxParameterAttachment>(
        *processor.apvts.getParameter("rev_preset"), presetBox, nullptr);

    setupKnob(compThreshold, "comp_threshold", "THRESH");
    setupKnob(compRatio,     "comp_ratio",     "RATIO");
    setupKnob(compAttack,    "comp_attack",    "ATTACK");
    setupKnob(compRelease,   "comp_release",   "RELEASE");
    setupKnob(compMakeup,    "comp_makeup",    "MAKEUP");

    setupKnob(revRoom, "rev_room", "ROOM");
    setupKnob(revDamp, "rev_damp", "DAMP");
    setupKnob(revMix,  "rev_mix",  "MIX");

    setupKnob(gateThreshold, "gate_threshold", "GATE");
    setupKnob(gateRelease,   "gate_release",   "G.REL");

    setupKnob(sfxDrive,     "sfx_drive",      "DRIVE");
    setupKnob(sfxDelay,     "sfx_delay",      "D.MIX");
    setupKnob(sfxDelayTime, "sfx_delay_time", "D.TIME");

    setupKnob(outGain, "out_gain", "OUTPUT");

    setLookAndFeel(&lnf);
}

VoxStudioEditor::~VoxStudioEditor()
{
    setLookAndFeel(nullptr);
}

void VoxStudioEditor::setupKnob(KnobWithLabel& k, const juce::String& paramID, const juce::String& text)
{
    k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 16);
    k.slider.setColour(juce::Slider::rotarySliderFillColourId, VoxColours::violet);
    k.slider.setColour(juce::Slider::rotarySliderOutlineColourId, VoxColours::panelEdge);
    k.slider.setColour(juce::Slider::thumbColourId, VoxColours::sky);
    k.slider.setColour(juce::Slider::textBoxTextColourId, VoxColours::text);
    k.slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(k.slider);

    k.label.setText(text, juce::dontSendNotification);
    k.label.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    k.label.setColour(juce::Label::textColourId, VoxColours::muted);
    k.label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(k.label);

    k.attachment = std::make_unique<juce::SliderParameterAttachment>(
        *processor.apvts.getParameter(paramID), k.slider, nullptr);
}

void VoxStudioEditor::drawPanel(juce::Graphics& g, juce::Rectangle<int> r,
                                const juce::String& name, juce::Colour accent)
{
    g.setColour(VoxColours::panel);
    g.fillRoundedRectangle(r.toFloat(), 14.0f);

    g.setColour(accent.withAlpha(0.45f));
    g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 14.0f, 1.0f);

    g.setColour(accent);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText(name, r.removeFromTop(28).reduced(16, 0), juce::Justification::centredLeft);
}

void VoxStudioEditor::paint(juce::Graphics& g)
{
    g.fillAll(VoxColours::background);

    juce::ColourGradient grad(VoxColours::violet.withAlpha(0.28f), 0.0f, 0.0f,
                              VoxColours::sky.withAlpha(0.10f), (float) getWidth(), 0.0f, false);
    g.setGradientFill(grad);
    g.fillRect(0, 0, getWidth(), 76);

    const auto body = getLocalBounds().withTrimmedTop(88).reduced(20, 0).withTrimmedBottom(20);
    const int gap = 16;
    const int colW = (body.getWidth() - 2 * gap) / 3;

    drawPanel(g, body.withWidth(colW), "COMPRESSOR", VoxColours::violet);
    drawPanel(g, body.withX(body.getX() + colW + gap).withWidth(colW), "REVERB", VoxColours::sky);
    drawPanel(g, body.withX(body.getX() + 2 * (colW + gap)).withWidth(colW), "DYNAMICS · SFX", VoxColours::violet);
}

void VoxStudioEditor::resized()
{
    auto header = getLocalBounds().removeFromTop(76).reduced(20, 14);
    title.setBounds(header.removeFromTop(28));
    subtitle.setBounds(header.removeFromTop(16));

    const auto body = getLocalBounds().withTrimmedTop(88).reduced(20, 0).withTrimmedBottom(20);
    const int gap = 16;
    const int colW = (body.getWidth() - 2 * gap) / 3;

    auto place = [](KnobWithLabel& k, juce::Rectangle<int> cell)
    {
        k.label.setBounds(cell.removeFromBottom(16));
        k.slider.setBounds(cell.reduced(2));
    };

    auto grid = [](juce::Rectangle<int> area, int cols, int rows)
    {
        std::vector<juce::Rectangle<int>> cells;
        const int cw = area.getWidth() / cols;
        const int rh = area.getHeight() / rows;
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c)
                cells.push_back({ area.getX() + c * cw, area.getY() + r * rh, cw, rh });
        return cells;
    };

    auto compArea = body.withWidth(colW).reduced(12, 36);
    auto cc = grid(compArea, 3, 2);
    place(compThreshold, cc[0]); place(compRatio, cc[1]); place(compMakeup, cc[2]);
    place(compAttack,    cc[3]); place(compRelease, cc[4]);

    auto revArea = body.withX(body.getX() + colW + gap).withWidth(colW).reduced(12, 36);
    presetBox.setBounds(revArea.removeFromTop(28).reduced(24, 0));
    auto rc = grid(revArea.reduced(0, 8), 3, 1);
    place(revRoom, rc[0]); place(revDamp, rc[1]); place(revMix, rc[2]);

    auto dArea = body.withX(body.getX() + 2 * (colW + gap)).withWidth(colW).reduced(12, 36);
    auto dRows = grid(dArea, 1, 3);
    auto gateCells = grid(dRows[0], 2, 1);
    place(gateThreshold, gateCells[0]); place(gateRelease, gateCells[1]);
    auto sfxCells = grid(dRows[1], 3, 1);
    place(sfxDrive, sfxCells[0]); place(sfxDelay, sfxCells[1]); place(sfxDelayTime, sfxCells[2]);
    place(outGain, dRows[2]);
}
