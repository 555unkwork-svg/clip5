// Motor completo: null test, latencia, Techo estricto, aliasing, crossfade, offline, silencio.
#include "clip5/Engine.h"
#include "util.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cstdio>

using namespace clip5;

namespace
{
struct Stereo { std::vector<float> l, r; };

Stereo run (Engine& e, const std::vector<float>& inL, const std::vector<float>& inR, bool realtime = false, int block = 512)
{
    Stereo out { inL, inR };
    float* ch[2];
    for (size_t i = 0; i < out.l.size(); i += (size_t) block)
    {
        const int n = (int) std::min ((size_t) block, out.l.size() - i);
        ch[0] = out.l.data() + i;
        ch[1] = out.r.data() + i;
        e.process (ch, n, realtime);
    }
    return out;
}

EngineParams neutral()
{
    EngineParams p;
    p.ceilingDb = 0; p.densityDb = 0; p.mode = ClipMode::Apagado; p.suavizado = 0;
    p.tono = 0; p.strictCeiling = true; p.unitGain = false; p.wetDb = 0; p.dryDb = kSilenceDb;
    return p;
}
}

TEST_CASE ("Null test: todo neutro + mezcla 100 % seca = entrada (< −140 dB)", "[motor]")
{
    const double fs = GENERATE (44100.0, 48000.0, 96000.0);
    const int q = GENERATE (0, 1, 2, 3);
    Engine e;
    EngineParams p = neutral();
    p.wetDb = kSilenceDb; p.dryDb = 0; p.qualityRealtime = q;
    e.setParams (p);
    e.prepare (fs, 2);
    const int n = 48000;
    auto x = testutil::pinkNoise (n, 0.25, 7);
    auto y = run (e, x, x);
    const int L = e.latencySamples();
    double err = 0, ref = 0;
    for (int i = L + 1000; i < n; ++i)
    {
        const double d = (double) y.l[(size_t) i] - x[(size_t) (i - L)];
        err += d * d; ref += (double) x[(size_t) (i - L)] * x[(size_t) (i - L)];
    }
    const double nullDb = err > 0 ? 10 * std::log10 (err / ref) : -400.0;
    CAPTURE (fs, q, L, nullDb);
    CHECK (nullDb < -140.0);
}

TEST_CASE ("Latencia entera y fase lineal: el húmedo neutro coincide con el seco", "[motor]")
{
    const double fs = GENERATE (44100.0, 48000.0, 96000.0);
    const int q = GENERATE (0, 1, 2, 3);
    Engine e;
    EngineParams p = neutral();
    p.qualityRealtime = q;
    e.setParams (p);
    e.prepare (fs, 2);
    const int L = e.latencySamples();
    for (double f : { 100.0, 1000.0, 5000.0, 12000.0 })
    {
        const int n = 32768;
        const auto x = testutil::sine (f, fs, n + L, 0.1);
        auto y = run (e, x, x);
        const auto H = testutil::bin (y.l.data() + L + 8192, n - 8192, f, fs);
        const auto X = testutil::bin (x.data() + 8192, n - 8192, f, fs);
        const double magDb = testutil::db (std::abs (H) / std::abs (X));
        double ph = std::arg (H / X) * 180 / kPi;
        CAPTURE (fs, q, L, f, magDb, ph);
        // Eco (1x) tiene compensación de ADAA más agresiva; aun así ±0,1 dB
        CHECK (std::abs (magDb) < (q == 0 ? 0.1 : 0.03));
        CHECK (std::abs (ph) < 0.5);
    }
}

TEST_CASE ("Fase mínima: latencia baja y respuesta plana", "[motor]")
{
    const int q = GENERATE (1, 2, 3);
    Engine e;
    EngineParams p = neutral();
    p.qualityRealtime = q; p.phase = (int) PhaseMode::Minima;
    e.setParams (p);
    e.prepare (48000.0, 2);
    const int wet = e.latencySamples() - e.latencyFor (q, 0) + e.designs().latency[q][0];
    CAPTURE (q, wet, e.designs().latency[q][1]);
    CHECK (e.designs().latency[q][1] <= 8);
    const int n = 32768;
    for (double f : { 1000.0, 10000.0, 18000.0 })
    {
        const auto x = testutil::sine (f, 48000.0, n, 0.1);
        auto y = run (e, x, x);
        const double magDb = testutil::db (std::abs (testutil::bin (y.l.data() + 8192, n - 8192, f, 48000.0))
                                           / std::abs (testutil::bin (x.data() + 8192, n - 8192, f, 48000.0)));
        CAPTURE (f, magDb);
        CHECK (std::abs (magDb) < 0.3);
    }
}

TEST_CASE ("Techo estricto: true peak de salida ≤ techo en todos los casos", "[motor][techo]")
{
    // tipo: 0 senoide, 1 ruido rosa, 2 impulsos, 3 cuadrada. bandaCompleta = sin limitar a 20 kHz
    struct Caso { const char* nombre; ClipMode m; float in, dens, ceil, wet, dry, out; int q; double f; int tipo; bool bandaCompleta = false; };
    const Caso casos[] = {
        { "Duro 1k +12",           ClipMode::Duro,    12, 0, -1,  0, kSilenceDb, 0, 1, 1000, 0 },
        { "Duro 7k +12",           ClipMode::Duro,    12, 0, -1,  0, kSilenceDb, 0, 1, 7000, 0 },
        { "Duro 11k +12 Eco",      ClipMode::Duro,    12, 0, -3,  0, kSilenceDb, 0, 0, 11025, 0 },
        { "Firme 15k +9",          ClipMode::Firme,    9, 3, -0.3f, 0, kSilenceDb, 0, 2, 15000, 0 },
        { "Suave 19k +6",          ClipMode::Suave,    6, 6, -6,  0, kSilenceDb, 0, 3, 19000, 0 },
        { "Apagado ruido +12",     ClipMode::Apagado, 12, 6, -1,  0, kSilenceDb, 0, 1, 0, 1 },
        { "Paralelo seco +6",      ClipMode::Duro,    12, 0, -1,  0, 6,          0, 1, 0, 1 },
        { "Húmedo +6 Salida +12",  ClipMode::Firme,    6, 3, -12, 6, kSilenceDb, 12, 1, 0, 1 },
        { "Impulsos",              ClipMode::Duro,    12, 0, -1,  0, 0,          0, 1, 0, 2 },
        { "Cuadrada 3k",           ClipMode::Firme,   12, 0, -0.1f, 0, kSilenceDb, 0, 1, 3000, 3 },
        { "Seco +6 (banda compl.)", ClipMode::Duro,   12, 0, -1,  0, 6,          0, 1, 0, 1, true },
        { "Impulsos (banda compl.)", ClipMode::Duro,  12, 0, -1,  0, 0,          0, 1, 0, 2, true },
        { "Cuadrada (banda compl.)", ClipMode::Firme, 12, 0, -0.1f, 0, 0,        0, 1, 3000, 3, true },
    };
    for (const auto& c : casos)
    {
        Engine e;
        EngineParams p;
        p.mode = c.m; p.inputDb = c.in; p.densityDb = c.dens; p.ceilingDb = c.ceil; p.wetDb = c.wet;
        p.dryDb = c.dry; p.outputDb = c.out; p.qualityRealtime = c.q; p.strictCeiling = true; p.suavizado = 0.5f;
        e.setParams (p);
        e.prepare (48000.0, 2);
        const int n = 48000 * 2;
        std::vector<float> x;
        if (c.tipo == 0) x = testutil::sine (c.f, 48000.0, n, 0.5);
        else if (c.tipo == 1) x = testutil::pinkNoise (n, 0.6, 3);
        else if (c.tipo == 2) { x.assign ((size_t) n, 0.0f); for (int i = 1000; i < n; i += 997) x[(size_t) i] = (i / 997) % 2 ? 0.9f : -0.9f; }
        else { x.resize ((size_t) n); for (int i = 0; i < n; ++i) x[(size_t) i] = std::sin (2 * kPi * c.f * i / 48000.0) > 0 ? 0.5f : -0.5f; }
        if (! c.bandaCompleta && c.tipo != 0) x = testutil::lowpass (x, 20000.0, 48000.0); // material real: ≤ 20 kHz
        auto y = run (e, x, x);

        // medidor 4x propio (mismo diseño que el detector) y referencia 16x independiente
        TruePeakDetector d;
        d.prepare();
        float tp4 = 0;
        for (float v : y.l) tp4 = std::max (tp4, d.push (v));
        std::vector<float> seg (y.l.begin() + 4800, y.l.begin() + 4800 + 24000);
        const double tp16 = testutil::referenceTruePeak (seg);
        const double ceilDb = c.ceil;
        CAPTURE (c.nombre, testutil::db (tp4), testutil::db (tp16), ceilDb);
        CHECK (testutil::db (tp4) <= ceilDb + 1e-3);        // medición BS.1770 (4x): garantía
        if (! c.bandaCompleta)
            CHECK (testutil::db (tp16) <= ceilDb + 0.03);    // reconstrucción ideal, material ≤ 20 kHz
        std::printf ("  techo estricto | %-22s | TP 4x %+7.3f dB | TP 16x %+7.3f dB | techo %+6.2f dB\n",
                     c.nombre, testutil::db (tp4), testutil::db (tp16), ceilDb);
    }
}

TEST_CASE ("Aliasing por calidad (informe; umbrales mínimos)", "[motor][alias]")
{
    const int q = GENERATE (0, 1, 2, 3);
    const bool adaa = GENERATE (false, true);
    const double fs = 48000;
    const int N = 1 << 15;
    std::vector<double> sars;
    for (double f0 : { 1000.0, 2500.0, 5000.0, 7000.0, 10000.0, 15000.0 })
    {
        int k = (int) std::lround (f0 * N / fs);
        if (k % 2 == 0) ++k;
        Engine e;
        EngineParams p = neutral();
        p.mode = ClipMode::Duro; p.qualityRealtime = q; p.strictCeiling = false; p.adaa = adaa; p.ceilingDb = -6;
        e.setParams (p);
        e.prepare (fs, 2);
        const int pad = 8192;
        std::vector<float> x ((size_t) (N + pad));
        for (int i = 0; i < N + pad; ++i) x[(size_t) i] = (float) (std::pow (10.0, 0.0 / 20) * std::sin (2 * kPi * k * (double) i / N));
        auto y = run (e, x, x);
        // DFT completo vía bins: potencia en armónicos legítimos vs. resto en 0–20 kHz
        const float* s = y.l.data() + pad;
        std::vector<std::complex<double>> X ((size_t) N);
        // FFT radix-2 simple
        for (int i = 0; i < N; ++i) X[(size_t) i] = s[i];
        for (int i = 1, j = 0; i < N; ++i)
        {
            int bit = N >> 1;
            for (; j & bit; bit >>= 1) j ^= bit;
            j ^= bit;
            if (i < j) std::swap (X[(size_t) i], X[(size_t) j]);
        }
        for (int len = 2; len <= N; len <<= 1)
        {
            const auto wl = std::polar (1.0, -2 * kPi / len);
            for (int i = 0; i < N; i += len)
            {
                std::complex<double> w = 1;
                for (int j = 0; j < len / 2; ++j)
                {
                    const auto u = X[(size_t) (i + j)], v = X[(size_t) (i + j + len / 2)] * w;
                    X[(size_t) (i + j)] = u + v; X[(size_t) (i + j + len / 2)] = u - v;
                    w *= wl;
                }
            }
        }
        const int band = (int) (20000.0 * N / fs);
        double sig = 0, ali = 0;
        for (int b = 1; b < band; ++b)
        {
            const double pw = std::norm (X[(size_t) b]);
            if (b % k == 0) sig += pw; else ali += pw;
        }
        for (int b = k; b < N / 2; b += k) if (b >= band) sig += std::norm (X[(size_t) b]);
        sars.push_back (10 * std::log10 (sig / std::max (ali, 1e-300)));
    }
    std::sort (sars.begin(), sars.end());
    const double med = 0.5 * (sars[2] + sars[3]);
    std::printf ("  alias | calidad %d | ADAA %-3s | señal/alias mediana %6.1f dB | mínimo %6.1f dB\n",
                 q, adaa ? "sí" : "no", med, sars.front());
    if (adaa)
    {
        const double minMedian[] = { 35.0, 55.0, 70.0, 80.0 };
        CHECK (med >= minMedian[q]);
    }
}

TEST_CASE ("Cambios de Densidad y Modo en vivo: crossfade sin saltos ni NaN", "[motor]")
{
    Engine e;
    EngineParams p;
    p.mode = ClipMode::Firme; p.densityDb = 0; p.inputDb = 6; p.ceilingDb = -3;
    e.setParams (p);
    e.prepare (48000.0, 2);
    e.startBackgroundWorker();
    const int n = 256;
    auto x = testutil::sine (220.0, 48000.0, n * 2000, 0.5);
    std::vector<float> l (x), r (x);
    float maxStep = 0, prev = 0;
    bool finite = true;
    for (int b = 0; b < 2000; ++b)
    {
        p.densityDb = (float) (3.0 + 3.0 * std::sin (b * 0.05));
        p.mode = (ClipMode) ((b / 300) % 4);
        e.setParams (p);
        float* ch[2] = { l.data() + b * n, r.data() + b * n };
        e.process (ch, n, true);
        if (b < 100) { prev = ch[0][n - 1]; continue; }
        for (int i = 0; i < n; ++i)
        {
            finite = finite && std::isfinite (ch[0][i]);
            maxStep = std::max (maxStep, std::abs (ch[0][i] - prev));
            prev = ch[0][i];
        }
    }
    e.stopBackgroundWorker();
    CAPTURE (maxStep);
    CHECK (finite);
    CHECK (maxStep < 0.08f); // 220 Hz a −3 dBFS: paso máximo natural ≈ 0,02
}

TEST_CASE ("Render offline determinista", "[motor]")
{
    auto render = [] {
        Engine e;
        EngineParams p;
        p.mode = ClipMode::Suave; p.densityDb = 4; p.inputDb = 8; p.ceilingDb = -2; p.suavizado = 0.7f;
        p.qualityRealtime = 1; p.qualityOffline = 3; p.dryDb = -6;
        e.setParams (p);
        e.prepare (48000.0, 2);
        auto x = testutil::pinkNoise (48000, 0.5, 11);
        return run (e, x, x, false).l;
    };
    const auto a = render(), b = render();
    CHECK (a == b);
}

TEST_CASE ("Silencio: se duerme sin cortar colas y despierta limpio", "[motor]")
{
    Engine e;
    EngineParams p = neutral();
    e.setParams (p);
    e.prepare (48000.0, 2);
    const int L = e.latencySamples();
    std::vector<float> x ((size_t) 48000 * 3, 0.0f);
    // ráfagas con fundidos (un escalón produciría pre-ringing legítimo del filtro de fase lineal)
    auto burst = [&] (int start, int len) {
        for (int i = 0; i < len; ++i)
        {
            const double w = std::min (1.0, std::min (i, len - 1 - i) / 480.0);
            x[(size_t) (start + i)] = (float) (0.3 * w * std::sin (0.05 * i));
        }
    };
    burst (0, 4800);
    burst (100000, 10000);
    auto y = run (e, x, x);
    double err = 0;
    for (size_t i = (size_t) L; i < y.l.size(); ++i) err = std::max (err, (double) std::abs (y.l[i] - x[i - (size_t) L]));
    CAPTURE (L, err);
    CHECK (err < 1e-4);
}

TEST_CASE ("Mono", "[motor]")
{
    Engine e;
    EngineParams p;
    p.mode = ClipMode::Firme; p.inputDb = 6;
    e.setParams (p);
    e.prepare (48000.0, 1);
    auto x = testutil::sine (1000.0, 48000.0, 9600, 0.5);
    float* ch[1] = { x.data() };
    e.process (ch, 9600, true);
    for (float v : x) REQUIRE (std::isfinite (v));
    CHECK (testutil::db (*std::max_element (x.begin(), x.end())) <= -1.0 + 0.01);
}

TEST_CASE ("Techo estricto apagado: sin latencia extra; conmutación en vivo sin saltos", "[motor]")
{
    Engine e;
    EngineParams p = neutral();
    p.phase = (int) PhaseMode::Minima; p.qualityRealtime = 1; p.strictCeiling = false;
    e.setParams (p);
    e.prepare (48000.0, 2);
    CHECK (e.latencySamples() == e.designs().latency[1][1]);
    CHECK (e.latencySamples() <= 8);
    // conmutar en tiempo real: fundido de salida/entrada, sin NaN ni saltos grandes
    auto x = testutil::sine (440.0, 48000.0, 48000, 0.3);
    std::vector<float> l (x), r (x);
    float prev = 0, maxStep = 0;
    for (int b = 0; b < 48000 / 256; ++b)
    {
        p.strictCeiling = (b / 40) % 2 == 1;
        e.setParams (p);
        float* ch[2] = { l.data() + b * 256, r.data() + b * 256 };
        e.process (ch, 256, true);
        for (int i = 0; i < 256; ++i) { REQUIRE (std::isfinite (ch[0][i])); maxStep = std::max (maxStep, std::abs (ch[0][i] - prev)); prev = ch[0][i]; }
    }
    CAPTURE (maxStep);
    CHECK (maxStep < 0.05f); // 440 Hz a 0,3: paso natural ≈ 0,017
}
