#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>

namespace
{
    // Retardos base de las 8 líneas del reverb (ms), en proporciones no múltiplas entre sí
    const float kBaseMs[8] = { 29.7f, 37.1f, 41.1f, 43.7f, 47.9f, 53.3f, 59.9f, 67.7f };

    // Ajustes de carácter por tipo de reverb: tamaño de la red y brillo
    const float kPresetScale[5]      = { 1.0f, 1.4f, 0.7f, 0.9f, 1.2f };
    const float kPresetDampOffset[5] = { 0.0f, 0.05f, -0.1f, 0.05f, -0.2f };

    // Transformada Hadamard de 8 puntos, normalizada (conserva la energía)
    void hadamard8(std::array<float, 8>& v)
    {
        for (int len = 1; len < 8; len <<= 1)
            for (int i = 0; i < 8; i += len << 1)
                for (int j = i; j < i + len; ++j)
                {
                    const float a = v[(size_t) j];
                    const float b = v[(size_t) (j + len)];
                    v[(size_t) j] = a + b;
                    v[(size_t) (j + len)] = a - b;
                }

        for (auto& x : v)
            x *= 0.35355339f;
    }
}

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
    for (auto& s : specBands)
        s.store(0.0f);
}

juce::AudioProcessorValueTreeState::ParameterLayout VoxStudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("in_gain", "Input",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("dyn_gate", "Gate",
        juce::NormalisableRange<float>(-80.0f, 0.0f, 0.1f), -50.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dyn_hyst", "Hysteresis",
        juce::NormalisableRange<float>(0.0f, 12.0f, 0.1f), 6.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dyn_deess", "De-Ess",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dyn_focus", "Focus",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dyn_drive", "Dyn Drive",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dyn_color", "Color",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dyn_hp", "HP Freq",
        juce::NormalisableRange<float>(20.0f, 500.0f, 0.1f, 0.4f), 20.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dyn_lp", "LP Freq",
        juce::NormalisableRange<float>(2000.0f, 20000.0f, 1.0f, 0.4f), 20000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dyn_mix", "Dyn Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));

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
    params.push_back(std::make_unique<juce::AudioParameterFloat>("rev_decay", "Decay",
        juce::NormalisableRange<float>(0.1f, 5.0f, 0.01f), 1.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("rev_damp", "Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("rev_mix", "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.25f));
    params.push_back(std::make_unique<juce::AudioParameterInt>("rev_preset", "Preset", 0, 4, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("rev_pre", "Pre-Delay",
        juce::NormalisableRange<float>(0.0f, 200.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("mod_rate", "Mod Rate",
        juce::NormalisableRange<float>(0.05f, 5.0f, 0.01f, 0.5f), 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("mod_depth", "Mod Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.35f));

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

    for (auto& band : eqBands)
        band.prepare(spec);
    deEsserBand.prepare(spec);

    dynColorBand.prepare(spec);
    dynHpBand.prepare(spec);
    dynLpBand.prepare(spec);
    dynDryBuffer.setSize(2, samplesPerBlock);
    dynDryBuffer.clear();

    sideBuffer.setSize(2, samplesPerBlock);
    sideBuffer.clear();
    deEsserEnv = 0.0f;

    const int maxDelay = static_cast<int>(sampleRate * 4.0) + 1;
    delayBuffer.setSize(2, maxDelay);
    delayBuffer.clear();
    delayWritePos = 0;

    // Pre-delay: 200 ms más margen para la modulación
    preDelayBuffer.setSize(2, static_cast<int>(sampleRate * 0.25) + 1);
    preDelayBuffer.clear();
    preDelayWritePos = 0;
    lfoPhase = 0.0;

    revSendBuffer.setSize(2, samplesPerBlock);
    revSendBuffer.clear();

    // Reverb propio: cada línea de retardo hasta 0.25 s
    const int lineMax = static_cast<int>(sampleRate * 0.25) + 64;
    for (auto& line : fdnBuf)
        line.assign((size_t) lineMax, 0.0f);
    fdnWrite.fill(0);
    fdnDamp.fill(0.0f);
    reverbHpState = { 0.0f, 0.0f };
    lfoPhaseRev = 0.0;

    gateEnv = 0.0f;
    gateGainSm = 0.0f;
    gateOpenState = false;

    fftFill = 0;

    applyReverbPreset(static_cast<int>(apvts.getRawParameterValue("rev_preset")->load()));
    updateEq();
}

bool VoxStudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}

// Solo guarda el tipo de reverb; el carácter (tamaño y brillo) se aplica en runFdnReverb
void VoxStudioProcessor::applyReverbPreset(int index)
{
    reverbPresetIdx = juce::jlimit(0, 4, index);
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

void VoxStudioProcessor::updateDynFilters()
{
    const float hp  = apvts.getRawParameterValue("dyn_hp")->load();
    const float lp  = apvts.getRawParameterValue("dyn_lp")->load();
    const float col = apvts.getRawParameterValue("dyn_color")->load();
    const float nyq = 0.45f * (float) currentSampleRate;

    *dynHpBand.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass((float) currentSampleRate,
                                                                          juce::jmin(hp, nyq), 0.707f);
    *dynLpBand.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass((float) currentSampleRate,
                                                                         juce::jmin(lp, nyq), 0.707f);
    *dynColorBand.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf((float) currentSampleRate, 3000.0f, 0.707f,
                              juce::Decibels::decibelsToGain((col - 0.5f) * 12.0f));
}

void VoxStudioProcessor::processDynamics(juce::AudioBuffer<float>& buffer)
{
    const float thrDb    = apvts.getRawParameterValue("dyn_gate")->load();
    const float hyst     = apvts.getRawParameterValue("dyn_hyst")->load();
    const float openLin  = juce::Decibels::decibelsToGain(thrDb);
    const float closeLin = juce::Decibels::decibelsToGain(thrDb - hyst);
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

void VoxStudioProcessor::processDynModule(juce::AudioBuffer<float>& buffer)
{
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || n == 0 || dynDryBuffer.getNumSamples() < n)
        return;

    const float mix   = apvts.getRawParameterValue("dyn_mix")->load();
    const float drive = apvts.getRawParameterValue("dyn_drive")->load();

    for (int ch = 0; ch < 2; ++ch)
        dynDryBuffer.copyFrom(ch, 0, buffer, ch, 0, n);

    if (drive > 0.0f)
    {
        const float k    = 1.0f + drive * 9.0f;
        const float norm = std::tanh(k);
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* d = buffer.getWritePointer(ch);
            for (int i = 0; i < n; ++i)
                d[i] = std::tanh(d[i] * k) / norm;
        }
    }

    updateDynFilters();
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    dynHpBand.process(ctx);
    dynLpBand.process(ctx);
    dynColorBand.process(ctx);

    for (int ch = 0; ch < 2; ++ch)
    {
        buffer.applyGain(ch, 0, n, mix);
        buffer.addFrom(ch, 0, dynDryBuffer, ch, 0, n, 1.0f - mix);
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

// Pre-delay con LFO por muestra e interpolación lineal (sin saltos entre bloques)
void VoxStudioProcessor::processPreDelay(juce::AudioBuffer<float>& buffer)
{
    const float ms    = apvts.getRawParameterValue("rev_pre")->load();
    const float rate  = apvts.getRawParameterValue("mod_rate")->load();
    const float depth = apvts.getRawParameterValue("mod_depth")->load();

    const int bufLen = preDelayBuffer.getNumSamples();
    if (bufLen <= 2)
        return;

    const int n = buffer.getNumSamples();
    const int numCh = juce::jmin(buffer.getNumChannels(), preDelayBuffer.getNumChannels());

    const double baseSamples = ms * 0.001 * currentSampleRate;
    const double modAmp      = depth * 0.004 * currentSampleRate;
    const double lfoInc      = 2.0 * juce::MathConstants<double>::pi * rate / currentSampleRate;
    const double twoPi       = 2.0 * juce::MathConstants<double>::pi;

    for (int i = 0; i < n; ++i)
    {
        const double d = juce::jlimit(0.0, bufLen - 2.0, baseSamples + modAmp * std::sin(lfoPhase));

        lfoPhase += lfoInc;
        if (lfoPhase > twoPi)
            lfoPhase -= twoPi;

        const int i0 = static_cast<int>(d);
        const float frac = static_cast<float>(d - i0);

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            auto* pb   = preDelayBuffer.getWritePointer(ch);

            const int wp = (preDelayWritePos + i) % bufLen;
            pb[wp] = data[i];

            const int r0 = (wp - i0 + bufLen) % bufLen;
            const int r1 = (r0 - 1 + bufLen) % bufLen;
            data[i] = pb[r0] * (1.0f - frac) + pb[r1] * frac;
        }
    }

    preDelayWritePos = (preDelayWritePos + n) % bufLen;
}

// Red de 8 líneas de retardo con realimentación Hadamard, filtro de amortiguación y modulación suave.
// Procesa la señal de entrada como cola 100 % mojada.
void VoxStudioProcessor::runFdnReverb(juce::AudioBuffer<float>& buffer)
{
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || n == 0)
        return;

    const float sr    = (float) currentSampleRate;
    const float room  = apvts.getRawParameterValue("rev_room")->load();
    const float decay = juce::jmax(0.1f, apvts.getRawParameterValue("rev_decay")->load());
    const float damp  = apvts.getRawParameterValue("rev_damp")->load();
    const float depth = apvts.getRawParameterValue("mod_depth")->load();
    const float rate  = apvts.getRawParameterValue("mod_rate")->load();

    const int pIdx = juce::jlimit(0, 4, reverbPresetIdx);
    const float scale = (0.5f + room) * kPresetScale[pIdx];
    const float dampA = juce::jmap(juce::jlimit(0.0f, 1.0f, damp + kPresetDampOffset[pIdx]),
                                   0.0f, 1.0f, 0.9f, 0.12f);

    std::array<float, kFdnLines> lenS {};
    std::array<float, kFdnLines> gains {};
    for (int i = 0; i < kFdnLines; ++i)
    {
        const size_t ui = (size_t) i;
        const float size = (float) fdnBuf[ui].size();
        lenS[ui] = juce::jlimit(4.0f, size - 8.0f, kBaseMs[i] * 0.001f * sr * scale);
        gains[ui] = std::pow(0.001f, lenS[ui] / (decay * sr));
    }

    const float modAmp = depth * 0.0015f * sr;
    const float lfoInc = 2.0f * juce::MathConstants<float>::pi * rate / sr;
    const float twoPi  = 2.0f * juce::MathConstants<float>::pi;
    const float hpA    = std::exp(-2.0f * juce::MathConstants<float>::pi * 150.0f / sr);

    float* L = buffer.getWritePointer(0);
    float* R = buffer.getWritePointer(1);

    for (int s = 0; s < n; ++s)
    {
        const float inL = L[s];
        const float inR = R[s];

        // Filtro paso alto de entrada: quita el barro de graves
        reverbHpState[0] = hpA * reverbHpState[0] + (1.0f - hpA) * inL;
        reverbHpState[1] = hpA * reverbHpState[1] + (1.0f - hpA) * inR;
        const float hpL = inL - reverbHpState[0];
        const float hpR = inR - reverbHpState[1];

        // Lectura de las 8 líneas (con modulación y amortiguación)
        std::array<float, kFdnLines> taps {};
        for (int i = 0; i < kFdnLines; ++i)
        {
            const size_t ui = (size_t) i;
            const int size = (int) fdnBuf[ui].size();

            const float mod = modAmp * std::sin(lfoPhaseRev + 0.9f * (float) i);
            const float d = juce::jlimit(2.0f, (float) size - 4.0f, lenS[ui] + mod);
            const int di = (int) d;
            const float frac = d - (float) di;

            int r0 = fdnWrite[ui] - di;
            if (r0 < 0)
                r0 += size;
            int r1 = r0 - 1;
            if (r1 < 0)
                r1 += size;

            const float delayed = fdnBuf[ui][(size_t) r0] * (1.0f - frac)
                                + fdnBuf[ui][(size_t) r1] * frac;

            fdnDamp[ui] += dampA * (delayed - fdnDamp[ui]);
            taps[ui] = fdnDamp[ui];
        }

        // Realimentación: mezcla Hadamard de las salidas, con la ganancia de cada línea
        std::array<float, kFdnLines> mixed = taps;
        hadamard8(mixed);

        float wetL = 0.0f;
        float wetR = 0.0f;

        for (int i = 0; i < kFdnLines; ++i)
        {
            const size_t ui = (size_t) i;
            const float in = (i % 2 == 0) ? hpL : hpR;
            const int size = (int) fdnBuf[ui].size();

            fdnBuf[ui][(size_t) fdnWrite[ui]] = in * 0.3f + mixed[ui] * gains[ui];
            fdnWrite[ui] = (fdnWrite[ui] + 1) % size;

            if (i % 2 == 0)
                wetL += taps[ui];
            else
                wetR += taps[ui];
        }

        L[s] = wetL * 0.3f;
        R[s] = wetR * 0.3f;

        lfoPhaseRev += lfoInc;
        if (lfoPhaseRev > twoPi)
            lfoPhaseRev -= twoPi;
    }
}

// Reverb con pre-delay: la cola se procesa aparte, así la voz seca no se retrasa
void VoxStudioProcessor::processReverbWithPreDelay(juce::AudioBuffer<float>& buffer)
{
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || n == 0)
        return;

    updateReverb();

    if (revSendBuffer.getNumSamples() < n)
        return;

    const float mix = apvts.getRawParameterValue("rev_mix")->load();

    for (int ch = 0; ch < 2; ++ch)
        revSendBuffer.copyFrom(ch, 0, buffer, ch, 0, n);

    processPreDelay(revSendBuffer);
    runFdnReverb(revSendBuffer);

    for (int ch = 0; ch < 2; ++ch)
    {
        buffer.applyGain(ch, 0, n, 1.0f - mix);
        buffer.addFrom(ch, 0, revSendBuffer, ch, 0, n, mix);
    }
}

void VoxStudioProcessor::pushSpectrum(const juce::AudioBuffer<float>& buffer)
{
    const int n = buffer.getNumSamples();
    if (n == 0 || buffer.getNumChannels() < 1)
        return;

    const float* L = buffer.getReadPointer(0);
    const float* R = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : L;

    for (int i = 0; i < n; ++i)
    {
        fftBuffer[(size_t) fftFill++] = 0.5f * (L[i] + R[i]);

        if (fftFill < 2048)
            continue;

        std::fill(fftBuffer.begin() + 2048, fftBuffer.end(), 0.0f);
        fftWindow.multiplyWithWindowingTable(fftBuffer.data(), 2048);
        fft.performFrequencyOnlyForwardTransform(fftBuffer.data());

        for (int b = 0; b < numSpecBands; ++b)
        {
            const float t0 = (float) b / (float) numSpecBands;
            const float t1 = (float) (b + 1) / (float) numSpecBands;
            const int k0 = juce::jlimit(1, 1022, (int) std::pow(1023.0f, t0));
            const int k1 = juce::jlimit(k0 + 1, 1023, (int) std::pow(1023.0f, t1));

            float peak = 0.0f;
            for (int k = k0; k < k1; ++k)
                peak = juce::jmax(peak, fftBuffer[(size_t) k]);

            const float db   = juce::Decibels::gainToDecibels(peak / 1024.0f, -90.0f);
            const float v    = juce::jmap(juce::jlimit(-90.0f, 0.0f, db), -90.0f, 0.0f, 0.0f, 1.0f);
            const float prev = specBands[(size_t) b].load();
            specBands[(size_t) b].store(v > prev ? v : prev * 0.8f + v * 0.2f);
        }

        fftFill = 0;
    }
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
    processDynModule(buffer);

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

    processReverbWithPreDelay(buffer);

    outputGain.setGainDecibels(apvts.getRawParameterValue("out_gain")->load());
    outputGain.process(ctx);

    pushSpectrum(buffer);
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
