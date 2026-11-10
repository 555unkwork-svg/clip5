// clip5 — paneles superpuestos: Medidores, Curva E/S, Avanzado y Apariencia.
#pragma once

#include "Controls.h"
#include "../PluginProcessor.h"

namespace clip5::ui
{
/** Base: tarjeta con título (Bevan), sombra y "Esc cierra". */
class Overlay : public juce::Component
{
public:
    explicit Overlay (juce::String title);
    void paintOverChildren (juce::Graphics&) override {}
    void paint (juce::Graphics&) override;
    virtual void tick() {}

protected:
    juce::String title;
    juce::Rectangle<int> content() const { return getLocalBounds().reduced (16, 12).withTrimmedTop (28); }
};

class MetersPanel : public Overlay
{
public:
    MetersPanel (MeterHub& h, Engine& e);
    void paint (juce::Graphics&) override;
    void resized() override;
    void tick() override;

private:
    MeterHub& hub;
    Engine& engine;
    FlatButton resetMeters, resetMax;
};

class CurvePanel : public Overlay
{
public:
    explicit CurvePanel (juce::AudioProcessorValueTreeState& s);
    void paint (juce::Graphics&) override;
    void tick() override;

private:
    juce::AudioProcessorValueTreeState& state;
    float dens = -1; int mode = -1;
};

class AdvancedPanel : public Overlay
{
public:
    explicit AdvancedPanel (juce::AudioProcessorValueTreeState& s);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    Segmented offline, fase, tono;
    Switch truePeak, rango, compEntrada, compDensidad;
    std::vector<std::pair<juce::String, juce::Component*>> rows;
};

/** Apariencia: lista de temas con vista previa en vivo; Aplicar guarda, Cancelar restaura. */
class AppearancePanel : public Overlay
{
public:
    AppearancePanel();
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    void open();
    void cancel();
    void apply();
    std::function<void()> onClose;

private:
    int original = 0, selected = 0;
    FlatButton cancelBtn, applyBtn;
    juce::Rectangle<int> row (int i) const;
    void preview (int i);
};
} // namespace clip5::ui
