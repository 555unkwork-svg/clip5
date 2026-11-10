// clip5 — motor completo (cadena de señal de la especificación, fases 2–4).
//
//  Entrada → [seco: retardo = latencia húmeda] ──────────────────────────────┐
//          → 1/C → ↑N → núcleo fusionado (tabla + ADAA) → Suavizado → ↓N + Tono (FIR único,
//            absorbe caída y ½ muestra del ADAA) → C·(1/g₀ si Ganancia unitaria) → DC (Suave)
//          → HÚMEDO ──(+)── SECO → Salida → Techo estricto (true peak, lookahead) → medición
//
// Sin allocations, locks ni I/O en process(). Todo se reserva en prepare().
#pragma once

#include "clip5/Dynamics.h"
#include "clip5/Filters.h"
#include "clip5/Metering.h"
#include "clip5/TableBank.h"

#include <array>
#include <memory>

namespace clip5
{
enum class Quality : int { Eco = 0, Normal = 1, Alta = 2, Maxima = 3 };
enum class PhaseMode : int { Lineal = 0, Minima = 1 };

constexpr int kNumQualities = 4;
constexpr int kNumTonos = 3; // Plano, Seda, Terciopelo
constexpr float kSilenceDb = -120.0f;

struct EngineParams
{
    float inputDb = 0, outputDb = 0;
    bool ioLink = false;
    float ceilingDb = -1.0f;      // 0 … −36 dBFS
    float densityDb = 0;          // 0 … +6
    ClipMode mode = ClipMode::Firme;
    float suavizado = 0;          // 0 … 1
    bool suavizadoExtendido = false;
    int tono = 0;
    bool strictCeiling = true;
    bool unitGain = false;
    float wetDb = 0, dryDb = kSilenceDb; // ≤ −120 dB = −∞
    int qualityRealtime = (int) Quality::Normal;
    int qualityOffline = -1;     // −1 = igual que en tiempo real
    int phase = (int) PhaseMode::Lineal;
    bool adaa = true;            // siempre activo en el plugin (desactivable para mediciones)
    bool meteringEnabled = true; // el benchmark puede apagarlo para aislar el costo
};

/** Coeficientes de todas las configuraciones para una fs (se calculan una vez por fs). */
struct ChainDesigns
{
    double fs = 0;
    HalfbandKernels hb[3];
    IirHalfbandCoefs iir[3];
    DecimKernels decim[kNumQualities][kNumTonos]; // q = 1..3
    FirKernel fir1[kNumTonos];                    // q = 0, fase lineal
    design::BiquadCoefs tonoBq[kNumTonos];
    design::BiquadCoefs compBq[kNumQualities];
    int latency[kNumQualities][2] {};
    double tonoFitErrDb = 0, compFitErrDb[kNumQualities] {};
    int lsTaps = 0;

    void build (double sampleRate);
};

class Engine
{
public:
    static constexpr int kSub = 256; // sub-bloque interno
    static constexpr int kMaxOs = 8;

    Engine();
    ~Engine();

    void prepare (double sampleRate, int numChannels);
    void reset();

    void setParams (const EngineParams& p) noexcept { params = p; }

    /** hostSeconds < 0 → sin transporte: se usa un contador interno para el Reloj de recorte. */
    void process (float* const* channels, int numSamples, bool realtime, double hostSeconds = -1.0) noexcept;

    int latencySamples() const noexcept { return totalLatency; }
    int latencyFor (int quality, int phase) const noexcept;
    int activeQuality() const noexcept { return activeQ; }
    int activePhase() const noexcept { return activeP; }
    const ChainDesigns& designs() const noexcept { return *des; }

    MeterFifo& meterFifo() noexcept { return fifo; }
    ClipClock& clipClock() noexcept { return clock; }
    TableBank& tables() noexcept { return bank; }

    void startBackgroundWorker() { bank.startWorker(); }
    void stopBackgroundWorker() { bank.stopWorker(); }

    /** Para pruebas: procesa con la configuración forzada (sin fundido de cambio). */
    void forceConfig (int quality, int phase);

private:
    struct ChannelChain
    {
        HalfbandFirUp fu[3];
        HalfbandFirDown fd[3];
        HalfbandIirUp iu[3];
        HalfbandIirDown id[3];
        FirDecimator2 dec;
        FirFilter fir1;
        Biquad tonoA, tonoB, comp;
        Suavizado suav;
        DcBlocker dc;
        float uPrev = 0;
        void reset();
    };

    void bindConfig (int q, int ph);
    void processSub (float* const* ch, int n, bool realtime, double t0) noexcept;
    void runWet (int c, float* x, int n, float depth0, float depth1, float& uPeak) noexcept;
    void startTonoFade (int newTono) noexcept;

    std::unique_ptr<ChainDesigns> des;
    ChannelChain chain[kMaxChannels];
    TableBank bank;
    StrictCeiling ceiling;
    BiquadF kInA[kMaxChannels], kInB[kMaxChannels], kOutA[kMaxChannels], kOutB[kMaxChannels];

    EngineParams params;
    double fs = 48000;
    int nch = 2, activeQ = 1, activeP = 0, osFactor = 2, wetLatency = 0, totalLatency = 0;
    int activeTono = 0, pendingTono = -1, tonoFadePos = 0, tonoFadeLen = 960;

    // fundido para cambios de calidad / fase (cambian latencia y estados)
    int switchState = 0; // 0 nada, 1 bajando, 2 subiendo
    float switchGain = 1.0f, switchStep = 0.0f;
    int pendingQ = -1, pendingP = -1;

    LinearRamp inGain, invCeil, ceil, wetGain, dryGain, outGain, unitGain, suavDepth;

    // buffers
    alignas (16) float xbuf[kMaxChannels][kSub] {};
    alignas (16) float wbuf[kMaxChannels][kSub] {};
    alignas (16) float os[kSub * kMaxOs] {};
    alignas (16) float os2[kSub * kMaxOs] {};
    alignas (16) float uabs[kSub * kMaxOs] {};
    alignas (16) float rampIn[kSub] {}, rampInvC[kSub] {}, rampC[kSub] {}, rampWet[kSub] {}, rampDry[kSub] {}, rampOut[kSub] {}, rampUnit[kSub] {};
    static constexpr int kDryRing = 8192;
    float dryRing[kMaxChannels][kDryRing] {};
    uint32_t dryPos = 0;

    // silencio
    long silentRun = 0;
    bool sleeping = false;

    // medición
    MeterFifo fifo;
    ClipClock clock;
    MeterFrame frame;
    int frameLen = 480;
    double frameUSq = 0; long frameUN = 0;
    double internalSeconds = 0;
};
} // namespace clip5
