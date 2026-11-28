#pragma once

#include <JuceHeader.h>

class VoxStudioProcessor : public juce::AudioProcessor
{
public:
    VoxStudioProcessor();
    ~VoxStudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VoxStudio 1.0"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    void applyReverbPreset(int index);
    static const juce::StringArray reverbPresetNames;
    static const juce::StringArray delayDivisionNames;
    static constexpr int numEqBands = 4;
    static constexpr int numSpecBands = 64;

    float getInputLevel() const { return inputLevel.load(); }
    float getOutputLevel() const { return outputLevel.load(); }
    float getGainReduction() const { return gainReductionDb.load(); }
    bool getGateOpen() const { return gateOpen.load(); }
    float getSpectrumBand(int i) const { return specBands[(size_t) i].load(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void updateCompressor();
    void updateReverb();
    void updateEq();
    void updateDeEsser();
    void updateDynFilters();
    void processDynamics(juce::AudioBuffer<float>& buffer);
    void processDeEsser(juce::AudioBuffer<float>& buffer);
    void processDynModule(juce::AudioBuffer<float>& buffer);
    void processSFX(juce::AudioBuffer<float>& buffer);
    void processPreDelay(juce::AudioBuffer<float>& buffer);
    void processReverbWithPreDelay(juce::AudioBuffer<float>& buffer);
    void runFdnReverb(juce::AudioBuffer<float>& buffer);
    void pushSpectrum(const juce::AudioBuffer<float>& buffer);
    double delayTimeMs() const;

    double currentSampleRate = 44100.0;
    juce::dsp::ProcessSpec spec {};

    juce::dsp::Compressor<float> compressor;
    juce::dsp::Gain<float> inputGain;
    juce::dsp::Gain<float> makeupGain;
    juce::dsp::Gain<float> outputGain;

    using Filter = juce::dsp::IIR::Filter<float>;
    using FilterBand = juce::dsp::ProcessorDuplicator<Filter, juce::dsp::IIR::Coefficients<float>>;
    std::array<FilterBand, numEqBands> eqBands;
    FilterBand deEsserBand;

    // Módulo DYNAMICS: color (shelf), paso alto y paso bajo
    FilterBand dynColorBand;
    FilterBand dynHpBand;
    FilterBand dynLpBand;
    juce::AudioBuffer<float> dynDryBuffer;

    float gateEnv = 0.0f;
    float gateGainSm = 0.0f;
    bool gateOpenState = false;

    float deEsserEnv = 0.0f;
    juce::AudioBuffer<float> sideBuffer;

    juce::AudioBuffer<float> delayBuffer;
    int delayWritePos = 0;

    // Pre-delay del reverb (hasta 200 ms + modulación) y copia de trabajo para la cola
    juce::AudioBuffer<float> preDelayBuffer;
    int preDelayWritePos = 0;
    juce::AudioBuffer<float> revSendBuffer;
    double lfoPhase = 0.0;

    // Reverb propio: red de 8 líneas de retardo con mezcla Hadamard
    static constexpr int kFdnLines = 8;
    std::array<std::vector<float>, kFdnLines> fdnBuf;
    std::array<int, kFdnLines> fdnWrite {};
    std::array<float, kFdnLines> fdnDamp {};
    std::array<float, 2> reverbHpState { 0.0f, 0.0f };
    double lfoPhaseRev = 0.0;
    int reverbPresetIdx = 0;

    // Analizador de espectro (FFT de 2048 muestras)
    juce::dsp::FFT fft { 11 };
    juce::dsp::WindowingFunction<float> fftWindow { 2048, juce::dsp::WindowingFunction<float>::hann };
    std::array<float, 4096> fftBuffer {};
    int fftFill = 0;
    std::array<std::atomic<float>, numSpecBands> specBands;

    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };
    std::atomic<float> gainReductionDb { 0.0f };
    std::atomic<bool> gateOpen { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoxStudioProcessor)
};
