#pragma once
#include "PluginProcessor.h"
#include "JuceGfx.h"

// The original Binocular @gfx, run natively at ~30 fps and drawn through JuceGfx
class BinocularEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit BinocularEditor(BinocularProcessor&);
    ~BinocularEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override { if (sizeReady) { proc.editorW = proc.sessionW = getWidth(); proc.editorH = proc.sessionH = getHeight(); } }   // remembered for next time

    void mouseMove(const juce::MouseEvent& e) override { mouse(e); }
    void mouseDrag(const juce::MouseEvent& e) override { mouse(e); }
    void mouseDown(const juce::MouseEvent& e) override { grabKeyboardFocus(); mouse(e); }
    void mouseUp(const juce::MouseEvent& e) override { mouse(e, true); }
    void mouseEnter(const juce::MouseEvent& e) override { gfx.mouseInside = true; mouse(e); }
    void mouseExit(const juce::MouseEvent& e) override { gfx.mouseInside = false; mouse(e); }
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& w) override;
    bool keyPressed(const juce::KeyPress& k) override;
    void focusGained(FocusChangeType) override { gfx.focused = true; }
    void focusLost(FocusChangeType) override { gfx.focused = false; }

private:
    void timerCallback() override;
    void mouse(const juce::MouseEvent& e, bool up = false);
    double scale() const;

    BinocularProcessor& proc;
    rs::JuceGfx gfx;
    float lastPaintScale = 1.0f;
    int liveCap = 0;          // modifier + button state right now
    int latchedButtons = 0;
    int lastCursor = -1;
    bool sizeReady = false;
    // diagnostics: per-second timing lines appended to ~/Desktop/binocular-debug.txt if that file exists
    juce::File diagFile;
    double diagT0 = 0, lastTick = 0;
    int dFrames = 0, dPaints = 0, dEvents = 0;
    double dGfx = 0, dGfxMax = 0, dGap = 0, dGapMax = 0, dPaint = 0, dPaintMax = 0;
    void diagTick();   // presses since the last @gfx frame (a quick click between two frames is not lost)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BinocularEditor)
};
