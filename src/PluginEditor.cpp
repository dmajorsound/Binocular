#include "PluginEditor.h"

BinocularEditor::BinocularEditor(BinocularProcessor& p)
    : AudioProcessorEditor(&p), proc(p), gfx(*p.engine.gen) {
    setOpaque(true);
    setWantsKeyboardFocus(true);    // options:want_all_kb
    setResizable(true, true);
    setResizeLimits(340, 260, 4000, 3000);
    setSize(680, 520);              // @gfx 680 520
    gfx.isKeyDown = [](int c) {
        using K = juce::KeyPress;
        switch (c) {
            case 1818584692: return K::isKeyCurrentlyDown(K::leftKey);    // 'left'
            case 1919379572: return K::isKeyCurrentlyDown(K::rightKey);   // 'rght'
            case 30064:      return K::isKeyCurrentlyDown(K::upKey);      // 'up'
            case 1685026670: return K::isKeyCurrentlyDown(K::downKey);    // 'down'
            default: break;
        }
        if (c > 0 && c < 256) {
            return K::isKeyCurrentlyDown(c) || K::isKeyCurrentlyDown(juce::CharacterFunctions::toUpperCase((juce::juce_wchar)c));
        }
        return false;
    };
    gfx.onCursor = [this](int id) {
        if (id == lastCursor) return;   // the script sets the cursor every frame; only real changes go to the OS
        lastCursor = id;
        using C = juce::MouseCursor;
        C::StandardCursorType t = C::NormalCursor;
        switch (id) {
            case 32513: t = C::IBeamCursor; break;
            case 32642: t = C::BottomRightCornerResizeCursor; break;
            case 32643: t = C::BottomLeftCornerResizeCursor; break;
            case 32644: t = C::LeftRightResizeCursor; break;
            case 32645: t = C::UpDownResizeCursor; break;
            case 32646: t = C::UpDownLeftRightResizeCursor; break;
            case 32649: t = C::PointingHandCursor; break;
            default: break;
        }
        setMouseCursor(t);
    };
    diagFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("binocular-debug.txt");
    if (!diagFile.existsAsFile()) diagFile = juce::File();
    else diagFile.appendText("\n--- Binocular editor opened " + juce::Time::getCurrentTime().toString(true, true) + "\n");
    startTimerHz(60);   // 60 frames per second: smoother meters and goniometer
}

void BinocularEditor::diagTick() {
    const double now = juce::Time::getMillisecondCounterHiRes();
    if (diagT0 == 0) diagT0 = now;
    if (now - diagT0 < 1000.0) return;
    auto& d = proc.diag;
    const int nc = d.notifyCount.exchange(0), sc = d.stateCount.exchange(0);
    const double nm = d.notifyMs.exchange(0), nmax = d.notifyMaxMs.exchange(0), sm = d.stateMs.exchange(0);
    juce::String line;
    line << juce::Time::getCurrentTime().toString(false, true, true) << "  frames " << dFrames
         << "  gfx avg " << juce::String(dFrames ? dGfx / dFrames : 0, 1) << " max " << juce::String(dGfxMax, 1)
         << " ms  frame gap avg " << juce::String(dFrames ? dGap / dFrames : 0, 1) << " max " << juce::String(dGapMax, 1)
         << " ms  paints " << dPaints << " avg " << juce::String(dPaints ? dPaint / dPaints : 0, 1) << " max " << juce::String(dPaintMax, 1)
         << " ms  mouse events " << dEvents << "  host notify " << nc << " x, " << juce::String(nm, 1) << " ms (max " << juce::String(nmax, 1)
         << ")  state " << sc << " x, " << juce::String(sm, 1) << " ms  scale " << juce::String(scale(), 2)
         << "  size " << gfx.screen().getWidth() << "x" << gfx.screen().getHeight() << "\n";
    diagFile.appendText(line);
    diagT0 = now; dFrames = dPaints = dEvents = 0; dGfx = dGfxMax = dGap = dGapMax = dPaint = dPaintMax = 0;
}

BinocularEditor::~BinocularEditor() { stopTimer(); }

double BinocularEditor::scale() const {
    const auto* g = proc.engine.gen.get();
    return g->v_gfx_ext_retina > 0 ? (double)lastPaintScale : 1.0;
}

void BinocularEditor::timerCallback() {
    const double s = scale();
    const int w = juce::roundToInt(getWidth() * s), h = juce::roundToInt(getHeight() * s);
    auto* g = proc.engine.gen.get();
    if (g->v_gfx_ext_retina > 0) g->v_gfx_ext_retina = s;
    g->v_mouse_cap = liveCap | latchedButtons;   // a press seen since the last frame counts for this frame
    const double t0 = juce::Time::getMillisecondCounterHiRes();
    gfx.beginFrame(w, h);
    proc.engine.runGfx(&gfx, w, h);
    gfx.endFrame();
    if (diagFile != juce::File()) {
        const double t1 = juce::Time::getMillisecondCounterHiRes();
        dFrames++; dGfx += t1 - t0; dGfxMax = juce::jmax(dGfxMax, t1 - t0);
        if (lastTick > 0) { dGap += t0 - lastTick; dGapMax = juce::jmax(dGapMax, t0 - lastTick); }
        lastTick = t0;
        diagTick();
    }
    latchedButtons = 0;
    g->v_mouse_cap = liveCap;                     // the next frame sees the release
    repaint();
}

void BinocularEditor::paint(juce::Graphics& g) {
    const double pt0 = juce::Time::getMillisecondCounterHiRes();
    const juce::ScopeGuard sg { [&] { if (diagFile != juce::File()) { const double dt = juce::Time::getMillisecondCounterHiRes() - pt0; dPaints++; dPaint += dt; dPaintMax = juce::jmax(dPaintMax, dt); } } };
    lastPaintScale = juce::jmax(1.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    const juce::Image& img = gfx.screen();
    if (!img.isValid()) { g.fillAll(juce::Colours::black); return; }
    if (img.getWidth() == getWidth() && img.getHeight() == getHeight()) { g.drawImageAt(img, 0, 0); return; }
    g.setImageResamplingQuality(juce::Graphics::mediumResamplingQuality);
    g.drawImage(img, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
}

void BinocularEditor::mouse(const juce::MouseEvent& e, bool up) {
    dEvents++;
    auto* g = proc.engine.gen.get();
    const double s = scale();
    g->v_mouse_x = std::floor(e.position.x * s);
    g->v_mouse_y = std::floor(e.position.y * s);
    const auto m = e.mods;
    int cap = 0;
    if (!up) {
        if (m.isLeftButtonDown()) cap |= 1;
        if (m.isRightButtonDown()) cap |= 2;
        if (m.isMiddleButtonDown()) cap |= 64;
    }
#if JUCE_MAC
    if (m.isCommandDown()) cap |= 4;
    if (m.isCtrlDown()) cap |= 32;
#else
    if (m.isCtrlDown()) cap |= 4;
#endif
    if (m.isShiftDown()) cap |= 8;
    if (m.isAltDown()) cap |= 16;
    liveCap = cap;
    latchedButtons |= cap & (1 | 2 | 64);
    g->v_mouse_cap = cap | latchedButtons;
}

void BinocularEditor::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) {
    mouse(e);
    auto* g = proc.engine.gen.get();
    g->v_mouse_wheel += w.deltaY * (w.isReversed ? -1.0 : 1.0) * 857.0;   // one notch (~0.14) = 120 as in REAPER
    g->v_mouse_hwheel += w.deltaX * 857.0;
}

bool BinocularEditor::keyPressed(const juce::KeyPress& k) {
    using K = juce::KeyPress;
    const int kc = k.getKeyCode();
    int c = 0;
    if (kc == K::returnKey) c = 13;
    else if (kc == K::escapeKey) c = 27;
    else if (kc == K::backspaceKey) c = 8;
    else if (kc == K::tabKey) c = 9;
    else if (kc == K::deleteKey) c = 6579564;        // 'del'
    else if (kc == K::leftKey) c = 1818584692;
    else if (kc == K::rightKey) c = 1919379572;
    else if (kc == K::upKey) c = 30064;
    else if (kc == K::downKey) c = 1685026670;
    else if (kc == K::homeKey) c = 1752132965;       // 'home'
    else if (kc == K::endKey) c = 6647396;           // 'end'
    else if (kc == K::pageUpKey) c = 1885828464;     // 'pgup'
    else if (kc == K::pageDownKey) c = 1885824110;   // 'pgdn'
    else if (kc == K::insertKey) c = 6909555;        // 'ins'
    else {
        const juce::juce_wchar t = k.getTextCharacter();
        const bool ctrl = k.getModifiers().isCtrlDown() || k.getModifiers().isCommandDown();
        if (ctrl && ((t >= 'a' && t <= 'z') || (t >= 'A' && t <= 'Z'))) c = (int)(juce::CharacterFunctions::toLowerCase(t) - 'a' + 1);
        else if (t >= 32) c = (int)t;
    }
    if (c) { gfx.keyQueue.push_back(c); if (gfx.keyQueue.size() > 64) gfx.keyQueue.pop_front(); }
    return true;   // all keys go to the plugin while it has the focus
}
