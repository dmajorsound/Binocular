#pragma once
#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Analyzer.h"
#include "Params.h"

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

    // a parameter by its number (bn::Param): the value, and a change from the editor (one undo step in the host)
    double get(int number) const;
    void set(int number, double value);
    juce::AudioParameterFloat* param(int number) const { return params[number]; }

    bn::Analyzer analyzer;

private:
    template <class Real> void process(juce::AudioBuffer<Real>& buffer);
    bn::Settings settings() const;

    juce::AudioParameterFloat* params[bn::kNumParams + 1] {};
    std::vector<double> silence;               // zeros for an absent sidechain
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BinocularProcessor)
};
