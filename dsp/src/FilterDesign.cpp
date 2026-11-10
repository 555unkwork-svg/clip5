#include "clip5/FilterDesign.h"
#include "clip5/Common.h"

#include <complex>
#include <stdexcept>

namespace clip5::design
{
namespace
{
    double besselI0 (double x)
    {
        double sum = 1.0, term = 1.0, k = 1.0;
        const double q = x * x / 4.0;
        while (term > 1e-14 * sum)
        {
            term *= q / (k * k);
            sum += term;
            k += 1.0;
        }
        return sum;
    }

    double sinc (double x) { return std::abs (x) < 1e-12 ? 1.0 : std::sin (kPi * x) / (kPi * x); }

    double kaiserBeta (double A)
    {
        if (A > 50) return 0.1102 * (A - 8.7);
        if (A > 21) return 0.5842 * std::pow (A - 21, 0.4) + 0.07886 * (A - 21);
        return 0.0;
    }
}

std::vector<double> kaiserHalfband (double fsHigh, double fPass, double attenDb)
{
    const double fStop = fsHigh / 2 - fPass;
    const double width = (fStop - fPass) / (fsHigh / 2);
    int n = (int) std::ceil ((attenDb - 7.95) / 2.285 / (kPi * width) + 1);
    n += (3 - n % 4 + 4) % 4; // 4m+3
    const double beta = kaiserBeta (attenDb);
    const double c = (n - 1) / 2.0;
    std::vector<double> h ((size_t) n, 0.0);
    double sum = 0;
    for (int i = 0; i < n; ++i)
    {
        const double m = i - c;
        const int mi = (int) std::lround (m);
        if (mi != 0 && mi % 2 == 0) continue; // ceros exactos del half-band
        const double r = m / c;
        const double w = besselI0 (beta * std::sqrt (std::max (0.0, 1 - r * r))) / besselI0 (beta);
        h[(size_t) i] = mi == 0 ? 0.5 : 0.5 * sinc (m / 2) * w;
        sum += h[(size_t) i];
    }
    // normalizar la suma de los taps impares para que la ganancia en DC sea exactamente 1
    const double centre = 0.5;
    const double scale = (1.0 - centre) / (sum - centre);
    for (int i = 0; i < n; ++i)
        if (i != (int) c) h[(size_t) i] *= scale;
    return h;
}

// ------------------------------------------------------------------ half-band IIR (HIIR)
namespace
{
    void transitionParams (double transition, double& k, double& q)
    {
        k = std::tan ((1 - transition * 2) * kPi / 4);
        k *= k;
        const double kksqrt = std::pow (1 - k * k, 0.25);
        const double e = 0.5 * (1 - kksqrt) / (1 + kksqrt);
        const double e2 = e * e, e4 = e2 * e2;
        q = e * (1 + e4 * (2 + e4 * (15 + 150 * e4)));
    }

    double iirCoef (int index, double k, double q, int order)
    {
        const int c = index + 1;
        double acc = 0, j = 1;
        for (int i = 0;; ++i)
        {
            double t = std::pow (q, (double) (i * (i + 1))) * std::sin ((i * 2 + 1) * c * kPi / order) * j;
            acc += t;
            j = -j;
            if (std::abs (t) < 1e-100) break;
        }
        const double num = acc * std::pow (q, 0.25);
        acc = 0; j = -1;
        for (int i = 1;; ++i)
        {
            double t = std::pow (q, (double) (i * i)) * std::cos (i * 2 * c * kPi / order) * j;
            acc += t;
            j = -j;
            if (std::abs (t) < 1e-100) break;
        }
        const double den = acc + 0.5;
        const double ww = num / den, wwsq = ww * ww;
        const double x = std::sqrt ((1 - wwsq * k) * (1 - wwsq / k)) / (1 + wwsq);
        return (1 - x) / (1 + x);
    }
}

double iirHalfbandMagnitude (const std::vector<double>& a, double f)
{
    using C = std::complex<double>;
    const C z = std::polar (1.0, 2 * kPi * f);
    const C zi2 = 1.0 / (z * z);
    C A0 = 1.0, A1 = 1.0;
    for (size_t i = 0; i < a.size(); ++i)
    {
        const C ap = (a[i] + zi2) / (1.0 + a[i] * zi2);
        if (i % 2 == 0) A0 *= ap; else A1 *= ap;
    }
    return std::abs (0.5 * (A0 + A1 / z));
}

std::vector<double> iirHalfband (double transition, double attenDb)
{
    double k, q;
    transitionParams (transition, k, q);
    const double fStop = 0.25 + transition;
    for (int n = 2; n <= 20; ++n)
    {
        std::vector<double> a ((size_t) n);
        for (int i = 0; i < n; ++i) a[(size_t) i] = iirCoef (i, k, q, 2 * n + 1);
        double worst = 0;
        for (int g = 0; g <= 400; ++g)
            worst = std::max (worst, iirHalfbandMagnitude (a, fStop + (0.5 - fStop) * g / 400.0));
        if (20 * std::log10 (worst + 1e-300) <= -attenDb) return a;
    }
    throw std::runtime_error ("iirHalfband: atenuación inalcanzable");
}

// ------------------------------------------------------------------ FIR por mínimos cuadrados
std::vector<double> leastSquaresFir (int taps, double fs, double fPass, double fStop,
                                     const std::function<double (double)>& target, double D,
                                     double stopWeight)
{
    const int ngrid = std::max (2048, taps * 24);
    std::vector<double> r ((size_t) taps, 0.0), p ((size_t) taps, 0.0);
    for (int g = 0; g <= ngrid; ++g)
    {
        const double w = kPi * g / ngrid;
        const double f = w * fs / (2 * kPi);
        double W, T;
        if (f <= fPass) { W = 1.0; T = target (f); }
        else if (f >= fStop) { W = stopWeight; T = 0.0; }
        else continue;
        if (g == 0 || g == ngrid) W *= 0.5;
        for (int m = 0; m < taps; ++m)
        {
            r[(size_t) m] += W * std::cos (w * m);
            p[(size_t) m] += W * T * std::cos (w * (m - D));
        }
    }
    // matriz de Toeplitz simétrica → Cholesky con regularización mínima
    const int n = taps;
    std::vector<double> L ((size_t) (n * n), 0.0);
    const double reg = r[0] * 1e-13;
    for (int i = 0; i < n; ++i)
        for (int j = 0; j <= i; ++j)
        {
            double s = r[(size_t) std::abs (i - j)] + (i == j ? reg : 0.0);
            for (int k = 0; k < j; ++k) s -= L[(size_t) (i * n + k)] * L[(size_t) (j * n + k)];
            if (i == j)
                L[(size_t) (i * n + i)] = std::sqrt (std::max (s, 1e-300));
            else
                L[(size_t) (i * n + j)] = s / L[(size_t) (j * n + j)];
        }
    std::vector<double> y ((size_t) n), h ((size_t) n);
    for (int i = 0; i < n; ++i)
    {
        double s = p[(size_t) i];
        for (int k = 0; k < i; ++k) s -= L[(size_t) (i * n + k)] * y[(size_t) k];
        y[(size_t) i] = s / L[(size_t) (i * n + i)];
    }
    for (int i = n - 1; i >= 0; --i)
    {
        double s = y[(size_t) i];
        for (int k = i + 1; k < n; ++k) s -= L[(size_t) (k * n + i)] * h[(size_t) k];
        h[(size_t) i] = s / L[(size_t) (i * n + i)];
    }
    return h;
}

double tonoDb (int tono, double f)
{
    double D = 0, f0 = 1;
    if (tono == 1) { D = 3.0; f0 = 14000.0; }      // Seda
    else if (tono == 2) { D = 6.0; f0 = 10000.0; } // Terciopelo
    if (D == 0) return 0.0;
    const double r = f / f0, r2 = r * r;
    return -D * r2 / (1 + r2);
}

std::vector<double> truePeakInterpolator()
{
    // 4x, 128 taps (32 por fase), mínimos cuadrados: plano hasta 0,46·fs, rechazo desde 0,54·fs.
    // Junto con el refinamiento parabólico del detector: error ≤ ±0,02 dB frente a la
    // reconstrucción ideal para material limitado a 20 kHz (ver proto/tp_diseno3.py).
    auto one = [] (double) { return 1.0; };
    std::vector<double> h = leastSquaresFir (128, 4.0, 0.46, 0.54, one, 63.5, 1e3);
    for (auto& v : h) v *= 4.0;
    return h;
}

BiquadCoefs highShelf (double fs, double f0, double gainDb, double q)
{
    const double A = std::pow (10.0, gainDb / 40.0);
    const double w0 = 2 * kPi * f0 / fs, cw = std::cos (w0), sw = std::sin (w0);
    const double alpha = sw / (2 * q), sA = 2 * std::sqrt (A) * alpha;
    const double a0 = (A + 1) - (A - 1) * cw + sA;
    BiquadCoefs c;
    c.b0 = A * ((A + 1) + (A - 1) * cw + sA) / a0;
    c.b1 = -2 * A * ((A - 1) + (A + 1) * cw) / a0;
    c.b2 = A * ((A + 1) + (A - 1) * cw - sA) / a0;
    c.a1 = 2 * ((A - 1) - (A + 1) * cw) / a0;
    c.a2 = ((A + 1) - (A - 1) * cw - sA) / a0;
    return c;
}

double biquadMagnitudeDb (const BiquadCoefs& c, double fs, double f)
{
    using C = std::complex<double>;
    const C z1 = std::polar (1.0, -2 * kPi * f / fs), z2 = z1 * z1;
    const C H = (c.b0 + c.b1 * z1 + c.b2 * z2) / (1.0 + c.a1 * z1 + c.a2 * z2);
    return 20 * std::log10 (std::abs (H));
}

BiquadCoefs fitHighShelf (double fs, const std::function<double (double)>& targetDb, double* maxErrDb)
{
    const double fMax = std::min (19000.0, 0.45 * fs);
    auto err = [&] (const BiquadCoefs& c) {
        double e = 0;
        for (int i = 0; i <= 60; ++i)
        {
            const double f = 1000.0 * std::pow (fMax / 1000.0, i / 60.0);
            e = std::max (e, std::abs (biquadMagnitudeDb (c, fs, f) - targetDb (f)));
        }
        return e;
    };
    BiquadCoefs best;
    double bestE = 1e9, bf = 10000, bg = 0, bq = 0.7071;
    auto tryOne = [&] (double f0, double g, double q) {
        if (f0 < 200 || f0 > 0.49 * fs || q < 0.3 || q > 3) return;
        const BiquadCoefs c = highShelf (fs, f0, g, q);
        const double e = err (c);
        if (e < bestE) { bestE = e; best = c; bf = f0; bg = g; bq = q; }
    };
    for (double q : { 0.5, 0.6, 0.7071, 0.85, 1.0, 1.25 })
        for (double f0 = 2000; f0 <= std::min (30000.0, 0.48 * fs); f0 *= 1.05)
            for (double g = -16; g <= 16; g += 0.5)
                tryOne (f0, g, q);
    // refinamiento por coordenadas con pasos decrecientes
    double sf = 0.04, sg = 0.25, sq = 0.08;
    for (int it = 0; it < 40; ++it)
    {
        const double f0 = bf, g = bg, q = bq;
        for (int a = -2; a <= 2; ++a) tryOne (f0 * (1 + sf * a), g, q);
        for (int a = -2; a <= 2; ++a) tryOne (bf, g + sg * a, q);
        for (int a = -2; a <= 2; ++a) tryOne (bf, bg, q + sq * a);
        if (bf == f0 && bg == g && bq == q) { sf *= 0.5; sg *= 0.5; sq *= 0.5; }
    }
    if (maxErrDb) *maxErrDb = bestE;
    return best;
}

void kWeighting (double fs, BiquadCoefs& shelf, BiquadCoefs& hp)
{
    {
        const double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196;
        const double K = std::tan (kPi * f0 / fs);
        const double Vh = std::pow (10.0, G / 20.0), Vb = std::pow (Vh, 0.4996667741545416);
        const double a0 = 1.0 + K / Q + K * K;
        shelf.b0 = (Vh + Vb * K / Q + K * K) / a0;
        shelf.b1 = 2.0 * (K * K - Vh) / a0;
        shelf.b2 = (Vh - Vb * K / Q + K * K) / a0;
        shelf.a1 = 2.0 * (K * K - 1.0) / a0;
        shelf.a2 = (1.0 - K / Q + K * K) / a0;
    }
    {
        const double f0 = 38.13547087602444, Q = 0.5003270373238773;
        const double K = std::tan (kPi * f0 / fs);
        const double a0 = 1.0 + K / Q + K * K;
        hp.b0 = 1.0; hp.b1 = -2.0; hp.b2 = 1.0;
        hp.a1 = 2.0 * (K * K - 1.0) / a0;
        hp.a2 = (1.0 - K / Q + K * K) / a0;
    }
}
} // namespace clip5::design
