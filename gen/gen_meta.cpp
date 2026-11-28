// GENERATED
#include "meta.h"
namespace rs {
const char* kDesc = "Binocular";
const SliderInfo kSliders[] = {
  {1, 7.0, 0.0, 9.0, 1.0, "Bands", true, "4|8|12|16|24|32|48|64|128|256"},
  {2, 50.0, 0.0, 100.0, 0.1, "Selectivity (%)", true, nullptr},
  {3, 300.0, 10.0, 5000.0, 1.0, "Averaging (ms)", true, nullptr},
  {4, 0.0, 0.0, 1.0, 1.0, "Input", true, "Stereo|Sidechain"},
  {5, 1.0, 0.0, 2.0, 1.0, "Hold", true, "Off|3 s|Infinite"},
  {6, 0.0, 0.0, 1.0, 1.0, "Freeze", true, "Off|On"},
  {7, 0.0, 0.0, 1.0, 1.0, "Theme", true, "Dark|Light"},
  {8, 0.0, 0.0, 3.0, 1.0, "View", true, "Correlation|Mono loss|Goniometer|Loudness"},
  {9, 1.0, 0.0, 2.0, 1.0, "Range", true, "24 dB|48 dB|96 dB"},
  {10, 0.0, 0.0, 5.0, 1.0, "Loudness target", true, "EBU R128|Netflix|Spotify|YouTube|Apple Music|Tidal"},
  {11, 1.0, 0.0, 2.0, 1.0, "Measure", true, "Always|Playback|Timecode"},
  {12, 1.0, 0.0, 6.0, 1.0, "History span", true, "1 min|5 min|15 min|30 min|1 h|2 h|5 h"},
};
const int kNumSliders = 12;
}
