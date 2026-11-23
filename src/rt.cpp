#include "rt.h"
#include <chrono>
#include <cstdio>
#include <algorithm>
#include <cctype>
extern "C" {
#include "fft.h"
}

#if defined(_MSC_VER)
#define RS_NOINLINE __declspec(noinline)
#else
#define RS_NOINLINE __attribute__((noinline))
#endif

namespace rs {

// ---- libm wrappers: separate TU, never inlined, never constant-folded by the compiler ----
RS_NOINLINE double rt_sin(double x) { return ::sin(x); }
RS_NOINLINE double rt_cos(double x) { return ::cos(x); }
RS_NOINLINE double rt_tan(double x) { return ::tan(x); }
RS_NOINLINE double rt_exp(double x) { return ::exp(x); }
RS_NOINLINE double rt_log(double x) { return ::log(x); }
RS_NOINLINE double rt_log10(double x) { return ::log10(x); }
RS_NOINLINE double rt_pow(double a, double b) { return ::pow(a, b); }
RS_NOINLINE double rt_atan2(double a, double b) { return ::atan2(a, b); }

static bool g_fftInit = false;

Runtime::Runtime() : mem((size_t)MEMSIZE, 0.0) {
    if (!g_fftInit) { WDL_fft_init(); g_fftInit = true; }
}

// ---------------- memory (nseel-ram.c semantics) ----------------
double Runtime::memset(double dest, double v, double lenv) {
    int offs = (int)(dest + 0.0001), len = (int)(lenv + 0.0001);
    if (offs < 0) { len += offs; offs = 0; }
    if (offs >= MEMSIZE) return dest;
    if (offs + (int64_t)len > MEMSIZE) len = (int)(MEMSIZE - offs);
    if (len < 1) return dest;
    std::fill(mem.begin() + offs, mem.begin() + offs + len, v);
    return dest;
}
double Runtime::memcpy(double dest, double src, double lenv) {
    int d = (int)(dest + 0.0001), s = (int)(src + 0.0001), len = (int)(lenv + 0.0001);
    if (s < 0) { len += s; d -= s; s = 0; }
    if (d < 0) { len += d; s -= d; d = 0; }
    if (s + (int64_t)len > MEMSIZE) len = (int)(MEMSIZE - s);
    if (d + (int64_t)len > MEMSIZE) len = (int)(MEMSIZE - d);
    if (s == d || len < 1) return dest;
    std::memmove(&mem[d], &mem[s], sizeof(double) * (size_t)len);
    return dest;
}
static int fftBits(double lenv, int minbits) {
    int l = (int)(lenv + 0.0001), bitl = 0;
    while (l > 1 && bitl < 15) { bitl++; l >>= 1; }
    return bitl < minbits ? -1 : bitl;
}
static double doFFT(Runtime& r, double start, double lenv, int dir) {
    const int offs = (int)(start + 0.0001);
    const int bitl = fftBits(lenv, (dir & 4) ? 3 : 4);
    if (bitl < 0) return start;
    const int ilen = 1 << bitl;
    const int shift = (dir & 2) ? 0 : 1;
    if (offs < 0 || offs / 65536 != (offs + (ilen << shift) - 1) / 65536) return start;
    if (offs + (ilen << shift) > MEMSIZE) return start;
    double* data = &r.mem[offs];
    if (dir == 4 || dir == 5) {
        std::vector<double> tmp(data, data + ilen * 2);
        for (int x = 0; x < ilen * 2; x += 2) {
            const int y = WDL_fft_permute(ilen, x / 2) * 2;
            if (dir == 4) { data[x] = tmp[y]; data[x + 1] = tmp[y + 1]; }
            else { data[y] = tmp[x]; data[y + 1] = tmp[x + 1]; }
        }
    } else if (dir == 0 || dir == 1) {
        WDL_fft((WDL_FFT_COMPLEX*)data, ilen, dir & 1);
    }
    return start;
}
double Runtime::fft(double s, double l) { return doFFT(*this, s, l, 0); }
double Runtime::ifft(double s, double l) { return doFFT(*this, s, l, 1); }
double Runtime::fft_permute(double s, double l) { return doFFT(*this, s, l, 4); }
double Runtime::fft_ipermute(double s, double l) { return doFFT(*this, s, l, 5); }

// ---------------- strings (eel_strings.h semantics, ids: 0..1023 user, 10000 literals, 90000 named, 190000 unnamed) ----------------
Runtime::Str* Runtime::str(double id, bool forWrite) {
    const int idx = (int)(id + 0.5);
    auto it = strs.find(idx);
    if (it == strs.end()) {
        if (idx >= 0 && idx < 1024) { auto& s = strs[idx]; return &s; }
        return nullptr;
    }
    if (forWrite && it->second.ro) return nullptr;
    return &it->second;
}
const std::string& Runtime::cstr(double id) {
    static const std::string empty;
    Str* s = str(id, false);
    return s ? s->s : empty;
}
double Runtime::strcpy(double d, double s) {
    Str* w = str(d, true); if (!w) return d;
    std::string v = cstr(s); w->s = v; return d;
}
double Runtime::strcat(double d, double s) {
    Str* w = str(d, true); if (!w) return d;
    std::string v = cstr(s); w->s += v; return d;
}
double Runtime::strcmp(double a, double b) {
    const int c = cstr(a).compare(cstr(b));
    return c < 0 ? -1.0 : (c > 0 ? 1.0 : 0.0);
}
double Runtime::strlen(double s) { return (double)cstr(s).size(); }
double Runtime::str_setlen(double s, double lenv) {
    Str* w = str(s, true); if (!w) return s;
    int l = (int)lenv; if (l < 0) l = 0;
    w->s.resize((size_t)l, ' ');
    return s;
}
double Runtime::str_getchar(std::initializer_list<double> a) {
    const double* p = a.begin();
    const std::string& s = cstr(p[0]);
    const double idx = p[1];
    int l = (int)idx; const int wl = (int)s.size();
    if (idx < 0.0) l += wl;
    if (l >= 0 && l < wl) return (double)(unsigned char)s[(size_t)l];
    return 0.0;
}
std::string Runtime::format(const std::string& fmt, const double* args, int nargs) {
    std::string out; int ap = 0; size_t i = 0;
    while (i < fmt.size()) {
        if (fmt[i] == '%' && i + 1 < fmt.size() && fmt[i + 1] == '%') { out += '%'; i += 2; continue; }
        if (fmt[i] != '%') { out += fmt[i++]; continue; }
        size_t j = i + 1;
        while (j < fmt.size() && std::strchr("-+ #0", fmt[j])) j++;
        while (j < fmt.size() && (std::isdigit((unsigned char)fmt[j]) || fmt[j] == '.')) j++;
        while (j < fmt.size() && std::strchr("lhLqjzt", fmt[j])) j++;
        if (j >= fmt.size()) break;
        const char ct = fmt[j];
        std::string fs = fmt.substr(i, j - i);
        fs.erase(std::remove_if(fs.begin() + 1, fs.end(), [](char c) { return std::strchr("lhLqjzt", c) != nullptr; }), fs.end());
        const double v = ap < nargs ? args[ap] : 0.0; ap++;
        char buf[512];
        if (ct == 's' || ct == 'S') {
            if (fs == "%") out += cstr(v);
            else { std::snprintf(buf, sizeof buf, (fs + "s").c_str(), cstr(v).c_str()); out += buf; }
        } else if (std::strchr("xXdui", ct)) {
            std::snprintf(buf, sizeof buf, (fs + ct).c_str(), (int)v); out += buf;
        } else if (ct == 'c') {
            out += (char)(int)v;
        } else if (std::strchr("eEfFgG", ct)) {
            std::snprintf(buf, sizeof buf, (fs + ct).c_str(), v); out += buf;
        } else break;
        i = j + 1;
    }
    return out;
}
double Runtime::sprintf(std::initializer_list<double> a) {
    const double* p = a.begin();
    const double d = p[0];
    std::string res = format(cstr(p[1]), p + 2, (int)a.size() - 2);
    Str* w = str(d, true); if (w) w->s = res;
    return d;
}

// ---------------- @serialize ----------------
double Runtime::file_avail(double h) {
    if ((int)std::lround(h) != 0) return 0.0;
    return serMode == 1 ? -1.0 : 0.0;
}
double Runtime::file_var(double h, double& v) {
    if ((int)std::lround(h) != 0) return 0.0;
    if (serMode == 1) { serBuf.push_back((float)v); return 1.0; }
    if (serMode == 0) {
        if (serPos >= serBuf.size()) { v = 0; return 0.0; }
        v = (double)serBuf[serPos++]; return 1.0;
    }
    return 0.0;
}
double Runtime::file_mem(double h, double offv, double lenv) {
    const int off = (int)std::lround(offv), len = (int)std::lround(lenv);
    if ((int)std::lround(h) != 0 || off < 0 || len <= 0) return 0.0;
    for (int i = 0; i < len; i++) {
        double& slot = M(off + i);
        double v = slot;
        if (!file_var(h, v)) return (double)i;
        if (serMode == 0) slot = v;
    }
    return (double)len;
}

// ---------------- sliders ----------------
double Runtime::slider_automate_idx(std::initializer_list<double> a) {
    const double* p = a.begin();
    const int idx = (int)p[0];
    const int mode = a.size() > 1 ? (B(p[1]) ? 1 : 0) : -1;
    if (host) host->sliderAutomate(idx, mode);
    return 0.0;
}
double Runtime::slider_automate_mask(std::initializer_list<double> a) {
    const double* p = a.begin();
    const uint64_t m = (uint64_t)rt_i64(p[0]);
    const int mode = a.size() > 1 ? (B(p[1]) ? 1 : 0) : -1;
    for (int i = 0; i < 64; i++) if (m & (1ull << i)) if (host) host->sliderAutomate(i + 1, mode);
    return 0.0;
}
double Runtime::sliderchange_idx(double i) { if (host) host->sliderChanged((int)i); return 0.0; }
double Runtime::sliderchange_mask(double m) {
    if (!host) return 0.0;
    if (m < 0) { host->sliderChanged(-1); return 0.0; }
    const uint64_t b = (uint64_t)rt_i64(m);
    for (int i = 0; i < 64; i++) if (b & (1ull << i)) host->sliderChanged(i + 1);
    return 0.0;
}
double Runtime::time_precise() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

// ---------------- gfx (forward to the editor's backend) ----------------
#define ARGS const double* p = a.begin(); const int n = (int)a.size(); (void)n
double Runtime::gfx_rect(std::initializer_list<double> a) { ARGS; if (gfx) gfx->rect(p[0], p[1], p[2], p[3], n > 4 ? B(p[4]) : true); return 0; }
double Runtime::gfx_line(std::initializer_list<double> a) { ARGS; if (gfx) gfx->line(p[0], p[1], p[2], p[3], n > 4 ? B(p[4]) : true); return 0; }
// gfx_lineto(x, y[, aa]): line from gfx_x/gfx_y to x/y, then gfx_x/gfx_y = x/y
double Runtime::gfx_lineto(std::initializer_list<double> a) { ARGS; if (gfx) gfx->line(v_gfx_x, v_gfx_y, p[0], p[1], n > 2 ? B(p[2]) : true); v_gfx_x = p[0]; v_gfx_y = p[1]; return 0; }
double Runtime::gfx_circle(std::initializer_list<double> a) { ARGS; if (gfx) gfx->circle(p[0], p[1], p[2], n > 3 ? B(p[3]) : false, n > 4 ? B(p[4]) : true); return 0; }
double Runtime::gfx_arc(std::initializer_list<double> a) { ARGS; if (gfx) gfx->arc(p[0], p[1], p[2], p[3], p[4], n > 5 ? B(p[5]) : true); return 0; }
double Runtime::gfx_triangle(std::initializer_list<double> a) { ARGS; if (gfx) gfx->triangle(p, n / 2); return 0; }
double Runtime::gfx_roundrect(std::initializer_list<double> a) { ARGS; if (gfx) gfx->roundrect(p[0], p[1], p[2], p[3], p[4], n > 5 ? B(p[5]) : true); return 0; }
double Runtime::gfx_gradrect(std::initializer_list<double> a) { ARGS; if (gfx) gfx->gradrect(p, n); return 0; }
double Runtime::gfx_drawstr(std::initializer_list<double> a) {
    ARGS; if (gfx) gfx->drawstr(cstr(p[0]), n > 1 ? (int)p[1] : 0, n > 2 ? p[2] : 0, n > 3 ? p[3] : 0); return 0;
}
double Runtime::gfx_measurestr(double s, double& w, double& h) { if (gfx) gfx->measurestr(cstr(s), w, h); else { w = 0; h = 0; } return 0; }
double Runtime::gfx_setfont(std::initializer_list<double> a) {
    ARGS;
    if (gfx) {
        if (n >= 3) { std::string face = cstr(p[1]); gfx->setfont((int)p[0], &face, p[2], n > 3 ? (int)p[3] : 0); }
        else gfx->setfont((int)p[0], nullptr, 0, 0);
    }
    return 1;
}
double Runtime::gfx_setimgdim(double img, double w, double h) { if (gfx) gfx->setimgdim((int)img, (int)w, (int)h); return 0; }
double Runtime::gfx_blit(std::initializer_list<double> a) { ARGS; if (gfx) gfx->blit(p, n); return 0; }
double Runtime::gfx_setcursor(std::initializer_list<double> a) { ARGS; if (gfx) gfx->setcursor((int)p[0]); return 0; }
double Runtime::gfx_getchar(std::initializer_list<double> a) { ARGS; return gfx ? gfx->getchar(n > 0 ? p[0] : 0, n > 0) : 0; }

} // namespace rs
