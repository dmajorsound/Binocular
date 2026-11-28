# Binocular 1.2.0 – VST3 / AU / CLAP

Multiband correlation meter, goniometer and loudness meter (ITU-R BS.1770 / EBU R128, with timecode measuring)
by dmajorsound (dmajorsound.com) as VST3, CLAP and (macOS) AU.
The audio passes through untouched, no latency.

Binocular is plain C++ (JUCE). There is no JSFX version any more.

## Install on the Mac (no programming needed)

**Installer:** on GitHub, open *Releases* and download `Binocular-1.2.0-mac.dmg` (Mac) or
`Binocular-1.2.0-win64.exe` (Windows). On the Mac, open the .dmg and follow the three steps in its window
(Install, then "Blocked? Open Privacy & Security" if macOS refuses the unsigned installer, then Install again). It installs
VST3, CLAP and (Mac) AU into the common plug-in folders. "Uninstall Binocular" in the .dmg removes it again.

**Download without installer:** on GitHub, open *Actions* → the newest green run → *Artifacts* → `Binocular_1.2.0_mac`. Unzip,
then copy `Binocular.vst3` into `~/Library/Audio/Plug-Ins/VST3/` and `Binocular.component` into
`~/Library/Audio/Plug-Ins/Components/`. In REAPER: *Options → Preferences → Plug-ins → VST → Re-scan*.

**Build yourself:** Terminal, in this folder: `bash build_mac.sh`. The script checks the tools, fetches JUCE,
builds and installs. The first build takes 5–15 minutes.

## Sidechain

The plug-in has a stereo sidechain bus (inputs 3/4). In REAPER, route the second source to channels 3/4 of the track
and pick *Sidechain* in the plug-in. Loudness is always measured on the main input.

## Code

- `src/Analyzer.*` – the measuring, on the audio thread: multiband correlation and mono loss from an FFT cross-spectrum
  (long FFT for the bass, short FFT for the rest), time offset and polarity (GCC-PHAT), the goniometer buffer, and
  loudness (ITU-R BS.1770-4 K-weighting, momentary, short-term, integrated with the EBU R128 gates, loudness range
  after EBU Tech 3342, true peak with 4x oversampling, a 5-hour timeline of 100 ms slots for timecode measuring).
- `src/PluginProcessor.*` – parameters (IDs `s1` … `s12`, unchanged since the first version, so old projects load),
  state, audio buses.
- `src/PluginEditor.*`, `src/Look.h` – the window: drawn with JUCE in units of a 680 × 520 layout, scaled to any size.
- `src/UpdateCheck.*` – the update note in the footer (GitHub releases, download and install).
- `fonts/` – Mulish and Red Hat Mono (SIL Open Font License), embedded.
- `third_party/wdl/` – the FFT.

## Tests

- `tests/update_download.cpp` – the GitHub build downloads a published installer the way the update note does.
- The analyzer was checked against the earlier engine with the same test signals (4 to 256 bands, 44.1 to 192 kHz,
  block sizes 33 to 2048, stereo and sidechain, offset, polarity flipped, mono, tone): every reading identical to within
  1e-13 (the goniometer buffer, now stored as float, within 3e-8). Loudness against reference signals: 1 kHz at
  -20 dBFS on both sides reads -19.99 LUFS (EBU tolerance ±0.1), the EBU Tech 3342 range case reads 10.0 LU.
