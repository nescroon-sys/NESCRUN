#pragma once
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <atomic>

// "Vocal EQ AI": supresor dinamico de resonancias en el dominio espectral.
// En cada cuadro STFT estima la envolvente espectral "sana" de la voz y atenua
// solo los picos estrechos que sobresalen (resonancias de la sala, del mic,
// silbidos nasales, durezas de 2-5 kHz) de forma dinamica: si la resonancia
// aparece, se corta; si no, no toca nada.
namespace nescrun
{
class ResonanceSuppressor
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        order = sr > 70000.0 ? 11 : 10;
        N = 1 << order; hop = N / 4;
        fft = std::make_unique<juce::dsp::FFT> (order);
        win.resize ((size_t) N);
        for (int i = 0; i < N; ++i)
            win[(size_t) i] = std::sqrt (0.5f - 0.5f * std::cos (2.f * juce::MathConstants<float>::pi * (float) i / (float) N));
        inBuf.assign ((size_t) N, 0.f);
        outBuf.assign ((size_t) (2 * N), 0.f);
        frame.assign ((size_t) (2 * N), 0.f);
        const int bins = N / 2 + 1;
        magDb.assign ((size_t) bins, 0.f); env.assign ((size_t) bins, 0.f);
        gainDb.assign ((size_t) bins, 0.f); prefix.assign ((size_t) bins + 1, 0.f);
        weight.assign ((size_t) bins, 0.f);
        for (int k = 0; k < bins; ++k)
        {
            const float f = (float) k * (float) sr / (float) N;
            // zona de trabajo 150 Hz - 15 kHz, con mas sensibilidad en 1.5-6 kHz (dureza)
            float w = 0.f;
            if (f > 150.f && f < 15000.f)
            {
                w = 0.55f;
                if (f > 1500.f && f < 6500.f) w = 1.f;
                else if (f > 800.f && f < 9000.f) w = 0.8f;
            }
            weight[(size_t) k] = w;
        }
        inPos = 0; hopCount = 0;
        reduction.store (0.f);
    }

    int getLatency() const { return N; }

    void setParams (float amount01, float focus01) { amount = amount01; focus = focus01; }

    float processSample (float x)
    {
        // buffer de entrada deslizante circular
        inBuf[(size_t) inPos] = x;
        inPos = (inPos + 1) % N;
        const float y = outBuf[(size_t) hopCount];
        if (++hopCount >= hop)
        {
            hopCount = 0;
            processFrame();
        }
        return y;
    }

    std::atomic<float> reduction { 0.f }; // dB promedio, para el medidor

private:
    void processFrame()
    {
        const int bins = N / 2 + 1;
        for (int i = 0; i < N; ++i)
            frame[(size_t) i] = inBuf[(size_t) ((inPos + i) % N)] * win[(size_t) i];
        std::fill (frame.begin() + N, frame.end(), 0.f);
        fft->performRealOnlyForwardTransform (frame.data(), true);

        for (int k = 0; k < bins; ++k)
        {
            const float re = frame[(size_t) (2 * k)], im = frame[(size_t) (2 * k + 1)];
            magDb[(size_t) k] = 10.f * std::log10 (re * re + im * im + 1e-12f);
        }
        // envolvente: promedio movil de ancho proporcional a la frecuencia (~1/3 oct)
        prefix[0] = 0.f;
        for (int k = 0; k < bins; ++k) prefix[(size_t) k + 1] = prefix[(size_t) k] + magDb[(size_t) k];
        const float widthFrac = 0.10f + 0.12f * (1.f - focus);
        for (int k = 0; k < bins; ++k)
        {
            const int hw = std::max (3, (int) ((float) k * widthFrac));
            const int a = std::max (0, k - hw), b = std::min (bins - 1, k + hw);
            env[(size_t) k] = (prefix[(size_t) b + 1] - prefix[(size_t) a]) / (float) (b - a + 1);
        }

        const float thresh = 9.f - 5.f * focus;  // dB por encima de la envolvente
        const float maxCut = 3.f + 9.f * amount; // hasta -12 dB
        const float slope = 0.35f + 0.65f * amount;
        float sumRed = 0.f; int cnt = 0;
        float peak = -200.f; for (int k = 0; k < bins; ++k) peak = std::max (peak, magDb[(size_t) k]);

        for (int k = 0; k < bins; ++k)
        {
            float target = 0.f;
            const float ex = magDb[(size_t) k] - env[(size_t) k] - thresh;
            if (ex > 0.f && magDb[(size_t) k] > peak - 60.f)
                target = -std::min (maxCut, ex * slope) * weight[(size_t) k];
            float& g = gainDb[(size_t) k];
            g = target < g ? g + (target - g) * 0.7f : g + (target - g) * 0.12f; // ataque rapido, release suave
            if (weight[(size_t) k] > 0.f) { sumRed += g; ++cnt; }
        }
        // suaviza la curva de ganancia entre bins vecinos (evita artefactos)
        for (int pass = 0; pass < 2; ++pass)
        {
            float prev = gainDb[0];
            for (int k = 1; k < bins - 1; ++k)
            {
                const float cur = gainDb[(size_t) k];
                gainDb[(size_t) k] = 0.25f * prev + 0.5f * cur + 0.25f * gainDb[(size_t) k + 1];
                prev = cur;
            }
        }
        reduction.store (cnt > 0 ? sumRed / (float) cnt * 4.f : 0.f);

        for (int k = 0; k < bins; ++k)
        {
            const float g = amount > 0.001f ? juce::Decibels::decibelsToGain (gainDb[(size_t) k]) : 1.f;
            frame[(size_t) (2 * k)] *= g; frame[(size_t) (2 * k + 1)] *= g;
        }
        fft->performRealOnlyInverseTransform (frame.data());

        // overlap-add: sqrt-hann * sqrt-hann = hann, con hop N/4 suma 2 -> escala 0.5
        for (int i = 0; i < N - hop; ++i) outBuf[(size_t) i] = outBuf[(size_t) (i + hop)];
        for (int i = N - hop; i < N; ++i) outBuf[(size_t) i] = 0.f;
        for (int i = 0; i < N; ++i) outBuf[(size_t) i] += frame[(size_t) i] * win[(size_t) i] * 0.5f;
    }

    double sr = 44100.0;
    int order = 10, N = 1024, hop = 256, inPos = 0, hopCount = 0;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> win, inBuf, outBuf, frame, magDb, env, gainDb, prefix, weight;
    float amount = 0.5f, focus = 0.5f;
};
} // namespace nescrun
