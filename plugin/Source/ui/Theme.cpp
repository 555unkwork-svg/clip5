#include "Theme.h"
#include "Clip5Assets.h"

namespace clip5::ui
{
namespace
{
    juce::Colour C (juce::uint32 rgb) { return juce::Colour (0xff000000 | rgb); }

    Palette make (const char* id, const char* nombre, const char* tipo, const char* base,
                  std::initializer_list<juce::uint32> v, const char* data, int size,
                  std::initializer_list<juce::uint32> hyb = {})
    {
        Palette p;
        p.id = id; p.nombre = juce::String::fromUTF8 (nombre); p.tipo = juce::String::fromUTF8 (tipo); p.base = juce::String::fromUTF8 (base);
        auto it = v.begin();
        juce::Colour* f[] = { &p.fondo, &p.sup, &p.proc, &p.graf, &p.txt, &p.txt2, &p.sel, &p.selTxt, &p.inFill, &p.inBorde,
                              &p.outLine, &p.outCont, &p.dens, &p.densTint, &p.over, &p.overArea, &p.foco, &p.perilla };
        for (auto* c : f) *c = C (*it++);
        if (hyb.size() == 4)
        {
            auto h = hyb.begin();
            p.grafBase = C (h[0]); p.grafTxt = C (h[1]); p.grafEscala = C (h[2]); p.grafBoton = C (h[3]);
        }
        p.stickerData = data; p.stickerSize = size;
        return p;
    }

    juce::Typeface::Ptr load (const char* data, int size) { return juce::Typeface::createSystemTypefaceFor (data, (size_t) size); }
    juce::Typeface::Ptr interRegular() { static auto t = load (Clip5Assets::InterRegular_otf, Clip5Assets::InterRegular_otfSize); return t; }
    juce::Typeface::Ptr interSemi() { static auto t = load (Clip5Assets::InterSemiBold_otf, Clip5Assets::InterSemiBold_otfSize); return t; }
    juce::Typeface::Ptr monoRegular() { static auto t = load (Clip5Assets::JetBrainsMonoRegular_ttf, Clip5Assets::JetBrainsMonoRegular_ttfSize); return t; }
    juce::Typeface::Ptr bevan() { static auto t = load (Clip5Assets::BevanRegular_ttf, Clip5Assets::BevanRegular_ttfSize); return t; }
}

// ====================================================================================== temas
Themes& Themes::get()
{
    static Themes t;
    return t;
}

Themes::Themes()
{
    using namespace Clip5Assets;
    // orden de roles: fondo, sup, proc, graf, txt, txt2, sel, selTxt, inFill, inBorde, outLine, outCont,
    //                 dens, densTint, over, overArea, foco, perilla
    list.push_back (make ("ice", "ice", "Clara", "Acero glaciar",
        { 0xAFC5D5, 0xE4EFF5, 0xC1D5E2, 0xC7DCE9, 0x070D19, 0x3F4E58, 0x34556C, 0xF5FAFC, 0x728FAB, 0x3F6077, 0xFBFDFF, 0x304B60,
          0x286D70, 0x91B9BC, 0x913124, 0xF17C68, 0x195A86, 0x3F4E58 }, ice_png, ice_pngSize));
    list.push_back (make ("raw", "raw", "Clara", "Papel mineral (modificado)",
        { 0xCDBFA8, 0xF6EEE1, 0xE2D3BC, 0xE9DFCC, 0x27241F, 0x514639, 0xB3232B, 0xFFF9EE, 0xD18A80, 0x8E1D24, 0xFFFAEF, 0x3A2E26,
          0x9C1F27, 0xE2A69F, 0x1F4FA0, 0x6E9BE0, 0x7E1A20, 0x514A40 }, raw_png, raw_pngSize));
    list.push_back (make ("violeta", "violeta", "Clara", "Lavanda técnica",
        { 0xC2BBD1, 0xF1EDF7, 0xD9D1E6, 0xDDD5ED, 0x251A37, 0x4F4265, 0x554477, 0xFCFAFF, 0x8874AA, 0x604A83, 0xFCFAFF, 0x473461,
          0x266D67, 0xA9C9BD, 0x8B3045, 0xE57E91, 0x5C428F, 0x49435C }, violeta_png, violeta_pngSize));
    list.push_back (make ("pro", "pro", "Oscura", "Pizarra nocturna",
        { 0x070D19, 0x223646, 0x1B2D3C, 0x142635, 0xEDF5F8, 0xC1D4DF, 0x91B9BC, 0x070D19, 0x728FAB, 0x98B6D0, 0xF3FAFC, 0xF3FAFC,
          0xA8D4CE, 0x91B9BC, 0xFFAA94, 0xFF846E, 0xBDEAFF, 0x334A5C }, pro_png, pro_pngSize));
    list.push_back (make ("cafe", "cafe", "Oscura", "Carbón y bronce",
        { 0x161210, 0x372C26, 0x2E241F, 0x241B17, 0xFFF2E5, 0xE3CEBC, 0xD8B381, 0x21160E, 0xA78973, 0xC2AA96, 0xFFF5E7, 0xFFF5E7,
          0xAACBA6, 0x8DAD91, 0xFFA58F, 0xFF907C, 0xFBE0AA, 0x52433B }, cafe_png, cafe_pngSize));
    list.push_back (make ("marfil", "marfil", "Clara", "Marfil y tinta",
        { 0xD1D0CC, 0xF5F2EB, 0xE4E1DA, 0xECEAE5, 0x1A1A19, 0x494A46, 0x313533, 0xFAF8F1, 0x9EA29F, 0x505652, 0xFFFCF5, 0x343A37,
          0x535A55, 0xBFC5BF, 0x96342C, 0xE47562, 0x4B514D, 0x424541 }, marfil_png, marfil_pngSize));
    list.push_back (make ("sonic", "sonic", "Híbrida", "Cobalto de referencia",
        { 0xB6C0D9, 0xF1F4FC, 0xDAE3F6, 0x172B61, 0x151E35, 0x455474, 0x23408B, 0xF6F9FF, 0x5D85DE, 0xA9C7FF, 0xF5FEFC, 0xF5FEFC,
          0xA0E4D3, 0x60AAAB, 0xFFC0B1, 0xFF9B83, 0x2147B1, 0x354568 }, sonic_png, sonic_pngSize,
        { 0x243B73, 0xF4F7FF, 0xD3E0FA, 0x304A83 }));
    list.push_back (make ("rosegold", "rose gold", "Clara", "Arcilla y ciruela",
        { 0xC6A3A1, 0xF8EAE5, 0xEDD3CA, 0xF0DFD9, 0x321E2B, 0x603D48, 0x713F53, 0xFFF5F1, 0xA5809D, 0x614A67, 0xFFF7F1, 0x4B3346,
          0x215D60, 0x9EBDB6, 0x8B2F25, 0xEA836D, 0x6A3B7F, 0x543B48 }, roseGold_png, roseGold_pngSize));
    list.push_back (make ("420", "420", "Oscura", "Bosque polar",
        { 0x0A1713, 0x263C33, 0x1B2D26, 0x13251F, 0xEDF7F2, 0xC4DBD0, 0x94CBB4, 0x0B211A, 0x6B9380, 0xB0D2C1, 0xF2FBF7, 0xF2FBF7,
          0xE5DCAA, 0x879070, 0xFFB49E, 0xF88D75, 0xCEF2C3, 0x354D42 }, c420_png, c420_pngSize));
    list.push_back (make ("serum", "serum", "Oscura", "Grafito eléctrico",
        { 0x0D0F12, 0x1E2228, 0x181B20, 0x0A0C0F, 0xE8EDF4, 0x9DA9B8, 0x3D9BFF, 0x04101D, 0x2E6CB6, 0x79BCFF, 0xF4F8FF, 0xF4F8FF,
          0xFFB13B, 0x8C6A35, 0xFF5E7E, 0xFF3D64, 0x8FD4FF, 0x2A2F37 }, serum_png, serum_pngSize));

    juce::PropertiesFile::Options o;
    o.applicationName = "clip5";
    o.filenameSuffix = ".settings";
    o.folderName = "clip5";
    o.osxLibrarySubFolder = "Application Support";
    props = std::make_unique<juce::PropertiesFile> (o);
    const auto saved = props->getValue ("tema", "sonic");
    for (int i = 0; i < kCount; ++i)
        if (list[(size_t) i].id == saved) index = i;
}

void Themes::setIndex (int i, bool persist)
{
    i = juce::jlimit (0, kCount - 1, i);
    const bool changed = i != index;
    index = i;
    if (persist && props)
    {
        props->setValue ("tema", list[(size_t) i].id);
        props->saveIfNeeded();
    }
    if (changed) sendSynchronousChangeMessage();
}

juce::Image Themes::sticker (int i)
{
    auto it = stickerCache.find (i);
    if (it != stickerCache.end()) return it->second;
    const auto& p = palette (i);
    auto img = juce::ImageCache::getFromMemory (p.stickerData, p.stickerSize);
    stickerCache[i] = img;
    return img;
}

// ====================================================================================== fuentes
juce::Font Fonts::sans (float h, bool semibold) { return juce::Font (juce::FontOptions (semibold ? interSemi() : interRegular()).withHeight (h)); }
juce::Font Fonts::mono (float h) { return juce::Font (juce::FontOptions (monoRegular()).withHeight (h)); }
juce::Font Fonts::brand (float h) { return juce::Font (juce::FontOptions (bevan()).withHeight (h)); }

void drawFocusRing (juce::Graphics& g, juce::Rectangle<float> r, float radius)
{
    g.setColour (pal().foco);
    g.drawRoundedRectangle (r.expanded (2.5f), radius + 2.5f, 2.2f);
}

// ====================================================================================== LookAndFeel
LookAndFeel::LookAndFeel()
{
    setDefaultSansSerifTypeface (interRegular());
    applyPalette();
}

void LookAndFeel::applyPalette()
{
    const auto& p = pal();
    setColour (juce::ResizableWindow::backgroundColourId, p.fondo);
    setColour (juce::PopupMenu::backgroundColourId, p.sup);
    setColour (juce::PopupMenu::textColourId, p.txt);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, p.proc);
    setColour (juce::PopupMenu::highlightedTextColourId, p.txt);
    setColour (juce::TextEditor::backgroundColourId, juce::Colours::white);
    setColour (juce::TextEditor::textColourId, juce::Colour (0xff151E35));
    setColour (juce::TextEditor::highlightColourId, p.sel.withAlpha (0.35f));
    setColour (juce::TextEditor::highlightedTextColourId, juce::Colour (0xff151E35));
    setColour (juce::TextEditor::focusedOutlineColourId, p.foco);
    setColour (juce::CaretComponent::caretColourId, p.foco);
    setColour (juce::Label::textColourId, p.txt);
    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff0b0c0d));
    setColour (juce::TooltipWindow::textColourId, juce::Colour (0xffECE8E1));
    setColour (juce::AlertWindow::backgroundColourId, p.sup);
    setColour (juce::AlertWindow::textColourId, p.txt);
    setColour (juce::AlertWindow::outlineColourId, p.txt2);
    setColour (juce::TextButton::buttonColourId, p.proc);
    setColour (juce::TextButton::textColourOffId, p.txt);
}

juce::Typeface::Ptr LookAndFeel::getTypefaceForFont (const juce::Font& f)
{
    if (f.getTypefaceName() == juce::Font::getDefaultSansSerifFontName()) return interRegular();
    return LookAndFeel_V4::getTypefaceForFont (f);
}

void LookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (pal().sup);
    g.setColour (pal().txt2.withAlpha (0.4f));
    g.drawRect (0, 0, w, h);
}

void LookAndFeel::drawPopupMenuSectionHeader (juce::Graphics& g, const juce::Rectangle<int>& area, const juce::String& text)
{
    g.setColour (pal().txt2);
    g.setFont (Fonts::sans (11.5f, true));
    g.drawText (text.toUpperCase(), area.withTrimmedLeft (12), juce::Justification::bottomLeft);
}

void LookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator, bool isActive,
                                     bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
                                     const juce::String& shortcutKeyText, const juce::Drawable*, const juce::Colour*)
{
    const auto& p = pal();
    if (isSeparator)
    {
        g.setColour (p.txt2.withAlpha (0.3f));
        g.fillRect (area.reduced (8, 0).withHeight (1).withY (area.getCentreY()));
        return;
    }
    auto r = area.reduced (3, 1);
    if (isHighlighted && isActive)
    {
        g.setColour (p.proc);
        g.fillRoundedRectangle (r.toFloat(), 5.0f);
    }
    g.setColour (! isActive ? p.txt2 : p.txt);
    g.setFont (Fonts::sans (14.0f, isTicked));
    g.drawText (text, r.withTrimmedLeft (24), juce::Justification::centredLeft);
    if (isTicked)
    {
        g.setColour (p.sel);
        g.fillEllipse (juce::Rectangle<float> (7, 7).withCentre ({ (float) r.getX() + 12.0f, (float) r.getCentreY() }));
    }
    g.setColour (p.txt2);
    if (shortcutKeyText.isNotEmpty()) g.drawText (shortcutKeyText, r.withTrimmedRight (10), juce::Justification::centredRight);
    if (hasSubMenu) g.drawText (juce::String::fromUTF8 ("›"), r.withTrimmedRight (10), juce::Justification::centredRight);
}

void LookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int w, int h)
{
    g.fillAll (juce::Colour (0xff0b0c0d));
    g.setColour (juce::Colour (0xff2a3035));
    g.drawRect (0, 0, w, h);
    juce::AttributedString s;
    s.append (text, Fonts::sans (13.0f), juce::Colour (0xffECE8E1));
    s.setWordWrap (juce::AttributedString::byWord);
    juce::TextLayout tl;
    tl.createLayout (s, (float) w - 16.0f);
    tl.draw (g, { 8.0f, 6.0f, (float) w - 16.0f, (float) h - 12.0f });
}

juce::Rectangle<int> LookAndFeel::getTooltipBounds (const juce::String& tipText, juce::Point<int> pos, juce::Rectangle<int> parent)
{
    juce::AttributedString s;
    s.append (tipText, Fonts::sans (13.0f), juce::Colours::white);
    s.setWordWrap (juce::AttributedString::byWord);
    juce::TextLayout tl;
    tl.createLayout (s, 280.0f);
    const int w = (int) std::ceil (tl.getWidth()) + 18, h = (int) std::ceil (tl.getHeight()) + 14;
    return juce::Rectangle<int> (pos.x, pos.y + 18, w, h).constrainedWithin (parent);
}

void LookAndFeel::fillTextEditorBackground (juce::Graphics& g, int w, int h, juce::TextEditor&)
{
    g.setColour (juce::Colours::white);
    g.fillRoundedRectangle (0, 0, (float) w, (float) h, 5.0f);
}

void LookAndFeel::drawTextEditorOutline (juce::Graphics& g, int w, int h, juce::TextEditor&)
{
    g.setColour (pal().foco);
    g.drawRoundedRectangle (1.0f, 1.0f, (float) w - 2, (float) h - 2, 5.0f, 2.0f);
}

void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    const auto& p = pal();
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    const bool onGraph = (bool) b.getProperties().getWithDefault ("enGrafico", false);
    juce::Colour bg = on ? p.sel : (onGraph ? p.graphButton() : p.proc);
    if (down && ! on) bg = bg.overlaidWith (p.txt.withAlpha (0.10f));
    else if (over && ! on) bg = bg.overlaidWith (p.txt.withAlpha (0.06f));
    g.setColour (bg);
    g.fillRoundedRectangle (r, 6.0f);
    if (! on)
    {
        g.setColour ((onGraph ? p.graphScale() : p.txt2).withAlpha (over ? 0.6f : 0.3f));
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
    }
    if (b.hasKeyboardFocus (false) && (bool) b.getProperties().getWithDefault ("focoTeclado", false))
        drawFocusRing (g, r.reduced (1.0f), 6.0f);
}

void LookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    const auto& p = pal();
    const bool onGraph = (bool) b.getProperties().getWithDefault ("enGrafico", false);
    g.setFont (getTextButtonFont (b, b.getHeight()));
    juce::Colour c = b.getToggleState() ? p.selTxt : (onGraph ? p.graphText() : p.txt);
    if (! b.isEnabled()) c = c.withAlpha (0.4f);
    g.setColour (c);
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

void LookAndFeel::drawCornerResizer (juce::Graphics& g, int w, int h, bool, bool)
{
    g.setColour (pal().txt2.withAlpha (0.5f));
    for (int i = 1; i <= 3; ++i)
        g.drawLine ((float) w - (float) i * 4.0f, (float) h - 1.0f, (float) w - 1.0f, (float) h - (float) i * 4.0f, 1.0f);
}
} // namespace clip5::ui
