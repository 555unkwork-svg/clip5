#include "MeterPanel.h"

namespace clip5::ui
{
using params::es;
using params::formatDb;

namespace
{
    juce::String lufs (double v) { return v < -99.0 ? es ("—") : formatDb ((float) v); }
    juce::String dbOrDash (float v) { return v < -119.0f ? es ("—") : formatDb (v); }
    void box (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title, const juce::String& right = {})
    {
        g.setColour (col::sup);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (col::lin);
        g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);
        g.setFont (Fonts::sans (10.5f, true));
        g.setColour (col::txt2);
        auto t = r.reduced (10.0f, 8.0f).removeFromTop (14.0f);
        g.drawText (title, t, juce::Justification::left);
        if (right.isNotEmpty())
        {
            g.setFont (Fonts::sans (10.5f));
            g.drawText (right, t, juce::Justification::right);
        }
    }
}

MeterPanel::MeterPanel (MeterHub& h) : hub (h)
{
    setTooltip (es ("Medidores de entrada y salida. Pico y true peak: máximos desde el último reinicio; "
                    "LUFS momentáneo, de corto plazo e integrado (ITU-R BS.1770-4). Clic para reiniciar."));
}

void MeterPanel::tick (float c)
{
    ceilDb = c;
    repaint();
}

void MeterPanel::mouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasClicked()) hub.resetReadouts();
}

void MeterPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    box (g, r, es ("ENTRADA · SALIDA"), "dBFS");
    auto inner = r.reduced (10.0f).withTrimmedTop (22.0f);

    // escala
    auto scale = inner.removeFromLeft (22.0f);
    const float barTop = inner.getY() + 4.0f, barBot = inner.getBottom() - 16.0f;
    auto yFor = [&] (float db) { return barBot - juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f) * (barBot - barTop); };
    g.setFont (Fonts::mono (9.5f));
    g.setColour (col::txt2);
    for (float d : { 0.0f, -6.0f, -12.0f, -24.0f, -48.0f })
        g.drawText (d == 0 ? juce::String ("0") : formatDb (d, 0), scale.withY (yFor (d) - 6.0f).withHeight (12.0f), juce::Justification::left);

    auto bar = [&] (float x, float db, float hold, bool out) {
        auto rr = juce::Rectangle<float> (x, barTop, 9.0f, barBot - barTop);
        g.setColour (col::pista);
        g.fillRoundedRectangle (rr, 2.0f);
        g.setColour (out ? col::txt : col::txt2);
        g.fillRoundedRectangle (rr.withTop (yFor (db)), 2.0f);
        g.setColour (col::txt);
        g.fillRect (x, yFor (hold) - 1.0f, 9.0f, 2.0f);
        if (out)
        {
            g.setColour (col::coral);
            g.fillRect (x - 2.0f, yFor (ceilDb), 13.0f, 1.0f);
        }
    };
    float x = inner.getX();
    bar (x, hub.barIn[0], hub.holdIn[0], false);
    bar (x + 12, hub.barIn[1], hub.holdIn[1], false);
    bar (x + 32, hub.barOut[0], hub.holdOut[0], true);
    bar (x + 44, hub.barOut[1], hub.holdOut[1], true);
    g.setFont (Fonts::sans (11.0f));
    g.setColour (col::txt2);
    g.drawText ("E", juce::Rectangle<float> (x, barBot + 2, 21, 14), juce::Justification::centred);
    g.drawText ("S", juce::Rectangle<float> (x + 32, barBot + 2, 21, 14), juce::Justification::centred);

    // lecturas
    auto rd = inner.withTrimmedLeft (64.0f);
    const float rowH = 16.0f;
    const float c1 = rd.getX(), c2 = rd.getRight() - 84.0f, c3 = rd.getRight() - 40.0f;
    auto row = [&] (float y, const juce::String& k, const juce::String& a, const juce::String& b, juce::Colour cb = col::txt) {
        g.setFont (Fonts::mono (11.0f));
        g.setColour (col::txt2);
        g.drawText (k, juce::Rectangle<float> (c1, y, 60, rowH), juce::Justification::left);
        g.setColour (col::txt);
        g.drawText (a, juce::Rectangle<float> (c2, y, 40, rowH), juce::Justification::right);
        g.setColour (cb);
        g.drawText (b, juce::Rectangle<float> (c3, y, 40, rowH), juce::Justification::right);
    };
    float y = rd.getY();
    g.setFont (Fonts::sans (10.0f));
    g.setColour (col::txt2);
    g.drawText ("E", juce::Rectangle<float> (c2, y, 40, rowH), juce::Justification::right);
    g.drawText ("S", juce::Rectangle<float> (c3, y, 40, rowH), juce::Justification::right);
    y += rowH;
    row (y, "Pico", dbOrDash (hub.peakInMax), dbOrDash (hub.peakOutMax)); y += rowH;
    row (y, "TP", es ("—"), dbOrDash (hub.tpOutMax), hub.tpOutMax > ceilDb + 0.05f ? col::coral : col::txt); y += rowH;
    row (y, "LUFS M", lufs (hub.lufsIn.momentary()), lufs (hub.lufsOut.momentary())); y += rowH;
    row (y, "LUFS S", lufs (hub.lufsIn.shortTerm()), lufs (hub.lufsOut.shortTerm())); y += rowH;
    row (y, "LUFS I", lufs (hub.lufsIn.integrated()), lufs (hub.lufsOut.integrated())); y += rowH + 6;
    row (y, "Recorte", {}, juce::String (hub.clipNow, 1).replaceCharacter ('.', ','), hub.clipNow > 0.05f ? col::coral : col::txt2); y += rowH;
    row (y, "Densidad", {}, formatDb (hub.densNow), col::menta);
}

// =====================================================================================
ClipClockView::ClipClockView (Engine& e) : engine (e)
{
    setTooltip (es ("Reloj de recorte: recorte máximo y el momento en que ocurrió (posición del host, o tiempo "
                    "desde que empezó la reproducción si no hay transporte). Clic para reiniciar."));
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void ClipClockView::tick()
{
    const float d = engine.clipClock().maxClipDb.load();
    const double t = engine.clipClock().atSeconds.load();
    if (d != shownDb || t != shownAt) { shownDb = d; shownAt = t; repaint(); }
}

void ClipClockView::mouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasClicked()) engine.clipClock().resetRequest.store (true);
}

void ClipClockView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    box (g, r, es ("RELOJ DE RECORTE"), es ("clic: reiniciar"));
    auto inner = r.reduced (10.0f).withTrimmedTop (20.0f);
    g.setFont (Fonts::mono (22.0f));
    g.setColour (shownDb > 0.05f ? col::coral : col::txt2);
    g.drawText ((shownDb > 0.005f ? formatDb (-shownDb) : juce::String ("0,0")) + " dB", inner.removeFromTop (28.0f), juce::Justification::left);
    const int secs = (int) std::max (0.0, shownAt);
    g.setFont (Fonts::mono (11.0f));
    g.setColour (col::txt2);
    g.drawText (es ("en ") + juce::String::formatted ("%02d:%02d", secs / 60, secs % 60), inner.removeFromTop (16.0f), juce::Justification::left);
}
} // namespace clip5::ui
