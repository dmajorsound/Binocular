// JSFX gfx_* (LICE) on top of JUCE Graphics: framebuffer + offscreen images 0..1023, fonts 0..16
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "rt.h"
#include <map>
#include <deque>
#include <unordered_map>

namespace rs {

class JuceGfx : public GfxBackend {
public:
    explicit JuceGfx(Runtime& r) : rt(r) {}

    // called by the editor around each @gfx run
    void beginFrame(int w, int h);
    void endFrame();
    const juce::Image& screen() const { return screenImg; }

    // keyboard / window state (filled by the editor)
    std::deque<int> keyQueue;
    bool focused = false, mouseInside = false;
    std::function<bool(int)> isKeyDown;
    std::function<void(int)> onCursor;

    // GfxBackend
    void rect(double x, double y, double w, double h, bool filled) override;
    void line(double x1, double y1, double x2, double y2, bool aa) override;
    void circle(double x, double y, double r, bool fill, bool aa) override;
    void arc(double x, double y, double r, double a1, double a2, bool aa) override;
    void triangle(const double* xy, int npts) override;
    void roundrect(double x, double y, double w, double h, double radius, bool aa) override;
    void gradrect(const double* p, int n) override;
    void drawstr(const std::string& s, int flags, double right, double bottom) override;
    void measurestr(const std::string& s, double& w, double& h) override;
    void setfont(int idx, const std::string* face, double sz, int flags) override;
    void setimgdim(int img, int w, int h) override;
    void blit(const double* p, int n) override;
    void setcursor(int id) override { if (onCursor) onCursor(id); }
    double getchar(double c, bool hasArg) override;

private:
    juce::Graphics& G();          // graphics for the current gfx_dest
    juce::Image* destImage();
    juce::Colour colour() const;
    struct FontSlot { juce::Font font { juce::FontOptions() }; std::string key; bool set = false; };
    // text is the expensive part in JUCE: fonts, string widths and laid-out glyphs are cached
    std::unordered_map<std::string, juce::Font> fontCache;
    std::unordered_map<std::string, float> widthCache;
    std::unordered_map<std::string, juce::Image> textCache;   // rendered strings (font + colour + text): a blit per frame
                                                              // instead of rasterizing glyphs (CoreText on the Mac) every time
    float textWidth(const std::string& s);
    FontSlot fonts[17];
    int curFont = 0;
    bool inClip = false;   // a clip region is active (direct pixel writes would ignore it)
    const juce::Font& font();

    Runtime& rt;
    juce::Image screenImg;
    std::map<int, juce::Image> images;
    int gDest = -2;               // which image the cached Graphics draws into
    std::unique_ptr<juce::Graphics> g;
};

} // namespace rs
