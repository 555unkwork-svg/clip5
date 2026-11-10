// clip5 — estados de filtros en tiempo real (sin allocations; los coeficientes los posee el diseño).
#pragma once

#include "clip5/Common.h"
#include "clip5/FilterDesign.h"

#include <vector>

namespace clip5
{
constexpr int kHistCap = 256;

/** Coeficientes listos para dot4: invertidos (más antiguo primero) y rellenos a múltiplo de 4. */
struct FirKernel
{
    std::vector<float> rev;
    int len = 0; // múltiplo de 4

    void setFromTaps (const std::vector<double>& taps, double gain = 1.0)
    {
        const int n = (int) taps.size();
        len = roundUp4 (std::max (1, n));
        rev.assign ((size_t) len, 0.0f);
        for (int k = 0; k < n; ++k) rev[(size_t) (len - 1 - k)] = (float) (taps[(size_t) k] * gain);
    }
};

/** Half-band FIR (4m+3 taps) descompuesto en fases para sobremuestreo 2x. */
struct HalfbandKernels
{
    FirKernel up, down; // fase par (×2 para subir), fase par (×1 para bajar)
    int m = 0, taps = 0;

    void design (const std::vector<double>& h)
    {
        taps = (int) h.size();
        m = (taps - 3) / 4;
        std::vector<double> even;
        for (int i = 0; i < taps; i += 2) even.push_back (h[(size_t) i]);
        up.setFromTaps (even, 2.0);
        down.setFromTaps (even, 1.0);
    }
    double delayHigh() const noexcept { return (taps - 1) / 2.0; }
};

struct HalfbandFirUp
{
    const HalfbandKernels* k = nullptr;
    FirHistory<kHistCap> h;

    void bind (const HalfbandKernels* kk) noexcept { k = kk; h.setLength (std::max (kk->up.len, kk->m + 1)); }
    void reset() noexcept { h.reset(); }
    void process (const float* in, int n, float* out) noexcept
    {
        const int len = k->up.len, m = k->m;
        const float* c = k->up.rev.data();
        for (int i = 0; i < n; ++i)
        {
            h.push (in[i]);
            out[2 * i] = dot4 (c, h.window() + (h.len - len), len);
            out[2 * i + 1] = h.newest (m);
        }
    }
};

struct HalfbandFirDown
{
    const HalfbandKernels* k = nullptr;
    FirHistory<kHistCap> he, ho;

    void bind (const HalfbandKernels* kk) noexcept
    {
        k = kk;
        he.setLength (kk->down.len);
        ho.setLength (kk->m + 2);
    }
    void reset() noexcept { he.reset(); ho.reset(); }
    void process (const float* in, int n, float* out) noexcept // in: 2n muestras
    {
        const int len = k->down.len, m = k->m;
        const float* c = k->down.rev.data();
        for (int i = 0; i < n; ++i)
        {
            he.push (in[2 * i]);
            out[i] = dot4 (c, he.window(), len) + 0.5f * ho.newest (m);
            ho.push (in[2 * i + 1]);
        }
    }
};

/** Decimador 2:1 con FIR completo (Tono + compensación del ADAA + antialias) y crossfade
    entre dos juegos de coeficientes que comparten el mismo historial. */
struct DecimKernels
{
    FirKernel even, odd;
    void design (const std::vector<double>& g)
    {
        std::vector<double> e, o;
        for (size_t i = 0; i < g.size(); ++i) (i % 2 == 0 ? e : o).push_back (g[i]);
        even.setFromTaps (e);
        odd.setFromTaps (o);
    }
};

struct FirDecimator2
{
    const DecimKernels* a = nullptr;
    const DecimKernels* b = nullptr; // destino del crossfade (o nullptr)
    FirHistory<kHistCap> he, ho;

    void bind (const DecimKernels* k) noexcept
    {
        a = k; b = nullptr;
        he.setLength (k->even.len);
        ho.setLength (k->odd.len);
    }
    void reset() noexcept { he.reset(); ho.reset(); }

    /** fadeStart/fadeStep: peso de `b` (0 → 1). Sin crossfade, b == nullptr. */
    void process (const float* in, int n, float* out, float fade = 0.0f, float fadeStep = 0.0f) noexcept
    {
        const int le = a->even.len, lo = a->odd.len;
        for (int i = 0; i < n; ++i)
        {
            he.push (in[2 * i]);
            float y = dot4 (a->even.rev.data(), he.window(), le) + dot4 (a->odd.rev.data(), ho.window(), lo);
            if (b != nullptr)
            {
                const float yb = dot4 (b->even.rev.data(), he.window(), le) + dot4 (b->odd.rev.data(), ho.window(), lo);
                y += fade * (yb - y);
                fade += fadeStep;
            }
            out[i] = y;
            ho.push (in[2 * i + 1]);
        }
    }
};

struct FirFilter
{
    const FirKernel* a = nullptr;
    const FirKernel* b = nullptr;
    FirHistory<kHistCap> h;

    void bind (const FirKernel* k) noexcept { a = k; b = nullptr; h.setLength (k->len); }
    void reset() noexcept { h.reset(); }
    void process (const float* in, int n, float* out, float fade = 0.0f, float fadeStep = 0.0f) noexcept
    {
        const int len = a->len;
        for (int i = 0; i < n; ++i)
        {
            h.push (in[i]);
            float y = dot4 (a->rev.data(), h.window(), len);
            if (b != nullptr)
            {
                const float yb = dot4 (b->rev.data(), h.window(), len);
                y += fade * (yb - y);
                fade += fadeStep;
            }
            out[i] = y;
        }
    }
};

/** Half-band IIR polifásico (dos cadenas allpass de 1.er orden en z², fase mínima). */
constexpr int kMaxIirCoefs = 20;
struct IirHalfbandCoefs
{
    float a[kMaxIirCoefs] {};
    int n = 0;
    void set (const std::vector<double>& c)
    {
        n = std::min ((int) c.size(), kMaxIirCoefs);
        for (int i = 0; i < n; ++i) a[i] = (float) c[(size_t) i];
    }
};

struct IirAllpassChains
{
    const IirHalfbandCoefs* c = nullptr;
    float x1[kMaxIirCoefs] {}, y1[kMaxIirCoefs] {};

    void reset() noexcept { std::memset (x1, 0, sizeof (x1)); std::memset (y1, 0, sizeof (y1)); }
    float path (float s, int first) noexcept
    {
        for (int i = first; i < c->n; i += 2)
        {
            const float y = c->a[i] * (s - y1[i]) + x1[i];
            x1[i] = s;
            y1[i] = y;
            s = y;
        }
        return s;
    }
};

struct HalfbandIirUp
{
    IirAllpassChains ch;
    void bind (const IirHalfbandCoefs* c) noexcept { ch.c = c; reset(); }
    void reset() noexcept { ch.reset(); }
    void process (const float* in, int n, float* out) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            out[2 * i] = ch.path (in[i], 0);
            out[2 * i + 1] = ch.path (in[i], 1);
        }
    }
};

struct HalfbandIirDown
{
    IirAllpassChains ch;
    float prevOdd = 0.0f;
    void bind (const IirHalfbandCoefs* c) noexcept { ch.c = c; reset(); }
    void reset() noexcept { ch.reset(); prevOdd = 0.0f; }
    void process (const float* in, int n, float* out) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            const float p0 = ch.path (in[2 * i], 0);
            const float p1 = ch.path (prevOdd, 1);
            prevOdd = in[2 * i + 1];
            out[i] = 0.5f * (p0 + p1);
        }
    }
};

/** Biquad transpuesto forma II (estado en doble precisión: barato a tasa base y robusto en graves). */
struct Biquad
{
    design::BiquadCoefs c;
    double s1 = 0, s2 = 0;
    void reset() noexcept { s1 = s2 = 0; }
    float process (float xf) noexcept
    {
        const double x = xf;
        const double y = c.b0 * x + s1;
        s1 = c.b1 * x - c.a1 * y + s2;
        s2 = c.b2 * x - c.a2 * y;
        return (float) y;
    }
};
/** Biquad TDF-II en float (medición K-ponderada: suficiente precisión y la mitad de costo). */
struct BiquadF
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, s1 = 0, s2 = 0;
    void set (const design::BiquadCoefs& c) noexcept
    {
        b0 = (float) c.b0; b1 = (float) c.b1; b2 = (float) c.b2; a1 = (float) c.a1; a2 = (float) c.a2;
    }
    void reset() noexcept { s1 = s2 = 0; }
    float process (float x) noexcept
    {
        const float y = b0 * x + s1;
        s1 = b1 * x - a1 * y + s2;
        s2 = b2 * x - a2 * y;
        return y;
    }
};
} // namespace clip5
