// Binocular - colors, type and drawing helpers.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace bn {

// calm panels, one accent for what is switched on, color only for signals.
// Light mode is white: every surface white, set apart by thin lines only.
struct Theme {
    bool dark = true;
    juce::uint32 bg, panel, panel2, line, grid, hover, text, muted, dim, accent, accentText, accent2, accent2Text,
                 mint, blue, coral, red, warn;

    void set(bool isDark, bool colorblindSafe = false) {
        dark = isDark;
        if (dark) {
            bg = 0x121418; panel = 0x1C1F24; panel2 = 0x262A30; line = 0x30343B; grid = 0x24282E; hover = 0x30343B;
            text = 0xE8EAED; muted = 0x9AA0A8; dim = 0x686E77;
            mint = 0x8FE3B0; blue = 0x8CC8F2; coral = 0xF59A6B; red = 0xF06E6E; warn = 0xF2B36B;
        } else {
            bg = 0xFFFFFF; panel = 0xFFFFFF; panel2 = 0xFFFFFF; line = 0xDFE3E8; grid = 0xF0F2F5; hover = 0xEAF4FC;
            text = 0x1C1F24; muted = 0x626870; dim = 0x969BA2;
            mint = 0x1F9D63; blue = 0x3880C4; coral = 0xE2784A; red = 0xD64848; warn = 0xD48C28;
        }
        accent = 0x8CC8F2; accentText = 0x0F2638; accent2 = 0x2F5675; accent2Text = 0xFFFFFF;
        if (colorblindSafe) {
            // Okabe-Ito colors, safe for red-green color blindness: good = sky blue, quiet = purple, louder = yellow,
            // too loud / out of phase = orange and vermillion
            if (dark) { mint = 0x56B4E9; blue = 0xCC79A7; coral = 0xE69F00; red = 0xD55E00; warn = 0xF0E442; }
            else { mint = 0x0072B2; blue = 0xA0507F; coral = 0xC77F00; red = 0xC04A00; warn = 0x9C8200; }
        }
    }

    static juce::Colour colour(juce::uint32 rgb, float alpha = 1.0f) { return juce::Colour(0xFF000000u | rgb).withAlpha(alpha); }

    // a color between two (t = 0: the first, 1: the second)
    static juce::uint32 mix(juce::uint32 a, juce::uint32 b, double t) {
        t = juce::jlimit(0.0, 1.0, t);
        auto ch = [&](int s) { return (juce::uint32)std::floor(((a >> s) & 255) * (1 - t) + ((b >> s) & 255) * t + 0.5) << s; };
        return ch(16) | ch(8) | ch(0);
    }

    // a correlation value: from 0 up blue turns mint toward +1, below 0 coral turns red toward -1
    juce::uint32 correlation(double v) const { return v >= 0 ? mix(blue, mint, v / 0.6) : mix(coral, red, -v / 0.6); }

    // a loudness against the target: mint on target, amber and red louder, blue much quieter
    juce::uint32 loudness(double L, double target, double tol) const {
        if (L <= -70) return dim;
        if (L - target > tol + 2) return red;
        if (L - target > tol) return warn;
        if (L - target < -(tol + 6)) return blue;
        return mint;
    }
};

// Mulish for words, Red Hat Mono for numbers (both embedded); sizes are em sizes in layout units
struct Fonts {
    juce::Font title { juce::FontOptions() }, body { juce::FontOptions() }, axis { juce::FontOptions() }, valueSmall { juce::FontOptions() }, value { juce::FontOptions() }, tabOn { juce::FontOptions() }, label { juce::FontOptions() }, note { juce::FontOptions() }, small { juce::FontOptions() }, tiny { juce::FontOptions() }, timecode { juce::FontOptions() }, knobValue { juce::FontOptions() }, helpTitle { juce::FontOptions() }, help { juce::FontOptions() }, meter { juce::FontOptions() }, valueBig { juce::FontOptions() }, valueMid { juce::FontOptions() }, valueTiny { juce::FontOptions() };
    void set(float usc);
    static juce::Font make(const char* face, float em);
};

// drawing in pixels with the theme's colors
struct Canvas {
    juce::Graphics& g;
    float hair;                                  // one physical pixel

    void colour(juce::uint32 rgb, float a = 1.0f) { g.setColour(Theme::colour(rgb, a)); }
    void rect(float x, float y, float w, float h) { if (w > 0 && h > 0) g.fillRect(x, y, w, h); }
    void hline(float x, float y, float w) { rect(x, y, w, hair); }
    void vline(float x, float y, float h) { rect(x, y, hair, h); }
    void box(float x, float y, float w, float h, float r) { if (w > 0 && h > 0) g.fillRoundedRectangle(x, y, w, h, juce::jmin(r, w / 2, h / 2)); }
    void frame(float x, float y, float w, float h, float r) {
        const float t = hair;
        g.drawRoundedRectangle(x + t / 2, y + t / 2, w - t, h - t, juce::jmin(r, w / 2, h / 2), t);
    }
    void disc(float cx, float cy, float r) { g.fillEllipse(cx - r, cy - r, 2 * r, 2 * r); }
    void ring(float cx, float cy, float r, float width) { g.drawEllipse(cx - r, cy - r, 2 * r, 2 * r, width); }
    // an arc: angles in radians, 0 at the top, clockwise
    void arc(float cx, float cy, float r, float a1, float a2, float width) {
        juce::Path p;
        p.addCentredArc(cx, cy, r, r, 0, a1, a2, true);
        g.strokePath(p, juce::PathStrokeType(width, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
    }
    void line(float x1, float y1, float x2, float y2, float width) { g.drawLine(x1, y1, x2, y2, width); }
    void polygon(std::initializer_list<float> xy) {
        juce::Path p;
        auto it = xy.begin();
        p.startNewSubPath(it[0], it[1]);
        for (size_t i = 2; i + 1 < xy.size(); i += 2) p.lineTo(it[i], it[i + 1]);
        p.closeSubPath();
        g.fillPath(p);
    }

    // text, vertically centered on cy: centered on cx, from x, or ending at rx
    static float width(const juce::Font& f, const juce::String& s) { return juce::GlyphArrangement::getStringWidth(f, s); }
    void text(const juce::Font& f, const juce::String& s, float x, float cy, juce::Justification j) {
        g.setFont(f);
        const float w = width(f, s) + 4, h = f.getHeight();
        const float left = j == juce::Justification::centred ? x - w / 2 : j == juce::Justification::right ? x - w : x;
        g.drawText(s, juce::Rectangle<float>(left, cy - h / 2, w, h),
                   j == juce::Justification::centred ? juce::Justification::centred : j == juce::Justification::right ? juce::Justification::centredRight : juce::Justification::centredLeft, false);
    }
    void textC(const juce::Font& f, const juce::String& s, float cx, float cy) { text(f, s, cx, cy, juce::Justification::centred); }
    void textL(const juce::Font& f, const juce::String& s, float x, float cy) { text(f, s, x, cy, juce::Justification::left); }
    void textR(const juce::Font& f, const juce::String& s, float rx, float cy) { text(f, s, rx, cy, juce::Justification::right); }
};

} // namespace bn
