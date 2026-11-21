#include "PluginEditor.h"

namespace Neon
{
    const juce::Colour bg      { 0xff05040b };
    const juce::Colour panel   { 0xff0a0816 };
    const juce::Colour panelHi { 0xff19122f };
    const juce::Colour grid    { 0xff2a2150 };
    const juce::Colour violet  { 0xffa855f7 };
    const juce::Colour sky     { 0xff38bdf8 };
    const juce::Colour mint    { 0xff2dd4bf };
    const juce::Colour amber   { 0xfffbbf24 };
    const juce::Colour text    { 0xffeee9ff };
    const juce::Colour muted   { 0xff6b6490 };
}

namespace
{
    constexpr int pad      = 16;
    constexpr int headerH  = 74;
    constexpr int panelTop = 88;
    constexpr int footerH  = 34;

    juce::Rectangle<int> panelRect(int i, int w, int h)
    {
        const int colW = (w - pad * 5) / 4;
        return { pad + i * (colW + pad), panelTop, colW, h - panelTop - pad - footerH };
    }

    juce::Rectangle<int> innerRect(juce::Rectangle<int> p)
    {
        return p.reduced(14, 0).withTrimmedTop(46).withTrimmedBottom(14);
    }

    std::vector<juce::Rectangle<int>> cellsOf(juce::Rectangle<int> a, int cols, int rows)
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
        k.label.setBounds(cell.removeFromBottom(16));
        k.slider.setBounds(cell.reduced(2));
    }

    void drawIcon(juce::Graphics& g, juce::Rectangle<float> r, int kind, juce::Colour c)
    {
        g.setColour(c);
        juce::Path p;
        switch (kind)
        {
            case 0:
                p.startNewSubPath(r.getX(), r.getCentreY());
                p.lineTo(r.getX() + r.getWidth() * 0.3f, r.getCentreY());
                p.lineTo(r.getX() + r.getWidth() * 0.45f, r.getY());
                p.lineTo(r.getX() + r.getWidth() * 0.6f, r.getBottom());
                p.lineTo(r.getRight(), r.getCentreY());
                g.strokePath(p, juce::PathStrokeType(1.6f));
                break;
            case 1:
                g.fillRoundedRectangle(r.getX(), r.getY() + r.getHeight() * 0.3f, 4.0f, r.getHeight() * 0.7f, 1.5f);
                g.fillRoundedRectangle(r.getX() + 7.0f, r.getY(), 4.0f, r.getHeight(), 1.5f);
                g.fillRoundedRectangle(r.getX() + 14.0f, r.getY() + r.getHeight() * 0.5f, 4.0f, r.getHeight() * 0.5f, 1.5f);
                break;
            case 2:
                g.drawEllipse(r.reduced(r.getWidth() * 0.3f), 1.5f);
                g.fillEllipse(r.getCentreX() - 2.0f, r.getCentreY() - 2.0f, 4.0f, 4.0f);
                break;
            case 3:
                g.drawEllipse(r.reduced(1.0f), 1.5f);
                g.drawEllipse(r.reduced(r.getWidth() * 0.28f), 1.0f);
                break;
            default:
                break;
        }
    }

    void drawPanel(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& name,
                   juce::Colour accent, int icon)
    {
        const float c = 14.0f;
        juce::Path shape;
        shape.startNewSubPath(r.getX() + c, r.getY());
        shape.lineTo(r.getRight() - c, r.getY());
        shape.lineTo(r.getRight(), r.getY() + c);
        shape.lineTo(r.getRight(), r.getBottom() - c);
        shape.lineTo(r.getRight() - c, r.getBottom());
        shape.lineTo(r.getX() + c, r.getBottom());
        shape.lineTo(r.getX(), r.getBottom() - c);
        shape.lineTo(r.getX(), r.getY() + c);
        shape.closeSubPath();

        g.setGradientFill(juce::ColourGradient(Neon::panelHi, r.getX(), r.getY(),
                                               Neon::panel, r.getX(), r.getBottom(), false));
        g.fillPath(shape);
        g.setColour(accent.withAlpha(0.45f));
        g.strokePath(shape, juce::PathStrokeType(1.0f));

        drawIcon(g, { r.getX() + 18.0f, r.getY() + 14.0f, 14.0f, 14.0f }, icon, accent);

        g.setColour(accent);
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.drawText(name, juce::Rectangle<int>((int) (r.getX() + 40.0f), (int) (r.getY() + 12.0f),
                                              (int) (r.getWidth() - 50.0f), 18),
                   juce::Justification::centredLeft, false);

        g.setGradientFill(juce::ColourGradient(accent.withAlpha(0.5f), r.getX() + 18.0f, r.getY() + 38.0f,
                                               accent.withAlpha(0.0f), r.getRight() - 18.0f, r.getY() + 38.0f, false));
        g.fillRect(r.getX() + 18.0f, r.getY() + 38.0f, r.getWidth() - 36.0f, 1.0f);
    }

    void drawMeter(juce::Graphics& g, juce::Rectangle<int> r, float level)
    {
        g.setColour(Neon::grid);
        g.fillRoundedRectangle(r.toFloat(), 2.0f);
        const float fillW = (float) r.getWidth() * juce::jlimit(0.0f, 1.0f, level);
        g.setColour(Neon::mint);
        g.fillRoundedRectangle(r.toFloat().withWidth(fillW), 2.0f);
    }
}

// ---------- Look and feel ----------

NeonLookAndFeel::NeonLookAndFeel()
{
    setColour(juce::ComboBox::backgroundColourId, Neon::panel);
    setColour(juce::ComboBox::outlineColourId, Neon::grid);
    setColour(juce::ComboBox::textColourId, Neon::text);
    setColour(juce::ComboBox::arrowColourId, Neon::sky);
    setColour(juce::PopupMenu::backgroundColourId, Neon::panel);
    setColour(juce::PopupMenu::textColourId, Neon::text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, Neon::violet.withAlpha(0.5f));
    setColour(juce::Slider::textBoxTextColourId, Neon::text);
    setColour(juce::TextButton::buttonColourId, Neon::panel);
    setColour(juce::TextButton::buttonOnColourId, Neon::violet.withAlpha(0.4f));
    setColour(juce::TextButton::textColourOffId, Neon::muted);
    setColour(juce::TextButton::textColourOnId, Neon::text);
}

void NeonLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
                                   int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float>(0.0f, 0.0f, (float) width, (float) height);
    g.setColour(Neon::panel);
    g.fillRoundedRectangle(r, 3.0f);
    g.setColour(Neon::violet.withAlpha(0.35f));
    g.drawRoundedRectangle(r.reduced(0.5f), 3.0f, 1.0f);

    juce::Path arrow;
    arrow.startNewSubPath(width - 14.0f, height * 0.42f);
    arrow.lineTo(width - 10.0f, height * 0.62f);
    arrow.lineTo(width - 6.0f, height * 0.42f);
    g.setColour(Neon::sky);
    g.strokePath(arrow, juce::PathStrokeType(1.2f));
}

void NeonLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float startAngle, float endAngle,
                                       juce::Slider& slider)
{
    auto area = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height).reduced(8.0f);
    const float radius = juce::jmin(area.getWidth(), area.getHeight()) * 0.5f;
    const float cx = area.getCentreX();
    const float cy = area.getCentreY();
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const float trackR = radius - 5.0f;

    for (int i = 0; i <= 10; ++i)
    {
        const float t = (float) i / 10.0f;
        const float a = startAngle + t * (endAngle - startAngle);
        const float r1 = radius + 1.0f;
        const float r2 = radius + (i % 5 == 0 ? 6.0f : 3.5f);
        g.setColour(t <= sliderPos + 0.001f ? Neon::violet : Neon::grid);
        g.drawLine(cx + r1 * std::sin(a), cy - r1 * std::cos(a),
                   cx + r2 * std::sin(a), cy - r2 * std::cos(a), 1.5f);
    }

    juce::Path track;
    track.addCentredArc(cx, cy, trackR, trackR, 0.0f, startAngle, endAngle, true);
    g.setColour(Neon::grid);
    g.strokePath(track, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (sliderPos > 0.001f)
    {
        juce::Path value;
        value.addCentredArc(cx, cy, trackR, trackR, 0.0f, startAngle, angle, true);
        g.setGradientFill(juce::ColourGradient(Neon::violet.withAlpha(0.25f), cx - trackR, cy,
                                               Neon::sky.withAlpha(0.25f), cx + trackR, cy, false));
        g.strokePath(value, juce::PathStrokeType(9.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setGradientFill(juce::ColourGradient(Neon::violet, cx - trackR, cy,
                                               Neon::sky, cx + trackR, cy, false));
        g.strokePath(value, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const float bodyR = trackR - 8.0f;
    g.setGradientFill(juce::ColourGradient(Neon::panelHi, cx, cy - bodyR, Neon::bg, cx, cy + bodyR, false));
    g.fillEllipse(cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour(Neon::grid);
    g.drawEllipse(cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

    if (slider.isMouseOverOrDragging())
    {
        g.setColour(Neon::text);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText(slider.getTextFromValue(slider.getValue()),
                   juce::Rectangle<int>((int) (cx - bodyR), (int) (cy - 8.0f), (int) (bodyR * 2.0f), 16),
                   juce::Justification::centred, false);
    }
    else
    {
        const float innerX = cx + bodyR * 0.35f * std::sin(angle);
        const float innerY = cy - bodyR * 0.35f * std::cos(angle);
        const float tipX   = cx + bodyR * 0.8f * std::sin(angle);
        const float tipY   = cy - bodyR * 0.8f * std::cos(angle);
        g.setColour(Neon::text);
        g.drawLine(juce::Line<float>(innerX, innerY, tipX, tipY), 2.0f);
        g.setColour(Neon::mint);
        g.fillEllipse(tipX - 2.5f, tipY - 2.5f, 5.0f, 5.0f);
    }
}

// ---------- Editor ----------

VoxStudioEditor::VoxStudioEditor(VoxStudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1040, 680);
    setResizable(false, false);
    setLookAndFeel(&lnf);

    title.setText("VOXSTUDIO", juce::dontSendNotification);
    title.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    title.setColour(juce::Label::textColourId, Neon::text);
    addAndMakeVisible(title);

    subtitle.setText("1.0   /   GATE   COMP   EQ   SFX   REVERB   OUTPUT", juce::dontSendNotification);
    subtitle.setFont(juce::FontOptions(11.0f));
    subtitle.setColour(juce::Label::textColourId, Neon::muted);
    addAndMakeVisible(subtitle);

    producer.setText("TIEM", juce::dontSendNotification);
    producer.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    producer.setColour(juce::Label::textColourId, Neon::muted);
    producer.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(producer);

    presetBox.addItemList(VoxStudioProcessor::reverbPresetNames, 1);
    addAndMakeVisible(presetBox);
    presetAttachment = std::make_unique<juce::ComboBoxParameterAttachment>(
        *processor.apvts.getParameter("rev_preset"), presetBox, nullptr);

    bypassButton.setClickingTogglesState(true);
    bypassButton.onClick = [this] { bypassed = bypassButton.getToggleState(); };
    addAndMakeVisible(bypassButton);

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
        setupKnob(eqFreq[b], n + "freq", "FREQ");
        setupKnob(eqGain[b], n + "gain", "GAIN");
        setupKnob(eqQ[b],    n + "q",    "Q");
    }

    setupKnob(sfxDrive,     "sfx_drive",      "DRIVE");
    setupKnob(sfxDelay,     "sfx_delay",      "DELAY");
    setupKnob(sfxDelayTime, "sfx_delay_time", "TIME");

    setupKnob(revRoom, "rev_room", "ROOM");
    setupKnob(revDamp, "rev_damp", "DAMP");
    setupKnob(revMix,  "rev_mix",  "MIX");

    setupKnob(outGain, "out_gain", "OUT");

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
    k.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
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

// ----- EQ interactivo -----

juce::Rectangle<float> VoxStudioEditor::eqArea() const
{
    const int w = getWidth();
    const int h = getHeight();
    return panelRect(1, w, h).toFloat().reduced(14.0f, 0.0f).withTrimmedTop(50.0f).withHeight(220.0f);
}

juce::Point<float> VoxStudioEditor::nodePosition(int band) const
{
    const auto area = eqArea();
    const float f = processor.apvts.getRawParameterValue("eq" + juce::String(band + 1) + "_freq")->load();
    const float g = processor.apvts.getRawParameterValue("eq" + juce::String(band + 1) + "_gain")->load();

    const float x = area.getX() + area.getWidth() * std::log(f / 20.0f) / std::log(1000.0f);
    const float y = juce::jmap(juce::jlimit(-12.0f, 12.0f, g), -12.0f, 12.0f,
                               area.getBottom(), area.getY());
    return { x, y };
}

void VoxStudioEditor::setBandFromPoint(int band, juce::Point<float> p)
{
    const auto area = eqArea();
    const float t = juce::jlimit(0.0f, 1.0f, (p.x - area.getX()) / area.getWidth());
    const float f = 20.0f * std::pow(1000.0f, t);
    const float g = juce::jmap(juce::jlimit(area.getY(), area.getBottom(), p.y),
                               area.getBottom(), area.getY(), -12.0f, 12.0f);

    if (auto* pf = processor.apvts.getParameter("eq" + juce::String(band + 1) + "_freq"))
        pf->setValueNotifyingHost(pf->convertTo0to1(f));
    if (auto* pg = processor.apvts.getParameter("eq" + juce::String(band + 1) + "_gain"))
        pg->setValueNotifyingHost(pg->convertTo0to1(g));
}

int VoxStudioEditor::nodeAt(juce::Point<float> p) const
{
    for (int b = 0; b < VoxStudioProcessor::numEqBands; ++b)
        if (nodePosition(b).getDistanceFrom(p) < 12.0f)
            return b;
    return -1;
}

void VoxStudioEditor::mouseDown(const juce::MouseEvent& e)
{
    draggingBand = nodeAt(e.position);
}

void VoxStudioEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (draggingBand >= 0)
        setBandFromPoint(draggingBand, e.position);
}

void VoxStudioEditor::mouseUp(const juce::MouseEvent&)
{
    draggingBand = -1;
}

void VoxStudioEditor::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int band = nodeAt(e.position);
    if (band < 0) return;

    if (auto* pq = processor.apvts.getParameter("eq" + juce::String(band + 1) + "_q"))
    {
        const float q = pq->convertFrom0to1(pq->getValue());
        pq->setValueNotifyingHost(pq->convertTo0to1(juce::jlimit(0.1f, 10.0f, q + w.deltaY * 2.0f)));
    }
}

// ----- Pintado -----

void VoxStudioEditor::paint(juce::Graphics& g)
{
    const int w = getWidth();
    const int h = getHeight();

    g.fillAll(Neon::bg);

    g.setColour(Neon::grid.withAlpha(0.45f));
    for (int x = 12; x < w; x += 24)
        for (int y = 12; y < h; y += 24)
            g.fillRect(x, y, 1, 1);

    g.setGradientFill(juce::ColourGradient(Neon::violet.withAlpha(0.18f), 0.0f, 0.0f,
                                           Neon::sky.withAlpha(0.03f), (float) w, 0.0f, false));
    g.fillRect(0.0f, 0.0f, (float) w, (float) headerH);
    g.setGradientFill(juce::ColourGradient(Neon::violet, 0.0f, (float) headerH - 1.0f,
                                           Neon::sky, (float) w, (float) headerH - 1.0f, false));
    g.fillRect(0.0f, (float) headerH - 1.0f, (float) w, 1.0f);

    const juce::Colour accents[] = { Neon::sky, Neon::mint, Neon::violet, Neon::amber };
    const juce::String names[]   = { "GATE / COMP", "EQ  4 BAND", "SFX / REVERB", "OUTPUT" };
    const int icons[]            = { 0, 1, 2, 3 };
    for (int i = 0; i < 4; ++i)
        drawPanel(g, panelRect(i, w, h).toFloat(), names[i], accents[i], icons[i]);

    const auto area = eqArea();
    g.setColour(Neon::bg.withAlpha(0.75f));
    g.fillRoundedRectangle(area, 8.0f);
    g.setColour(Neon::grid);
    g.drawRoundedRectangle(area, 8.0f, 1.0f);

    g.setFont(juce::FontOptions(8.0f));
    const float marks[]  = { 50.0f, 100.0f, 500.0f, 1000.0f, 5000.0f, 10000.0f };
    const char* labels[] = { "50", "100", "500", "1k", "5k", "10k" };
    for (int i = 0; i < 6; ++i)
    {
        const float x = area.getX() + area.getWidth() * std::log(marks[i] / 20.0f) / std::log(1000.0f);
        g.setColour(Neon::grid);
        g.drawVerticalLine((int) x, area.getY(), area.getBottom());
        g.setColour(Neon::muted);
        g.drawText(labels[i], juce::Rectangle<float>(x - 20.0f, area.getBottom() - 14.0f, 40.0f, 12.0f),
                   juce::Justification::centred, false);
    }
    g.setColour(Neon::grid);
    g.drawHorizontalLine((int) area.getCentreY(), area.getX(), area.getRight());

    juce::Path curve;
    const int points = 140;
    for (int i = 0; i <= points; ++i)
    {
        const float t = (float) i / (float) points;
        const float freq = 20.0f * std::pow(1000.0f, t);
        float db = 0.0f;
        for (int b = 0; b < VoxStudioProcessor::numEqBands; ++b)
        {
            const juce::String n = "eq" + juce::String(b + 1) + "_";
            const float f0 = processor.apvts.getRawParameterValue(n + "freq")->load();
            const float g0 = processor.apvts.getRawParameterValue(n + "gain")->load();
            const float q  = processor.apvts.getRawParameterValue(n + "q")->load();
            const float x = std::log2(freq / f0);
            db += g0 * std::exp(-x * x * q * 2.0f);
        }
        const float y = juce::jmap(juce::jlimit(-12.0f, 12.0f, db), -12.0f, 12.0f,
                                   area.getBottom() - 14.0f, area.getY() + 4.0f);
        const float px = area.getX() + t * area.getWidth();
        if (i == 0) curve.startNewSubPath(px, y);
        else        curve.lineTo(px, y);
    }
    g.setGradientFill(juce::ColourGradient(Neon::violet, area.getX(), 0.0f,
                                           Neon::sky, area.getRight(), 0.0f, false));
    g.strokePath(curve, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    for (int b = 0; b < VoxStudioProcessor::numEqBands; ++b)
    {
        const auto p = nodePosition(b);
        const juce::Colour c = (b % 2 == 0) ? Neon::mint : Neon::violet;
        g.setColour(c.withAlpha(0.25f));
        g.fillEllipse(p.x - 9.0f, p.y - 9.0f, 18.0f, 18.0f);
        g.setColour(c);
        g.fillEllipse(p.x - 5.0f, p.y - 5.0f, 10.0f, 10.0f);
    }

    // Franja inferior: INPUT a la izquierda, OUTPUT a la derecha
    const int footerY = h - footerH + 10;
    const juce::Rectangle<int> inMeter (pad, footerY, 200, 6);
    const juce::Rectangle<int> outMeter(w - pad - 200, footerY, 200, 6);

    g.setColour(Neon::muted);
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    g.drawText("INPUT",  juce::Rectangle<int>(pad, footerY - 14, 60, 12), juce::Justification::left, false);
    g.drawText("OUTPUT", juce::Rectangle<int>(w - pad - 60, footerY - 14, 60, 12), juce::Justification::right, false);

    drawMeter(g, inMeter,  inLevel);
    drawMeter(g, outMeter, outLevel);
}

void VoxStudioEditor::resized()
{
    const int w = getWidth();
    const int h = getHeight();

    title.setBounds(24, 14, 320, 30);
    subtitle.setBounds(24, 46, 700, 18);
    producer.setBounds(w - 120, 14, 96, 14);
    bypassButton.setBounds(w - 120, 44, 96, 22);

    auto p0 = innerRect(panelRect(0, w, h));
    auto gateRow = p0.removeFromTop(120);
    const auto gateCells = cellsOf(gateRow, 2, 1);
    placeKnob(gateThreshold, gateCells[0]);
    placeKnob(gateRelease,   gateCells[1]);

    const auto compCells = cellsOf(p0, 2, 3);
    placeKnob(compThreshold, compCells[0]);
    placeKnob(compRatio,     compCells[1]);
    placeKnob(compAttack,    compCells[2]);
    placeKnob(compRelease,   compCells[3]);
    placeKnob(compMakeup,    compCells[4]);

    auto p1 = innerRect(panelRect(1, w, h));
    p1.removeFromTop(270);
    const auto bandCols = cellsOf(p1, 4, 1);
    for (int b = 0; b < VoxStudioProcessor::numEqBands; ++b)
    {
        const auto rows = cellsOf(bandCols[(size_t) b], 1, 3);
        placeKnob(eqFreq[b], rows[0]);
        placeKnob(eqGain[b], rows[1]);
        placeKnob(eqQ[b],    rows[2]);
    }

    auto p2 = innerRect(panelRect(2, w, h));
    const int rowH = (p2.getHeight() - 40) / 2;
    auto sfxRow    = p2.removeFromTop(rowH);
    auto presetRow = p2.removeFromTop(40);
    const auto sfxCells = cellsOf(sfxRow, 3, 1);
    placeKnob(sfxDrive,     sfxCells[0]);
    placeKnob(sfxDelay,     sfxCells[1]);
    placeKnob(sfxDelayTime, sfxCells[2]);
    presetBox.setBounds(presetRow.reduced(4, 10));
    const auto revCells = cellsOf(p2, 3, 1);
    placeKnob(revRoom, revCells[0]);
    placeKnob(revDamp, revCells[1]);
    placeKnob(revMix,  revCells[2]);

    auto p3 = innerRect(panelRect(3, w, h));
    const int knob = juce::jmin(p3.getWidth(), 90);
    placeKnob(outGain, p3.withSizeKeepingCentre(knob, knob + 16));
}
