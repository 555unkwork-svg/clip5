// clip5 — display de historial (1 columna = 10 ms = 1 píxel) y curva de transferencia.
#pragma once

#include "Theme.h"
#include "../PluginProcessor.h"

namespace clip5::ui
{
class TransferCurve : public juce::Component, public juce::SettableTooltipClient
{
public:
    TransferCurve();
    void set (float densityDb, int mode);
    void paint (juce::Graphics&) override;

private:
    float density = -1;
    int mode = -1;
    juce::Path curve;
    void rebuild();
};

class HistoryDisplay : public juce::Component
{
public:
    explicit HistoryDisplay (MeterHub& h);
    void paint (juce::Graphics&) override;
    void resized() override;
    /** Llamado por el temporizador del editor: repinta solo si llegaron columnas nuevas. */
    void tick (float ceilingDb);

    TransferCurve transfer;

private:
    MeterHub& hub;
    juce::Image background;
    uint64_t lastTotal = 0;
    float ceilDb = -1.0f, bgCeil = 1000.0f;
    void renderBackground();
    float yFor (float db) const noexcept;
};
} // namespace clip5::ui
