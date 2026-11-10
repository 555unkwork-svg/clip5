// Medición: LUFS (BS.1770-4), true peak, Reloj de recorte.
#include "clip5/Engine.h"
#include "util.h"

#include <catch2/catch_test_macros.hpp>

using namespace clip5;

TEST_CASE ("LUFS: senoide de 997 Hz a −20 dBFS en estéreo = −20 LUFS (±0,1)", "[medicion]")
{
    const double fs = 48000;
    Engine e;
    EngineParams p;
    p.mode = ClipMode::Apagado; p.ceilingDb = 0; p.strictCeiling = false;
    e.setParams (p);
    e.prepare (fs, 2);
    const int n = (int) fs * 5;
    auto x = testutil::sine (997.0, fs, n, std::pow (10.0, -20.0 / 20));
    std::vector<float> l (x), r (x);
    for (int i = 0; i < n; i += 480)
    {
        float* ch[2] = { l.data() + i, r.data() + i };
        e.process (ch, std::min (480, n - i), true);
    }
    LoudnessAccumulator in, out;
    in.reset(); out.reset();
    MeterFrame f;
    int frames = 0;
    while (e.meterFifo().pop (f)) { in.add (f.kIn, f.samples); out.add (f.kOut, f.samples); ++frames; }
    CAPTURE (frames, in.momentary(), in.shortTerm(), in.integrated(), out.integrated());
    CHECK (std::abs (in.momentary() - (-20.0)) < 0.1);
    CHECK (std::abs (in.shortTerm() - (-20.0)) < 0.1);
    CHECK (std::abs (in.integrated() - (-20.0)) < 0.1);
    CHECK (std::abs (out.integrated() - (-20.0)) < 0.1);
}

TEST_CASE ("True peak: senoide a fs/4 con fase de 45° (pico entre muestras +3 dB)", "[medicion]")
{
    TruePeakDetector d;
    d.prepare();
    float m = 0, sp = 0;
    for (int i = 0; i < 4800; ++i)
    {
        const float x = (float) std::sin (2 * kPi * 12000.0 * i / 48000.0 + kPi / 4);
        sp = std::max (sp, std::abs (x));
        m = std::max (m, d.push (x));
    }
    CAPTURE (testutil::db (sp), testutil::db (m));
    CHECK (testutil::db (sp) < -2.9);           // los picos de muestra quedan en −3 dB
    CHECK (std::abs (testutil::db (m)) < 0.2);  // el true peak recupera ~0 dB
}

TEST_CASE ("Reloj de recorte: máximo y posición; reinicio", "[medicion]")
{
    const double fs = 48000;
    Engine e;
    EngineParams p;
    p.mode = ClipMode::Duro; p.ceilingDb = -6;
    e.setParams (p);
    e.prepare (fs, 2);
    const int n = (int) fs * 3;
    std::vector<float> x ((size_t) n);
    for (int i = 0; i < n; ++i)
    {
        const double amp = (i > fs * 2 && i < fs * 2.1) ? 1.0 : 0.25; // pico de +6 dB sobre el techo en t≈2 s
        x[(size_t) i] = (float) (amp * std::sin (2 * kPi * 500 * i / fs));
    }
    std::vector<float> r (x);
    for (int i = 0; i < n; i += 512)
    {
        float* ch[2] = { x.data() + i, r.data() + i };
        e.process (ch, std::min (512, n - i), true, 10.0 + i / fs); // transporte del host desde 10 s
    }
    const float maxDb = e.clipClock().maxClipDb.load();
    const double at = e.clipClock().atSeconds.load();
    CAPTURE (maxDb, at);
    CHECK (std::abs (maxDb - 6.0f) < 0.1f);
    CHECK (at > 11.99);
    CHECK (at < 12.11);
    e.clipClock().resetRequest.store (true);
    float* ch[2] = { x.data(), r.data() };
    std::fill (x.begin(), x.begin() + 512, 0.0f);
    std::fill (r.begin(), r.begin() + 512, 0.0f);
    e.process (ch, 512, true, 0.0);
    CHECK (e.clipClock().maxClipDb.load() == 0.0f);
}
