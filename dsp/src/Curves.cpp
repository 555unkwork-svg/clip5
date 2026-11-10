#include "clip5/Curves.h"

#include <algorithm>
#include <cmath>

namespace clip5::curves
{
double densityGain (double a, double densityDb) noexcept
{
    if (a >= 1.0) return 1.0;
    a = std::max (0.0, a);
    const double g0 = std::pow (10.0, densityDb / 20.0);
    const double w = 1.0 - a * a;
    return 1.0 + (g0 - 1.0) * w * w;
}

double density (double u, double densityDb) noexcept
{
    return u * densityGain (std::abs (u), densityDb);
}

double knee (double v, double w) noexcept
{
    if (w <= 0.0) return std::min (v, 1.0);
    const double a = 1.0 - 0.5 * w;
    if (v <= a) return v;
    const double t = std::min ((v - a) / w, 1.0);
    const double t3 = t * t * t;
    return a + w * (t - t3 + 0.5 * t3 * t);
}

void kneeWidths (ClipMode mode, double& wPos, double& wNeg) noexcept
{
    switch (mode)
    {
        case ClipMode::Suave:   wPos = 0.50; wNeg = 0.70; break; // asimetría leve → pares
        case ClipMode::Firme:   wPos = 0.16; wNeg = 0.16; break;
        case ClipMode::Duro:    wPos = 0.0;  wNeg = 0.0;  break;
        case ClipMode::Apagado: wPos = -1.0; wNeg = -1.0; break;
    }
}

double clip (double v, ClipMode mode) noexcept
{
    if (mode == ClipMode::Apagado) return v;
    double wp, wn;
    kneeWidths (mode, wp, wn);
    return v >= 0.0 ? knee (v, wp) : -knee (-v, wn);
}

double shaper (double u, double densityDb, ClipMode mode) noexcept
{
    return clip (density (u, densityDb), mode);
}

double outerSlope (ClipMode mode) noexcept
{
    return mode == ClipMode::Apagado ? 1.0 : 0.0;
}
} // namespace clip5::curves
