#include "clip5/Engine.h"

#include <complex>
#include <vector>

namespace clip5
{
namespace
{
    float faderGain (float db) noexcept { return db <= kSilenceDb + 0.5f ? 0.0f : dbToGain (db); }

    double passBandEdge (double fs) { return std::min (20000.0, 0.45 * fs); }

    double adaaDroopComp (double f, double fsOs) { return 1.0 / std::cos (kPi * f / fsOs); }
}

// =====================================================================================
//  Diseños por frecuencia de muestreo
// =====================================================================================
void ChainDesigns::build (double sampleRate)
{
    fs = sampleRate;
    const double fPass = passBandEdge (fs);

    double hbDelayBase[3];
    for (int s = 0; s < 3; ++s)
    {
        const double fsH = fs * (1 << (s + 1));
        hb[s].design (design::kaiserHalfband (fsH, fPass, 118.0));
        hbDelayBase[s] = hb[s].delayHigh() / (double) (1 << (s + 1));
        iir[s].set (design::iirHalfband ((fsH / 4 - fPass) / fsH, 110.0));
    }

    // ---- fase lineal, N ≥ 2: FIR decimador final a 2·fs con Tono + compensación del ADAA
    const double fs2 = 2 * fs;
    // rechazo desde fs/2: nada se pliega dentro de la banda de audio y la salida húmeda queda
    // libre de contenido cerca de Nyquist (true peak predecible por el Techo estricto)
    const double fStop = fs / 2;
    const double trans = fStop - fPass;
    lsTaps = std::min (256, std::max (64, roundUp4 ((int) std::ceil (68.0 * 8000.0 / trans))));
    for (int q = 1; q < kNumQualities; ++q)
    {
        const int N = 1 << q;
        double A = 0.5 / N;
        for (int s = 0; s < q; ++s) A += hbDelayBase[s];     // subida
        for (int s = 1; s < q; ++s) A += hbDelayBase[s];     // bajada (half-band)
        const double D0 = (lsTaps - 1) / 2.0;
        const int T = (int) std::lround (A + D0 / 2.0);
        const double D = 2.0 * (T - A);
        latency[q][0] = T;
        for (int t = 0; t < kNumTonos; ++t)
        {
            auto target = [t, N, fsl = fs] (double f) {
                return std::pow (10.0, design::tonoDb (t, f) / 20.0) * adaaDroopComp (f, N * fsl);
            };
            decim[q][t].design (design::leastSquaresFir (lsTaps, fs2, fPass, fStop, target, D, 1e5));
        }
    }

    // ---- fase lineal, N = 1 (Eco): FIR corto a fs, retardo 15,5 + ½ (ADAA) = 16
    {
        const int taps = 32;
        const double fP = std::min (19000.0, 0.4 * fs);
        latency[0][0] = 16;
        for (int t = 0; t < kNumTonos; ++t)
        {
            auto target = [t, fsl = fs] (double f) {
                return std::pow (10.0, design::tonoDb (t, f) / 20.0) * adaaDroopComp (f, fsl);
            };
            std::vector<double> g = design::leastSquaresFir (taps, fs, fP, 0.49 * fs, target, 15.5, 10.0);
            fir1[t].setFromTaps (g);
        }
    }

    // ---- fase mínima: biquads a tasa base
    tonoFitErrDb = 0;
    for (int t = 0; t < kNumTonos; ++t)
    {
        if (t == 0) { tonoBq[t] = {}; continue; }
        double e = 0;
        tonoBq[t] = design::fitHighShelf (fs, [t] (double f) { return design::tonoDb (t, f); }, &e);
        tonoFitErrDb = std::max (tonoFitErrDb, e);
    }
    for (int q = 0; q < kNumQualities; ++q)
    {
        const double fsOs = fs * (1 << q);
        compBq[q] = design::fitHighShelf (fs, [=] (double f) { return 20.0 * std::log10 (adaaDroopComp (f, fsOs)); },
                                          &compFitErrDb[q]);
    }
}

// =====================================================================================
//  Engine
// =====================================================================================
void Engine::ChannelChain::reset()
{
    for (auto& s : fu) if (s.k) s.reset();
    for (auto& s : fd) if (s.k) s.reset();
    for (auto& s : iu) if (s.ch.c) s.reset();
    for (auto& s : id) if (s.ch.c) s.reset();
    if (dec.a) dec.reset();
    if (fir1.a) fir1.reset();
    tonoA.reset(); tonoB.reset(); comp.reset();
    suav.reset();
    dc.reset();
    uPrev = 0;
}

Engine::Engine() : des (std::make_unique<ChainDesigns>()) {}
Engine::~Engine() { bank.stopWorker(); }

int Engine::latencyFor (int q, int ph) const noexcept
{
    return des->latency[std::clamp (q, 0, 3)][std::clamp (ph, 0, 1)] + (activeStrict ? ceiling.latency() : 0);
}

void Engine::prepare (double sampleRate, int numChannels)
{
    fs = sampleRate;
    nch = std::clamp (numChannels, 1, kMaxChannels);
    if (des->fs != fs)
    {
        des->build (fs);

        // latencia de fase mínima: retardo de fase medido a 200 Hz con un impulso
        for (int q = 0; q < kNumQualities; ++q)
        {
            auto tmp = std::make_unique<ChannelChain>();
            const int N = 1 << q;
            for (int s = 0; s < q; ++s) { tmp->iu[s].bind (&des->iir[s]); tmp->id[s].bind (&des->iir[s]); }
            tmp->comp.c = des->compBq[q];
            const int len = 4096;
            std::vector<float> a ((size_t) (len * N)), b ((size_t) (len * N)), out ((size_t) len);
            std::vector<float> x ((size_t) len, 0.0f);
            x[0] = 1.0f;
            std::copy (x.begin(), x.end(), a.begin());
            int L = len;
            float* cur = a.data(); float* nxt = b.data();
            for (int s = 0; s < q; ++s) { tmp->iu[s].process (cur, L, nxt); std::swap (cur, nxt); L *= 2; }
            float prev = 0;
            for (int i = 0; i < L; ++i) { const float v = cur[i]; cur[i] = 0.5f * (v + prev); prev = v; } // ADAA lineal
            for (int s = q - 1; s >= 0; --s) { L /= 2; tmp->id[s].process (cur, L, nxt); std::swap (cur, nxt); }
            for (int i = 0; i < len; ++i) out[(size_t) i] = tmp->comp.process (cur[i]);
            const double w = 2 * kPi * 200.0 / fs;
            std::complex<double> H = 0;
            for (int i = 0; i < len; ++i) H += (double) out[(size_t) i] * std::polar (1.0, -w * i);
            const double phaseDelay = -std::arg (H) / w;
            des->latency[q][1] = std::max (0, (int) std::lround (phaseDelay));
        }
    }

    ceiling.prepare (fs, nch);
    for (int c = 0; c < kMaxChannels; ++c)
    {
        design::BiquadCoefs sh, hp;
        design::kWeighting (fs, sh, hp);
        kInA[c].set (sh); kInB[c].set (hp); kOutA[c].set (sh); kOutB[c].set (hp);
        chain[c].dc.prepare (fs);
    }
    frameLen = std::max (1, (int) std::lround (0.01 * fs));
    tonoFadeLen = std::max (1, (int) std::lround (0.02 * fs));
    activeTono = std::clamp (params.tono, 0, kNumTonos - 1);
    pendingTono = -1;

    activeStrict = params.strictCeiling;
    bindConfig (std::clamp (params.qualityRealtime, 0, 3), std::clamp (params.phase, 0, 1));
    reset();
}

void Engine::forceConfig (int q, int ph)
{
    bindConfig (q, ph);
    reset();
}

void Engine::bindConfig (int q, int ph)
{
    activeQ = q;
    activeP = ph;
    osFactor = 1 << q;
    for (int c = 0; c < kMaxChannels; ++c)
    {
        auto& ch = chain[c];
        for (int s = 0; s < 3; ++s)
        {
            ch.fu[s].bind (&des->hb[s]);
            ch.fd[s].bind (&des->hb[s]);
            ch.iu[s].bind (&des->iir[s]);
            ch.id[s].bind (&des->iir[s]);
        }
        ch.dec.bind (&des->decim[std::max (1, q)][activeTono]);
        ch.fir1.bind (&des->fir1[activeTono]);
        ch.tonoA.c = des->tonoBq[activeTono];
        ch.comp.c = des->compBq[q];
        ch.suav.prepare (fs * osFactor);
        ch.reset();
    }
    pendingTono = -1;
    wetLatency = des->latency[q][ph];
    totalLatency = wetLatency + (activeStrict ? ceiling.latency() : 0);
    bank.setFadeLength ((int) std::lround (0.02 * fs * osFactor));
}

void Engine::applyPendingConfig() noexcept
{
    if (pendingStrict != activeStrict)
    {
        activeStrict = pendingStrict;
        ceiling.reset();
    }
    bindConfig (pendingQ, pendingP);
}

void Engine::reset()
{
    for (auto& ch : chain) ch.reset();
    ceiling.reset();
    for (int c = 0; c < kMaxChannels; ++c)
    {
        kInA[c].reset(); kInB[c].reset(); kOutA[c].reset(); kOutB[c].reset();
    }
    std::memset (dryRing, 0, sizeof (dryRing));
    dryPos = 0;
    const EngineParams& p = params;
    inGain.reset (dbToGain (p.inputDb));
    const float C = dbToGain (p.ceilingDb);
    invCeil.reset (1.0f / C);
    ceil.reset (C);
    wetGain.reset (faderGain (p.wetDb));
    dryGain.reset (faderGain (p.dryDb));
    outGain.reset (dbToGain (p.outputDb));
    linkGain.reset (p.ioLink ? dbToGain (-p.inputDb) : 1.0f);
    unitGain.reset (p.unitGain ? 1.0f / dbToGain (p.densityDb) : 1.0f);
    suavDepth.reset (std::clamp (p.suavizado, 0.0f, 1.0f) * (p.suavizadoExtendido ? 3.0f : 2.0f));
    switchState = 0; switchGain = 1.0f;
    silentRun = 0; sleeping = false;
    frame = {};
    frameUSq = 0; frameUN = 0;
    bank.setTarget ({ p.densityDb, p.mode });
    for (auto& ch : chain) { ch.dc.weightTarget = p.mode == ClipMode::Suave ? 1.0f : 0.0f; ch.dc.weight = ch.dc.weightTarget; }
}

void Engine::startTonoFade (int t) noexcept
{
    pendingTono = t;
    tonoFadePos = 0;
    for (auto& ch : chain)
    {
        ch.dec.b = &des->decim[std::max (1, activeQ)][t];
        ch.fir1.b = &des->fir1[t];
        ch.tonoB.c = des->tonoBq[t];
        ch.tonoB.s1 = ch.tonoA.s1; ch.tonoB.s2 = ch.tonoA.s2;
    }
}

// =====================================================================================
void Engine::process (float* const* ch, int numSamples, bool realtime, double hostSeconds) noexcept
{
    // ----- detección de silencio digital (respeta la latencia: se duerme solo con colas vacías)
    bool silent = true;
    for (int c = 0; c < nch && silent; ++c)
        for (int i = 0; i < numSamples; ++i)
            if (ch[c][i] != 0.0f) { silent = false; break; }
    silentRun = silent ? silentRun + numSamples : 0;
    const bool canSleep = silentRun > (long) totalLatency + (long) (0.5 * fs);
    if (canSleep)
    {
        if (! sleeping) { sleeping = true; }
        for (int c = 0; c < nch; ++c) std::memset (ch[c], 0, sizeof (float) * (size_t) numSamples);
        // tramas de medición en silencio para que la UI siga avanzando
        frame.samples += numSamples;
        while (frame.samples >= frameLen)
        {
            MeterFrame f {};
            f.samples = frameLen; f.channels = nch; f.ceilingDb = params.ceilingDb;
            if (params.meteringEnabled) fifo.push (f);
            frame.samples -= frameLen;
        }
        internalSeconds += numSamples / fs;
        return;
    }
    if (sleeping)
    {
        sleeping = false;
        reset(); // estados exactamente en cero tras el silencio
    }

    int done = 0;
    while (done < numSamples)
    {
        const int n = std::min (kSub, numSamples - done);
        float* sub[kMaxChannels];
        for (int c = 0; c < nch; ++c) sub[c] = ch[c] + done;
        const double t0 = hostSeconds >= 0 ? hostSeconds + done / fs : internalSeconds;
        processSub (sub, n, realtime, t0);
        internalSeconds += n / fs;
        done += n;
    }
}

void Engine::runWet (int c, float* x, int n, float depth0, float depth1, float& uPeak) noexcept
{
    ChannelChain& C = chain[c];
    const int q = activeQ, N = osFactor;
    const bool lin = activeP == (int) PhaseMode::Lineal;

    // ↑N
    float* cur = os;
    float* nxt = os2;
    int L = n;
    if (N == 1) std::memcpy (cur, x, sizeof (float) * (size_t) n);
    else
    {
        if (lin) C.fu[0].process (x, n, cur); else C.iu[0].process (x, n, cur);
        L = 2 * n;
        for (int s = 1; s < q; ++s)
        {
            if (lin) C.fu[s].process (cur, L, nxt); else C.iu[s].process (cur, L, nxt);
            std::swap (cur, nxt);
            L *= 2;
        }
    }

    // núcleo fusionado (+ |u| para el detector del Suavizado, costo cero)
    for (int i = 0; i < L; ++i) uabs[i] = std::abs (cur[i]);
    uPeak = std::max (uPeak, bank.process (cur, L, C.uPrev, params.adaa));

    // Suavizado a tasa sobremuestreada
    C.suav.process (cur, uabs, L, depth0, depth1);

    // ↓N + Tono
    const float fade = pendingTono >= 0 ? (float) tonoFadePos / (float) tonoFadeLen : 0.0f;
    const float fstep = pendingTono >= 0 ? 1.0f / (float) tonoFadeLen : 0.0f;
    if (lin)
    {
        for (int s = q - 1; s >= 1; --s)
        {
            L /= 2;
            C.fd[s].process (cur, L, nxt);
            std::swap (cur, nxt);
        }
        if (N == 1) C.fir1.process (cur, n, x, fade, fstep);
        else C.dec.process (cur, n, x, fade, fstep);
    }
    else
    {
        for (int s = q - 1; s >= 0; --s)
        {
            L /= 2;
            C.id[s].process (cur, L, nxt);
            std::swap (cur, nxt);
        }
        float fd = fade;
        for (int i = 0; i < n; ++i)
        {
            float y = C.comp.process (cur[i]);
            const float ya = C.tonoA.process (y);
            if (pendingTono >= 0)
            {
                const float yb = C.tonoB.process (y);
                y = ya + std::min (1.0f, fd) * (yb - ya);
                fd += fstep;
            }
            else y = ya;
            x[i] = y;
        }
    }
}

void Engine::processSub (float* const* ch, int n, bool realtime, double t0) noexcept
{
    const EngineParams& p = params;

    // ----- cambio de calidad / fase (cambia latencia): fundido de salida, cambio, fundido de entrada
    const int wantQ = std::clamp (realtime ? p.qualityRealtime : (p.qualityOffline < 0 ? p.qualityRealtime : p.qualityOffline), 0, 3);
    const int wantP = std::clamp (p.phase, 0, 1);
    if (switchState == 0 && (wantQ != activeQ || wantP != activeP || p.strictCeiling != activeStrict))
    {
        pendingQ = wantQ; pendingP = wantP; pendingStrict = p.strictCeiling;
        if (! realtime) applyPendingConfig();
        else switchState = 1;
    }
    // ganancia del cambio por muestra: 1 → fundido de salida → silencio durante toda la nueva
    // latencia (las líneas de retardo y los filtros se llenan) → fundido de entrada
    bool switching = switchState != 0;
    for (int i = 0; i < n; ++i)
    {
        float g = 1.0f;
        if (switchState == 1) g = 1.0f - (float) (i + 1) / (float) n;
        else if (switchState == 2) { g = 0.0f; if (--switchHold <= 0) { switchState = 3; switchFade = 0; } }
        else if (switchState == 4) { g = 0.0f; --switchHold; } // vaciado del retardo del Techo estricto
        else if (switchState == 3) { g = (float) (++switchFade) / (float) kSwitchFade; if (switchFade >= kSwitchFade) switchState = 0; }
        rampSw[i] = g;
    }

    // ----- Tono
    if (pendingTono < 0 && p.tono != activeTono) startTonoFade (std::clamp (p.tono, 0, kNumTonos - 1));

    // ----- tablas del núcleo
    bank.setTarget ({ p.densityDb, p.mode });
    bank.beginBlock (realtime);
    for (auto& c : chain) c.dc.weightTarget = p.mode == ClipMode::Suave ? 1.0f : 0.0f;

    // ----- rampas por bloque
    const float C = dbToGain (std::clamp (p.ceilingDb, -36.0f, 0.0f));
    inGain.setTarget (dbToGain (p.inputDb), n);
    invCeil.setTarget (1.0f / C, n);
    ceil.setTarget (C, n);
    wetGain.setTarget (faderGain (p.wetDb), n);
    dryGain.setTarget (faderGain (p.dryDb), n);
    outGain.setTarget (dbToGain (p.outputDb), n);
    linkGain.setTarget (p.ioLink ? dbToGain (-p.inputDb) : 1.0f, n);
    unitGain.setTarget (p.unitGain ? 1.0f / dbToGain (std::clamp (p.densityDb, 0.0f, 6.0f)) : 1.0f, n);
    for (int i = 0; i < n; ++i)
    {
        rampIn[i] = inGain.next(); rampInvC[i] = invCeil.next(); rampC[i] = ceil.next();
        rampWet[i] = wetGain.next(); rampDry[i] = dryGain.next(); rampOut[i] = outGain.next();
        rampUnit[i] = unitGain.next(); rampLink[i] = linkGain.next();
    }
    const float depth0 = suavDepth.current;
    suavDepth.setTarget (std::clamp (p.suavizado, 0.0f, 1.0f) * (p.suavizadoExtendido ? 3.0f : 2.0f), n);
    for (int i = 0; i < n; ++i) suavDepth.next();
    const float depth1 = suavDepth.current;

    float uPeak = 0.0f;
    float inPk[kMaxChannels] {}, outPk[kMaxChannels] {};
    float outTpk = 0;
    double kin = 0, kout = 0;

    for (int c = 0; c < nch; ++c)
    {
        float* io = ch[c];
        float* x = xbuf[c];
        for (int i = 0; i < n; ++i)
        {
            const float s = io[i];
            inPk[c] = std::max (inPk[c], std::abs (s));
            if (p.meteringEnabled)
            {
                const float z = kInB[c].process (kInA[c].process (s));
                kin += (double) z * z;
            }
            // seco: señal original (antes de Entrada) → Entrada solo excita el camino húmedo
            dryRing[c][(dryPos + (uint32_t) i) & (kDryRing - 1)] = s;
            x[i] = s * rampIn[i];
        }

        // húmedo
        float* w = wbuf[c];
        for (int i = 0; i < n; ++i)
        {
            w[i] = x[i] * rampInvC[i];
            frameUSq += (double) w[i] * w[i];
        }
        frameUN += n;
        runWet (c, w, n, depth0, depth1, uPeak);
        for (int i = 0; i < n; ++i) w[i] *= rampC[i] * rampUnit[i] * rampLink[i];
        chain[c].dc.process (w, n);

        // mezcla paralela + Salida
        for (int i = 0; i < n; ++i)
        {
            const float d = dryRing[c][(dryPos + (uint32_t) i - (uint32_t) wetLatency) & (kDryRing - 1)];
            io[i] = (w[i] * rampWet[i] + d * rampDry[i]) * rampOut[i] * (switching ? rampSw[i] : 1.0f);
        }
    }
    dryPos += (uint32_t) n;
    bank.endBlock (n * osFactor);

    if (pendingTono >= 0)
    {
        tonoFadePos += n;
        if (tonoFadePos >= tonoFadeLen)
        {
            for (auto& c : chain)
            {
                c.dec.a = c.dec.b; c.dec.b = nullptr;
                c.fir1.a = c.fir1.b; c.fir1.b = nullptr;
                c.tonoA = c.tonoB;
            }
            activeTono = pendingTono;
            pendingTono = -1;
        }
    }

    // ----- Techo estricto (al final de la cadena: garantía absoluta), latencia constante
    ceiling.process (ch, n, C, activeStrict);

    if (switchState == 1)
    {
        switchState = 4;
        switchHold = activeStrict ? ceiling.latency() : 0;
    }
    if (switchState == 4 && switchHold <= 0)
    {
        applyPendingConfig();
        switchState = 2;
        switchHold = totalLatency + 64;
    }

    // ----- medición de salida (el true peak lo entrega el Techo estricto, que ya lo detecta)
    if (! p.meteringEnabled) return;
    outTpk = ceiling.lastTruePeak();
    for (int c = 0; c < nch; ++c)
        for (int i = 0; i < n; ++i)
        {
            const float s = ch[c][i];
            outPk[c] = std::max (outPk[c], std::abs (s));
            const float z = kOutB[c].process (kOutA[c].process (s));
            kout += (double) z * z;
        }

    if (clock.resetRequest.exchange (false)) { clock.maxClipDb.store (0.0f); clock.atSeconds.store (0.0); }
    const float clipDb = (p.mode == ClipMode::Apagado || uPeak <= 1.0f) ? 0.0f : 20.0f * std::log10 (uPeak);
    if (clipDb > clock.maxClipDb.load (std::memory_order_relaxed))
    {
        clock.maxClipDb.store (clipDb);
        clock.atSeconds.store (t0);
    }

    for (int c = 0; c < nch; ++c)
    {
        frame.inPeak[c] = std::max (frame.inPeak[c], inPk[c]);
        frame.outPeak[c] = std::max (frame.outPeak[c], outPk[c]);
    }
    frame.outTruePeak = std::max (frame.outTruePeak, outTpk);
    frame.clipDb = std::max (frame.clipDb, clipDb);
    frame.preClipDb = std::max (frame.preClipDb, p.ceilingDb + gainToDb (uPeak, -120.0f));
    frame.kIn += kin;
    frame.kOut += kout;
    frame.samples += n;
    if (frame.samples >= frameLen)
    {
        frame.channels = nch;
        frame.ceilingDb = p.ceilingDb;
        const double uRms = frameUN > 0 ? std::sqrt (frameUSq / (double) frameUN) : 0.0;
        frame.densityDb = (float) (20.0 * std::log10 (curves::densityGain (std::min (1.0, uRms * 1.41421356), p.densityDb)));
        fifo.push (frame);
        frame = {};
        frameUSq = 0; frameUN = 0;
    }
}
} // namespace clip5
