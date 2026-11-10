// clip5 — datos de medición audio → UI. El hilo de audio solo acumula y empuja tramas de 10 ms
// a una FIFO lock-free de un productor / un consumidor. Los cálculos finales (LUFS M/S/I con
// gating, historial del display) se hacen en el hilo de la UI (LoudnessAccumulator).
#pragma once

#include "clip5/Common.h"

#include <atomic>
#include <vector>

namespace clip5
{
struct MeterFrame
{
    float inPeak[kMaxChannels] {};   // lineal, entrada del plugin
    float outPeak[kMaxChannels] {};  // lineal, salida final
    float outTruePeak = 0;           // lineal (4x + parábola), salida final
    float clipDb = 0;                // dB recortados (máximo de la trama)
    float densityDb = 0;             // ganancia de Densidad aplicada (dB) al nivel RMS de la trama
    float ceilingDb = 0;             // techo vigente (dBFS)
    float preClipDb = -120;          // nivel pre-recorte máximo (dBFS) para la zona coral del display
    double kIn = 0, kOut = 0;        // Σ de cuadrados K-ponderados (sumados sobre canales)
    int samples = 0;
    int channels = 2;
};

template <typename T, int Capacity>
class SpscFifo
{
    static_assert ((Capacity & (Capacity - 1)) == 0, "potencia de 2");

public:
    bool push (const T& v) noexcept
    {
        const auto w = write.load (std::memory_order_relaxed);
        if (w - read.load (std::memory_order_acquire) >= (unsigned) Capacity) return false;
        items[w & (Capacity - 1)] = v;
        write.store (w + 1, std::memory_order_release);
        return true;
    }
    bool pop (T& v) noexcept
    {
        const auto r = read.load (std::memory_order_relaxed);
        if (r == write.load (std::memory_order_acquire)) return false;
        v = items[r & (Capacity - 1)];
        read.store (r + 1, std::memory_order_release);
        return true;
    }

private:
    T items[Capacity] {};
    std::atomic<unsigned> write { 0 }, read { 0 };
};

using MeterFifo = SpscFifo<MeterFrame, 512>;

/** Reloj de recorte: máximo recorte y posición donde ocurrió (escrito por audio, leído por UI). */
struct ClipClock
{
    std::atomic<float> maxClipDb { 0.0f };
    std::atomic<double> atSeconds { 0.0 };
    std::atomic<bool> resetRequest { false };
};

/** Integración de sonoridad (ITU-R BS.1770-4 / EBU R128) a partir de tramas de 10 ms.
    Uso exclusivo del hilo de la UI (puede reservar memoria). */
class LoudnessAccumulator
{
public:
    void reset()
    {
        ring.assign (300, 0.0);
        ringSamples.assign (300, 0);
        head = 0; filled = 0;
        blocks.clear();
        sub100 = 0; sub100Samples = 0; subCount = 0;
        last100.assign (4, 0.0); last100Samples.assign (4, 0); l100 = 0;
    }

    /** Agrega una trama (ms = suma de cuadrados ya ponderada por canal). */
    void add (double sumSquares, int samples)
    {
        if (ring.empty()) reset();
        ring[(size_t) head] = sumSquares;
        ringSamples[(size_t) head] = samples;
        head = (head + 1) % 300;
        filled = std::min (filled + 1, 300);

        // bloques de 400 ms con 75 % de solape → uno nuevo cada 100 ms
        sub100 += sumSquares; sub100Samples += samples;
        if (++subCount == 10)
        {
            last100[(size_t) l100] = sub100; last100Samples[(size_t) l100] = sub100Samples;
            l100 = (l100 + 1) % 4;
            ++hundreds;
            if (hundreds >= 4)
            {
                double s = 0; long n = 0;
                for (int i = 0; i < 4; ++i) { s += last100[(size_t) i]; n += last100Samples[(size_t) i]; }
                if (n > 0) blocks.push_back (s / (double) n);
            }
            sub100 = 0; sub100Samples = 0; subCount = 0;
        }
    }

    double momentary() const { return windowLufs (40); }
    double shortTerm() const { return windowLufs (300); }

    double integrated() const
    {
        if (blocks.empty()) return -200.0;
        double s = 0; int n = 0;
        for (double z : blocks) if (toLufs (z) > -70.0) { s += z; ++n; }
        if (n == 0) return -200.0;
        const double rel = toLufs (s / n) - 10.0;
        s = 0; n = 0;
        for (double z : blocks) if (toLufs (z) > -70.0 && toLufs (z) > rel) { s += z; ++n; }
        return n == 0 ? -200.0 : toLufs (s / n);
    }

    static double toLufs (double meanSquare) { return meanSquare > 0 ? -0.691 + 10.0 * std::log10 (meanSquare) : -200.0; }

private:
    std::vector<double> ring;
    std::vector<int> ringSamples;
    int head = 0, filled = 0;
    std::vector<double> blocks;
    double sub100 = 0; long sub100Samples = 0; int subCount = 0;
    std::vector<double> last100; std::vector<long> last100Samples; int l100 = 0; long hundreds = 0;

    double windowLufs (int frames) const
    {
        if (filled < 1) return -200.0;
        const int n = std::min (frames, filled);
        double s = 0; long k = 0;
        for (int i = 1; i <= n; ++i)
        {
            const int idx = (head - i + 300) % 300;
            s += ring[(size_t) idx]; k += ringSamples[(size_t) idx];
        }
        return k > 0 ? toLufs (s / (double) k) : -200.0;
    }
};
} // namespace clip5
