#include "PluginEditor.h"

namespace Neon
{
    const juce::Colour bg      { 0xff05040b };
    const juce::Colour panel   { 0xff0e0b1c };
    const juce::Colour grid    { 0xff1d1738 };
    const juce::Colour violet  { 0xffa855f7 };
    const juce::Colour sky     { 0xff38bdf8 };
    const juce::Colour mint    { 0xff2dd4bf };
    const juce::Colour amber   { 0xfffbbf24 };
    const juce::Colour text    { 0xffeee9ff };
    const juce::Colour muted   { 0xff6b6490 };
}

namespace
{
    void drawPanel(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& name, juce::Colour accent)
    {
        const float c = 12.0f;
        juce::Path p;
        p.startNewSubPath(r.getX() + c, r.getY());
        p.lineTo(r.getRight() - c, r.getY());
        p.lineTo(r.getRight(), r.getY() + c);
        p.lineTo(r.getRight(), r.getBottom() - c);
        p.lineTo(r.getRight() - c, r.getBottom());
        p.lineTo(r.getX() + c, r.getBottom());
        p.lineTo(r.getX(), r.getBottom() - c);
        p.lineTo(r.getX(), r.getY() + c);
        p.closeSubPath();

        g.setColour(Neon::panel);
        g.fillPath(p);

        g.setColour(accent.withAlpha(0.55f));
        g.strokePath(p, juce::PathStrokeType(1.2f));

        juce::ColourGradient glow(accent.withAlpha(0.35f), r.getX(), r.getY(),
                                  accent.withAlpha(0.0f), r.getX(), r.getY() + 60.0f, false);
        g.setGradientFill(glow);
        g.fillRect(r.withHeight(60.0f));

        g.setColour(accent);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText(name, r.removeFromTop(26.0f).reduced(14.0f, 0.0f),
                   juce::Justification::centredLeft, false);
    }

    std::vector<juce::Rectangle<int>> cells(juce::Rectangle<int> a, int cols, int rows)
    {
        std::vector<juce::Rectangle<int>> out;
        const int cw = a.getWidth() / cols;
        const int rh = a.getHeight() / rows;
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c)
                out.push_back({ a.getX() + c * cw, a.getY() + r * rh, cw, rh });
        return out;
    }

    void placeKnob(VoxStudioEditor::Knob& k, juce::Rectangle<int> cell)
    {
        k.label.setBounds(cell.removeFromTop(14));
        k.slider.setBounds(cell.reduced(4));
    }
}

// ---------- Look and feel ----------

NeonLookAndFeel::NeonLookAndFeel()
{
    setColour(juce::ComboBox::backgroundColourId, Neon::panel);
    setColour(juce::ComboBox::outlineColourId, Neon::violet.withAlpha(0.5f));
    setColour(juce::ComboBox::textColourId, Neon::text);
    setColour(juce::ComboBox::arrowColourId, Neon::sky);
    setColour(juce::PopupMenu::backgroundColourId, Neon::panel);
    setColour(juce::PopupMenu::textColourId, Neon::text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, Neon::violet.withAlpha(0.6f));
    setColour(juce::Slider::textBoxTextColourId, Neon::text);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
}

void NeonLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float startAngle, float endAngle,
                                       juce::Slider&)
{
    auto b = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height).reduced(5.0f);
    const float radius = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f;
    const float cx = b.getCentreX();
    const float cy = b.getCentreY();
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    juce::Path ring;
    ring.addCentredArc(cx, cy, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour(Neon::grid);
    g.strokePath(ring, juce::PathStrokeType(3.0f));

    juce::Path value;
    value.addCentredArc(cx, cy, radius, radius, 0.0f, startAngle, angle, true);
    juce::ColourGradient grad(Neon::violet, cx - radius, cy, Neon::mint, cx + radius, cy, false);
    g.setGradientFill(grad);
    g.strokePath(value, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float body = radius * 0.62f;
    g.setColour(Neon::bg);
    g.fillEllipse(cx - body, cy - body, body * 2.0f, body * 2.0f);

    const float px = cx + (body * 0.75f) * std::sin(angle);
    const float py = cy - (body * 0.75f) * std::cos(angle);
    g.setColour(Neon::sky);
    g.fillEllipse(px - 2.5f, py - 2.5f, 5.0f, 5.0f);
}

// ---------- Editor ----------

VoxStudioEditor::VoxStudioEditor(VoxStudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1040, 640);
    setResizable(false, false);
    setLookAndFeel(&lnf);

    title.setText("VOXSTUDIO", juce::dontSendNotification);
    title.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, Neon::text);
    addAndMakeVisible(title);

    subtitle.setText("1.0  //  GATE  ·  COMP  ·  EQ  ·  SFX  ·  REVERB  ·  OUT", juce::dontSendNotification);
    subtitle.setFont(juce::FontOptions(11.0f));
    subtitle.setColour(juce::Label::textColourId, Neon::muted);
    addAndMakeVisible(subtitle);

    presetBox.addItemList(VoxStudioProcessor::reverbPresetNames, 1);
    addAndMakeVisible(presetBox);
    presetAttachment = std::make_unique<juce::ComboBoxParameterAttachment>(
        *processor.apvts.getParameter("rev_preset"), presetBox, nullptr);

    setupKnob(gateThreshold, "gate_threshold", "THRESH");
    setupKnob(gateRelease,   "gate_release",   "RELEASE");

    setupKnob(compThreshold, "comp_threshold", "THRESH");
    setupKnob(compRatio,     "comp_ratio",     "RATIO");
    setupKnob(compAttack,    "comp_attack",    "ATTACK");
    setupKnob(compRelease,   "comp_release",   "RELEASE");
    setupKnob(compMakeup,    "comp_makeup",    "MAKEUP");

    for (int b = 0; b < VoxStudioProcessor::numEqBands; ++b)
    {
        const juce::String n = "eq" + juce::String(b + 1) + "_";
        const juce::String tag = "B" + juce::String(b + 1);
        setupKnob(eqFreq[b], n + "freq", tag + " FREQ");
        setupKnob(eqGain[b], n + "gain", tag + " GAIN");
        setupKnob(eqQ[b],    n + "q",    tag + " Q");
    }

    setupKnob(sfxDrive,     "sfx_drive",      "DRIVE");
    setupKnob(sfxDelay,     "sfx_delay",      "D.MIX");
    setupKnob(sfxDelayTime, "sfx_delay_time", "D.TIME");

    setupKnob(revRoom, "rev_room", "ROOM");
    setupKnob(revDamp, "rev_damp", "DAMP");
    setupKnob(revMix,  "rev_mix",  "MIX");

    setupKnob(outGain, "out_gain", "OUTPUT");

    startTimerHz(30);
}

VoxStudioEditor::~VoxStudioEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void VoxStudioEditor::setupKnob(Knob& k, const juce::String& paramID, const juce::String& text)
{
    k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 14);
    addAndMakeVisible(k.slider);

    k.label.setText(text, juce::dontSendNotification);
    k.label.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    k.label.setColour(juce::Label::textColourId, Neon::muted);
    k.label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(k.label);

    k.attachment = std::make_unique<juce::SliderParameterAttachment>(
        *processor.apvts.getParameter(paramID), k.slider, nullptr);
}

void VoxStudioEditor::timerCallback()
{
    inLevel  = juce::jmax(processor.getInputLevel()  * 0.9f, inLevel  * 0.85f);
    outLevel = juce::jmax(processor.getOutputLevel() * 0.9f, outLevel * 0.85f);
    repaint();
}

void VoxStudioEditor::drawEqCurve(juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::Path curve;
    const int points = 120;

    for (int i = 0; i <= points; ++i)
    {
        const float t = (float) i / (float) points;
        const float freq = 20.0f * std::pow(1000.0f, t);
        float db = 0.0f;

        for (int b = 0; b < VoxStudioProcessor::numEqBands; ++b)
        {
            const float f0 = processor.apvts.getRawParameterValue(
                "eq" + juce::String(b + 1) + "_freq")->load();
            const float g0 = processor.apvts.getRawParameterValue(
                "eq" + juce::String(b + 1) + "_gain")->load();
            const float q  = processor.apvts.getRawParameterValue(
                "eq" + juce::String(b + 1) + "_q")->load();

            const float x = std::log2(freq / f0);
            db += g0 * std::exp(-x * x * q * 2.0f);
        }

        const float y = juce::jmap(juce::jlimit(-14.0f, 14.0f, db), -14.0f, 14.0f,
                                   area.getBottom(), area.getY());
        const float px = area.getX() + t * area.getWidth();
        if (i == 0) curve.startNewSubPath(px, y);
        else        curve.lineTo(px, y);
    }

    g.setColour(Neon::violet);
    g.strokePath(curve, juce::PathStrokeType(2.0f));
}

void VoxStudioEditor::paint(juce::Graphics& g)
{
    g.fillAll(Neon::bg);

    g.setColour(Neon::violet.withAlpha(0.15f));
    g.fillRect(0, 0, getWidth(), 70);
    g.setColour(Neon::sky.withAlpha(0.7f));
    g.fillRect(0, 69, getWidth(), 1);

    const float top = 84.0f;
    const float pad = 16.0f;
    const float colW = (getWidth() - pad * 5.0f) / 4.0f;
    const float h = getHeight() - top - pad;

    drawPanel(g, { pad, top, colW, h },                    "GATE · COMP", Neon::sky);
    drawPanel(g, { pad + (colW + pad), top, colW, h },     "EQ · 4 BAND", Neon::mint);
    drawPanel(g, { pad + 2 * (colW + pad), top, colW, h }, "SFX · REVERB", Neon::violet);
    drawPanel(g, { pad + 3 * (colW + pad), top, colW, h }, "OUTPUT", Neon::amber);

    drawEqCurve(g, { pad + (colW + pad) + 12.0f, top + 140.0f, colW - 24.0f, 150.0f });

    const float meterY = getHeight() - 14.0f;
    g.setColour(Neon::grid);
    g.fillRect(pad, meterY, getWidth() - 2 * pad, 4.0f);
    g.setColour(Neon::mint);
    g.fillRect(pad, meterY, (getWidth() - 2 * pad) * juce::jlimit(0.0f, 1.0f, inLevel), 4.0f);
    g.setColour(Neon::amber);
    g.fillRect(pad, meterY - 8.0f, (getWidth() - 2 * pad) * juce::jlimit(0.0f, 1.0f, outLevel), 4.0f);
}

void VoxStudioEditor::resized()
{
    title.setBounds(20, 12, 400, 30);
    subtitle.setBounds(20, 44, 800, 18);

    const float pad = 16.0f;
    const int top = 84;
    const int colW = (int) ((getWidth() - pad * 5.0f) / 4.0f);
    const int h = getHeight() - top - 16 - 24;

    auto col = [&](int i)
    {
        return juce::Rectangle<int>(16 + i * (colW + 16), top + 26, colW, h - 26).reduced(10, 0);
    };

    // Columna 1: gate y compresor
    auto c1 = col(0);
    auto gateCells = cells(c1.removeFromTop(c1.getHeight() / 3), 2, 1);
    placeKnob(gateThreshold, gateCells[0]);
    placeKnob(gateRelease,   gateCells[1]);
    auto compCells = cells(c1, 2, 3);
    placeKnob(compThreshold, compCells[0]);
    placeKnob(compRatio,     compCells[1]);
    placeKnob(compAttack,    compCells[2]);
    placeKnob(compRelease,   compCells[3]);
    placeKnob(compMakeup,    compCells[4]);

    // Columna 2: EQ (curva arriba, 4 bandas con 3 perillas cada una abajo)
    auto c2 = col(1);
    auto eqBottom = c2.removeFromBottom(c2.getHeight() - 160);
    const auto bandCells = cells(eqBottom, 4, 1);
    for (int b = 0; b < VoxStudioProcessor::numEqBands; ++b)
    {
        auto bandArea = bandCells[(size_t) b];
        auto rows = cells(bandArea, 1, 3);
        placeKnob(eqFreq[b], rows[0]);
        placeKnob(eqGain[b], rows[1]);
        placeKnob(eqQ[b],    rows[2]);
    }

    // Columna 3: SFX y reverb
    auto c3 = col(2);
    auto sfxCells = cells(c3.removeFromTop(c3.getHeight() / 2), 3, 1);
    placeKnob(sfxDrive,     sfxCells[0]);
    placeKnob(sfxDelay,     sfxCells[1]);
    placeKnob(sfxDelayTime, sfxCells[2]);
    presetBox.setBounds(c3.removeFromTop(26).reduced(8, 0));
    auto revCells = cells(c3, 3, 1);
    placeKnob(revRoom, revCells[0]);
    placeKnob(revDamp, revCells[1]);
    placeKnob(revMix,  revCells[2]);

    // Columna 4: salida
    auto c4 = col(3);
    placeKnob(outGain, c4.removeFromTop(c4.getHeight() / 2));
}
