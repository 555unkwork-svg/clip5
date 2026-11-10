#include "Display.h"
#include "clip5/Curves.h"

namespace clip5::ui
{
using params::es;
using params::formatDb;

// =====================================================================================
TransferCurve::TransferCurve()
{
    setTooltip (es ("Curva de transferencia actual: entrada contra salida en dB relativos al techo. "
                    "Se actualiza con Densidad y Modo."));
    setInterceptsMouseClicks (true, false);
}

void TransferCurve::set (float d, int m)
{
    if (d == density && m == mode) return;
    density = d;
    mode = m;
    rebuild();
    repaint();
}

void TransferCurve::rebuild()
{
    curve.clear();
    for (int i = 0; i <= 160; ++i)
    {
        const double din = -36.0 + 42.0 * i / 160.0;
        const double y = curves::shaper (std::pow (10.0, din / 20.0), density, (ClipMode) mode);
        const double dout = std::min (6.0, 20.0 * std::log10 (std::max (y, 1e-6)));
        const float x = (float) ((din + 36.0) / 42.0), yy = (float) ((dout + 36.0) / 42.0);
        if (i == 0) curve.startNewSubPath (x, 1.0f - yy); else curve.lineTo (x, 1.0f - yy);
    }
}

void TransferCurve::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (col::fondo.withAlpha (0.92f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (col::lin);
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
    g.setColour (col::txt2);
    g.setFont (Fonts::sans (10.5f));
    g.drawText (es ("Transferencia"), r.reduced (8, 5).removeFromTop (14), juce::Justification::topLeft);

    auto plot = r.reduced (12.0f).withTrimmedTop (10.0f);
    auto map = [&] (float nx, float ny) { return juce::Point<float> (plot.getX() + nx * plot.getWidth(), plot.getY() + ny * plot.getHeight()); };
    g.setColour (col::lin);
    for (float d : { -30.0f, -18.0f, -6.0f })
    {
        const float n = (d + 36.0f) / 42.0f;
        g.drawLine ({ map (n, 0), map (n, 1) }, 1.0f);
        g.drawLine ({ map (0, 1 - n), map (1, 1 - n) }, 1.0f);
    }
    // identidad
    juce::Path diag;
    diag.startNewSubPath (map (0, 1));
    diag.lineTo (map (1, 0));
    float dashes[] = { 3.0f, 3.0f };
    juce::Path dashed;
    juce::PathStrokeType (1.0f).createDashedStroke (dashed, diag, dashes, 2);
    g.setColour (col::txt2);
    g.fillPath (dashed);
    // techo
    const float n0 = 36.0f / 42.0f;
    g.setColour (col::coral);
    g.drawLine ({ map (0, 1 - n0), map (1, 1 - n0) }, 1.2f);
    // curva
    g.setColour (col::menta);
    g.strokePath (curve, juce::PathStrokeType (2.0f), juce::AffineTransform::scale (plot.getWidth(), plot.getHeight()).translated (plot.getX(), plot.getY()));
}

// =====================================================================================
HistoryDisplay::HistoryDisplay (MeterHub& h) : hub (h)
{
    addAndMakeVisible (transfer);
    setOpaque (true);
}

void HistoryDisplay::resized()
{
    transfer.setBounds (getWidth() - 162, 10, 152, 152);
    bgCeil = 1000.0f;
}

float HistoryDisplay::yFor (float db) const noexcept
{
    // escala: de max(0 dBFS, techo + 6) hacia abajo, 36 dB de rango
    const float top = 34.0f, bottom = (float) getHeight() - 28.0f;
    const float hi = std::max (0.0f, ceilDb + 6.0f), lo = hi - 36.0f;
    return top + (hi - juce::jlimit (lo, hi, db)) / (hi - lo) * (bottom - top);
}

void HistoryDisplay::renderBackground()
{
    const float scale = (float) juce::Component::getApproximateScaleFactorForComponent (this);
    background = juce::Image (juce::Image::RGB, std::max (1, (int) (getWidth() * scale)), std::max (1, (int) (getHeight() * scale)), true);
    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale (scale));
    g.fillAll (col::sup);
    g.setFont (Fonts::mono (10.5f));
    // rejilla en múltiplos absolutos de 6 dBFS dentro del rango visible
    const float hi = std::max (0.0f, ceilDb + 6.0f), lo = hi - 36.0f;
    for (float db = std::floor (hi / 6.0f) * 6.0f; db >= lo; db -= 6.0f)
    {
        const float y = yFor (db);
        g.setColour (col::lin);
        g.drawHorizontalLine ((int) y, 0.0f, (float) getWidth());
        g.setColour (col::txt2);
        g.drawText (formatDb (db, 0), 6, (int) y - 14, 40, 12, juce::Justification::left);
    }
    // leyenda
    struct Item { juce::Colour c; const char* t; };
    const Item items[] = { { col::txt, "Salida" }, { col::coral, "Recorte" }, { col::menta, "Densidad" }, { col::txt2.withAlpha (0.6f), "Entrada" } };
    float x = 48.0f;
    g.setFont (Fonts::sans (11.0f));
    for (const auto& it : items)
    {
        g.setColour (it.c);
        g.fillRoundedRectangle (x, 13.0f, 10.0f, 3.0f, 1.5f);
        g.setColour (col::txt2);
        const auto t = es (it.t);
        g.drawText (t, (int) x + 15, 6, 80, 16, juce::Justification::left);
        x += 22.0f + (float) juce::GlyphArrangement::getStringWidthInt (Fonts::sans (11.0f), t);
    }
    bgCeil = ceilDb;
}

void HistoryDisplay::tick (float c)
{
    ceilDb = c;
    if (hub.totalColumns() != lastTotal || bgCeil != ceilDb)
    {
        lastTotal = hub.totalColumns();
        repaint (getLocalBounds().withTrimmedRight (0));
    }
}

void HistoryDisplay::paint (juce::Graphics& g)
{
    if (bgCeil != ceilDb || background.isNull()) renderBackground();
    g.drawImage (background, getLocalBounds().toFloat());

    const int W = getWidth();
    const int n = std::min (W, hub.columnsAvailable());
    const float bottom = (float) getHeight();

    // entrada (gris) y salida (marfil) — una columna por píxel, de derecha a izquierda
    juce::Path in, out, outLine;
    in.startNewSubPath ((float) W, bottom);
    out.startNewSubPath ((float) W, bottom);
    for (int i = 0; i < n; ++i)
    {
        const auto& c = hub.column (i);
        const float x = (float) (W - 1 - i);
        in.lineTo (x, yFor (c.inDb));
        out.lineTo (x, yFor (c.outDb));
        if (i == 0) outLine.startNewSubPath (x, yFor (c.outDb)); else outLine.lineTo (x, yFor (c.outDb));
    }
    in.lineTo ((float) (W - n), bottom);
    out.lineTo ((float) (W - n), bottom);
    in.closeSubPath();
    out.closeSubPath();
    g.setColour (col::txt2.withAlpha (0.14f));
    g.fillPath (in);
    g.setColour (col::txt.withAlpha (0.07f));
    g.fillPath (out);
    g.setColour (col::txt);
    g.strokePath (outLine, juce::PathStrokeType (1.3f));

    // recorte (coral): del techo al nivel pre-recorte, solo donde hubo recorte
    const float yC = yFor (ceilDb);
    g.setColour (col::coral.withAlpha (0.6f));
    for (int i = 0; i < n; ++i)
    {
        const auto& c = hub.column (i);
        if (c.clipDb > 0.05f)
        {
            const float y = yFor (c.ceilDb + c.clipDb);
            g.fillRect ((float) (W - 1 - i), y, 1.0f, std::max (1.0f, yC - y));
        }
    }
    // Densidad (menta): banda inferior proporcional a la ganancia aplicada
    g.setColour (col::menta.withAlpha (0.75f));
    for (int i = 0; i < n; ++i)
    {
        const float h = hub.column (i).densDb / 6.0f * 18.0f;
        if (h > 0.2f) g.fillRect ((float) (W - 1 - i), bottom - 2.0f - h, 1.0f, h);
    }
    // línea de techo
    g.setColour (col::coral);
    g.drawHorizontalLine ((int) yC, 0.0f, (float) W);
    g.setFont (Fonts::sans (11.0f));
    g.drawText (es ("techo ") + formatDb (ceilDb) + " dB", 48, (int) yC + 3, 150, 14, juce::Justification::left);
}
} // namespace clip5::ui
