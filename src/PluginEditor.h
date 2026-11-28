#pragma once
#include "PluginProcessor.h"
#include "Look.h"

// Binocular's window: header, display (correlation, mono loss, goniometer, loudness), total, controls, footer.
// Everything is laid out in units of a 680 x 520 window and scaled; extra width and height go to the display.
class BinocularEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit BinocularEditor(BinocularProcessor&);
    ~BinocularEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void mouseMove(const juce::MouseEvent& e) override { mousePos = e.position; }
    void mouseExit(const juce::MouseEvent&) override { mousePos = { -1000, -1000 }; }
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override;
    bool keyPressed(const juce::KeyPress& k) override;

    std::unique_ptr<juce::FileChooser> chooser;   // "Download to:" in the update menu

private:
    // what the mouse can point at
    enum Hit {
        hNone = 0, hBands = 1, hSelectivity = 2, hAveraging = 3, hHold = 4, hPower = 10,
        hTheme = 13, hResetPlugin = 14, hPair0 = 15, hSurroundTag = 18, hBandsArea = 20, hTotal = 21, hGonio = 22, hBalance = 23, hWidth = 24, hVector = 25, hFrontRear = 26, hDmxCentre = 27, hDmxSurround = 28, hDmxLfe = 29, hDownmix = 37,
        hTabCorr = 30, hTabMono = 31, hRange = 32, hReference = 33, hOffset = 34, hTabGonio = 35, hTabLoud = 36,
        hUpdate = 40, hHelp = 41, hManual = 42, hHelpPanel = 43, hHelpClose = 44,
        hColors = 45, hTarget = 50, hMaxMomentary = 51, hMaxShort = 52, hTimecode = 53, hMeasure = 56, hLoudReset = 57, hSpan = 58, hHistory = 59, hList = 69, hListItem0 = 70,
        hReading0 = 80, hPeaks = 87, hVerdict = 89
    };
    struct HitArea { int id; juce::Rectangle<float> r; bool round; };

    void timerCallback() override;
    int hitAt(juce::Point<float> p) const;
    void addHit(int id, float x, float y, float w, float h) { hits.push_back({ id, { x, y, w, h }, false }); }
    void addHitCircle(int id, float cx, float cy, float r) { hits.push_back({ id, { cx - r, cy - r, 2 * r, 2 * r }, true }); }

    // the parts of the window
    void drawHeader(bn::Canvas& c);
    void drawBands(bn::Canvas& c);
    void drawGoniometer(bn::Canvas& c);
    void drawVectorscope(bn::Canvas& c);
    void drawLoudness(bn::Canvas& c);
    void drawTotal(bn::Canvas& c);
    void drawControls(bn::Canvas& c);
    void drawList(bn::Canvas& c);
    void listPick(int k);
    bool downmixShown() const;
    void drawDownmix(bn::Canvas& c, float x, float y);
    void drawHelp(bn::Canvas& c);
    void drawFooter(bn::Canvas& c);
    void drawStatus(bn::Canvas& c);

    void knob(bn::Canvas& c, int id, float cx, float cy, float r, double f, const juce::String& label, const juce::String& value);
    void button(bn::Canvas& c, float x, float y, float w, float h, bool on, bool hot, const juce::String& text);
    void dropdown(bn::Canvas& c, float x, float y, float w, float h, bool hot, const juce::String& text, const juce::Font& f);

    // knobs: 0..1 on their track
    double knobFraction(int id) const;
    void knobSet(int id, double f, bool gesture);
    void knobDefault(int id);

    void updateClick();
    void updateMenu();

    BinocularProcessor& proc;
    bn::Analyzer& an;
    bn::Theme th;
    bn::Fonts fonts;

    // layout of the last frame (units -> pixels)
    float usc = 1, ox = 0, wx = 0, ex = 0;
    float ux(float v) const { return ox + v * usc; }
    float uy(float v) const { return v * usc; }

    std::vector<HitArea> hits, lastHits;
    juce::Point<float> mousePos { -1000, -1000 };
    int hover = 0;
    bool altDown = false;

    // interaction
    int dragId = 0;
    float dragY = 0;
    double dragValue = 0;
    int lastClickId = 0;
    double lastClickTime = 0;
    bool helpOpen = false;
    int listOpen = 0;                          // 1: history span, 2: measure, 3: target, 4: timecode, 5: downmix
    juce::Rectangle<float> spanBox, measureBox, targetBox, tcBox, downmixBox;
    int hoverBand = -1;

    // reference curve (captured from the bands)
    bool refOn = false;
    int refBands = 0, refSetup = -1;
    std::array<double, bn::Analyzer::kMaxBands> refCorr {}, refMono {};
    std::array<bool, bn::Analyzer::kMaxBands> refValid {};

    // goniometer: the trace draws into an image that fades every frame (afterglow)
    juce::Image gonioImage;
    juce::uint32 gonioBg = 0;
    int gonioRead = 0;
    juce::Image vecImage;
    juce::uint32 vecBg = 0;
    int vecRead = 0;
    double shapeTop = -120;                                        // the loudest speaker, for the shape's scale
    double shapeDb[6] = { -120, -120, -120, -120, -120, -120 };   // the speaker shape: up at once, down at 24 dB per second
    double lastVecX = std::numeric_limits<double>::quiet_NaN(), lastVecY = 0;
    double gonioPeak = 0;

    // peak meters
    double peakDb[6] = { -120, -120, -120, -120, -120, -120 }, holdDb[6] = { -120, -120, -120, -120, -120, -120 }, holdTime[6] = {};
    std::array<float, bn::Analyzer::kSteps> stepY {};

    double lastFrame = 0, frameDt = 1.0 / 60;
    bool sizeReady = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BinocularEditor)
};
