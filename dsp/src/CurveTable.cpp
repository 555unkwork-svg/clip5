#include "clip5/CurveTable.h"

namespace clip5
{
void CurveTable::build (const CurveParams& p) noexcept
{
    params = p;
    const double G = (double) p.densityDb;
    const ClipMode m = p.mode;
    const double h = (double) kH;
    const double d = 1e-5;
    auto f = [&] (double u) { return curves::shaper (u, G, m); };

    double prevVal = f ((double) kLo);
    double prevDp = (-3 * prevVal + 4 * f (kLo + d) - f (kLo + 2 * d)) / (2 * d);
    double acc = 0.0;
    double Pd[kSeg + 1];
    Pd[0] = 0.0;

    for (int i = 0; i < kSeg; ++i)
    {
        const double x1 = (double) kLo + h * (i + 1);
        const double f1 = f (x1);
        // derivadas laterales: los quiebres (modo Duro, |v| = 1) caen en nodos y se respetan
        const double dm = (3 * f1 - 4 * f (x1 - d) + f (x1 - 2 * d)) / (2 * d);
        const double dp = (-3 * f1 + 4 * f (x1 + d) - f (x1 + 2 * d)) / (2 * d);

        const double f0 = prevVal, d0 = prevDp * h, d1 = dm * h;
        const double c0 = f0, c1 = d0;
        const double c2 = 3 * (f1 - f0) - 2 * d0 - d1;
        const double c3 = 2 * (f0 - f1) + d0 + d1;
        c[i][0] = (float) c0; c[i][1] = (float) c1; c[i][2] = (float) c2; c[i][3] = (float) c3;
        acc += h * (c0 + c1 / 2 + c2 / 3 + c3 / 4);
        Pd[i + 1] = acc;

        prevVal = f1;
        prevDp = dp;
    }
    const double anchor = Pd[kHalf];
    for (int i = 0; i <= kSeg; ++i)
        P[i] = (float) (Pd[i] - anchor);

    fLo = (float) f ((double) kLo);
    fHi = (float) f ((double) kHi);
    slope = (float) curves::outerSlope (m);
}
} // namespace clip5
