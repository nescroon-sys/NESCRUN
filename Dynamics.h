#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <vector>

namespace nescrun
{
inline float dbOf (float lin) { return 20.f * std::log10 (std::max (lin, 1.0e-9f)); }
inline float linOf (float db) { return std::pow (10.f, db * 0.05f); }
inline float msCoef (double sr, float ms) { return 1.f - std::exp (-1.f / (std::max (0.01f, ms) * 0.001f * (float) sr)); }

// ---------------------------------------------------------------- Noise floor (expander)
class Gate
{
public:
    void prepare (double s) { sr = s; env = 0.f; g = 1.f; hold = 0; }
    void setThreshold (float db) { thr = db; }
    float process (float x)
    {
        const float a = std::abs (x);
        env += (a - env) * (a > env ? msCoef (sr, 0.5f) : msCoef (sr, 40.f));
        float target = 1.f;
        const float lvl = dbOf (env);
        if (lvl < thr) { if (hold > 0) --hold; else target = linOf (std::max (-40.f, (lvl - thr) * 2.f)); }
        else hold = (int) (0.06 * sr);
        g += (target - g) * (target > g ? msCoef (sr, 1.f) : msCoef (sr, 90.f));
        return x * g;
    }
private:
    double sr = 44100.0; float env = 0.f, g = 1.f, thr = -65.f; int hold = 0;
};

// ---------------------------------------------------------------- De-esser (banda partida)
class DeEsser
{
public:
    void prepare (double s) { sr = s; svf.reset(); svf.prepare ({ s, 512, 1 }); svf.setType (juce::dsp::StateVariableTPTFilterType::lowpass); env = 0.f; g = 1.f; }
    void setParams (float threshDb, float freq) { thr = threshDb; svf.setCutoffFrequency (freq); svf.setResonance (0.6f); }
    float process (float x)
    {
        const float lp = svf.processSample (0, x);
        const float hi = x - lp; // complementario exacto
        const float a = std::abs (hi);
        env += (a - env) * (a > env ? msCoef (sr, 0.3f) : msCoef (sr, 50.f));
        const float over = dbOf (env) - thr;
        const float target = over > 0.f ? linOf (-std::min (14.f, over * 0.75f)) : 1.f;
        g += (target - g) * (target < g ? msCoef (sr, 0.5f) : msCoef (sr, 60.f));
        grDb = dbOf (g);
        return lp + hi * g;
    }
    float grDb = 0.f;
private:
    double sr = 44100.0; juce::dsp::StateVariableTPTFilter<float> svf;
    float env = 0.f, g = 1.f, thr = -26.f;
};

// ---------------------------------------------------------------- Compresor con estilos
class Compressor
{
public:
    enum Style { Clean = 0, FET = 1, Opto = 2, VariMu = 3 };
    void prepare (double s) { sr = s; envDb = -120.f; grSlow = 0.f; gr = 0.f; }
    void setParams (int st, float amount01, float mix01)
    {
        style = st; mix = mix01;
        switch (style)
        {
            case FET:    ratio = 6.f;  atk = 0.4f; rel = 70.f;  knee = 4.f; break; // rapido y agresivo, al frente
            case Opto:   ratio = 3.5f; atk = 10.f; rel = 120.f; knee = 10.f; break; // suave, nivelador
            case VariMu: ratio = 2.2f; atk = 25.f; rel = 350.f; knee = 12.f; break; // pegamento calido
            default:     ratio = 3.f;  atk = 6.f;  rel = 90.f;  knee = 6.f; break;
        }
        thr = -4.f - amount01 * 30.f;
        makeup = amount01 > 0.f ? (-thr * (1.f - 1.f / ratio)) * 0.45f : 0.f;
        active = amount01 > 0.001f;
    }
    float process (float x)
    {
        if (! active) { grDb = 0.f; return x; }
        const float lvl = dbOf (std::abs (x));
        envDb += (lvl - envDb) * (lvl > envDb ? msCoef (sr, atk) : msCoef (sr, rel));
        const float over = envDb - thr;
        float target = 0.f;
        if (over > knee * 0.5f) target = over * (1.f / ratio - 1.f);
        else if (over > -knee * 0.5f) { const float t = over + knee * 0.5f; target = (1.f / ratio - 1.f) * t * t / (2.f * knee); }
        if (style == Opto)
        {   // release en dos etapas tipo opto
            gr += (target - gr) * (target < gr ? msCoef (sr, atk) : msCoef (sr, 60.f));
            grSlow += (gr - grSlow) * (gr < grSlow ? msCoef (sr, 5.f) : msCoef (sr, 900.f));
            grDb = 0.6f * gr + 0.4f * grSlow;
        }
        else
        {
            gr += (target - gr) * (target < gr ? msCoef (sr, atk * 0.5f) : msCoef (sr, rel * 0.5f));
            grDb = gr;
        }
        float y = x * linOf (grDb + makeup);
        if (style == FET)    y = std::tanh (y * 1.15f) / 1.15f * 1.0f + 0.f; // leve mordida de transistor
        if (style == VariMu) y = y - 0.04f * y * std::abs (y);                // armonicos pares suaves
        return x * (1.f - mix) + y * mix;
    }
    float grDb = 0.f;
private:
    double sr = 44100.0; int style = 1; bool active = true;
    float ratio = 4.f, atk = 5.f, rel = 80.f, knee = 6.f, thr = -18.f, makeup = 0.f, mix = 1.f;
    float envDb = -120.f, gr = 0.f, grSlow = 0.f;
};

// ---------------------------------------------------------------- Limitador lookahead (estereo)
class Limiter
{
public:
    void prepare (double s, int)
    {
        sr = s; look = (int) std::ceil (0.0015 * sr);
        for (auto& b : buf) b.assign ((size_t) look + 1, 0.f);
        win.assign ((size_t) look + 1, 1.f);
        pos = 0; g = 1.f; minQ.clear();
    }
    int getLatency() const { return look; }
    void setParams (float driveDb, float ceilingDb) { drive = linOf (driveDb); ceil = linOf (ceilingDb); }

    void process (float& l, float& r)
    {
        l *= drive; r *= drive;
        // soft clip muy suave sobre el techo +2 dB para atrapar picos sin distorsion audible
        auto soft = [this] (float v)
        {
            const float lim = ceil * 1.25f;
            const float a = std::abs (v);
            if (a <= ceil * 0.9f) return v;
            const float k = ceil * 0.9f;
            const float o = k + (lim - k) * std::tanh ((a - k) / (lim - k));
            return std::copysign (o, v);
        };
        l = soft (l); r = soft (r);
        const float peak = std::max (std::abs (l), std::abs (r));
        const float need = peak > ceil ? ceil / peak : 1.f;
        // minimo deslizante sobre la ventana lookahead
        win[(size_t) pos] = need;
        float mn = 1.f; for (auto v : win) mn = std::min (mn, v);
        g = mn < g ? g + (mn - g) * msCoef (sr, 0.35f) : g + (mn - g) * msCoef (sr, 80.f);
        buf[0][(size_t) pos] = l; buf[1][(size_t) pos] = r;
        pos = (pos + 1) % (look + 1);
        const float dl = buf[0][(size_t) pos], dr = buf[1][(size_t) pos];
        const float gg = g;
        grDb = dbOf (gg);
        l = juce::jlimit (-ceil, ceil, dl * gg);
        r = juce::jlimit (-ceil, ceil, dr * gg);
    }
    // limitador apagado: solo mantiene el mismo retardo (latencia constante)
    void delayOnly (float& l, float& r)
    {
        buf[0][(size_t) pos] = l; buf[1][(size_t) pos] = r; win[(size_t) pos] = 1.f;
        pos = (pos + 1) % (look + 1);
        l = buf[0][(size_t) pos]; r = buf[1][(size_t) pos]; grDb = 0.f; g = 1.f;
    }
    float grDb = 0.f;
private:
    float safeMin() { float mn = 1.f; for (auto v : win) mn = std::min (mn, v); return mn; }
    double sr = 44100.0; int look = 64, pos = 0;
    std::vector<float> buf[2], win; std::vector<float> minQ;
    float g = 1.f, drive = 1.f, ceil = 0.89f;
};
} // namespace nescrun
