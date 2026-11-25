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

    params.push_back(std::make_unique<juce::AudioParameterFloat>("dyn_gate", "Gate",
        juce::NormalisableRange<float>(-80.0f, 0.0f, 0.1f), -50.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dyn_deess", "De-Ess",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dyn_focus", "Focus",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

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

    params.push_back(std::make_unique<juce::AudioParameterFloat>("sfx_delay", "Delay Wet",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.25f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("sfx_delay_dry", "Delay Dry",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("sfx_delay_fb", "Delay Feedback",
        juce::NormalisableRange<float>(0.0f, 0.9f, 0.001f), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("sfx_delay_drv", "Delay Drive",
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
    deEsserBand.prepare(spec);

    sideBuffer.setSize(2, samplesPerBlock);
    sideBuffer.clear();
    deEsserEnv = 0.0f;

    const int maxDelay = static_cast<int>(sampleRate * 4.0) + 1;
    delayBuffer.setSize(2, maxDelay);
    delayBuffer.clear();
    delayWritePos = 0;

    gateEnv = 0.0f;
    gateGainSm = 0.0f;
    gateOpenState = false;

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

void VoxStudioProcessor::updateDeEsser()
{
    const float focus = apvts.getRawParameterValue("dyn_focus")->load();
    const float freq = 2000.0f * std::pow(5.0f, focus);

    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(currentSampleRate, freq, 2.0f);
    *deEsserBand.state = *coeffs;
}

void VoxStudioProcessor::processDynamics(juce::AudioBuffer<float>& buffer)
{
    const float thrDb    = apvts.getRawParameterValue("dyn_gate")->load();
    const float openLin  = juce::Decibels::decibelsToGain(thrDb);
    const float closeLin = juce::Decibels::decibelsToGain(thrDb - 6.0f);
    const float attackC  = std::exp(-1.0f / (0.003f * (float) currentSampleRate));
    const float releaseC = std::exp(-1.0f / (0.150f * (float) currentSampleRate));
    const float envC     = std::exp(-1.0f / (0.010f * (float) currentSampleRate));

    const int n = buffer.getNumSamples();
    const int nCh = juce::jmin(buffer.getNumChannels(), 2);
    if (nCh == 0 || n == 0)
        return;

    float* L = buffer.getWritePointer(0);
    float* R = (nCh > 1) ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < n; ++i)
    {
        float level = std::abs(L[i]);
        if (R != nullptr)
            level = juce::jmax(level, std::abs(R[i]));

        gateEnv = level > gateEnv ? level : gateEnv * envC;

        if (! gateOpenState && gateEnv > openLin)
            gateOpenState = true;
        else if (gateOpenState && gateEnv < closeLin)
            gateOpenState = false;

        const float target = gateOpenState ? 1.0f : 0.0f;
        const float coef   = gateOpenState ? attackC : releaseC;
        gateGainSm = target + (gateGainSm - target) * coef;

        L[i] *= gateGainSm;
        if (R != nullptr)
            R[i] *= gateGainSm;
    }

    gateOpen.store(gateOpenState);
}

void VoxStudioProcessor::processDeEsser(juce::AudioBuffer<float>& buffer)
{
    const float amount = apvts.getRawParameterValue("dyn_deess")->load();
    const int n = buffer.getNumSamples();
    if (amount <= 0.001f || n == 0 || buffer.getNumChannels() < 2)
        return;

    if (sideBuffer.getNumSamples() != n)
        sideBuffer.setSize(2, n, false, false, true);

    sideBuffer.copyFrom(0, 0, buffer, 0, 0, n);
    sideBuffer.copyFrom(1, 0, buffer, 1, 0, n);

    updateDeEsser();
    juce::dsp::AudioBlock<float> sideBlock(sideBuffer);
    juce::dsp::ProcessContextReplacing<float> sideCtx(sideBlock);
    deEsserBand.process(sideCtx);

    const float attackC  = std::exp(-1.0f / (0.001f * (float) currentSampleRate));
    const float releaseC = std::exp(-1.0f / (0.080f * (float) currentSampleRate));
    const float thrDb    = -30.0f;
    const float maxRedDb = amount * 15.0f;

    float* d0 = buffer.getWritePointer(0);
    float* d1 = buffer.getWritePointer(1);
    const float* b0 = sideBuffer.getReadPointer(0);
    const float* b1 = sideBuffer.getReadPointer(1);

    for (int i = 0; i < n; ++i)
    {
        const float level = juce::jmax(std::abs(b0[i]), std::abs(b1[i]));
        deEsserEnv = level > deEsserEnv
                       ? attackC * deEsserEnv + (1.0f - attackC) * level
                       : releaseC * deEsserEnv + (1.0f - releaseC) * level;

        const float envDb = juce::Decibels::gainToDecibels(deEsserEnv, -100.0f);
        const float reduceDb = juce::jmin(juce::jmax(0.0f, envDb - thrDb) * 0.7f, maxRedDb);
        const float g = juce::Decibels::decibelsToGain(-reduceDb);

        d0[i] -= b0[i] * (1.0f - g);
        d1[i] -= b1[i] * (1.0f - g);
    }
}

double VoxStudioProcessor::delayTimeMs() const
{
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
        case 0: return quarterMs * 4.0;
        case 1: return quarterMs * 2.0;
        case 2: return quarterMs;
        case 3: return quarterMs * 0.5;
        case 4: return quarterMs * 0.25;
        case 5: return quarterMs * 1.5;
        case 6: return quarterMs * 0.75;
        case 7: return quarterMs * (2.0 / 3.0);
        default: return quarterMs;
    }
}

void VoxStudioProcessor::processSFX(juce::AudioBuffer<float>& buffer)
{
    const float drive    = apvts.getRawParameterValue("sfx_drive")->load();
    const float wet      = apvts.getRawParameterValue("sfx_delay")->load();
    const float dry      = apvts.getRawParameterValue("sfx_delay_dry")->load();
    const float fb       = apvts.getRawParameterValue("sfx_delay_fb")->load();
    const float dlyDrive = apvts.getRawParameterValue("sfx_delay_drv")->load();

    const int bufLen = delayBuffer.getNumSamples();
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

            const int rp = (wp - delaySamples + bufLen) % bufLen;
            float echo = dBuf[rp];

            if (dlyDrive > 0.0f)
                echo = std::tanh(echo * (1.0f + dlyDrive * 4.0f));

            dBuf[wp] = x + echo * fb;
            data[i] = x * dry + echo * wet;

            wp = (wp + 1) % bufLen;
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
        gateOpen.store(false);
        return;
    }

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);

    inputGain.setGainDecibels(apvts.getRawParameterValue("in_gain")->load());
    inputGain.process(ctx);

    processDynamics(buffer);
    processDeEsser(buffer);

    updateCompressor();
    compressor.process(ctx);

    {
        const float peakIn    = juce::jmax(0.0001f, buffer.getMagnitude(0, buffer.getNumSamples()));
        const float thrLin    = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("comp_threshold")->load());
        const float ratio     = apvts.getRawParameterValue("comp_ratio")->load();
        const float levelDb   = juce::Decibels::gainToDecibels(peakIn);
        const float thrDb     = juce::Decibels::gainToDecibels(thrLin);
        const float over      = juce::jmax(0.0f, levelDb - thrDb);
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
