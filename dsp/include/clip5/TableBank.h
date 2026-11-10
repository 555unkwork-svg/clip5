// clip5 — banco de tablas del núcleo fusionado con generación en segundo plano y crossfade.
//
// Protocolo sin locks (5 ranuras preasignadas):
//  · El hilo de fondo sondea el objetivo (atómicos) cada 2 ms; si cambió, construye la tabla
//    en una ranura libre y la publica con ready.exchange(slot).
//  · El hilo de audio, al inicio de cada bloque, marca la ranura publicada en `inUse` ANTES de
//    tomarla con compare_exchange; así el hilo de fondo nunca reutiliza una ranura que el audio
//    puede estar leyendo. Al terminar el crossfade (~20 ms) la ranura anterior se libera.
//  · En render offline (isNonRealtime) la tabla se construye en el mismo hilo de audio, de forma
//    determinista y sin allocations, en las ranuras 3–4 (el hilo de fondo solo usa 0–2).
#pragma once

#include "clip5/CurveTable.h"

#include <atomic>
#include <thread>

namespace clip5
{
class TableBank
{
public:
    static constexpr int kSlots = 5;
    static constexpr int kWorkerSlots = 3; // 0..2 hilo de fondo, 3..4 render offline

    TableBank();
    ~TableBank();

    void startWorker();
    void stopWorker();

    /** Cualquier hilo. */
    void setTarget (const CurveParams& p) noexcept;

    /** Hilo de audio: duración del crossfade en muestras (a la tasa a la que corre el núcleo). */
    void setFadeLength (int samples) noexcept { fadeLen = std::max (1, samples); }

    /** Hilo de audio, al inicio de cada bloque. */
    void beginBlock (bool realtime) noexcept;

    /** Procesa un canal in-place (u → f(u)) con ADAA opcional. Devuelve max |u| visto.
        Todos los canales de un bloque comparten la misma posición de crossfade. */
    float process (float* buf, int n, float& uPrev, bool adaa) const noexcept;

    /** Hilo de audio, al final del bloque: avanza el crossfade. */
    void endBlock (int n) noexcept;

    const CurveTable& current() const noexcept { return slots[cur]; }
    bool isCrossfading() const noexcept { return prev >= 0; }

private:
    CurveTable slots[kSlots];
    int cur = 0, prev = -1, fadePos = 0, fadeLen = 1920;

    std::atomic<float> targetDensity { 0.0f };
    std::atomic<int> targetMode { (int) ClipMode::Firme };
    std::atomic<int> ready { -1 };
    std::atomic<unsigned> inUse { 1u };

    std::atomic<bool> running { false };
    std::thread worker;
    int lastPublished = -1;
    CurveParams lastBuilt {};

    CurveParams target() const noexcept;
    void workerLoop();
    int findFreeSlot (unsigned mask, int pendingReady) const noexcept;
    void publishMask() noexcept;
};
} // namespace clip5
