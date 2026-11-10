#pragma once
#include "clip5/Common.h"

#include <cmath>
#include <complex>
#include <random>
#include <vector>

namespace testutil
{
inline std::vector<float> sine (double f, double fs, int n, double amp, double phase = 0)
{
    std::vector<float> x ((size_t) n);
    for (int i = 0; i < n; ++i) x[(size_t) i] = (float) (amp * std::sin (2 * clip5::kPi * f * i / fs + phase));
    return x;
}

inline std::vector<float> pinkNoise (int n, double amp, unsigned seed = 1)
{
    std::mt19937 rng (seed);
    std::normal_distribution<double> nd (0, 1);
    std::vector<float> x ((size_t) n);
    double b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
    for (int i = 0; i < n; ++i)
    {
        const double w = nd (rng);
        b0 = 0.99886 * b0 + w * 0.0555179; b1 = 0.99332 * b1 + w * 0.0750759;
        b2 = 0.96900 * b2 + w * 0.1538520; b3 = 0.86650 * b3 + w * 0.3104856;
        b4 = 0.55000 * b4 + w * 0.5329522; b5 = -0.7616 * b5 - w * 0.0168980;
        x[(size_t) i] = (float) (amp * 0.11 * (b0 + b1 + b2 + b3 + b4 + b5 + b6 + w * 0.5362));
        b6 = w * 0.115926;
    }
    return x;
}

/** DFT de un solo bin (doble precisión). */
inline std::complex<double> bin (const float* x, int n, double f, double fs)
{
    std::complex<double> s = 0;
    const double w = 2 * clip5::kPi * f / fs;
    for (int i = 0; i < n; ++i) s += (double) x[i] * std::polar (1.0, -w * i);
    return s * (2.0 / n);
}

/** True peak de referencia: interpolación sinc de 128 taps (ventana Blackman-Harris) a 16x. */
inline double referenceTruePeak (const std::vector<float>& x, int start = 0)
{
    const int half = 64, os = 16;
    double peak = 0;
    for (int i = start + half; i + half < (int) x.size(); ++i)
    {
        peak = std::max (peak, (double) std::abs (x[(size_t) i]));
        for (int p = 1; p < os; ++p)
        {
            const double t = (double) p / os;
            double s = 0;
            for (int k = -half + 1; k <= half; ++k)
            {
                const double d = k - t;
                const double sincv = std::sin (clip5::kPi * d) / (clip5::kPi * d);
                const double r = (d + half) / (2.0 * half);
                const double w = 0.35875 - 0.48829 * std::cos (2 * clip5::kPi * r) + 0.14128 * std::cos (4 * clip5::kPi * r)
                               - 0.01168 * std::cos (6 * clip5::kPi * r);
                s += x[(size_t) (i + k)] * sincv * w;
            }
            peak = std::max (peak, std::abs (s));
        }
    }
    return peak;
}

/** Pasa-bajos de fase lineal (sinc + Blackman, 255 taps) para limitar material a fc. */
inline std::vector<float> lowpass (const std::vector<float>& x, double fc, double fs)
{
    const int taps = 255, c = taps / 2;
    std::vector<double> h ((size_t) taps);
    double s = 0;
    for (int i = 0; i < taps; ++i)
    {
        const double m = i - c, wc = 2 * fc / fs;
        const double sincv = m == 0 ? wc : std::sin (clip5::kPi * wc * m) / (clip5::kPi * m);
        const double w = 0.42 - 0.5 * std::cos (2 * clip5::kPi * i / (taps - 1)) + 0.08 * std::cos (4 * clip5::kPi * i / (taps - 1));
        h[(size_t) i] = sincv * w; s += h[(size_t) i];
    }
    std::vector<float> y (x.size(), 0.0f);
    for (size_t n = 0; n < x.size(); ++n)
    {
        double acc = 0;
        for (int k = 0; k < taps; ++k)
        {
            const long idx = (long) n - k + c;
            if (idx >= 0 && idx < (long) x.size()) acc += h[(size_t) k] / s * x[(size_t) idx];
        }
        y[n] = (float) acc;
    }
    return y;
}

inline double db (double v) { return 20.0 * std::log10 (std::max (v, 1e-30)); }
} // namespace testutil
