/*
 * Granitos saltarines - native GUI (DPF / DGL, Cairo backend, X11UI)
 *
 * Top: the delay buffer, with the read position, one dot per sounding grain and the bouncing ball.
 * Bottom: the controls in three columns (Delay, Grains, Hop), four per column, as on zynthian.
 * Controls: drag or click a bar, scroll to nudge, click the Freeze cell to toggle.
 *
 * The GUI only displays and edits parameters: the plugin works without it.
 *
 * Copyright (C) 2026 Jofemodo
 * License: GPL-3.0-or-later
 */

#include "DistrhoUI.hpp"
#include "ParamDefs.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

START_NAMESPACE_DISTRHO

using DGL_NAMESPACE::CairoGraphicsContext;

namespace {
constexpr uint32_t kColumns = 3;         // one column per control page
constexpr uint32_t kRowsPerColumn = 4;   // 4 controls per page
constexpr uint32_t kMaxGrainDots = 64;   // must match kMaxGrains in the DSP
constexpr double kViewFraction = 0.46;   // top part of the window: buffer view
constexpr double kMargin = 10.0;
}

class GranitosSaltarinesUI : public UI
{
public:
    GranitosSaltarinesUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT),
          fDragging(-1)
    {
        for (uint32_t i = 0; i < kParamCount; ++i)
            fValues[i] = kParams[i].def;
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH / 2, DISTRHO_UI_DEFAULT_HEIGHT / 2, true);
    }

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // Host -> UI (inputs and outputs)

    void parameterChanged(const uint32_t index, const float value) override
    {
        if (index >= kParamCount || fValues[index] == value)
            return;
        fValues[index] = value;
        repaint();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Drawing

    void onCairoDisplay(const CairoGraphicsContext& context) override
    {
        cairo_t* const cr = context.handle;
        const double w = getWidth();
        const double h = getHeight();

        cairo_set_source_rgb(cr, 0.08, 0.09, 0.11);
        cairo_paint(cr);

        drawBufferView(cr, w, h);
        drawControls(cr, w, h);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Input

    bool onMouse(const MouseEvent& ev) override
    {
        if (ev.button != 1)
            return false;
        if (! ev.press)
        {
            if (fDragging < 0)
                return false;
            editParameter(fDragging, false);
            fDragging = -1;
            return true;
        }
        const int index = controlAt(ev.pos.getX(), ev.pos.getY());
        if (index < 0)
            return false;
        const ParamDef& p(kParams[index]);
        if ((p.hints & kParameterIsBoolean) != 0)
        {
            editParameter(index, true);
            setValue(index, fValues[index] > (p.min + p.max) / 2 ? p.min : p.max);
            editParameter(index, false);
            return true;
        }
        fDragging = index;
        editParameter(index, true);
        setFromX(index, ev.pos.getX());
        return true;
    }

    bool onMotion(const MotionEvent& ev) override
    {
        if (fDragging < 0)
            return false;
        setFromX(fDragging, ev.pos.getX());
        return true;
    }

    bool onScroll(const ScrollEvent& ev) override
    {
        const int index = controlAt(ev.pos.getX(), ev.pos.getY());
        if (index < 0)
            return false;
        const ParamDef& p(kParams[index]);
        const bool stepped = (p.hints & (kParameterIsInteger | kParameterIsBoolean)) != 0 || p.enumCount > 0;
        const float step = stepped ? 1.0f : (p.max - p.min) / 100.0f;
        editParameter(index, true);
        setValue(index, fValues[index] + (ev.delta.getY() > 0 ? step : -step));
        editParameter(index, false);
        return true;
    }

private:
    float fValues[kParamCount];
    int fDragging;

    // ----------------------------------------------------------------------------------------------------------------
    // Layout

    double bodyTop(const double h) const
    {
        return h * kViewFraction;
    }

    // Control i sits in column i / 4 (its page), row i % 4
    void cellRect(const uint32_t i, const double w, const double h,
                  double& x, double& y, double& cw, double& ch) const
    {
        cw = w / kColumns;
        ch = (h - bodyTop(h)) / kRowsPerColumn;
        x = (i / kRowsPerColumn) * cw;
        y = bodyTop(h) + (i % kRowsPerColumn) * ch;
    }

    // Index of the control under (x, y), or -1
    int controlAt(const double x, const double y) const
    {
        const double w = getWidth(), h = getHeight();
        if (y < bodyTop(h) || x < 0.0 || x >= w)
            return -1;
        const uint32_t col = static_cast<uint32_t>(x / (w / kColumns));
        const uint32_t row = static_cast<uint32_t>((y - bodyTop(h)) / ((h - bodyTop(h)) / kRowsPerColumn));
        const uint32_t i = col * kRowsPerColumn + std::min(row, kRowsPerColumn - 1);
        if (i >= kParamCount || (kParams[i].hints & kParameterIsOutput) != 0)
            return -1;
        return static_cast<int>(i);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Buffer view (top)

    void drawBufferView(cairo_t* const cr, const double w, const double h)
    {
        const double stripTop = h * 0.30, stripBot = h * 0.40;
        const double r = h * 0.03;
        const double maxHop = h * 0.18;

        // Title and grain count
        cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, h * 0.07);
        cairo_set_source_rgb(cr, 0.92, 0.92, 0.92);
        cairo_move_to(cr, kMargin, h * 0.09);
        cairo_show_text(cr, "Granitos saltarines");

        cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        char text[32];
        const int count = std::min(static_cast<int>(kMaxGrainDots), static_cast<int>(std::lround(fValues[kParamGrainCount])));
        std::snprintf(text, sizeof(text), "%d grains", count);
        drawTextRight(cr, text, w - kMargin, h * 0.09);

        // The buffer strip
        cairo_set_source_rgb(cr, 0.20, 0.21, 0.24);
        cairo_rectangle(cr, kMargin, stripTop, w - 2 * kMargin, stripBot - stripTop);
        cairo_fill(cr);

        // Read position marker
        const double readX = kMargin + std::fmin(1.0, std::fmax(0.0, fValues[kParamReadPos])) * (w - 2 * kMargin);
        cairo_set_source_rgb(cr, 0.20, 0.75, 0.35);
        cairo_set_line_width(cr, 2.0);
        cairo_move_to(cr, readX, stripTop);
        cairo_line_to(cr, readX, stripBot);
        cairo_stroke(cr);

        // Grain dots: spread around the read point, one per sounding grain
        const double spread = w * 0.12;
        const double stripH = stripBot - stripTop;
        const double gr = std::fmax(2.0, h * 0.02);
        cairo_set_source_rgb(cr, 0.20, 0.75, 0.35);
        for (int i = 0; i < count; ++i)
        {
            const double dx = (std::fmod(i * 0.618034, 1.0) - 0.5) * spread;
            const double dy = std::fmod(i * 0.381966, 1.0) * stripH;
            cairo_arc(cr, readX + dx, stripTop + dy, gr, 0.0, 2.0 * M_PI);
            cairo_fill(cr);
        }

        // The bouncing ball: its height follows hop_height
        const double hopH = std::fmin(1.0, std::fmax(0.0, fValues[kParamHopHeight])) * maxHop;
        const double ballY = stripTop - r - 2.0 - hopH;
        cairo_set_source_rgb(cr, 0.95, 0.55, 0.15);
        cairo_arc(cr, readX, ballY, r, 0.0, 2.0 * M_PI);
        cairo_fill(cr);

    }

    // ----------------------------------------------------------------------------------------------------------------
    // Controls (bottom)

    void drawControls(cairo_t* const cr, const double w, const double h)
    {
        cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        const double fontSize = std::fmin(h * 0.075, (h - bodyTop(h)) / kRowsPerColumn * 0.42);
        cairo_set_font_size(cr, fontSize);

        for (uint32_t i = 0; i < kParamCount; ++i)
        {
            if ((kParams[i].hints & kParameterIsOutput) != 0)
                continue;
            double x, y, cw, ch;
            cellRect(i, w, h, x, y, cw, ch);
            const ParamDef& p(kParams[i]);

            // Name (left) and value (right)
            const double baseline = y + ch * 0.42;
            cairo_set_source_rgb(cr, 0.80, 0.80, 0.80);
            cairo_move_to(cr, x + kMargin, baseline);
            cairo_show_text(cr, p.name);

            char text[48];
            formatValue(i, text, sizeof(text));
            cairo_set_source_rgb(cr, 0.92, 0.92, 0.92);
            drawTextRight(cr, text, x + cw - kMargin, baseline);

            // Bar
            const double bx0 = x + kMargin, bx1 = x + cw - kMargin;
            const double by = y + ch * 0.55, bh = ch * 0.25;
            cairo_set_source_rgb(cr, 0.20, 0.21, 0.24);
            cairo_rectangle(cr, bx0, by, bx1 - bx0, bh);
            cairo_fill(cr);
            cairo_set_source_rgb(cr, 0.95, 0.55, 0.15);
            cairo_rectangle(cr, bx0, by, (bx1 - bx0) * normalized(i, fValues[i]), bh);
            cairo_fill(cr);
        }
    }

    void drawTextRight(cairo_t* const cr, const char* const text, const double right, const double baseline) const
    {
        cairo_text_extents_t ext;
        cairo_text_extents(cr, text, &ext);
        cairo_move_to(cr, right - ext.x_advance, baseline);
        cairo_show_text(cr, text);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Value helpers

    bool isLog(const uint32_t index) const
    {
        const ParamDef& p(kParams[index]);
        return (p.hints & kParameterIsLogarithmic) != 0 && p.min > 0.0f;
    }

    double normalized(const uint32_t index, const float value) const
    {
        const ParamDef& p(kParams[index]);
        double n = isLog(index) ? std::log(value / p.min) / std::log(p.max / p.min)
                                : (value - p.min) / (p.max - p.min);
        return std::fmin(1.0, std::fmax(0.0, n));
    }

    void setFromX(const uint32_t index, const double x)
    {
        const ParamDef& p(kParams[index]);
        const double w = getWidth(), h = getHeight();
        double cx, cy, cw, ch;
        cellRect(index, w, h, cx, cy, cw, ch);
        const double bx0 = cx + kMargin, bx1 = cx + cw - kMargin;
        const double n = std::fmin(1.0, std::fmax(0.0, (x - bx0) / (bx1 - bx0)));
        const double v = isLog(index) ? p.min * std::pow(p.max / p.min, n) : p.min + n * (p.max - p.min);
        setValue(index, static_cast<float>(v));
    }

    void setValue(const uint32_t index, float value)
    {
        const ParamDef& p(kParams[index]);
        value = std::fmax(p.min, std::fmin(p.max, value));
        if ((p.hints & (kParameterIsInteger | kParameterIsBoolean)) != 0 || p.enumCount > 0)
            value = std::round(value);
        if (value == fValues[index])
            return;
        fValues[index] = value;
        setParameterValue(index, value);
        repaint();
    }

    void formatValue(const uint32_t index, char* const text, const size_t size) const
    {
        const ParamDef& p(kParams[index]);
        const float v = fValues[index];
        if (p.enumCount > 0)
        {
            const int e = static_cast<int>(std::round(v - p.min));
            std::snprintf(text, size, "%s", e >= 0 && e < static_cast<int>(p.enumCount) ? p.enumLabels[e] : "?");
        }
        else if ((p.hints & kParameterIsBoolean) != 0)
            std::snprintf(text, size, "%s", v > (p.min + p.max) / 2 ? "On" : "Off");
        else if ((p.hints & kParameterIsInteger) != 0)
            std::snprintf(text, size, "%d %s", static_cast<int>(std::round(v)), p.unit);
        else
            std::snprintf(text, size, "%.1f %s", v, p.unit);
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranitosSaltarinesUI)
};

UI* createUI()
{
    return new GranitosSaltarinesUI();
}

END_NAMESPACE_DISTRHO
