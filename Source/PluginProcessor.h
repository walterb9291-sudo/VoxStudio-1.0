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

    float getInputLevel() const { return inputLevel.load(); }
    float getOutputLevel() const { return outputLevel.load(); }
    float getGainReduction() const { return gainReductionDb.load(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void updateCompressor();
    void updateReverb();
    void updateEq();
    void processDynamics(juce::AudioBuffer<float>& buffer);
    void processSFX(juce::AudioBuffer<float>& buffer);
    double delayTimeMs() const;

    double currentSampleRate = 44100.0;
    juce::dsp::ProcessSpec spec {};

    juce::dsp::Compressor<float> compressor;
    juce::dsp::Gain<float> inputGain;
    juce::dsp::Gain<float> makeupGain;
    juce::dsp::Gain<float> outputGain;
    juce::Reverb reverb;
    juce::Reverb::Parameters reverbParams;

    using Filter = juce::dsp::IIR::Filter<float>;
    using FilterBand = juce::dsp::ProcessorDuplicator<Filter, juce::dsp::IIR::Coefficients<float>>;
    std::array<FilterBand, numEqBands> eqBands;

    std::array<float, 2> gateEnvelope { 0.0f, 0.0f };

    juce::AudioBuffer<float> delayBuffer;
    int delayWritePos = 0;

    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };
    std::atomic<float> gainReductionDb { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoxStudioProcessor)
};
