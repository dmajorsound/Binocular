#include "Analyzer.h"
#include "fft.h"
#include <algorithm>
#include <cstring>
#include <limits>

namespace bn {

static constexpr double kTiny = 1e-24, kTiny12 = 1e-12;
static constexpr double kPi = 3.141592653589793;

static double energyToLufs(double e) { return -0.691 + 10.0 * std::log10(e); }

// what the correlation pages compare: stereo, or in 5.1 front, surround or downmix
static int inputKey(const Settings& s) { return s.surround ? 10 + s.pair : 0; }

constexpr int Analyzer::kBandCounts[10];

Analyzer::Analyzer() {
    static bool fftReady = false;
    if (!fftReady) { WDL_fft_init(); fftReady = true; }
    histL.assign(65536, 0.0); histR.assign(65536, 0.0);
    fftBuf.assign(65536, 0.0);
    for (auto* v : {&sxx, &syy, &sxy, &sxi, &cxx, &cyy, &cxy}) v->assign(16400, 0.0);
    for (auto* v : {&s2x, &s2y, &s2c, &c2x, &c2y, &c2c}) v->assign(8200, 0.0);
    slotMom.assign(kSlots, 0.0); slotShort.assign(kSlots, 0.0); slotUsed.assign(kSlots, 0); slotDialog.assign(kSlots, 0);
    spBuf.reserve(4096);
    historyRing.assign(kSlots, 0.0);
}

int Analyzer::spanSlots(int span) {
    static const int slots[7] = {600, 3000, 9000, 18000, 36000, 72000, 180000};
    return slots[std::clamp(span, 0, 6)];
}

void Analyzer::prepare(double sr, const Settings& s) {
    sampleRate = sr;
    std::fill(gonioX.begin(), gonioX.end(), 0.0f); std::fill(gonioY.begin(), gonioY.end(), 0.0f);
    gonioWrite = 0;
    offsetTurn = 0;
    clearLows = false;
    setup(s);
    loudnessFilters();
    loudnessReset();
    measureMode = s.measure;
    builtSpan = s.span;
}

// ---------------------------------------------------------------- band layout
// Log-spaced bands from 20 Hz to 20 kHz, read from an FFT cross-spectrum. A band takes the short FFT (fast) when it is
// at least 3 of its bins wide, otherwise the long FFT (fine enough for the bass). The long FFT always runs: it also
// finds the offset and the polarity.
void Analyzer::setup(const Settings& s) {
    bands = kBandCounts[std::clamp(s.bandsIndex, 0, 9)];
    fftLong = sampleRate <= 50000 ? 16384 : 32768;   // 2.9 Hz per bin at 48 and 96 kHz
    hopLong = fftLong / 16;                          // long spectrum ~47 times per second
    fftShort = fftLong / 4;                          // 4096 at 48 kHz, about 85 ms
    hopShort = fftShort / 8;                         // short spectrum ~94 times per second
    lowHz = 20;
    highHz = std::min(20000.0, 0.45 * sampleRate);
    const double spacing = std::log(highHz / lowHz) / std::log(2.0) / bands;   // octaves per band
    const double width = spacing * std::pow(2.0, 1 - 2 * s.selectivity / 100);  // 0 %: twice the spacing, 50 %: the spacing, 100 %: half
    std::fill(histL.begin(), histL.end(), 0.0); std::fill(histR.begin(), histR.end(), 0.0);
    for (auto* v : {&sxx, &syy, &sxy, &sxi, &cxx, &cyy, &cxy}) std::fill(v->begin(), v->end(), 0.0);
    for (auto* v : {&s2x, &s2y, &s2c, &c2x, &c2y, &c2c}) std::fill(v->begin(), v->end(), 0.0);
    winLong.resize((size_t)fftLong); winShort.resize((size_t)fftShort);
    for (int i = 0; i < fftLong; i++) winLong[(size_t)i] = 0.5 - 0.5 * std::cos(2 * kPi * (i + 0.5) / fftLong);
    for (int i = 0; i < fftShort; i++) winShort[(size_t)i] = 0.5 - 0.5 * std::cos(2 * kPi * (i + 0.5) / fftShort);
    histWrite = hopCount = hopCount2 = 0;
    longDue = shortDue = offsetDue = false;
    const int binsLong = fftLong / 2, binsShort = fftShort / 2;
    for (int i = 0; i < bands; i++) {
        const double fc = lowHz * std::pow(highHz / lowHz, (i + 0.5) / bands);
        const double e0 = fc * std::pow(2.0, -width / 2), e1 = fc * std::pow(2.0, width / 2);
        shortEngine[(size_t)i] = (e1 - e0) * fftShort / sampleRate >= 3;
        if (shortEngine[(size_t)i]) {
            edgeLo[(size_t)i] = std::max(0.5, std::min((double)binsShort - 1, e0 * fftShort / sampleRate + 0.5));
            edgeHi[(size_t)i] = std::max(0.5, std::min((double)binsShort - 1, e1 * fftShort / sampleRate + 0.5));
        } else {
            edgeLo[(size_t)i] = std::max(0.5, std::min((double)binsLong - 1, e0 * fftLong / sampleRate + 0.5));
            edgeHi[(size_t)i] = std::max(0.5, std::min((double)binsLong - 1, e1 * fftLong / sampleRate + 0.5));
        }
        centerHz[(size_t)i] = fc;
        corr[(size_t)i] = 0; level[(size_t)i] = -200; hasSignal[(size_t)i] = false; monoLoss[(size_t)i] = 0;
        corrHeld[(size_t)i] = 1; held_t[(size_t)i] = 0; monoHeld[(size_t)i] = 0; monoHeld_t[(size_t)i] = 0;
    }
    sxyAvg = sxxAvg = syyAvg = 0;
    total = 0; totalValid = false; totalHeld = 1; totalHeldTime = 0;
    offsetFound = false; offsetSamples = 0; polarity = 1;
    setupCount++;
    lastBands = s.bandsIndex; lastSelectivity = s.selectivity; lastRate = sampleRate; lastInput = inputKey(s);
}

// ---------------------------------------------------------------- FFT engine
// a running sum over the bins, read at a fractional bin position
static double runningSum(const double* c, double u, int bins) {
    const int k = std::min((int)std::floor(u), bins - 1);
    return c[k] + (u - k) * (c[k + 1] - c[k]);
}

// one band from band powers: correlation, level, mono loss
void Analyzer::band(int i, double xx, double yy, double xy) {
    const double dd = std::sqrt(std::fabs(xx * yy));
    corr[(size_t)i] = dd > kTiny ? std::max(-1.0, std::min(1.0, xy / dd)) : 0;
    level[(size_t)i] = 10 * std::log10(0.5 * (xx + yy) + kTiny);
    hasSignal[(size_t)i] = (10 * std::log10(xx + kTiny) > -100) || (10 * std::log10(yy + kTiny) > -100);   // one side silent: correlation 0, still shown
    monoLoss[(size_t)i] = 10 * std::log10(std::max(kTiny12, (xx + yy + 2 * xy) / (2 * (xx + yy) + kTiny)));
}

// left in the real part, right in the imaginary part of one FFT; split, then smooth per bin.
// sx, sy, sc: smoothed powers and real cross part. si: imaginary cross part (nullptr = not needed).
void Analyzer::spectrum(int n, int start, const std::vector<double>& win, double* sx, double* sy, double* sc, double* si, double kf) {
    double* fb = fftBuf.data();
    for (int i = 0, j = start; i < n; i++, j = (j + 1) & 65535) {
        fb[2 * i] = histL[(size_t)j] * win[(size_t)i];
        fb[2 * i + 1] = histR[(size_t)j] * win[(size_t)i];
    }
    WDL_fft((WDL_FFT_COMPLEX*)fb, n, 0);
    // into natural order
    std::vector<double> tmp(fb, fb + 2 * n);
    for (int x = 0; x < n; x++) {
        const int y = WDL_fft_permute(n, x);
        fb[2 * x] = tmp[(size_t)(2 * y)]; fb[2 * x + 1] = tmp[(size_t)(2 * y + 1)];
    }
    for (int k = 1; k < n / 2; k++) {
        const double a = fb[2 * k], b = fb[2 * k + 1], c = fb[2 * (n - k)], d = fb[2 * (n - k) + 1];
        const double xr = (a + c) * 0.5, xi = (b - d) * 0.5, yr = (b + d) * 0.5, yi = (c - a) * 0.5;
        sx[k] += (xr * xr + xi * xi - sx[k]) * kf;
        sy[k] += (yr * yr + yi * yi - sy[k]) * kf;
        sc[k] += (xr * yr + xi * yi - sc[k]) * kf;
        if (si) si[k] += (xi * yr - xr * yi - si[k]) * kf;
    }
}

// running sums, then every band of the given engine (0 long, 1 short)
void Analyzer::bandsFromSpectrum(int engine, int n, const double* sx, const double* sy, const double* sc, double* cx, double* cy, double* cc) {
    cx[0] = cy[0] = cc[0] = 0;
    for (int k = 0; k < n / 2; k++) {
        cx[k + 1] = cx[k] + sx[k];
        cy[k + 1] = cy[k] + sy[k];
        cc[k + 1] = cc[k] + sc[k];
    }
    const double norm = 2.0 / (n * (double)n * 0.375);   // bin power to signal power (Hann, one-sided)
    for (int i = 0; i < bands; i++) {
        if ((int)shortEngine[(size_t)i] != engine) continue;
        const double u0 = edgeLo[(size_t)i], u1 = edgeHi[(size_t)i];
        band(i, (runningSum(cx, u1, n / 2) - runningSum(cx, u0, n / 2)) * norm,
                (runningSum(cy, u1, n / 2) - runningSum(cy, u0, n / 2)) * norm,
                (runningSum(cc, u1, n / 2) - runningSum(cc, u0, n / 2)) * norm);
    }
}

void Analyzer::longFrame(double averagingMs) {
    spectrum(fftLong, frameStart, winLong, sxx.data(), syy.data(), sxy.data(), sxi.data(),
             1 - std::exp(-hopLong / (std::max(1.0, averagingMs) * 0.001 * sampleRate)));
    bandsFromSpectrum(0, fftLong, sxx.data(), syy.data(), sxy.data(), cxx.data(), cyy.data(), cxy.data());
}

void Analyzer::shortFrame(double averagingMs) {
    spectrum(fftShort, frameStart2, winShort, s2x.data(), s2y.data(), s2c.data(), nullptr,
             1 - std::exp(-hopShort / (std::max(1.0, averagingMs) * 0.001 * sampleRate)));
    bandsFromSpectrum(1, fftShort, s2x.data(), s2y.data(), s2c.data(), c2x.data(), c2y.data(), c2c.data());
}

// offset and polarity: the phase of the cross-spectrum alone (GCC-PHAT), back to time, strongest peak within 10 ms
void Analyzer::findOffset() {
    const int n = fftLong, bins = n / 2;
    double* fb = fftBuf.data();
    std::fill(fb, fb + 2 * n, 0.0);
    for (int k = 1; k < bins; k++) {
        const double re = sxy[(size_t)k], im = sxi[(size_t)k];
        const double mg = std::sqrt(re * re + im * im);
        if (mg > kTiny) {
            fb[2 * k] = re / mg; fb[2 * k + 1] = im / mg;
            fb[2 * (n - k)] = re / mg; fb[2 * (n - k) + 1] = -im / mg;
        }
    }
    // from natural order into the FFT's order, then back to time
    std::vector<double> tmp(fb, fb + 2 * n);
    for (int x = 0; x < n; x++) {
        const int y = WDL_fft_permute(n, x);
        fb[2 * y] = tmp[(size_t)(2 * x)]; fb[2 * y + 1] = tmp[(size_t)(2 * x + 1)];
    }
    WDL_fft((WDL_FFT_COMPLEX*)fb, n, 1);
    const int lm = (int)std::floor(0.01 * sampleRate);
    double best = 0, s2 = 0; int bi = 0;
    for (int i = -lm; i <= lm; i++) {
        const double v = fb[2 * ((i + n) % n)];
        s2 += v * v;
        if (std::fabs(v) > std::fabs(best)) { best = v; bi = i; }
    }
    const double y0 = std::fabs(fb[2 * ((bi - 1 + n) % n)]), y1 = std::fabs(best), y2 = std::fabs(fb[2 * ((bi + 1 + n) % n)]);
    const double den = y0 - 2 * y1 + y2;
    const double frac = std::fabs(den) >= 0.00001 ? std::max(-0.5, std::min(0.5, 0.5 * (y0 - y2) / den)) : 0;
    const double conf = s2 > 0 ? std::fabs(best) / std::sqrt(s2 / (2 * lm + 1)) : 0;
    offsetFound = conf > 8 && totalValid;
    offsetSamples = bi + frac;
    polarity = best < 0 ? -1 : 1;
}

// ---------------------------------------------------------------- loudness (ITU-R BS.1770-4 / EBU R128 / EBU Tech 3342)
// Measured on the main input. The timeline is cut into 100 ms slots; every slot keeps its momentary block (400 ms)
// and its short-term value (3 s), so a slot measured again (timecode: the same part played again) replaces the old one.
// Integrated and loudness range come from two histograms with the standard gates.

// K-weighting (shelf + high-pass): the BS.1770 filters re-derived for any sample rate (as libebur128 does; at 48 kHz
// they are the coefficients of the standard), and the true peak interpolation filter
void Analyzer::loudnessFilters() {
    double K = std::tan(kPi * 1681.974450955533 / sampleRate), Q = 0.7071752369554196;
    const double Vh = std::pow(10.0, 3.999843853973347 / 20), Vb = std::pow(Vh, 0.4996667741545416);
    double a0 = 1 + K / Q + K * K;
    kb0 = (Vh + Vb * K / Q + K * K) / a0; kb1 = 2 * (K * K - Vh) / a0; kb2 = (Vh - Vb * K / Q + K * K) / a0;
    ka1 = 2 * (K * K - 1) / a0; ka2 = (1 - K / Q + K * K) / a0;
    K = std::tan(kPi * 38.13547087613982 / sampleRate); Q = 0.5003270373253953;
    a0 = 1 + K / Q + K * K;
    hb0 = 1; hb1 = -2; hb2 = 1; ha1 = 2 * (K * K - 1) / a0; ha2 = (1 - K / Q + K * K) / a0;
    // true peak: 4x oversampling up to 96 kHz, 2x above (12 taps per phase, Hann-windowed sinc)
    oversample = sampleRate < 100000 ? 4 : 2;
    const int L = 12 * oversample;
    const double c = (L - 1) / 2.0;
    for (int t = 0; t < L; t++) {
        const double x = (t - c) / oversample;
        const double w = 0.5 - 0.5 * std::cos(2 * kPi * (t + 1) / (L + 1));
        tpCoef[(size_t)t] = (std::fabs(x) < 0.000001 ? 1 : std::sin(kPi * x) / (kPi * x)) * w;
    }
    for (int p = 0; p < oversample; p++) {   // each phase sums to 1
        double s = 0;
        for (int k = 0; k < 12; k++) s += tpCoef[(size_t)(p + oversample * k)];
        for (int k = 0; k < 12; k++) tpCoef[(size_t)(p + oversample * k)] /= s;
    }
    // dialog detection: band-pass 250 Hz .. 3.5 kHz (RBJ, Q 0.707), the band at ~8 kHz, sub-frames of 25 ms
    auto biquad = [&](double f, bool high, double* b, double* a) {
        const double w0 = 2 * kPi * f / sampleRate, al = std::sin(w0) / (2 * 0.7071), cs = std::cos(w0), a0 = 1 + al;
        if (high) { b[0] = (1 + cs) / 2 / a0; b[1] = -(1 + cs) / a0; b[2] = b[0]; }
        else { b[0] = (1 - cs) / 2 / a0; b[1] = (1 - cs) / a0; b[2] = b[0]; }
        a[0] = -2 * cs / a0; a[1] = (1 - al) / a0;
    };
    biquad(250, true, spB, spA);
    biquad(3500, false, spB + 3, spA + 2);
    spDecim = std::max(1, (int)std::lround(sampleRate / 8000));
    spSubLen = std::max(1, (int)std::lround(0.025 * sampleRate));
    loudnessRate = sampleRate;
}

void Analyzer::loudnessReset() {
    std::fill(slotMom.begin(), slotMom.end(), 0.0); std::fill(slotShort.begin(), slotShort.end(), 0.0);
    std::fill(slotUsed.begin(), slotUsed.end(), 0);
    momCount.fill(0); momEnergy.fill(0); stCount.fill(0); stEnergy.fill(0);
    last30.fill(0); stepEnergy.fill(0); stepCount.fill(0);
    for (auto& h : tpHist) h.fill(0);
    for (auto& k : kState) k.fill(0);
    std::fill(slotDialog.begin(), slotDialog.end(), 0);
    dlgCount.fill(0); dlgEnergy.fill(0); dialogSlots = 0; dialog = -200; dialogShare = 0; speechNow = false;
    spHx1 = spHx2 = spHy1 = spHy2 = spLx1 = spLx2 = spLy1 = spLy2 = 0;
    spBandSum = spFullSum = spDecAcc = spSubSum = 0; spDecCount = spSubCount = 0; spBuf.clear();
    spSubDb.fill(-120); spSubPos = spSubFilled = 0; spVoiced.fill(0); spVoicedPos = 0; spRatio.fill(0); spPitch.fill(0); spHold = 0;
    currentSlot = -1000000000; previousSlot = currentSlot; contiguous = 0; slotSum = 0; slotCount = 0; measuredSamples = 0;
    last30Pos = 0; momentary = shortTerm = -200; measuredSlots = 0; historyCount = 0; newestStep = -1;
    integrated = -200; range = -1; truePeakMax = 0; tpWrite = 0; momentaryMax = shortTermMax = -200;
    loudnessDirty = false;
}

// one value into or out of a histogram (absolute gate: below -70 LUFS it is not counted)
void Analyzer::histogram(double e, double* counts, double* energies, double sign) {
    if (e <= 0) return;
    const double L = energyToLufs(e);
    if (L < -70) return;
    const int b = std::min(kHistBins - 1, (int)std::floor((L + 70) * 10));
    counts[b] += sign; energies[b] += sign * e;
}

// a finished slot into the timeline, replacing whatever was measured there before
void Analyzer::store(long long slot, double em, double es, bool isDialog) {
    const size_t i = (size_t)(((slot % kSlots) + kSlots) % kSlots);
    if (slotUsed[i]) {
        histogram(slotMom[i], momCount.data(), momEnergy.data(), -1);
        histogram(slotShort[i], stCount.data(), stEnergy.data(), -1);
        if (slotDialog[i]) { histogram(slotMom[i], dlgCount.data(), dlgEnergy.data(), -1); dialogSlots--; }
        measuredSlots--;
    }
    slotMom[i] = em; slotShort[i] = es; slotUsed[i] = 1; slotDialog[i] = isDialog ? 1 : 0;
    histogram(em, momCount.data(), momEnergy.data(), 1);
    histogram(es, stCount.data(), stEnergy.data(), 1);
    if (isDialog) { histogram(em, dlgCount.data(), dlgEnergy.data(), 1); dialogSlots++; }
    measuredSlots++;
}

// dialog in the slot that just ended? Voiced (a pitch between 70 and 400 Hz) in some but not all of the last second,
// most of the energy in the speech band, and the level moving with syllables. Held for half a second.
// This is Binocular's own detector: close to, but not the same as Dolby Dialogue Intelligence.
bool Analyzer::detectSpeech() {
    const double ratio = spBandSum / (spFullSum + kTiny);
    const double bandDb = 10 * std::log10(spBandSum / std::max<long long>(1, slotCount) + kTiny);
    // voicing: the strongest normalized autocorrelation at a pitch lag
    double periodicity = 0, bestLag = 0;
    const int N = (int)spBuf.size();
    const double fs8 = sampleRate / spDecim;
    const int lagMin = std::max(2, (int)(fs8 / 400)), lagMax = std::min(N / 2, (int)(fs8 / 70));
    for (int lag = lagMin; lag <= lagMax; lag++) {
        double xy = 0, xx = 0, yy = 0;
        for (int i = 0; i + lag < N; i++) { const double a = spBuf[(size_t)i], b = spBuf[(size_t)(i + lag)]; xy += a * b; xx += a * a; yy += b * b; }
        if (xx > 0 && yy > 0 && xy / std::sqrt(xx * yy) > periodicity) { periodicity = xy / std::sqrt(xx * yy); bestLag = lag; }
    }
    const bool voiced = periodicity > 0.5 && bandDb > -60;
    spVoiced[(size_t)spVoicedPos] = voiced ? 1 : 0;
    spRatio[(size_t)spVoicedPos] = ratio;
    spPitch[(size_t)spVoicedPos] = voiced && bestLag > 0 ? fs8 / bestLag : 0;
    spVoicedPos = (spVoicedPos + 1) % (int)spVoiced.size();
    int nv = 0; double ratioSum = 0;
    for (size_t k = 0; k < spVoiced.size(); k++) { nv += spVoiced[k]; ratioSum += spRatio[k]; }
    // syllable rhythm: how much the 25 ms levels of the last second move (speech rises and falls, sustained music less)
    // (levels more than 40 dB under the loudest count as 40 dB under: digital silence would rule otherwise)
    const int n = std::min(spSubFilled, (int)spSubDb.size());
    double top = -200;
    for (int k = 0; k < n; k++) top = std::max(top, spSubDb[(size_t)k]);
    auto lv = [&](int k) { return std::max(spSubDb[(size_t)k], top - 40); };
    double mean = 0, var = 0;
    for (int k = 0; k < n; k++) mean += lv(k);
    mean /= std::max(1, n);
    for (int k = 0; k < n; k++) var += (lv(k) - mean) * (lv(k) - mean);
    const double movement = n > 1 ? std::sqrt(var / n) : 0;
    // intonation: speech glides in pitch from slot to slot, held notes and chords stay put
    double glide = 0; int pairs = 0;
    const size_t L = spPitch.size();
    for (size_t k = 1; k < L; k++) {
        const double p0 = spPitch[(spVoicedPos + k - 1) % L], p1 = spPitch[(spVoicedPos + k) % L];
        if (p0 > 0 && p1 > 0) { glide += std::fabs(std::log(p1 / p0)); pairs++; }
    }
    const bool gliding = pairs > 0 && glide / pairs >= 0.01;   // on average more than 1 % from slot to slot
    const bool speech = nv >= 3 && gliding && ratioSum / spVoiced.size() >= 0.4 && movement >= 4.5 && bandDb > -60;
    if (speech) spHold = 0.5; else spHold -= 0.1;
    spBandSum = spFullSum = 0; spBuf.clear();
    return speech || spHold > 0;
}

void Analyzer::addToHistory(double e, int span) {
    const int S = spanSlots(span);
    historyRing[(size_t)(historyCount % kSlots)] = e;
    const int k = (int)((historyCount % S) * kSteps / S);
    if (k != newestStep) { stepEnergy[(size_t)k] = 0; stepCount[(size_t)k] = 0; newestStep = k; }
    stepEnergy[(size_t)k] += e; stepCount[(size_t)k] += 1;
    historyCount++;
}

// a new span: the steps are rebuilt from the history
void Analyzer::rebuildHistory(int span) {
    const int S = spanSlots(span);
    stepEnergy.fill(0); stepCount.fill(0);
    const long long n = std::min<long long>(historyCount, S);
    newestStep = -1;
    for (long long j = historyCount - n; j < historyCount; j++) {
        const int k = (int)((j % S) * kSteps / S);
        stepEnergy[(size_t)k] += historyRing[(size_t)(j % kSlots)]; stepCount[(size_t)k] += 1; newestStep = k;
    }
    builtSpan = span;
}

// the slot that just ended: momentary (last 4 slots), short-term (last 30), into the timeline and the history
void Analyzer::finishSlot(int span) {
    const double e = slotSum / (double)slotCount;
    contiguous = currentSlot == previousSlot + 1 ? contiguous + 1 : 1;   // a jump (timecode) starts the blocks again
    previousSlot = currentSlot;
    last30[(size_t)last30Pos] = e; last30Pos = (last30Pos + 1) % 30;
    double em = 0, es = 0;
    const int nm = std::min(4, contiguous), ns = std::min(30, contiguous);
    for (int i = 0; i < nm; i++) em += last30[(size_t)((last30Pos - 1 - i + 60) % 30)];
    for (int i = 0; i < ns; i++) es += last30[(size_t)((last30Pos - 1 - i + 60) % 30)];
    em /= nm; es /= ns;
    momentary = em > 0 ? energyToLufs(em) : -200;
    shortTerm = es > 0 ? energyToLufs(es) : -200;
    if (contiguous >= 4) momentaryMax = std::max(momentaryMax, momentary);    // the highest full 400 ms block
    if (contiguous >= 30) shortTermMax = std::max(shortTermMax, shortTerm);   // the highest full 3 s window
    speechNow = detectSpeech();
    store(currentSlot, contiguous >= 4 ? em : 0, contiguous >= 30 ? es : 0, speechNow);
    addToHistory(em, span);
    loudnessDirty = true;
}

// integrated (gates -70 and -10) and loudness range (gates -70 and -20, 10th to 95th percentile)
void Analyzer::loudnessValues() {
    double n = 0, e = 0;
    for (int b = 0; b < kHistBins; b++) { n += momCount[(size_t)b]; e += momEnergy[(size_t)b]; }
    integrated = -200;
    if (n > 0.5 && e > 0) {
        const int g = std::max(0, (int)std::ceil((energyToLufs(e / n) - 10 + 70) * 10));
        double n2 = 0, e2 = 0;
        for (int b = g; b < kHistBins; b++) { n2 += momCount[(size_t)b]; e2 += momEnergy[(size_t)b]; }
        if (n2 > 0.5 && e2 > 0) integrated = energyToLufs(e2 / n2);
    }
    // dialog-gated: the momentary blocks found to be dialog (absolute gate -70 LUFS)
    n = 0; e = 0;
    for (int b = 0; b < kHistBins; b++) { n += dlgCount[(size_t)b]; e += dlgEnergy[(size_t)b]; }
    dialog = (n > 0.5 && e > 0) ? energyToLufs(e / n) : -200;
    dialogShare = measuredSlots > 0 ? (double)dialogSlots / measuredSlots : 0;
    n = 0; e = 0;
    for (int b = 0; b < kHistBins; b++) { n += stCount[(size_t)b]; e += stEnergy[(size_t)b]; }
    range = -1;
    if (n > 0.5 && e > 0) {
        const int g = std::max(0, (int)std::ceil((energyToLufs(e / n) - 20 + 70) * 10));
        double n2 = 0;
        for (int b = g; b < kHistBins; b++) n2 += stCount[(size_t)b];
        if (n2 > 0.5) {
            double cum = 0; int lo = -1, hi = -1;
            for (int b = g; b < kHistBins; b++) {
                cum += stCount[(size_t)b];
                if (lo < 0 && cum >= 0.1 * n2) lo = b;
                if (hi < 0 && cum >= 0.95 * n2) hi = b;
            }
            range = std::max(0.0, (hi - lo) * 0.1);
        }
    }
    loudnessDirty = false;
}

// ---------------------------------------------------------------- per block
// holds and the full signal, from what the last block measured
void Analyzer::endOfBlock(const Settings& s, int n) {
    const bool clear = clearLows.load();
    const double dt = n / sampleRate;
    const int hm = s.hold;
    for (int i = 0; i < bands; i++) {
        const size_t b = (size_t)i;
        const double rr = corr[b];
        if (hasSignal[b]) {
            if (hm == 0 || rr < corrHeld[b] || clear) { corrHeld[b] = rr; held_t[b] = 0; }
            else { held_t[b] += dt; if (hm == 1 && held_t[b] > 3) corrHeld[b] = std::min(rr, corrHeld[b] + dt * 0.5); }
            // the deepest mono loss, held the same way (released at 6 dB per second)
            if (hm == 0 || clear || monoLoss[b] < monoHeld[b]) { monoHeld[b] = monoLoss[b]; monoHeld_t[b] = 0; }
            else { monoHeld_t[b] += dt; if (hm == 1 && monoHeld_t[b] > 3) monoHeld[b] = std::min(monoLoss[b], monoHeld[b] + dt * 6); }
        } else if (clear) {
            corrHeld[b] = 1; held_t[b] = 0; monoHeld[b] = 0;
        }
    }
    // the full signal, as on a classic correlation meter
    const double dd = std::sqrt(std::fabs(sxxAvg * syyAvg));
    total = dd > kTiny ? std::max(-1.0, std::min(1.0, sxyAvg / dd)) : 0;
    levelL = 10 * std::log10(sxxAvg + kTiny);
    levelR = 10 * std::log10(syyAvg + kTiny);
    // balance (left over right) and width (side over mid), for the goniometer page
    balanceDb = levelL - levelR;
    widthDb = 10 * std::log10((sxxAvg + syyAvg - 2 * sxyAvg) * 0.5 + kTiny) - 10 * std::log10((sxxAvg + syyAvg + 2 * sxyAvg) * 0.5 + kTiny);
    totalValid = levelL > -100 || levelR > -100;
    if (totalValid) {
        if (hm == 0 || total < totalHeld || clear) { totalHeld = total; totalHeldTime = 0; }
        else { totalHeldTime += dt; if (hm == 1 && totalHeldTime > 3) totalHeld = std::min(total, totalHeld + dt * 0.5); }
    } else if (clear) {
        totalHeld = 1; totalHeldTime = 0;
    }
    if (clear) clearLows = false;
}

template <class Real>
void Analyzer::process(const Real* const* ch, int n, const Settings& s, const Transport& t) {
    if (s.bandsIndex != lastBands || s.selectivity != lastSelectivity || sampleRate != lastRate || inputKey(s) != lastInput) setup(s);
    const int nch = s.surround ? 6 : 2;
    if (nch != channels) { channels = nch; loudnessReset(); }   // stereo <-> 5.1: a new measurement

    // loudness: a new sample rate or a new way of measuring starts a new measurement; Reset asks for one too
    if (loudnessRate != sampleRate) { loudnessFilters(); loudnessReset(); }
    if (s.measure != measureMode) { measureMode = s.measure; loudnessReset(); }
    if (resetLoudness.exchange(false)) loudnessReset();
    if (builtSpan != s.span) rebuildHistory(s.span);
    if (loudnessDirty) loudnessValues();
    measuring = !s.frozen && (s.measure == 0 || t.playing);   // Playback and Timecode: only while the DAW plays (or records)
    if (!measuring && slotCount > 0) { finishSlot(s.span); slotSum = 0; slotCount = 0; }   // stopped: the slot in progress counts as it is

    const double kav = 1 - std::exp(-1000 / (std::max(1.0, s.averagingMs) * sampleRate));
    const double vecK = 1 - std::exp(-1 / (0.03 * sampleRate));   // fast: the speaker shape moves with the music
    const bool running = !s.frozen;           // switched off: analysis stops, the display keeps the last picture
    if (running) {
        // the FFT work, spread over the blocks so no single block carries all of it:
        // short frame when due, then either the offset of the last long frame or a new long frame
        if (shortDue) { shortDue = false; shortFrame(s.averagingMs); }
        if (offsetDue) { offsetDue = false; findOffset(); }
        else if (longDue) { longDue = false; longFrame(s.averagingMs); offsetDue = (++offsetTurn & 1) != 0; }
        endOfBlock(s, n);
    }

    // BS.1770 channel weights: L, R, C 1.0, Ls, Rs 1.41 (+1.5 dB), LFE not counted
    static const double weight[6] = { 1.0, 1.0, 1.0, 0.0, 1.41, 1.41 };
    int gw = gonioWrite.load(std::memory_order_relaxed);
    float pk[6] = { 0, 0, 0, 0, 0, 0 };
    double x[6] = { 0, 0, 0, 0, 0, 0 };
    for (int i = 0; i < n; i++) {
        for (int c = 0; c < nch; c++) x[c] = (double)ch[c][i];
        if (running) {
            double xl, xr;
            if (s.surround) {
                if (s.pair == 1) { xl = x[4]; xr = x[5]; }                                  // surround: Ls / Rs
                else if (s.pair == 2) {                                                     // downmix Lo / Ro
                    xl = x[0] + s.dmxCentre * x[2] + s.dmxSurround * x[4] + s.dmxLfe * x[3];
                    xr = x[1] + s.dmxCentre * x[2] + s.dmxSurround * x[5] + s.dmxLfe * x[3];
                }
                else { xl = x[0]; xr = x[1]; }                                              // front: L / R
            } else { xl = x[0]; xr = x[1]; }
            denormal = -denormal;
            xl += denormal; xr += denormal;
            sxyAvg += (xl * xr - sxyAvg) * kav;
            sxxAvg += (xl * xl - sxxAvg) * kav;
            syyAvg += (xr * xr - syyAvg) * kav;
            // goniometer: rotated by 45 degrees, mid straight up, left to the upper left
            gonioX[(size_t)gw] = (float)((xr - xl) * 0.70711);
            gonioY[(size_t)gw] = (float)((xl + xr) * 0.70711);
            gw = (gw + 1) & (kGonioSize - 1);
            // history for the FFTs (the work itself runs per block)
            histL[(size_t)histWrite] = xl; histR[(size_t)histWrite] = xr;
            histWrite = (histWrite + 1) & 65535;
            if (++hopCount >= hopLong) { hopCount = 0; frameStart = (histWrite - fftLong + 65536) & 65535; longDue = true; }
            if (++hopCount2 >= hopShort) { hopCount2 = 0; frameStart2 = (histWrite - fftShort + 65536) & 65535; shortDue = true; }
        }
        if (running && nch == 6) {
            // 5.1 vectorscope: levels per channel, and every 32 samples the energy vector
            for (int c = 0; c < 6; c++) { channelPower[(size_t)c] += (x[c] * x[c] - channelPower[(size_t)c]) * vecK; vecAcc[(size_t)c] += x[c] * x[c]; }
            if (++vecCount >= 32) {
                static const double ang[6] = { -0.5236, 0.5236, 0.0, 0.0, -1.9199, 1.9199 };   // L R C (LFE) Ls Rs, radians
                double sum = 0, vx = 0, vy = 0;
                for (int c = 0; c < 6; c++) {
                    if (c == 3) continue;
                    const double e = vecAcc[(size_t)c];
                    sum += e; vx += e * std::sin(ang[c]); vy += e * std::cos(ang[c]);
                }
                const int w = vecWrite.load(std::memory_order_relaxed);
                const bool heard = sum > 32 * 1e-10;   // about -100 dBFS
                vecX[(size_t)w] = heard ? (float)(vx / sum) : std::numeric_limits<float>::quiet_NaN();
                vecY[(size_t)w] = heard ? (float)(vy / sum) : std::numeric_limits<float>::quiet_NaN();
                vecWrite.store((w + 1) & (kVecSize - 1), std::memory_order_release);
                vecAcc.fill(0); vecCount = 0;
            }
        }
        if (measuring) {
            // K-weighting per channel: shelf, then high-pass (transposed direct form II), on the main input
            double sum = 0;
            for (int c = 0; c < nch; c++) {
                if (weight[c] == 0) continue;
                auto& z = kState[(size_t)c];
                const double a = x[c] + denormal;
                const double y = kb0 * a + z[0]; z[0] = kb1 * a - ka1 * y + z[1]; z[1] = kb2 * a - ka2 * y;
                const double k = hb0 * y + z[2]; z[2] = hb1 * y - ha1 * k + z[3]; z[3] = hb2 * y - ha2 * k;
                sum += weight[c] * k * k;
            }
            // the 100 ms slot of this sample: by the DAW position (Timecode) or by the measured time
            const double secs = s.measure == 2 ? t.position + i / sampleRate : measuredSamples / sampleRate;
            const long long slot = (long long)std::floor(secs * 10 + 0.000001);
            if (slot != currentSlot) {
                if (slotCount > 0) finishSlot(s.span);
                currentSlot = slot; slotSum = 0; slotCount = 0;
            }
            slotSum += sum; slotCount++;
            measuredSamples++;
            // dialog detection: the band-limited mix (5.1: mostly the centre)
            const double d = nch == 6 ? x[2] + 0.25 * (x[0] + x[1]) : 0.5 * (x[0] + x[1]);
            const double h = spB[0] * d + spB[1] * spHx1 + spB[2] * spHx2 - spA[0] * spHy1 - spA[1] * spHy2;
            spHx2 = spHx1; spHx1 = d; spHy2 = spHy1; spHy1 = h + denormal;
            const double b = spB[3] * h + spB[4] * spLx1 + spB[5] * spLx2 - spA[2] * spLy1 - spA[3] * spLy2;
            spLx2 = spLx1; spLx1 = h; spLy2 = spLy1; spLy1 = b + denormal;
            spBandSum += b * b; spFullSum += d * d;
            spDecAcc += b;
            if (++spDecCount >= spDecim) { if (spBuf.size() < 4096) spBuf.push_back(spDecAcc / spDecim); spDecAcc = 0; spDecCount = 0; }
            spSubSum += b * b;
            if (++spSubCount >= spSubLen) {
                spSubDb[(size_t)spSubPos] = 10 * std::log10(spSubSum / spSubLen + 1e-20);
                spSubPos = (spSubPos + 1) % (int)spSubDb.size(); spSubFilled++;
                spSubSum = 0; spSubCount = 0;
            }
        }
        if (running) {
            // true peak per channel: the samples and the points between them
            double all = 0;
            for (int c = 0; c < nch; c++) {
                auto& hist = tpHist[(size_t)c];
                hist[(size_t)tpWrite] = x[c];
                double p = std::fabs(x[c]);
                for (int ph = 0; ph < oversample; ph++) {
                    double v = 0;
                    for (int k = 0; k < 12; k++) v += tpCoef[(size_t)(ph + oversample * k)] * hist[(size_t)((tpWrite - k) & 15)];
                    p = std::max(p, std::fabs(v));
                }
                pk[c] = std::max(pk[c], (float)p);
                all = std::max(all, p);
            }
            tpWrite = (tpWrite + 1) & 15;
            if (measuring) truePeakMax = std::max(truePeakMax, all);
        }
    }
    gonioWrite.store(gw, std::memory_order_release);
    // the meters take the peaks and set them back to 0
    for (int c = 0; c < nch; c++)
        if (pk[c] > peaks[(size_t)c].load(std::memory_order_relaxed)) peaks[(size_t)c].store(pk[c], std::memory_order_relaxed);
}

template void Analyzer::process<float>(const float* const*, int, const Settings&, const Transport&);
template void Analyzer::process<double>(const double* const*, int, const Settings&, const Transport&);

} // namespace bn
