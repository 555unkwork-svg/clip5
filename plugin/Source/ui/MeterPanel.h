// clip5 — medidores de E/S con lecturas (pico, true peak, LUFS M/S/I, recorte, Densidad)
// y Reloj de recorte.
#pragma once

#include "Theme.h"
#include "../PluginProcessor.h"

namespace clip5::ui
{
class MeterPanel : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit MeterPanel (MeterHub& h);
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void tick (float ceilingDb);

private:
    MeterHub& hub;
    float ceilDb = -1;
    juce::Rectangle<float> barArea, readArea;
};

class ClipClockView : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit ClipClockView (Engine& e);
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void tick();

private:
    Engine& engine;
    float shownDb = -1;
    double shownAt = -1;
};
} // namespace clip5::ui
