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
