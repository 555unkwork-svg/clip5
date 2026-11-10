// clip5 — paleta, tipografía y LookAndFeel (diseño plano y preciso, máximo 2 acentos).
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace clip5::ui
{
namespace col
{
    const juce::Colour fondo   { 0xff121416 }; // carbón frío
    const juce::Colour sup     { 0xff1b1f22 }; // superficie
    const juce::Colour sup2    { 0xff22272b };
    const juce::Colour lin     { 0xff2a3035 }; // líneas
    const juce::Colour pista   { 0xff15181a };
    const juce::Colour txt     { 0xffece8e1 }; // marfil
    const juce::Colour txt2    { 0xff8c949a }; // secundario (contraste 5,4:1 sobre superficie)
    const juce::Colour menta   { 0xff6fe3c1 }; // marca / Densidad
    const juce::Colour coral   { 0xffff6a4d }; // recorte / alertas (solo donde hay recorte)
    inline juce::Colour menta55() { return menta.withAlpha (0.55f); } // Suavizado
    inline juce::Colour menta12() { return menta.withAlpha (0.12f); }
}

struct Fonts
{
    static juce::Font sans (float h, bool semibold = false);
    static juce::Font mono (float h);
};

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area, bool isSeparator, bool isActive,
                            bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
                            const juce::String& shortcutKeyText, const juce::Drawable* icon, const juce::Colour* textColour) override;
    juce::Font getPopupMenuFont() override { return Fonts::sans (14.0f); }
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return Fonts::sans (13.0f); }
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    void drawTooltip (juce::Graphics&, const juce::String& text, int w, int h) override;
    juce::Rectangle<int> getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos, juce::Rectangle<int> parentArea) override;
    void fillTextEditorBackground (juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawTextEditorOutline (juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return Fonts::sans (13.0f); }
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
};
} // namespace clip5::ui
