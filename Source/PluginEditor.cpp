#include "PluginEditor.h"

namespace VoxColours
{
    const juce::Colour bg      { 0xff07060f };
    const juce::Colour rack    { 0xff110d1f };
    const juce::Colour module  { 0xff1a1430 };
    const juce::Colour edge    { 0xff2e2350 };
    const juce::Colour violet  { 0xff9d5cff };
    const juce::Colour sky     { 0xff4fc3ff };
    const juce::Colour text    { 0xffeae4ff };
    const juce::Colour muted   { 0xff7d78a3 };
}

namespace
{
    juce::Rectangle<int> rackArea(juce::Rectangle<int> full)
    {
        return full.withTrimmedTop(96).reduced(20, 0).withTrimmedBottom(20);
    }

    juce::Rectangle<int> moduleBounds(juce::Rectangle<int> area, int index, int count)
    {
        const int gap = 12;
        const int w = (area.getWidth() - gap * (count - 1)) / count;
        return { area.getX() + index * (w + gap), area.getY(), w, area.getHeight() };
    }

    juce::Rectangle<int> moduleContent(juce::Rectangle<int> r)
    {
        return r.reduced(10, 0).withTrimmedTop(38).withTrimmedBottom(12);
    }

    std::vector<juce::Rectangle<int>> gridCells(juce::Rectangle<int> a, int cols, int rows)
    {
        std::vector<juce::Rectangle<int>> out;
        const int cw = a.getWidth() / cols;
        const int rh = a.getHeight() / rows;
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c)
                out.push_back({ a.getX() + c * cw, a.getY() + r * rh, cw, rh });
        return out;
    }

    // Coloca una perilla con su etiqueta arriba de la celda
    void placeKnob(VoxStudioEditor::Knob& k, juce::Rectangle<int> cell)
    {
        k.label.setBounds(cell.removeFromTop(14));
        k.slider.setBounds(cell.reduced(2));
    }

    void drawModule(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& name, juce::Colour accent)
    {
        auto f = r.toFloat();

        g.setColour(VoxColours::module);
        g.fillRoundedRectangle(f, 14.0f);

        g.setColour(VoxColours::edge);
        g.drawRoundedRectangle(f.reduced(0.5f), 14.0f, 1.0f);

        juce::ColourGradient grad(VoxColours::violet, f.getX(), f.getY(),
                                  VoxColours::sky, f.getRight(), f.getY(), false);
        g.setGradientFill(grad);
        g.fillRect(f.getX() + 14.0f, f.getY() + 1.0f, f.getWidth() - 28.0f, 2.0f);

        g.setColour(accent);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText(name, r.withHeight(34).reduced(14, 0), juce::Justification::centredLeft, false);
    }
}

// ---------- Look and feel ----------

RackLookAndFeel::RackLookAndFeel()
{
    setColour(juce::ComboBox::backgroundColourId, VoxColours::rack);
    setColour(juce::ComboBox::outlineColourId, VoxColours::violet.withAlpha(0.5f));
    setColour(juce::ComboBox::textColourId, VoxColours::text);
    setColour(juce::ComboBox::arrowColourId, VoxColours::sky);

    setColour(juce::PopupMenu::backgroundColourId, VoxColours::module);
    setColour(juce::PopupMenu::textColourId, VoxColours::text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, VoxColours::violet.withAlpha(0.6f));
    setColour(juce::PopupMenu::highlightedTextColourId, VoxColours::text);

    setColour(juce::Slider::textBoxTextColourId, VoxColours::text);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
}

void RackLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float startAngle, float endAngle,
                                       juce::Slider&)
{
    auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height).reduced(4.0f);
    const float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const float radius = diameter * 0.5f;
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float thickness = 4.0f;
    const float arcR = radius - thickness * 0.5f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    juce::Path track;
    track.addCentredArc(cx, cy, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour(VoxColours::edge);
    g.strokePath(track, juce::PathStrokeType(thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (sliderPos > 0.001f)
    {
        juce::Path value;
        value.addCentredArc(cx, cy, arcR, arcR, 0.0f, startAngle, angle, true);
        juce::ColourGradient grad(VoxColours::violet, cx - radius, cy,
                                  VoxColours::sky, cx + radius, cy, false);
        g.setGradientFill(grad);
        g.strokePath(value, juce::PathStrokeType(thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const float knobR = radius - thickness * 2.5f;
    g.setColour(VoxColours::rack);
    g.fillEllipse(cx - knobR, cy - knobR, knobR * 2.0f, knobR * 2.0f);

    const float inner = knobR * 0.35f;
    const float outer = knobR * 0.85f;
    juce::Line<float> pointer(cx + inner * std::sin(angle), cy - inner * std::cos(angle),
                              cx + outer * std::sin(angle), cy - outer * std::cos(angle));
    g.setColour(VoxColours::text);
    g.drawLine(pointer, 2.0f);
}

// ---------- Editor ----------

VoxStudioEditor::VoxStudioEditor(VoxStudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(980, 600);
    setResizable(false, false);
    setLookAndFeel(&lnf);

    title.setText("VOXSTUDIO", juce::dontSendNotification);
    title.setFont(juce::FontOptions(26.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, VoxColours::text);
    addAndMakeVisible(title);

    subtitle.setText("1.0  ·  Gate  ·  Compressor  ·  SFX  ·  Reverb  ·  Output", juce::dontSendNotification);
    subtitle.setFont(juce::FontOptions(12.0f));
    subtitle.setColour(juce::Label::textColourId, VoxColours::muted);
    addAndMakeVisible(subtitle);

    presetBox.addItemList(VoxStudioProcessor::reverbPresetNames, 1);
    addAndMakeVisible(presetBox);
    presetAttachment = std::make_unique<juce::ComboBoxParameterAttachment>(
        *processor.apvts.getParameter("rev_preset"), presetBox, nullptr);

    setupKnob(gateThreshold, "gate_threshold", "THRESH");
    setupKnob(gateRelease,   "gate_release",   "RELEASE");

    setupKnob(compThreshold, "comp_threshold", "THRESH");
    setupKnob(compRatio,     "comp_ratio",     "RATIO");
    setupKnob(compMakeup,    "comp_makeup",    "MAKEUP");
    setupKnob(compAttack,    "comp_attack",    "ATTACK");
    setupKnob(compRelease,   "comp_release",   "RELEASE");

    setupKnob(sfxDrive,     "sfx_drive",      "DRIVE");
    setupKnob(sfxDelay,     "sfx_delay",      "D.MIX");
    setupKnob(sfxDelayTime, "sfx_delay_time", "D.TIME");

    setupKnob(revRoom, "rev_room", "ROOM");
    setupKnob(revDamp, "rev_damp", "DAMP");
    setupKnob(revMix,  "rev_mix",  "MIX");

    setupKnob(outGain, "out_gain", "OUTPUT");
}

VoxStudioEditor::~VoxStudioEditor()
{
    setLookAndFeel(nullptr);
}

void VoxStudioEditor::setupKnob(Knob& k, const juce::String& paramID, const juce::String& text)
{
    k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 14);
    addAndMakeVisible(k.slider);

    k.label.setText(text, juce::dontSendNotification);
    k.label.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    k.label.setColour(juce::Label::textColourId, VoxColours::muted);
    k.label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(k.label);

    k.attachment = std::make_unique<juce::SliderParameterAttachment>(
        *processor.apvts.getParameter(paramID), k.slider, nullptr);
}

void VoxStudioEditor::paint(juce::Graphics& g)
{
    auto full = getLocalBounds();

    g.setColour(VoxColours::bg);
    g.fillRect(full);

    auto header = full.removeFromTop(84);
    juce::ColourGradient headGrad(VoxColours::violet.withAlpha(0.22f), 0.0f, 0.0f,
                                  VoxColours::sky.withAlpha(0.06f), (float) header.getRight(), 0.0f, false);
    g.setGradientFill(headGrad);
    g.fillRect(header);

    g.setColour(VoxColours::violet.withAlpha(0.7f));
    g.fillRect(header.removeFromBottom(1));

    const char* names[] = { "GATE", "COMPRESSOR", "SFX · DELAY", "REVERB", "OUTPUT" };
    const auto area = rackArea(getLocalBounds());
    const int count = 5;

    for (int i = 0; i < count; ++i)
    {
        const juce::Colour accent = (i % 2 == 0) ? VoxColours::sky : VoxColours::violet;
        drawModule(g, moduleBounds(area, i, count), names[i], accent);
    }
}

void VoxStudioEditor::resized()
{
    title.setBounds(24, 14, 300, 34);
    subtitle.setBounds(24, 50, 600, 18);

    const auto area = rackArea(getLocalBounds());
    const int count = 5;

    const auto gateC = moduleContent(moduleBounds(area, 0, count));
    const auto compC = moduleContent(moduleBounds(area, 1, count));
    const auto sfxC  = moduleContent(moduleBounds(area, 2, count));
    const auto revC  = moduleContent(moduleBounds(area, 3, count));
    const auto outC  = moduleContent(moduleBounds(area, 4, count));

    // Gate: 2 perillas
    const auto gateCells = gridCells(gateC, 2, 1);
    placeKnob(gateThreshold, gateCells[0]);
    placeKnob(gateRelease,   gateCells[1]);

    // Compresor: 3 arriba, 2 abajo
    const auto compCells = gridCells(compC, 3, 2);
    placeKnob(compThreshold, compCells[0]);
    placeKnob(compRatio,     compCells[1]);
    placeKnob(compMakeup,    compCells[2]);
    placeKnob(compAttack,    compCells[3]);
    placeKnob(compRelease,   compCells[4]);

    // SFX: 3 perillas
    const auto sfxCells = gridCells(sfxC, 3, 1);
    placeKnob(sfxDrive,     sfxCells[0]);
    placeKnob(sfxDelay,     sfxCells[1]);
    placeKnob(sfxDelayTime, sfxCells[2]);

    // Reverb: selector de preset arriba y 3 perillas
    auto revArea = revC;
    presetBox.setBounds(revArea.removeFromTop(26).reduced(2, 0));
    revArea.removeFromTop(10);
    const auto revCells = gridCells(revArea, 3, 1);
    placeKnob(revRoom, revCells[0]);
    placeKnob(revDamp, revCells[1]);
    placeKnob(revMix,  revCells[2]);

    // Salida: 1 perilla
    const auto outCells = gridCells(outC, 1, 1);
    placeKnob(outGain, outCells[0]);
}
