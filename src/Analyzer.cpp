#include "Analyzer.h"
#include "fft.h"
#include <algorithm>
#include <cstring>

namespace bn {

static constexpr double kTiny = 1e-24, kTiny12 = 1e-12;
static constexpr double kPi = 3.141592653589793;

static double energyToLufs(double e) { return -0.691 + 10.0 * std::log10(e); }

constexpr int Analyzer::kBandCounts[10];

Analyzer::Analyzer() {
    static bool fftReady = false;
    if (!fftReady) { WDL_fft_init(); fftReady = true; }
    histL.assign(65536, 0.0); histR.assign(65536, 0.0);
    fftBuf.assign(65536, 0.0);
    for (auto* v : {&sxx, &syy, &sxy, &sxi, &cxx, &cyy, &cxy}) v->assign(16400, 0.0);
    for (auto* v : {&s2x, &s2y, &s2c, &c2x, &c2y, &c2c}) v->assign(8200, 0.0);
    slotMom.assign(kSlots, 0.0); slotShort.assign(kSlots, 0.0); slotUsed.assign(kSlots, 0);
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
    lastBands = s.bandsIndex; lastSelectivity = s.selectivity; lastRate = sampleRate; lastInput = s.sidechain ? 1 : 0;
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
    loudnessRate = sampleRate;
}

void Analyzer::loudnessReset() {
    std::fill(slotMom.begin(), slotMom.end(), 0.0); std::fill(slotShort.begin(), slotShort.end(), 0.0);
    std::fill(slotUsed.begin(), slotUsed.end(), 0);
    momCount.fill(0); momEnergy.fill(0); stCount.fill(0); stEnergy.fill(0);
    last30.fill(0); stepEnergy.fill(0); stepCount.fill(0); tpHistL.fill(0); tpHistR.fill(0);
    k1L = k2L = k1R = k2R = h1L = h2L = h1R = h2R = 0;
    currentSlot = -1000000000; previousSlot = currentSlot; contiguous = 0; slotSum = 0; slotCount = 0; measuredSamples = 0;
    last30Pos = 0; momentary = shortTerm = -200; measuredSlots = 0; historyCount = 0; newestStep = -1;
    integrated = -200; range = -1; truePeakMax = 0; tpWrite = 0;
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
void Analyzer::store(long long slot, double em, double es) {
    const long long i = ((slot % kSlots) + kSlots) % kSlots;
    if (slotUsed[(size_t)i]) {
        histogram(slotMom[(size_t)i], momCount.data(), momEnergy.data(), -1);
        histogram(slotShort[(size_t)i], stCount.data(), stEnergy.data(), -1);
        measuredSlots--;
    }
    slotMom[(size_t)i] = em; slotShort[(size_t)i] = es; slotUsed[(size_t)i] = 1;
    histogram(em, momCount.data(), momEnergy.data(), 1);
    histogram(es, stCount.data(), stEnergy.data(), 1);
    measuredSlots++;
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
    store(currentSlot, contiguous >= 4 ? em : 0, contiguous >= 30 ? es : 0);
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
void Analyzer::process(const Real* inL, const Real* inR, const Real* scL, const Real* scR, int n, const Settings& s, const Transport& t) {
    if (s.bandsIndex != lastBands || s.selectivity != lastSelectivity || sampleRate != lastRate || (s.sidechain ? 1 : 0) != lastInput) setup(s);

    // loudness: a new sample rate or a new way of measuring starts a new measurement; Reset asks for one too
    if (loudnessRate != sampleRate) { loudnessFilters(); loudnessReset(); }
    if (s.measure != measureMode) { measureMode = s.measure; loudnessReset(); }
    if (resetLoudness.exchange(false)) loudnessReset();
    if (builtSpan != s.span) rebuildHistory(s.span);
    if (loudnessDirty) loudnessValues();
    measuring = !s.frozen && (s.measure == 0 || t.playing);   // Playback and Timecode: only while the DAW plays (or records)
    if (!measuring && slotCount > 0) { finishSlot(s.span); slotSum = 0; slotCount = 0; }   // stopped: the slot in progress counts as it is

    const double kav = 1 - std::exp(-1000 / (std::max(1.0, s.averagingMs) * sampleRate));
    const bool running = !s.frozen;           // switched off: analysis stops, the display keeps the last picture
    if (running) {
        // the FFT work, spread over the blocks so no single block carries all of it:
        // short frame when due, then either the offset of the last long frame or a new long frame
        if (shortDue) { shortDue = false; shortFrame(s.averagingMs); }
        if (offsetDue) { offsetDue = false; findOffset(); }
        else if (longDue) { longDue = false; longFrame(s.averagingMs); offsetDue = (++offsetTurn & 1) != 0; }
        endOfBlock(s, n);
    }

    int gw = gonioWrite.load(std::memory_order_relaxed);
    float pkL = 0, pkR = 0;
    for (int i = 0; i < n; i++) {
        const double s0 = (double)inL[i], s1 = (double)inR[i];
        if (running) {
            double xl, xr;
            if (s.sidechain) { xl = (s0 + s1) * 0.5; xr = ((double)scL[i] + (double)scR[i]) * 0.5; }
            else { xl = s0; xr = s1; }
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
        if (measuring) {
            // K-weighting: shelf, then high-pass (transposed direct form II), on the main input
            const double a = s0 + denormal, b = s1 + denormal;
            double y = kb0 * a + k1L; k1L = kb1 * a - ka1 * y + k2L; k2L = kb2 * a - ka2 * y;
            const double zl = hb0 * y + h1L; h1L = hb1 * y - ha1 * zl + h2L; h2L = hb2 * y - ha2 * zl;
            y = kb0 * b + k1R; k1R = kb1 * b - ka1 * y + k2R; k2R = kb2 * b - ka2 * y;
            const double zr = hb0 * y + h1R; h1R = hb1 * y - ha1 * zr + h2R; h2R = hb2 * y - ha2 * zr;
            // the 100 ms slot of this sample: by the DAW position (Timecode) or by the measured time
            const double secs = s.measure == 2 ? t.position + i / sampleRate : measuredSamples / sampleRate;
            const long long slot = (long long)std::floor(secs * 10 + 0.000001);
            if (slot != currentSlot) {
                if (slotCount > 0) finishSlot(s.span);
                currentSlot = slot; slotSum = 0; slotCount = 0;
            }
            slotSum += zl * zl + zr * zr; slotCount++;
            measuredSamples++;
        }
        if (running) {
            // true peak per side: the samples and the points between them
            tpHistL[(size_t)tpWrite] = s0; tpHistR[(size_t)tpWrite] = s1;
            double pl = std::fabs(s0), pr = std::fabs(s1);
            for (int p = 0; p < oversample; p++) {
                double l = 0, r = 0;
                for (int k = 0; k < 12; k++) {
                    const double c = tpCoef[(size_t)(p + oversample * k)];
                    const int j = (tpWrite - k) & 15;
                    l += c * tpHistL[(size_t)j]; r += c * tpHistR[(size_t)j];
                }
                pl = std::max(pl, std::fabs(l)); pr = std::max(pr, std::fabs(r));
            }
            tpWrite = (tpWrite + 1) & 15;
            pkL = std::max(pkL, (float)pl); pkR = std::max(pkR, (float)pr);
            if (measuring) truePeakMax = std::max(truePeakMax, std::max(pl, pr));
        }
    }
    gonioWrite.store(gw, std::memory_order_release);
    // the meters take the peaks and set them back to 0
    if (pkL > peakL.load(std::memory_order_relaxed)) peakL.store(pkL, std::memory_order_relaxed);
    if (pkR > peakR.load(std::memory_order_relaxed)) peakR.store(pkR, std::memory_order_relaxed);
}

template void Analyzer::process<float>(const float*, const float*, const float*, const float*, int, const Settings&, const Transport&);
template void Analyzer::process<double>(const double*, const double*, const double*, const double*, int, const Settings&, const Transport&);

} // namespace bn
