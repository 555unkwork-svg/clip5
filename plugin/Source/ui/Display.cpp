#include "Display.h"
#include "clip5/Curves.h"

namespace clip5::ui
{
using params::es;
using params::formatDb;

HistoryDisplay::HistoryDisplay (MeterHub& h) : hub (h)
{
    setOpaque (false);
    setInterceptsMouseClicks (false, false);
}

juce::Rectangle<int> HistoryDisplay::plotArea() const
{
    return { 12, 54, getWidth() - 58, getHeight() - 118 };
}

float HistoryDisplay::yFor (float db, juce::Rectangle<float> plot) const noexcept
{
    const float hi = std::max (3.0f, ceilDb + 6.0f), lo = hi - 36.0f;
    return plot.getY() + (hi - juce::jlimit (lo, hi, db)) / (hi - lo) * plot.getHeight();
}

void HistoryDisplay::tick (float c)
{
    ceilDb = c;
    if (hub.totalColumns() != lastTotal || paintedCeil != ceilDb)
    {
        lastTotal = hub.totalColumns();
        repaint();
    }
}

void HistoryDisplay::paint (juce::Graphics& g)
{
    paintedCeil = ceilDb;
    const auto& c = pal();
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (c.graphPanel());
    g.fillRoundedRectangle (bounds, 10.0f);
    const auto plot = plotArea().toFloat();
    g.setColour (c.graf);
    g.fillRect (plot);

    // ---- rejilla y escalas
    const auto sc = c.graphScale();
    const float hi = std::max (3.0f, ceilDb + 6.0f), lo = hi - 36.0f;
    g.setFont (Fonts::mono (10.0f));
    for (float d = std::floor (hi / 6.0f) * 6.0f; d >= lo; d -= 6.0f)
    {
        const float y = yFor (d, plot);
        g.setColour (sc.withAlpha (0.18f));
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.setColour (sc);
        g.drawText (d == 0 ? juce::String ("0") : formatDb (d, 0), (int) plot.getRight() + 8, (int) y - 7, 40, 14, juce::Justification::left);
    }
    g.drawText ("dBFS", (int) plot.getRight() + 8, (int) plot.getBottom() - 4, 40, 12, juce::Justification::left);
    const int W = (int) plot.getWidth();
    for (int s = 0; s <= 8; s += 2)
    {
        const float x = plot.getRight() - (float) s * 100.0f;
        if (x < plot.getX() + 20) break;
        g.setColour (sc.withAlpha (0.12f));
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
        g.setColour (sc);
        g.drawText (s == 0 ? juce::String ("0 s") : es ("−") + juce::String (s) + " s", (int) x - (s == 0 ? 26 : 14), (int) plot.getY() - 16, 40, 12,
                    s == 0 ? juce::Justification::right : juce::Justification::left);
    }

    // ---- datos (una columna por píxel, de derecha a izquierda)
    const int n = std::min (W, hub.columnsAvailable());
    if (n > 1)
    {
        juce::Path in, outLine;
        in.startNewSubPath (plot.getRight(), plot.getBottom());
        for (int i = 0; i < n; ++i)
        {
            const auto& col = hub.column (i);
            const float x = plot.getRight() - (float) i;
            in.lineTo (x, yFor (col.inDb, plot));
            const float yo = yFor (col.outDb, plot);
            if (i == 0) outLine.startNewSubPath (x, yo); else outLine.lineTo (x, yo);
        }
        in.lineTo (plot.getRight() - (float) (n - 1), plot.getBottom());
        in.closeSubPath();
        g.saveState();
        g.reduceClipRegion (plot.toNearestInt());
        g.setColour (c.inFill.withAlpha (0.9f));
        g.fillPath (in);
        g.setColour (c.inBorde);
        g.strokePath (in, juce::PathStrokeType (1.1f));
        g.setColour (c.outCont);
        g.strokePath (outLine, juce::PathStrokeType (c.outCont == c.outLine ? 1.8f : 3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (c.outLine);
        g.strokePath (outLine, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        // sobre el techo: exceso antes de la curva
        g.setColour (c.overArea);
        const float yC = yFor (ceilDb, plot);
        for (int i = 0; i < n; ++i)
        {
            const auto& col = hub.column (i);
            if (col.clipDb > 0.05f)
            {
                const float y = yFor (col.ceilDb + col.clipDb, plot);
                g.fillRect (plot.getRight() - (float) i - 0.5f, y, 1.6f, std::max (1.0f, yC - y));
            }
        }
        g.restoreState();
        // banda de Densidad
        g.setColour (c.dens);
        for (int i = 0; i < n; ++i)
        {
            const float h = juce::jlimit (0.0f, 1.0f, hub.column (i).densDb / 6.0f) * 13.0f;
            if (h > 0.3f) g.fillRect (plot.getRight() - (float) i - 0.5f, plot.getBottom() + 18.0f - h, 1.2f, h);
        }
    }

    // ---- techo
    const float yC = yFor (ceilDb, plot);
    g.setColour (c.over);
    const float dashes[] = { 6.0f, 4.0f };
    g.drawDashedLine ({ plot.getX(), yC, plot.getRight(), yC }, dashes, 2, 1.6f);

    // ---- leyenda
    const float ly = bounds.getBottom() - 22.0f;
    float x = plot.getX();
    g.setFont (Fonts::sans (11.0f));
    auto label = [&] (const juce::String& t) { g.setColour (sc); g.drawText (t, (int) x, (int) ly - 7, 120, 14, juce::Justification::left); x += juce::GlyphArrangement::getStringWidth (Fonts::sans (11.0f), t) + 18.0f; };
    g.setColour (c.inFill); g.fillRect (x, ly - 4.0f, 10.0f, 8.0f); g.setColour (c.inBorde); g.drawRect (x, ly - 4.0f, 10.0f, 8.0f); x += 15; label ("Entrada");
    g.setColour (c.outCont); g.fillRect (x, ly - 2.0f, 13.0f, 4.0f); g.setColour (c.outLine); g.fillRect (x, ly - 1.0f, 13.0f, 2.0f); x += 18; label ("Salida");
    g.setColour (c.over); g.drawDashedLine ({ x, ly, x + 16.0f, ly }, dashes, 2, 1.6f); x += 21; label (es ("Sobre el techo"));
    g.setColour (c.dens); g.fillRect (x, ly - 5.0f, 3.0f, 9.0f); g.fillRect (x + 4.0f, ly - 2.0f, 3.0f, 6.0f); x += 12; label ("Densidad");

    auto chip = juce::Rectangle<float> (134.0f, 20.0f).withRightX (plot.getRight() + 12.0f).withCentre ({ plot.getRight() - 55.0f, ly });
    g.setColour (c.graphButton());
    g.fillRoundedRectangle (chip, 5.0f);
    g.setColour (c.over);
    g.fillEllipse (chip.getX() + 8.0f, ly - 3.0f, 6.0f, 6.0f);
    g.setFont (Fonts::mono (10.5f));
    g.drawText (es ("Techo ") + formatDb (ceilDb) + " dBFS", chip.withTrimmedLeft (18.0f), juce::Justification::centredLeft);
}

void drawTransferCurve (juce::Graphics& g, juce::Rectangle<float> area, float densityDb, int mode)
{
    const auto& c = pal();
    g.setColour (c.proc);
    g.fillRoundedRectangle (area, 6.0f);
    auto plot = area.reduced (14.0f);
    auto map = [&] (float din, float dout) {
        return juce::Point<float> (plot.getX() + (din + 36.0f) / 42.0f * plot.getWidth(),
                                   plot.getBottom() - (std::min (dout, 6.0f) + 36.0f) / 42.0f * plot.getHeight());
    };
    g.setFont (Fonts::mono (9.5f));
    for (float d : { -30.0f, -18.0f, -6.0f })
    {
        g.setColour (c.txt2.withAlpha (0.2f));
        g.drawLine ({ map (d, -36), map (d, 6) }, 1.0f);
        g.drawLine ({ map (-36, d), map (6, d) }, 1.0f);
    }
    const float dashes[] = { 3.0f, 3.0f };
    g.setColour (c.txt2.withAlpha (0.7f));
    g.drawDashedLine ({ map (-36, -36), map (6, 6) }, dashes, 2, 1.0f);
    const float d2[] = { 6.0f, 4.0f };
    g.setColour (c.over);
    g.drawDashedLine ({ map (-36, 0), map (6, 0) }, d2, 2, 1.5f);
    juce::Path p;
    for (int i = 0; i <= 160; ++i)
    {
        const double din = -36.0 + 42.0 * i / 160.0;
        const double y = curves::shaper (std::pow (10.0, din / 20.0), densityDb, (ClipMode) mode);
        const auto pt = map ((float) din, (float) (20.0 * std::log10 (std::max (y, 1e-6))));
        if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
    }
    g.setColour (c.sel);
    g.strokePath (p, juce::PathStrokeType (2.6f));
    g.setColour (c.txt2);
    g.drawText (es ("entrada (dB re techo) →"), plot.removeFromBottom (12.0f), juce::Justification::right);
}
} // namespace clip5::ui
