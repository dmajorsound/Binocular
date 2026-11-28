#include "PluginProcessor.h"
#include "PluginEditor.h"

BinocularProcessor::BinocularProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
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
    s.sidechain = get(bn::pInput) >= 0.5;
    s.hold = (int)std::floor(get(bn::pHold) + 0.5);
    s.frozen = get(bn::pFreeze) >= 0.5;
    s.measure = (int)std::floor(get(bn::pMeasure) + 0.5);
    s.span = (int)std::floor(get(bn::pSpan) + 0.5);
    return s;
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
    // main bus (mono feeds both sides), sidechain bus (mono feeds both, absent = silence)
    auto mainIn = getBusBuffer(buffer, true, 0);
    const int nMain = mainIn.getNumChannels();
    const Real* l = nMain > 0 ? mainIn.getReadPointer(0) : zeros;
    const Real* r = nMain > 1 ? mainIn.getReadPointer(1) : l;
    const Real* sl = zeros;
    const Real* sr = zeros;
    if (getBusCount(true) > 1 && getBus(true, 1)->isEnabled()) {
        auto sc = getBusBuffer(buffer, true, 1);
        if (sc.getNumChannels() > 0) { sl = sc.getReadPointer(0); sr = sc.getReadPointer(sc.getNumChannels() > 1 ? 1 : 0); }
    }
    analyzer.process(l, r, sl, sr, n, settings(), t);
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
        if (number >= 1 && number <= bn::kNumParams) params[number]->setValueNotifyingHost(params[number]->convertTo0to1((float)value));
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
