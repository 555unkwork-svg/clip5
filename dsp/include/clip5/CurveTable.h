// clip5 — tabla por tramos cúbicos de la curva fusionada + antiderivada exacta + ADAA.
//
// · 512 tramos uniformes en [-2, 2] (h = 1/128, potencia de 2 → u/h es exacto en float).
// · Fuera de [-2, 2] la curva es exactamente lineal (pendiente 0 o 1): extrapolación exacta.
// · F1 anclada en u = 0 (F1(0) = 0) para no perder precisión con señales de bajo nivel.
// · ADAA "por tramo": si u y u₋₁ caen en el mismo tramo, la media del polinomio se evalúa
//   factorizada (sin restas que cancelen, sin ε). Tramos vecinos: se parte en el nodo.
//   Solo con saltos ≥ 1 tramo se usa (F1(u) − F1(u₋₁)) / (u − u₋₁).
#pragma once

#include "clip5/Common.h"
#include "clip5/Curves.h"

namespace clip5
{
struct CurveTable
{
    static constexpr int kSeg = 512;
    static constexpr int kHalf = kSeg / 2;
    static constexpr float kLo = -2.0f, kHi = 2.0f;
    static constexpr float kInvH = 128.0f;
    static constexpr float kH = 1.0f / 128.0f;
    static constexpr float kMaxAbsInput = 4096.0f;

    alignas (16) float c[kSeg][4] {};
    float P[kSeg + 1] {};
    float fLo = -1.0f, fHi = 1.0f, slope = 0.0f;
    CurveParams params {};

    /** Genera la tabla (sin allocations; apto para hilo de fondo o tiempo real offline). */
    void build (const CurveParams& p) noexcept;

    float eval (float u) const noexcept
    {
        u = clampInput (u);
        const float q = u * kInvH;
        const int fi = fastFloor (q);
        const float fq = (float) fi;
        const int i = fi + kHalf;
        if (i < 0) return fLo + slope * (u - kLo);
        if (i >= kSeg) return fHi + slope * (u - kHi);
        const float t = q - fq;
        const float* k = c[i];
        return k[0] + t * (k[1] + t * (k[2] + t * k[3]));
    }

    float antiderivative (float u) const noexcept
    {
        u = clampInput (u);
        const float q = u * kInvH;
        const int fi = fastFloor (q);
        const float fq = (float) fi;
        const int i = fi + kHalf;
        if (i < 0)
        {
            const float d = u - kLo;
            return P[0] + d * (fLo + 0.5f * slope * d);
        }
        if (i >= kSeg)
        {
            const float d = u - kHi;
            return P[kSeg] + d * (fHi + 0.5f * slope * d);
        }
        const float t = q - fq;
        const float* k = c[i];
        return P[i] + kH * t * (k[0] + t * (k[1] * 0.5f + t * (k[2] * (1.0f / 3.0f) + t * k[3] * 0.25f)));
    }

    /** ADAA de 1.er orden: media de f sobre [u₋₁, u]. */
    float adaa (float u, float u1) const noexcept
    {
        u = clampInput (u);
        u1 = clampInput (u1);
        const float q = u * kInvH, q1 = u1 * kInvH;
        const int fi = fastFloor (q), fj = fastFloor (q1);
        const float fq = (float) fi, fq1 = (float) fj;
        const int i = fi + kHalf, j = fj + kHalf;

        if (i == j)
            return meanInSegment (i, q - fq, q1 - fq1, u, u1);

        const int d = i - j;
        if (d == 1 || d == -1)
        {
            const int kNode = std::max (i, j);                 // nodo compartido
            const float b = (float) (kNode - kHalf) * kH;
            // tramo de u1 y tramo de u, cada uno con su media exacta
            const float m1 = meanInSegment (j, q1 - fq1, (j < i) ? 1.0f : 0.0f, u1, b);
            const float m2 = meanInSegment (i, (i > j) ? 0.0f : 1.0f, q - fq, b, u);
            const float w = (b - u1) / (u - u1);
            return w * m1 + (1.0f - w) * m2;
        }
        return (antiderivative (u) - antiderivative (u1)) / (u - u1);
    }

private:
    /** floor() rápido: truncamiento + corrección (std::floor sin SSE4.1 es una llamada a libm). */
    static int fastFloor (float q) noexcept
    {
        const int i = (int) q;
        return i - (q < (float) i ? 1 : 0);
    }

    static float clampInput (float u) noexcept
    {
        // también neutraliza NaN (std::max/min con NaN devuelve el primer argumento)
        return std::min (kMaxAbsInput, std::max (-kMaxAbsInput, u == u ? u : 0.0f));
    }

    /** Media de f entre dos puntos del mismo tramo (coordenadas locales t, s ∈ [0,1]). */
    float meanInSegment (int k, float t, float s, float ua, float ub) const noexcept
    {
        if (k < 0) return fLo + slope * (0.5f * (ua + ub) - kLo);
        if (k >= kSeg) return fHi + slope * (0.5f * (ua + ub) - kHi);
        const float* cc = c[k];
        const float ts = t + s, t2s2 = t * t + s * s;
        return cc[0] + cc[1] * 0.5f * ts + cc[2] * (1.0f / 3.0f) * (t2s2 + t * s) + cc[3] * 0.25f * ts * t2s2;
    }
};
} // namespace clip5
