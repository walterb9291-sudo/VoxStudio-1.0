#include "PluginEditor.h"

namespace Neon
{
    const juce::Colour bg      { 0xff07080d };
    const juce::Colour panel   { 0xff0c0e16 };
    const juce::Colour panelHi { 0xff141827 };
    const juce::Colour grid    { 0xff1d2233 };
    const juce::Colour violet  { 0xffa855f7 };
    const juce::Colour sky     { 0xff22d3ee };
    const juce::Colour mint    { 0xff22c55e };
    const juce::Colour amber   { 0xfff59e0b };
    const juce::Colour blue    { 0xff38bdf8 };
    const juce::Colour purple  { 0xffa855f7 };
    const juce::Colour pink    { 0xffd946ef };
    const juce::Colour yellow  { 0xffeab308 };
    const juce::Colour text    { 0xfff3f4f6 };
    const juce::Colour label   { 0xffcbd5e1 };
    const juce::Colour muted   { 0xff6b7280 };
}

namespace
{
    constexpr int pad      = 12;
    constexpr int sideW    = 56;
    constexpr int gap      = 10;
    constexpr int headerH  = 74;
    constexpr int panelTop = 88;
    constexpr int footerH  = 72;
    constexpr int gateBoxH = 110;

    const char* const kDivLabels[8] = { "1/1", "1/2", "1/4", "1/8", "1/16", "1/4.", "1/8.", "1/4T" };

    const juce::StringArray kDynPresets   { "Warm Tape", "Clean", "Crunch", "Lo-Fi", "Vintage", "Modern" };
    const juce::StringArray kDelayPresets { "Slapback", "Dotted Echo", "Ping Pong", "Tape Echo", "Ambient" };
    const juce::StringArray kEqPresets    { "Warm", "Airy", "Tape", "Flat" };

    struct PV { const char* id; float value; };

    void applyValues(juce::AudioProcessorValueTreeState& s, std::initializer_list<PV> list)
    {
        for (const auto& pv : list)
            if (auto* p = s.getParameter(pv.id))
                p->setValueNotifyingHost(p->convertTo0to1(pv.value));
    }

    void applyEqPreset(juce::AudioProcessorValueTreeState& s, int i)
    {
        switch (i)
        {
            case 0:
                applyValues(s, { { "eq1_freq", 150.0f }, { "eq1_gain", 1.5f },
                                 { "eq2_freq", 400.0f }, { "eq2_gain", 0.5f },
                                 { "eq3_freq", 2500.0f }, { "eq3_gain", -1.0f },
                                 { "eq4_freq", 9000.0f }, { "eq4_gain", -2.5f } });
                break;
            case 1:
                applyValues(s, { { "eq1_freq", 120.0f }, { "eq1_gain", -1.5f },
                                 { "eq2_freq", 350.0f }, { "eq2_gain", -1.0f },
                                 { "eq3_freq", 3000.0f }, { "eq3_gain", 1.0f },
                                 { "eq4_freq", 12000.0f }, { "eq4_gain", 3.0f } });
                break;
            case 2:
                applyValues(s, { { "eq1_freq", 100.0f }, { "eq1_gain", 1.0f },
                                 { "eq2_freq", 1000.0f }, { "eq2_gain", -0.5f },
                                 { "eq3_freq", 3500.0f }, { "eq3_gain", -1.5f },
                                 { "eq4_freq", 10000.0f }, { "eq4_gain", -3.0f } });
                break;
            default:
                applyValues(s, { { "eq1_freq", 120.0f }, { "eq1_gain", 0.0f },
                                 { "eq2_freq", 500.0f }, { "eq2_gain", 0.0f },
                                 { "eq3_freq", 2500.0f }, { "eq3_gain", 0.0f },
                                 { "eq4_freq", 8000.0f }, { "eq4_gain", 0.0f } });
                break;
        }
    }

    // Reverb: tipo, tamaño, decay, amortiguación, mezcla, pre-delay, rate y depth de la modulación
    void applyReverbPresetValues(juce::AudioProcessorValueTreeState& s, int i)
    {
        switch (i)
        {
            case 0: applyValues(s, { { "rev_preset", 0.0f }, { "rev_room", 0.3f }, { "rev_decay", 0.8f },
                                     { "rev_damp", 0.6f }, { "rev_mix", 0.2f }, { "rev_pre", 5.0f },
                                     { "mod_rate", 0.6f }, { "mod_depth", 0.20f } }); break;
            case 1: applyValues(s, { { "rev_preset", 1.0f }, { "rev_room", 0.7f }, { "rev_decay", 2.8f },
                                     { "rev_damp", 0.4f }, { "rev_mix", 0.25f }, { "rev_pre", 20.0f },
                                     { "mod_rate", 0.4f }, { "mod_depth", 0.15f } }); break;
            case 2: applyValues(s, { { "rev_preset", 2.0f }, { "rev_room", 0.5f }, { "rev_decay", 1.8f },
                                     { "rev_damp", 0.2f }, { "rev_mix", 0.25f }, { "rev_pre", 10.0f },
                                     { "mod_rate", 0.9f }, { "mod_depth", 0.35f } }); break;
            case 3: applyValues(s, { { "rev_preset", 3.0f }, { "rev_room", 0.45f }, { "rev_decay", 1.6f },
                                     { "rev_damp", 0.7f }, { "rev_mix", 0.2f }, { "rev_pre", 30.0f },
                                     { "mod_rate", 0.5f }, { "mod_depth", 0.25f } }); break;
            default: applyValues(s, { { "rev_preset", 4.0f }, { "rev_room", 0.9f }, { "rev_decay", 4.0f },
                                      { "rev_damp", 0.1f }, { "rev_mix", 0.3f }, { "rev_pre", 40.0f },
                                      { "mod_rate", 1.5f }, { "mod_depth", 0.50f } }); break;
        }
    }

    void applyDelayPresetValues(juce::AudioProcessorValueTreeState& s, int i)
    {
        switch (i)
        {
            case 0: applyValues(s, { { "sfx_delay_div", 3.0f }, { "sfx_delay_fb", 0.15f }, { "sfx_delay", 0.20f },
                                     { "sfx_delay_dry", 1.0f }, { "sfx_delay_drv", 0.0f } }); break;
            case 1: applyValues(s, { { "sfx_delay_div", 5.0f }, { "sfx_delay_fb", 0.40f }, { "sfx_delay", 0.25f },
                                     { "sfx_delay_dry", 1.0f }, { "sfx_delay_drv", 0.1f } }); break;
            case 2: applyValues(s, { { "sfx_delay_div", 3.0f }, { "sfx_delay_fb", 0.45f }, { "sfx_delay", 0.30f },
                                     { "sfx_delay_dry", 1.0f }, { "sfx_delay_drv", 0.0f } }); break;
            case 3: applyValues(s, { { "sfx_delay_div", 2.0f }, { "sfx_delay_fb", 0.50f }, { "sfx_delay", 0.30f },
                                     { "sfx_delay_dry", 0.9f }, { "sfx_delay_drv", 0.4f } }); break;
            default: applyValues(s, { { "sfx_delay_div", 0.0f }, { "sfx_delay_fb", 0.70f }, { "sfx_delay", 0.35f },
                                      { "sfx_delay_dry", 0.9f }, { "sfx_delay_drv", 0.2f } }); break;
        }
    }

    void applyDynPresetValues(juce::AudioProcessorValueTreeState& s, int i)
    {
        switch (i)
        {
            case 0: applyValues(s, { { "dyn_drive", 0.25f }, { "dyn_color", 0.45f }, { "dyn_hp", 30.0f },
                                     { "dyn_lp", 16000.0f }, { "dyn_deess", 0.20f }, { "dyn_focus", 0.50f },
                                     { "dyn_mix", 1.0f } }); break;
            case 1: applyValues(s, { { "dyn_drive", 0.0f }, { "dyn_color", 0.50f }, { "dyn_hp", 60.0f },
                                     { "dyn_lp", 20000.0f }, { "dyn_deess", 0.10f }, { "dyn_focus", 0.50f },
                                     { "dyn_mix", 1.0f } }); break;
            case 2: applyValues(s, { { "dyn_drive", 0.50f }, { "dyn_color", 0.55f }, { "dyn_hp", 80.0f },
                                     { "dyn_lp", 12000.0f }, { "dyn_deess", 0.30f }, { "dyn_focus", 0.55f },
                                     { "dyn_mix", 1.0f } }); break;
            case 3: applyValues(s, { { "dyn_drive", 0.60f }, { "dyn_color", 0.20f }, { "dyn_hp", 180.0f },
                                     { "dyn_lp", 4500.0f }, { "dyn_deess", 0.0f }, { "dyn_focus", 0.50f },
                                     { "dyn_mix", 0.9f } }); break;
            case 4: applyValues(s, { { "dyn_drive", 0.35f }, { "dyn_color", 0.40f }, { "dyn_hp", 50.0f },
                                     { "dyn_lp", 10000.0f }, { "dyn_deess", 0.25f }, { "dyn_focus", 0.45f },
                                     { "dyn_mix", 1.0f } }); break;
            default: applyValues(s, { { "dyn_drive", 0.15f }, { "dyn_color", 0.60f }, { "dyn_hp", 40.0f },
                                      { "dyn_lp", 20000.0f }, { "dyn_deess", 0.35f }, { "dyn_focus", 0.60f },
                                      { "dyn_mix", 1.0f } }); break;
        }
    }

    // ---------- Presets globales: una sola selección ajusta las cuatro columnas ----------
    // eq, rev, dly, dyn: índice del preset de cada columna (-1 = no cambiar la reverb o el delay)
    struct GlobalPreset
    {
        const char* name;
        int eq, rev, dly, dyn;
        float thr, ratio, att, rel, mk, gate;
    };

    const GlobalPreset kGlobalPresets[] = {
        { "Vocal Clean",  3, 3, 0,  1, -20.0f, 3.0f, 10.0f, 120.0f, 3.0f, -50.0f },
        { "Vocal Bright", 1, 2, 0,  1, -18.0f, 3.5f,  8.0f, 100.0f, 4.0f, -50.0f },
        { "Podcast",      0, 0, 0,  1, -22.0f, 4.0f, 12.0f, 150.0f, 5.0f, -45.0f },
        { "Radio",        2, 0, -1, 3, -15.0f, 6.0f,  5.0f,  80.0f, 6.0f, -40.0f },
        { "Telefono",     2, -1, -1, 3, -12.0f, 8.0f, 3.0f,  60.0f, 8.0f, -35.0f },
    };
    constexpr int kNumGlobalPresets = (int) (sizeof(kGlobalPresets) / sizeof(kGlobalPresets[0]));

    void applyGlobalPreset(juce::AudioProcessorValueTreeState& s, int i)
    {
        const auto& gp = kGlobalPresets[i];

        applyEqPreset(s, gp.eq);

        if (gp.rev >= 0)
            applyReverbPresetValues(s, gp.rev);
        else
            applyValues(s, { { "rev_mix", 0.0f } });

        if (gp.dly >= 0)
            applyDelayPresetValues(s, gp.dly);
        else
            applyValues(s, { { "sfx_delay", 0.0f } });

        applyDynPresetValues(s, gp.dyn);

        applyValues(s, { { "comp_threshold", gp.thr }, { "comp_ratio", gp.ratio },
                         { "comp_attack", gp.att },    { "comp_release", gp.rel },
                         { "comp_makeup", gp.mk },     { "dyn_gate", gp.gate } });
    }

    // Muestra la lista de presets globales filtrada por lo escrito en el buscador
    void showGlobalPresetMenu(juce::TextEditor& box, juce::AudioProcessorValueTreeState& s)
    {
        const juce::String query = box.getText().trim().toLowerCase();

        juce::PopupMenu menu;
        for (int i = 0; i < kNumGlobalPresets; ++i)
        {
            const juce::String name = kGlobalPresets[i].name;
            if (query.isEmpty() || name.toLowerCase().contains(query))
                menu.addItem(i + 1, name);
        }

        if (menu.getNumItems() == 0)
            return;

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&box),
                           [sp = &s, bp = &box](int result)
                           {
                               if (result > 0)
                               {
                                   applyGlobalPreset(*sp, result - 1);
                                   bp->setText(kGlobalPresets[result - 1].name, false);
                               }
                           });
    }

    juce::Rectangle<int> panelRect(int i, int w, int h)
    {
        const int colW = (w - 2 * sideW - 2 * pad - 3 * gap) / 4;
        return { sideW + pad + i * (colW + gap), panelTop, colW, h - panelTop - footerH };
    }

    juce::Rectangle<int> innerRect(juce::Rectangle<int> p)
    {
        return p.reduced(12, 0).withTrimmedTop(46).withTrimmedBottom(12);
    }

    juce::Rectangle<int> gateBoxRect(int w, int h)
    {
        auto p = innerRect(panelRect(0, w, h));
        return p.withTop(p.getBottom() - gateBoxH);
    }

    juce::Rectangle<float> eqAreaFor(int w, int h)
    {
        return panelRect(1, w, h).toFloat().reduced(12.0f, 0.0f).withTrimmedTop(50.0f).withHeight(200.0f);
    }

    juce::Rectangle<int> eqPresetRect(int w, int h)
    {
        const auto pr = panelRect(1, w, h);
        return { pr.getX() + 12, panelTop + 262, pr.getWidth() - 24, 28 };
    }

    juce::Rectangle<int> eqBandsArea(int w, int h)
    {
        const auto pr = panelRect(1, w, h);
        const int y = panelTop + 298;
        return { pr.getX() + 12, y, pr.getWidth() - 24, pr.getBottom() - 12 - y };
    }

    std::vector<juce::Rectangle<int>> cellsOf(juce::Rectangle<int> a, int cols, int rows)
    {
        std::vector<juce::Rectangle<int>> out;
        const int cw = a.getWidth() / cols;
        const int rh = a.getHeight() / rows;
        for (int r = 0; r < rows; ++r)
        {
            for (int c = 0; c < cols; ++c)
            {
                out.push_back({ a.getX() + c * cw, a.getY() + r * rh, cw, rh });
            }
        }
        return out;
    }

    void tint(VoxStudioEditor::Knob& k, juce::Colour c)
    {
        k.slider.setColour(juce::Slider::rotarySliderFillColourId, c);
    }

    void placeKnob(VoxStudioEditor::Knob& k, juce::Rectangle<int> cell, int size)
    {
        k.label.setBounds(cell.removeFromBottom(12));
        const int s = juce::jmin(size, juce::jmin(cell.getWidth(), cell.getHeight()));
        k.slider.setBounds(cell.getCentreX() - s / 2, cell.getCentreY() - s / 2, s, s);
    }

    juce::Rectangle<int> footerInMeter(int h)         { return { pad + 140, h - footerH + 33, 250, 6 }; }
    juce::Rectangle<int> footerOutMeter(int w, int h) { return { w - 412, h - footerH + 33, 250, 6 }; }

    void drawVoiceIcon(juce::Graphics& g, float x, float cy)
    {
        const float heights[] = { 10.0f, 20.0f, 30.0f, 16.0f, 8.0f };
        for (int i = 0; i < 5; ++i)
        {
            g.setColour(i % 2 == 0 ? Neon::mint : Neon::sky);
            const float bh = heights[i];
            g.fillRoundedRectangle(x + i * 6.0f, cy - bh * 0.5f, 3.0f, bh, 1.5f);
        }
    }

    void drawFooterIcon(juce::Graphics& g, float cx, float cy)
    {
        g.setColour(Neon::panel);
        g.fillEllipse(cx - 20.0f, cy - 20.0f, 40.0f, 40.0f);
        g.setColour(Neon::grid);
        g.drawEllipse(cx - 20.0f, cy - 20.0f, 40.0f, 40.0f, 1.0f);

        const float heights[] = { 8.0f, 16.0f, 24.0f, 30.0f, 24.0f, 16.0f, 8.0f };
        for (int i = 0; i < 7; ++i)
        {
            g.setColour(i < 4 ? Neon::sky : Neon::pink);
            const float bh = heights[i];
            g.fillRoundedRectangle(cx - 18.0f + i * 6.0f, cy - bh * 0.5f, 3.0f, bh, 1.5f);
        }
    }

    void drawIcon(juce::Graphics& g, juce::Rectangle<float> r, int kind, juce::Colour c)
    {
        g.setColour(c);

        if (kind == 0)
        {
            juce::Path p;
            p.startNewSubPath(r.getX(), r.getCentreY());
            p.lineTo(r.getX() + r.getWidth() * 0.3f, r.getCentreY());
            p.lineTo(r.getX() + r.getWidth() * 0.45f, r.getY());
            p.lineTo(r.getX() + r.getWidth() * 0.6f, r.getBottom());
            p.lineTo(r.getRight(), r.getCentreY());
            g.strokePath(p, juce::PathStrokeType(1.6f));
        }
        else if (kind == 1)
        {
            g.fillRoundedRectangle(r.getX(), r.getY() + r.getHeight() * 0.3f, 4.0f, r.getHeight() * 0.7f, 1.5f);
            g.fillRoundedRectangle(r.getX() + 7.0f, r.getY(), 4.0f, r.getHeight(), 1.5f);
            g.fillRoundedRectangle(r.getX() + 14.0f, r.getY() + r.getHeight() * 0.5f, 4.0f, r.getHeight() * 0.5f, 1.5f);
        }
        else if (kind == 2)
        {
            g.drawEllipse(r.reduced(r.getWidth() * 0.3f), 1.5f);
            g.fillEllipse(r.getCentreX() - 2.0f, r.getCentreY() - 2.0f, 4.0f, 4.0f);
        }
        else if (kind == 3)
        {
            g.drawEllipse(r.reduced(1.0f), 1.5f);
            g.drawEllipse(r.reduced(r.getWidth() * 0.28f), 1.0f);
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
        g.setColour(accent.withAlpha(0.7f));
        g.strokePath(shape, juce::PathStrokeType(1.5f));

        drawIcon(g, { r.getX() + 16.0f, r.getY() + 14.0f, 14.0f, 14.0f }, icon, accent);

        g.setColour(accent);
        g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
        g.drawText(name, juce::Rectangle<int>((int) (r.getX() + 36.0f), (int) (r.getY() + 12.0f),
                                              (int) (r.getWidth() - 46.0f), 18),
                   juce::Justification::centredLeft, false);

        g.setGradientFill(juce::ColourGradient(accent.withAlpha(0.6f), r.getX() + 16.0f, r.getY() + 38.0f,
                                               accent.withAlpha(0.0f), r.getRight() - 16.0f, r.getY() + 38.0f, false));
        g.fillRect(r.getX() + 16.0f, r.getY() + 38.0f, r.getWidth() - 32.0f, 1.0f);
    }

    void drawMeter(juce::Graphics& g, juce::Rectangle<int> r, float level)
    {
        g.setColour(Neon::grid);
        g.fillRoundedRectangle(r.toFloat(), 2.0f);
        const float fillW = (float) r.getWidth() * juce::jlimit(0.0f, 1.0f, level);
        g.setColour(Neon::mint);
        g.fillRoundedRectangle(r.toFloat().withWidth(fillW), 2.0f);
    }

    void drawSideMeter(juce::Graphics& g, juce::Rectangle<int> r, float level)
    {
        g.setColour(Neon::panel);
        g.fillRoundedRectangle(r.toFloat(), 6.0f);
        g.setColour(Neon::grid);
        g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 6.0f, 1.0f);

        const int segs = 16;
        const float lv = juce::jlimit(0.0f, 1.0f, level);
        const int lit = (int) std::round(lv * segs);
        const float segH = ((float) r.getHeight() - 14.0f) / (float) segs;

        for (int s = 0; s < segs; ++s)
        {
            const float y = (float) r.getBottom() - 7.0f - (s + 1) * segH + 1.0f;
            juce::Colour c = Neon::mint;
            if (s >= 11)
                c = Neon::amber;
            else if (s >= 8)
                c = Neon::yellow;

            g.setColour(s < lit ? c : Neon::grid);
            g.fillRoundedRectangle((float) r.getX() + 8.0f, y, (float) r.getWidth() - 16.0f, segH - 2.0f, 1.5f);
        }
    }

    void drawVuMeter(juce::Graphics& g, juce::Rectangle<float> r, float grDb)
    {
        g.setColour(Neon::bg);
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour(Neon::grid);
        g.drawRoundedRectangle(r.reduced(0.5f), 6.0f, 1.0f);

        const float cx = r.getCentreX();
        const float cy = r.getBottom() - 4.0f;
        const float radius = r.getHeight() * 0.95f;
        const float startA = -0.75f;
        const float endA   =  0.75f;

        juce::Path scale;
        scale.addCentredArc(cx, cy, radius, radius, 0.0f, startA, endA, true);
        g.setColour(Neon::muted);
        g.strokePath(scale, juce::PathStrokeType(1.5f));

        const float marksDb[] = { 0.0f, -3.0f, -6.0f, -10.0f, -20.0f };
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        for (float db : marksDb)
        {
            const float t = juce::jmap(-db, 0.0f, 20.0f, 0.0f, 1.0f);
            const float a = startA + t * (endA - startA);

            g.setColour(Neon::label);
            g.drawLine(cx + (radius - 2.0f) * std::sin(a), cy - (radius - 2.0f) * std::cos(a),
                       cx + (radius - 9.0f) * std::sin(a), cy - (radius - 9.0f) * std::cos(a), 1.5f);

            const float lx = cx + (radius - 20.0f) * std::sin(a);
            const float ly = cy - (radius - 20.0f) * std::cos(a);
            g.drawText(juce::String((int) db), juce::Rectangle<float>(lx - 12.0f, ly - 7.0f, 24.0f, 14.0f),
                       juce::Justification::centred, false);
        }

        const float t = juce::jlimit(0.0f, 1.0f, grDb / 20.0f);
        const float a = startA + t * (endA - startA);
        g.setColour(Neon::sky);
        g.drawLine(cx, cy, cx + (radius - 8.0f) * std::sin(a), cy - (radius - 8.0f) * std::cos(a), 2.0f);
        g.fillEllipse(cx - 4.0f, cy - 4.0f, 8.0f, 8.0f);

        g.setColour(Neon::label);
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.drawText("GR dB", juce::Rectangle<float>(r.getRight() - 56.0f, r.getY() + 4.0f, 50.0f, 14.0f),
                   juce::Justification::right, false);
    }

    void drawAnalogKnob(juce::Graphics& g, float cx, float cy, float radius,
                        float sliderPos, float startAngle, float endAngle, juce::Colour accent)
    {
        const float angle = startAngle + sliderPos * (endAngle - startAngle);
        const juce::Colour bright = accent.brighter(0.35f);

        for (int i = 0; i <= 10; ++i)
        {
            const float t = (float) i / 10.0f;
            const float a = startAngle + t * (endAngle - startAngle);
            const float r1 = radius + 1.0f;
            const float r2 = radius + (i % 5 == 0 ? 6.0f : 3.5f);
            g.setColour(t <= sliderPos + 0.001f ? accent : Neon::grid);
            g.drawLine(cx + r1 * std::sin(a), cy - r1 * std::cos(a),
                       cx + r2 * std::sin(a), cy - r2 * std::cos(a), 1.5f);
        }

        const float trackR = radius - 5.0f;

        juce::Path track;
        track.addCentredArc(cx, cy, trackR, trackR, 0.0f, startAngle, endAngle, true);
        g.setColour(Neon::grid);
        g.strokePath(track, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (sliderPos > 0.001f)
        {
            juce::Path value;
            value.addCentredArc(cx, cy, trackR, trackR, 0.0f, startAngle, angle, true);
            g.setGradientFill(juce::ColourGradient(accent.withAlpha(0.25f), cx - trackR, cy,
                                                   bright.withAlpha(0.25f), cx + trackR, cy, false));
            g.strokePath(value, juce::PathStrokeType(9.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setGradientFill(juce::ColourGradient(accent, cx - trackR, cy, bright, cx + trackR, cy, false));
            g.strokePath(value, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        const float capR = radius * 0.66f;
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2a2f45), cx - capR * 0.35f, cy - capR * 0.45f,
                                               juce::Colour(0xff07080d), cx + capR, cy + capR, true));
        g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);
        g.setColour(accent.withAlpha(0.35f));
        g.drawEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f, 1.0f);

        const float px0 = cx + capR * 0.15f * std::sin(angle);
        const float py0 = cy - capR * 0.15f * std::cos(angle);
        const float px1 = cx + capR * 0.78f * std::sin(angle);
        const float py1 = cy - capR * 0.78f * std::cos(angle);
        g.setColour(Neon::text);
        g.drawLine(juce::Line<float>(px0, py0, px1, py1), 2.5f);
        g.setColour(bright);
        g.fillEllipse(px1 - 3.0f, py1 - 3.0f, 6.0f, 6.0f);
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
    setColour(juce::PopupMenu::highlightedBackgroundColourId, Neon::violet.withAlpha(0.35f));
    setColour(juce::PopupMenu::highlightedTextColourId, Neon::text);
    setColour(juce::Slider::textBoxTextColourId, Neon::text);
    setColour(juce::TextButton::buttonColourId, Neon::panel);
    setColour(juce::TextButton::buttonOnColourId, Neon::violet.withAlpha(0.35f));
    setColour(juce::TextButton::textColourOffId, Neon::text);
    setColour(juce::TextButton::textColourOnId, Neon::text);
}

juce::Font NeonLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return juce::Font(juce::FontOptions("Segoe UI", 12.0f, juce::Font::bold));
}

void NeonLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
                                   int, int, int, int, juce::ComboBox&)
{
    const auto r = juce::Rectangle<float>(0.0f, 0.0f, (float) width, (float) height);
    g.setColour(Neon::panel);
    g.fillRoundedRectangle(r, 6.0f);
    g.setColour(Neon::sky.withAlpha(0.6f));
    g.drawRoundedRectangle(r.reduced(0.5f), 6.0f, 1.0f);

    juce::Path arrow;
    arrow.startNewSubPath(width - 14.0f, height * 0.44f);
    arrow.lineTo(width - 10.5f, height * 0.60f);
    arrow.lineTo(width - 7.0f, height * 0.44f);
    g.setColour(Neon::sky);
    g.strokePath(arrow, juce::PathStrokeType(1.2f));
}

void NeonLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float startAngle, float endAngle,
                                       juce::Slider& slider)
{
    const auto area = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height).reduced(8.0f);
    const float radius = juce::jmin(area.getWidth(), area.getHeight()) * 0.5f;
    const float cx = area.getCentreX();
    const float cy = area.getCentreY();

    const juce::Colour accent = slider.findColour(juce::Slider::rotarySliderFillColourId);
    drawAnalogKnob(g, cx, cy, radius, sliderPos, startAngle, endAngle, accent);

    const bool alwaysShow = slider.getComponentID() == "readout";
    if (alwaysShow || slider.isMouseOverOrDragging())
    {
        const float capR = radius * 0.66f;
        g.setColour(Neon::text);
        g.setFont(juce::FontOptions("Segoe UI", juce::jlimit(8.0f, 16.0f, capR * 0.55f), juce::Font::bold));
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

    title.setVisible(false);
    subtitle.setVisible(false);
    producer.setVisible(false);

    // Buscador de presets globales: escribe el nombre y pulsa Enter
    searchBox.setTextToShowWhenEmpty("Buscar preset...", Neon::muted);
    searchBox.setColour(juce::TextEditor::backgroundColourId, Neon::panel);
    searchBox.setColour(juce::TextEditor::textColourId, Neon::text);
    searchBox.setColour(juce::TextEditor::outlineColourId, Neon::sky.withAlpha(0.6f));
    searchBox.setColour(juce::TextEditor::focusedOutlineColourId, Neon::sky);
    searchBox.setFont(juce::Font(juce::FontOptions("Segoe UI", 12.0f, juce::Font::plain)));
    searchBox.onReturnKey = [this] { showGlobalPresetMenu(searchBox, processor.apvts); };
    addAndMakeVisible(searchBox);

    presetBox.addItemList(VoxStudioProcessor::reverbPresetNames, 1);
    presetBox.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(presetBox);
    presetAttachment = std::make_unique<juce::ComboBoxParameterAttachment>(
        *processor.apvts.getParameter("rev_preset"), presetBox, nullptr);

    eqPresetBox.addItemList(kEqPresets, 1);
    eqPresetBox.setSelectedId(1, juce::dontSendNotification);
    eqPresetBox.setJustificationType(juce::Justification::centred);
    eqPresetBox.onChange = [this] { applyEqPreset(processor.apvts, eqPresetBox.getSelectedId() - 1); };
    addAndMakeVisible(eqPresetBox);

    dynPresetBox.addItemList(kDynPresets, 1);
    dynPresetBox.setSelectedId(1, juce::dontSendNotification);
    dynPresetBox.setJustificationType(juce::Justification::centredLeft);
    dynPresetBox.onChange = [this] { applyDynPresetValues(processor.apvts, dynPresetBox.getSelectedId() - 1); };
    addAndMakeVisible(dynPresetBox);

    delayPresetBox.addItemList(kDelayPresets, 1);
    delayPresetBox.setSelectedId(1, juce::dontSendNotification);
    delayPresetBox.setJustificationType(juce::Justification::centredLeft);
    delayPresetBox.onChange = [this] { applyDelayPresetValues(processor.apvts, delayPresetBox.getSelectedId() - 1); };
    addAndMakeVisible(delayPresetBox);

    bypassButton.setClickingTogglesState(true);
    addAndMakeVisible(bypassButton);
    bypassAttachment = std::make_unique<juce::ButtonParameterAttachment>(
        *processor.apvts.getParameter("bypass"), bypassButton, nullptr);

    setupKnob(inGain,  "in_gain",  "INPUT");
    setupKnob(outGain, "out_gain", "OUTPUT");
    tint(inGain,  Neon::mint);
    tint(outGain, Neon::yellow);
    inGain.slider.textFromValueFunction = [](double v) {
        return juce::String(juce::jlimit(0.0, 100.0, (v + 24.0) / 36.0 * 100.0), 0);
    };
    outGain.slider.textFromValueFunction = [](double v) {
        return juce::String(juce::jlimit(0.0, 100.0, (v + 24.0) / 36.0 * 100.0), 0);
    };

    setupKnob(compThreshold, "comp_threshold", "THRESHOLD");
    compThreshold.slider.setComponentID("readout");
    compThreshold.slider.textFromValueFunction = [](double v) { return juce::String(v, 0) + " dB"; };
    setupKnob(compRatio,   "comp_ratio",   "RATIO");
    compRatio.slider.textFromValueFunction = [](double v) { return juce::String(v, 1) + ":1"; };
    setupKnob(compAttack,  "comp_attack",  "ATTACK");
    compAttack.slider.textFromValueFunction = [](double v) { return juce::String(v, 0) + " ms"; };
    setupKnob(compRelease, "comp_release", "RELEASE");
    compRelease.slider.textFromValueFunction = [](double v) { return juce::String(v, 0) + " ms"; };
    setupKnob(compMakeup,  "comp_makeup",  "MAKEUP");
    compMakeup.slider.textFromValueFunction = [](double v) { return "+" + juce::String(v, 1) + " dB"; };

    tint(compThreshold, Neon::sky);
    tint(compRatio,     Neon::sky);
    tint(compAttack,    Neon::sky);
    tint(compRelease,   Neon::sky);
    tint(compMakeup,    Neon::sky);

    setupKnob(dynGate, "dyn_gate", "UMBRAL");
    dynGate.slider.setComponentID("readout");
    dynGate.slider.textFromValueFunction = [](double v) { return juce::String(v, 0) + " dB"; };
    tint(dynGate, Neon::sky);

    setupKnob(gateHyst, "dyn_hyst", "HISTERESIS");
    gateHyst.slider.textFromValueFunction = [](double v) { return juce::String(v, 1) + " dB"; };
    tint(gateHyst, Neon::sky);

    const juce::Colour bandCol[4] = { Neon::blue, Neon::mint, Neon::amber, Neon::purple };
    for (int b = 0; b < VoxStudioProcessor::numEqBands; ++b)
    {
        const juce::String n = "eq" + juce::String(b + 1) + "_";
        setupKnob(eqFreq[b], n + "freq", "FREQ");
        eqFreq[b].slider.textFromValueFunction = [](double v) {
            return v >= 1000.0 ? juce::String(v / 1000.0, 1) + "k" : juce::String((int) v) + " Hz";
        };
        setupKnob(eqGain[b], n + "gain", "GAIN");
        eqGain[b].slider.textFromValueFunction = [](double v) {
            return (v > 0.0 ? "+" : "") + juce::String(v, 1) + " dB";
        };
        setupKnob(eqQ[b], n + "q", "Q");
        eqQ[b].slider.setVisible(false);
        eqQ[b].label.setVisible(false);

        tint(eqFreq[b], bandCol[b]);
        tint(eqGain[b], bandCol[b]);
    }

    setupKnob(sfxDrive, "sfx_drive", "DRIVE");
    sfxDrive.slider.setComponentID("drive");
    sfxDrive.slider.textFromValueFunction = [](double v) { return juce::String(v * 100.0, 0); };
    tint(sfxDrive, Neon::pink);

    setupKnob(revRoom, "rev_room", "ROOM");
    revRoom.slider.textFromValueFunction = [](double v) { return juce::String(v * 100.0, 0); };

    setupKnob(revDecay, "rev_decay", "DECAY");
    revDecay.slider.textFromValueFunction = [](double v) { return juce::String(v, 1) + " s"; };

    setupKnob(revDamp, "rev_damp", "DAMP");
    revDamp.slider.textFromValueFunction = [](double v) { return juce::String(v * 100.0, 0); };
    setupKnob(revMix,  "rev_mix",  "MIX");
    revMix.slider.textFromValueFunction = [](double v) { return juce::String(v * 100.0, 0); };

    setupKnob(revPre, "rev_pre", "PRE");
    revPre.slider.textFromValueFunction = [](double v) { return juce::String(v, 0) + " ms"; };

    setupKnob(modRate, "mod_rate", "RATE");
    modRate.slider.textFromValueFunction = [](double v) { return juce::String(v, 2) + " Hz"; };
    setupKnob(modDepth, "mod_depth", "DEPTH");
    modDepth.slider.textFromValueFunction = [](double v) { return juce::String(v * 100.0, 0) + "%"; };

    tint(revRoom,  Neon::pink);
    tint(revDecay, Neon::pink);
    tint(revDamp,  Neon::pink);
    tint(revMix,   Neon::pink);
    tint(revPre,   Neon::pink);
    tint(modRate,  Neon::pink);
    tint(modDepth, Neon::pink);

    setupKnob(delayMix, "sfx_delay", "WET");
    delayMix.slider.textFromValueFunction = [](double v) { return juce::String(v * 100.0, 0); };
    setupKnob(delayDry, "sfx_delay_dry", "DRY");
    delayDry.slider.textFromValueFunction = [](double v) { return juce::String(v * 100.0, 0); };
    setupKnob(delayFb, "sfx_delay_fb", "FEEDBACK");
    delayFb.slider.textFromValueFunction = [](double v) { return juce::String(v * 100.0, 0) + "%"; };
    setupKnob(delayDrv, "sfx_delay_drv", "DRIVE");
    delayDrv.slider.textFromValueFunction = [](double v) { return juce::String(v * 100.0, 0); };
    tint(delayMix, Neon::pink);
    tint(delayDry, Neon::pink);
    tint(delayFb,  Neon::pink);
    tint(delayDrv, Neon::pink);

    delayDiv.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    delayDiv.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    delayDiv.slider.setRange(0.0, 7.0, 1.0);
    delayDiv.slider.setComponentID("readout");
    delayDiv.slider.textFromValueFunction = [](double v) {
        const int i = juce::jlimit(0, 7, (int) std::round(v));
        return juce::String(kDivLabels[i]);
    };
    delayDiv.slider.setValue(processor.apvts.getRawParameterValue("sfx_delay_div")->load(),
                             juce::dontSendNotification);
    delayDiv.slider.onValueChange = [this] {
        setDelayDivision((int) std::round(delayDiv.slider.getValue()));
    };
    addAndMakeVisible(delayDiv.slider);
    tint(delayDiv, Neon::pink);
    delayDiv.label.setText("D TIME", juce::dontSendNotification);
    delayDiv.label.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    delayDiv.label.setColour(juce::Label::textColourId, Neon::label);
    delayDiv.label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(delayDiv.label);

    setupKnob(dynDeess, "dyn_deess", "DE-ESS");
    dynDeess.slider.textFromValueFunction = [](double v) { return juce::String(v * 100.0, 0) + "%"; };
    tint(dynDeess, Neon::amber);

    setupKnob(dynFocus, "dyn_focus", "FOCUS");
    dynFocus.slider.textFromValueFunction = [](double v) {
        const int hz = (int) (2000.0 * std::pow(5.0, v));
        return hz >= 1000 ? juce::String(hz / 1000.0, 1) + "k" : juce::String(hz) + " Hz";
    };
    tint(dynFocus, Neon::amber);

    setupKnob(dynDrive, "dyn_drive", "DRIVE");
    dynDrive.slider.textFromValueFunction = [](double v) { return juce::String(v * 100.0, 0) + "%"; };
    tint(dynDrive, Neon::amber);

    setupKnob(dynColor, "dyn_color", "COLOR");
    dynColor.slider.textFromValueFunction = [](double v) {
        const double db = (v - 0.5) * 12.0;
        return (db > 0.0 ? "+" : "") + juce::String(db, 1) + " dB";
    };
    tint(dynColor, Neon::amber);

    setupKnob(dynHpFreq, "dyn_hp", "HP FREQ");
    dynHpFreq.slider.textFromValueFunction = [](double v) { return juce::String((int) v) + " Hz"; };
    tint(dynHpFreq, Neon::violet);

    setupKnob(dynLpFreq, "dyn_lp", "LP FREQ");
    dynLpFreq.slider.textFromValueFunction = [](double v) {
        return v >= 1000.0 ? juce::String(v / 1000.0, 1) + "k" : juce::String((int) v) + " Hz";
    };
    tint(dynLpFreq, Neon::amber);

    setupKnob(dynMix, "dyn_mix", "MIX");
    dynMix.slider.textFromValueFunction = [](double v) { return juce::String(v * 100.0, 0) + "%"; };
    tint(dynMix, Neon::amber);

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
    k.label.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    k.label.setColour(juce::Label::textColourId, Neon::label);
    k.label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(k.label);

    k.attachment = std::make_unique<juce::SliderParameterAttachment>(
        *processor.apvts.getParameter(paramID), k.slider, nullptr);
}

void VoxStudioEditor::setupPending(Knob& k, const juce::String& text)
{
    k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    k.slider.setRange(0.0, 1.0, 0.001);
    k.slider.setValue(0.5, juce::dontSendNotification);
    k.slider.setEnabled(false);
    k.slider.setComponentID("readout");
    k.slider.textFromValueFunction = [](double) { return juce::String("--"); };
    addAndMakeVisible(k.slider);

    k.label.setText(text, juce::dontSendNotification);
    k.label.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    k.label.setColour(juce::Label::textColourId, Neon::label);
    k.label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(k.label);
}

void VoxStudioEditor::timerCallback()
{
    inLevel  = juce::jmax(processor.getInputLevel()  * 0.9f, inLevel  * 0.85f);
    outLevel = juce::jmax(processor.getOutputLevel() * 0.9f, outLevel * 0.85f);
    grLevel  = juce::jmax(processor.getGainReduction() * 0.9f, grLevel * 0.85f);

    const int activeDiv = (int) processor.apvts.getRawParameterValue("sfx_delay_div")->load();
    if ((int) std::round(delayDiv.slider.getValue()) != activeDiv)
        delayDiv.slider.setValue(activeDiv, juce::dontSendNotification);

    repaint();
}

juce::Rectangle<float> VoxStudioEditor::eqArea() const
{
    return eqAreaFor(getWidth(), getHeight());
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
    {
        if (nodePosition(b).getDistanceFrom(p) < 12.0f)
            return b;
    }
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
            if (g < bestAbs)
            {
                bestAbs = g;
                best = b;
            }
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
    if (band < 0)
        return;

    if (auto* pg = processor.apvts.getParameter("eq" + juce::String(band + 1) + "_gain"))
        pg->setValueNotifyingHost(pg->convertTo0to1(0.0f));
}

void VoxStudioEditor::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int band = nodeAt(e.position);
    if (band < 0)
        return;

    if (auto* pq = processor.apvts.getParameter("eq" + juce::String(band + 1) + "_q"))
    {
        const float q = pq->convertFrom0to1(pq->getValue());
        pq->setValueNotifyingHost(pq->convertTo0to1(juce::jlimit(0.1f, 10.0f, q + w.deltaY * 2.0f)));
    }
}

void VoxStudioEditor::paint(juce::Graphics& g)
{
    const int w = getWidth();
    const int h = getHeight();

    g.fillAll(Neon::bg);

    g.setColour(Neon::grid.withAlpha(0.4f));
    for (int x = 12; x < w; x += 24)
    {
        for (int y = 12; y < h; y += 24)
            g.fillRect(x, y, 1, 1);
    }

    g.setGradientFill(juce::ColourGradient(Neon::panel, 0.0f, 0.0f, Neon::bg, (float) w, 0.0f, false));
    g.fillRect(0.0f, 0.0f, (float) w, (float) headerH);
    g.setColour(Neon::grid);
    g.fillRect(0.0f, (float) headerH - 1.0f, (float) w, 1.0f);

    drawVoiceIcon(g, (float) sideW + pad, 34.0f);

    const float textX = (float) sideW + pad + 40.0f;
    g.setColour(Neon::violet);
    g.setFont(juce::FontOptions("Segoe UI", 24.0f, juce::Font::bold));
    g.drawText("VoxStudio", juce::Rectangle<float>(textX, 8.0f, 260.0f, 30.0f),
               juce::Justification::centredLeft, false);

    g.setColour(Neon::mint);
    g.setFont(juce::FontOptions("Segoe UI", 10.0f, juce::Font::bold));
    g.drawText("TiemFX", juce::Rectangle<float>(textX + 2.0f, 40.0f, 120.0f, 16.0f),
               juce::Justification::centredLeft, false);

    const juce::Colour accents[] = { Neon::sky, Neon::mint, Neon::pink, Neon::amber };
    const juce::String names[]   = { "COMPRESION Y DINAMICA", "EQUALIZADOR 4 BANDAS", "REVERB - DELAY", "DYNAMICS" };
    const int icons[]            = { 1, 1, 2, 0 };
    for (int i = 0; i < 4; ++i)
        drawPanel(g, panelRect(i, w, h).toFloat(), names[i], accents[i], icons[i]);

    {
        const auto p0 = panelRect(0, w, h);
        drawVuMeter(g, juce::Rectangle<float>((float) p0.getX() + 12.0f, (float) panelTop + 46.0f,
                                              (float) p0.getWidth() - 24.0f, 80.0f), grLevel);
    }

    {
        const auto gr = gateBoxRect(w, h);
        g.setColour(Neon::sky.withAlpha(0.6f));
        g.drawRoundedRectangle(gr.toFloat().reduced(0.5f), 8.0f, 1.0f);

        g.setColour(Neon::sky);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText("GATE", juce::Rectangle<int>(gr.getX() + 10, gr.getY() + 6, 80, 16),
                   juce::Justification::left, false);

        const juce::Colour ledCol = processor.getGateOpen() ? Neon::mint : Neon::grid;
        g.setColour(ledCol.withAlpha(0.3f));
        g.fillEllipse((float) gr.getRight() - 24.0f, (float) gr.getY() + 6.0f, 14.0f, 14.0f);
        g.setColour(ledCol);
        g.fillEllipse((float) gr.getRight() - 20.0f, (float) gr.getY() + 10.0f, 6.0f, 6.0f);
    }

    const auto area = eqArea();
    g.setColour(Neon::bg.withAlpha(0.9f));
    g.fillRoundedRectangle(area, 8.0f);

    const float bandW = area.getWidth() / (float) VoxStudioProcessor::numSpecBands;
    for (int b = 0; b < VoxStudioProcessor::numSpecBands; ++b)
    {
        const float v   = processor.getSpectrumBand(b);
        const float bh  = v * (area.getHeight() - 16.0f);
        const float bx  = area.getX() + b * bandW;
        g.setColour(Neon::violet.withAlpha(0.15f + 0.6f * v));
        g.fillRect(bx + 0.5f, area.getBottom() - 14.0f - bh, bandW - 1.0f, bh);
    }

    g.setColour(Neon::grid);
    g.drawRoundedRectangle(area, 8.0f, 1.0f);

    g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
    const float marks[]  = { 50.0f, 100.0f, 500.0f, 1000.0f, 5000.0f, 10000.0f };
    const char* labels[] = { "50", "100", "500", "1k", "5k", "10k" };
    for (int i = 0; i < 6; ++i)
    {
        const float x = area.getX() + area.getWidth() * std::log(marks[i] / 20.0f) / std::log(1000.0f);
        g.setColour(Neon::grid);
        g.drawVerticalLine((int) x, area.getY(), area.getBottom());
        g.setColour(Neon::label);
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
        if (i == 0)
            curve.startNewSubPath(px, y);
        else
            curve.lineTo(px, y);
    }
    g.setGradientFill(juce::ColourGradient(Neon::sky, area.getX(), 0.0f,
                                           Neon::violet, area.getRight(), 0.0f, false));
    g.strokePath(curve, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const juce::Colour nodeCol[4] = { Neon::blue, Neon::mint, Neon::amber, Neon::purple };
    for (int b = 0; b < VoxStudioProcessor::numEqBands; ++b)
    {
        const auto p = nodePosition(b);
        const float gain = processor.apvts.getRawParameterValue("eq" + juce::String(b + 1) + "_gain")->load();
        const juce::Colour c = nodeCol[b];
        const float alpha = std::abs(gain) < 0.05f ? 0.35f : 1.0f;
        g.setColour(c.withAlpha(0.25f * alpha));
        g.fillEllipse(p.x - 9.0f, p.y - 9.0f, 18.0f, 18.0f);
        g.setColour(c.withAlpha(alpha));
        g.fillEllipse(p.x - 5.0f, p.y - 5.0f, 10.0f, 10.0f);
    }

    drawSideMeter(g, { pad / 2, panelTop, sideW - pad, h - panelTop - footerH }, inLevel);
    drawSideMeter(g, { w - sideW + pad / 2, panelTop, sideW - pad, h - panelTop - footerH }, outLevel);

    drawMeter(g, footerInMeter(h), inLevel);
    drawMeter(g, footerOutMeter(w, h), outLevel);

    drawFooterIcon(g, w * 0.5f, h - 36.0f);
}

void VoxStudioEditor::resized()
{
    const int w = getWidth();
    const int h = getHeight();

    const auto c2 = panelRect(2, w, h);
    const auto c3 = panelRect(3, w, h);

    searchBox.setBounds(c2.getX(), 18, c3.getX() - c2.getX() - 10, 26);
    bypassButton.setBounds(c3.getX(), 18, c3.getWidth(), 26);

    auto p0 = innerRect(panelRect(0, w, h));
    p0.removeFromTop(88);
    placeKnob(compThreshold, p0.removeFromTop(120), 108);
    p0.removeFromBottom(gateBoxH);
    const auto compCells = cellsOf(p0, 2, 2);
    placeKnob(compRatio,   compCells[0], 56);
    placeKnob(compAttack,  compCells[1], 56);
    placeKnob(compRelease, compCells[2], 56);
    placeKnob(compMakeup,  compCells[3], 56);

    const auto gr = gateBoxRect(w, h).reduced(10, 6).withTrimmedTop(18);
    const auto gateCells = cellsOf(gr, 2, 1);
    placeKnob(dynGate,  gateCells[0], 56);
    placeKnob(gateHyst, gateCells[1], 56);

    eqPresetBox.setBounds(eqPresetRect(w, h));

    const auto bands = eqBandsArea(w, h);
    const auto bandCols = cellsOf(bands, 4, 1);
    for (int b = 0; b < VoxStudioProcessor::numEqBands; ++b)
    {
        const auto rows = cellsOf(bandCols[(size_t) b], 1, 2);
        placeKnob(eqFreq[b], rows[0], 56);
        placeKnob(eqGain[b], rows[1], 56);
    }

    auto p2 = innerRect(panelRect(2, w, h));
    placeKnob(sfxDrive, p2.removeFromTop(88), 80);
    presetBox.setBounds(p2.removeFromTop(30).reduced(4, 4));
    p2.removeFromTop(6);

    const auto revCells = cellsOf(p2.removeFromTop(80), 4, 1);
    placeKnob(revRoom,  revCells[0].reduced(2, 0), 44);
    placeKnob(revDecay, revCells[1].reduced(2, 0), 44);
    placeKnob(revMix,   revCells[2].reduced(2, 0), 44);
    placeKnob(revPre,   revCells[3].reduced(2, 0), 44);
    p2.removeFromTop(6);

    const auto modCells = cellsOf(p2.removeFromTop(58), 2, 1);
    placeKnob(modRate,  modCells[0].reduced(2, 0), 44);
    placeKnob(modDepth, modCells[1].reduced(2, 0), 44);
    p2.removeFromTop(6);

    delayPresetBox.setBounds(p2.removeFromTop(30).reduced(4, 4));
    p2.removeFromTop(6);

    const auto dlyTop = cellsOf(p2.removeFromTop(72), 2, 1);
    placeKnob(delayDiv, dlyTop[0], 56);
    placeKnob(delayFb,  dlyTop[1], 56);

    const auto dlyBot = cellsOf(p2, 3, 1);
    placeKnob(delayDrv, dlyBot[0].reduced(2, 0), 62);
    placeKnob(delayMix, dlyBot[1].reduced(2, 0), 44);
    placeKnob(delayDry, dlyBot[2].reduced(2, 0), 62);

    auto p3 = innerRect(panelRect(3, w, h));
    dynPresetBox.setBounds(p3.removeFromTop(30).reduced(4, 4));
    p3.removeFromTop(8);

    const auto row1 = cellsOf(p3.removeFromTop(112), 2, 1);
    placeKnob(dynDeess, row1[0], 80);
    placeKnob(dynFocus, row1[1], 80);
    p3.removeFromTop(8);

    const auto row2 = cellsOf(p3.removeFromTop(112), 2, 1);
    placeKnob(dynDrive, row2[0], 72);
    placeKnob(dynColor, row2[1], 72);
    p3.removeFromTop(8);

    const auto row3 = cellsOf(p3.removeFromTop(112), 2, 1);
    placeKnob(dynHpFreq, row3[0], 64);
    placeKnob(dynLpFreq, row3[1], 64);
    p3.removeFromTop(8);

    placeKnob(dynMix, p3, 56);

    inGain.slider.setBounds(pad + 4, h - footerH + 8, 52, 52);
    inGain.label.setBounds(pad + 60, h - footerH + 30, 70, 14);
    inGain.label.setJustificationType(juce::Justification::left);

    outGain.slider.setBounds(w - pad - 56, h - footerH + 8, 52, 52);
    outGain.label.setBounds(w - pad - 130, h - footerH + 30, 70, 14);
    outGain.label.setJustificationType(juce::Justification::right);
}
