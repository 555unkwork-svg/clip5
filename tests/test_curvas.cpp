// Verificaciones de las curvas y de la tabla (equivalentes a proto/test_curvas.py).
#include "clip5/CurveTable.h"
#include "util.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using namespace clip5;

static const ClipMode kModes[] = { ClipMode::Suave, ClipMode::Firme, ClipMode::Duro, ClipMode::Apagado };

TEST_CASE ("Curvas: monotonía, techo, ganancia de bajo nivel, continuidad de f'", "[curvas]")
{
    const double G = GENERATE (0.0, 3.0, 6.0);
    for (ClipMode m : kModes)
    {
        CAPTURE (G, (int) m);
        double prev = curves::shaper (-3.0, G, m), maxAbs = 0;
        double prevD = 0, maxJump = 0;
        const double h = 1e-5;
        for (double u = -3.0 + h; u <= 3.0; u += h)
        {
            const double f = curves::shaper (u, G, m);
            REQUIRE (f >= prev - 1e-15);           // monótona
            const double d = (f - prev) / h;
            const double v = curves::density (u, G);
            const bool nearKink = m == ClipMode::Duro && std::abs (std::abs (v) - 1.0) < 1e-3;
            if (u > -3.0 + 2 * h && ! nearKink) maxJump = std::max (maxJump, std::abs (d - prevD));
            prevD = d;
            prev = f;
            maxAbs = std::max (maxAbs, std::abs (f));
        }
        CHECK (maxJump < 1e-2);                    // f' continua
        CHECK (curves::density (1.0, G) == 1.0);   // f_d(C) = C
        CHECK (curves::density (-1.0, G) == -1.0);
        if (m == ClipMode::Duro || m == ClipMode::Apagado)
            CHECK (std::abs (curves::shaper (1.0, G, m) - 1.0) < 1e-12); // f(C) = C
        if (m != ClipMode::Apagado)
            CHECK (maxAbs <= 1.0 + 1e-12);         // la salida nunca supera el techo
        const double x = 1e-4;
        CHECK (std::abs (20 * std::log10 (curves::shaper (x, G, m) / x) - G) < 1e-3); // ganancia = G
    }
}

TEST_CASE ("Tabla float: error frente a la curva y F1 consistente con f", "[curvas][tabla]")
{
    const float G = GENERATE (0.0f, 3.0f, 6.0f);
    for (ClipMode m : kModes)
    {
        CurveTable t;
        t.build ({ G, m });
        double maxErr = 0, maxErrF = 0;
        const double du = 1e-3;
        for (double u = -2.6; u <= 2.6; u += 1e-4)
            maxErr = std::max (maxErr, std::abs ((double) t.eval ((float) u) - curves::shaper (u, G, m)));
        for (double u = -2.5; u <= 2.5; u += 0.0137)
        {
            const double dF = ((double) t.antiderivative ((float) (u + du)) - (double) t.antiderivative ((float) (u - du))) / (2 * du);
            maxErrF = std::max (maxErrF, std::abs (dF - (double) t.eval ((float) u)));
        }
        CAPTURE (G, (int) m, maxErr, maxErrF);
        CHECK (maxErr < 2e-6);
        CHECK (maxErrF < 2e-3);
    }
}

TEST_CASE ("ADAA por tramo en float32 frente a referencia en doble precisión", "[curvas][adaa]")
{
    // referencia: media exacta de la curva analítica por cuadratura de Gauss-Legendre (20 puntos por tramo)
    auto meanRef = [] (double a, double b, double G, ClipMode m) {
        if (std::abs (b - a) < 1e-12) return curves::shaper (0.5 * (a + b), G, m);
        static const double xg[5] = { 0.1488743389816312, 0.4333953941292472, 0.6794095682990244, 0.8650633666889845, 0.9739065285171717 };
        static const double wg[5] = { 0.2955242247147529, 0.2692667193099963, 0.2190863625159820, 0.1494513491505806, 0.0666713443086881 };
        const int pieces = 64;
        double s = 0;
        for (int p = 0; p < pieces; ++p)
        {
            const double lo = a + (b - a) * p / pieces, hi = a + (b - a) * (p + 1) / pieces;
            const double c = 0.5 * (lo + hi), r = 0.5 * (hi - lo);
            for (int k = 0; k < 5; ++k)
                s += wg[k] * r * (curves::shaper (c + r * xg[k], G, m) + curves::shaper (c - r * xg[k], G, m));
        }
        return s / (b - a);
    };

    const double level = GENERATE (-40.0, -12.0, 0.0, 6.0);
    const ClipMode m = ClipMode::Firme;
    const double G = 4.0;
    CurveTable t;
    t.build ({ (float) G, m });
    const int n = 8000;
    const double amp = std::pow (10.0, level / 20.0), fs = 96000;
    double errSq = 0, refSq = 0;
    float u1 = 0;
    for (int i = 0; i < n; ++i)
    {
        const float u = (float) (amp * std::sin (2 * kPi * 997 * i / fs));
        const double y = t.adaa (u, u1);
        const double r = meanRef ((double) u1, (double) u, G, m);
        errSq += (y - r) * (y - r);
        refSq += r * r;
        u1 = u;
    }
    const double errDb = 10 * std::log10 (errSq / refSq);
    CAPTURE (level, errDb);
    CHECK (errDb < -110.0);
}
