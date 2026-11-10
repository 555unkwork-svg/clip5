// clip5 — temas (10 paletas con roles de color + sticker), tipografías y LookAndFeel.
// El tema es una preferencia local (archivo de ajustes), no viaja en los presets.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace clip5::ui
{
struct Palette
{
    juce::String id, nombre, tipo, base;
    juce::Colour fondo, sup, proc, graf, txt, txt2, sel, selTxt, inFill, inBorde, outLine, outCont,
                 dens, densTint, over, overArea, foco, perilla;
    // gráfico de los temas híbridos (si grafBase es transparente se usan sup/txt/txt2)
    juce::Colour grafBase, grafTxt, grafEscala, grafBoton;
    const char* stickerData = nullptr;
    int stickerSize = 0;

    bool hybrid() const noexcept { return ! grafBase.isTransparent(); }
    juce::Colour graphPanel() const noexcept { return hybrid() ? grafBase : graf; }
    juce::Colour graphText() const noexcept { return hybrid() ? grafTxt : txt; }
    juce::Colour graphScale() const noexcept { return hybrid() ? grafEscala : txt2; }
    juce::Colour graphButton() const noexcept { return hybrid() ? grafBoton : sup; }
};

/** Temas disponibles y tema activo (compartido por todas las instancias del plugin). */
class Themes : public juce::ChangeBroadcaster
{
public:
    static Themes& get();
    static constexpr int kCount = 10;

    const Palette& palette (int i) const noexcept { return list[(size_t) juce::jlimit (0, kCount - 1, i)]; }
    const Palette& current() const noexcept { return palette (index); }
    int currentIndex() const noexcept { return index; }

    /** persist = false: vista previa (no se guarda). */
    void setIndex (int i, bool persist);
    juce::Image sticker (int i);

private:
    Themes();
    std::vector<Palette> list;
    std::map<int, juce::Image> stickerCache;
    int index = 6;
    std::unique_ptr<juce::PropertiesFile> props;
};

inline const Palette& pal() { return Themes::get().current(); }

struct Fonts
{
    static juce::Font sans (float h, bool semibold = false);
    static juce::Font mono (float h);
    static juce::Font brand (float h); // Bevan
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
    void drawPopupMenuSectionHeader (juce::Graphics&, const juce::Rectangle<int>&, const juce::String&) override;
    juce::Font getPopupMenuFont() override { return Fonts::sans (14.0f); }
    void drawTooltip (juce::Graphics&, const juce::String& text, int w, int h) override;
    juce::Rectangle<int> getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos, juce::Rectangle<int> parentArea) override;
    void fillTextEditorBackground (juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawTextEditorOutline (juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return Fonts::sans (12.5f); }
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawCornerResizer (juce::Graphics&, int w, int h, bool isMouseOver, bool isMouseDragging) override;
    void applyPalette();
};

/** Anillo de foco de teclado (solo cuando el foco llegó con Tab). */
void drawFocusRing (juce::Graphics&, juce::Rectangle<float> r, float radius);
} // namespace clip5::ui
