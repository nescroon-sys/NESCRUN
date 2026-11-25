#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace nescrun;

NescrunProcessor::NescrunProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "NESCRUN", createLayout())
{
}

bool NescrunProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    const auto in  = layouts.getMainInputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    return in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono();
}

void NescrunProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    tuner.prepare (sr);
    reso.prepare (sr);
    gate.prepare (sr);
    deesser.prepare (sr);
    comp.prepare (sr); leveler.prepare (sr);
    eq.prepare (sr); silk.prepare (sr); sat.prepare (sr); boom.prepare (sr);
    mod.prepare (sr); delay.prepare (sr); reverb.prepare (sr, samplesPerBlock);
    limiter.prepare (sr, samplesPerBlock);

    totalLatency = tuner.getLatency() + reso.getLatency() + limiter.getLatency();
    setLatencySamples (totalLatency);
    dryL.prepare (totalLatency + 8); dryR.prepare (totalLatency + 8);

    const int maxB = std::max (samplesPerBlock, 32);
    mono.assign ((size_t) maxB, 0.f); revL.assign ((size_t) maxB, 0.f); revR.assign ((size_t) maxB, 0.f);

    for (auto* s : { &inGainS, &outGainS, &mixS, &delayMixS, &revMixS, &widthS }) s->reset (sr, 0.03);
    inGainS.setCurrentAndTargetValue (linOf (p ("in_gain")));
    outGainS.setCurrentAndTargetValue (linOf (p ("out_gain")));
    mixS.setCurrentAndTargetValue (p ("mix") * 0.01f);
    delayMixS.setCurrentAndTargetValue (p ("delay_mix") * 0.01f);
    revMixS.setCurrentAndTargetValue (p ("rev_mix") * 0.01f);
    widthS.setCurrentAndTargetValue (p ("width") * 0.01f);

    const float freqs[6] = { 120.f, 350.f, 1200.f, 4000.f, 12000.f, 7000.f };
    for (int i = 0; i < 6; ++i)
    {
        const float f = (float) std::min ((double) freqs[i], sr * 0.45);
        amBands[(size_t) i].f.coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass (sr, f, 1.4f);
        amBands[(size_t) i].f.reset();
    }
}

void NescrunProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels(), numOut = getTotalNumOutputChannels();
    for (int ch = numIn; ch < numOut; ++ch) buffer.clear (ch, 0, n);
    if (n <= 0) return;

    if ((int) mono.size() < n) { mono.resize ((size_t) n); revL.resize ((size_t) n); revR.resize ((size_t) n); } // por si el host cambia el bloque

    // ---------- parametros
    const int tuneMode = (int) p ("tune_mode");
    tuner.setParams (tuneMode, (int) p ("tune_key"), (int) p ("tune_scale"), p ("tune_speed"), p ("tune_amount") * 0.01f, p ("tune_human") * 0.01f);
    const bool cleanOn = p ("clean_on") > 0.5f, compOn = p ("comp_on") > 0.5f, eqOn = p ("eq_on") > 0.5f, colorOn = p ("color_on") > 0.5f;
    const bool modOn = p ("mod_on") > 0.5f, delayOn = p ("delay_on") > 0.5f, revOn = p ("rev_on") > 0.5f, limOn = p ("lim_on") > 0.5f;
    gate.setThreshold (p ("gate_thresh"));
    reso.setParams (cleanOn ? p ("reso_amount") * 0.01f : 0.f, p ("reso_sens") * 0.01f);
    deesser.setParams (p ("deess_thresh"), p ("deess_freq"));
    comp.setParams ((int) p ("comp_style"), p ("comp_amount") * 0.01f, p ("comp_mix") * 0.01f);
    leveler.setParams (Compressor::Opto, p ("level_amount") * 0.006f, 1.f);
    eq.setParams (cleanOn ? p ("hpf_freq") : 20.f, p ("eq_body"), p ("eq_mud"), p ("eq_mid"), p ("eq_presence"), p ("eq_air"));
    silk.setParams (p ("silk_amount") * 0.01f, p ("silk_blue") > 0.5f);
    sat.setParams ((int) p ("sat_type"), p ("sat_drive") * 0.01f, p ("sat_tone") * 0.01f, p ("sat_mix") * 0.01f);
    boom.setAmount (p ("boom") * 0.01f);
    mod.setParams ((int) p ("mod_type"), p ("mod_amount") * 0.01f, p ("mod_rate"));

    double bpm = 120.0;
    if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) if (auto b = pos->getBpm()) bpm = *b;
    static const float beats[8] = { 0.25f, 0.5f, 0.75f, 1.f, 1.5f, 2.f, 1.f / 3.f, 2.f / 3.f };
    delay.setParams ((float) (60.0 / juce::jlimit (30.0, 300.0, bpm)) * beats[juce::jlimit (0, 7, (int) p ("delay_div"))],
                     p ("delay_fb") * 0.01f, p ("delay_tone") * 0.01f, p ("delay_pp") > 0.5f, p ("delay_duck") * 0.01f);
    reverb.setParams (p ("rev_size") * 0.01f, p ("rev_damp") * 0.01f, p ("rev_predelay"), p ("rev_duck") * 0.01f);
    limiter.setParams (limOn ? p ("loud") : 0.f, p ("ceiling"));

    inGainS.setTargetValue (linOf (p ("in_gain")));
    outGainS.setTargetValue (linOf (p ("out_gain")));
    mixS.setTargetValue (p ("mix") * 0.01f);
    delayMixS.setTargetValue (delayOn ? p ("delay_mix") * 0.01f : 0.f);
    revMixS.setTargetValue (revOn ? p ("rev_mix") * 0.01f : 0.f);
    widthS.setTargetValue (p ("width") * 0.01f);

    float* L = buffer.getWritePointer (0);
    float* R = numOut > 1 ? buffer.getWritePointer (1) : nullptr;
    const float* inR = numIn > 1 ? buffer.getReadPointer (1) : buffer.getReadPointer (0);

    // ---------- etapa mono (cadena vocal)
    float inPk = 0.f, gMinC = 0.f, gMinD = 0.f;
    for (int i = 0; i < n; ++i)
    {
        const float raw = 0.5f * (L[i] + inR[i]);
        inPk = std::max (inPk, std::abs (raw));
        mono[(size_t) i] = raw * inGainS.getNextValue();
    }
    if (autoMixState.load() == 1) analyseAutoMix (mono.data(), n);

    for (int i = 0; i < n; ++i)
    {
        float x = mono[(size_t) i];
        if (cleanOn) { x = eq.processHPF (x); x = gate.process (x); }
        x = tuner.processSample (x);
        x = reso.processSample (x);
        if (cleanOn) { x = deesser.process (x); gMinD = std::min (gMinD, deesser.grDb); }
        if (compOn)  { x = comp.process (x); x = leveler.process (x); gMinC = std::min (gMinC, comp.grDb + leveler.grDb); }
        if (eqOn)    { x = eq.processEQ (x); x = silk.process (x); }
        if (colorOn) { x = sat.process (x); x = boom.process (x); }
        mono[(size_t) i] = x;
    }

    // ---------- reverb en bloque
    reverb.processBlock (mono.data(), revL.data(), revR.data(), n);

    // ---------- etapa estereo + master
    float outPk = 0.f, limMin = 0.f;
    const float ceilLin = linOf (p ("ceiling"));
    for (int i = 0; i < n; ++i)
    {
        const float x = mono[(size_t) i];
        float l = x, r = x;
        if (modOn) mod.process (l, r);
        float dl, dr; delay.process (x, dl, dr);
        const float dm = delayMixS.getNextValue(), rm = revMixS.getNextValue();
        l += dl * dm + revL[(size_t) i] * rm * 2.f;
        r += dr * dm + revR[(size_t) i] * rm * 2.f;
        // ancho estereo (M/S)
        const float w = widthS.getNextValue();
        const float m = 0.5f * (l + r), s = 0.5f * (l - r) * w;
        l = m + s; r = m - s;
        if (limOn) limiter.process (l, r);
        else { limiter.delayOnly (l, r); l = juce::jlimit (-ceilLin, ceilLin, l); r = juce::jlimit (-ceilLin, ceilLin, r); }
        limMin = std::min (limMin, limiter.grDb);

        // mezcla seco/mojado con el seco alineado en tiempo
        dryL.push (L[i]); dryR.push (inR[i]);
        const float dryLv = dryL.read ((float) totalLatency), dryRv = dryR.read ((float) totalLatency);
        const float mx = mixS.getNextValue(), og = outGainS.getNextValue();
        l = (dryLv * (1.f - mx) + l * mx) * og;
        r = (dryRv * (1.f - mx) + r * mx) * og;
        outPk = std::max (outPk, std::max (std::abs (l), std::abs (r)));
        L[i] = l; if (R != nullptr) R[i] = r;
    }

    inLevel.store (dbOf (inPk)); outLevel.store (dbOf (outPk));
    compGr.store (gMinC); deessGr.store (gMinD); limGr.store (limOn ? limMin : 0.f); resoGr.store (reso.reduction.load());
}

// =============================================================== Auto Mix
void NescrunProcessor::startAutoMix()
{
    for (auto& b : amBands) { b.sum = 0.0; b.f.reset(); }
    amSumSq = 0.0; amPeak = 0.0; amActive = 0; amBlockDb.clear();
    amTarget = (long) (sr * 7.0); // 7 segundos de voz real
    autoMixProgress.store (0.f);
    autoMixState.store (1);
    startTimerHz (10);
}

void NescrunProcessor::analyseAutoMix (const float* x, int n)
{
    // el mono ya incluye la ganancia de entrada actual; la compensamos para medir la toma cruda
    const float g = linOf (p ("in_gain"));
    double ss = 0.0;
    for (int i = 0; i < n; ++i) ss += (double) (x[i] / g) * (x[i] / g);
    const float blockDb = dbOf ((float) std::sqrt (ss / n));
    if (amBlockDb.size() < 50000) amBlockDb.push_back (blockDb);
    if (blockDb < -50.f) return; // silencio: no cuenta
    for (int i = 0; i < n; ++i)
    {
        const float v = x[i] / g;
        amSumSq += (double) v * v; amPeak = std::max (amPeak, (double) std::abs (v));
        for (auto& b : amBands) { const float y = b.f.processSample (v); b.sum += (double) y * y; }
    }
    amActive += n;
    autoMixProgress.store (std::min (1.f, (float) amActive / (float) amTarget));
    if (amActive < amTarget) return;

    const double cnt = (double) amActive;
    const float rmsDb = dbOf ((float) std::sqrt (amSumSq / cnt));
    const float crest = dbOf ((float) amPeak) - rmsDb;
    float band[6];
    for (int i = 0; i < 6; ++i) band[i] = dbOf ((float) std::sqrt (amBands[(size_t) i].sum / cnt));
    const float ref = band[2];
    // perfil objetivo de una voz comercial (relativo a 1.2 kHz)
    const float target[5] = { -6.f, -1.5f, 0.f, -5.f, -14.f };
    const float base[5]   = { 0.f, -1.f, 0.f, 1.5f, 2.5f };
    float corr[5];
    for (int i = 0; i < 5; ++i)
        corr[i] = juce::jlimit (-8.f, 8.f, base[i] + juce::jlimit (-6.f, 6.f, (target[i] - (band[i] - ref)) * 0.6f));

    AutoResult r {};
    r.inGain = juce::jlimit (-18.f, 18.f, -20.f - rmsDb);
    std::vector<float> sorted = amBlockDb; std::sort (sorted.begin(), sorted.end());
    const float floorDb = sorted.empty() ? -80.f : sorted[(size_t) (sorted.size() / 10)];
    r.gate  = juce::jlimit (-90.f, -40.f, floorDb + r.inGain + 6.f);
    r.deess = juce::jlimit (-45.f, -10.f, band[5] + r.inGain + 3.f);
    r.comp  = juce::jlimit (30.f, 75.f, (crest - 10.f) * 4.f + 35.f);
    r.level = juce::jlimit (25.f, 60.f, crest * 2.f + 10.f);
    r.body = corr[0]; r.mud = corr[1]; r.mid = corr[2]; r.pres = corr[3]; r.air = corr[4];
    amResult = r;
    autoMixState.store (2);
}

void NescrunProcessor::timerCallback()
{
    if (autoMixState.load() != 2) return;
    stopTimer();
    const auto& r = amResult;
    applyPresetValues ({ { "in_gain", r.inGain }, { "gate_thresh", r.gate }, { "deess_thresh", r.deess },
                         { "comp_amount", r.comp }, { "level_amount", r.level }, { "eq_body", r.body },
                         { "eq_mud", r.mud }, { "eq_mid", r.mid }, { "eq_presence", r.pres }, { "eq_air", r.air },
                         { "reso_amount", 55.f }, { "clean_on", 1.f }, { "comp_on", 1.f }, { "eq_on", 1.f } });
    autoMixState.store (0);
    autoMixProgress.store (1.f);
}

// =============================================================== Presets
int NescrunProcessor::getNumPrograms() { return (int) factoryPresets().size(); }

const juce::String NescrunProcessor::getProgramName (int index)
{
    const auto& ps = factoryPresets();
    if (index < 0 || index >= (int) ps.size()) return {};
    return juce::String (ps[(size_t) index].category) + " - " + ps[(size_t) index].name;
}

void NescrunProcessor::applyPresetValues (const std::vector<std::pair<const char*, float>>& values)
{
    for (auto& [id, v] : values)
        if (auto* prm = apvts.getParameter (id))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 (v));
            prm->endChangeGesture();
        }
}

void NescrunProcessor::setCurrentProgram (int index)
{
    const auto& ps = factoryPresets();
    if (index < 0 || index >= (int) ps.size()) return;
    currentPreset = index;
    userPresetName.clear();
    // primero todo a default, despues los valores del preset
    std::vector<std::pair<const char*, float>> vals;
    for (auto& d : paramDefs())
    {
        if (juce::String (d.id) == "in_gain" || juce::String (d.id) == "out_gain") continue; // respeta la ganancia del usuario
        float v = d.def;
        for (auto& [id, pv] : ps[(size_t) index].values) if (juce::String (id) == d.id) v = pv;
        vals.push_back ({ d.id, v });
    }
    applyPresetValues (vals);
}

// =============================================================== Estado
void NescrunProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset, nullptr);
    state.setProperty ("userPreset", userPresetName, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void NescrunProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            currentPreset = (int) vt.getProperty ("preset", 0);
            userPresetName = vt.getProperty ("userPreset", "").toString();
            apvts.replaceState (vt);
        }
}

juce::AudioProcessorEditor* NescrunProcessor::createEditor() { return new NescrunEditor (*this); }

#ifndef NESCRUN_TEST
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NescrunProcessor(); }
#endif
