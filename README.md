# Binocular 1.0.2 – VST3 / AU / CLAP

Multiband correlation meter and goniometer by dmajorsound (dmajorsound.com). Native C++ port of `jsfx/Binocular.jsfx`
as VST3 (macOS additionally AU). Audio and interface are those of the JSFX; the audio passes through untouched, no latency.

## Install on the Mac (no programming needed)

**Installer:** on GitHub, open *Releases* and download `Binocular_1.0.2_installer_mac.pkg` (Mac) or
`Binocular_1.0.2_installer_win64.exe` (Windows). Open it and click through; it installs VST3, CLAP and (Mac) AU into the
common plug-in folders. Mac: the installer is not signed by Apple, so right-click it and choose *Open* the first time.

**Download without installer:** on GitHub, open *Actions* → the newest green run → *Artifacts* → `Binocular_1.0.2_mac`. Unzip,
then copy `Binocular.vst3` into `~/Library/Audio/Plug-Ins/VST3/` and `Binocular.component` into
`~/Library/Audio/Plug-Ins/Components/`. In REAPER: *Options → Preferences → Plug-ins → VST → Re-scan*.

**Build yourself:** Terminal, in this folder: `bash build_mac.sh`. The script checks the tools, fetches JUCE,
builds and installs. The first build takes 5–15 minutes.

## Sidechain

The second input pair of the JSFX (inputs 3/4) is a sidechain bus in the VST3. In REAPER, route the second source
to channels 3/4 of the track and pick *Sidechain* in the plugin.

## How the port works

The JSFX code is translated to C++ by a program, not by hand: `tools/transpile.py` → `gen/`. The translator follows
the arithmetic of REAPER's EEL2 exactly (operator order, fuzzy compares, denormal handling, memory indexing).
`src/rt.*` is the runtime (memory, strings, FFT from WDL), `src/engine.*` the JSFX host flow, `src/JuceGfx.*` maps the
gfx drawing to JUCE, `src/PluginEditor.*` runs `@gfx` at 30 fps.

Change the JSFX, then: `python3 tools/transpile.py jsfx/Binocular.jsfx gen`, then build. Do not edit `gen/` by hand.
Do not remove `-ffp-contract=off` (or `/fp:precise`): the arithmetic would no longer match the JSFX.

## Tests (Linux)

- `test/refcheck.cpp`: runs the original JSFX in ysfx (the EEL2 engine), `test/natcheck.cpp` the generated C++,
  both with the same 4-channel test signals; `test/run.sh` compares every meter value (correlation and mono loss per
  band, total, offset, polarity, balance, width, goniometer buffer).
- `test/host/`: small VST3 host – editor on screen, state save/load round trip.

## Status

- 10 comparisons JSFX ↔ C++ bit-identical (4 to 256 bands, 44.1 to 192 kHz, block sizes 33 to 2048, stereo and sidechain,
  offset, polarity flipped, mono, tone)
- VST3 binary: loads, editor draws all three pages, state save/load byte-identical
- Tested only on Linux. Not yet tested: REAPER, macOS, Windows, Retina.
