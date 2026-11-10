// clip5 — controles con tema y teclado (Tab enfoca, flechas ajustan, Enter edita, Esc cancela).
#pragma once

#include "Theme.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace clip5::ui
{
/** Base: componente enlazado a un parámetro (gestos para el host, foco de teclado, edición directa). */
class ParamControl : public juce::Component, public juce::SettableTooltipClient
{
public:
    ParamControl (juce::AudioProcessorValueTreeState& s, const juce::String& paramId);

    float value() const noexcept { return current; }
    juce::RangedAudioParameter& param() noexcept { return *p; }
    void focusGained (FocusChangeType cause) override;
    void focusLost (FocusChangeType) override;

    /** Abre la edición directa del valor sobre `area` (coordenadas locales). */
    void showEditor (juce::Rectangle<int> area);
    bool isEditing() const noexcept { return editor != nullptr; }
    std::function<juce::String (float)> format;

protected:
    juce::RangedAudioParameter* p = nullptr;
    std::unique_ptr<juce::ParameterAttachment> att;
    float current = 0;
    bool keyboardFocus = false;

    virtual void valueChanged() { repaint(); }
    float norm() const noexcept { return p->convertTo0to1 (current); }
    void setNorm (float n);
    void setReal (float v);
    void stepNorm (float delta); // gesto completo
    juce::String rangeText() const;

private:
    std::unique_ptr<juce::TextEditor> editor;
    std::unique_ptr<juce::Label> hint;
    void closeEditor (bool commit, bool fromFocusLost);
};

/** Perilla de arco: nombre arriba (opcional), dial y recuadro con el valor abajo. */
class Knob : public ParamControl
{
public:
    Knob (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, juce::String name, bool bipolar,
          std::function<juce::String (float)> fmt, float nameSize = 13.0f);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed (const juce::KeyPress&) override;

    std::function<ParamControl*()> shiftTarget; // Shift+arrastrar controla otro parámetro (compensación)
    juce::String name;
    int valueBoxWidth = 96;

private:
    bool bipolar;
    float nameSize;
    juce::Rectangle<float> dial;
    juce::Rectangle<int> valueBox;
    float dragStartNorm = 0, dragStartY = 0;
    bool dragging = false, draggingOther = false, lastFine = false;
};

/** Fader horizontal con nombre y recuadro de valor (Procesada / Original). */
class HFader : public ParamControl
{
public:
    HFader (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, juce::String name);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    std::function<void (float db)> onUserChange;
    void setFromLink (float db);

private:
    juce::String name;
    juce::Rectangle<int> track, valueBox;
    bool dragging = false;
    float startNorm = 0, startX = 0;
    void changed();
};

/** Botón tipo píldora para un parámetro booleano (Enlace). */
class PillToggle : public ParamControl
{
public:
    PillToggle (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, juce::String text);
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    juce::String text;
};

/** Interruptor para parámetros booleanos (panel Avanzado). */
class Switch : public ParamControl
{
public:
    Switch (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, juce::String onText, juce::String offText);
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    juce::String onText, offText;
};

/** Selector segmentado para parámetros de elección (← → dentro del grupo). */
class Segmented : public ParamControl
{
public:
    Segmented (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, juce::StringArray labels, float gap = 5.0f);
    float fontSize = 12.5f;
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    juce::StringArray labels;
    float gap;
    juce::Rectangle<float> cell (int i) const;
};

/** Curva de recorte: cuatro cuadraditos con el dibujo de la curva real (sin texto). */
class CurveSelector : public ParamControl
{
public:
    explicit CurveSelector (juce::AudioProcessorValueTreeState& s);
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    juce::String getTooltip() override;

private:
    int hover = -1;
    juce::Rectangle<float> cell (int i) const;
};

/** Botón de texto con tema. */
class FlatButton : public juce::TextButton
{
public:
    FlatButton (const juce::String& text, const juce::String& tip, bool onGraph = false);
    void focusGained (FocusChangeType cause) override;
    void focusLost (FocusChangeType) override;
};

/** Recuadro de lectura (valor monoespaciado). */
void drawValueBox (juce::Graphics&, juce::Rectangle<float> r, const juce::String& text, bool focus = false);
} // namespace clip5::ui
