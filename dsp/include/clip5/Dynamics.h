// clip5 — Suavizado (contorno dinámico de agudos), Techo estricto (true peak) y bloqueo de DC.
#pragma once

#include "clip5/Common.h"
#include "clip5/FilterDesign.h"

#include <vector>

namespace clip5
{
/** Suavizado: corte dinámico de medios-agudos que solo actúa cerca del techo.
    · Detector: |u| pre-recorte (ya normalizado al techo), ataque instantáneo, release exponencial.
    · Filtro: SVF TPT pasa-altos FIJO; salida = x + (G − 1)·HP(x). Modular G no toca los
      coeficientes del filtro → estable con cualquier modulación, sin pow/tan por muestra. */
struct Suavizado
{
    // estado
    float ic1 = 0, ic2 = 0, env = 0;
    // coeficientes (se fijan en prepare)
    float g = 0, k = 1.41421356f, a1 = 0, a2 = 0, a3 = 0, release = 0.999f;

    void prepare (double fsOs, double fc = 3000.0, double releaseMs = 15.0)
    {
        g = (float) std::tan (kPi * fc / fsOs);
        k = 1.41421356f; // Q = 0.707
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
        release = (float) std::exp (-1.0 / (releaseMs * 1e-3 * fsOs));
        reset();
    }
    void reset() noexcept { ic1 = ic2 = env = 0; }

    /** depth0 → depth1: corte máximo (dB) al inicio y al final del bloque (rampa lineal).
        uAbs: |u| por muestra. Apagado (0 → 0): no procesa; al reactivarse arranca desde estado
        cero con profundidad 0, así que no hay saltos. */
    void process (float* y, const float* uAbs, int n, float depth0, float depth1) noexcept
    {
        if (depth0 <= 0.0f && depth1 <= 0.0f)
        {
            if (active) { reset(); active = false; }
            return;
        }
        active = true;
        constexpr float kLn = 0.115129255f; // ln(10)/20
        const float dStep = (depth1 - depth0) / (float) n;
        float depth = depth0;
        for (int i = 0; i < n; ++i)
        {
            env = std::max (uAbs[i], env * release);
            // acción: 0 por debajo de −6 dB re techo, 1 desde +3 dB re techo (smoothstep)
            float t = (env - 0.5012f) * (1.0f / (1.4125f - 0.5012f));
            t = std::min (1.0f, std::max (0.0f, t));
            const float s = t * t * (3.0f - 2.0f * t);
            depth += dStep;
            const float c = kLn * depth * s;                                   // nepers
            const float G = 1.0f - c + c * c * 0.5f - c * c * c * (1.0f / 6.0f); // ≈ e^{-c}
            const float hp = tick (y[i]);
            y[i] += (G - 1.0f) * hp;
        }
    }

    bool active = false;

private:
    float tick (float x) noexcept
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        return x - k * v1 - v2; // pasa-altos
    }
};

/** Bloqueo de DC suave (solo modo Suave, cuya asimetría genera DC). y = x − w·LP(x). */
struct DcBlocker
{
    float lp = 0, coef = 0, weight = 0, weightTarget = 0, weightStep = 0;
    void prepare (double fs, double fc = 3.0)
    {
        coef = (float) (1.0 - std::exp (-2.0 * kPi * fc / fs));
        weightStep = (float) (1.0 / (0.05 * fs)); // 50 ms de transición
        lp = 0; weight = weightTarget;
    }
    void reset() noexcept { lp = 0; weight = weightTarget; }
    void process (float* x, int n) noexcept
    {
        if (weight == 0.0f && weightTarget == 0.0f)
        {
            for (int i = 0; i < n; ++i) lp += coef * (x[i] - lp);
            return;
        }
        for (int i = 0; i < n; ++i)
        {
            lp += coef * (x[i] - lp);
            if (weight < weightTarget) weight = std::min (weightTarget, weight + weightStep);
            else if (weight > weightTarget) weight = std::max (weightTarget, weight - weightStep);
            x[i] -= weight * lp;
        }
    }
};

/** Detector de true peak: interpolador 4x (128 taps, 32 por fase) + refinamiento parabólico del
    máximo local entre puntos 4x (equivale a sobremuestrear mucho más, casi sin costo).
    Por cada muestra de entrada devuelve el pico estimado del intervalo (n−16, n−15]. */
struct TruePeakDetector
{
    static constexpr int kTaps = 32;
    static constexpr int kDelay = 16; // muestras base
    alignas (16) float coefT[kTaps][4] {}; // [tap (más antiguo primero)][fase]
    FirHistory<kTaps> h;
    float z2 = 0, z3 = 0; // dos últimos valores 4x del intervalo anterior

    void prepare()
    {
        const auto c = design::truePeakInterpolator();
        for (int p = 0; p < 4; ++p)
            for (int k = 0; k < kTaps; ++k)
                coefT[kTaps - 1 - k][p] = (float) c[(size_t) (4 * k + p)];
        h.setLength (kTaps);
        reset();
    }
    void reset() noexcept { h.reset(); z2 = z3 = 0; }

    /** `skipBelow`: si el máximo de los 4 puntos ×1,07 queda por debajo, se omite el refinamiento
        parabólico (la parábola entre puntos 4x no puede subir más de ~7 % para f ≤ 0,47·fs). */
    float push (float x, float skipBelow = 0.0f) noexcept
    {
        h.push (x);
        alignas (16) float z[8];
        z[0] = z2; z[1] = z3;
        mac4 (&coefT[0][0], h.window(), kTaps, z + 2);
        float m = 0.0f;
        for (int p = 2; p < 6; ++p) m = std::max (m, std::abs (z[p]));
        m = std::max (m, std::abs (z[1]));
        if (m * 1.07f < skipBelow) { z2 = z[4]; z3 = z[5]; return m; }
        // vértice de la parábola por los máximos locales (z[1] … z[4] tienen ambos vecinos)
        for (int k = 1; k <= 4; ++k)
        {
            const float s = z[k] >= 0.0f ? 1.0f : -1.0f;
            const float A = s * z[k - 1], B = s * z[k], C = s * z[k + 1];
            if (B >= A && B >= C)
            {
                const float den = A - 2.0f * B + C;
                if (den < -1e-12f)
                {
                    const float pp = 0.5f * (A - C) / den;
                    if (std::abs (pp) <= 0.5f) m = std::max (m, B - 0.25f * (A - C) * pp);
                }
            }
        }
        z2 = z[4]; z3 = z[5];
        return m;
    }
};

/** Techo estricto: limitador con lookahead guiado por true peak (4x + parábola).
    Garantía por construcción: g[n] ≤ techo / pico(n). Mínimo deslizante de W+1 requisitos seguido
    de dos promedios en cascada (W/2 y W/2+1 → núcleo triangular de soporte W): cada término del
    promedio es un mínimo que incluye la muestra, así que el promedio no la supera. El núcleo
    triangular reduce la modulación de la ganancia (menos sobretiro entre muestras). El release es
    exponencial y solo puede bajar la ganancia (min), por lo que conserva la garantía.
    Latencia fija = W + 16 muestras (también cuando está desactivado: la latencia no cambia). */
class StrictCeiling
{
public:
    static constexpr int kMaxLookahead = 512;
    static constexpr int kRing = 1024;

    void prepare (double fs, int numChannels)
    {
        W = std::min (kMaxLookahead - 8, std::max (8, (int) std::lround (0.0015 * fs)));
        W1 = W / 2;
        W2 = W - W1 + 1;
        delay = W + TruePeakDetector::kDelay;
        nch = numChannels;
        releaseCoef = (float) (1.0 - std::exp (-1.0 / (0.060 * fs)));
        for (auto& d : tp) d.prepare();
        reset();
    }

    void reset()
    {
        for (auto& d : tp) d.reset();
        std::memset (sig, 0, sizeof (sig));
        for (int c = 0; c < kMaxChannels; ++c) lastIval[c] = 0;
        std::fill (std::begin (req), std::end (req), 1.0f);
        dqHead = dqTail = 0;
        sum1 = (double) W1; sum2 = (double) W2;
        for (int i = 0; i < kRing; ++i) { mins[i] = 1.0f; avg1[i] = 1.0f; peakRing[i] = 0.0f; }
        gRel = 1.0f;
        pos = 0;
        minGain = 1.0f;
    }

    int latency() const noexcept { return delay; }

    /** ceilingLin: techo lineal absoluto. enabled = false → solo retardo. */
    void process (float* const* ch, int n, float ceilingLin, bool enabled) noexcept
    {
        const float target = ceilingLin * kMargin;
        minGain = 1.0f;
        outTp = 0.0f;
        float blockPeak = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            // 1) requisito para la muestra k = t − 6 a partir del TP de los dos intervalos vecinos
            float peak = 0.0f;
            for (int c = 0; c < nch; ++c)
            {
                const float x = ch[c][i];
                const float ival = tp[c].push (x, std::min (target, blockPeak)); // intervalo (k, k+1]
                peak = std::max (peak, std::max (ival, lastIval[c]));
                lastIval[c] = ival;
                sig[c][pos & (kRing - 1)] = x;
            }
            // |x[k]| exacto
            for (int c = 0; c < nch; ++c)
                peak = std::max (peak, std::abs (sig[c][(pos - (uint32_t) TruePeakDetector::kDelay) & (kRing - 1)]));

            peakRing[pos & (kRing - 1)] = peak;
            blockPeak = std::max (blockPeak, peak);
            const float r = (enabled && peak > target) ? target / peak : 1.0f;

            // 2) mínimo deslizante sobre W+1 requisitos (deque monótona)
            pushMin (r);
            const float m = req[dqHead & (kRing - 1)];

            // 3) promedio triangular (dos promedios en cascada) de los mínimos
            sum1 += (double) m - (double) mins[(pos - (uint32_t) W1) & (kRing - 1)];
            mins[pos & (kRing - 1)] = m;
            const float a = (float) (sum1 / (double) W1);
            sum2 += (double) a - (double) avg1[(pos - (uint32_t) W2) & (kRing - 1)];
            avg1[pos & (kRing - 1)] = a;
            const float g = std::min (1.0f, (float) (sum2 / (double) W2));

            // 4) release exponencial que solo puede bajar
            gRel = std::min (g, gRel + releaseCoef * (1.0f - gRel));
            minGain = std::min (minGain, gRel);
            // true peak de la salida (medición): ganancia × pico ya detectado del intervalo de salida
            outTp = std::max (outTp, gRel * peakRing[(pos - (uint32_t) W) & (kRing - 1)]);

            // 5) salida retardada
            const uint32_t rd = (pos - (uint32_t) delay) & (kRing - 1);
            for (int c = 0; c < nch; ++c) ch[c][i] = sig[c][rd] * gRel;
            ++pos;
        }
    }

    float lastMinGain() const noexcept { return minGain; }
    /** True peak (4x + parábola) de la salida durante la última llamada a process(). */
    float lastTruePeak() const noexcept { return outTp; }

private:
    static constexpr float kMargin = 0.9977f; // −0.02 dB de margen frente a medidores TP distintos

    int W = 72, W1 = 36, W2 = 37, delay = 84, nch = 2;
    uint32_t pos = 0; // aritmética módulo 2^32: sin desbordes en sesiones largas
    TruePeakDetector tp[kMaxChannels];
    float lastIval[kMaxChannels] {};
    float sig[kMaxChannels][kRing] {};

    // deque monótona de mínimos (índices y valores)
    float req[kRing] {};
    uint32_t reqIdx[kRing] {};
    uint32_t dqHead = 0, dqTail = 0;
    float mins[kRing] {}, avg1[kRing] {}, peakRing[kRing] {};
    double sum1 = 36.0, sum2 = 37.0;
    float gRel = 1.0f, releaseCoef = 0.0f, minGain = 1.0f, outTp = 0.0f;

    void pushMin (float r) noexcept
    {
        while (dqTail != dqHead && req[(dqTail - 1) & (kRing - 1)] >= r) --dqTail;
        req[dqTail & (kRing - 1)] = r;
        reqIdx[dqTail & (kRing - 1)] = pos;
        ++dqTail;
        while ((int32_t) (pos - reqIdx[dqHead & (kRing - 1)]) > W) ++dqHead;
    }
};
} // namespace clip5
