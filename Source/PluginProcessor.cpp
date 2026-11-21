#include "PluginProcessor.h"
#include "PluginEditor.h"

const juce::StringArray VoxStudioProcessor::reverbPresetNames {
    "Room", "Hall", "Plate", "Vocal Space", "Shimmer Air"
};

const juce::StringArray VoxStudioProcessor::delayDivisionNames {
    "1/1", "1/2", "1/4", "1/8", "1/16", "1/4 dot", "1/8 dot", "1/4 trip"
};

VoxStudioProcessor::VoxStudioProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout VoxStudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("in_gain", "Input",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("gate_threshold", "Gate Threshold",
        juce::NormalisableRange<float>(-80.0f, 0.0f, 0.1f), -50.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("gate_release", "Gate Release",
        juce::NormalisableRange<float>(5.0f, 500.0f, 0.1f, 0.4f), 80.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("comp_threshold", "Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -18.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("comp_ratio", "Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f), 4.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("comp_attack", "Attack",
        juce::NormalisableRange<float>(0.1f, 200.0f, 0.1f, 0.4f), 10.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("comp_release", "Release",
        juce::NormalisableRange<float>(10.0f, 1500.0f, 0.1f, 0.4f), 120.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("comp_makeup", "Makeup",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f), 0.0f));

    const float defFreq[] = { 120.0f, 500.0f, 2500.0f, 8000.0f };
    for (int b = 0; b < VoxStudioProcessor::numEqBands; ++b)
    {
        const juce::String n = "eq" + juce::String(b + 1) + "_";
        params.push_back(std::make_unique<juce::AudioParameterFloat>(n + "freq", "EQ " + juce::String(b + 1) + " Freq",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), defFreq[b]));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(n + "gain", "EQ " + juce::String(b + 1) + " Gain",
            juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(n + "q", "EQ " + juce::String(b + 1) + " Q",
            juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f), 1.0f));
    }

    params.push_back(std::make_unique<juce::AudioParameterFloat>("rev_room", "Room Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("rev_damp", "Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("rev_mix", "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.25f));
    params.push_back(std::make_unique<juce::AudioParameterInt>("rev_preset", "Preset", 0, 4, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("sfx_drive", "Drive",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("sfx_delay", "Delay Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("sfx_delay_div", "Delay Division",
        delayDivisionNames, 2));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("out_gain", "Output",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f));

    return { params.begin(), params.end() };
}

void VoxStudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    inputGain.prepare(spec);
    compressor.prepare(spec);
    makeupGain.prepare(spec);
    outputGain.prepare(spec);
    reverb.setSampleRate(sampleRate);

    for (auto& band : eqBands)
        band.prepare(spec);

    const int maxDelay = static_cast<int>(sampleRate) + 1;
    delayBuffer.setSize(2, maxDelay);
    delayBuffer.clear();
    delayWritePos = 0;
    gateEnvelope = { 0.0f, 0.0f };

    applyReverbPreset(static_cast<int>(apvts.getRawParameterValue("rev_preset")->load()));
    updateEq();
}

bool VoxStudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}

void VoxStudioProcessor::applyReverbPreset(int index)
{
    switch (index)
    {
        case 0: reverbParams = { 0.30f, 0.60f, 1.0f, 0.0f, 0.8f, 0.0f }; break;
        case 1: reverbParams = { 0.85f, 0.40f, 1.0f, 0.0f, 1.0f, 0.0f }; break;
        case 2: reverbParams = { 0.60f, 0.20f, 1.0f, 0.0f, 1.0f, 0.0f }; break;
        case 3: reverbParams = { 0.45f, 0.70f, 1.0f, 0.0f, 0.6f, 0.0f }; break;
        case 4: reverbParams = { 0.90f, 0.10f, 1.0f, 0.0f, 1.0f, 0.0f }; break;
        default: break;
    }
    reverb.setParameters(reverbParams);
}

void VoxStudioProcessor::updateCompressor()
{
    compressor.setThreshold(apvts.getRawParameterValue("comp_threshold")->load());
    compressor.setRatio(apvts.getRawParameterValue("comp_ratio")->load());
    compressor.setAttack(apvts.getRawParameterValue("comp_attack")->load());
    compressor.setRelease(apvts.getRawParameterValue("comp_release")->load());
}

void VoxStudioProcessor::updateReverb()
{
    applyReverbPreset(static_cast<int>(apvts.getRawParameterValue("rev_preset")->load()));

    reverbParams.roomSize = apvts.getRawParameterValue("rev_room")->load();
    reverbParams.damping  = apvts.getRawParameterValue("rev_damp")->load();
    const float mix = apvts.getRawParameterValue("rev_mix")->load();
    reverbParams.wetLevel = mix;
    reverbParams.dryLevel = 1.0f - mix;
    reverb.setParameters(reverbParams);
}

void VoxStudioProcessor::updateEq()
{
    for (int b = 0; b < numEqBands; ++b)
    {
        const juce::String n = "eq" + juce::String(b + 1) + "_";
        const float freq   = apvts.getRawParameterValue(n + "freq")->load();
        const float gainDb = apvts.getRawParameterValue(n + "gain")->load();
        const float q      = apvts.getRawParameterValue(n + "q")->load();
        const float gain   = juce::Decibels::decibelsToGain(gainDb);

        auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, freq, q, gain);
        *eqBands[(size_t) b].state = *coeffs;
    }
}

void VoxStudioProcessor::processDynamics(juce::AudioBuffer<float>& buffer)
{
    const float threshold = juce::Decibels::decibelsToGain(
        apvts.getRawParameterValue("gate_threshold")->load());
    const float releaseSec = apvts.getRawParameterValue("gate_release")->load() * 0.001f;
    const float releaseCoeff = std::exp(-1.0f / (releaseSec * (float) currentSampleRate));

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        float& env = gateEnvelope[(size_t) juce::jmin(ch, 1)];

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float level = std::abs(data[i]);
            env = level > env ? level : env * releaseCoeff;
            if (env < threshold)
                data[i] = 0.0f;
        }
    }
}

double VoxStudioProcessor::delayTimeMs() const
{
    // Tempo del host; si no hay, 120 BPM
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto hostBpm = pos->getBpm())
                if (*hostBpm > 20.0)
                    bpm = *hostBpm;

    const double quarterMs = 60000.0 / bpm;
    const int idx = static_cast<int>(apvts.getRawParameterValue("sfx_delay_div")->load());

    switch (idx)
    {
        case 0: return quarterMs * 4.0;        // 1/1
        case 1: return quarterMs * 2.0;        // 1/2
        case 2: return quarterMs;              // 1/4
        case 3: return quarterMs * 0.5;        // 1/8
        case 4: return quarterMs * 0.25;       // 1/16
        case 5: return quarterMs * 1.5;        // 1/4 con puntillo
        case 6: return quarterMs * 0.75;       // 1/8 con puntillo
        case 7: return quarterMs * (2.0 / 3.0);// 1/4 de tresillo
        default: return quarterMs;
    }
}

void VoxStudioProcessor::processSFX(juce::AudioBuffer<float>& buffer)
{
    const float drive    = apvts.getRawParameterValue("sfx_drive")->load();
    const float delayMix = apvts.getRawParameterValue("sfx_delay")->load();
    const int   bufLen   = delayBuffer.getNumSamples();

    int delaySamples = static_cast<int>(delayTimeMs() * 0.001 * currentSampleRate);
    delaySamples = juce::jlimit(1, bufLen - 1, delaySamples);

    const int numSamples = buffer.getNumSamples();
    const int numCh = juce::jmin(buffer.getNumChannels(), delayBuffer.getNumChannels());

    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto* dBuf = delayBuffer.getWritePointer(ch);
        int wp = delayWritePos;

        for (int i = 0; i < numSamples; ++i)
        {
            float x = data[i];

            if (drive > 0.0f)
            {
                const float k = 1.0f + drive * 9.0f;
                x = std::tanh(x * k) / std::tanh(k);
            }

            if (delayMix > 0.0f)
            {
                const int readPos = (wp - delaySamples + bufLen) % bufLen;
                const float delayed = dBuf[readPos];
                dBuf[wp] = x;
                x = x * (1.0f - delayMix) + delayed * delayMix;
            }
            else
            {
                dBuf[wp] = x;
            }

            wp = (wp + 1) % bufLen;
            data[i] = x;
        }
    }

    delayWritePos = (delayWritePos + numSamples) % bufLen;
}

void VoxStudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    inputLevel.store(buffer.getMagnitude(0, buffer.getNumSamples()));

    if (apvts.getRawParameterValue("bypass")->load() > 0.5f)
    {
        outputLevel.store(buffer.getMagnitude(0, buffer.getNumSamples()));
        gainReductionDb.store(0.0f);
        return;
    }

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);

    inputGain.setGainDecibels(apvts.getRawParameterValue("in_gain")->load());
    inputGain.process(ctx);

    processDynamics(buffer);

    updateCompressor();
    compressor.process(ctx);

    // Reducción de ganancia aproximada para la aguja del VU
    {
        const float peakIn  = juce::jmax(0.0001f, buffer.getMagnitude(0, buffer.getNumSamples()));
        const float thrLin  = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("comp_threshold")->load());
        const float ratio   = apvts.getRawParameterValue("comp_ratio")->load();
        const float levelDb = juce::Decibels::gainToDecibels(peakIn);
        const float thrDb   = juce::Decibels::gainToDecibels(thrLin);
        const float over    = juce::jmax(0.0f, levelDb - thrDb);
        const float reduction = over - over / ratio;
        gainReductionDb.store(reduction);
    }

    makeupGain.setGainDecibels(apvts.getRawParameterValue("comp_makeup")->load());
    makeupGain.process(ctx);

    updateEq();
    for (int b = 0; b < numEqBands; ++b)
        eqBands[(size_t) b].process(ctx);

    processSFX(buffer);

    updateReverb();
    if (buffer.getNumChannels() >= 2)
        reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), buffer.getNumSamples());

    outputGain.setGainDecibels(apvts.getRawParameterValue("out_gain")->load());
    outputGain.process(ctx);

    outputLevel.store(buffer.getMagnitude(0, buffer.getNumSamples()));
}

juce::AudioProcessorEditor* VoxStudioProcessor::createEditor()
{
    return new VoxStudioEditor(*this);
}

void VoxStudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void VoxStudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoxStudioProcessor();
}
