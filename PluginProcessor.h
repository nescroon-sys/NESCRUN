#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Params.h"
#include "Presets.h"
#include "DSP/PitchCorrector.h"
#include "DSP/ResonanceSuppressor.h"
#include "DSP/Dynamics.h"
#include "DSP/ToneFX.h"
#include "DSP/SpaceFX.h"

class NescrunProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    NescrunProcessor();
    ~NescrunProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "NESCRUN"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- API para la interfaz
    juce::AudioProcessorValueTreeState apvts;
    void startAutoMix();
    std::atomic<int>   autoMixState { 0 };   // 0 inactivo, 1 escuchando, 2 listo
    std::atomic<float> autoMixProgress { 0.f };
    std::atomic<float> inLevel { -100.f }, outLevel { -100.f }, compGr { 0.f }, deessGr { 0.f }, limGr { 0.f }, resoGr { 0.f };
    nescrun::PitchCorrector& getTuner() { return tuner; }
    void applyPresetValues (const std::vector<std::pair<const char*, float>>& values);
    juce::String userPresetName;

private:
    void timerCallback() override;
    float p (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    nescrun::PitchCorrector tuner;
    nescrun::ResonanceSuppressor reso;
    nescrun::Gate gate;
    nescrun::DeEsser deesser;
    nescrun::Compressor comp, leveler;
    nescrun::ToneEQ eq;
    nescrun::Silk silk;
    nescrun::Saturator sat;
    nescrun::Boom boom;
    nescrun::Modulation mod;
    nescrun::SyncDelay delay;
    nescrun::VocalReverb reverb;
    nescrun::Limiter limiter;
    nescrun::DelayLine dryL, dryR;

    std::vector<float> mono, revL, revR;
    int totalLatency = 0, currentPreset = 0;
    double sr = 44100.0;
    juce::LinearSmoothedValue<float> inGainS, outGainS, mixS, delayMixS, revMixS, widthS;

    // ---- Auto Mix (asistente de mezcla)
    struct Band { juce::dsp::IIR::Filter<float> f; double sum = 0.0; };
    std::array<Band, 6> amBands;
    double amSumSq = 0.0, amPeak = 0.0; long amActive = 0; long amTarget = 0;
    std::vector<float> amBlockDb;
    struct AutoResult { float inGain, gate, deess, comp, level, body, mud, mid, pres, air; };
    AutoResult amResult {};
    void analyseAutoMix (const float* x, int n);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NescrunProcessor)
};
