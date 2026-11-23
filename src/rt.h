// Binocular native port (from the roomservice port) - EEL2 runtime semantics in C++
// Mirrors the REAPER / ysfx x64 EEL2 JIT:
//  * all values double; memory is 8M doubles, index = trunc(x + 0.00001), out of range -> one shared dummy slot
//  * truth: |x| >= 0.00001;  == / != are fuzzy (0.00001), === / !== exact
//  * '%' = trunc|a| mod trunc|b| (0 when b truncates to 0); & | ~ on int64; << >> on int32
//  * plain '=' of a computed value flushes denormals, tiny values and inf/NaN to 0 (x64 nseel_asm_assign)
//  * min(a,b) = a > b ? b : a ; max(a,b) = a < b ? b : a ; sqrt(x) = sqrt(|x|)
#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include <initializer_list>
#include <limits>

namespace rs {

constexpr int64_t MEMSIZE = 8388608;
constexpr double RT_INF = std::numeric_limits<double>::infinity();
constexpr double RT_NAN = std::numeric_limits<double>::quiet_NaN();

// noinline libm wrappers (EEL folds constants with the runtime libm; keep the compiler from folding them differently)
double rt_sin(double); double rt_cos(double); double rt_tan(double); double rt_exp(double);
double rt_log(double); double rt_log10(double); double rt_pow(double, double); double rt_atan2(double, double);

inline bool B(double x) { return std::fabs(x) >= 0.00001; }
inline bool EQ(double a, double b) { return std::fabs(a - b) < 0.00001; }
inline bool NE(double a, double b) { return std::fabs(a - b) >= 0.00001; }
inline double dnf(double v) {
    uint64_t u; std::memcpy(&u, &v, 8);
    const uint32_t hw = (uint32_t)(u >> 32);
    return (((hw + 0x00100000u) & 0x7ff00000u) <= 0x00200000u) ? 0.0 : v;
}
inline double& ASG(double& d, double v) { d = dnf(v); return d; }
inline double& CASG(double& d, double v) { d = v; return d; }
inline int64_t rt_i64(double x) {   // cvttsd2si semantics
    if (!(x > -9223372036854775808.0 && x < 9223372036854775808.0)) return INT64_MIN;
    return (int64_t)x;
}
inline int32_t rt_i32(double x) {
    if (!(x > -2147483649.0 && x < 2147483648.0)) return INT32_MIN;
    return (int32_t)x;
}
inline double rt_mod(double a, double b) {
    const uint64_t ib = (uint64_t)rt_i64(std::fabs(b)), ia = (uint64_t)rt_i64(std::fabs(a));
    return ib ? (double)(ia % ib) : 0.0;
}
inline double rt_and(double a, double b) { return (double)(rt_i64(a) & rt_i64(b)); }
inline double rt_or(double a, double b) { return (double)(rt_i64(a) | rt_i64(b)); }
inline double rt_xor(double a, double b) { return (double)(rt_i64(a) ^ rt_i64(b)); }
inline double rt_or0(double a) { return (double)rt_i64(a); }
inline double rt_shl(double a, double b) {
    const uint64_t v = (uint64_t)(uint32_t)rt_i64(a); const int s = (int)(rt_i64(b) & 63);
    return (double)(int32_t)(uint32_t)(v << s);
}
inline double rt_shr(double a, double b) {
    const int64_t v = (int64_t)(uint32_t)rt_i64(a); const int s = (int)(rt_i64(b) & 63);
    return (double)(int32_t)(uint32_t)(v >> s);
}
inline double rt_sqr(double x) { return x * x; }
inline double rt_sqrt(double x) { return std::sqrt(std::fabs(x)); }
inline double rt_min(double a, double b) { return a > b ? b : a; }
inline double rt_max(double a, double b) { return a < b ? b : a; }
inline double rt_sign(double x) { return x < 0.0 ? -1.0 : (x > 0.0 ? 1.0 : x); }

// ---------- drawing backend (implemented by the editor) ----------
struct GfxBackend {
    virtual ~GfxBackend() = default;
    // all coordinates in script pixels; colors from the runtime's gfx_r/g/b/a; mode gfx_mode
    virtual void rect(double x, double y, double w, double h, bool filled) = 0;
    virtual void line(double x1, double y1, double x2, double y2, bool aa) = 0;
    virtual void circle(double x, double y, double r, bool fill, bool aa) = 0;
    virtual void arc(double x, double y, double r, double a1, double a2, bool aa) = 0;
    virtual void triangle(const double* xy, int npts) = 0;
    virtual void roundrect(double x, double y, double w, double h, double radius, bool aa) = 0;
    virtual void gradrect(const double* p, int n) = 0;
    virtual void drawstr(const std::string& s, int flags, double right, double bottom) = 0;
    virtual void measurestr(const std::string& s, double& w, double& h) = 0;
    virtual void setfont(int idx, const std::string* face, double sz, int flags) = 0;
    virtual void setimgdim(int img, int w, int h) = 0;
    virtual void blit(const double* p, int n) = 0;
    virtual void setcursor(int id) = 0;
    virtual double getchar(double c, bool hasArg) = 0;
};

// ---------- host callbacks ----------
struct HostCallbacks {
    virtual ~HostCallbacks() = default;
    virtual void sliderAutomate(int idx, int touchMode) = 0;   // touchMode: -1 one-shot, 0 begin/continue, 1 end
    virtual void sliderChanged(int idx) = 0;                    // idx < 0: all
};

struct Runtime {
    Runtime();
    virtual ~Runtime() = default;

    std::vector<double> mem;
    double memfail = 0.0;
    double sl[257] = {};
    double spl[64] = {};
    double v_srate = 48000, v_samplesblock = 0, v_play_state = 0, v_play_position = 0, v_beat_position = 0,
           v_tempo = 120, v_ts_num = 4, v_ts_denom = 4, v_num_ch = 2, v_ext_noinit = 0, v_ext_nodenorm = 0,
           v_pdc_delay = 0, v_pdc_bot_ch = 0, v_pdc_top_ch = 0, v_pdc_midi = 0,
           v_gfx_r = 1, v_gfx_g = 1, v_gfx_b = 1, v_gfx_a = 1, v_gfx_x = 0, v_gfx_y = 0, v_gfx_w = 0, v_gfx_h = 0,
           v_gfx_mode = 0, v_gfx_clear = 0, v_gfx_dest = -1, v_gfx_texth = 8, v_gfx_ext_retina = 0,
           v_mouse_x = 0, v_mouse_y = 0, v_mouse_cap = 0, v_mouse_wheel = 0, v_mouse_hwheel = 0;

    GfxBackend* gfx = nullptr;
    HostCallbacks* host = nullptr;

    inline double& M(double a) {
        const double x = a + 0.00001;
        if (!(x > -1.0 && x < (double)MEMSIZE)) return memfail;
        return mem[(size_t)(int64_t)x];
    }
    inline double& SL(double n) {
        const int i = rt_i32(n + 0.00001);
        if (i < 1 || i > 256) return memfail;
        return sl[i];
    }

    // sections (generated)
    virtual void S_init() = 0;
    virtual void S_slider() = 0;
    virtual void S_block() = 0;
    virtual void S_sample() = 0;
    virtual void S_serialize() = 0;
    virtual void S_gfx() = 0;
    virtual void initStrings() = 0;

    // memory
    double memset(double dest, double v, double len);
    double memcpy(double dest, double src, double len);
    double fft(double start, double len);
    double ifft(double start, double len);
    double fft_permute(double start, double len);
    double fft_ipermute(double start, double len);

    // strings
    struct Str { std::string s; bool ro = false; };
    std::unordered_map<int, Str> strs;
    void setLiteral(int id, const char* s) { strs[id] = Str{s, true}; }
    void setString(int id, const char* s) { strs[id] = Str{s, false}; }
    Str* str(double id, bool forWrite);
    const std::string& cstr(double id);
    double strcpy(double d, double s);
    double strcat(double d, double s);
    double strcmp(double a, double b);
    double strlen(double s);
    double str_setlen(double s, double len);
    double str_getchar(std::initializer_list<double> a);
    double sprintf(std::initializer_list<double> a);
    std::string format(const std::string& fmt, const double* args, int nargs);

    // serialization (@serialize): float32 values, like REAPER / ysfx
    int serMode = -1;                 // 1 write, 0 read, -1 idle
    std::vector<float> serBuf;
    size_t serPos = 0;
    double file_avail(double h);
    double file_var(double h, double& v);
    double file_mem(double h, double off, double len);

    // sliders / host
    double slider_automate_idx(std::initializer_list<double> a);
    double slider_automate_mask(std::initializer_list<double> a);
    double sliderchange_idx(double i);
    double sliderchange_mask(double m);
    double time_precise();

    // gfx
    double gfx_rect(std::initializer_list<double> a);
    double gfx_line(std::initializer_list<double> a);
    double gfx_lineto(std::initializer_list<double> a);
    double gfx_circle(std::initializer_list<double> a);
    double gfx_arc(std::initializer_list<double> a);
    double gfx_triangle(std::initializer_list<double> a);
    double gfx_roundrect(std::initializer_list<double> a);
    double gfx_gradrect(std::initializer_list<double> a);
    double gfx_drawstr(std::initializer_list<double> a);
    double gfx_measurestr(double s, double& w, double& h);
    double gfx_setfont(std::initializer_list<double> a);
    double gfx_setimgdim(double img, double w, double h);
    double gfx_blit(std::initializer_list<double> a);
    double gfx_setcursor(std::initializer_list<double> a);
    double gfx_getchar(std::initializer_list<double> a);
};

} // namespace rs
