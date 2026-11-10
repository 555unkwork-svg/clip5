// clip5 — utilidades comunes del motor DSP (sin dependencias de JUCE).
#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
 #define CLIP5_SSE 1
 #include <emmintrin.h>
 #include <xmmintrin.h>
#elif defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(_M_ARM64)
 #define CLIP5_NEON 1
 #include <arm_neon.h>
#endif

namespace clip5
{
constexpr double kPi = 3.14159265358979323846;
constexpr int kMaxChannels = 2;

inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float g, float floorDb = -200.0f) noexcept
{
    return g > 0.0f ? std::max (floorDb, 20.0f * std::log10 (g)) : floorDb;
}

/** Redondea hacia arriba al múltiplo de 4 (largo de vectores SIMD). */
constexpr int roundUp4 (int n) noexcept { return (n + 3) & ~3; }

/** Producto escalar de n floats (n múltiplo de 4). SSE2 / NEON / escalar. */
inline float dot4 (const float* a, const float* b, int n) noexcept
{
#if CLIP5_SSE
    __m128 acc0 = _mm_setzero_ps(), acc1 = _mm_setzero_ps();
    int i = 0;
    for (; i + 8 <= n; i += 8)
    {
        acc0 = _mm_add_ps (acc0, _mm_mul_ps (_mm_loadu_ps (a + i), _mm_loadu_ps (b + i)));
        acc1 = _mm_add_ps (acc1, _mm_mul_ps (_mm_loadu_ps (a + i + 4), _mm_loadu_ps (b + i + 4)));
    }
    for (; i < n; i += 4)
        acc0 = _mm_add_ps (acc0, _mm_mul_ps (_mm_loadu_ps (a + i), _mm_loadu_ps (b + i)));
    acc0 = _mm_add_ps (acc0, acc1);
    __m128 shuf = _mm_shuffle_ps (acc0, acc0, _MM_SHUFFLE (2, 3, 0, 1));
    __m128 sums = _mm_add_ps (acc0, shuf);
    shuf = _mm_movehl_ps (shuf, sums);
    sums = _mm_add_ss (sums, shuf);
    return _mm_cvtss_f32 (sums);
#elif CLIP5_NEON
    float32x4_t acc0 = vdupq_n_f32 (0.0f), acc1 = vdupq_n_f32 (0.0f);
    int i = 0;
    for (; i + 8 <= n; i += 8)
    {
        acc0 = vmlaq_f32 (acc0, vld1q_f32 (a + i), vld1q_f32 (b + i));
        acc1 = vmlaq_f32 (acc1, vld1q_f32 (a + i + 4), vld1q_f32 (b + i + 4));
    }
    for (; i < n; i += 4)
        acc0 = vmlaq_f32 (acc0, vld1q_f32 (a + i), vld1q_f32 (b + i));
    acc0 = vaddq_f32 (acc0, acc1);
   #if defined(__aarch64__) || defined(_M_ARM64)
    return vaddvq_f32 (acc0);
   #else
    float32x2_t s = vadd_f32 (vget_low_f32 (acc0), vget_high_f32 (acc0));
    return vget_lane_f32 (vpadd_f32 (s, s), 0);
   #endif
#else
    float s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    for (int i = 0; i < n; i += 4)
    {
        s0 += a[i] * b[i]; s1 += a[i + 1] * b[i + 1];
        s2 += a[i + 2] * b[i + 2]; s3 += a[i + 3] * b[i + 3];
    }
    return (s0 + s1) + (s2 + s3);
#endif
}

/** Cuatro salidas a la vez: out[p] = Σ_j cT[j][p]·w[j] (coeficientes "transpuestos" [taps][4]).
    Calcula las 4 fases de un interpolador 4x en un solo vector, sin sumas horizontales. */
inline void mac4 (const float* cT, const float* w, int taps, float* out) noexcept
{
#if CLIP5_SSE
    __m128 a0 = _mm_setzero_ps(), a1 = _mm_setzero_ps();
    int j = 0;
    for (; j + 2 <= taps; j += 2)
    {
        a0 = _mm_add_ps (a0, _mm_mul_ps (_mm_load_ps (cT + 4 * j), _mm_set1_ps (w[j])));
        a1 = _mm_add_ps (a1, _mm_mul_ps (_mm_load_ps (cT + 4 * j + 4), _mm_set1_ps (w[j + 1])));
    }
    for (; j < taps; ++j) a0 = _mm_add_ps (a0, _mm_mul_ps (_mm_load_ps (cT + 4 * j), _mm_set1_ps (w[j])));
    _mm_storeu_ps (out, _mm_add_ps (a0, a1));
#elif CLIP5_NEON
    float32x4_t a0 = vdupq_n_f32 (0.0f), a1 = vdupq_n_f32 (0.0f);
    int j = 0;
    for (; j + 2 <= taps; j += 2)
    {
        a0 = vmlaq_n_f32 (a0, vld1q_f32 (cT + 4 * j), w[j]);
        a1 = vmlaq_n_f32 (a1, vld1q_f32 (cT + 4 * j + 4), w[j + 1]);
    }
    for (; j < taps; ++j) a0 = vmlaq_n_f32 (a0, vld1q_f32 (cT + 4 * j), w[j]);
    vst1q_f32 (out, vaddq_f32 (a0, a1));
#else
    float o0 = 0, o1 = 0, o2 = 0, o3 = 0;
    for (int j = 0; j < taps; ++j)
    {
        o0 += cT[4 * j] * w[j]; o1 += cT[4 * j + 1] * w[j];
        o2 += cT[4 * j + 2] * w[j]; o3 += cT[4 * j + 3] * w[j];
    }
    out[0] = o0; out[1] = o1; out[2] = o2; out[3] = o3;
#endif
}

/** Historial de una línea FIR con doble escritura: la ventana de los últimos `len`
    valores es siempre contigua en memoria (sin módulo en el bucle interno).
    `window()` devuelve el más antiguo primero → los coeficientes se guardan invertidos. */
template <int Cap>
struct FirHistory
{
    alignas (16) float buf[2 * Cap] {};
    int len = Cap, pos = 0;

    void setLength (int n) noexcept { len = std::min (n, Cap); reset(); }
    void reset() noexcept { std::memset (buf, 0, sizeof (buf)); pos = 0; }
    void push (float x) noexcept
    {
        buf[pos] = x;
        buf[pos + len] = x;
        if (++pos == len) pos = 0;
    }
    const float* window() const noexcept { return buf + pos; } // [más antiguo … más reciente]
    float newest (int k) const noexcept // k = 0 → la última muestra empujada
    {
        int i = pos - 1 - k;
        while (i < 0) i += len;
        return buf[i];
    }
};

/** Rampa lineal por bloque para parámetros continuos. */
struct LinearRamp
{
    float current = 0.0f, target = 0.0f, step = 0.0f;
    int remaining = 0;

    void reset (float v) noexcept { current = target = v; step = 0; remaining = 0; }
    void setTarget (float v, int rampSamples) noexcept
    {
        if (v == target && remaining == 0 && current == v) return;
        target = v;
        remaining = std::max (1, rampSamples);
        step = (target - current) / (float) remaining;
    }
    float next() noexcept
    {
        if (remaining > 0)
        {
            current += step;
            if (--remaining == 0) current = target;
        }
        return current;
    }
    bool isSmoothing() const noexcept { return remaining > 0; }
};

/** Activa flush-to-zero / denormals-are-zero mientras vive el objeto. */
struct ScopedFlushDenormals
{
#if CLIP5_SSE
    unsigned int prev;
    ScopedFlushDenormals() noexcept : prev (_mm_getcsr()) { _mm_setcsr (prev | 0x8040); }
    ~ScopedFlushDenormals() { _mm_setcsr (prev); }
#else
    ScopedFlushDenormals() noexcept {}
#endif
};
} // namespace clip5
