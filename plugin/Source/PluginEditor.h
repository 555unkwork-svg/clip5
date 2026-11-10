// clip5 — interfaz (en español). Base 1040×640, escalable 75–200 %, 10 temas.
#pragma once

#include "PluginProcessor.h"
#include "ui/Controls.h"
#include "ui/Display.h"
#include "ui/Panels.h"
#include "ui/Theme.h"

namespace clip5
{
class Clip5Editor : public juce::AudioProcessorEditor, private juce::Timer, private juce::ChangeListener
{
public:
    explicit Clip5Editor (Clip5Processor&);
    ~Clip5Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;

    static constexpr int kW = 1040, kH = 640;

    /** Actualiza todos los elementos dinámicos (temporizador y herramienta de capturas). */
    void refresh();
    /** "Medidores", "Curva E/S", "Avanzado", "Apariencia" o vacío para cerrar. */
    void showPanel (const juce::String& name);

private:
    class Content;

    Clip5Processor& proc;
    ui::LookAndFeel lnf;
    std::unique_ptr<Content> content;
    juce::TooltipWindow tooltips { this, 600 };
    std::unique_ptr<juce::PropertiesFile> settings;
    float scale = 1.0f;
    int fps = 30;

    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void setScale (float s);
    void restartTimer();
    friend class Content;
};
} // namespace clip5
