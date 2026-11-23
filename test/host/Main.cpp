// headless test host for the Binocular VST3: null test through the binary, state round trip, editor snapshot
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <fstream>
#include "../common.h"
using namespace juce;

static std::vector<double> readf(const String& p) { MemoryBlock mb; File(p).loadFileAsData(mb); std::vector<double> d(mb.getSize() / 8); memcpy(d.data(), mb.getData(), d.size() * 8); return d; }
static void writef(const String& p, const std::vector<double>& d) { File(p).replaceWithData(d.data(), d.size() * 8); }

static std::unique_ptr<AudioPluginInstance> load(const String& path, double sr, int bs) {
    VST3PluginFormat fmt; OwnedArray<PluginDescription> types;
    fmt.findAllTypesForFile(types, path);
    if (types.isEmpty()) { std::cerr << "no plugin types\n"; return nullptr; }
    String err;
    auto inst = fmt.createInstanceFromDescription(*types[0], sr, bs, err);
    if (!inst) std::cerr << "create failed: " << err << "\n";
    return inst;
}

// state blob with exact double slider values (as RoomServiceProcessor::getStateInformation writes it)
static MemoryBlock stateFor(const var& meta, const String& caseFile) {
    std::map<int, double> v;
    for (auto& s : *meta["sliders"].getArray()) v[(int)s["idx"]] = (double)s["default"];
    StringArray lines; lines.addLines(File(caseFile).loadFileAsString());
    for (auto& l : lines) { auto t = StringArray::fromTokens(l, " ", ""); if (t.size() == 2) v[t[0].getIntValue()] = t[1].getDoubleValue(); }
    MemoryBlock mb; MemoryOutputStream os(mb, false);
    os.writeInt(0x31565352); os.writeInt((int)v.size());
    for (auto& [n, x] : v) { os.writeInt(n); os.writeDouble(x); }
    os.writeInt(0);
    return mb;
}

struct TestPlayHead : AudioPlayHead {
    double t = 0;
    Optional<PositionInfo> getPosition() const override {
        PositionInfo pi; pi.setBpm(120.0); pi.setTimeInSeconds(t); pi.setTimeInSamples((int64)std::llround(t * 48000.0)); pi.setPpqPosition(t * 2); pi.setIsPlaying(true);
        pi.setTimeSignature(TimeSignature{4, 4}); return pi;
    }
};
static TestPlayHead g_ph;
static std::vector<double> render(AudioPluginInstance& p, const std::vector<double>& in, int bs) {
    p.setPlayHead(&g_ph);
    const size_t frames = in.size() / 2; std::vector<double> out(in.size());
    AudioBuffer<double> buf(2, bs); MidiBuffer midi;
    for (size_t pos = 0; pos < frames; pos += (size_t)bs) {
        const int nf = (int)std::min<size_t>((size_t)bs, frames - pos);
        buf.setSize(2, nf, false, false, true);
        for (int i = 0; i < nf; i++) { buf.setSample(0, i, in[2 * (pos + i)]); buf.setSample(1, i, in[2 * (pos + i) + 1]); }
        g_ph.t = (double)pos / 48000.0;
        p.processBlock(buf, midi);
        for (int i = 0; i < nf; i++) { out[2 * (pos + i)] = buf.getSample(0, i); out[2 * (pos + i) + 1] = buf.getSample(1, i); }
    }
    return out;
}

int main(int argc, char** argv) {
    ScopedJuceInitialiser_GUI gui;
    StringArray a; for (int i = 1; i < argc; i++) a.add(argv[i]);
    const String plug = a[0], mode = a[1];
    const double sr = 48000; const int bs = 256;
    auto p = load(plug, sr, bs);
    if (!p) return 1;
    p->setPlayConfigDetails(2, 2, sr, bs);
    if (!p->supportsDoublePrecisionProcessing()) { std::cerr << "no double precision\n"; return 1; }
    p->setProcessingPrecision(AudioProcessor::doublePrecision);
    std::cout << "loaded: " << p->getName() << ", params " << p->getParameters().size() << ", latency " << p->getLatencySamples() << "\n";
    if (mode == "null") {   // null <plug> null meta.json case.txt in.f64 out.f64
        var meta = JSON::parse(File(a[2]));
        auto st = stateFor(meta, a[3]);
        XmlElement x("VST3PluginState");
        x.createNewChildElement("IComponent")->addTextElement(st.toBase64Encoding());
        MemoryBlock wrapped; AudioProcessor::copyXmlToBinary(x, wrapped);
        p->setStateInformation(wrapped.getData(), (int)wrapped.getSize());
        p->prepareToPlay(sr, bs);
        writef(a[5], render(*p, readf(a[4]), bs));
        p->releaseResources();
    } else if (mode == "state") {   // state round trip: get -> new instance set -> get, bytes equal
        p->prepareToPlay(sr, bs);
        std::vector<double> in(2 * 48000, 0.0); in[200] = 1.0;
        render(*p, in, bs);
        MemoryBlock s1; p->getStateInformation(s1);
        auto q = load(plug, sr, bs); q->setPlayConfigDetails(2, 2, sr, bs);
        q->setStateInformation(s1.getData(), (int)s1.getSize());
        MemoryBlock s2; q->getStateInformation(s2);
        std::cout << "state bytes " << s1.getSize() << " / " << s2.getSize() << (s1 == s2 ? "  IDENTISCH" : "  VERSCHIEDEN") << "\n";
    } else if (mode == "gui") {   // gui <plug> gui out.png [w h] [scale]
        p->prepareToPlay(sr, bs);
        // the test signal: noise, right side 12 samples late (as in the previews)
        const int kind = a[6].isNotEmpty() ? a[6].getIntValue() : 0;
        auto s4 = makeSignal(kind, sr, 48000 * 3); std::vector<double> in(2 * 48000 * 3);
        for (size_t i = 0; i < in.size() / 2; i++) { in[2 * i] = s4[4 * i]; in[2 * i + 1] = s4[4 * i + 1]; }
        if (a[7].isNotEmpty()) { auto* pr = dynamic_cast<RangedAudioParameter*>(p->getParameters()[a[7].getIntValue()]); if (pr) pr->setValueNotifyingHost(pr->convertTo0to1((float)a[8].getDoubleValue())); else p->getParameters()[a[7].getIntValue()]->setValueNotifyingHost((float)a[8].getDoubleValue()); }
        render(*p, in, bs);
        std::unique_ptr<AudioProcessorEditor> ed(p->createEditorIfNeeded());
        if (!ed) { std::cerr << "no editor\n"; return 1; }
        const int w = a[3].isNotEmpty() ? a[3].getIntValue() : ed->getWidth(), h = a[4].isNotEmpty() ? a[4].getIntValue() : ed->getHeight();
        ed->setSize(w, h);
        ed->addToDesktop(0); ed->setVisible(true);
        for (int i = 0; i < 40; i++) { MessageManager::getInstance()->runDispatchLoopUntil(30); std::vector<double> tail(in.end() - 2 * bs * 8, in.end()); render(*p, tail, bs); }
        if (a[5].isNotEmpty()) {   // keep the window on screen for N seconds (external screenshots / input)
            const double until = Time::getMillisecondCounterHiRes() + a[5].getDoubleValue() * 1000.0;
            std::vector<double> blk(2 * bs); double ph = 0;
            while (Time::getMillisecondCounterHiRes() < until) {
                MessageManager::getInstance()->runDispatchLoopUntil(5);
                for (int k = 0; k < 2; k++) {   // ~real time: 2 blocks of 256 per ~10 ms
                    for (int i = 0; i < bs; i++) { const size_t j = (size_t)std::fmod(ph, (double)(in.size() / 2)); blk[2 * i] = in[2 * j]; blk[2 * i + 1] = in[2 * j + 1]; ph += 1; }
                    render(*p, blk, bs);
                }
            }
        }
        Image img = ed->createComponentSnapshot(ed->getLocalBounds(), true, 1.0f);
        File out(a[2]); out.deleteFile(); FileOutputStream fos(out); PNGImageFormat().writeImageToStream(img, fos);
        std::cout << "editor " << ed->getWidth() << "x" << ed->getHeight() << " -> " << a[2] << "\n";
        ed = nullptr;
    }
    p = nullptr;
    return 0;
}
