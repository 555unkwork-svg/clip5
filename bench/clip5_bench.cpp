// clip5 — benchmark de CPU. Procesa 60 s de ruido rosa y 60 s de "música" sintética (batería,
// bajo, acordes) en cada calidad y fase; imprime tiempo por muestra y % de un núcleo por
// instancia estéreo a 48 kHz. Uso: clip5_bench [--json salida.json] [--segundos N]
#include "clip5/Engine.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

using namespace clip5;

namespace
{
std::vector<float> pink (int n, unsigned seed)
{
    std::mt19937 rng (seed);
    std::normal_distribution<double> nd (0, 1);
    std::vector<float> x ((size_t) n);
    double b[7] {};
    for (int i = 0; i < n; ++i)
    {
        const double w = nd (rng);
        b[0] = 0.99886 * b[0] + w * 0.0555179; b[1] = 0.99332 * b[1] + w * 0.0750759;
        b[2] = 0.96900 * b[2] + w * 0.1538520; b[3] = 0.86650 * b[3] + w * 0.3104856;
        b[4] = 0.55000 * b[4] + w * 0.5329522; b[5] = -0.7616 * b[5] - w * 0.0168980;
        x[(size_t) i] = (float) (0.06 * (b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + b[6] + w * 0.5362));
        b[6] = w * 0.115926;
    }
    return x;
}

std::vector<float> music (int n, double fs, unsigned seed)
{
    std::mt19937 rng (seed);
    std::uniform_real_distribution<double> ud (-1, 1);
    std::vector<float> x ((size_t) n, 0.0f);
    const double beat = fs * 0.5; // 120 BPM
    const double chord[4][3] = { { 220, 277.2, 329.6 }, { 196, 246.9, 293.7 }, { 174.6, 220, 261.6 }, { 196, 246.9, 311.1 } };
    double hpPrev = 0, hpOut = 0, bassPh = 0;
    for (int i = 0; i < n; ++i)
    {
        const double t = i / fs;
        const double pos = std::fmod (i, beat) / fs;
        const int bar = (int) (i / (beat * 4)) % 4;
        double s = 0;
        s += 0.9 * std::exp (-pos * 18) * std::sin (2 * kPi * (50 + 120 * std::exp (-pos * 40)) * pos); // bombo
        if (((int) (i / beat)) % 2 == 1) s += 0.45 * std::exp (-pos * 25) * ud (rng);                     // caja
        const double hpos = std::fmod (i, beat / 2) / fs;
        const double nz = ud (rng);
        hpOut = 0.7 * (hpOut + nz - hpPrev); hpPrev = nz;
        s += 0.12 * std::exp (-hpos * 60) * hpOut;                                                        // hi-hat
        bassPh += chord[bar][0] / 2 / fs;
        s += 0.35 * (2 * std::fmod (bassPh, 1.0) - 1) * (0.6 + 0.4 * std::exp (-pos * 6));               // bajo
        for (double f : chord[bar]) s += 0.08 * std::sin (2 * kPi * f * 2 * t);                          // acordes
        x[(size_t) i] = (float) (0.55 * s);
    }
    return x;
}

struct Result { const char* q; const char* ph; double nsPerFrame, pctCore; };
}

int main (int argc, char** argv)
{
    std::string jsonPath;
    double seconds = 60;
    for (int i = 1; i < argc; ++i)
    {
        if (! std::strcmp (argv[i], "--json") && i + 1 < argc) jsonPath = argv[++i];
        else if (! std::strcmp (argv[i], "--segundos") && i + 1 < argc) seconds = std::atof (argv[++i]);
    }
    ScopedFlushDenormals ftz;
    const double fs = 48000;
    const int n = (int) (fs * seconds);
    const int block = 256;
    const auto sigs = { std::make_pair ("ruido rosa", pink (n, 1)), std::make_pair ("música", music (n, fs, 2)) };
    const char* qn[] = { "Eco 1x", "Normal 2x", "Alta 4x", "Máxima 8x" };
    const char* pn[] = { "lineal", "mínima" };

    std::printf ("clip5 benchmark — %.0f s por señal (mínimo de 3), estéreo, %.0f Hz, bloques de %d\n", seconds, fs, block);
    std::printf ("%-11s %-7s %-11s %12s %12s %10s\n", "calidad", "fase", "señal", "ns/muestra", "% núcleo", "meta");
    std::vector<Result> results;
    for (int ph = 0; ph < 2; ++ph)
        for (int q = 0; q < 4; ++q)
        {
            double worstPct = 0, worstNs = 0;
            for (const auto& [name, sig] : sigs)
            {
                Engine e;
                EngineParams p;
                p.inputDb = 4; p.densityDb = 3; p.mode = ClipMode::Firme; p.ceilingDb = -1; p.suavizado = 0.5f;
                p.tono = 1; p.strictCeiling = true; p.unitGain = false; p.wetDb = 0; p.dryDb = -12;
                p.qualityRealtime = q; p.phase = ph;
                e.setParams (p);
                e.prepare (fs, 2);
                // mínimo de 3 repeticiones: aísla interrupciones del sistema operativo / máquina virtual
                double el = 1e30;
                for (int rep = 0; rep < 3; ++rep)
                {
                    std::vector<float> l (sig), r (sig);
                    MeterFrame f;
                    const auto t0 = std::chrono::steady_clock::now();
                    for (int i = 0; i < n; i += block)
                    {
                        float* ch[2] = { l.data() + i, r.data() + i };
                        e.process (ch, std::min (block, n - i), true);
                        while (e.meterFifo().pop (f)) {}
                    }
                    el = std::min (el, std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count());
                }
                const double ns = el * 1e9 / n;
                const double pct = 100.0 * el / seconds;
                worstPct = std::max (worstPct, pct);
                worstNs = std::max (worstNs, ns);
                const char* meta = q == 1 ? (pct <= 1.0 ? "≤ 1 %  OK" : "> 1 % FALLA") : "";
                std::printf ("%-11s %-7s %-11s %12.1f %11.3f%% %10s\n", qn[q], pn[ph], name, ns, pct, meta);
            }
            results.push_back ({ qn[q], pn[ph], worstNs, worstPct });
        }
    if (! jsonPath.empty())
    {
        if (FILE* fp = std::fopen (jsonPath.c_str(), "w"))
        {
            std::fprintf (fp, "{\n \"fs\": %.0f, \"segundos\": %.0f, \"bloque\": %d, \"resultados\": [\n", fs, seconds, block);
            for (size_t i = 0; i < results.size(); ++i)
                std::fprintf (fp, "  {\"calidad\": \"%s\", \"fase\": \"%s\", \"ns_por_muestra\": %.2f, \"pct_nucleo\": %.4f}%s\n",
                              results[i].q, results[i].ph, results[i].nsPerFrame, results[i].pctCore, i + 1 < results.size() ? "," : "");
            std::fprintf (fp, " ]\n}\n");
            std::fclose (fp);
        }
    }
    // código de salida ≠ 0 si Normal (fase lineal) supera la meta: sirve para CI
    return results[1].pctCore <= 1.0 ? 0 : 2;
}
