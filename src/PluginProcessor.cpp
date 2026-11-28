#include "PluginProcessor.h"
#include "PluginEditor.h"

BinocularProcessor::BinocularProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
    for (const auto& info : bn::kParams) {
        juce::NormalisableRange<float> range((float)info.lo, (float)info.hi, (float)info.step);
        auto attr = juce::AudioParameterFloatAttributes();
        if (info.choices != nullptr) {
            const juce::StringArray items = juce::StringArray::fromTokens(juce::String::fromUTF8(info.choices), "|", "");
            const double lo = info.lo;
            attr = attr.withStringFromValueFunction([items, lo](float v, int) {
                const int i = (int)std::floor(v - lo + 0.5);
                return juce::isPositiveAndBelow(i, items.size()) ? items[i] : juce::String(v);
            });
        }
        auto* p = new juce::AudioParameterFloat(juce::ParameterID("s" + juce::String(info.number), 1),
                                                juce::String::fromUTF8(info.name), range, (float)info.def, attr);
        addParameter(p);
        params[info.number] = p;
    }
}

double BinocularProcessor::get(int number) const { return (double)params[number]->get(); }

void BinocularProcessor::set(int number, double value) {
    auto* p = params[number];
    p->beginChangeGesture();
    p->setValueNotifyingHost(p->convertTo0to1((float)value));
    p->endChangeGesture();
}

bn::Settings BinocularProcessor::settings() const {
    bn::Settings s;
    s.bandsIndex = (int)std::floor(get(bn::pBands) + 0.5);
    s.selectivity = get(bn::pSelectivity);
    s.averagingMs = get(bn::pAveraging);
    s.hold = (int)std::floor(get(bn::pHold) + 0.5);
    s.frozen = get(bn::pFreeze) >= 0.5;
    s.measure = (int)std::floor(get(bn::pMeasure) + 0.5);
    s.span = (int)std::floor(get(bn::pSpan) + 0.5);
    s.pair = (int)std::floor(get(bn::pPair) + 0.5);
    // the downmix the correlation pages compare in 5.1: ITU-R BS.775 (centre and surrounds -3 dB, no LFE),
    // Modern (as Pro Tools' Modern fold-down: surrounds -3 dB, LFE -10 dB; centre -3 dB) or manual
    const int dm = (int)std::floor(get(bn::pDownmix) + 0.5);
    if (dm == 0) { s.dmxCentre = s.dmxSurround = 0.70710678; s.dmxLfe = 0; }
    else if (dm == 1) { s.dmxCentre = s.dmxSurround = 0.70710678; s.dmxLfe = 0.31622777; }
    else {
        s.dmxCentre = std::pow(10.0, get(bn::pDmxCentre) / 20);
        s.dmxSurround = std::pow(10.0, get(bn::pDmxSurround) / 20);
        s.dmxLfe = get(bn::pDmxLfe) <= -39.9 ? 0.0 : std::pow(10.0, get(bn::pDmxLfe) / 20);
    }
    s.surround = surround.load();
    return s;
}

// a 5.1 set: L, R, C, LFE and two surrounds (rear or side)
static bool isFivePointOne(const juce::AudioChannelSet& s) {
    using T = juce::AudioChannelSet;
    return s.size() == 6 && s.getChannelIndexForType(T::left) >= 0 && s.getChannelIndexForType(T::right) >= 0
        && s.getChannelIndexForType(T::centre) >= 0 && s.getChannelIndexForType(T::LFE) >= 0
        && (s.getChannelIndexForType(T::leftSurround) >= 0 || s.getChannelIndexForType(T::leftSurroundSide) >= 0)
        && (s.getChannelIndexForType(T::rightSurround) >= 0 || s.getChannelIndexForType(T::rightSurroundSide) >= 0);
}

// stereo (or mono in, stereo out) and 5.1 (in = out): the host picks what fits the track, Binocular follows
bool BinocularProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    const auto mono = juce::AudioChannelSet::mono(), stereo = juce::AudioChannelSet::stereo();
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    if (isFivePointOne(out)) { if (in != out) return false; }
    else if (out != stereo || (in != stereo && in != mono)) return false;
    return true;
}

void BinocularProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    surround = isFivePointOne(getChannelLayoutOfBus(true, 0));
    analyzer.prepare(sampleRate, settings());
    silence.assign((size_t)juce::jmax(1, samplesPerBlock) * 2, 0.0);
}

template <class Real>
void BinocularProcessor::process(juce::AudioBuffer<Real>& buffer) {
    bn::Transport t;
    if (auto* ph = getPlayHead()) {
        if (auto pos = ph->getPosition()) {
            if (auto secs = pos->getTimeInSeconds()) t.position = *secs;
            t.playing = pos->getIsPlaying() || pos->getIsRecording();
        }
    }
    const int n = buffer.getNumSamples();
    if ((int)silence.size() < n * 2) silence.assign((size_t)n * 2, 0.0);   // only grows when the host's block grows
    const Real* zeros = reinterpret_cast<const Real*>(silence.data());
    // main bus: stereo (mono feeds both sides) or 5.1 in the order L, R, C, LFE, Ls, Rs whatever order the host uses
    auto mainIn = getBusBuffer(buffer, true, 0);
    const int nMain = mainIn.getNumChannels();
    const auto layout = getChannelLayoutOfBus(true, 0);
    const bool isSurround = isFivePointOne(layout) && nMain == 6;
    surround = isSurround;
    const Real* ch[6] = { zeros, zeros, zeros, zeros, zeros, zeros };
    if (isSurround) {
        using T = juce::AudioChannelSet;
        const T::ChannelType types[6][2] = { { T::left, T::left }, { T::right, T::right }, { T::centre, T::centre }, { T::LFE, T::LFE },
                                             { T::leftSurround, T::leftSurroundSide }, { T::rightSurround, T::rightSurroundSide } };
        for (int c = 0; c < 6; c++) {
            int idx = layout.getChannelIndexForType(types[c][0]);
            if (idx < 0) idx = layout.getChannelIndexForType(types[c][1]);
            if (idx >= 0 && idx < nMain) ch[c] = mainIn.getReadPointer(idx);
        }
    } else {
        ch[0] = nMain > 0 ? mainIn.getReadPointer(0) : zeros;
        ch[1] = nMain > 1 ? mainIn.getReadPointer(1) : ch[0];
    }
    analyzer.process(ch, n, settings(), t);
    // the audio passes untouched: a mono input goes out on both sides
    if (nMain == 1 && getTotalNumOutputChannels() > 1) buffer.copyFrom(1, 0, buffer, 0, 0, n);
}

void BinocularProcessor::processBlock(juce::AudioBuffer<float>& b, juce::MidiBuffer&) { process(b); }
void BinocularProcessor::processBlock(juce::AudioBuffer<double>& b, juce::MidiBuffer&) { process(b); }

// state: "RSV1", the parameter values (double), an empty block (earlier versions kept script data there),
// "SIZE" and the editor window size. Earlier versions write the same, so old projects load.
void BinocularProcessor::getStateInformation(juce::MemoryBlock& dest) {
    juce::MemoryOutputStream os(dest, false);
    os.writeInt(0x31565352);   // "RSV1"
    os.writeInt(bn::kNumParams);
    for (const auto& info : bn::kParams) { os.writeInt(info.number); os.writeDouble(get(info.number)); }
    os.writeInt(0);
    os.writeInt(0x455A4953);   // "SIZE"
    os.writeInt(editorW.load());
    os.writeInt(editorH.load());
}

void BinocularProcessor::setStateInformation(const void* data, int size) {
    juce::MemoryInputStream is(data, (size_t)size, false);
    if (is.readInt() != 0x31565352) return;
    const int count = is.readInt();
    for (int i = 0; i < count && !is.isExhausted(); i++) {
        const int number = is.readInt();
        const double value = is.readDouble();
        if (number >= 1 && number < kMaxParam && params[number] != nullptr)   // values of removed parameters are skipped
            params[number]->setValueNotifyingHost(params[number]->convertTo0to1((float)value));
    }
    const int skip = is.readInt();                // script data of earlier versions: not needed
    for (int i = 0; i < skip && !is.isExhausted(); i++) is.readFloat();
    if (!is.isExhausted() && is.readInt() == 0x455A4953) {
        const int w = is.readInt(), h = is.readInt();
        if (w >= 340 && w <= 4000 && h >= 260 && h <= 3000) {
            editorW = w;
            editorH = h;
            if (auto* ed = getActiveEditor()) {   // a project loaded while the window is open
                juce::Component::SafePointer<juce::AudioProcessorEditor> sp(ed);
                juce::MessageManager::callAsync([sp, w, h] { if (sp != nullptr) sp->setSize(w, h); });
            }
        }
    }
}

juce::AudioProcessorEditor* BinocularProcessor::createEditor() { return new BinocularEditor(*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BinocularProcessor(); }
