// Binocular - the measuring part: multiband correlation, mono loss, offset and polarity, goniometer, loudness.
// Runs on the audio thread. The editor reads the results without a lock (a reading may be one block old,
// which a meter never shows); requests from the editor go through atomics.
#pragma once
#include <array>
#include <atomic>
#include <cmath>
#include <vector>

namespace bn {

// the settings of one block, read from the parameters by the processor
struct Settings {
    int bandsIndex = 7;          // 0..9: 4, 8, 12, 16, 24, 32, 48, 64, 128, 256 bands
    double selectivity = 50;     // %
    double averagingMs = 300;
    int hold = 1;                // 0 off, 1 three seconds, 2 infinite
    bool frozen = false;         // switched off: the meter keeps its last picture
    int measure = 1;             // loudness: 0 always, 1 while the DAW plays, 2 by timecode
    int span = 1;                // history: 0..6 = 1, 5, 15, 30 min, 1, 2, 5 h
    double dmxCentre = 0.70710678, dmxSurround = 0.70710678, dmxLfe = 0;   // downmix gains (linear)
    bool surround = false;       // the track is 5.1: channels come as L, R, C, LFE, Ls, Rs
    int pair = 0;                // 5.1: what the correlation pages compare: 0 front (L / R), 1 surround (Ls / Rs),
                                 //      2 the stereo downmix (Lo / Ro after ITU-R BS.775: centre and surrounds at -3 dB, no LFE)
};

struct Transport {
    bool playing = false;        // playing or recording
    double position = 0;         // seconds from the project start
};

class Analyzer {
public:
    static constexpr int kMaxBands = 256;
    static constexpr int kBandCounts[10] = {4, 8, 12, 16, 24, 32, 48, 64, 128, 256};
    static constexpr int kGonioSize = 8192;                 // goniometer ring (a power of 2)
    static constexpr int kSlots = 180000;                   // loudness timeline: 5 hours of 100 ms
    static constexpr int kHistBins = 800;                   // 0.1 LU from -70 to +10 LUFS
    static constexpr int kSteps = 360;                      // history graph steps

    Analyzer();

    // a fresh start at this sample rate: every reading, the loudness measurement, the history
    void prepare(double sampleRate, const Settings& s);

    // ch: the input, 2 channels (L, R) or 6 (L, R, C, LFE, Ls, Rs)
    template <class Real>
    void process(const Real* const* ch, int n, const Settings& s, const Transport& t);

    // ---- requests from the editor
    std::atomic<bool> clearLows { false };      // the held lows start again
    std::atomic<bool> resetLoudness { false };  // a new loudness measurement

    // ---- readings (audio thread writes, editor reads)
    double sampleRate = 48000;
    int bands = 64;                             // bands in use
    int setupCount = 0;                         // counts every new band layout (the editor drops its reference curve)
    double lowHz = 20, highHz = 20000;
    std::array<double, kMaxBands> corr {}, level {}, monoLoss {}, corrHeld {}, monoHeld {}, centerHz {};
    std::array<bool, kMaxBands> hasSignal {};

    // the full signal
    double total = 0, totalHeld = 1;            // correlation
    bool totalValid = false;
    double levelL = -200, levelR = -200, balanceDb = 0, widthDb = -60;

    // time offset and polarity between the sides
    bool offsetFound = false;
    double offsetSamples = 0;                   // negative: the second side (right) is late
    int polarity = 1;

    // goniometer: side (x) and mid (y) of the last samples, the write position
    std::array<float, kGonioSize> gonioX {}, gonioY {};
    std::atomic<int> gonioWrite { 0 };

    // 5.1 vectorscope: where the sound comes from. Every 32 samples the energy vector of L, R, C, Ls, Rs
    // (speakers at -30, +30, 0, -110, +110 degrees after ITU-R BS.775): x to the right, y to the front, length 0..1
    // (1 = one speaker alone, toward 0 = spread over all). Silence is stored as NaN.
    static constexpr int kVecSize = 4096;
    std::array<float, kVecSize> vecX {}, vecY {};
    std::atomic<int> vecWrite { 0 };
    std::array<double, 6> channelPower {};      // per channel, averaged over 30 ms

    // loudness
    int channels = 2;                           // 2 or 6 (5.1)
    bool measuring = false;
    double dialog = -200;                       // dialog-gated integrated loudness (only the parts found to be dialog)
    double dialogShare = 0;                     // how much of the measurement is dialog, 0..1
    bool speechNow = false;                     // the detector hears dialog right now
    double momentary = -200, shortTerm = -200, integrated = -200, range = -1;
    double truePeakMax = 0;                     // linear, 0 = none yet
    double momentaryMax = -200, shortTermMax = -200;
    int measuredSlots = 0;                      // 100 ms slots in the measurement
    std::array<std::atomic<float>, 6> peaks {};   // true peak per channel since the editor last took it
    // history graph: 360 steps of momentary energy (sum and count), the newest step
    std::array<double, kSteps> stepEnergy {}, stepCount {};
    int newestStep = -1;
    long long historyCount = 0;                 // measured slots so far

    static int spanSlots(int span);

private:
    void setup(const Settings& s);
    void spectrum(int n, int start, const std::vector<double>& win, double* sx, double* sy, double* sc, double* si, double kf);
    void bandsFromSpectrum(int engine, int n, const double* sx, const double* sy, const double* sc, double* cx, double* cy, double* cc);
    void band(int i, double xx, double yy, double xy);
    void longFrame(double averagingMs);
    void shortFrame(double averagingMs);
    void findOffset();
    void endOfBlock(const Settings& s, int n);

    // loudness
    void loudnessFilters();
    void loudnessReset();
    void histogram(double e, double* counts, double* energies, double sign);
    void store(long long slot, double em, double es, bool isDialog);
    void addToHistory(double e, int span);
    void rebuildHistory(int span);
    void finishSlot(int span);
    void loudnessValues();
    bool detectSpeech();

    // correlation engine
    int fftLong = 16384, hopLong = 1024, fftShort = 4096, hopShort = 512;
    std::array<double, kMaxBands> held_t {}, monoHeld_t {}, edgeLo {}, edgeHi {};
    std::array<bool, kMaxBands> shortEngine {};
    std::vector<double> histL, histR;           // input history, 65536 each
    std::vector<double> fftBuf;                 // complex, up to 32768
    std::vector<double> winLong, winShort;
    std::vector<double> sxx, syy, sxy, sxi, cxx, cyy, cxy;          // long spectrum: smoothed powers, cross part, running sums
    std::vector<double> s2x, s2y, s2c, c2x, c2y, c2c;               // short spectrum
    int histWrite = 0, hopCount = 0, hopCount2 = 0, frameStart = 0, frameStart2 = 0, offsetTurn = 0;
    bool longDue = false, shortDue = false, offsetDue = false;
    double sxyAvg = 0, sxxAvg = 0, syyAvg = 0;  // the full signal
    double totalHeldTime = 0;
    std::array<double, 6> vecAcc {};
    int vecCount = 0;
    double denormal = 1e-18;                    // alternating -360 dB offset keeps the meter averages out of denormals
    int lastBands = -1, lastInput = -1;
    double lastSelectivity = -1, lastRate = -1;

    // loudness engine
    double kb0 = 0, kb1 = 0, kb2 = 0, ka1 = 0, ka2 = 0, hb0 = 1, hb1 = -2, hb2 = 1, ha1 = 0, ha2 = 0;
    std::array<std::array<double, 4>, 6> kState {};   // K-weighting per channel: shelf z1 z2, high-pass z1 z2
    int oversample = 4;
    std::array<double, 48> tpCoef {};
    std::array<std::array<double, 16>, 6> tpHist {};
    int tpWrite = 0;
    std::vector<double> slotMom, slotShort;     // per slot of the timeline
    std::vector<char> slotUsed, slotDialog;
    std::array<double, kHistBins> dlgCount {}, dlgEnergy {};
    int dialogSlots = 0;

    // dialog detection: a band-limited mix (centre-weighted in 5.1), its voicing (pitch periodicity), its share of the
    // energy and the syllable rhythm of its level. Decided per 100 ms slot, held for half a second.
    std::array<double, 2> spHp {}, spLp {};     // band-pass 250 Hz .. 3.5 kHz, biquad states (x1, x2 / y1, y2 style below)
    double spB[6] {}, spA[4] {};                // high-pass b0 b1 b2, low-pass b0 b1 b2 / a1 a2 each (shared form)
    double spHx1 = 0, spHx2 = 0, spHy1 = 0, spHy2 = 0, spLx1 = 0, spLx2 = 0, spLy1 = 0, spLy2 = 0;
    double spBandSum = 0, spFullSum = 0, spDecAcc = 0, spSubSum = 0;
    int spDecim = 6, spDecCount = 0, spSubCount = 0, spSubLen = 1200;
    std::vector<double> spBuf;                  // the band-limited mix at ~8 kHz, one slot
    std::array<double, 40> spSubDb {};          // level of the last 40 sub-frames of 25 ms (1 s), dB
    int spSubPos = 0, spSubFilled = 0;
    std::array<char, 10> spVoiced {};           // voicing of the last 10 slots
    int spVoicedPos = 0;
    std::array<double, 10> spRatio {};          // speech-band share of the last 10 slots
    std::array<double, 10> spPitch {};          // pitch of the last 10 slots, Hz (0 = not voiced)
    double spHold = 0;                          // seconds the dialog state still holds
    std::vector<double> historyRing;            // momentary energy of every measured slot, in order
    std::array<double, kHistBins> momCount {}, momEnergy {}, stCount {}, stEnergy {};
    std::array<double, 30> last30 {};
    int last30Pos = 0, contiguous = 0, measureMode = -1, builtSpan = -1;
    long long currentSlot = -1000000000, previousSlot = -1000000000, measuredSamples = 0;
    double slotSum = 0;
    long long slotCount = 0;
    bool loudnessDirty = false;
    double loudnessRate = -1;
};

} // namespace bn
