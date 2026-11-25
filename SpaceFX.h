#pragma once
#include <juce_dsp/juce_dsp.h>
#include "Dynamics.h"

namespace nescrun
{
// Linea de retardo fraccional simple
struct DelayLine
{
    std::vector<float> b; int w = 0, mask = 0;
    void prepare (int maxSamples) { int s = 1; while (s < maxSamples + 4) s <<= 1; b.assign ((size_t) s, 0.f); mask = s - 1; w = 0; }
    void push (float x) { b[(size_t) w] = x; w = (w + 1) & mask; }
    float read (float d) const
    {
        const float rp = (float) w - 1.f - d;
        const int i = (int) std::floor (rp); const float f = rp - (float) i;
        const float a = b[(size_t) (i & mask)], c = b[(size_t) ((i + 1) & mask)];
        return a + (c - a) * f;
    }
};

// ---------------------------------------------------------------- Modulacion: Doubler / Chorus / Flanger / Phaser
class Modulation
{
public:
    void prepare (double s)
    {
        sr = s;
        for (auto& d : dl) d.prepare ((int) (0.06 * sr));
        phase = 0.f; fbL = fbR = 0.f; rnd1 = rnd2 = 0.f; rt1 = rt2 = 0.f;
        for (auto& a : apL) a = {};
        for (auto& a : apR) a = {};
    }
    void setParams (int type_, float amount01, float rateHz) { type = type_; amount = amount01; rate = rateHz; }

    void process (float& l, float& r)
    {
        const float inL = l, inR = r;
        const float twoPi = juce::MathConstants<float>::twoPi;
        phase += rate / (float) sr; if (phase >= 1.f) phase -= 1.f;
        const float s1 = std::sin (twoPi * phase), s2 = std::sin (twoPi * phase + 1.7f);
        switch (type)
        {
            case 0: // Doubler: dos tomas "falsas" con micro-desafinacion y timing humano, abiertas L/R
            {
                if (++rc >= 2048) { rc = 0; rt1 = rng.nextFloat() * 2.f - 1.f; rt2 = rng.nextFloat() * 2.f - 1.f; }
                rnd1 += (rt1 - rnd1) * 0.0004f; rnd2 += (rt2 - rnd2) * 0.0004f;
                dl[0].push (inL); dl[1].push (inR);
                const float d1 = (0.014f + 0.004f * rnd1 + 0.0007f * s1) * (float) sr;
                const float d2 = (0.021f + 0.004f * rnd2 + 0.0007f * s2) * (float) sr;
                const float a = dl[0].read (d1), b = dl[1].read (d2);
                l = inL + a * amount * 0.8f - b * amount * 0.15f;
                r = inR + b * amount * 0.8f - a * amount * 0.15f;
                break;
            }
            case 1: // Chorus ochentoso (estilo guitarras/voces new wave)
            {
                dl[0].push (inL); dl[1].push (inR);
                const float base = 0.008f * (float) sr, dep = 0.0035f * (float) sr;
                const float a = dl[0].read (base + dep * (0.5f + 0.5f * s1));
                const float b = dl[1].read (base + dep * (0.5f - 0.5f * s1));
                l = inL * (1.f - amount * 0.3f) + a * amount * 0.7f;
                r = inR * (1.f - amount * 0.3f) + b * amount * 0.7f;
                break;
            }
            case 2: // Flanger con realimentacion
            {
                const float base = 0.0008f * (float) sr, dep = 0.0045f * (float) sr;
                dl[0].push (inL + fbL * 0.55f * amount); dl[1].push (inR + fbR * 0.55f * amount);
                const float a = dl[0].read (base + dep * (0.5f + 0.5f * s1));
                const float b = dl[1].read (base + dep * (0.5f + 0.5f * s2));
                fbL = a; fbR = b;
                l = (inL + a * amount) / (1.f + amount * 0.5f);
                r = (inR + b * amount) / (1.f + amount * 0.5f);
                break;
            }
            default: // Phaser de 6 etapas (psicodelico)
            {
                auto stage = [] (AP& s, float x, float c) { const float y = -c * x + s.x1 + c * s.y1; s.x1 = x; s.y1 = y; return y; };
                const float fmin = 300.f, fmax = 3500.f;
                auto coefFor = [&] (float m)
                {
                    const float f = fmin * std::pow (fmax / fmin, 0.5f + 0.5f * m);
                    const float t = std::tan (juce::MathConstants<float>::pi * f / (float) sr);
                    return (t - 1.f) / (t + 1.f);
                };
                const float cL = coefFor (s1), cR = coefFor (s2);
                float a = inL + fbL * 0.5f, b = inR + fbR * 0.5f;
                for (auto& st : apL) a = stage (st, a, cL);
                for (auto& st : apR) b = stage (st, b, cR);
                fbL = a; fbR = b;
                l = inL + a * amount; r = inR + b * amount;
                l /= 1.f + amount * 0.5f; r /= 1.f + amount * 0.5f;
                break;
            }
        }
    }
private:
    struct AP { float x1 = 0.f, y1 = 0.f; };
    double sr = 44100.0; int type = 0, rc = 0; float amount = 0.f, rate = 0.5f, phase = 0.f, fbL = 0.f, fbR = 0.f;
    float rnd1 = 0.f, rnd2 = 0.f, rt1 = 0.f, rt2 = 0.f;
    DelayLine dl[2]; std::array<AP, 6> apL {}, apR {}; juce::Random rng { 1234 };
};

// ---------------------------------------------------------------- Delay sincronizado con ducking
class SyncDelay
{
public:
    void prepare (double s) { sr = s; dl[0].prepare ((int) (4.0 * sr)); dl[1].prepare ((int) (4.0 * sr)); lpL = lpR = hpL = hpR = 0.f; duckEnv = 0.f; curD = -1.f; }
    void setParams (float seconds, float fb01, float tone01, bool pp, float duck01)
    {
        target = (float) juce::jlimit (0.01, 3.9, (double) seconds) * (float) sr;
        if (curD < 0.f) curD = target;
        fb = fb01; pingpong = pp; duck = duck01;
        lpA = 1.f - std::exp (-2.f * juce::MathConstants<float>::pi * (2000.f + tone01 * 10000.f) / (float) sr);
        hpA = 1.f - std::exp (-2.f * juce::MathConstants<float>::pi * (150.f + (1.f - tone01) * 400.f) / (float) sr);
    }
    // devuelve solo la senal mojada
    void process (float inMono, float& wl, float& wr)
    {
        curD += (target - curD) * 0.0005f;
        float a = dl[0].read (curD), b = dl[1].read (curD);
        // filtros en el loop (los ecos se oscurecen como un delay de cinta)
        lpL += (a - lpL) * lpA; lpR += (b - lpR) * lpA;
        hpL += (lpL - hpL) * hpA; hpR += (lpR - hpR) * hpA;
        a = lpL - hpL; b = lpR - hpR;
        if (pingpong) { dl[0].push (inMono + b * fb); dl[1].push (a * fb); }
        else          { dl[0].push (inMono + a * fb); dl[1].push (inMono + b * fb); }
        const float ax = std::abs (inMono);
        duckEnv += (ax - duckEnv) * (ax > duckEnv ? msCoef (sr, 5.f) : msCoef (sr, 250.f));
        const float g = 1.f / (1.f + duck * 12.f * duckEnv);
        wl = a * g; wr = b * g;
        if (std::abs (wl) < 1e-15f) wl = 0.f;
        if (std::abs (wr) < 1e-15f) wr = 0.f;
    }
private:
    double sr = 44100.0; DelayLine dl[2];
    float target = 10000.f, curD = -1.f, fb = 0.3f, lpA = 0.5f, hpA = 0.1f, lpL = 0.f, lpR = 0.f, hpL = 0.f, hpR = 0.f, duck = 0.f, duckEnv = 0.f;
    bool pingpong = true;
};

// ---------------------------------------------------------------- Reverb con pre-delay y ducking
class VocalReverb
{
public:
    void prepare (double s, int block)
    {
        sr = s; pre.prepare ((int) (0.2 * sr));
        rev.setSampleRate (s); rev.reset();
        hp = 0.f; duckEnv = 0.f; juce::ignoreUnused (block); updateHp();
    }
    void setParams (float size01, float damp01, float preMs, float duck01)
    {
        juce::Reverb::Parameters p;
        p.roomSize = 0.35f + size01 * 0.62f; p.damping = 0.15f + damp01 * 0.8f;
        p.wetLevel = 1.f; p.dryLevel = 0.f; p.width = 1.f; p.freezeMode = 0.f;
        rev.setParameters (p);
        preS = preMs * 0.001f * (float) sr; duck = duck01;
    }
    // procesa un bloque: mono -> L/R mojado
    void processBlock (const float* mono, float* wl, float* wr, int num)
    {
        for (int i = 0; i < num; ++i)
        {
            hp += (mono[i] - hp) * hpA;      // corta graves antes de la reverb para que no embarre
            pre.push (mono[i] - hp);
            wl[i] = wr[i] = pre.read (std::max (0.f, preS));
        }
        rev.processStereo (wl, wr, num);
        for (int i = 0; i < num; ++i)
        {
            const float ax = std::abs (mono[i]);
            duckEnv += (ax - duckEnv) * (ax > duckEnv ? msCoef (sr, 8.f) : msCoef (sr, 300.f));
            const float g = 0.35f / (1.f + duck * 10.f * duckEnv);
            wl[i] *= g; wr[i] *= g;
        }
    }
    void updateHp() { hpA = 1.f - std::exp (-2.f * juce::MathConstants<float>::pi * 280.f / (float) sr); }
private:
    double sr = 44100.0; DelayLine pre; juce::Reverb rev; float preS = 0.f, duck = 0.f, duckEnv = 0.f, hp = 0.f, hpA = 0.04f;
};
} // namespace nescrun
