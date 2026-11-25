#pragma once
#include <juce_dsp/juce_dsp.h>
#include "Dynamics.h"

namespace nescrun
{
using Coefs = juce::dsp::IIR::Coefficients<float>;

// ---------------------------------------------------------------- EQ de tono (5 bandas + corte de graves)
class ToneEQ
{
public:
    void prepare (double s) { sr = s; for (auto& f : f_) f.reset(); for (auto& h : hp) h.reset(); last.fill (-999.f); }
    void setParams (float hpf, float body, float mud, float mid, float pres, float air)
    {
        const std::array<float, 6> v { hpf, body, mud, mid, pres, air };
        if (v == last) return;
        last = v;
        auto hpc = Coefs::makeHighPass (sr, hpf, 0.707f);
        *hp[0].coefficients = *hpc; *hp[1].coefficients = *Coefs::makeHighPass (sr, hpf, 0.54f);
        *f_[0].coefficients = *Coefs::makeLowShelf  (sr, 120.0, 0.7, linOf (body));
        *f_[1].coefficients = *Coefs::makePeakFilter (sr, 350.0, 1.0, linOf (mud));
        *f_[2].coefficients = *Coefs::makePeakFilter (sr, 1200.0, 0.8, linOf (mid));
        *f_[3].coefficients = *Coefs::makePeakFilter (sr, 4000.0, 0.7, linOf (pres));
        *f_[4].coefficients = *Coefs::makeHighShelf (sr, 12000.0, 0.6, linOf (air));
    }
    float processHPF (float x) { return hp[1].processSample (hp[0].processSample (x)); }
    float processEQ (float x) { for (auto& f : f_) x = f.processSample (x); return x; }
private:
    double sr = 44100.0;
    std::array<juce::dsp::IIR::Filter<float>, 5> f_ { mk(), mk(), mk(), mk(), mk() };
    std::array<juce::dsp::IIR::Filter<float>, 2> hp { mk(), mk() };
    std::array<float, 6> last {};
    static juce::dsp::IIR::Filter<float> mk() { juce::dsp::IIR::Filter<float> f; f.coefficients = new Coefs (1, 0, 1, 0); return f; }
};

// ---------------------------------------------------------------- Silk (Red = brillo sedoso, Blue = calidez)
class Silk
{
public:
    void prepare (double s) { sr = s; svf.prepare ({ s, 512, 1 }); svf.reset(); }
    void setParams (float amount01, bool blue_)
    {
        amount = amount01; blue = blue_;
        svf.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
        svf.setCutoffFrequency (blue ? 420.f : 5200.f);
        svf.setResonance (blue ? 0.5f : 0.45f);
    }
    float process (float x)
    {
        if (amount < 0.001f) return x;
        const float band = svf.processSample (0, x);
        const float d = 2.f + amount * 6.f;
        const float harm = std::tanh (band * d) / d - band * 0.6f; // armonicos de la banda
        return x + harm * amount * (blue ? 1.2f : 1.6f) + band * amount * 0.25f;
    }
private:
    double sr = 44100.0; juce::dsp::StateVariableTPTFilter<float> svf; float amount = 0.f; bool blue = false;
};

// ---------------------------------------------------------------- Saturacion (Tape / Tube / Clip)
class Saturator
{
public:
    void prepare (double s) { sr = s; lpState = 0.f; dcX = dcY = 0.f; }
    void setParams (int type_, float drive01, float tone01, float mix01)
    {
        type = type_; mix = mix01;
        drive = 1.f + drive01 * drive01 * 14.f;
        const float fc = 3000.f + tone01 * 15000.f;
        lpA = 1.f - std::exp (-2.f * juce::MathConstants<float>::pi * fc / (float) sr);
        active = drive01 > 0.001f && mix01 > 0.001f;
    }
    float process (float x)
    {
        if (! active) return x;
        float v = x * drive, y;
        switch (type)
        {
            case 1: y = v > 0.f ? std::tanh (v) : std::tanh (v * 0.8f) / 0.8f; break; // tubo asimetrico
            case 2: y = v / (1.f + std::abs (v)); y *= 1.4f; break;                              // clip suave
            default: y = std::tanh (v + 0.08f) - std::tanh (0.08f); break;                    // cinta
        }
        y /= std::pow (drive, 0.85f); // ganancia ~unitaria en senales bajas
        // tono
        lpState += (y - lpState) * lpA;
        y = lpState;
        // bloqueo DC
        const float dc = y - dcX + 0.995f * dcY; dcX = y; dcY = dc;
        return x * (1.f - mix) + dc * mix;
    }
private:
    double sr = 44100.0; int type = 0; bool active = false;
    float drive = 1.f, mix = 0.5f, lpA = 0.5f, lpState = 0.f, dcX = 0.f, dcY = 0.f;
};

// ---------------------------------------------------------------- BOOM: cuerpo, pecho y pegada
// Compresion ascendente de la banda grave (pecho de la voz ~ 90-220 Hz) + realce de pegada
// en el ataque de cada silaba. Hace que la voz se sienta grande y adelante sin subir picos.
class Boom
{
public:
    void prepare (double s)
    {
        sr = s;
        lp.prepare ({ s, 512, 1 }); lp.reset(); lp.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
        lp.setCutoffFrequency (150.f); lp.setResonance (0.55f);
        env = 0.f; fastEnv = slowEnv = 0.f;
    }
    void setAmount (float a) { amount = a; }
    float process (float x)
    {
        if (amount < 0.001f) return x;
        const float low = lp.processSample (0, x);
        const float a = std::abs (low);
        env += (a - env) * (a > env ? msCoef (sr, 3.f) : msCoef (sr, 120.f));
        const float target = 0.12f;
        const float up = juce::jlimit (1.f, 4.f, std::sqrt (target / std::max (env, 1e-4f)));
        // detector de transitorios (pegada)
        const float ax = std::abs (x);
        fastEnv += (ax - fastEnv) * msCoef (sr, 1.f);
        slowEnv += (ax - slowEnv) * msCoef (sr, 40.f);
        const float punch = juce::jlimit (1.f, 2.f, fastEnv / std::max (slowEnv, 1e-5f));
        const float y = x + low * up * amount * 0.9f;
        return y * (1.f + (punch - 1.f) * amount * 0.5f);
    }
private:
    double sr = 44100.0; juce::dsp::StateVariableTPTFilter<float> lp;
    float amount = 0.f, env = 0.f, fastEnv = 0.f, slowEnv = 0.f;
};
} // namespace nescrun
