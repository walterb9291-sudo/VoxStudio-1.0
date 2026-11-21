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

    struct EqLayout
    {
        juce::Rectangle<int> curve, names, bands;
    };

    juce::Rectangle<int> panelRect(int i, int w, int h)
    {
        const int colW = (w - pad * 5) / 4;
        return { pad + i * (colW + pad), panelTop, colW, h - panelTop - pad };
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

    EqLayout eqLayout(juce::Rectangle<int> panel)
    {
        auto inner = innerRect(panel);
        EqLayout l;
        l.curve = inner.removeFromTop(150);
        inner.removeFromTop(8);
        l.names = inner.removeFromTop(18);
        l.bands = inner;
        return l;
    }

    juce::Rectangle<int> meterArea(juce::Rectangle<int> panel)
    {
        return innerRect(panel).removeFromBottom(150);
    }

    void placeKnob(VoxStudioEditor::Knob& k, juce::Rectangle<int> cell)
    {
        k.label.setBounds(cell.removeFromBottom(16));
        k.slider.setBounds(cell.reduced(2));
    }

    void drawPanel(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& name, juce::Colour accent)
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

        // Acentos en las esquinas
        g.setColour(accent);
        g.fillRect(r.getX() + 22.0f, r.getY(), 28.0f, 2.0f);
        g.fillRect(r.getRight() - 50.0f, r.getBottom() - 2.0f, 28.0f, 2.0f);

        // Marcador y título
        g.fillRoundedRectangle(r.getX() + 18.0f, r.getY() + 14.0f, 6.0f, 6.0f, 1.5f);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText(name, juce::Rectangle<int>((int) (r.getX() + 30.0f), (int) (r.getY() + 10.0f),
                                              (int) (r.getWidth() - 40.0f), 18),
                   juce::Justification::centredLeft, false);

        // Divisor que se desvanece
        g.setGradientFill(juce::ColourGradient(accent.withAlpha(0.6f), r.getX() + 18.0f, r.getY() + 38.0f,
                                               accent.withAlpha(0.0f), r.getRight() - 18.0f, r.getY() + 38.0f, false));
        g.fillRect(r.getX() + 18.0f, r.getY() + 38.0f, r.getWidth() - 36.0f, 1.0f);
    }

    void drawMeter(juce::Graphics& g, juce::Rectangle<int> r, float level, juce::Colour c)
    {
        g.setColour(Neon::grid);
        g.fillRoundedRectangle(r.toFloat(), 3.0f);

        const float fillH = (float) r.getHeight() * juce::jlimit(0.0f, 1.0f, level);
        auto filled = r.toFloat().removeFromBottom(fillH);
        g.setColour(c);
        g.fillRoundedRectangle(filled, 3.0f);

        // Segmentos
        g.setColour(Neon::panel);
        for (float y = (float) r.getY() + 5.0f; y < (float) r.getBottom(); y += 6.0f)
            g.fillRect((float) r.getX(), y, (float) r.getWidth(), 1.5f);
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
                                       juce::Slider& slider)
{
    auto area = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height).reduced(8.0f);
    const float radius = juce::jmin(area.getWidth(), area.getHeight()) * 0.5f;
    const float cx = area.getCentreX();
    const float cy = area.getCentreY();
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const float trackR = radius - 5.0f;

    // Marcas de escala
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

    // Pista
    juce::Path track;
    track.addCentredArc(cx, cy, trackR, trackR, 0.0f, startAngle, endAngle, true);
    g.setColour(Neon::grid);
    g.strokePath(track, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Arco de valor con brillo
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

    // Cuerpo de la perilla
    const float bodyR = trackR - 8.0f;
    g.setGradientFill(juce::ColourGradient(Neon::panelHi, cx, cy - bodyR, Neon::bg, cx, cy + bodyR, false));
    g.fillEllipse(cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour(Neon::grid);
    g.drawEllipse(cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

    if (slider.isMouseOverOrDragging())
    {
        // Muestra el valor solo al pasar el mouse o girar
        g.setColour(Neon::text);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText(slider.getTextFromValue(slider.getValue()),
                   juce::Rectangle<int>((int) (cx - bodyR), (int) (cy - 8.0f), (int) (bodyR * 2.0f), 16),
                   juce::Justification::centred, false);
    }
    else
    {
        // Indicador con punto en la punta
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
    setSize(1040, 640);
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

    presetBox.addItemList(VoxStudioProcessor::reverbPresetNames, 1);
    addAndMakeVisible(presetBox);
    presetAttachment = std::make_unique<juce::ComboBoxParameterAttachment>(
        *processor.apvts.getParameter("rev_preset"), presetBox, nullptr);

    setupKnob(gateThreshold, "gate_threshold", "GATE THR");
    setupKnob(gateRelease,   "gate_release",   "GATE REL");

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
    setupKnob(sfxDelay,     "sfx_delay",      "D MIX");
    setupKnob(sfxDelayTime, "sfx_delay_time", "D TIME");

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

void VoxStudioEditor::drawEqCurve(juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour(Neon::bg.withAlpha(0.75f));
    g.fillRoundedRectangle(area, 8.0f);
    g.setColour(Neon::grid);
    g.drawRoundedRectangle(area, 8.0f, 1.0f);

    // Línea de 0 dB y marcas de frecuencia
    const float zeroY = area.getCentreY();
    g.drawHorizontalLine((int) zeroY, area.getX(), area.getRight());

    const float marks[] = { 100.0f, 1000.0f, 10000.0f };
    for (float f : marks)
    {
        const float x = area.getX() + area.getWidth() * std::log(f / 20.0f) / std::log(1000.0f);
        g.drawVerticalLine((int) x, area.getY() + 4.0f, area.getBottom() - 4.0f);
    }

    // Curva (aproximación visual de las bandas)
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

        const float y = juce::jmap(juce::jlimit(-14.0f, 14.0f, db), -14.0f, 14.0f,
                                   area.getBottom() - 6.0f, area.getY() + 6.0f);
        const float px = area.getX() + t * area.getWidth();
        if (i == 0) curve.startNewSubPath(px, y);
        else        curve.lineTo(px, y);
    }

    juce::Path fill(curve);
    fill.lineTo(area.getRight(), zeroY);
    fill.lineTo(area.getX(), zeroY);
    fill.closeSubPath();

    g.setGradientFill(juce::ColourGradient(Neon::violet.withAlpha(0.30f), area.getX(), area.getY(),
                                           Neon::violet.withAlpha(0.0f), area.getX(), zeroY, false));
    g.fillPath(fill);

    g.setGradientFill(juce::ColourGradient(Neon::violet, area.getX(), 0.0f,
                                           Neon::sky, area.getRight(), 0.0f, false));
    g.strokePath(curve, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void VoxStudioEditor::paint(juce::Graphics& g)
{
    const int w = getWidth();
    const int h = getHeight();

    g.fillAll(Neon::bg);

    // Cuadrícula de puntos de fondo
    g.setColour(Neon::grid.withAlpha(0.45f));
    for (int x = 12; x < w; x += 24)
        for (int y = 12; y < h; y += 24)
            g.fillRect(x, y, 1, 1);

    // Cabecera
    g.setGradientFill(juce::ColourGradient(Neon::violet.withAlpha(0.20f), 0.0f, 0.0f,
                                           Neon::sky.withAlpha(0.03f), (float) w, 0.0f, false));
    g.fillRect(0.0f, 0.0f, (float) w, (float) headerH);

    g.setGradientFill(juce::ColourGradient(Neon::violet, 0.0f, (float) headerH - 1.0f,
                                           Neon::sky, (float) w, (float) headerH - 1.0f, false));
    g.fillRect(0.0f, (float) headerH - 1.0f, (float) w, 1.0f);

    // Barras decorativas en la cabecera
    for (int i = 0; i < 6; ++i)
    {
        const float bh = 8.0f + (float) ((i * 7) % 5) * 4.0f;
        g.setColour(i % 2 == 0 ? Neon::mint : Neon::sky);
        g.fillRoundedRectangle((float) w - 40.0f - (float) i * 12.0f, 34.0f - bh * 0.5f, 6.0f, bh, 2.0f);
    }

    // Paneles
    const juce::Colour accents[] = { Neon::sky, Neon::mint, Neon::violet, Neon::amber };
    const juce::String names[]   = { "GATE / COMP", "EQ  //  4 BAND", "SFX / REVERB", "OUTPUT" };
    for (int i = 0; i < 4; ++i)
        drawPanel(g, panelRect(i, w, h).toFloat(), names[i], accents[i]);

    // Curva y nombres de banda del EQ
    const auto eq = eqLayout(panelRect(1, w, h));
    drawEqCurve(g, eq.curve.toFloat());

    const juce::String bandNames[] = { "LOW", "LO-MID", "HI-MID", "HIGH" };
    const auto nameCells = cellsOf(eq.names, 4, 1);
    g.setColour(Neon::muted);
    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    for (int b = 0; b < 4; ++b)
        g.drawText(bandNames[b], nameCells[(size_t) b], juce::Justification::centred, false);

    // Medidores de entrada y salida
    const auto meter = meterArea(panelRect(3, w, h));
    const auto inBar  = meter.withSizeKeepingCentre(14, meter.getHeight()).translated(-12, 0);
    const auto outBar = meter.withSizeKeepingCentre(14, meter.getHeight()).translated(12, 0);
    drawMeter(g, inBar,  inLevel,  Neon::mint);
    drawMeter(g, outBar, outLevel, Neon::amber);
}

void VoxStudioEditor::resized()
{
    const int w = getWidth();
    const int h = getHeight();

    title.setBounds(24, 14, 320, 30);
    subtitle.setBounds(24, 46, 700, 18);

    // 1. GATE / COMP
    auto p0 = innerRect(panelRect(0, w, h));
    auto gateRow = p0.removeFromTop(130);
    const auto gateCells = cellsOf(gateRow, 2, 1);
    placeKnob(gateThreshold, gateCells[0]);
    placeKnob(gateRelease,   gateCells[1]);

    const auto compCells = cellsOf(p0, 2, 3);
    placeKnob(compThreshold, compCells[0]);
    placeKnob(compRatio,     compCells[1]);
    placeKnob(compAttack,    compCells[2]);
    placeKnob(compRelease,   compCells[3]);
    placeKnob(compMakeup,    compCells[4]);

    // 2. EQ
    const auto eq = eqLayout(panelRect(1, w, h));
    const auto bandCols = cellsOf(eq.bands, 4, 1);
    for (int b = 0; b < VoxStudioProcessor::numEqBands; ++b)
    {
        const auto rows = cellsOf(bandCols[(size_t) b], 1, 3);
        placeKnob(eqFreq[b], rows[0]);
        placeKnob(eqGain[b], rows[1]);
        placeKnob(eqQ[b],    rows[2]);
    }

    // 3. SFX / REVERB
    auto p2 = innerRect(panelRect(2, w, h));
    const int rowH = (p2.getHeight() - 44) / 2;
    auto sfxRow    = p2.removeFromTop(rowH);
    auto presetRow = p2.removeFromTop(44);
    auto revRow    = p2;

    const auto sfxCells = cellsOf(sfxRow, 3, 1);
    placeKnob(sfxDrive,     sfxCells[0]);
    placeKnob(sfxDelay,     sfxCells[1]);
    placeKnob(sfxDelayTime, sfxCells[2]);

    presetBox.setBounds(presetRow.reduced(4, 8));

    const auto revCells = cellsOf(revRow, 3, 1);
    placeKnob(revRoom, revCells[0]);
    placeKnob(revDamp, revCells[1]);
    placeKnob(revMix,  revCells[2]);

    // 4. OUTPUT (la zona de medidores queda libre abajo)
    auto p3 = innerRect(panelRect(3, w, h));
    p3.removeFromBottom(150);
    const int knobSize = juce::jmin(p3.getWidth(), p3.getHeight());
    placeKnob(outGain, p3.withSizeKeepingCentre(p3.getWidth(), knobSize));
}
