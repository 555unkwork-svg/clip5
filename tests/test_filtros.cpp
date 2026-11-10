// Filtros de sobremuestreo y decimación.
#include "clip5/Engine.h"
#include "util.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using namespace clip5;

static double firMagnitude (const std::vector<double>& h, double f, double fs)
{
    std::complex<double> s = 0;
    for (size_t i = 0; i < h.size(); ++i) s += h[i] * std::polar (1.0, -2 * kPi * f / fs * (double) i);
    return std::abs (s);
}

TEST_CASE ("Half-band FIR: rechazo ≥ 105 dB y ganancia DC = 1", "[filtros]")
{
    const double fs = GENERATE (44100.0, 48000.0, 96000.0);
    for (int s = 0; s < 3; ++s)
    {
        const double fsH = fs * (1 << (s + 1)), fPass = std::min (20000.0, 0.45 * fs);
        const auto h = design::kaiserHalfband (fsH, fPass, 118.0);
        double worst = 0, ripple = 0;
        for (double f = fsH / 2 - fPass; f <= fsH / 2; f += 50) worst = std::max (worst, firMagnitude (h, f, fsH));
        for (double f = 0; f <= fPass; f += 50) ripple = std::max (ripple, std::abs (testutil::db (firMagnitude (h, f, fsH))));
        CAPTURE (fs, s, h.size(), testutil::db (worst), ripple);
        CHECK (testutil::db (worst) < -105.0);
        CHECK (ripple < 0.001);
        CHECK (h.size() % 4 == 3);
    }
}

TEST_CASE ("Half-band IIR (fase mínima): rechazo ≥ 110 dB", "[filtros]")
{
    const double fs = GENERATE (44100.0, 48000.0);
    const double fsH = 2 * fs, fPass = std::min (20000.0, 0.45 * fs);
    const double tr = (fsH / 4 - fPass) / fsH;
    const auto a = design::iirHalfband (tr, 110.0);
    double worst = 0;
    for (double f = 0.25 + tr; f <= 0.5; f += 0.0005) worst = std::max (worst, design::iirHalfbandMagnitude (a, f));
    CAPTURE (fs, a.size(), testutil::db (worst));
    CHECK (testutil::db (worst) <= -110.0);
}

TEST_CASE ("FIR decimador (Tono + compensación ADAA): rizado, rechazo y fase", "[filtros]")
{
    const double fs = GENERATE (44100.0, 48000.0, 88200.0, 96000.0);
    ChainDesigns d;
    d.build (fs);
    const double fPass = std::min (20000.0, 0.45 * fs);
    for (int q = 1; q < kNumQualities; ++q)
        for (int t = 0; t < kNumTonos; ++t)
        {
            // reconstruir los taps desde los núcleos par/impar
            const auto& K = d.decim[q][t];
            std::vector<double> g;
            const int ne = K.even.len, no = K.odd.len;
            for (int i = 0; i < std::max (ne, no) * 2; ++i)
            {
                const int k = i / 2;
                const auto& ker = (i % 2 == 0) ? K.even : K.odd;
                const int len = ker.len;
                g.push_back (k < len ? (double) ker.rev[(size_t) (len - 1 - k)] : 0.0);
            }
            const double fs2 = 2 * fs, N = 1 << q;
            double worstStop = 0, worstPass = 0;
            for (double f = fs / 2; f <= fs; f += 25) worstStop = std::max (worstStop, firMagnitude (g, f, fs2));
            for (double f = 20; f <= fPass; f += 50)
            {
                const double target = design::tonoDb (t, f) - 20 * std::log10 (std::cos (kPi * f / (N * fs)));
                worstPass = std::max (worstPass, std::abs (testutil::db (firMagnitude (g, f, fs2)) - target));
            }
            CAPTURE (fs, q, t, d.lsTaps, testutil::db (worstStop), worstPass);
            CHECK (testutil::db (worstStop) < -100.0);
            CHECK (worstPass < 0.02);
        }
}

TEST_CASE ("Ajuste de shelves (fase mínima) dentro de tolerancia", "[filtros]")
{
    ChainDesigns d;
    d.build (48000.0);
    CAPTURE (d.tonoFitErrDb, d.compFitErrDb[0], d.compFitErrDb[1], d.compFitErrDb[2], d.compFitErrDb[3]);
    CHECK (d.tonoFitErrDb < 0.1);
    CHECK (d.compFitErrDb[0] < 0.2);
    for (int q = 1; q < kNumQualities; ++q) CHECK (d.compFitErrDb[q] < 0.03);
}
