#pragma once
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cmath>
#include <vector>
#include <deque>

// Afinador en tiempo real: deteccion YIN + desplazamiento TD-PSOLA (conserva formantes).
//  - Modo Natural: corrige el "centro" de cada nota y conserva el vibrato y las
//    inflexiones (estilo afinacion transparente tipo editor de notas).
//  - Modo Hard: correccion instantanea nota por nota (efecto autotune clasico / T-Pain).
namespace nescrun
{
class PitchCorrector
{
public:
    enum Mode { Off = 0, Natural = 1, Hard = 2 };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        tMax = (int) std::ceil (sr / minF);
        latency = 3 * tMax + 64;
        ringSize = 1;
        while (ringSize < latency * 2 + 4 * tMax + 8) ringSize <<= 1;
        ringMask = ringSize - 1;
        in.assign ((size_t) ringSize, 0.f);
        acc.assign ((size_t) ringSize, 0.f);
        wsum.assign ((size_t) ringSize, 0.f);

        decim = sr > 70000.0 ? 4 : 2;
        dsr = sr / decim;
        yinW = 512;
        tauMin = std::max (2, (int) std::floor (dsr / maxF));
        tauMaxD = std::min ((int) std::ceil (dsr / minF), 1000);
        detLen = yinW + tauMaxD + 2;
        det.assign ((size_t) detLen, 0.f);
        diff.assign ((size_t) tauMaxD + 2, 0.f);
        detHop = 128;

        // Filtro anti-alias para el decimado (pasabajos ~ 4.5 kHz)
        lpCoef = juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, 4500.0, 0.707);
        lp1.coefficients = lpCoef; lp2.coefficients = lpCoef;
        lp1.reset(); lp2.reset();

        window.resize ((size_t) (2 * tMax + 2));
        reset();
    }

    void reset()
    {
        std::fill (in.begin(), in.end(), 0.f);
        std::fill (acc.begin(), acc.end(), 0.f);
        std::fill (wsum.begin(), wsum.end(), 0.f);
        std::fill (det.begin(), det.end(), 0.f);
        n = 0; nextMark = 0; nextSyn = 0; synFrac = 0.0; decimCount = 0; detCount = 0; detWrite = 0;
        marks.clear(); markHead = 0;
        voiced = false; period = (float) unvoicedPeriod();
        curShift = 0.f; desiredShift = 0.f; slowMidi = -1.f; targetNote = -1;
        detectedMidi.store (-1.f); correctionCents.store (0.f); targetMidi.store (-1);
    }

    int getLatency() const { return latency; }

    void setParams (int mode_, int key_, int scale_, float speedMs, float amount01, float human01)
    {
        mode = mode_; key = key_; scale = scale_;
        amount = amount01; humanize = human01;
        const float tauSec = std::max (0.0005f, speedMs * 0.001f);
        shiftCoef = (mode == Hard && speedMs < 1.f) ? 1.f : 1.f - std::exp (-1.f / (tauSec * (float) sr));
    }

    float processSample (float x)
    {
        in[(size_t) (n & ringMask)] = x;
        feedDetector (x);

        // suavizado de la correccion (velocidad de afinacion)
        curShift += (desiredShift - curShift) * shiftCoef;

        generateMarks();
        const int m = n - latency;
        spawnGrains (m);

        float y;
        if (m < 0) y = 0.f;
        else
        {
            const size_t i = (size_t) (m & ringMask);
            const float w = wsum[i];
            y = w > 1.0e-4f ? acc[i] / std::max (w, 0.35f) : 0.f;
            acc[i] = 0.f; wsum[i] = 0.f;
            if (mode == Off) y = in[i];
        }
        ++n;
        if (n > (1 << 30)) rebase();
        return y;
    }

    // para la interfaz
    std::atomic<float> detectedMidi { -1.f };
    std::atomic<float> correctionCents { 0.f };
    std::atomic<int>   targetMidi { -1 };

private:
    struct Mark { int pos; int half; bool voiced; float ratio; float exactT; };

    int unvoicedPeriod() const { return std::max (32, (int) (sr / 200.0)); }

    // --- deteccion de pitch -------------------------------------------------
    void feedDetector (float x)
    {
        const float f = lp2.processSample (lp1.processSample (x));
        if (++decimCount < decim) return;
        decimCount = 0;
        // buffer lineal deslizante
        det[(size_t) detWrite] = f;
        detWrite = (detWrite + 1) % detLen;
        if (++detCount >= detHop) { detCount = 0; runYin(); }
    }

    void runYin()
    {
        // copia ordenada
        lin.resize ((size_t) detLen);
        for (int i = 0; i < detLen; ++i) lin[(size_t) i] = det[(size_t) ((detWrite + i) % detLen)];

        float energy = 0.f;
        for (int i = detLen - yinW; i < detLen; ++i) energy += lin[(size_t) i] * lin[(size_t) i];
        const float rms = std::sqrt (energy / (float) yinW);

        bool ok = false; float tauEst = 0.f;
        if (rms > 0.0025f) // ~ -52 dBFS
        {
            const float* xw = lin.data() + (detLen - yinW - tauMaxD);
            diff[0] = 1.f;
            float running = 0.f;
            int best = -1;
            for (int tau = 1; tau <= tauMaxD; ++tau)
            {
                float s = 0.f;
                const float* a = xw + tauMaxD;
                const float* b = xw + tauMaxD - tau;
                for (int j = 0; j < yinW; ++j) { const float d = a[j] - b[j]; s += d * d; }
                running += s;
                diff[(size_t) tau] = running > 0.f ? s * (float) tau / running : 1.f;
            }
            for (int tau = tauMin; tau < tauMaxD; ++tau)
            {
                if (diff[(size_t) tau] < 0.15f)
                {
                    while (tau + 1 < tauMaxD && diff[(size_t) tau + 1] < diff[(size_t) tau]) ++tau;
                    best = tau; break;
                }
            }
            if (best > 0)
            {
                const float a = diff[(size_t) best - 1], b = diff[(size_t) best], c = diff[(size_t) best + 1];
                const float den = a + c - 2.f * b;
                const float off = std::abs (den) > 1e-9f ? 0.5f * (a - c) / den : 0.f;
                tauEst = (float) best + juce::jlimit (-0.5f, 0.5f, off);
                ok = true;
            }
        }

        if (ok)
        {
            const float freq = (float) dsr / tauEst;
            const float midi = 69.f + 12.f * std::log2 (freq / 440.f);
            // descarta saltos de octava espurios
            if (voiced && slowMidi > 0.f && std::abs (midi - slowMidi) > 10.f && voicedHops < 3) { ++voicedHops; return; }
            voiced = true; ++voicedHops;
            period = (float) (sr / freq);
            const float slowA = 0.25f; // ~ 25 ms por hop de 128@24k -> tiempo ~ 20-25 ms
            slowMidi = slowMidi < 0.f ? midi : slowMidi + (midi - slowMidi) * slowA;
            // referencia para elegir nota
            const float ref = (mode == Hard) ? midi : slowMidi;
            updateTarget (ref);
            float wanted;
            if (mode == Hard) wanted = (float) targetNote - midi;
            else
            {
                const float centre = humanize * slowMidi + (1.f - humanize) * midi;
                wanted = (float) targetNote - centre;
            }
            desiredShift = juce::jlimit (-12.f, 12.f, wanted * amount);
            if (mode == Off) desiredShift = 0.f;
            detectedMidi.store (midi);
            targetMidi.store (targetNote);
            correctionCents.store (desiredShift * 100.f);
        }
        else
        {
            voiced = false; voicedHops = 0;
            period = (float) unvoicedPeriod();
            desiredShift *= 0.6f; // vuelve suave a cero en consonantes / silencios
            slowMidi = -1.f; targetNote = -1;
            detectedMidi.store (-1.f);
            targetMidi.store (-1);
            correctionCents.store (0.f);
        }
    }

    bool inScale (int note) const
    {
        static const int masks[8] = {
            0xFFF,                                                   // chromatic
            (1<<0)|(1<<2)|(1<<4)|(1<<5)|(1<<7)|(1<<9)|(1<<11),       // major
            (1<<0)|(1<<2)|(1<<3)|(1<<5)|(1<<7)|(1<<8)|(1<<10),       // natural minor
            (1<<0)|(1<<2)|(1<<3)|(1<<5)|(1<<7)|(1<<8)|(1<<11),       // harmonic minor
            (1<<0)|(1<<3)|(1<<5)|(1<<7)|(1<<10),                     // pent minor
            (1<<0)|(1<<2)|(1<<4)|(1<<7)|(1<<9),                      // pent major
            (1<<0)|(1<<2)|(1<<3)|(1<<5)|(1<<7)|(1<<9)|(1<<10),       // dorian
            (1<<0)|(1<<1)|(1<<3)|(1<<5)|(1<<7)|(1<<8)|(1<<10)        // phrygian
        };
        const int deg = ((note - key) % 12 + 12) % 12;
        return (masks[juce::jlimit (0, 7, scale)] >> deg) & 1;
    }

    void updateTarget (float ref)
    {
        // histeresis: solo cambia de nota si la nueva esta claramente mas cerca
        if (targetNote >= 0 && std::abs (ref - (float) targetNote) < 0.62f && inScale (targetNote)) return;
        int bestN = (int) std::round (ref); float bestD = 1e9f;
        for (int c = (int) std::floor (ref) - 3; c <= (int) std::ceil (ref) + 3; ++c)
            if (inScale (c)) { const float d = std::abs (ref - (float) c); if (d < bestD) { bestD = d; bestN = c; } }
        targetNote = bestN;
    }

    // --- PSOLA -------------------------------------------------------------
    void generateMarks()
    {
        const int T = juce::jlimit (16, tMax, (int) std::round (period));
        const int q = T / 4;
        while (nextMark + q <= n)
        {
            int pos = nextMark;
            if (voiced)
            {   // refina la marca al pico mas cercano (sincronia con el ciclo glotal)
                float best = -1.f;
                for (int k = nextMark - q; k <= nextMark + q; ++k)
                {
                    const float v = std::abs (in[(size_t) (k & ringMask)]);
                    if (v > best) { best = v; pos = k; }
                }
                if (! marks.empty() && pos <= marks.back().pos + T / 2) pos = marks.back().pos + T;
            }
            const float ratio = std::pow (2.f, curShift / 12.f);
            marks.push_back ({ pos, T, voiced, voiced ? ratio : 1.f, voiced ? juce::jlimit (16.f, (float) tMax, period) : (float) T });
            nextMark = pos + T;
        }
        // limpia marcas viejas
        while (marks.size() > 2 && marks[1].pos < n - latency - 4 * tMax) marks.pop_front();
    }

    void spawnGrains (int m)
    {
        while (nextSyn <= m + tMax)
        {
            if (marks.empty()) { synFrac += unvoicedPeriod(); nextSyn = (int) std::llround (synFrac); continue; }
            // marca de analisis mas cercana al instante de sintesis
            const Mark* best = &marks.front(); int bestD = std::abs (best->pos - nextSyn);
            for (auto& mk : marks)
            {
                const int d = std::abs (mk.pos - nextSyn);
                if (d < bestD) { bestD = d; best = &mk; }
                if (mk.pos > nextSyn + tMax) break;
            }
            const int h = std::max (8, best->half);
            if (best->pos + h > n) break; // todavia no llegaron las muestras
            const float invH = 1.f / (float) h;
            for (int k = -h; k < h; ++k)
            {
                const float w = 0.5f + 0.5f * std::cos (juce::MathConstants<float>::pi * (float) k * invH);
                const size_t o = (size_t) ((nextSyn + k) & ringMask);
                acc[o]  += w * in[(size_t) ((best->pos + k) & ringMask)];
                wsum[o] += w;
            }
            const float r = juce::jlimit (0.5f, 2.f, best->ratio);
            synFrac += std::max (4.0, (double) best->exactT / (double) r);
            nextSyn = (int) std::llround (synFrac);
        }
    }

    void rebase()
    {
        // evita overflow luego de muchas horas: desplaza todos los indices
        const int shift = (n - ringSize * 4) & ~ringMask;
        n -= shift; nextMark -= shift; nextSyn -= shift; synFrac -= shift;
        for (auto& mk : marks) mk.pos -= shift;
    }

    double sr = 44100.0, dsr = 22050.0;
    const float minF = 70.f, maxF = 1100.f;
    int tMax = 630, latency = 1954, ringSize = 8192, ringMask = 8191;
    std::vector<float> in, acc, wsum, window;

    int decim = 2, decimCount = 0, yinW = 512, tauMin = 20, tauMaxD = 315, detLen = 830, detHop = 128, detCount = 0, detWrite = 0;
    std::vector<float> det, lin, diff;
    juce::dsp::IIR::Coefficients<float>::Ptr lpCoef;
    juce::dsp::IIR::Filter<float> lp1, lp2;

    int n = 0, nextMark = 0, nextSyn = 0; double synFrac = 0.0;
    std::deque<Mark> marks; int markHead = 0;
    bool voiced = false; int voicedHops = 0;
    float period = 220.f, slowMidi = -1.f;
    int targetNote = -1;
    float curShift = 0.f, desiredShift = 0.f, shiftCoef = 0.01f;

    int mode = Natural, key = 0, scale = 0;
    float amount = 1.f, humanize = 0.7f;
};
} // namespace nescrun
