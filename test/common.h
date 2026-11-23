// test signal: 4 channels (main L, R, sidechain L, R), interleaved. seed-based noise with an offset.
#include <vector>
#include <cmath>
#include <cstdint>
#include <cstdio>
struct Rng { uint64_t s; double n() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return (double)(s >> 11) / 9007199254740992.0 * 2 - 1; } };
static std::vector<double> makeSignal(int kind, double sr, int frames) {
    std::vector<double> v((size_t)frames * 4); Rng r{88172645463325252ull}; std::vector<double> a(frames), b(frames), c(frames);
    double lp = 0, lp2 = 0, lp3 = 0;
    for (int i = 0; i < frames; i++) { lp = 0.97 * lp + 0.03 * r.n(); lp2 = 0.97 * lp2 + 0.03 * r.n(); lp3 = 0.97 * lp3 + 0.03 * r.n(); a[i] = lp * 3; b[i] = lp2 * 3; c[i] = lp3 * 3; }
    for (int i = 0; i < frames; i++) {
        const int d = 12;
        double L = a[i] + 0.25 * b[i], R = (i >= d ? a[i - d] : 0) + 0.25 * c[i];
        if (kind == 1) R = -a[i] + 0.2 * c[i];                       // polarity flipped
        if (kind == 2) { L = a[i]; R = a[i]; }                        // mono
        double S1 = 0, S2 = 0;
        if (kind == 3) { S1 = S2 = (i >= 144 ? a[i - 144] : 0) * 0.6 + 0.4 * c[i]; }   // sidechain 3 ms late
        if (kind == 4) { L = 0.3 * std::sin(2 * M_PI * 440 * i / sr); R = L * 0.5; } // tone, left louder
        v[4 * (size_t)i] = L; v[4 * (size_t)i + 1] = R; v[4 * (size_t)i + 2] = S1; v[4 * (size_t)i + 3] = S2;
    }
    return v;
}
// sliders per case: {idx, value}
struct SV { int n; double v; };
static std::vector<SV> caseSliders(int c) {
    switch (c) {
        case 0: return {};                                         // defaults: 128 bands (FFT)
        case 1: return {{1, 5}};                                   // 32 bands (filter bank)
        case 2: return {{1, 9}, {2, 80}, {3, 1000}};               // 256 bands, sharp, slow
        case 3: return {{1, 7}, {4, 1}};                           // 64 bands, sidechain input
        case 4: return {{1, 0}, {3, 10}, {5, 2}, {8, 2}};          // 4 bands, fast, hold infinite, goniometer page
        default: return {};
    }
}
static const char* kVars[] = {"bcor","bxx","byy","bxy","bbal","bwid","ofs_lag","ofs_sgn","ofs_ok","ofs_conf","bhld","nb","ffm","fn","gwp", nullptr};
