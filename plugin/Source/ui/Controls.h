// clip5 — controles propios: perilla de arco fino, fader, interruptor, selector segmentado y modos.
#pragma once

#include "Theme.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace clip5::ui
{
/** Base: componente enlazado a un parámetro (ParameterAttachment) con gestos para el host. */
class ParamControl : public juce::Component, public juce::SettableTooltipClient
{
public:
    ParamControl (juce::AudioProcessorValueTreeState& s, const juce::String& paramId);
    ~ParamControl() override = default;

    float value() const noexcept { return current; }
    juce::RangedAudioParameter& param() noexcept { return *p; }

protected:
    juce::AudioProcessorValueTreeState& state;
    juce::RangedAudioParameter* p = nullptr;
    std::unique_ptr<juce::ParameterAttachment> att;
    float current = 0;
    virtual void valueChanged() { repaint(); }

    float norm() const noexcept { return p->convertTo0to1 (current); }
    void setNorm (float n);
    void setReal (float v);
};

/** Perilla de arco fino. Arrastre vertical; Shift = fino; Ctrl/Cmd+clic = valor por defecto;
    doble clic = escribir el valor; rueda = pasos finos. */
class Knob : public ParamControl
{
public:
    enum class Style { Menta, Menta55, Marfil };

    Knob (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, const juce::String& name,
          Style style, bool bipolar, std::function<juce::String (float)> fmt);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    /** Si se define, Shift+arrastrar controla este otro parámetro (offset de Enlace E/S). */
    std::function<ParamControl*()> shiftTarget;
    bool compact = false;
    juce::String nameOverride;

private:
    juce::String name;
    Style style;
    bool bipolar;
    std::function<juce::String (float)> format;
    float dragStartNorm = 0, dragStartY = 0;
    bool dragging = false, draggingOther = false, lastFine = false;
    std::unique_ptr<juce::TextEditor> editor;
    juce::Rectangle<float> dial;
    void showEditor();
};

/** Fader vertical para HÚMEDO / SECO (−∞ … +6 dB). */
class Fader : public ParamControl
{
public:
    Fader (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, const juce::String& name);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    /** Llamado al mover este fader (para el enlace de potencia constante). */
    std::function<void (float db)> onUserChange;
    void setFromLink (float db);

private:
    juce::String name;
    juce::Rectangle<int> rail;
    bool dragging = false;
    float startNorm = 0, startY = 0;
    void setFromY (float y, bool fine);
};

/** Interruptor para parámetros booleanos. */
class Switch : public ParamControl
{
public:
    Switch (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, juce::String onText, juce::String offText);
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::String onText, offText;
};

/** Selector segmentado para parámetros de elección. */
class Segmented : public ParamControl
{
public:
    Segmented (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, juce::StringArray labels);
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::StringArray labels;
};

/** Selector de modo de recorte con mini-gráficos de la curva real. */
class ModeSelector : public ParamControl
{
public:
    ModeSelector (juce::AudioProcessorValueTreeState& s);
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void setDensity (float db);
    juce::String getTooltip() override;

private:
    float density = 0;
    int hover = -1;
    juce::Rectangle<float> cell (int i) const;
};

/** Botón de texto simple con estado (A, B, ⇄, ↶, ↷, ⚙…). */
class FlatButton : public juce::TextButton
{
public:
    FlatButton (const juce::String& text, const juce::String& tip)
    {
        setButtonText (text);
        setTooltip (tip);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }
};
} // namespace clip5::ui
