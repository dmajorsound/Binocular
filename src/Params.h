// Binocular - the parameters. The IDs ("s1" ... "s20") and ranges stay as they were, so projects and automation
// saved with earlier versions load unchanged.
#pragma once

namespace bn {

struct ParamInfo {
    int number;             // 1-based, also the ID "s<number>"
    const char* name;
    double lo, hi, step, def;
    const char* choices;    // "a|b|c" for a choice, nullptr for a value
};

enum Param { pBands = 1, pSelectivity, pAveraging, pInputRemoved /* 4: the sidechain switch, removed */, pHold, pFreeze, pTheme, pView, pRange, pTarget, pMeasure, pSpan, pPair,
                 pDownmix, pDmxCentre, pDmxSurround, pDmxLfe, pFps, pTcStart, pColors };

inline constexpr ParamInfo kParams[] = {
    { pBands,       "Bands",           0, 9,    1,   7,   "4|8|12|16|24|32|48|64|128|256" },
    { pSelectivity, "Selectivity (%)", 0, 100,  0.1, 50,  nullptr },
    { pAveraging,   "Averaging (ms)",  10, 5000, 1,  300, nullptr },
    { pHold,        "Hold",            0, 2,    1,   1,   "Off|3 s|Infinite" },
    { pFreeze,      "Freeze",          0, 1,    1,   0,   "Off|On" },
    { pTheme,       "Theme",           0, 1,    1,   0,   "Dark|Light" },
    { pView,        "View",            0, 3,    1,   0,   "Correlation|Mono loss|Goniometer|Loudness" },
    { pRange,       "Range",           0, 2,    1,   1,   "24 dB|48 dB|96 dB" },
    { pTarget,      "Loudness target", 0, 9,    1,   0,   "EBU R128|Netflix|Spotify|YouTube|Apple Music|Tidal|Amazon Prime Video|ATSC A/85|ARIB TR-B32|OP-59" },
    { pMeasure,     "Measure",         0, 2,    1,   1,   "Always|Playback|Timecode" },
    { pSpan,        "History span",    0, 6,    1,   1,   "1 min|5 min|15 min|30 min|1 h|2 h|5 h" },
    { pPair,        "5.1 compare",     0, 2,    1,   0,   "Front|Surround|Downmix" },
    { pDownmix,     "Downmix",         0, 2,    1,   0,   "ITU-R BS.775|Modern|Manual" },
    { pDmxCentre,   "Downmix centre (dB)",   -24, 3,  0.5, -3,  nullptr },
    { pDmxSurround, "Downmix surround (dB)", -24, 0,  0.5, -3,  nullptr },
    { pDmxLfe,      "Downmix LFE (dB)",      -40, 10, 0.5, -40, nullptr },   // -40 = off
    { pFps,         "Timecode rate",   0, 5,    1,   2,   "23.976|24|25|29.97 DF|29.97|30" },
    { pTcStart,     "Timecode start",  0, 2,    1,   0,   "00:00:00:00|01:00:00:00|10:00:00:00" },
    { pColors,      "Colors",          0, 1,    1,   0,   "Standard|Colorblind-safe" },
};
inline constexpr int kNumParams = (int)(sizeof(kParams) / sizeof(kParams[0]));

} // namespace bn
