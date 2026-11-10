#include "clip5/TableBank.h"

#include <chrono>

namespace clip5
{
TableBank::TableBank()
{
    slots[0].build (target());
    lastBuilt = slots[0].params;
}

TableBank::~TableBank() { stopWorker(); }

CurveParams TableBank::target() const noexcept
{
    CurveParams p;
    p.densityDb = targetDensity.load (std::memory_order_relaxed);
    p.mode = (ClipMode) targetMode.load (std::memory_order_relaxed);
    return p;
}

void TableBank::setTarget (const CurveParams& p) noexcept
{
    targetDensity.store (std::min (6.0f, std::max (0.0f, p.densityDb)), std::memory_order_relaxed);
    targetMode.store ((int) p.mode, std::memory_order_relaxed);
}

void TableBank::startWorker()
{
    if (running.exchange (true)) return;
    worker = std::thread ([this] { workerLoop(); });
}

void TableBank::stopWorker()
{
    if (! running.exchange (false)) return;
    if (worker.joinable()) worker.join();
}

int TableBank::findFreeSlot (unsigned mask, int pendingReady) const noexcept
{
    for (int s = 0; s < kWorkerSlots; ++s) // las ranuras 3 y 4 son exclusivas del render offline
        if (! (mask & (1u << s)) && s != pendingReady && s != lastPublished)
            return s;
    return -1;
}

void TableBank::workerLoop()
{
    while (running.load())
    {
        const CurveParams t = target();
        if (t != lastBuilt)
        {
            const unsigned mask = inUse.load (std::memory_order_acquire);
            const int pending = ready.load (std::memory_order_acquire);
            const int s = findFreeSlot (mask, pending);
            if (s >= 0)
            {
                slots[s].build (t);
                lastBuilt = t;
                ready.exchange (s, std::memory_order_acq_rel);
                lastPublished = s;
            }
        }
        std::this_thread::sleep_for (std::chrono::milliseconds (2));
    }
}

void TableBank::publishMask() noexcept
{
    unsigned m = 1u << cur;
    if (prev >= 0) m |= 1u << prev;
    inUse.store (m, std::memory_order_release);
}

void TableBank::beginBlock (bool realtime) noexcept
{
    if (prev >= 0) return; // un crossfade a la vez

    if (! realtime)
    {
        // offline: determinista, en este hilo (sin allocations)
        const CurveParams t = target();
        if (t != slots[cur].params)
        {
            int s = -1;
            for (int k = kWorkerSlots; k < kSlots; ++k)
                if (k != cur) { s = k; break; }
            if (s >= 0)
            {
                slots[s].build (t);
                prev = cur; cur = s; fadePos = 0;
                publishMask();
            }
        }
        return;
    }

    int r = ready.load (std::memory_order_acquire);
    if (r < 0 || r == cur) return;
    inUse.store ((1u << cur) | (1u << r), std::memory_order_release); // reservar antes de tomar
    if (ready.compare_exchange_strong (r, -1, std::memory_order_acq_rel))
    {
        prev = cur; cur = r; fadePos = 0;
    }
    publishMask();
}

float TableBank::process (float* buf, int n, float& uPrev, bool adaa) const noexcept
{
    const CurveTable& A = slots[cur];
    float peak = 0.0f, u1 = uPrev;

    if (prev < 0)
    {
        if (adaa)
            for (int i = 0; i < n; ++i)
            {
                const float u = buf[i];
                peak = std::max (peak, std::abs (u));
                buf[i] = A.adaa (u, u1);
                u1 = u;
            }
        else
            for (int i = 0; i < n; ++i)
            {
                const float u = buf[i];
                peak = std::max (peak, std::abs (u));
                buf[i] = A.eval (u);
                u1 = u;
            }
    }
    else
    {
        const CurveTable& B = slots[prev];
        const float inv = 1.0f / (float) fadeLen;
        for (int i = 0; i < n; ++i)
        {
            const float u = buf[i];
            peak = std::max (peak, std::abs (u));
            const float w = std::min (1.0f, (float) (fadePos + i) * inv);
            const float ya = adaa ? A.adaa (u, u1) : A.eval (u);
            const float yb = adaa ? B.adaa (u, u1) : B.eval (u);
            buf[i] = yb + w * (ya - yb);
            u1 = u;
        }
    }
    uPrev = u1;
    return peak;
}

void TableBank::endBlock (int n) noexcept
{
    if (prev < 0) return;
    fadePos += n;
    if (fadePos >= fadeLen)
    {
        prev = -1;
        publishMask();
    }
}
} // namespace clip5
