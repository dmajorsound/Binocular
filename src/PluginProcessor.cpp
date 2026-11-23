#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::String enumLabel(const rs::SliderInfo& s, double v) {
    juce::StringArray items;
    items.addTokens(juce::String::fromUTF8(s.enums), "|", "");
    const double step = s.step > 0 ? s.step : 1.0;
    const int i = (int)std::floor((v - s.lo) / step + 0.5);
    return juce::isPositiveAndBelow(i, items.size()) ? items[i] : juce::String(v);
}

BinocularProcessor::BinocularProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
    paramBySlider.assign(257, nullptr);
    lastPushed.reset(new std::atomic<float>[257]);
    for (int i = 0; i < 257; i++) lastPushed[i] = 0.0f;
    touching.assign(257, 0);
    for (int i = 0; i < rs::kNumSliders; i++) {
        const rs::SliderInfo& s = rs::kSliders[i];
        juce::NormalisableRange<float> range((float)s.lo, (float)s.hi, (float)juce::jmax(0.0, s.step));
        auto attr = juce::AudioParameterFloatAttributes();
        if (s.enums) {
            const rs::SliderInfo* sp = &s;
            attr = attr.withStringFromValueFunction([sp](float v, int) { return enumLabel(*sp, v); });
        }
        auto* p = new juce::AudioParameterFloat(juce::ParameterID("s" + juce::String(s.idx), 1),
                                                juce::String::fromUTF8(s.label), range, (float)s.def, attr);
        addParameter(p);
        paramBySlider[(size_t)s.idx] = p;
        lastPushed[s.idx] = p->convertFrom0to1(p->convertTo0to1((float)s.def));
    }
    engine.onAutomate = [this](int idx, int mode) { scriptAutomate(idx, mode); };
    engine.onChanged = [this](int idx) { scriptChanged(idx); };
}

bool BinocularProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    const auto mono = juce::AudioChannelSet::mono(), stereo = juce::AudioChannelSet::stereo();
    if (l.getMainOutputChannelSet() != stereo) return false;
    if (l.getMainInputChannelSet() != stereo && l.getMainInputChannelSet() != mono) return false;
    if (l.inputBuses.size() > 1) {                 // sidechain: off, mono or stereo
        const auto sc = l.getChannelSet(true, 1);
        if (!sc.isDisabled() && sc != stereo && sc != mono) return false;
    }
    return true;
}

void BinocularProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    const juce::ScopedLock sl(engineLock);
    engine.setSampleRate(sampleRate);
    engine.setBlockSize(samplesPerBlock);
    scSilence.assign((size_t)juce::jmax(1, samplesPerBlock) * 2, 0.0);
}

// host / automation -> script: only real host changes are pushed (a value the script wrote itself is not sent back,
// so its double precision survives)
void BinocularProcessor::syncParamsToEngine() {
    for (int i = 0; i < rs::kNumSliders; i++) {
        const int n = rs::kSliders[i].idx;
        auto* p = paramBySlider[(size_t)n];
        const float v = p->convertFrom0to1(p->getValue());
        const float last = lastPushed[n].load();
        if (v != last) {
            lastPushed[n] = v;
            // a value the script wrote itself comes back from the host as float (VST3 echoes edits): within float
            // precision of the script's own double it is the same value - keep the double
            const double cur = engine.getSlider(n);
            const double tol = 4.0e-7 * juce::jmax(1.0, std::fabs(cur), std::fabs(rs::kSliders[i].hi - rs::kSliders[i].lo));
            if (std::fabs((double)v - cur) > tol) engine.setSlider(n, (double)v);
        }
    }
}

// script (slider_automate) -> host
void BinocularProcessor::scriptAutomate(int idx, int mode) {
    if (idx < 1 || idx > 256 || !paramBySlider[(size_t)idx]) return;
    auto* p = paramBySlider[(size_t)idx];
    const float v = (float)engine.getSlider(idx);
    const double t0 = juce::Time::getMillisecondCounterHiRes();
    if (mode == 0 && !touching[(size_t)idx]) { p->beginChangeGesture(); touching[(size_t)idx] = 1; }
    lastPushed[idx] = p->convertFrom0to1(p->convertTo0to1(v));
    p->setValueNotifyingHost(p->convertTo0to1(v));
    if (mode == 1 && touching[(size_t)idx]) { p->endChangeGesture(); touching[(size_t)idx] = 0; }
    const double dt = juce::Time::getMillisecondCounterHiRes() - t0;
    diag.notifyCount++; diag.notifyMs = diag.notifyMs + dt; if (dt > diag.notifyMaxMs) diag.notifyMaxMs = dt;
}

// script (sliderchange) -> host display
void BinocularProcessor::scriptChanged(int idx) {
    for (int i = 0; i < rs::kNumSliders; i++) {
        const int n = rs::kSliders[i].idx;
        if (idx >= 0 && n != idx) continue;
        auto* p = paramBySlider[(size_t)n];
        const float v = (float)engine.getSlider(n);
        if (p->convertFrom0to1(p->getValue()) != v) {
            lastPushed[n] = p->convertFrom0to1(p->convertTo0to1(v));
            p->setValueNotifyingHost(p->convertTo0to1(v));
        }
    }
}

template <class Real>
void BinocularProcessor::process(juce::AudioBuffer<Real>& buffer) {
    // no ScopedNoDenormals: the script handles denormals itself (assignments flush them, as in EEL2), and FTZ/DAZ
    // would change the arithmetic against the JSFX
    const juce::ScopedLock sl(engineLock);
    syncParamsToEngine();
    rs::TimeInfo ti;
    if (auto* ph = getPlayHead()) {
        if (auto pos = ph->getPosition()) {
            if (auto bpm = pos->getBpm()) ti.tempo = *bpm;
            if (auto t = pos->getTimeInSeconds()) ti.position = *t;
            if (auto b = pos->getPpqPosition()) ti.beat = *b;
            if (auto sig = pos->getTimeSignature()) { ti.tsNum = sig->numerator; ti.tsDenom = sig->denominator; }
            ti.playState = pos->getIsRecording() ? 5 : (pos->getIsPlaying() ? 1 : 0);
        }
    }
    const int nOut = getTotalNumOutputChannels(), n = buffer.getNumSamples();
    // script inputs 1/2 = main bus (mono feeds both), 3/4 = sidechain bus (mono feeds both, absent = silence)
    auto mainIn = getBusBuffer(buffer, true, 0);
    const int nMain = mainIn.getNumChannels();
    if ((int)scSilence.size() < n) scSilence.assign((size_t)n, (Real)0);   // only grows when the host's block grows
    const Real* silence = reinterpret_cast<const Real*>(scSilence.data());
    const Real* ins[4] = {nMain > 0 ? mainIn.getReadPointer(0) : silence, nMain > 1 ? mainIn.getReadPointer(1) : (nMain > 0 ? mainIn.getReadPointer(0) : silence),
                          silence, silence};
    if (getBusCount(true) > 1 && getBus(true, 1)->isEnabled()) {
        auto sc = getBusBuffer(buffer, true, 1);
        const int nSc = sc.getNumChannels();
        if (nSc > 0) { ins[2] = sc.getReadPointer(0); ins[3] = sc.getReadPointer(nSc > 1 ? 1 : 0); }
    }
    Real* outs[2] = {buffer.getWritePointer(0), buffer.getWritePointer(nOut > 1 ? 1 : 0)};
    engine.processT<Real>(ins, outs, 4, nOut > 1 ? 2 : 1, n, ti);
    const int lat = engine.latency();
    if (lat != lastLatency) { lastLatency = lat; setLatencySamples(lat); }
}

void BinocularProcessor::processBlock(juce::AudioBuffer<float>& b, juce::MidiBuffer&) { process(b); }
void BinocularProcessor::processBlock(juce::AudioBuffer<double>& b, juce::MidiBuffer&) { process(b); }

// state: magic, slider values (double, exact), the @serialize stream (float32, as REAPER stores it)
void BinocularProcessor::getStateInformation(juce::MemoryBlock& dest) {
    const double t0 = juce::Time::getMillisecondCounterHiRes();
    const juce::ScopeGuard sg { [&] { diag.stateCount++; diag.stateMs = diag.stateMs + (juce::Time::getMillisecondCounterHiRes() - t0); } };
    juce::MemoryOutputStream os(dest, false);
    std::vector<float> ser;
    std::vector<double> sl((size_t)rs::kNumSliders);
    {
        const juce::ScopedLock l(engineLock);
        for (int i = 0; i < rs::kNumSliders; i++) sl[(size_t)i] = engine.getSlider(rs::kSliders[i].idx);
        ser = engine.saveState();
    }
    os.writeInt(0x31565352);   // "RSV1"
    os.writeInt(rs::kNumSliders);
    for (int i = 0; i < rs::kNumSliders; i++) { os.writeInt(rs::kSliders[i].idx); os.writeDouble(sl[(size_t)i]); }
    os.writeInt((int)ser.size());
    for (float f : ser) os.writeFloat(f);
}

void BinocularProcessor::setStateInformation(const void* data, int size) {
    juce::MemoryInputStream is(data, (size_t)size, false);
    if (is.readInt() != 0x31565352) return;
    const int ns = is.readInt();
    std::vector<std::pair<int, double>> sl;
    for (int i = 0; i < ns && !is.isExhausted(); i++) { const int n = is.readInt(); sl.push_back({n, is.readDouble()}); }
    const int nf = is.readInt();
    std::vector<float> ser;
    for (int i = 0; i < nf && !is.isExhausted(); i++) ser.push_back(is.readFloat());
    {
        const juce::ScopedLock l(engineLock);
        for (auto& [n, v] : sl) engine.setSlider(n, v);
        if (!ser.empty()) engine.loadState(ser);   // no @serialize data: nothing to read (running it would zero its variables)
    }
    for (auto& [n, v] : sl) {
        if (n < 1 || n > 256 || !paramBySlider[(size_t)n]) continue;
        auto* p = paramBySlider[(size_t)n];
        lastPushed[n] = p->convertFrom0to1(p->convertTo0to1((float)v));
        p->setValueNotifyingHost(p->convertTo0to1((float)v));
    }
}

juce::AudioProcessorEditor* BinocularProcessor::createEditor() { return new BinocularEditor(*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BinocularProcessor(); }
