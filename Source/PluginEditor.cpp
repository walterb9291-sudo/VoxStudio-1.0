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

    void placeKnob(VoxStudioEditor::Knob& k, juce::Rectangle<int> cell, int size)
    {
        k.label.setBounds(cell.removeFromBottom(12));
        const int s = juce::jmin(size, juce::jmin(cell.getWidth(), cell.getHeight()));
        k.slider.setBounds(cell.getCentreX() - s / 2, cell.getCentreY() - s / 2, s, s);
    }

    juce::Rectangle<int> footerInMeter(int h)         { return { pad + 140, h - footerH + 33, 250, 6 }; }
    juce::Rectangle<int> footerOutMeter(int w, int h) { return { w - 412, h - footerH + 33, 250, 6 }; }

    // Perilla con aspecto analógico: aro metálico, tapa oscura, arco de valor y puntero menta
    void drawAnalogKnob(juce::Graphics& g, float cx, float cy, float radius,
                        float sliderPos, float startAngle, float endAngle)
    {
        const float angle = startAngle + sliderPos * (endAngle - startAngle);

        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3a3360), cx, cy - radius,
                                               juce::Colour(0xff07060f), cx, cy + radius, false));
        g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
        g.setColour(Neon::violet.withAlpha(0.35f));
        g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.0f);

        const float arcR = radius - 4.0f;
        if (sliderPos > 0.001f)
        {
            juce::Path value;
            value.addCentredArc(cx, cy, arcR, arcR, 0.0f, startAngle, angle, true);
            g.setGradientFill(juce::ColourGradient(Neon::violet, cx - arcR, cy,
                                                   Neon::sky, cx + arcR, cy, false));
            g.strokePath(value, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
        }

        const float capR = radius * 0.68f;
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3d3766), cx - capR * 0.35f, cy - capR * 0.45f,
                                               juce::Colour(0xff0c0a18), cx + capR, cy + capR, true));
        g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);

        g.setColour(Neon::grid.withAlpha(0.8f));
        g.drawEllipse(cx - capR * 0.82f, cy - capR * 0.82f, capR * 1.64f, capR * 1.64f, 1.0f);

        const float px0 = cx + capR * 0.15f * std::sin(angle);
        const float py0 = cy - capR * 0.15f * std::cos(angle);
        const float px1 = cx + capR * 0.78f * std::sin(angle);
        const float py1 = cy - capR * 0.78f * std::cos(angle);
        g.setColour(Neon::mint);
        g.drawLine(juce::Line<float>(px0, py0, px1, py1), 3.0f);
        g.fillEllipse(px1 - 3.0f, py1 - 3.0f, 6.0f, 6.0f);
    }

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
    setColour(juce::PopupMenu::highlightedTextColourId, Neon::text);
    setColour(juce::Slider::textBoxTextColourId, Neon::text);
    setColour(juce::TextButton::buttonColourId, Neon::panel);
    setColour(juce::TextButton::buttonOnColourId, Neon::sky.withAlpha(0.35f));
    setColour(juce::TextButton::textColourOffId, Neon::muted);
    setColour(juce::TextButton::textColourOnId, Neon::text);
}

juce::Font NeonLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return juce::Font(juce::FontOptions("Segoe UI", 13.0f, juce::Font::bold | juce::Font::italic));
}

// Selector sin cuadro: solo una línea violeta fina abajo y una flecha pequeña
void NeonLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
                                   int, int, int, int, juce::ComboBox&)
{
    const auto r = juce::Rectangle<float>(0.0f, 0.0f, (float) width, (float) height);

    g.setGradientFill(juce::ColourGradient(Neon::violet.withAlpha(0.0f), r.getX() + 2.0f, r.getBottom(),
                                           Neon::violet, r.getRight() - 2.0f, r.getBottom(), false));
    g.fillRect(r.getX() + 2.0f, r.getBottom() - 1.0f, r.getWidth() - 4.0f, 1.0f);

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
    const auto area = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height).reduced(8.0f);
    const float radius = juce::jmin(area.getWidth(), area.getHeight()) * 0.5f;
    const float cx = area.getCentreX();
    const float cy = area.getCentreY();

    drawAnalogKnob(g, cx, cy, radius, sliderPos, startAngle, endAngle);

    // Valor sobre la tapa: siempre en THRESHOLD, y al pasar el mouse o girar en las demás
    const bool alwaysShow = slider.getComponentID() == "readout";
    if (alwaysShow || slider.isMouseOverOrDragging())
    {
        const float capR = radius * 0.68f;
        g.setColour(Neon::text);
        g.setFont(juce::FontOptions("Segoe UI", juce::jlimit(7.0f, 14.0f, capR * 0.55f), juce::Font::bold));
        g.drawText(slider.getTextFromValue(slider.getValue()),
                   juce::Rectangle<float>(cx - capR, cy - capR * 0.35f, capR * 2.0f, capR * 0.7f).toNearestInt(),
                   juce::Justification::centred, false);
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

    presetBox.addItemList(VoxStudioProcessor::reverbPresetNames, 1);
    presetBox.setColour(juce::ComboBox::textColourId, Neon::mint);
    presetBox.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(presetB
