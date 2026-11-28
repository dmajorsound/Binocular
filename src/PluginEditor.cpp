#include "PluginEditor.h"
#include "Mark.h"
#include "UpdateCheck.h"
#include "BinaryData.h"
#include <cstring>
#include <map>

using bn::Analyzer;
using bn::Theme;
using juce::String;

static const char* kVersionText = "V " JucePlugin_VersionString " - developed with AI assistance";
static const char* kManualUrl = "https://dmajorsound.com/software/binocular/manual.html";

// ---------------------------------------------------------------- type
juce::Font bn::Fonts::make(const char* face, float em) {
    struct Face { const char* name; const char* data; int size; float heightScale; };
    static const Face faces[] = {
        { "Mulish", BinaryData::MulishRegular_ttf, BinaryData::MulishRegular_ttfSize, 1.255f },
        { "Mulish SemiBold", BinaryData::MulishSemiBold_ttf, BinaryData::MulishSemiBold_ttfSize, 1.255f },
        { "Mulish Bold", BinaryData::MulishBold_ttf, BinaryData::MulishBold_ttfSize, 1.255f },
        { "Red Hat Mono", BinaryData::RedHatMonoMedium_ttf, BinaryData::RedHatMonoMedium_ttfSize, 1.32f },
        { "Red Hat Mono SemiBold", BinaryData::RedHatMonoSemiBold_ttf, BinaryData::RedHatMonoSemiBold_ttfSize, 1.32f },
    };
    static std::map<std::string, juce::Typeface::Ptr> loaded;
    for (const auto& f : faces) {
        if (std::strcmp(f.name, face) != 0) continue;
        auto& tf = loaded[f.name];
        if (tf == nullptr) tf = juce::Typeface::createSystemTypefaceFor(f.data, (size_t)f.size);
        // JUCE sizes a font by ascent + descent; the sizes here are em sizes
        return juce::Font(juce::FontOptions(tf).withHeight(juce::jmax(1.0f, em * f.heightScale)));
    }
    return juce::Font(juce::FontOptions(em));
}

void bn::Fonts::set(float usc) {
    title = make("Mulish Bold", 17 * usc);
    body = make("Mulish", 9.5f * usc);
    axis = make("Red Hat Mono", 7.5f * usc);
    valueSmall = make("Red Hat Mono SemiBold", 10 * usc);
    value = make("Red Hat Mono SemiBold", 13 * usc);
    tabOn = make("Mulish SemiBold", 9.5f * usc);
    label = make("Mulish SemiBold", 9 * usc);
    note = make("Mulish", 8.5f * usc);
    small = make("Mulish", 9 * usc);
    tiny = make("Mulish", 8 * usc);
    timecode = make("Red Hat Mono", 9.5f * usc);
    knobValue = make("Red Hat Mono", 9.5f * usc);
    helpTitle = make("Mulish Bold", 14 * usc);
    help = make("Mulish SemiBold", 11 * usc);
    meter = make("Red Hat Mono", 7 * usc);
    valueBig = make("Red Hat Mono SemiBold", 16 * usc);
    valueMid = make("Red Hat Mono SemiBold", 12 * usc);
    valueTiny = make("Red Hat Mono SemiBold", 9 * usc);
}

// ---------------------------------------------------------------- small helpers
static String signedValue(double v) {
    if (std::fabs(v) < 0.005) return "0.00";
    return (v > 0 ? "+" : "-") + String(std::fabs(v), 2);
}

static String frequency(double f) {
    if (f < 100) return String(f, 1) + " Hz";
    if (f < 1000) return String((int)std::floor(f + 0.5)) + " Hz";
    return String(f / 1000, 2) + " kHz";
}

static String oneDecimal(double v) { return String(v, 1); }

// loudness targets (parameter value = index; new ones were added at the end so saved projects keep theirs)
struct Target { const char* name; double lufs, tol; int peak; bool dialog, streaming, film; const char* note; };
static const Target kTargets[10] = {
    { "EBU R128",           -23, 0.5, -1, false, false, false, "-23 LUFS \xc2\xb1" "0.5 LU  \xc2\xb7  true peak -1 dBTP" },
    { "Netflix",            -27, 2,   -2, true,  false, true,  "-27 LKFS dialog-gated \xc2\xb1" "2 LU  \xc2\xb7  true peak -2 dBTP" },
    { "Spotify",            -14, 1,   -1, false, true,  false, "Plays at -14 LUFS  \xc2\xb7  true peak -1 dBTP" },
    { "YouTube",            -14, 1,   -1, false, true,  false, "Plays at -14 LUFS  \xc2\xb7  true peak -1 dBTP" },
    { "Apple Music",        -16, 1,   -1, false, true,  false, "Plays at -16 LUFS  \xc2\xb7  true peak -1 dBTP" },
    { "Tidal",              -14, 1,   -1, false, true,  false, "Plays at -14 LUFS  \xc2\xb7  true peak -1 dBTP" },
    { "Amazon Prime Video", -24, 2,   -2, true,  false, true,  "-24 LKFS dialog-gated \xc2\xb1" "2 LU (ATSC A/85)  \xc2\xb7  true peak -2 dBTP" },
    { "ATSC A/85",          -24, 2,   -2, true,  false, false, "-24 LKFS \xc2\xb1" "2 LU, dialog for long-form  \xc2\xb7  true peak -2 dBTP" },
    { "ARIB TR-B32",        -24, 1,   -1, false, false, false, "Japan: -24 LKFS \xc2\xb1" "1 LU  \xc2\xb7  true peak -1 dBTP" },
    { "OP-59",              -24, 1,   -2, false, false, false, "Australia: -24 LKFS \xc2\xb1" "1 LU  \xc2\xb7  true peak -2 dBTP" },
};
static const int kTargetOrder[10] = { 0, 7, 8, 9, 1, 6, 2, 3, 4, 5 };   // the list: broadcast, film streaming, music streaming
static const char* kFpsNames[6] = { "23.976", "24", "25", "29.97 DF", "29.97", "30" };
static const char* kDownmixNames[3] = { "ITU-R BS.775", "Modern", "Manual" };
static const char* kStartNames[3] = { "00:00:00:00", "01:00:00:00", "10:00:00:00" };

// the DAW position as timecode at the chosen rate (29.97 drop frame counted as SMPTE 12M), plus a start hour
static String timecode(double secs, int fps, int start) {
    const bool neg = secs < 0;
    secs = std::fabs(secs);
    static const double rate[6] = { 24000.0 / 1001, 24, 25, 30000.0 / 1001, 30000.0 / 1001, 30 };
    static const int base[6] = { 24, 24, 25, 30, 30, 30 };
    long long f = (long long)std::floor(secs * rate[fps] + 0.0001);
    if (fps == 3) {   // drop frame: frame numbers 0 and 1 are skipped every minute except every tenth
        const long long d = f / 17982, m = f % 17982;
        f += 18 * d + (m > 1 ? 2 * ((m - 2) / 1798) : 0);
    }
    const int b = base[fps];
    const int hh = (int)(f / (3600LL * b)) + (start == 1 ? 1 : start == 2 ? 10 : 0);
    return String(neg ? "-" : "") + String::formatted(fps == 3 ? "%02d:%02d:%02d;%02d" : "%02d:%02d:%02d:%02d",
                                                       hh % 24, (int)(f / (60LL * b) % 60), (int)(f / b % 60), (int)(f % b));
}
static const char* kSpanNames[7] = { "1 min", "5 min", "15 min", "30 min", "1 h", "2 h", "5 h" };
static const char* kMeasureNames[3] = { "Always", "Playback", "Timecode" };
static const char* kPairNames[3] = { "Front", "Surround", "Downmix" };
static const char* kPairFirst[3] = { "L", "Ls", "Lo" };
static const char* kPairSecond[3] = { "R", "Rs", "Ro" };

// ---------------------------------------------------------------- the window
BinocularEditor::BinocularEditor(BinocularProcessor& p) : AudioProcessorEditor(&p), proc(p), an(p.analyzer) {
    setOpaque(true);
    setWantsKeyboardFocus(true);
    setResizable(true, true);
    setResizeLimits(340, 260, 4000, 3000);
    setSize(juce::jlimit(340, 4000, p.editorW.load()), juce::jlimit(260, 3000, p.editorH.load()));   // 680 x 520, or the last size
    sizeReady = true;
    bn::updateCheckStart();   // once per host session, in the background
    startTimerHz(60);
}

BinocularEditor::~BinocularEditor() { stopTimer(); }

void BinocularEditor::resized() {
    if (sizeReady) { proc.editorW = proc.sessionW = getWidth(); proc.editorH = proc.sessionH = getHeight(); }   // remembered for next time
}

void BinocularEditor::timerCallback() {
    bn::updatePoll();   // "download and install": opens the installer once it is there
    repaint();
}

int BinocularEditor::hitAt(juce::Point<float> p) const {
    int id = 0;
    for (const auto& h : lastHits) {
        const bool in = h.round ? p.getDistanceFrom(h.r.getCentre()) <= h.r.getWidth() / 2 : h.r.contains(p);
        if (in) id = h.id;   // what is drawn later lies on top
    }
    return id;
}

void BinocularEditor::paint(juce::Graphics& g) {
    const double now = juce::Time::getMillisecondCounterHiRes();
    frameDt = lastFrame > 0 ? juce::jlimit(1.0 / 120, 0.1, (now - lastFrame) / 1000.0) : 1.0 / 60;
    lastFrame = now;
    th.set(proc.get(bn::pTheme) < 0.5, proc.get(bn::pColors) >= 0.5);
    const float w = (float)getWidth(), h = (float)getHeight();
    usc = juce::jmax(0.5f, juce::jmin(w / 680, h / 520));
    ex = juce::jmax(0.0f, h / usc - 520);
    wx = juce::jmax(0.0f, std::floor(w / usc - 680));
    ox = std::floor((w - (680 + wx) * usc) / 2);
    fonts.set(usc);
    altDown = juce::ModifierKeys::currentModifiers.isAltDown();
    hover = hitAt(mousePos);
    if (dragId) hover = dragId == hSelectivity ? (int)hBands : dragId;   // a knob being dragged stays lit
    if (listOpen >= 1 && listOpen <= 4 && proc.get(bn::pView) < 2.5) listOpen = 0;
    if (listOpen == 5 && !downmixShown()) listOpen = 0;
    hits.clear();

    const float scale = juce::jmax(1.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    bn::Canvas c { g, 1.0f / scale };
    c.colour(th.bg); g.fillAll();
    drawHeader(c);

    // the display panel
    c.colour(th.panel); c.box(ux(16), uy(52), (648 + wx) * usc, (286 + ex) * usc, 10 * usc);
    c.colour(th.line); c.frame(ux(16), uy(52), (648 + wx) * usc, (286 + ex) * usc, 10 * usc);
    const int view = (int)std::floor(proc.get(bn::pView) + 0.5);
    if (view <= 1) drawBands(c);
    else if (view == 2) { if (an.channels == 6) drawVectorscope(c); else drawGoniometer(c); }
    else drawLoudness(c);

    // switched off: a small label at the top of the display
    if (proc.get(bn::pFreeze) >= 0.5) {
        const float px = view == 3 ? ux(86) : view == 2 ? ux(26) : ux(16) + (648 + wx) * usc / 2 - 32 * usc;
        c.colour(th.accent); c.box(px, uy(62), 64 * usc, 18 * usc, 9 * usc);
        c.colour(th.accentText); c.textC(fonts.tabOn, "Paused", px + 32 * usc, uy(71));
    }
    drawTotal(c);
    drawControls(c);
    drawList(c);
    drawHelp(c);
    drawFooter(c);
    drawStatus(c);
    lastHits = hits;
}

// ---------------------------------------------------------------- header
void BinocularEditor::drawHeader(bn::Canvas& c) {
    const bool frozen = proc.get(bn::pFreeze) >= 0.5;
    // on / off: a round power button. On (blue) = the meter runs. Off = it keeps the last picture; the audio always passes.
    const float px = ux(28), py = uy(26), pr = 11 * usc;
    addHitCircle(hPower, px, py, pr + 3 * usc);
    c.colour(!frozen ? th.accent : (hover == hPower ? th.text : th.dim));
    c.ring(px, py, pr, 1.6f * usc);
    c.arc(px, py, 6 * usc, 0.55f, 5.73f, 1.6f * usc);
    c.line(px, py - 7.5f * usc, px, py - 1 * usc, 1.6f * usc);
    // the Binocular mark: two rings, blue and mint like the meter
    c.colour(th.blue); c.ring(ux(54), uy(26), 7.5f * usc, 1.8f * usc);
    c.colour(th.mint); c.ring(ux(63), uy(26), 7.5f * usc, 1.8f * usc);
    c.colour(th.text); c.textL(fonts.title, "Binocular", ux(78), uy(3.9f) + fonts.title.getHeight() / 2);
    c.colour(th.muted); c.textL(fonts.body, String::fromUTF8("Mmm\xe2\x80\xa6" "etering!"), ux(79), uy(26.45f) + fonts.body.getHeight() / 2);

    // views: one segmented control
    const int view = (int)std::floor(proc.get(bn::pView) + 0.5);
    c.colour(th.panel); c.box(ux(190), uy(12), 294 * usc, 28 * usc, 14 * usc);
    c.colour(th.line); c.frame(ux(190), uy(12), 294 * usc, 28 * usc, 14 * usc);
    static const char* tabs[4] = { "Correlation", "Mono loss", "Goniometer", "Loudness" };
    static const int tabIds[4] = { hTabCorr, hTabMono, hTabGonio, hTabLoud };
    for (int k = 0; k < 4; k++) {
        const float sx = ux(193 + k * 72.0f);
        addHit(tabIds[k], sx, uy(15), 72 * usc, 22 * usc);
        if (view == k) {
            c.colour(th.accent); c.box(sx, uy(15), 72 * usc, 22 * usc, 11 * usc);
            c.colour(th.accentText); c.textC(fonts.tabOn, tabs[k], sx + 36 * usc, uy(26.5f));
        } else {
            if (hover == tabIds[k]) { c.colour(th.hover); c.box(sx, uy(15), 72 * usc, 22 * usc, 11 * usc); }
            c.colour(hover == tabIds[k] ? th.text : th.muted); c.textC(fonts.body, tabs[k], sx + 36 * usc, uy(26.5f));
        }
    }

    const float wp = wx * usc;
    {
        // the format Binocular found on this track, as a small tag after the name
        const String tag = an.channels == 6 ? "5.1" : "Stereo";
        // (after the tagline, so it never meets the tabs)
        const float tagX = ux(79) + bn::Canvas::width(fonts.body, String::fromUTF8("Mmm\xe2\x80\xa6" "etering!")) + 7 * usc;
        const float tagW = bn::Canvas::width(fonts.tiny, tag) + 12 * usc, tagY = uy(26.45f) + fonts.body.getHeight() / 2;
        c.colour(th.accent2); c.box(tagX, tagY - 6.5f * usc, tagW, 13 * usc, 6.5f * usc);
        c.colour(th.accent2Text); c.textC(fonts.tiny, tag, tagX + tagW / 2, tagY);
        addHit(hSurroundTag, tagX, tagY - 6.5f * usc, tagW, 13 * usc);
    }
    if (an.channels == 6 && view != 2) {
        // 5.1: what the correlation pages compare: front, surround or the downmix (the goniometer page shows the vectorscope)
        const int pair = (int)std::floor(proc.get(bn::pPair) + 0.5);
        c.colour(th.panel); c.box(ux(488) + wp, uy(12), 120 * usc, 28 * usc, 14 * usc);
        c.colour(th.line); c.frame(ux(488) + wp, uy(12), 120 * usc, 28 * usc, 14 * usc);
        static const float segX[3] = { 491, 525, 557 }, segW[3] = { 34, 32, 48 };
        static const char* segText[3] = { "Front", "Surr.", "Downmix" };
        for (int k = 0; k < 3; k++) {
            const float sx = ux(segX[k]) + wp, sw = segW[k] * usc;
            const int id = hPair0 + k;
            addHit(id, sx, uy(15), sw, 22 * usc);
            if (pair == k) {
                c.colour(th.accent2); c.box(sx, uy(15), sw, 22 * usc, 11 * usc);
                c.colour(th.accent2Text); c.textC(fonts.label, segText[k], sx + sw / 2, uy(26.5f));
            } else {
                if (hover == id) { c.colour(th.hover); c.box(sx, uy(15), sw, 22 * usc, 11 * usc); }
                c.colour(hover == id ? th.text : th.muted); c.textC(fonts.small, segText[k], sx + sw / 2, uy(26.5f));
            }
        }
    }
    // help: a round "?"
    const float qx = ux(627) + wp, qy = uy(26);
    addHitCircle(hHelp, qx, qy, 12 * usc);
    c.colour(helpOpen ? th.accent : (hover == hHelp ? th.hover : th.panel)); c.disc(qx, qy, 11 * usc);
    if (!helpOpen) { c.colour(th.line); c.ring(qx, qy, 11 * usc, c.hair); }
    c.colour(helpOpen ? th.accentText : (hover == hHelp ? th.text : th.muted)); c.textC(fonts.help, "?", qx + 0.5f * usc, qy);

    // theme: dark = a crescent moon, light = a sun
    const float tx = ux(656) + wp, ty = uy(26);
    addHitCircle(hTheme, tx, ty, 12 * usc);
    c.colour(hover == hTheme ? th.text : th.muted);
    if (th.dark) {
        c.disc(tx, ty, 8 * usc);
        c.colour(th.bg); c.disc(tx + 4 * usc, ty - 3 * usc, 8 * usc);
    } else {
        c.ring(tx, ty, 4.2f * usc, 1.4f * usc);
        for (int i = 0; i < 8; i++) {
            const float a = i * 0.7854f;
            c.line(tx + std::sin(a) * 6.6f * usc, ty - std::cos(a) * 6.6f * usc, tx + std::sin(a) * 9 * usc, ty - std::cos(a) * 9 * usc, 1.4f * usc);
        }
    }
}

// ---------------------------------------------------------------- correlation / mono loss: +1 top, -1 bottom, or 0 dB top down to the range
void BinocularEditor::drawBands(bn::Canvas& c) {
    const bool mono = proc.get(bn::pView) >= 0.5;
    const float gx = ux(46), gw = (608 + wx) * usc, gy = uy(62), gh = (250 + ex) * usc;
    const bool dmx = downmixShown();
    const float top = mono || dmx ? 26 * usc : 0;   // room on top for the range button and the downmix
    const double rangeDb = proc.get(bn::pRange) < 0.5 ? 24 : proc.get(bn::pRange) < 1.5 ? 48 : 96;
    auto map = [&](double v) {
        return mono ? gy + top + (gh - top) * (float)juce::jlimit(0.0, 1.0, -v / rangeDb) : gy + top + (gh - top) * (float)(1 - v) / 2;
    };
    // a faint red ground where the mono sum loses level: below 0 / below -3 dB
    const float redTop = mono ? map(-3) : map(0);
    c.colour(th.red, th.dark ? 0.035f : 0.03f); c.rect(gx, redTop, gw, gy + gh - redTop);
    // grid and scale: five marks. Mono loss 24 dB: 0 -3 -6 -12 -24, 48 dB: 0 -6 -12 -24 -48, 96 dB: 0 -12 -24 -48 -96
    for (int k = 0; k < 5; k++) {
        double v; String lab;
        if (mono) {
            v = k == 0 ? 0 : -(rangeDb / 8) * std::pow(2.0, k - 1);
            lab = v == 0 ? String("0 dB") : String((int)v);
            c.colour(v == -3 ? th.line : th.grid);
        } else {
            v = 1 - k * 0.5;
            static const char* labs[5] = { "+1", "+0.5", "0", "-0.5", "-1" };
            lab = labs[k];
            c.colour(k == 2 ? th.line : th.grid);
        }
        const float y = map(v);
        c.hline(gx, y, gw);
        c.colour(th.dim); c.textR(fonts.axis, lab, ux(40), y);
    }
    // frequency grid
    const double lfr = std::log(an.highHz / an.lowHz);
    static const int freqs[10] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    for (int f : freqs) {
        if (f > an.highHz * 1.001) continue;
        const float x = gx + gw * (float)(std::log(f / an.lowHz) / lfr);
        c.colour(th.grid); c.vline(x, gy, gh);
        c.colour(th.dim); c.textC(fonts.axis, f < 1000 ? String(f) : String(f / 1000) + "k", x, gy + gh + 12 * usc);
    }

    // bars: color follows the correlation, height follows the view
    const int nb = an.bands;
    const float xw = gw / (float)juce::jmax(1, nb);
    const float bw = xw >= 5 * usc ? juce::jmax(1.0f, xw - 2 * usc) : juce::jmax(1.0f, xw * 0.72f);
    const float br = bw >= 4 * usc ? 1.5f * usc : 0;
    const float y0 = map(0);
    const bool hold = proc.get(bn::pHold) >= 0.5;
    hoverBand = -1;
    addHit(hBandsArea, gx, gy, gw, gh);
    if (hover == hBandsArea && juce::Rectangle<float>(gx, gy, gw, gh).contains(mousePos)) {
        hoverBand = juce::jlimit(0, nb - 1, (int)std::floor((mousePos.x - gx) / xw));
        c.colour(th.line, 0.5f); c.rect(gx + hoverBand * xw, gy, xw, gh);
    }
    const float holdH = juce::jmax(c.hair, 1.5f * usc);
    if (nb <= 128) {
        for (int i = 0; i < nb; i++) {
            if (!an.hasSignal[(size_t)i]) continue;
            const float bx = gx + (i + 0.5f) * xw - bw / 2;
            const double cv = an.corr[(size_t)i];
            const double v = mono ? an.monoLoss[(size_t)i] : cv;
            const double vh = mono ? an.monoHeld[(size_t)i] : an.corrHeld[(size_t)i];
            const double a = juce::jlimit(0.3, 1.0, (an.level[(size_t)i] + 80) / 50);   // quiet bands fade into the panel
            const float yv = map(v);
            const float bh = juce::jmax(c.hair, std::fabs(yv - y0));
            c.colour(Theme::mix(th.panel, th.correlation(cv), a));
            c.box(bx, juce::jmin(yv, y0), bw, bh, bh >= 2 * br + 2 ? br : 0);   // very short bars stay square
            // the lowest reading held: a short line in text color
            if (hold && vh < v - (mono ? 0.5 : 0.02)) { c.colour(th.text, 0.85f); c.rect(bx, map(vh), bw, holdH); }
        }
    } else {
        // more bands than room for bars: one pixel column each, the worst band of a column is shown
        const int cols = juce::jmax(1, (int)std::floor(gw / c.hair));
        for (int px = 0; px < cols; px++) {
            const int b0 = (int)std::floor((double)px * nb / cols), b1 = juce::jmax(b0, (int)std::floor((double)(px + 1) * nb / cols) - 1);
            double vm = 1000, hm = 1000, lm = -200, cm = 0; bool found = false;
            for (int i = b0; i <= b1 && i < nb; i++) {
                if (!an.hasSignal[(size_t)i]) continue;
                const double v = mono ? an.monoLoss[(size_t)i] : an.corr[(size_t)i];
                if (v < vm) { vm = v; lm = an.level[(size_t)i]; cm = an.corr[(size_t)i]; }
                hm = juce::jmin(hm, mono ? an.monoHeld[(size_t)i] : an.corrHeld[(size_t)i]);
                found = true;
            }
            if (!found) continue;
            const float x = gx + px * c.hair, yv = map(vm);
            c.colour(Theme::mix(th.panel, th.correlation(cm), juce::jlimit(0.3, 1.0, (lm + 80) / 50)));
            c.rect(x, juce::jmin(yv, y0), c.hair, juce::jmax(c.hair, std::fabs(yv - y0)));
            if (hold && hm < vm - (mono ? 0.5 : 0.02)) { c.colour(th.text, 0.85f); c.rect(x, map(hm), c.hair, holdH); }
        }
    }
    // reference curve: a thin line through the band centers, broken where a band had no signal
    if (refOn && (refBands != nb || refSetup != an.setupCount)) refOn = false;
    if (refOn) {
        c.colour(th.text, 0.75f);
        bool prev = false; float lx = 0, ly = 0;
        for (int i = 0; i < nb; i++) {
            if (!refValid[(size_t)i]) { prev = false; continue; }
            const float x = gx + (i + 0.5f) * xw, y = map(mono ? refMono[(size_t)i] : refCorr[(size_t)i]);
            if (prev) c.line(lx, ly, x, y, 2 * c.hair);
            lx = x; ly = y; prev = true;
        }
    }
    // range button, mono loss only: top right inside the display
    if (mono) {
        const String t = "Range " + String((int)rangeDb) + " dB";
        const float rw = bn::Canvas::width(fonts.small, t) + 20 * usc, rh = 20 * usc;
        const float rx = gx + gw - rw;
        addHit(hRange, rx, gy, rw, rh);
        button(c, rx, gy, rw, rh, false, hover == hRange, t);
    }
    if (dmx) drawDownmix(c, gx, gy);
}

// the 5.1 downmix that the Downmix pair compares: ITU-R BS.775 or own gains (drag up and down, double-click: -3 dB)
bool BinocularEditor::downmixShown() const {
    return proc.surround.load() && proc.get(bn::pView) < 1.5 && std::floor(proc.get(bn::pPair) + 0.5) == 2;
}

void BinocularEditor::drawDownmix(bn::Canvas& c, float x, float y) {
    const int mode = (int)std::floor(proc.get(bn::pDownmix) + 0.5);
    const bool manual = mode == 2;
    const float h = 20 * usc;
    c.colour(th.muted); c.textL(fonts.small, "Downmix", x, y + h / 2);
    downmixBox = { x + 58 * usc, y, 118 * usc, h };
    addHit(hDownmix, downmixBox.getX(), y, downmixBox.getWidth(), h);
    dropdown(c, downmixBox.getX(), y, downmixBox.getWidth(), h, hover == hDownmix || listOpen == 5, kDownmixNames[juce::jlimit(0, 2, mode)], fonts.small);
    float fx = downmixBox.getRight() + 14 * usc;
    if (!manual) {
        c.colour(th.dim);
        c.textL(fonts.note, mode == 0 ? "Lo = L + 0.707 C + 0.707 Ls, LFE not used"
                                      : "Lo = L + 0.707 C + 0.707 Ls + 0.316 LFE  (C, Ls/Rs -3 dB, LFE -10 dB)", fx, y + h / 2);
        return;
    }
    const int ids[3] = { hDmxCentre, hDmxSurround, hDmxLfe };
    const int nums[3] = { bn::pDmxCentre, bn::pDmxSurround, bn::pDmxLfe };
    const char* names[3] = { "C", "Ls/Rs", "LFE" };
    for (int k = 0; k < 3; k++) {
        const double v = proc.get(nums[k]);
        const String t = k == 2 && v <= -39.9 ? String("off") : (v > 0 ? "+" : "") + oneDecimal(v) + " dB";
        const float lw = bn::Canvas::width(fonts.small, names[k]) + 6 * usc, fw = 62 * usc;
        c.colour(th.muted); c.textL(fonts.small, names[k], fx, y + h / 2);
        const bool hot = hover == ids[k] || dragId == ids[k];
        addHit(ids[k], fx + lw, y, fw, h);
        c.colour(hot ? th.hover : th.panel2); c.box(fx + lw, y, fw, h, 6 * usc);
        c.colour(hot ? th.blue : th.line); c.frame(fx + lw, y, fw, h, 6 * usc);
        c.colour(th.text); c.textC(fonts.small, t, fx + lw + fw / 2, y + h / 2);
        fx += lw + fw + 12 * usc;
    }
}

// ---------------------------------------------------------------- goniometer: a square in the middle, balance on the left, width on the right
static void plotLine(juce::Image::BitmapData& bd, int W, int H, juce::Colour col, double x1, double y1, double x2, double y2) {
    // anti-aliased 1 pixel line (Xiaolin Wu) straight into the pixels: fast enough for thousands of segments per frame
    const float ca = col.getFloatAlpha();
    auto plot = [&](int px, int py, float cov) {
        if (px < 0 || py < 0 || px >= W || py >= H || cov <= 0.0f) return;
        ((juce::PixelARGB*)bd.getPixelPointer(px, py))->blend(col.withAlpha(juce::jlimit(0.0f, 1.0f, ca * cov)).getPixelARGB());
    };
    const bool steep = std::fabs(y2 - y1) > std::fabs(x2 - x1);
    if (steep) { std::swap(x1, y1); std::swap(x2, y2); }
    if (x1 > x2) { std::swap(x1, x2); std::swap(y1, y2); }
    const double dx = x2 - x1, dy = y2 - y1, grad = dx == 0.0 ? 1.0 : dy / dx;
    const int xs = (int)std::floor(x1 + 0.5), xe = (int)std::floor(x2 + 0.5);
    double yy = y1 + grad * (xs - x1);
    for (int x = xs; x <= xe; x++, yy += grad) {
        const int iy = (int)std::floor(yy); const float f = (float)(yy - iy);
        if (steep) { plot(iy, x, 1.0f - f); plot(iy + 1, x, f); }
        else { plot(x, iy, 1.0f - f); plot(x, iy + 1, f); }
    }
}

void BinocularEditor::drawGoniometer(bn::Canvas& c) {
    const float dx = ux(16), dw = (648 + wx) * usc, dy = uy(52), dh = (286 + ex) * usc;
    const float size = std::floor((262 + ex) * usc);
    const float cx = std::floor(dx + dw / 2), cy = std::floor(dy + dh / 2);
    const bool frozen = proc.get(bn::pFreeze) >= 0.5;
    const bool surround = an.channels == 6;
    const int pair = (int)std::floor(proc.get(bn::pPair) + 0.5);
    const String first = surround ? kPairFirst[pair] : "L";
    const String second = surround ? kPairSecond[pair] : "R";
    // the trace draws into an image in screen pixels, which fades a little every frame (afterglow)
    const float scale = 1.0f / c.hair;
    const int S = juce::jmax(8, (int)std::floor(size * scale));
    if (!gonioImage.isValid() || gonioImage.getWidth() != S || gonioBg != th.panel) {
        gonioImage = juce::Image(juce::Image::ARGB, S, S, false, juce::SoftwareImageType());
        gonioImage.clear(gonioImage.getBounds(), Theme::colour(th.panel));
        gonioBg = th.panel;
    }
    const double R = S / 2.0 - 8 * usc * scale, h2 = std::floor(S / 2.0);
    {
        juce::Graphics ig(gonioImage);
        if (!frozen) { ig.setColour(Theme::colour(th.panel, (float)(1 - std::pow(0.7, frameDt * 30)))); ig.fillAll(); }
        // the scale under the trace, so a mono trace on the M axis stays visible
        ig.setColour(Theme::colour(th.grid));
        ig.drawLine((float)(h2 - R), (float)h2, (float)(h2 + R), (float)h2, 1.0f);
        ig.drawLine((float)(h2 - R * 0.7071), (float)(h2 - R * 0.7071), (float)(h2 + R * 0.7071), (float)(h2 + R * 0.7071), 1.0f);
        ig.drawLine((float)(h2 + R * 0.7071), (float)(h2 - R * 0.7071), (float)(h2 - R * 0.7071), (float)(h2 + R * 0.7071), 1.0f);
        ig.setColour(Theme::colour(th.line));
        ig.drawEllipse((float)(h2 - R), (float)(h2 - R), (float)(2 * R), (float)(2 * R), 1.0f);
        ig.drawEllipse((float)(h2 - R / 2), (float)(h2 - R / 2), (float)R, (float)R, 1.0f);
        ig.drawLine((float)h2, (float)(h2 - R), (float)h2, (float)(h2 + R), 1.0f);
    }
    // the samples since the last frame (at most 4096), starting at the last point of the previous frame (no gaps)
    const int write = an.gonioWrite.load(std::memory_order_acquire);
    const int mask = Analyzer::kGonioSize - 1;
    const int n = juce::jmin(4096, (write - gonioRead + Analyzer::kGonioSize) & mask) + 1;
    const int start = (write - n + Analyzer::kGonioSize) & mask;
    double pk2 = 0;
    for (int i = 0; i < n; i++) {
        const int j = (start + i) & mask;
        pk2 = juce::jmax(pk2, (double)an.gonioX[(size_t)j] * an.gonioX[(size_t)j] + (double)an.gonioY[(size_t)j] * an.gonioY[(size_t)j]);
    }
    // automatic gain: follows the loudest point, falls back slowly, at most +40 dB
    gonioPeak = juce::jmax(std::sqrt(pk2), gonioPeak * std::pow(0.96, frameDt * 30));
    const double gain = juce::jmin(100.0, 0.9 / juce::jmax(gonioPeak, 1e-9));
    if (n > 1 && !frozen) {
        juce::Image::BitmapData bd(gonioImage, juce::Image::BitmapData::readWrite);
        const juce::Colour col = Theme::colour(th.correlation(an.total), 0.85f);
        for (int pass = 0; pass < (scale >= 1.5f ? 2 : 1); pass++) {
            double lx = 0, ly = 0;
            for (int i = 0; i < n; i++) {
                const int j = (start + i) & mask;
                const double x = juce::jlimit(0.0, S - 2.0, h2 + an.gonioX[(size_t)j] * gain * R + pass);
                const double y = juce::jlimit(0.0, S - 2.0, h2 - an.gonioY[(size_t)j] * gain * R);
                if (i > 0) plotLine(bd, S, S, col, lx, ly, x, y);
                lx = x; ly = y;
            }
        }
    }
    gonioRead = write;
    c.g.drawImage(gonioImage, juce::Rectangle<float>(cx - (float)h2 / scale, cy - (float)h2 / scale, (float)S / scale, (float)S / scale));
    // scale labels: mid (up), side (across), left and right diagonals
    const float r = (float)R / scale;
    c.colour(th.dim);
    c.textC(fonts.label, "M", cx, cy - r - 9 * usc);
    c.textC(fonts.label, first, cx - r * 0.7071f - (first.length() > 1 ? 14 : 8) * usc, cy - r * 0.7071f - 6 * usc);
    c.textC(fonts.label, second, cx + r * 0.7071f + (second.length() > 1 ? 12 : 9) * usc, cy - r * 0.7071f - 6 * usc);
    c.textR(fonts.label, "+S", cx - r - 5 * usc, cy);
    c.textL(fonts.label, "-S", cx + r + 5 * usc, cy);
    const int gdb = (int)std::floor(20 * std::log10(gain) + 0.5);
    c.textR(fonts.note, "Auto gain " + String(gdb > 0 ? "+" : "") + String(gdb) + " dB", dx + dw - 14 * usc, dy + dh - 14 * usc);
    addHit(hGonio, cx - size / 2, dy, size, dh);

    // side panels: balance and width, each a value and a bar
    const float pw = 150 * usc;
    for (int k = 0; k < 2; k++) {
        const float px = k == 0 ? dx + 26 * usc : dx + dw - 26 * usc - pw;
        const float py = cy - 30 * usc;
        c.colour(th.muted); c.textC(fonts.label, k == 0 ? "Balance" : "Width", px + pw / 2, py);
        String v; double f;
        if (an.totalValid) {
            if (k == 0) {
                const double b = an.balanceDb;
                v = std::fabs(b) < 0.25 ? String("Centered") : std::fabs(b) > 60 ? (b > 0 ? first : second) + " only"
                                                                                     : (b > 0 ? first : second) + " " + oneDecimal(std::fabs(b)) + " dB";
                f = juce::jlimit(-1.0, 1.0, b / 12);
            } else {
                v = "S/M " + oneDecimal(juce::jmax(-60.0, an.widthDb)) + " dB";
                f = juce::jlimit(0.0, 1.0, (an.widthDb + 40) / 46);
            }
        } else { v = "--"; f = k == 0 ? 0 : -1; }
        c.colour(th.text); c.textC(fonts.value, v, px + pw / 2, py + 20 * usc);
        const float by = py + 38 * usc, bh = 8 * usc;
        c.colour(th.panel2); c.box(px, by, pw, bh, 4 * usc);
        if (!th.dark) { c.colour(th.line); c.frame(px, by, pw, bh, 4 * usc); }
        c.colour(th.accent);
        if (k == 0) {
            if (f > 0) c.box(px + pw / 2 * (1 - (float)f), by, pw / 2 * (float)f, bh, 4 * usc);
            else if (f < 0) c.box(px + pw / 2, by, -pw / 2 * (float)f, bh, 4 * usc);
            c.colour(th.dim); c.vline(px + pw / 2, by - 2 * usc, bh + 4 * usc);
        } else {
            if (f > 0) c.box(px, by, pw * (float)f, bh, 4 * usc);
            c.colour(th.dim); c.vline(px + pw * 40 / 46, by - 2 * usc, bh + 4 * usc);   // 0 dB: side as loud as mid
        }
        c.colour(th.dim);
        c.textL(fonts.note, k == 0 ? first : String("mono"), px, by + bh + 9 * usc);
        c.textR(fonts.note, k == 0 ? second : String("wide"), px + pw, by + bh + 9 * usc);
        addHit(hBalance + k, px, py - 10 * usc, pw, 76 * usc);
    }
}

// ---------------------------------------------------------------- 5.1 vectorscope: where the sound comes from
// The five speakers on a circle (ITU-R BS.775: C ahead, L / R at 30 degrees, Ls / Rs at 110 degrees), their levels as a
// shape, and the energy vector as a trace with afterglow: in the middle = spread over all speakers, at the edge = one speaker.
void BinocularEditor::drawVectorscope(bn::Canvas& c) {
    const float dx = ux(16), dw = (648 + wx) * usc, dy = uy(52), dh = (286 + ex) * usc;
    const float size = std::floor((262 + ex) * usc);
    const float cx = std::floor(dx + dw / 2), cy = std::floor(dy + dh / 2) + 4 * usc;
    const bool frozen = proc.get(bn::pFreeze) >= 0.5;
    const float scale = 1.0f / c.hair;
    const int S = juce::jmax(8, (int)std::floor(size * scale));
    if (!vecImage.isValid() || vecImage.getWidth() != S || vecBg != th.panel) {
        vecImage = juce::Image(juce::Image::ARGB, S, S, false, juce::SoftwareImageType());
        vecImage.clear(vecImage.getBounds(), Theme::colour(th.panel));
        vecBg = th.panel;
    }
    const double R = S / 2.0 - 22 * usc * scale, h2 = std::floor(S / 2.0);
    static const float ang[5] = { -0.5236f, 0.0f, 0.5236f, 1.9199f, -1.9199f };   // L C R Rs Ls: around the circle
    static const int chan[5] = { 0, 2, 1, 5, 4 };
    static const char* names[5] = { "L", "C", "R", "Rs", "Ls" };
    {
        juce::Graphics ig(vecImage);
        if (!frozen) { ig.setColour(Theme::colour(th.panel, (float)(1 - std::pow(0.82, frameDt * 30)))); ig.fillAll(); }
        ig.setColour(Theme::colour(th.grid));
        for (float f : { 1.0f / 3, 2.0f / 3 }) ig.drawEllipse((float)(h2 - R * f), (float)(h2 - R * f), (float)(2 * R * f), (float)(2 * R * f), 1.0f);
        for (int k = 0; k < 5; k++) ig.drawLine((float)h2, (float)h2, (float)(h2 + std::sin(ang[k]) * R), (float)(h2 - std::cos(ang[k]) * R), 1.0f);
        ig.setColour(Theme::colour(th.line));
        ig.drawEllipse((float)(h2 - R), (float)(h2 - R), (float)(2 * R), (float)(2 * R), 1.0f);
    }
    // the energy vectors since the last frame, joined by lines (silence breaks the line)
    const int write = an.vecWrite.load(std::memory_order_acquire);
    const int mask = Analyzer::kVecSize - 1;
    const int n = juce::jmin(2048, (write - vecRead + Analyzer::kVecSize) & mask);
    double mx = 0, my = 0; int mn = 0;
    if (n > 0 && !frozen) {
        juce::Image::BitmapData bd(vecImage, juce::Image::BitmapData::readWrite);
        const juce::Colour col = Theme::colour(th.mint, 0.8f);
        double lx = 0, ly = 0; bool prev = false;
        for (int i = 0; i < n; i++) {
            const int j = (write - n + i + Analyzer::kVecSize) & mask;
            const float vx = an.vecX[(size_t)j], vy = an.vecY[(size_t)j];
            if (std::isnan(vx) || std::isnan(vy)) { prev = false; continue; }
            const double x = h2 + vx * R, y = h2 - vy * R;
            if (prev) plotLine(bd, S, S, col, lx, ly, x, y);
            lx = x; ly = y; prev = true;
            mx += vx; my += vy; mn++;
        }
    }
    vecRead = write;
    c.g.drawImage(vecImage, juce::Rectangle<float>(cx - (float)h2 / scale, cy - (float)h2 / scale, (float)S / scale, (float)S / scale));
    const float r = (float)R / scale;

    // the speaker levels as a shape
    // every speaker follows its energy: up at once, down at 24 dB per second, so the shape moves with the mix
    double db[6];
    for (int k = 0; k < 6; k++) {
        const double now = 10 * std::log10(an.channelPower[(size_t)k] + 1e-20);
        if (!frozen) shapeDb[k] = juce::jmax(now, shapeDb[k] - 24 * frameDt);
        db[k] = shapeDb[k];
    }
    // the scale follows the loudest speaker (falls back 3 dB per second): the shape always fills the circle,
    // quiet mixes as well as loud ones; 36 dB under the loudest is the middle
    double loudest = -120;
    for (int k = 0; k < 5; k++) loudest = juce::jmax(loudest, db[chan[k]]);
    if (!frozen) shapeTop = juce::jmax(loudest, shapeTop - 3 * frameDt);
    juce::Path shape;
    for (int k = 0; k < 5; k++) {
        const float lr = r * (float)juce::jlimit(0.0, 1.0, (db[chan[k]] - shapeTop + 36) / 36);
        const float px = cx + std::sin(ang[k]) * lr, py = cy - std::cos(ang[k]) * lr;
        if (k == 0) shape.startNewSubPath(px, py); else shape.lineTo(px, py);
    }
    shape.closeSubPath();
    c.colour(th.blue, 0.16f); c.g.fillPath(shape);
    c.colour(th.blue, 0.9f); c.g.strokePath(shape, juce::PathStrokeType(1.4f * usc, juce::PathStrokeType::curved));
    // where the sound sits now: the mean of this frame's vectors
    if (mn > 0 && !frozen) { lastVecX = mx / mn; lastVecY = my / mn; }
    if (!std::isnan(lastVecX)) { c.colour(th.text); c.disc(cx + (float)lastVecX * r, cy - (float)lastVecY * r, 3.5f * usc); }
    // the speakers, their names and levels
    for (int k = 0; k < 5; k++) {
        const float sx = cx + std::sin(ang[k]) * r, sy = cy - std::cos(ang[k]) * r;
        c.colour(th.panel); c.disc(sx, sy, 4 * usc);
        c.colour(db[chan[k]] > -60 ? th.blue : th.dim); c.ring(sx, sy, 4 * usc, 1.4f * usc);
        const float lx = cx + std::sin(ang[k]) * (r + 16 * usc), ly = cy - std::cos(ang[k]) * (r + 14 * usc);
        c.colour(th.muted); c.textC(fonts.label, names[k], lx, ly);
    }
    c.colour(th.dim);
    c.textR(fonts.note, "LFE " + (db[3] > -100 ? oneDecimal(db[3]) + " dB" : String("--")), dx + dw - 14 * usc, dy + dh - 14 * usc);
    addHit(hVector, cx - size / 2, dy, size, dh);

    // side panels: left / right and front / rear, each a value and a bar
    const float pw = 150 * usc;
    const double left = an.channelPower[0] + an.channelPower[4], right = an.channelPower[1] + an.channelPower[5];
    const double front = an.channelPower[0] + an.channelPower[1] + an.channelPower[2], rear = an.channelPower[4] + an.channelPower[5];
    for (int k = 0; k < 2; k++) {
        const float px = k == 0 ? dx + 26 * usc : dx + dw - 26 * usc - pw;
        const float py = cy - 34 * usc;
        const double a = k == 0 ? left : front, b = k == 0 ? right : rear;
        const bool has = a + b > 1e-10;
        const double d = has ? 10 * std::log10((a + 1e-20) / (b + 1e-20)) : 0;
        c.colour(th.muted); c.textC(fonts.label, k == 0 ? "Left / right" : "Front / rear", px + pw / 2, py);
        String v = !has ? String("--") : std::fabs(d) < 0.25 ? String(k == 0 ? "Centered" : "Even")
                 : std::fabs(d) > 60 ? String(k == 0 ? (d > 0 ? "Left only" : "Right only") : (d > 0 ? "Front only" : "Rear only"))
                 : String(k == 0 ? (d > 0 ? "L " : "R ") : (d > 0 ? "Front " : "Rear ")) + oneDecimal(std::fabs(d)) + " dB";
        c.colour(th.text); c.textC(fonts.value, v, px + pw / 2, py + 20 * usc);
        const float by = py + 38 * usc, bh = 8 * usc;
        c.colour(th.panel2); c.box(px, by, pw, bh, 4 * usc);
        if (!th.dark) { c.colour(th.line); c.frame(px, by, pw, bh, 4 * usc); }
        const float f = (float)juce::jlimit(-1.0, 1.0, d / 12);
        c.colour(th.accent);
        if (f > 0) c.box(px + pw / 2 * (1 - f), by, pw / 2 * f, bh, 4 * usc);
        else if (f < 0) c.box(px + pw / 2, by, -pw / 2 * f, bh, 4 * usc);
        c.colour(th.dim); c.vline(px + pw / 2, by - 2 * usc, bh + 4 * usc);
        c.textL(fonts.note, k == 0 ? "L" : "front", px, by + bh + 9 * usc);
        c.textR(fonts.note, k == 0 ? "R" : "rear", px + pw, by + bh + 9 * usc);
        addHit(k == 0 ? hBalance : hFrontRear, px, py - 10 * usc, pw, 76 * usc);
    }
}

// ---------------------------------------------------------------- loudness: the history graph on the left (time runs left to right,
// now at the right edge), meters next to it, the readings and the verdict in a narrow column on the right
void BinocularEditor::drawLoudness(bn::Canvas& c) {
    const int p = (int)std::floor(proc.get(bn::pTarget) + 0.5);
    const int span = (int)std::floor(proc.get(bn::pSpan) + 0.5);
    const int measure = (int)std::floor(proc.get(bn::pMeasure) + 0.5);
    const Target& tg = kTargets[juce::jlimit(0, 9, p)];
    const double target = tg.lufs, tol = tg.tol;
    const int peakLimit = tg.peak;
    const double lo = target - 26, hi = target + 6;          // the scale: the target sits at 80 % of the height
    const bool live = an.measuring && proc.get(bn::pFreeze) < 0.5;
    const float wp = wx * usc;
    const float divider = ux(452) + wp;                       // the graph gets all extra width
    const float gx0 = ux(62), gx1 = divider - 128 * usc, gw = gx1 - gx0;
    const float gy0 = uy(92), gy1 = uy(306 + ex), gh = gy1 - gy0;
    auto map = [&](double L) { return gy1 - gh * (float)juce::jlimit(0.0, 1.0, (L - lo) / (hi - lo)); };

    // grid every 10 LU, the tolerance as a mint band, the target as a mint line
    c.colour(Theme::mix(th.panel, th.mint, th.dark ? 0.10 : 0.12));
    c.rect(gx0, map(target + tol), gw, juce::jmax(c.hair, map(target - tol) - map(target + tol)));
    for (double v = std::ceil((lo + 0.01) / 10) * 10; v < hi - 0.5; v += 10) {
        c.colour(th.grid); c.hline(gx0, map(v), gw);
        if (std::fabs(v - target) > 2.5) { c.colour(th.dim); c.textR(fonts.axis, String((int)v), gx0 - 6 * usc, map(v)); }
    }
    for (int k = 1; k < 4; k++) { c.colour(th.grid); c.vline(gx0 + gw * k / 4, gy0, gh); }
    c.colour(th.mint); c.rect(gx0, map(target) - 0.7f * usc, gw, juce::jmax(c.hair, 1.4f * usc));
    c.textR(fonts.axis, String((int)target), gx0 - 6 * usc, map(target));
    c.colour(th.line); c.hline(gx0, gy1, gw);

    // the history: 360 steps, the oldest on the left. Filled softly, with a clean line on top in the color of each
    // moment (mint on target, amber and red louder, blue quieter). The line runs straight from step to step.
    const int newest = an.newestStep;
    const int N = Analyzer::kSteps;
    for (int k = 0; k < N; k++) {
        const int s = (newest + 1 + k) % N;
        const double e = an.stepEnergy[(size_t)s], cnt = an.stepCount[(size_t)s];
        stepY[(size_t)k] = (newest >= 0 && cnt > 0 && e > 0) ? map(-0.691 + 10 * std::log10(e / cnt)) : -1.0f;
    }
    const float cw = gw / N;
    for (int k = 0; k < N; k++) {
        const float y = stepY[(size_t)k];
        if (y < 0) continue;
        const int s = (newest + 1 + k) % N;
        const float yl = (k > 0 && stepY[(size_t)k - 1] >= 0) ? (y + stepY[(size_t)k - 1]) / 2 : y;
        const float yn = (k < N - 1 && stepY[(size_t)k + 1] >= 0) ? stepY[(size_t)k + 1] : y;
        const float yr = (y + yn) / 2;
        const juce::uint32 col = th.loudness(-0.691 + 10 * std::log10(an.stepEnergy[(size_t)s] / an.stepCount[(size_t)s]), target, tol);
        const float x0 = gx0 + k * cw, xm = x0 + cw / 2, x1 = x0 + cw;
        const float x2 = k < N - 1 ? x1 + juce::jmax(c.hair, 0.3f * cw) : x1;   // a pixel over the next step: no seams
        const float ye = yr + (yn - yr) * juce::jmin(1.0f, (x2 - x1) / juce::jmax(0.0001f, 0.5f * cw));
        c.colour(Theme::mix(th.panel, col, th.dark ? 0.32 : 0.30));
        c.polygon({ x0, gy1, x0, yl, xm, y, x1, yr, x2, ye, x2, gy1 });
        c.colour(col);
        c.line(x0, yl, xm, y, 1.6f * usc); c.line(xm, y, x1, yr, 1.6f * usc);
    }
    // the integrated loudness as a thin line across
    if (an.integrated > -70 && an.integrated > lo && an.integrated < hi) { c.colour(th.text, 0.55f); c.hline(gx0, map(an.integrated), gw); }
    // time: the span on the left, now on the right
    c.colour(th.dim);
    c.textL(fonts.axis, String("-") + kSpanNames[span], gx0, gy1 + 10 * usc);
    c.textR(fonts.axis, "now", gx1, gy1 + 10 * usc);
    addHit(hHistory, gx0, gy0, gw, gh);

    // meters right of the graph: loudness (M momentary, S short-term) on the graph's scale,
    // then the peak meters (L, R, true peak, dBFS) on their own scale
    const float mw = 11 * usc;                               // M and S: wider than the peak meters
    for (int k = 0; k < 2; k++) {
        const float mx = gx1 + (12 + k * 15) * usc;
        const double v = k == 0 ? an.momentary : an.shortTerm;
        c.colour(th.panel2); c.box(mx, gy0, mw, gh, 3 * usc);
        if (!th.dark) { c.colour(th.line); c.frame(mx, gy0, mw, gh, 3 * usc); }
        if (v > lo) { const float my = map(v); c.colour(th.loudness(v, target, tol), live ? 1.0f : 0.5f); c.box(mx, my, mw, gy1 - my, 3 * usc); }
        c.colour(th.mint); c.rect(mx - 1 * usc, map(target) - 0.7f * usc, mw + 2 * usc, juce::jmax(c.hair, 1.4f * usc));
        c.colour(th.dim); c.textC(fonts.meter, k == 0 ? "M" : "S", mx + mw / 2, gy1 + 10 * usc);
        addHit(hReading0 + k, mx - 3 * usc, gy0, mw + 6 * usc, gh);
    }
    // peaks: what came since the last frame, falling back 20 dB per second, the highest held for 2 s.
    // Stereo: L, R. 5.1: L, R, C, LFE, Ls, Rs.
    const bool frozen = proc.get(bn::pFreeze) >= 0.5;
    const int nch = an.channels;
    static const char* chNames[6] = { "L", "R", "C", "LF", "Ls", "Rs" };
    const float pw = nch == 6 ? 6 * usc : 7 * usc, pstep = 10 * usc;
    for (int k = 0; k < nch; k++) {
        const float v = an.peaks[(size_t)k].exchange(0.0f);
        const double d = v > 0.000001f ? 20 * std::log10((double)v) : -120;
        if (!frozen) {
            peakDb[k] = juce::jmax(d, peakDb[k] - 20 * frameDt);
            if (d >= holdDb[k]) { holdDb[k] = d; holdTime[k] = 0; }
            else { holdTime[k] += frameDt; if (holdTime[k] > 2) holdDb[k] = juce::jmax(peakDb[k], holdDb[k] - 20 * frameDt); }
        }
        const float mx = gx1 + (nch == 6 ? 46 : 52) * usc + k * pstep;
        c.colour(th.panel2); c.box(mx, gy0, pw, gh, 3 * usc);
        if (!th.dark) { c.colour(th.line); c.frame(mx, gy0, pw, gh, 3 * usc); }
        if (peakDb[k] > -60) {
            const float my = gy1 - gh * (float)juce::jmin(1.0, (peakDb[k] + 60) / 60);
            c.colour(peakDb[k] > peakLimit ? th.red : peakDb[k] > peakLimit - 6 ? th.warn : th.mint);
            c.box(mx, my, pw, gy1 - my, 3 * usc);
        }
        if (holdDb[k] > -60) {
            c.colour(holdDb[k] > peakLimit ? th.red : th.text, 0.9f);
            c.rect(mx, gy1 - gh * (float)juce::jmin(1.0, (holdDb[k] + 60) / 60), pw, juce::jmax(c.hair, 1.5f * usc));
        }
        c.colour(th.dim); c.textC(fonts.meter, chNames[k], mx + pw / 2, gy1 + 10 * usc);
        addHit(hPeaks, mx - 1 * usc, gy0, pw + 2 * usc, gh);
    }
    // the peak scale, right of the peak meters
    c.colour(th.dim);
    for (int v : { 0, -6, -12, -24, -36, -48 }) c.textR(fonts.meter, String(v), divider - 8 * usc, gy1 - gh * (v + 60) / 60.0f);

    // history span, top left
    spanBox = { ux(26), uy(62), 54 * usc, 18 * usc };
    addHit(hSpan, spanBox.getX(), spanBox.getY(), spanBox.getWidth(), spanBox.getHeight());
    dropdown(c, spanBox.getX(), spanBox.getY(), spanBox.getWidth(), spanBox.getHeight(), hover == hSpan || listOpen == 1, kSpanNames[span], fonts.note);

    // readings, right of a divider, compact
    c.colour(th.line); c.vline(divider, uy(66), (258 + ex) * usc);
    const float xl = divider + 12 * usc, xr = ux(652) + wp, xv = xr - 42 * usc;
    const double tp = an.truePeakMax > 0 ? 20 * std::log10(an.truePeakMax) : -200;
    const double wide = tg.film ? 20 : 15;
    // dialog-gated targets (Netflix, Prime Video, ATSC long-form) judge the dialog once enough of it is found
    const bool useDialog = tg.dialog && an.dialog > -70 && an.dialogShare >= 0.1;
    const double judged = useDialog ? an.dialog : an.integrated;
    auto judgeColour = [&](double v) {
        return v <= -70 ? th.dim : std::fabs(v - target) <= tol ? th.mint : !tg.streaming ? th.red : v > target ? th.warn : th.blue;
    };
    // the two values that decide a delivery, framed at the top: integrated (blue), true peak (mint) under it
    struct Boxed { float y, h; juce::uint32 frame; const char* label; double v; const char* unit; juce::uint32 dot; int id; };
    const Boxed boxed[2] = {
        { uy(66), 36 * usc, th.blue, "Integrated", an.integrated, "LUFS", useDialog ? th.dim : judgeColour(an.integrated), hReading0 + 2 },
        { uy(108), 28 * usc, th.mint, "True peak max", tp, "dBTP", an.truePeakMax <= 0 ? th.dim : tp > peakLimit ? th.red : th.mint, hReading0 + 5 },
    };
    for (int k = 0; k < 2; k++) {
        const auto& b = boxed[k];
        const float cy = b.y + b.h / 2;
        c.colour(b.frame); c.g.drawRoundedRectangle(xl + 0.7f * usc, b.y + 0.7f * usc, xr - xl - 1.4f * usc, b.h - 1.4f * usc, 8 * usc, 1.4f * usc);
        c.colour(th.text); c.textL(fonts.label, b.label, xl + 10 * usc, cy);
        const bool has = b.v > (k == 1 ? -120 : -70);
        c.colour(th.text); c.textR(k == 0 ? fonts.valueBig : fonts.valueMid, has ? oneDecimal(b.v) : String("--"), xr - 46 * usc, cy);
        c.colour(th.dim); c.textL(fonts.tiny, b.unit, xr - 42 * usc, cy + 0.5f * usc);
        c.colour(b.dot); c.disc(xr - 10 * usc, cy, 3 * usc);
        addHit(b.id, xl, b.y, xr - xl, b.h);
    }
    // the rest smaller, under them
    const int rows[7] = { 0, 1, 7, 8, 3, 4, 6 };   // momentary, short-term, their maxima, dialog, range, timecode
    for (int r = 0; r < 7; r++) {
        const int k = rows[r];
        const float ry = uy(148 + r * 13.0f);
        const int id = k == 7 ? hMaxMomentary : k == 8 ? hMaxShort : k == 6 ? hTimecode : hReading0 + k;
        addHit(id, xl, ry - 6.5f * usc, xr - xl, 13 * usc);
        String label = k == 0 ? "Momentary" : k == 1 ? "Short-term" : k == 7 ? "Momentary max" : k == 8 ? "Short-term max"
                     : k == 3 ? "Dialog" : k == 4 ? "Loudness range" : "Timecode";
        if (k == 3 && an.measuredSlots > 0) label << " (" << (int)std::floor(an.dialogShare * 100 + 0.5) << " %)";
        if (k == 6) label << " " << kFpsNames[juce::jlimit(0, 5, (int)std::floor(proc.get(bn::pFps) + 0.5))];
        c.colour(k == 6 && (hover == id || listOpen == 4) ? th.text : th.muted); c.textL(fonts.tiny, label, xl, ry);
        if (k != 6) {
            const double v = k == 0 ? an.momentary : k == 1 ? an.shortTerm : k == 7 ? an.momentaryMax : k == 8 ? an.shortTermMax : k == 3 ? an.dialog : an.range;
            const String vs = k == 4 ? (v >= 0 ? oneDecimal(v) : String("--")) : (v > -70 ? oneDecimal(v) : String("--"));
            c.colour(k <= 1 && !live ? th.muted : th.text); c.textR(fonts.valueTiny, vs, xr - 46 * usc, ry);
            c.colour(th.dim); c.textL(fonts.tiny, k == 4 ? "LU" : "LUFS", xr - 42 * usc, ry + 0.5f * usc);
            juce::uint32 col;
            if (k == 3) col = v <= -70 ? th.dim : tg.dialog ? judgeColour(v) : th.loudness(v, target, tol);
            else if (k <= 1 || k >= 7) col = th.loudness(v, target, tol);
            else col = an.range < 0 ? th.dim : an.range > wide ? th.warn : th.mint;
            c.colour(col); c.disc(xr - 10 * usc, ry, 2.5f * usc);
        } else {
            // the DAW position as timecode (without the DAW's own start offset); click: rate and start
            double secs = 0;
            if (auto* ph = proc.getPlayHead()) if (auto p2 = ph->getPosition()) if (auto t = p2->getTimeInSeconds()) secs = *t;
            tcBox = { xl, ry - 6.5f * usc, xr - xl, 13 * usc };
            c.colour(th.blue);
            c.textR(fonts.valueTiny, timecode(secs, juce::jlimit(0, 5, (int)std::floor(proc.get(bn::pFps) + 0.5)),
                                              juce::jlimit(0, 2, (int)std::floor(proc.get(bn::pTcStart) + 0.5))), xr - 4 * usc, ry);
        }
    }

    // the verdict: three lines that say what the numbers mean for the chosen target
    const String name = tg.name;
    const float cy0 = uy(244), ch = (80 + ex) * usc;
    c.colour(th.panel2); c.box(xl, cy0, xr - xl, ch, 10 * usc);
    if (!th.dark) { c.colour(th.line); c.frame(xl, cy0, xr - xl, ch, 10 * usc); }
    addHit(hVerdict, xl, cy0, xr - xl, ch);
    for (int k = 0; k < 3; k++) {
        const float ry = cy0 + (12 + k * 25) * usc;
        juce::uint32 col; String head, detail;
        if (k == 0) {
            if (judged <= -70) {
                col = th.dim; head = "Waiting for audio";
                detail = live ? "measuring ..." : measure >= 1 ? "starts when the DAW plays" : "no signal yet";
            } else {
                const double dev = judged - target;
                // dialog-gated targets: the dialog value; little dialog found: the full mix, and it says so
                const String tolText = String::fromUTF8(" \xc2\xb1") + (tol < 1 ? "0.5" : tol < 1.5 ? "1" : "2");
                if (tg.dialog) {
                    const String lu = oneDecimal(std::fabs(dev)) + " LU";
                    if (std::fabs(dev) <= tol) {
                        col = th.mint; head = "On target";
                        detail = useDialog ? "dialog " + String(dev >= 0 ? "+" : "-") + lu + " from " + String((int)target) + tolText
                                           : "full mix " + String(dev >= 0 ? "+" : "-") + lu + ", little dialog";
                    } else {
                        col = th.red; head = dev > 0 ? "Too loud" : "Too quiet";
                        detail = useDialog ? "dialog " + lu + (dev > 0 ? " over " : " under ") + String((int)target) + (dev > 0 ? ", turn down" : ", turn up")
                                           : "full mix " + lu + (dev > 0 ? " over" : " under") + ", little dialog";
                    }
                } else if (std::fabs(dev) <= tol) {
                    col = th.mint; head = "On target";
                    detail = String(dev >= 0 ? "+" : "") + oneDecimal(dev) + " LU from " + name + " (" + String((int)target) + tolText + ")";
                } else if (tg.streaming) {
                    if (dev > 0) { col = th.warn; head = "Turned down"; detail = name + " plays it " + oneDecimal(dev) + " dB quieter"; }
                    else { col = th.blue; head = "Below target"; detail = oneDecimal(-dev) + " LU under " + String((int)target) + ", may sound quiet"; }
                } else {
                    col = th.red;
                    if (dev > 0) { head = "Too loud"; detail = oneDecimal(dev) + " LU over " + String((int)target) + ", turn down " + oneDecimal(dev) + " dB"; }
                    else { head = "Too quiet"; detail = oneDecimal(-dev) + " LU under " + String((int)target) + ", turn up " + oneDecimal(-dev) + " dB"; }
                }
            }
        } else if (k == 1) {
            if (an.truePeakMax <= 0.000001) { col = th.dim; head = "True peak"; detail = "no signal yet"; }
            else if (tp > peakLimit) { col = th.red; head = "True peak too high"; detail = oneDecimal(tp) + " dBTP, " + name + " allows " + String(peakLimit); }
            else { col = th.mint; head = "True peak OK"; detail = oneDecimal(tp) + " dBTP, limit " + String(peakLimit); }
        } else {
            if (an.range < 0) { col = th.dim; head = "Loudness range"; detail = "needs a few seconds"; }
            else if (an.range > wide) { col = th.warn; head = "Wide loudness range"; detail = oneDecimal(an.range) + " LU, quiet parts may get lost"; }
            else {
                col = th.mint; head = "Loudness range";
                detail = oneDecimal(an.range) + " LU, " + (an.range < 4 ? "very even" : an.range < 8 ? "even" : tg.film ? "fine for film" : "natural dynamics");
            }
        }
        c.colour(col); c.disc(xl + 13 * usc, ry, 3 * usc);
        c.colour(th.text); c.textL(fonts.label, head, xl + 23 * usc, ry);
        c.colour(th.muted); c.textL(fonts.tiny, detail, xl + 23 * usc, ry + 11 * usc);
    }
}

// ---------------------------------------------------------------- total: the full signal on one classic bar
void BinocularEditor::drawTotal(bn::Canvas& c) {
    const float y = uy(347 + ex), h = 10 * usc, x = ux(54), w = (562 + wx) * usc;
    c.colour(th.muted); c.textL(fonts.label, "Total", ux(16), y + h / 2);
    c.colour(th.panel2); c.box(x, y, w, h, 5 * usc);
    if (!th.dark) { c.colour(th.line); c.frame(x, y, w, h, 5 * usc); }
    const float mid = std::floor(x + w / 2);
    if (an.totalValid) {
        const double v = an.total;
        const float xv = std::floor(x + w * (float)(v + 1) / 2);
        c.colour(th.correlation(v));
        if (v >= 0) c.box(mid, y, juce::jmax(h, xv - mid + h / 2), h, 5 * usc);
        else c.box(juce::jmin(xv - h / 2, mid - h), y, juce::jmax(h, mid - xv + h / 2), h, 5 * usc);
        if (proc.get(bn::pHold) >= 0.5 && an.totalHeld < v - 0.02) {
            c.colour(th.text, 0.85f); c.rect(x + w * (float)(an.totalHeld + 1) / 2, y - 2 * usc, juce::jmax(c.hair, 1.5f * usc), h + 4 * usc);
        }
    }
    c.colour(th.dim); c.vline(mid, y - 2 * usc, h + 4 * usc);
    c.colour(th.text); c.textR(fonts.valueSmall, an.totalValid ? signedValue(an.total) : String("--"), ux(664) + wx * usc, y + h / 2);
    addHit(hTotal, ux(16), y - 4 * usc, (648 + wx) * usc, h + 8 * usc);
}

// ---------------------------------------------------------------- controls
void BinocularEditor::knob(bn::Canvas& c, int id, float cx, float cy, float r, double f, const String& label, const String& value) {
    const bool hot = hover == id || dragId == id || (id == hBands && dragId == hSelectivity);
    c.colour(th.muted); c.textC(fonts.label, label, cx, uy(382 + ex));
    c.colour(th.dark ? (hot ? th.line : th.panel2) : (hot ? 0xC9DDEEu : th.line));
    c.arc(cx, cy, r, -2.356f, 2.356f, 3 * usc);
    if (f > 0.002) { c.colour(th.accent); c.arc(cx, cy, r, -2.356f, -2.356f + 4.712f * (float)f, 3 * usc); }
    const float a = -2.356f + 4.712f * (float)f;
    c.colour(th.text); c.line(cx, cy, cx + std::sin(a) * (r - 6 * usc), cy - std::cos(a) * (r - 6 * usc), 1.6f * usc);
    c.colour(th.text); c.textC(fonts.knobValue, value, cx, uy(454 + ex));
    addHitCircle(id, cx, cy, r + 8 * usc);
}

// a button: switched on it is filled with the accent
void BinocularEditor::button(bn::Canvas& c, float x, float y, float w, float h, bool on, bool hot, const String& text) {
    if (on) {
        c.colour(th.accent); c.box(x, y, w, h, 8 * usc);
        c.colour(th.accentText); c.textC(fonts.tabOn, text, x + w / 2, y + h / 2);
    } else {
        c.colour(hot ? th.hover : th.panel2); c.box(x, y, w, h, 8 * usc);
        c.colour(th.line); c.frame(x, y, w, h, 8 * usc);
        c.colour(th.text); c.textC(fonts.small, text, x + w / 2, y + h / 2);
    }
}

// a dropdown: the setting and a small triangle
void BinocularEditor::dropdown(bn::Canvas& c, float x, float y, float w, float h, bool hot, const String& text, const juce::Font& f) {
    c.colour(hot ? th.hover : th.panel2); c.box(x, y, w, h, 8 * usc);
    c.colour(th.line); c.frame(x, y, w, h, 8 * usc);
    c.colour(th.text); c.textC(f, text, x + (w - 12 * usc) / 2, y + h / 2);
    c.colour(th.muted);
    c.polygon({ x + w - 15 * usc, y + h / 2 - 2 * usc, x + w - 9 * usc, y + h / 2 - 2 * usc, x + w - 12 * usc, y + h / 2 + 2 * usc });
}

double BinocularEditor::knobFraction(int id) const {
    if (id == hBands) return proc.get(bn::pBands) / 9;
    if (id == hSelectivity) return proc.get(bn::pSelectivity) / 100;
    return std::log(juce::jmax(10.0, proc.get(bn::pAveraging)) / 10) / std::log(500.0);
}

void BinocularEditor::knobSet(int id, double f, bool gesture) {
    f = juce::jlimit(0.0, 1.0, f);
    const int number = id == hBands ? bn::pBands : id == hSelectivity ? bn::pSelectivity : bn::pAveraging;
    const double v = id == hBands ? std::floor(f * 9 + 0.5) : id == hSelectivity ? std::floor(f * 1000 + 0.5) / 10 : std::floor(10 * std::pow(500.0, f) + 0.5);
    if (gesture) proc.set(number, v);
    else { auto* p = proc.param(number); p->setValueNotifyingHost(p->convertTo0to1((float)v)); }
}

void BinocularEditor::knobDefault(int id) {
    proc.set(id == hBands ? bn::pBands : id == hSelectivity ? bn::pSelectivity : bn::pAveraging, id == hBands ? 7 : id == hSelectivity ? 50 : 300);
}

void BinocularEditor::drawControls(bn::Canvas& c) {
    const float x = ux(16), w = (648 + wx) * usc, y = uy(366 + ex), h = 104 * usc;
    const float wp = wx * usc;
    c.colour(th.panel); c.box(x, y, w, h, 10 * usc);
    c.colour(th.line); c.frame(x, y, w, h, 10 * usc);
    const float labelY = uy(382 + ex), by = uy(407 + ex), bh = 22 * usc, noteY = uy(454 + ex);
    const int view = (int)std::floor(proc.get(bn::pView) + 0.5);

    if (view == 3) {
        // target: a list of the specs, the chosen one's values next to it
        const Target& tg = kTargets[juce::jlimit(0, 9, (int)std::floor(proc.get(bn::pTarget) + 0.5))];
        targetBox = { ux(28), by, 150 * usc, bh };
        c.colour(th.muted); c.textC(fonts.label, "Target", targetBox.getCentreX(), labelY);
        addHit(hTarget, targetBox.getX(), by, targetBox.getWidth(), bh);
        dropdown(c, targetBox.getX(), by, targetBox.getWidth(), bh, hover == hTarget || listOpen == 3, tg.name, fonts.small);
        c.colour(th.text); c.textL(fonts.small, String::fromUTF8(tg.note), ux(190), by + bh / 2);
        if (tg.dialog) { c.colour(th.muted); c.textL(fonts.note, "Dialog found by Binocular. For delivery, check with Dolby's meter.", targetBox.getX() + 4 * usc, noteY); }
        // measure: Always / Playback / Timecode
        const int m = (int)std::floor(proc.get(bn::pMeasure) + 0.5);
        measureBox = { ux(466) + std::floor(wp * 3 / 4), by, 80 * usc, bh };
        c.colour(th.muted); c.textC(fonts.label, "Measure", measureBox.getCentreX(), labelY);
        addHit(hMeasure, measureBox.getX(), by, measureBox.getWidth(), bh);
        dropdown(c, measureBox.getX(), by, measureBox.getWidth(), bh, hover == hMeasure || listOpen == 2, kMeasureNames[m], fonts.small);
        c.colour(th.muted); c.textC(fonts.note, m == 0 ? "all the time" : m == 1 ? "while the DAW plays" : "by DAW position", measureBox.getCentreX(), noteY);
        // measurement: reset, and how much is measured
        const float hx = ux(572) + wp, hw = 80 * usc;
        c.colour(th.muted); c.textC(fonts.label, "Measurement", hx + hw / 2, labelY);
        addHit(hLoudReset, hx, by, hw, bh);
        button(c, hx, by, hw, bh, false, hover == hLoudReset, "Reset");
        const int sec = an.measuredSlots / 10;
        const String t = sec >= 3600 ? String::formatted("%d:%02d:%02d measured", sec / 3600, sec / 60 % 60, sec % 60)
                                     : String::formatted("%d:%02d measured", sec / 60, sec % 60);
        c.colour(th.muted); c.textC(fonts.note, t, hx + hw / 2, noteY);
        return;
    }

    // Bands, Averaging, Offset, Reference, Hold. The goniometer page shows only what acts on it.
    const float ky = uy(418 + ex), kr = 17 * usc;
    const bool selectivity = dragId == hSelectivity || (hover == hBands && altDown);   // Alt on Bands: selectivity
    const String v1 = selectivity ? "Sel. " + String((int)std::floor(proc.get(bn::pSelectivity) + 0.5)) + " %"
                                  : String(Analyzer::kBandCounts[juce::jlimit(0, 9, (int)std::floor(proc.get(bn::pBands) + 0.5))]);
    const double avg = proc.get(bn::pAveraging);
    const String v3 = avg < 1000 ? String((int)std::floor(avg + 0.5)) + " ms" : String(avg / 1000, 1) + " s";
    if (view != 2) knob(c, hBands, ux(80), ky, kr, selectivity ? knobFraction(hSelectivity) : knobFraction(hBands), "Bands", v1);
    knob(c, hAveraging, ux(190) + std::floor(wp / 4), ky, kr, knobFraction(hAveraging), "Averaging", v3);

    // offset: info only, the audio is never moved
    const bool surround = an.channels == 6;
    const int pair = (int)std::floor(proc.get(bn::pPair) + 0.5);
    const String first = surround ? kPairFirst[pair] : "left";
    const String second = surround ? kPairSecond[pair] : "right";
    const float ofx = ux(272) + std::floor(wp / 2), ofw = 140 * usc, ofy = uy(396 + ex), ofh = 46 * usc;
    c.colour(th.muted); c.textC(fonts.label, "Offset", ofx + ofw / 2, labelY);
    c.colour(th.panel2); c.box(ofx, ofy, ofw, ofh, 8 * usc);
    if (!th.dark) { c.colour(th.line); c.frame(ofx, ofy, ofw, ofh, 8 * usc); }
    addHit(hOffset, ofx, ofy, ofw, ofh + 20 * usc);
    String o1, o2;
    if (an.offsetFound) {
        const double ms = std::fabs(an.offsetSamples) / an.sampleRate * 1000;
        o1 = (ms < 0.995 ? String(ms, 2) : ms < 9.95 ? String(ms, 1) : String(ms, 0)) + " ms";
        const int smp = (int)std::floor(std::fabs(an.offsetSamples) + 0.5);
        const String dot = String::fromUTF8(" \xc2\xb7 ");
        o2 = smp == 0 ? "0 samples" + dot + "in time"
                      : String(smp) + (smp == 1 ? " sample" : " samples") + dot
                        + (an.offsetSamples < 0 ? second : first) + " is late";
    } else {
        o1 = "--";
        o2 = an.totalValid ? "no clear offset" : "no signal to compare";
    }
    c.colour(an.offsetFound ? th.text : th.dim); c.textC(fonts.value, o1, ofx + ofw / 2, ofy + 16 * usc);
    c.colour(th.muted); c.textC(fonts.note, o2, ofx + ofw / 2, ofy + 35 * usc);
    if (an.offsetFound) {
        if (an.polarity < 0) {
            c.colour(th.red); c.box(ofx + ofw / 2 - 52 * usc, noteY - 8 * usc, 104 * usc, 16 * usc, 8 * usc);
            c.colour(th.dark ? th.bg : 0xFFFFFFu); c.textC(fonts.label, "Polarity flipped", ofx + ofw / 2, noteY);
        } else {
            c.colour(th.mint); c.textC(fonts.small, "Polarity OK", ofx + ofw / 2, noteY);
        }
    }

    // reference: capture the current curve as a line, click again to clear it (not on the goniometer page)
    if (view != 2) {
        const float rx = ux(466) + std::floor(wp * 3 / 4), rw = 80 * usc;
        c.colour(th.muted); c.textC(fonts.label, "Reference", rx + rw / 2, labelY);
        addHit(hReference, rx, by, rw, bh);
        button(c, rx, by, rw, bh, refOn, hover == hReference, refOn ? "Clear" : "Capture");
    }

    // hold lows: click: off / 3 s. Shift-click: infinite.
    const float hx = ux(572) + wp, hw = 80 * usc;
    const double hold = proc.get(bn::pHold);
    c.colour(th.muted); c.textC(fonts.label, "Hold lows", hx + hw / 2, labelY);
    addHit(hHold, hx, by, hw, bh);
    button(c, hx, by, hw, bh, hold >= 0.5, hover == hHold, hold < 0.5 ? "Off" : hold < 1.5 ? "3 s" : "Infinite");

    // reset: a quiet text button under Hold lows
    addHit(hResetPlugin, hx, noteY - 9 * usc, hw, 18 * usc);
    c.colour(hover == hResetPlugin ? th.text : th.dim); c.textC(fonts.note, "Reset plugin", hx + hw / 2, noteY);
}

// ---------------------------------------------------------------- dropdown lists (over everything below them)
void BinocularEditor::drawList(bn::Canvas& c) {
    if (!listOpen) return;
    // items (an empty name is a section title, the text after it), the chosen ones and where the list sits
    struct Item { String text; bool title; bool on; };
    std::vector<Item> items;
    juce::Rectangle<float> owner;
    float lw = 0;
    bool up = false;
    const auto choice = [&](int number) { return (int)std::floor(proc.get(number) + 0.5); };
    if (listOpen == 1) { for (int k = 0; k < 7; k++) items.push_back({ kSpanNames[k], false, k == choice(bn::pSpan) }); owner = spanBox; lw = 72 * usc; }
    else if (listOpen == 2) { for (int k = 0; k < 3; k++) items.push_back({ kMeasureNames[k], false, k == choice(bn::pMeasure) }); owner = measureBox; up = true; }
    else if (listOpen == 3) {
        const char* groups[3] = { "Broadcast", "Film and series", "Music streaming" };
        for (int k = 0; k < 10; k++) {
            if (k == 0 || k == 4 || k == 6) items.push_back({ groups[k == 0 ? 0 : k == 4 ? 1 : 2], true, false });
            items.push_back({ kTargets[kTargetOrder[k]].name, false, kTargetOrder[k] == choice(bn::pTarget) });
        }
        owner = targetBox; up = true;
    } else if (listOpen == 4) {
        items.push_back({ "Frame rate", true, false });
        for (int k = 0; k < 6; k++) items.push_back({ kFpsNames[k], false, k == choice(bn::pFps) });
        items.push_back({ "Start", true, false });
        for (int k = 0; k < 3; k++) items.push_back({ kStartNames[k], false, k == choice(bn::pTcStart) });
        owner = { tcBox.getRight() - 120 * usc, tcBox.getY(), 120 * usc, tcBox.getHeight() };
    } else {
        for (int k = 0; k < 3; k++) items.push_back({ kDownmixNames[k], false, choice(bn::pDownmix) == k });
        owner = downmixBox;
    }
    if (lw <= 0) lw = owner.getWidth();
    float lh = 8 * usc;
    for (auto& it : items) lh += (it.title ? 18 : 20) * usc;
    const float lx = owner.getX();
    float ly = up ? owner.getY() - lh - 4 * usc : owner.getBottom() + 4 * usc;
    if (listOpen == 4) ly = juce::jmax(uy(56), juce::jmin(owner.getBottom() + 4 * usc, uy(332 + ex) - lh));   // stays inside the display
    c.colour(th.panel); c.box(lx, ly, lw, lh, 8 * usc);
    c.colour(th.line); c.frame(lx, ly, lw, lh, 8 * usc);
    addHit(hList, lx, ly, lw, lh);
    float iy = ly + 4 * usc;
    int k = 0;
    for (auto& it : items) {
        if (it.title) {
            c.colour(th.dim); c.textL(fonts.note, it.text, lx + 12 * usc, iy + 10 * usc);
            iy += 18 * usc;
            continue;
        }
        addHit(hListItem0 + k, lx, iy, lw, 20 * usc);
        if (hover == hListItem0 + k) { c.colour(th.hover); c.box(lx + 4 * usc, iy, lw - 8 * usc, 20 * usc, 6 * usc); }
        if (it.on) { c.colour(th.blue); c.textL(fonts.tabOn, it.text, lx + 12 * usc, iy + 10 * usc); }
        else { c.colour(th.text); c.textL(fonts.small, it.text, lx + 12 * usc, iy + 10 * usc); }
        iy += 20 * usc;
        k++;
    }
}

void BinocularEditor::listPick(int k) {
    if (listOpen == 1) proc.set(bn::pSpan, k);
    else if (listOpen == 2) proc.set(bn::pMeasure, k);
    else if (listOpen == 3) proc.set(bn::pTarget, kTargetOrder[juce::jlimit(0, 9, k)]);
    else if (listOpen == 4) { if (k < 6) proc.set(bn::pFps, k); else proc.set(bn::pTcStart, k - 6); }
    else if (listOpen == 5) proc.set(bn::pDownmix, k);
}

// ---------------------------------------------------------------- help panel: over the display, a short guide and "Read Manual"
void BinocularEditor::drawHelp(bn::Canvas& c) {
    if (!helpOpen) return;
    const float dw = (648 + wx) * usc, dh = (286 + ex) * usc;
    const float pw = juce::jmin(560 * usc, dw - 24 * usc), ph = juce::jmin(262 * usc, dh - 16 * usc);
    const float px = ux(16) + (dw - pw) / 2, py = uy(52) + (dh - ph) / 2;
    addHit(hHelpPanel, px, py, pw, ph);
    c.colour(th.panel2); c.box(px, py, pw, ph, 10 * usc);
    c.colour(th.line); c.frame(px, py, pw, ph, 10 * usc);
    c.colour(th.text); c.textL(fonts.helpTitle, "Binocular - Help", px + 20 * usc, py + 26 * usc);
    // close (x), top right of the panel
    addHit(hHelpClose, px + pw - 34 * usc, py + 12 * usc, 26 * usc, 26 * usc);
    c.colour(hover == hHelpClose ? th.text : th.muted);
    c.line(px + pw - 27 * usc, py + 19 * usc, px + pw - 15 * usc, py + 31 * usc, 1.4f * usc);
    c.line(px + pw - 15 * usc, py + 19 * usc, px + pw - 27 * usc, py + 31 * usc, 1.4f * usc);
    float y = py + 56 * usc;
    const float x = px + 20 * usc;
    c.colour(th.text);
    c.textL(fonts.small, "Correlation: how left and right relate in each band. +1 in phase, 0 unrelated, -1 out of phase.", x, y); y += 22 * usc;
    c.textL(fonts.small, "Mono loss: how many dB each band drops when the sides are summed to mono.", x, y); y += 22 * usc;
    c.textL(fonts.small, "Goniometer: the stereo image as a trace, the correlation of the full signal below.", x, y); y += 22 * usc;
    c.textL(fonts.small, "Loudness: momentary, short-term, integrated, range and true peak against a target. The graph", x, y); y += 16 * usc;
    c.textL(fonts.small, "shows the history, as much time as set top left. Timecode measures by the DAW position.", x, y); y += 22 * usc;
    c.textL(fonts.small, "On a 5.1 track Binocular switches to 5.1 by itself: compare front, surround or the downmix.", x, y); y += 22 * usc;
    c.colour(th.muted); c.textL(fonts.small, "Move the mouse over any control: its explanation shows at the bottom left.", x, y);
    // Read Manual
    const float mw = 120 * usc, mh = 24 * usc, mx = px + pw - mw - 20 * usc, my = py + ph - mh - 16 * usc;
    addHit(hManual, mx, my, mw, mh);
    c.colour(hover == hManual ? Theme::mix(th.accent, 0xFFFFFF, 0.25) : th.accent); c.box(mx, my, mw, mh, mh / 2);
    c.colour(th.accentText); c.textC(fonts.tabOn, "Read Manual", mx + mw / 2, my + mh / 2);
    // colors: standard or colorblind-safe
    const bool safe = proc.get(bn::pColors) >= 0.5;
    const String ct = safe ? "Colors: Colorblind-safe" : "Colors: Standard";
    const float cw = bn::Canvas::width(fonts.small, ct) + 24 * usc;
    addHit(hColors, x, my, cw, mh);
    button(c, x, my, cw, mh, safe, hover == hColors, ct);
}

// ---------------------------------------------------------------- footer: the same in every dmajorsound plugin
void BinocularEditor::drawFooter(bn::Canvas& c) {
    const float wp = wx * usc;
    c.colour(th.line); c.hline(ux(16), uy(496 + ex), (648 + wx) * usc);
    const float fy = uy(508 + ex);
    c.colour(th.dim); c.textL(fonts.note, kVersionText, ux(16), fy);
    // right: the dmajorsound mark in the corner, dmajorsound.com to its left
    static juce::Image mark;
    if (!mark.isValid()) {
        mark = juce::Image(juce::Image::SingleChannel, 48, 32, true);
        juce::Image::BitmapData bd(mark, juce::Image::BitmapData::writeOnly);
        for (int r = 0; r < 32; r++)
            for (int col = 0; col < 48; col++) {
                const char ch = bn::kMark2[r][col];
                const int v = ch <= '9' ? ch - '0' : ch - 'a' + 10;
                *bd.getPixelPointer(col, r) = (juce::uint8)(v * 17);
            }
    }
    const float mw = 24 * usc, mh = 16 * usc, mr = ux(656) + wp;   // clear of the window resize corner
    c.colour(th.muted);
    c.g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    c.g.drawImage(mark, juce::Rectangle<float>(mr - mw, fy - mh / 2, mw, mh), juce::RectanglePlacement::stretchToFit, true);
    const String site = "dmajorsound.com";
    const float siteRight = mr - mw - 8 * usc;
    c.colour(th.dim); c.textR(fonts.note, site, siteRight, fy);

    // a newer Binocular (as in OLAAF 0.8.7), right-aligned left of dmajorsound.com, a light red dot:
    //   "x.y.z is out" (blinks) - click downloads and installs; "Downloading x.y.z ... 42%" (a ring);
    //   "x.y.z downloaded" - click installs; "Download failed" - click tries again. Right-click: the menu.
    const int ver = bn::updateNewerVersion();
    if (ver > 0) {
        double prog = 0;
        const int st = bn::updateDownloadState(&prog, nullptr, nullptr);
        const String v = String(ver / 10000) + "." + String(ver / 100 % 100) + "." + String(ver % 100);
        const String ell = String::fromUTF8(" \xe2\x80\xa6");
        const String t = st == 1 ? (prog >= 0 ? "Downloading " + v + ell + " " + String((int)std::floor(prog * 100 + 0.5)) + "%" : "Downloading " + v + ell)
                       : st == 2 ? v + " downloaded" : st == 3 ? String("Download failed") : v + " is out";
        const float tw = bn::Canvas::width(fonts.note, t);
        const float right = siteRight - bn::Canvas::width(fonts.note, site) - 16 * usc;
        const float dx = right - tw - 11 * usc;
        addHit(hUpdate, dx - 4 * usc, fy - 9 * usc, tw + 19 * usc, 18 * usc);
        c.colour(0xF06E6E);
        const float r = 3.5f * usc, cx = dx + r;
        if (st == 1) { if (prog >= 0.5) c.disc(cx, fy, r); else c.ring(cx, fy, r, c.hair * 1.5f); }   // downloading: a ring that fills up
        else if (st == 2 || st == 3) c.disc(cx, fy, r);                                              // there or failed: a full dot
        else if (((int)std::floor(juce::Time::getMillisecondCounterHiRes() / 700.0)) % 2 == 0) c.disc(cx, fy, r);   // blinks every 0.7 s
        else c.ring(cx, fy, r, c.hair * 1.5f);
        c.colour(th.dark ? 0xFF7A6Eu : 0xC8423Bu); c.textL(fonts.note, t, dx + 11 * usc, fy);
    }
}

// ---------------------------------------------------------------- status line, above the footer line: explains whatever is under the mouse
void BinocularEditor::drawStatus(bn::Canvas& c) {
    const float x = ux(16), y = uy(483 + ex);
    const int view = (int)std::floor(proc.get(bn::pView) + 0.5);
    const bool frozen = proc.get(bn::pFreeze) >= 0.5, dark = th.dark, surround = an.channels == 6;
    const int measure = (int)std::floor(proc.get(bn::pMeasure) + 0.5);
    const bool selectivity = dragId == hSelectivity || (hover == hBands && altDown);
    String s;
    juce::uint32 col = th.text;
    const int hv = hover;
    if (hv == hBands) s = selectivity ? "Selectivity (Alt-drag): higher makes each band narrower and sharper, lower blends neighbors."
                                      : "Bands: how many frequency bands the meter splits the signal into. Alt-drag sets the selectivity.";
    else if (hv == hAveraging) s = "Averaging: mids and highs need at least 0.1 s, the bass about 0.3 s.";
    else if (hv == hHold) s = "Hold lows: keeps the lowest reading of each band for 3 s. Shift-click holds until you click the display.";
    else if (hv == hPower) s = frozen ? "Off: the meter shows its last picture. Click to switch it on." : "On: the meter runs. Off keeps the last picture to read it. The audio always passes untouched.";
    else if (hv == hPair0) s = "Front: compares L and R of the 5.1 track.";
    else if (hv == hPair0 + 1) s = "Surround: compares Ls and Rs of the 5.1 track.";
    else if (hv == hPair0 + 2) s = String::fromUTF8("Downmix: compares Lo and Ro of the stereo downmix (centre and surrounds at -3 dB, no LFE), as a stereo listener hears it.");
    else if (hv == hSurroundTag) s = surround ? "5.1: Binocular found a 5.1 track and measures all six channels. On a stereo track it measures stereo."
                                              : "Stereo: Binocular found a stereo track. On a 5.1 track it switches to 5.1 by itself.";
    else if (hv == hTheme) s = dark ? "Dark mode. Click for light." : "Light mode. Click for dark.";
    else if (hv == hHelp) s = helpOpen ? "Help: click to close it." : "Help: a short guide and the manual.";
    else if (hv == hManual) s = "Read Manual: opens the Binocular manual on dmajorsound.com in your browser.";
    else if (hv == hHelpPanel) s = "Esc or the x closes the help.";
    else if (hv == hHelpClose) s = "Close the help.";
    else if (hv == hUpdate) {
        const int ver = bn::updateNewerVersion();
        const String v = String(ver / 10000) + "." + String(ver / 100 % 100) + "." + String(ver % 100);
        const int st = bn::updateDownloadState(nullptr, nullptr, nullptr);
        const int err = bn::updateDownloadError();
        if (st == 1) s = "Binocular " + v + " is coming from GitHub into your download folder.";
        else if (st == 2) s = "Binocular " + v + " is in your download folder. Click to install it (quit your DAW before you click Install). Right-click for more.";
        else if (st == 3) s = "Binocular " + v + " could not be downloaded (" + String(err == 1 ? "no connection" : err == 2 ? "GitHub answered with an error" : err == 3 ? "nothing came for 30 seconds"
                              : err == 4 ? "it arrived incomplete" : err == 5 ? "the download folder cannot be written to" : "unknown") + "). Click to try again, right-click to open the page instead.";
        else if (!bn::updateAssetUrl().empty()) s = "Binocular " + v + " is out. Click to download and install it. Right-click: download only, the download folder, what's new.";
        else s = "Binocular " + v + " is out. Click to open the download page.";
    }
    else if (hv == hResetPlugin) s = "Reset plugin: every setting back to its default, holds, reference and loudness cleared.";
    else if (hv == hTabCorr) s = "Correlation: +1 in phase, 0 unrelated, -1 out of phase.";
    else if (hv == hTabMono) s = "Mono loss: how many dB each band drops when the two sides are summed to mono.";
    else if (hv == hTabGonio) s = surround ? "Goniometer: in 5.1 a vectorscope of all five speakers. Correlation stays below." : "Goniometer: the stereo image as a trace. Correlation stays below.";
    else if (hv == hTabLoud) s = "Loudness: momentary, short-term, integrated, range and true peak, with a graph of the history.";
    else if (hv == hGonio) s = "Upright line: mono. Lying flat: out of phase. Round cloud: wide and unrelated. Leaning: one side louder.";
    else if (hv == hBalance) s = surround ? "Left / right: L and Ls against R and Rs." : "Balance: how much louder one side is than the other.";
    else if (hv == hFrontRear) s = "Front / rear: L, C and R against Ls and Rs.";
    else if (hv == hVector) s = "Vectorscope: where the sound comes from. Blue shape: the level of each speaker. Trace: at the edge one speaker, in the middle all.";
    else if (hv == hWidth) s = "Width: side against mid. Very low is mono, around 0 dB is very wide, above 0 dB the sides work against each other.";
    else if (hv == hRange) s = "Range: how deep the mono loss scale reaches. Click for 24, 48 or 96 dB.";
    else if (hv == hReference) s = refOn ? "Reference: click to clear the line." : "Reference: keeps the current curve as a line, to compare before and after.";
    else if (hv == hOffset) s = "Offset: the time shift between the two sides, found across the spectrum. Info only, the audio is not moved.";
    else if (hv == hTarget) s = "Target: the delivery spec the verdict and the colors judge against. Click for the list.";
    else if (listOpen == 3 && hv >= hListItem0 && hv <= hListItem0 + 9) s = String::fromUTF8(kTargets[kTargetOrder[hv - hListItem0]].note);
    else if (hv == hMaxMomentary) s = "Momentary max: the loudest 400 ms since the last reset.";
    else if (hv == hMaxShort) s = "Short-term max: the loudest 3 seconds since the last reset. Some specs limit it, e.g. for commercials.";
    else if (hv == hTimecode || (listOpen == 4 && hv >= hListItem0 && hv <= hListItem0 + 8))
        s = "Timecode: the DAW position at the chosen frame rate, plus a start hour. Click to set them.";
    else if (hv == hDownmix || (listOpen == 5 && hv >= hListItem0 && hv <= hListItem0 + 2))
        s = "Downmix: how Lo/Ro are made. ITU-R BS.775: C and surrounds -3 dB, no LFE. Modern: also LFE at -10 dB. Manual: own gains.";
    else if (hv == hDmxCentre) s = "Center into Lo and Ro. Drag up or down, Shift for fine steps, double-click for -3 dB.";
    else if (hv == hDmxSurround) s = "Ls into Lo and Rs into Ro. Drag up or down, Shift for fine steps, double-click for -3 dB.";
    else if (hv == hDmxLfe) s = "LFE into Lo and Ro. All the way down is off. Double-click: off.";
    else if (hv == hColors) s = "Colors: Colorblind-safe uses a palette that stays apart with red-green color blindness.";
    else if (hv == hMeasure || (listOpen == 2 && hv >= hListItem0 && hv <= hListItem0 + 2))
        s = "Measure: Always (everything that comes in), Playback (only while the DAW plays) or Timecode (by DAW position).";
    else if (hv == hLoudReset) s = "Reset: starts a new loudness measurement. Momentary, short-term, integrated, range, true peak and history.";
    else if (hv == hSpan || (listOpen == 1 && hv >= hListItem0 && hv <= hListItem0 + 6))
        s = "History: how much time the graph shows, from 1 minute up to 5 hours (a whole feature film).";
    else if (hv == hHistory) s = "History: the loudness over time, now on the right. Mint band: target and tolerance. Amber and red louder, blue quieter.";
    else if (hv == hReading0) s = "Momentary: the loudness of the last 400 ms. The M meter next to the graph shows it too.";
    else if (hv == hReading0 + 1) s = "Short-term: the loudness of the last 3 seconds. The S meter next to the graph shows it too.";
    else if (hv == hReading0 + 2) s = surround ? "Integrated: all of the 5.1 measured, with the EBU R128 gates (-70 LUFS, -10 LU). Surrounds +1.5 dB, LFE not counted."
                                                 : "Integrated: the loudness of everything measured, with the EBU R128 gates (-70 LUFS, -10 LU).";
    else if (hv == hReading0 + 3) s = "Dialog: the loudness of the parts Binocular hears as dialog, and their share. Its own detector, close to Dolby's, not the same.";
    else if (hv == hReading0 + 4) s = "Loudness range: how far the quiet and loud parts lie apart (EBU Tech 3342, 10th to 95th percentile).";
    else if (hv == hReading0 + 5) s = "True peak max: the highest peak, also between the samples (oversampled as in ITU-R BS.1770).";
    else if (hv == hReading0 + 6) s = "Timecode: the position of the DAW, 25 frames per second, counted from the project start.";
    else if (hv == hVerdict) s = "The verdict: what the numbers mean for the chosen target.";
    else if (hv == hPeaks) s = surround ? "Peak meters: L, R, C, LFE, Ls, Rs in dBFS, true peak. Red is above the target's limit."
                                         : "Peak meters: left and right in dBFS, true peak (also between the samples). Red is above the target's limit.";
    else if (hv == hBandsArea && hoverBand >= 0) {
        const int b = hoverBand;
        const String head = frequency(an.centerHz[(size_t)b]) + " (" + frequency(an.lowHz * std::pow(an.highHz / an.lowHz, (double)b / an.bands))
                          + " to " + frequency(an.lowHz * std::pow(an.highHz / an.lowHz, (double)(b + 1) / an.bands)) + "): ";
        if (an.hasSignal[(size_t)b])
            s = head + signedValue(an.corr[(size_t)b]) + ", mono " + oneDecimal(an.monoLoss[(size_t)b]) + " dB, level "
              + String((int)std::floor(an.level[(size_t)b] + 0.5)) + " dB." + (proc.get(bn::pHold) >= 0.5 ? " Click clears the lows." : "");
        else s = head + "nothing to compare in this band.";
    }
    else if (hv == hTotal) s = "Total: the full signal, as on a classic correlation meter. Click clears the low.";
    else if (frozen) s = "Off: the display holds still. Move over it to read it, switch on to go on.";
    else if (view == 3) s = measure >= 2 ? "Timecode: only what plays is measured, by its position in the DAW. Playing a part again replaces it."
                          : measure >= 1 ? "Playback: measures while the DAW plays and pauses when it stops."
                                         : "Always: measures everything that comes in, also when the DAW stands still.";
    else if (an.offsetFound && an.polarity < 0) { col = th.red; s = "Polarity flipped: the two sides work against each other. Flip the phase of one side."; }
    else if ((an.levelL > -100) != (an.levelR > -100)) s = "Only one side carries signal, so there is nothing to compare.";
    else s = view == 2 && surround ? "5.1 vectorscope: the five speakers around you, their levels as a shape, the trace shows where the sound sits."
           : view == 2 ? "Goniometer: the trace takes the color of the correlation. The bar below is the correlation of the full signal."
           : view == 1 ? "Mono loss: 0 dB keeps its level in mono, -3 dB is two unrelated sides, deeper means the sides cancel."
                       : "+1 in phase, 0 unrelated, -1 out of phase (cancels in mono). Mint and blue hold up in mono, coral and red lose level.";
    c.colour(col); c.textL(fonts.small, s, x, y);
}

// ---------------------------------------------------------------- mouse and keys
void BinocularEditor::mouseDown(const juce::MouseEvent& e) {
    grabKeyboardFocus();
    mousePos = e.position;
    const int id = hitAt(e.position);
    if (listOpen) {
        // an open list: a click picks an item or closes it, nothing below reacts
        if (id >= hListItem0 && id <= hListItem0 + 9) listPick(id - hListItem0);
        const int owner = id == hSpan ? 1 : id == hMeasure ? 2 : id == hTarget ? 3 : id == hTimecode ? 4 : id == hDownmix ? 5 : 0;
        listOpen = owner && owner != listOpen ? owner : 0;
        repaint();
        return;
    }
    if (e.mods.isRightButtonDown()) { if (id == hUpdate) updateMenu(); return; }
    const auto clear = [this] { an.clearLows = true; };
    switch (id) {
        case hBands: case hAveraging: {
            const int k = (id == hBands && e.mods.isAltDown()) ? (int)hSelectivity : id;   // Alt on Bands: selectivity
            const double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
            if (e.mods.isCommandDown() || (k == lastClickId && now - lastClickTime < 0.3)) { knobDefault(k); dragId = 0; }
            else {
                dragId = k; dragY = e.position.y; dragValue = knobFraction(k);
                proc.param(k == hBands ? bn::pBands : k == hSelectivity ? bn::pSelectivity : bn::pAveraging)->beginChangeGesture();
            }
            lastClickId = k; lastClickTime = now;
            break;
        }
        case hHold: {
            const double h = proc.get(bn::pHold);
            proc.set(bn::pHold, e.mods.isShiftDown() ? (h >= 1.5 ? 1 : 2) : (h >= 0.5 ? 0 : 1));
            clear();
            break;
        }
        case hHelp: helpOpen = !helpOpen; break;
        case hManual: bn::openWebPage(kManualUrl); break;
        case hHelpClose: helpOpen = false; break;
        case hUpdate: updateClick(); break;
        case hPower: proc.set(bn::pFreeze, proc.get(bn::pFreeze) >= 0.5 ? 0 : 1); break;
        case hPair0: case hPair0 + 1: case hPair0 + 2: proc.set(bn::pPair, id - hPair0); clear(); break;
        case hTheme: proc.set(bn::pTheme, th.dark ? 1 : 0); break;
        case hResetPlugin:
            for (const auto& info : bn::kParams)
                if (info.number != bn::pTheme && info.number != bn::pView) proc.set(info.number, info.def);
            clear(); refOn = false; an.resetLoudness = true;
            break;
        case hBandsArea: case hTotal: clear(); break;
        case hTabCorr: proc.set(bn::pView, 0); break;
        case hTabMono: proc.set(bn::pView, 1); break;
        case hTabGonio: proc.set(bn::pView, 2); break;
        case hTabLoud: proc.set(bn::pView, 3); break;
        case hRange: { const double r = proc.get(bn::pRange); proc.set(bn::pRange, r >= 1.5 ? 0 : std::floor(r + 0.5) + 1); break; }
        case hReference:
            if (refOn) refOn = false;
            else {
                for (int i = 0; i < an.bands; i++) { refCorr[(size_t)i] = an.corr[(size_t)i]; refMono[(size_t)i] = an.monoLoss[(size_t)i]; refValid[(size_t)i] = an.hasSignal[(size_t)i]; }
                refBands = an.bands; refSetup = an.setupCount; refOn = true;
            }
            break;
        case hMeasure: listOpen = 2; break;
        case hSpan: listOpen = 1; break;
        case hTarget: listOpen = 3; break;
        case hTimecode: listOpen = 4; break;
        case hDownmix: listOpen = 5; break;
        case hColors: proc.set(bn::pColors, proc.get(bn::pColors) >= 0.5 ? 0 : 1); break;
        case hDmxCentre: case hDmxSurround: case hDmxLfe: {
            const int number = id == hDmxCentre ? bn::pDmxCentre : id == hDmxSurround ? bn::pDmxSurround : bn::pDmxLfe;
            auto* p = proc.param(number);
            const double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
            if (now - lastClickTime < 0.35 && lastClickId == id) {   // double-click: back to the default
                proc.set(number, id == hDmxLfe ? -40 : -3);
                lastClickId = 0;
                break;
            }
            lastClickId = id; lastClickTime = now;
            p->beginChangeGesture();
            dragId = id; dragY = e.position.y; dragValue = p->convertTo0to1((float)proc.get(number));
            break;
        }
        case hLoudReset: an.resetLoudness = true; break;   // a new loudness measurement
        default: break;
    }
    repaint();
}

void BinocularEditor::mouseDrag(const juce::MouseEvent& e) {
    mousePos = e.position;
    if (!dragId) return;
    dragValue += (dragY - e.position.y) / (160 * usc) * (e.mods.isShiftDown() ? 0.1 : 1.0);
    dragValue = juce::jlimit(0.0, 1.0, dragValue);
    dragY = e.position.y;
    if (dragId >= hDmxCentre && dragId <= hDmxLfe) {
        auto* p = proc.param(dragId == hDmxCentre ? bn::pDmxCentre : dragId == hDmxSurround ? bn::pDmxSurround : bn::pDmxLfe);
        const auto& r = p->getNormalisableRange();
        const float v = std::round(r.convertFrom0to1((float)dragValue) * 2) / 2;   // half-dB steps
        p->setValueNotifyingHost(r.convertTo0to1(v));
        return;
    }
    knobSet(dragId, dragValue, false);
}

void BinocularEditor::mouseUp(const juce::MouseEvent& e) {
    mousePos = e.position;
    if (dragId) proc.param(dragId == hBands ? bn::pBands : dragId == hSelectivity ? bn::pSelectivity : dragId == hAveraging ? bn::pAveraging
                           : dragId == hDmxCentre ? bn::pDmxCentre : dragId == hDmxSurround ? bn::pDmxSurround : bn::pDmxLfe)->endChangeGesture();
    dragId = 0;
}

void BinocularEditor::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) {
    const int id = hitAt(e.position);
    if (id != hBands && id != hAveraging) return;
    const int k = (id == hBands && e.mods.isAltDown()) ? (int)hSelectivity : id;
    const double dir = (w.deltaY * (w.isReversed ? -1 : 1)) > 0 ? 1 : -1;
    knobSet(k, knobFraction(k) + dir * (k == hBands ? 1.0 / 9 : e.mods.isShiftDown() ? 0.005 : 0.03), true);
}

bool BinocularEditor::keyPressed(const juce::KeyPress& k) {
    if (k == juce::KeyPress::escapeKey) {
        if (listOpen) listOpen = 0;
        else if (helpOpen) helpOpen = false;
        else return false;
        repaint();
        return true;
    }
    return false;
}

// ---------------------------------------------------------------- update note
// a click: download and install (or install what is there, or try again); without an installer for this system: the release page
void BinocularEditor::updateClick() {
    if (bn::updateNewerVersion() <= 0) return;
    std::string path;
    const int dl = bn::updateDownloadState(nullptr, &path, nullptr);
    if (bn::updateAssetUrl().empty()) bn::openWebPage(bn::updateUrl());
    else if (dl == 2) bn::openInstaller(path);
    else if (dl != 1) bn::updateDownloadStart(true);
}

// a right-click: download and install, download only, where to, what's new
void BinocularEditor::updateMenu() {
    if (bn::updateNewerVersion() <= 0) return;
    std::string path;
    const int dl = bn::updateDownloadState(nullptr, &path, nullptr);
    const bool direct = !bn::updateAssetUrl().empty();
    const String dir(bn::updateFolder());
    const String shortDir = juce::File(dir).getFileName().isNotEmpty() ? juce::File(dir).getFileName() : dir;
    juce::PopupMenu m;
    if (dl == 2) {
        m.addItem(3, "Install now");
       #if JUCE_MAC
        m.addItem(4, "Show in Finder");
       #else
        m.addItem(4, "Show in Explorer");
       #endif
    } else {
        m.addItem(1, "Download and install", direct && dl != 1);
        m.addItem(2, "Download only (install later)", direct && dl != 1);
    }
    m.addSeparator();
    m.addItem(5, "Download to: " + shortDir + String::fromUTF8(" \xe2\x80\xa6"), dl != 1);
    m.addItem(6, "What's new (release page)");
    juce::Component::SafePointer<BinocularEditor> safe(this);
    m.showMenuAsync(juce::PopupMenu::Options().withMousePosition(), [safe, path, dir](int cmd) {
        if (cmd == 1 || cmd == 2) bn::updateDownloadStart(cmd == 1);
        else if (cmd == 3) bn::openInstaller(path);
        else if (cmd == 4) bn::revealInFileBrowser(path);
        else if (cmd == 6) bn::openWebPage(bn::updateUrl());
        else if (cmd == 5 && safe != nullptr) {
            safe->chooser = std::make_unique<juce::FileChooser>("Binocular: choose where updates are downloaded to", juce::File(dir));
            safe->chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                                       [](const juce::FileChooser& fc) {
                                           const juce::File f = fc.getResult();
                                           if (f.isDirectory()) bn::setUpdateFolder(f.getFullPathName().toStdString(), true);
                                       });
        }
    });
}
