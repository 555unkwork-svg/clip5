// clip5 — gráfico de niveles (1 columna = 10 ms = 1 píxel), con tema.
#pragma once

#include "Theme.h"
#include "../PluginProcessor.h"

namespace clip5::ui
{
class HistoryDisplay : public juce::Component
{
public:
    explicit HistoryDisplay (MeterHub& h);
    void paint (juce::Graphics&) override;
    /** Temporizador del editor: repinta solo si llegaron columnas nuevas o cambió el techo. */
    void tick (float ceilingDb);
    juce::Rectangle<int> plotArea() const;

private:
    MeterHub& hub;
    uint64_t lastTotal = 0;
    float ceilDb = -1.0f, paintedCeil = 1000.0f;
    float yFor (float db, juce::Rectangle<float> plot) const noexcept;
};

/** Transferencia estática (entrada contra salida, dB relativos al techo). */
void drawTransferCurve (juce::Graphics&, juce::Rectangle<float> area, float densityDb, int mode);
} // namespace clip5::ui
