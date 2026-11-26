#pragma once
#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include "engine.h"

class BinocularProcessor : public juce::AudioProcessor {
public:
    // the editor window size, remembered with the project and for the next time the window opens
    // (a new Binocular starts with the size last used in this session)
    static inline std::atomic<int> sessionW { 680 }, sessionH { 520 };
    std::atomic<int> editorW { sessionW.load() }, editorH { sessionH.load() };
    BinocularProcessor();
    ~BinocularProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock(juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    bool supportsDoublePrecisionProcessing() const override { return true; }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    rs::Engine engine;

    // diagnostics (binocular-debug.txt on the desktop): time spent in host notifications and state calls
    struct Diag { std::atomic<int> notifyCount { 0 }, stateCount { 0 }; std::atomic<double> notifyMs { 0 }, notifyMaxMs { 0 }, stateMs { 0 }; };
    Diag diag;
    juce::CriticalSection engineLock;            // @serialize vs audio (the @gfx thread runs unlocked, as in REAPER)

private:
    template <class Real> void process(juce::AudioBuffer<Real>& buffer);
    void syncParamsToEngine();
    void scriptAutomate(int idx, int mode);
    void scriptChanged(int idx);

    std::vector<juce::RangedAudioParameter*> paramBySlider;   // index = JSFX slider number
    std::unique_ptr<std::atomic<float>[]> lastPushed;
    std::vector<char> touching;
    int lastLatency = -1;
    std::vector<double> scSilence;               // zeros for absent inputs (sized as double: covers float and double)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BinocularProcessor)
};
