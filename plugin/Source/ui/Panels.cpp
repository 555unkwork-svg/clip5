#include "Panels.h"
#include "Display.h"

namespace clip5::ui
{
using params::es;
using params::formatDb;
namespace id = params::id;

// ===================================================================================== base
Overlay::Overlay (juce::String t) : title (std::move (t))
{
    setWantsKeyboardFocus (false);
}

void Overlay::paint (juce::Graphics& g)
{
    const auto& c = pal();
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    for (int i = 6; i > 0; --i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.035f));
        g.fillRoundedRectangle (r.translated (0, (float) i * 0.8f).expanded ((float) i * 0.6f), 11.0f);
    }
    g.setColour (c.sup);
    g.fillRoundedRectangle (r, 10.0f);
    g.setColour (c.txt2.withAlpha (0.35f));
    g.drawRoundedRectangle (r, 10.0f, 1.0f);
    auto head = getLocalBounds().reduced (16, 12).removeFromTop (20);
    g.setColour (c.txt);
    g.setFont (Fonts::brand (13.5f));
    g.drawText (title, head, juce::Justification::centredLeft);
    g.setColour (c.txt2);
    g.setFont (Fonts::sans (10.5f));
    g.drawText (es ("Esc cierra"), head, juce::Justification::centredRight);
}

// ===================================================================================== Medidores
MetersPanel::MetersPanel (MeterHub& h, Engine& e)
    : Overlay ("MEDIDORES"), hub (h), engine (e),
      resetMeters (es ("Reiniciar medidores"), es ("Borra los picos máximos y las lecturas LUFS de Entrada y Salida.")),
      resetMax (es ("Reiniciar máximo"), es ("Borra el mayor exceso sobre el Techo y su marca de tiempo."))
{
    addAndMakeVisible (resetMeters);
    addAndMakeVisible (resetMax);
    resetMeters.onClick = [this] { hub.resetReadouts(); repaint(); };
    resetMax.onClick = [this] { engine.clipClock().resetRequest.store (true); repaint(); };
}

void MetersPanel::resized()
{
    auto r = content();
    resetMeters.setBounds (r.getX(), r.getY() + 150, 150, 24);
    resetMax.setBounds (r.getX(), r.getBottom() - 26, 130, 24);
}

void MetersPanel::tick() { repaint(); }

void MetersPanel::paint (juce::Graphics& g)
{
    Overlay::paint (g);
    const auto& c = pal();
    auto r = content().toFloat();
    const float c2 = r.getRight() - 120.0f, c3 = r.getRight() - 50.0f, rowH = 19.0f;
    auto dash = [] (float v) { return v < -119.0f ? es ("—") : formatDb (v); };
    auto lufs = [] (double v) { return v < -99.0 ? es ("—") : formatDb ((float) v); };
    float y = r.getY();
    auto row = [&] (const juce::String& k, const juce::String& a, const juce::String& b, bool bold = false, juce::Colour cb = juce::Colours::transparentBlack) {
        g.setColour (c.txt);
        g.setFont (Fonts::sans (12.0f, bold));
        g.drawText (k, juce::Rectangle<float> (r.getX(), y, 200, rowH), juce::Justification::centredLeft);
        g.setFont (Fonts::mono (12.0f));
        g.drawText (a, juce::Rectangle<float> (c2, y, 50, rowH), juce::Justification::centredRight);
        g.setColour (cb.isTransparent() ? c.txt : cb);
        g.drawText (b, juce::Rectangle<float> (c3, y, 50, rowH), juce::Justification::centredRight);
        y += rowH;
    };
    g.setColour (c.txt2);
    g.setFont (Fonts::sans (11.0f));
    g.drawText ("Entrada", juce::Rectangle<float> (c2 - 10, y, 60, rowH), juce::Justification::centredRight);
    g.drawText ("Salida", juce::Rectangle<float> (c3 - 10, y, 60, rowH), juce::Justification::centredRight);
    y += rowH;
    row (es ("Pico máximo · dBFS"), dash (hub.peakInMax), dash (hub.peakOutMax));
    const float ceil = hub.columnsAvailable() > 0 ? hub.column (0).ceilDb : 0.0f;
    row (es ("Pico real máx. · dBTP"), es ("—"), dash (hub.tpOutMax), false, hub.tpOutMax > ceil + 0.05f ? c.over : juce::Colours::transparentBlack);
    y += 6;
    g.setColour (c.txt2.withAlpha (0.35f));
    g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
    y += 6;
    g.setColour (c.txt);
    g.setFont (Fonts::brand (11.5f));
    g.drawText (es ("SONORIDAD · LUFS"), juce::Rectangle<float> (r.getX(), y, 200, rowH), juce::Justification::centredLeft);
    y += rowH;
    row (es ("M · Momentánea"), lufs (hub.lufsIn.momentary()), lufs (hub.lufsOut.momentary()));
    row (es ("S · Corto plazo"), lufs (hub.lufsIn.shortTerm()), lufs (hub.lufsOut.shortTerm()));
    row (es ("I · Integrada"), lufs (hub.lufsIn.integrated()), lufs (hub.lufsOut.integrated()), true);

    // exceso
    y = r.getBottom() - 74.0f;
    g.setColour (c.txt2.withAlpha (0.35f));
    g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
    y += 8;
    const float now = hub.clipNow, mx = engine.clipClock().maxClipDb.load();
    const double at = engine.clipClock().atSeconds.load();
    const int secs = (int) std::max (0.0, at);
    g.setFont (Fonts::sans (12.0f));
    g.setColour (c.txt2);
    g.drawText ("Exceso actual", juce::Rectangle<float> (r.getX(), y, 120, rowH), juce::Justification::centredLeft);
    g.setFont (Fonts::mono (12.0f));
    g.setColour (now > 0.05f ? c.over : c.txt);
    g.drawText (juce::String (now, 1).replaceCharacter ('.', ',') + " dB", juce::Rectangle<float> (r.getX() + 96, y, 70, rowH), juce::Justification::centredLeft);
    g.setFont (Fonts::sans (12.0f));
    g.setColour (c.txt2);
    g.drawText ("Densidad", juce::Rectangle<float> (c2 - 30, y, 70, rowH), juce::Justification::centredLeft);
    g.setFont (Fonts::mono (12.0f));
    g.setColour (c.txt);
    g.drawText (formatDb (hub.densNow) + " dB", juce::Rectangle<float> (c3 - 20, y, 70, rowH), juce::Justification::centredRight);
    y += rowH;
    g.setFont (Fonts::sans (12.0f, true));
    g.setColour (c.txt);
    g.drawText (es ("Exceso máximo"), juce::Rectangle<float> (r.getX(), y, 120, rowH), juce::Justification::centredLeft);
    g.setFont (Fonts::mono (12.0f));
    g.setColour (mx > 0.05f ? c.over : c.txt);
    g.drawText ((mx > 0.005f ? juce::String (mx, 1).replaceCharacter ('.', ',') + " dB" : es ("—")) + es ("  ·  ")
                    + juce::String::formatted ("%02d:%02d", secs / 60, secs % 60),
                juce::Rectangle<float> (c2 - 40, y, r.getRight() - c2 + 40, rowH), juce::Justification::centredRight);
}

// ===================================================================================== Curva E/S
CurvePanel::CurvePanel (juce::AudioProcessorValueTreeState& s) : Overlay (es ("CURVA E/S")), state (s) {}

void CurvePanel::tick()
{
    const float d = state.getRawParameterValue (id::densidad)->load();
    const int m = (int) state.getRawParameterValue (id::modo)->load();
    if (d != dens || m != mode) { dens = d; mode = m; repaint(); }
}

void CurvePanel::paint (juce::Graphics& g)
{
    Overlay::paint (g);
    auto r = content();
    static const char* names[] = { "Suave", "Medio", "Duro", "Sin recorte" };
    g.setColour (pal().txt2);
    g.setFont (Fonts::sans (11.0f));
    g.drawText (es ("Curva ") + es (names[juce::jlimit (0, 3, mode)]) + es (" · Densidad ") + formatDb (dens) + es (" dB · dB relativos al Techo"),
                r.removeFromTop (16), juce::Justification::centredLeft);
    drawTransferCurve (g, r.reduced (0, 4).toFloat(), dens, mode);
}

// ===================================================================================== Avanzado
AdvancedPanel::AdvancedPanel (juce::AudioProcessorValueTreeState& s)
    : Overlay ("AVANZADO"),
      offline (s, id::calidadOffline, { es ("Igual"), es ("1×"), es ("2×"), es ("4×"), es ("8×") }, 3.0f),
      fase (s, id::fase, { es ("Lineal"), es ("Mínima") }, 3.0f),
      tono (s, id::tono, { es ("Plano"), es ("Seda"), es ("Terciopelo") }, 3.0f),
      truePeak (s, id::techoEstricto, es ("Activado"), es ("Desactivado")),
      rango (s, id::suavExt, es ("Hasta −3 dB"), es ("Hasta −2 dB")),
      compEntrada (s, id::enlaceES, es ("Activada"), es ("Desactivada")),
      compDensidad (s, id::gananciaUnitaria, es ("Activada"), es ("Desactivada"))
{
    offline.setTooltip (es ("Oversampling usado cuando la DAW renderiza fuera de tiempo real."));
    fase.setTooltip (es ("Lineal conserva la fase al mezclar Procesada y Original. Mínima reduce la latencia y puede cambiar la suma en agudos."));
    truePeak.setTooltip (es ("Anticipa los picos entre muestras y usa el Techo como objetivo en dBTP sobre toda la mezcla."));
    tono.setTooltip (es ("Ajuste tonal permanente de la rama procesada: 0, −1 o −3 dB a 10 kHz."));
    rango.setTooltip (es ("Atenuación máxima de Smooth con la perilla al 100 %."));
    compEntrada.setTooltip (es ("Aplica la ganancia inversa de Entrada después del procesamiento (Shift+arrastrar Entrada ajusta el offset)."));
    compDensidad.setTooltip (es ("Resta el valor nominal de Densidad en la rama procesada para comparar a igual nivel."));
    rows = { { es ("Oversampling al exportar"), &offline }, { es ("Fase del filtrado"), &fase }, { es ("Limitador True Peak"), &truePeak },
             { es ("Atenuación fija de agudos"), &tono }, { es ("Rango de Smooth"), &rango }, { es ("Compensación de entrada"), &compEntrada },
             { es ("Compensación de Densidad"), &compDensidad } };
    offline.fontSize = fase.fontSize = tono.fontSize = 11.5f;
    for (auto& [_, comp] : rows) addAndMakeVisible (comp);
}

void AdvancedPanel::resized()
{
    auto r = content();
    const int h = r.getHeight() / (int) rows.size();
    for (auto& [label, comp] : rows)
    {
        auto line = r.removeFromTop (h);
        const bool seg = dynamic_cast<Segmented*> (comp) != nullptr;
        const int w = comp == &offline ? 190 : (comp == &tono ? 196 : (seg ? 160 : 130));
        comp->setBounds (line.removeFromRight (w).withSizeKeepingCentre (w, 24));
    }
}

void AdvancedPanel::paint (juce::Graphics& g)
{
    Overlay::paint (g);
    const auto& c = pal();
    auto r = content();
    const int h = r.getHeight() / (int) rows.size();
    for (size_t i = 0; i < rows.size(); ++i)
    {
        auto line = r.removeFromTop (h);
        g.setColour (c.txt);
        g.setFont (Fonts::sans (12.0f));
        g.drawText (rows[i].first, line, juce::Justification::centredLeft);
        if (i + 1 < rows.size()) { g.setColour (c.txt2.withAlpha (0.18f)); g.drawHorizontalLine (line.getBottom(), (float) line.getX(), (float) line.getRight()); }
    }
}

// ===================================================================================== Apariencia
AppearancePanel::AppearancePanel()
    : Overlay ("APARIENCIA"),
      cancelBtn (es ("Cancelar"), es ("Restaura la apariencia anterior (Esc).")),
      applyBtn (es ("Aplicar"), es ("Guarda el tema como preferencia de este equipo (Enter)."))
{
    setWantsKeyboardFocus (true);
    addAndMakeVisible (cancelBtn);
    addAndMakeVisible (applyBtn);
    applyBtn.setToggleState (true, juce::dontSendNotification);
    cancelBtn.onClick = [this] { cancel(); };
    applyBtn.onClick = [this] { apply(); };
}

void AppearancePanel::open()
{
    original = selected = Themes::get().currentIndex();
    grabKeyboardFocus();
    repaint();
}

juce::Rectangle<int> AppearancePanel::row (int i) const
{
    auto r = content().removeFromLeft (250);
    return r.withHeight (30).translated (0, i * 32);
}

void AppearancePanel::resized()
{
    auto r = content();
    auto bottom = r.removeFromBottom (30);
    applyBtn.setBounds (bottom.removeFromRight (100));
    bottom.removeFromRight (10);
    cancelBtn.setBounds (bottom.removeFromRight (100));
}

void AppearancePanel::paint (juce::Graphics& g)
{
    Overlay::paint (g);
    const auto& c = pal();
    auto& th = Themes::get();
    for (int i = 0; i < Themes::kCount; ++i)
    {
        const auto& t = th.palette (i);
        auto r = row (i).toFloat();
        if (i == selected)
        {
            g.setColour (c.proc);
            g.fillRoundedRectangle (r, 7.0f);
            g.setColour (c.foco);
            g.drawRoundedRectangle (r.reduced (0.5f), 7.0f, 1.5f);
        }
        float x = r.getX() + 10.0f;
        for (auto col : { t.sup, t.graf, t.sel })
        {
            g.setColour (col);
            g.fillRoundedRectangle (x, r.getCentreY() - 9.0f, 12.0f, 18.0f, 3.0f);
            g.setColour (juce::Colours::black.withAlpha (0.15f));
            g.drawRoundedRectangle (x, r.getCentreY() - 9.0f, 12.0f, 18.0f, 3.0f, 1.0f);
            x += 14.0f;
        }
        g.setColour (c.txt);
        g.setFont (Fonts::sans (13.0f, i == selected));
        g.drawText (juce::String (i + 1) + es (" · ") + t.nombre, r.withTrimmedLeft (62.0f), juce::Justification::centredLeft);
        g.setColour (c.txt2);
        g.setFont (Fonts::sans (11.0f));
        g.drawText (i == original ? es ("En uso") : t.tipo, r.withTrimmedRight (10.0f), juce::Justification::centredRight);
    }
    // vista previa: sticker y datos del tema seleccionado
    auto right = content().withTrimmedLeft (270).withTrimmedBottom (40);
    const auto img = th.sticker (selected);
    auto stickerArea = right.removeFromTop (right.getHeight() - 70).toFloat();
    g.drawImage (img, stickerArea.reduced (10.0f), juce::RectanglePlacement::centred);
    const auto& t = th.palette (selected);
    g.setColour (c.txt);
    g.setFont (Fonts::brand (20.0f));
    g.drawText (t.nombre, right.removeFromTop (28), juce::Justification::centred);
    g.setColour (c.txt2);
    g.setFont (Fonts::sans (12.0f));
    g.drawText (t.tipo + es (" · base ") + t.base, right.removeFromTop (18), juce::Justification::centred);
    g.drawText (es ("↑ ↓ cambian la vista previa · Enter aplica · Esc restaura"), right.removeFromTop (18), juce::Justification::centred);
}

void AppearancePanel::preview (int i)
{
    selected = juce::jlimit (0, Themes::kCount - 1, i);
    Themes::get().setIndex (selected, false);
    repaint();
}

void AppearancePanel::mouseUp (const juce::MouseEvent& e)
{
    for (int i = 0; i < Themes::kCount; ++i)
        if (row (i).contains (e.getPosition())) { preview (i); return; }
}

bool AppearancePanel::keyPressed (const juce::KeyPress& k)
{
    const int code = k.getKeyCode();
    if (code == juce::KeyPress::downKey) { preview (selected + 1); return true; }
    if (code == juce::KeyPress::upKey) { preview (selected - 1); return true; }
    if (code == juce::KeyPress::returnKey) { apply(); return true; }
    if (code == juce::KeyPress::escapeKey) { cancel(); return true; }
    return false;
}

void AppearancePanel::cancel()
{
    Themes::get().setIndex (original, false);
    if (onClose) onClose();
}

void AppearancePanel::apply()
{
    Themes::get().setIndex (selected, true);
    if (onClose) onClose();
}
} // namespace clip5::ui
