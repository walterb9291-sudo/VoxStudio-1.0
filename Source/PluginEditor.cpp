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
    constexpr int footerH  = 72;

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

    // Perilla con etiqueta abajo, tamaño fijo y centrada en su celda
    void placeKnob(VoxStudioEditor::Knob& k, juce::Rectangle<int> cell, int size)
    {
        k.label.setBounds(cell.removeFromBottom(12));
        const int s = juce::jmin(size, juce::jmin(cell.getWidth(), cell.getHeight()));
        k.slider.setBounds(cell.getCentreX() - s / 2, cell.getCentreY() - s / 2, s, s);
    }

    juce::Rectangle<int> footerInMeter(int h)         { return { pad + 140, h - footerH + 33, 250, 6 }; }
    juce::Rectangle<int> footerOutMeter(int w, int h) { return { w - 412, h - footerH + 33, 250, 6 }; }

    void drawIcon(juce::Graphics& g, juce::Rectangle<float> r, int kind, juce::Colour c)
    {
        g.setColour(c);
        switch (kind)
        {
            case 0:
            {
                juce::Path p;
                p.startNewSubPath(r.getX(), r.getCentreY());
                p.lineTo(r.getX() + r.getWidth() * 0.3f, r.getCentreY());
                p.lineTo(r.getX() + r.getWidth() * 0.45f, r.getY());
                p.lineTo(r.getX() + r.getWidth() * 0.6f, r.getBottom());
                p.lineTo(r.getRight(), r.getCentreY());
                g.strokePath(p, juce::PathStrokeType(1.6f));
                break;
            }
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
            case 4:
                g.fillEllipse(r.getX(), r.getCentreY() - 2.0f, 4.0f, 4.0f);
                g.fillEllipse(r.getCentreX() - 2.0f, r.getCentreY() - 2.0f, 3.0f, 3.0f);
                g.fillEllipse(r.getRight() - 2.0f, r.getCentreY() - 2.0f, 2.0f, 2.0f);
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

    void drawVuMeter(juce::Graphics& g, juce::Rectangle<float> r, float grDb)
    {
        g.setColour(Neon::bg);
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour(Neon::grid);
        g.drawRoundedRectangle(r.reduced(0.5f), 6.0f, 1.0f);

        const float cx = r.getCentreX();
        const float cy = r.getBottom() - 6.0f;
        const float radius = r.getHeight() * 0.9f;
        const float startA = -0.9f;
        const float endA   =  0.9f;

        juce::Path scale;
        scale.addCentredArc(cx, cy, radius, radius, 0.0f, startA, endA, true);
        g.setColour(Neon::muted);
        g.strokePath(scale, juce::PathStrokeType(1.0f));

        const float marksDb[] = { 0.0f, -3.0f, -6.0f, -10.0f, -20.0f };
        g.setFont(juce::FontOptions(7.5f, juce::Font::bold));
        for (float db : marksDb)
        {
            const float t = juce::jmap(-db, 0.0f, 20.0f, 0.0f, 1.0f);
            const float a = startA + t * (endA - startA);

            g.drawLine(cx + (radius - 2.0f) * std::sin(a), cy - (radius - 2.0f) * std::cos(a),
                       cx + (radius - 7.0f) * std::sin(a), cy - (radius - 7.0f) * std::cos(a), 1.0f);

            const float lx = cx + (radius - 17.0f) * std::sin(a);
            const float ly = cy - (radius - 17.0f) * std::cos(a);
            g.drawText(juce::String((int) db), juce::Rectangle<float>(lx - 10.0f, ly - 6.0f, 20.0f, 12.0f),
                       juce::Justification::centred, false);
        }

        const float t = juce::jlimit(0.0f, 1.0f, grDb / 20.0f);
        const float a = startA + t * (endA - startA);
        g.setColour(Neon::mint);
        g.drawLine(cx, cy, cx + (radius - 6.0f) * std::sin(a), cy - (radius - 6.0f) * std::cos(a), 1.6f);
        g.fillEllipse(cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);

        g.setColour(Neon::muted);
        g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
        g.drawText("GR dB", juce::Rectangle<float>(r.getRight() - 48.0f, r.getY() + 3.0f, 42.0f, 12.0f),
                   juce::Justification::right, false);
    }
}

// ---------- Look and feel ----------

NeonLookAndFeel::NeonLookAndFeel()
{
    setColour(juce::ComboBox::backgroundColourId, Neon::panel);
    setColour(juce::ComboBox::outlineColourId, Neon::grid);
    setColour(juce::ComboBox::textColourId, Neon::mint);
    setColour(juce::ComboBox::arrowColourId, Neon::violet);
    setColour(juce::PopupMenu::backgroundColourId, Neon::panel);
    setColour(juce::PopupMenu::textColourId, Neon::text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, Neon::violet.withAlpha(0.35f));
    setColour(juce::Slider::textBoxTextColourId, Neon::text);
    setColour(juce::TextButton::buttonColourId, Neon::panel);
    setColour(juce::TextButton::buttonOnColourId, Neon::sky.withAlpha(0.35f));
    setColour(juce::TextButton::textColourOffId, Neon::muted);
    setColour(juce::TextButton::textColourOnId, Neon::text);
}

juce::Font NeonLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return juce::Font(juce::FontOptions("Segoe UI", 11.0f, juce::Font::plain));
}

void NeonLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
                                   int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float>(0.0f, 0.0f, (float) width, (float) height);
    g.setColour(Neon::panel.withAlpha(0.6f));
    g.fillRoundedRectangle(r, 3.0f);

    g.setGradientFill(juce::ColourGradient(Neon::violet.withAlpha(0.0f), r.getX() + 6.0f, r.getBottom(),
                                           Neon::violet, r.getRight() - 6.0f, r.getBottom(), false));
    g.fillRect(r.getX() + 6.0f, r.getBottom() - 1.0f, r.getWidth() - 12.0f, 1.0f);

    juce::Path arrow;
    arrow.startNewSubPath(width - 14.0f, height * 0.44f);
    arrow.lineTo(width - 10.5f, height * 0.60f);
    arrow.lineTo(width - 7.0f, height * 0.44f);
    g.setColour(Neon::sky.withAlpha(0.8f));
    g.strokePath(arrow, juce::PathStrokeType(1.0f));
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

    if (slider.getComponentID() == "readout")
    {
        const juce::Rectangle<int> box((int) (cx - bodyR), (int) (cy - 14.0f), (int) (bodyR * 2.0f), 18);
        g.setColour(Neon::text);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText(juce::String(slider.getValue(), 1), box, juce::Justification::centred, false);

        g.setColour(Neon::muted);
        g.setFont(juce::FontOptions(8.0f, juce::Font::bold));
        g.drawText("dB", juce::Rectangle<int>(box.getX(), box.getBottom(), box.getWidth(), 12),
                   juce::Justification::centred, false);
        return;
    }

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

    title.setText("VoxStudio", juce::dontSendNotification);
    title.setFont(juce::FontOptions("Segoe UI", 22.0f, juce::Font::bold | juce::Font::italic));
    title.setColour(juce::Label::textColourId, Neon::mint);
    addAndMakeVisible(title);

    subtitle.setText("1.0   /   COMP   EQ   SFX   REVERB   DELAY", juce::dontSendNotification);
    subtitle.setFont(juce::FontOptions(11.0f));
    subtitle.setColour(juce::Label::textColourId, Neon::muted);
    addAndMakeVisible(subtitle);

    producer.setText("TIEM", juce::dontSendNotification);
    producer.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    producer.setColour(juce::Label::textColourId, Neon::muted);
    producer.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(producer);

    // Selector de reverb en la cabecera, sobre la columna SFX
    presetBox.addItemList(VoxStudioProcessor::reverbPresetNames, 1);
    addAndMakeVisible(presetBox);
    presetAttachment = std::make_unique<juce::ComboBoxParameterAttachment>(
        *processor.apvts.getParameter("rev_preset"), presetBox, nullptr);

    static const char* divLabels[numDivButtons] = { "1/1", "1/2", "1/4", "1/8", "1/16", "1/4.", "1/8.", "1/4T" };
    for (int i = 0; i < numDivButtons; ++i)
    {
        divButtons[i].setButtonText(divLabels[i]);
        divButtons[i].setClickingTogglesState(false);
        divButtons[i].onClick = [this, i] { setDelayDivision(i); };
        addAndMakeVisible(divButtons[i]);
    }

    // BYPASS en la cabecera, sobre la columna DELAY
    bypassButton.setClickingTogglesState(true);
    addAndMakeVisible(bypassButton);
    bypassAttachment = std::make_unique<juce::ButtonParameterAttachment>(
        *processor.apvts.getParameter("bypass"), bypassButton, nullptr);

    setupKnob(inGain,  "in_gain",  "INPUT");
    setupKnob(outGain, "out_gain", "OUTPUT");

    setupKnob(compThreshold, "comp_threshold", "THRESHOLD");
    compThreshold.slider.setComponentID("readout");
    setupKnob(compRatio,   "comp_ratio",   "RATIO");
    setupKnob(compAttack,  "comp_attack",  "ATTACK");
    setupKnob(compRelease, "comp_release", "RELEASE");
    setupKnob(compMakeup,  "comp_makeup",  "MAKEUP");

    for (int b = 0; b < VoxStudioProcessor::numEqBands; ++b)
    {
        const juce::String n = "eq" + juce::String(b + 1) + "_";
        setupKnob(eqFreq[b], n + "freq", "FREQ");
        setupKnob(eqGain[b], n + "gain", "GAIN");
        setupKnob(eqQ[b],    n + "q",    "Q");
        eqQ[b].slider.setVisible(false);
        eqQ[b].label.setVisible(false);
    }

    setupKnob(sfxDrive, "sfx_drive", "DRIVE");
    setupKnob(delayMix, "sfx_delay", "MIX");

    setupKnob(revRoom, "rev_room", "ROOM");
    setupKnob(revDamp, "rev_damp", "DAMP");
    setupKnob(revMix,  "rev_mix",  "MIX");

    startTimerHz(30);
}

VoxStudioEditor::~VoxStudioEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void VoxStudioEditor::setDelayDivision(int index)
{
    if (auto* p = processor.apvts.getParameter("sfx_delay_div"))
        p->setValueNotifyingHost(p->convertTo0to1((float) index));
}

void VoxStudioEditor::setupKnob(Knob& k, const juce::String& paramID, const juce::String& text)
{
    k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(k.slider);

    k.label.setText(text, juce::dontSendNotification);
    k.label.setFont(juce::FontOptions(8.5f, juce::Font::bold));
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
    grLevel  = juce::jmax(processor.getGainReduction() * 0.9f, grLevel * 0.85f);

    const int activeDiv = (int) processor.apvts.getRawParameterValue("sfx_delay_div")->load();
    for (int i = 0; i < numDivButtons; ++i)
        divButtons[i].setToggleState(i == activeDiv, juce::dontSendNotification);

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
    const int hit = nodeAt(e.position);
    if (hit >= 0)
    {
        draggingBand = hit;
        return;
    }

    if (eqArea().contains(e.position))
    {
        int best = 0;
        float bestAbs = 1.0e9f;
        for (int b = 0; b < VoxStudioProcessor::numEqBands; ++b)
        {
            const float g = std::abs(processor.apvts.getRawParameterValue("eq" + juce::String(b + 1) + "_gain")->load());
            if (g < bestAbs) { bestAbs = g; best = b; }
        }
        setBandFromPoint(best, e.position);
        draggingBand = best;
    }
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

void VoxStudioEditor::mouseDoubleClick(const juce::MouseEvent& e)
{
    const int band = nodeAt(e.position);
    if (band < 0) return;

    if (auto* pg = processor.apvts.getParameter("eq" + juce::String(band + 1) + "_gain"))
        pg->setValueNotifyingHost(pg->convertTo0to1(0.0f));
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
    const juce::String names[]   = { "COMPRESSOR", "EQ  4 BAND", "SFX / REVERB", "DELAY" };
    const int icons[]            = { 1, 1, 2, 4 };
    for (int i = 0; i < 4; ++i)
        drawPanel(g, panelRect(i, w, h).toFloat(), names[i], accents[i], icons[i]);

    {
        auto p0 = panelRect(0, w, h).toFloat();
        drawVuMeter(g, { p0.getX() + 14.0f, p0.getY() + 48.0f, p0.getWidth() - 28.0f, 60.0f }, grLevel);
    }

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
        const float gain = processor.apvts.getRawParameterValue("eq" + juce::String(b + 1) + "_gain")->load();
        const juce::Colour c = (b % 2 == 0) ? Neon::mint : Neon::violet;
        const float alpha = std::abs(gain) < 0.05f ? 0.35f : 1.0f;
        g.setColour(c.withAlpha(0.25f * alpha));
        g.fillEllipse(p.x - 9.0f, p.y - 9.0f, 18.0f, 18.0f);
        g.setColour(c.withAlpha(alpha));
        g.fillEllipse(p.x - 5.0f, p.y - 5.0f, 10.0f, 10.0f);
    }

    // Visual del delay: ecos decrecientes
    {
        auto d = innerRect(panelRect(3, w, h)).withTrimmedTop(230);
        const int taps = 6;
        const int tw = 14;
        const int gap = (d.getWidth() - taps * tw) / (taps - 1);
        for (int i = 0; i < taps; ++i)
        {
            const float ht = 60.0f * std::pow(0.7f, (float) i);
            const float a  = 1.0f - i * 0.14f;
            g.setColour(Neon::amber.withAlpha(a));
            g.fillRoundedRectangle((float) (d.getX() + i * (tw + gap)),
                                   (float) d.getBottom() - ht, (float) tw, ht, 3.0f);
        }
    }

    drawMeter(g, footerInMeter(h),     inLevel);
    drawMeter(g, footerOutMeter(w, h), outLevel);
}

void VoxStudioEditor::resized()
{
    const int w = getWidth();
    const int h = getHeight();

    // Cabecera: misma cuadrícula que los paneles
    title.setBounds(24, 14, 320, 30);
    subtitle.setBounds(24, 46, 500, 18);

    const auto c2 = panelRect(2, w, h);
    const auto c3 = panelRect(3, w, h);
    presetBox.setBounds(c2.getX(), 20, c2.getWidth(), 26);
    bypassButton.setBounds(c3.getX(), 20, c3.getWidth(), 26);
    producer.setBounds(c3.getX(), 50, c3.getWidth(), 14);

    // COMPRESSOR (sin cambios de tamaño)
    auto p0 = innerRect(panelRect(0, w, h));
    p0.removeFromTop(112);
    placeKnob(compThreshold, p0.removeFromTop(90).reduced(40, 0), 80);
    const auto compCells = cellsOf(p0, 2, 2);
    placeKnob(compRatio,   compCells[0], 40);
    placeKnob(compAttack,  compCells[1], 40);
    placeKnob(compRelease, compCells[2], 40);
    placeKnob(compMakeup,  compCells[3], 40);

    // EQ: perillas FREQ y GAIN más chicas, con espacio entre bandas
    auto p1 = innerRect(panelRect(1, w, h));
    p1.removeFromTop(270);
    const auto bandCols = cellsOf(p1, 4, 1);
    for (int b = 0; b < VoxStudioProcessor::numEqBands; ++b)
    {
        const auto rows = cellsOf(bandCols[(size_t) b], 1, 2);
        placeKnob(eqFreq[b], rows[0], 34);
        placeKnob(eqGain[b], rows[1], 34);
    }

    // SFX / REVERB: drive arriba, perillas de reverb abajo
    auto p2 = innerRect(panelRect(2, w, h));
    auto top2 = p2.removeFromTop(120);
    placeKnob(sfxDrive, top2, 40);
    const auto revCells = cellsOf(p2, 3, 1);
    placeKnob(revRoom, revCells[0].reduced(4, 0), 30);
    placeKnob(revDamp, revCells[1].reduced(4, 0), 30);
    placeKnob(revMix,  revCells[2].reduced(4, 0), 30);

    // DELAY: mezcla arriba, divisiones en 2 filas de 4 abajo
    auto p3 = innerRect(panelRect(3, w, h));
    auto top3 = p3.removeFromTop(120);
    placeKnob(delayMix, top3, 40);
    auto divArea = p3.removeFromTop(50).reduced(2, 4);
    const int cw = divArea.getWidth() / 4;
    const int rh = divArea.getHeight() / 2;
    for (int i = 0; i < numDivButtons; ++i)
    {
        const int row = i / 4;
        const int col = i % 4;
        divButtons[i].setBounds(divArea.getX() + col * cw, divArea.getY() + row * rh, cw - 3, rh - 3);
    }

    // Pie: INPUT a la izquierda, OUTPUT a la derecha
    inGain.slider.setBounds(pad, h - footerH + 6, 56, 56);
    inGain.label.setBounds(pad + 64, h - footerH + 30, 70, 14);
    inGain.label.setJustificationType(juce::Justification::left);

    outGain.slider.setBounds(w - pad - 56, h - footerH + 6, 56, 56);
    outGain.label.setBounds(w - pad - 134, h - footerH + 30, 70, 14);
    outGain.label.setJustificationType(juce::Justification::right);
}
