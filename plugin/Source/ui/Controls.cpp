#include "Controls.h"
#include "../Params.h"
#include "clip5/Curves.h"

namespace clip5::ui
{
using params::es;

// =====================================================================================
ParamControl::ParamControl (juce::AudioProcessorValueTreeState& s, const juce::String& paramId) : state (s)
{
    p = s.getParameter (paramId);
    jassert (p != nullptr);
    att = std::make_unique<juce::ParameterAttachment> (*p, [this] (float v) { current = v; valueChanged(); }, s.undoManager);
    att->sendInitialUpdate();
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void ParamControl::setNorm (float n) { att->setValueAsPartOfGesture (p->convertFrom0to1 (juce::jlimit (0.0f, 1.0f, n))); }
void ParamControl::setReal (float v) { att->setValueAsCompleteGesture (p->getNormalisableRange().snapToLegalValue (v)); }

// =====================================================================================
Knob::Knob (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, const juce::String& n,
            Style st, bool bip, std::function<juce::String (float)> fmt)
    : ParamControl (s, paramId), name (n), style (st), bipolar (bip), format (std::move (fmt))
{
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void Knob::resized()
{
    const float labelH = compact ? 30.0f : 40.0f;
    auto r = getLocalBounds().toFloat();
    const float size = std::min (r.getWidth(), r.getHeight() - labelH);
    dial = juce::Rectangle<float> (size, size).withCentre ({ r.getCentreX(), r.getY() + size * 0.5f });
}

void Knob::paint (juce::Graphics& g)
{
    // JUCE: ángulo 0 = arriba, sentido horario → de −135° a +135°
    const float start = -juce::MathConstants<float>::pi * 0.75f, end = juce::MathConstants<float>::pi * 0.75f;
    const float n = norm();
    const auto c = dial.getCentre();
    const float r = dial.getWidth() * 0.5f - 4.0f;
    const float stroke = compact ? 3.0f : 4.0f;

    juce::Path track;
    track.addCentredArc (c.x, c.y, r, r, 0, start, end, true);
    g.setColour (col::lin);
    g.strokePath (track, juce::PathStrokeType (stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float aN = start + (end - start) * n;
    float aC = start;
    if (bipolar) aC = start + (end - start) * p->convertTo0to1 (0.0f);
    if (std::abs (aN - aC) > 0.001f)
    {
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, r, r, 0, std::min (aN, aC), std::max (aN, aC), true);
        const auto colour = style == Style::Menta ? col::menta : (style == Style::Menta55 ? col::menta55() : col::txt);
        g.setColour (colour);
        g.strokePath (arc, juce::PathStrokeType (stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    const float inner = r - (compact ? 7.0f : 9.0f);
    g.setColour (col::sup);
    g.fillEllipse (juce::Rectangle<float> (inner * 2, inner * 2).withCentre (c));
    g.setColour (col::lin);
    g.drawEllipse (juce::Rectangle<float> (inner * 2, inner * 2).withCentre (c), 1.0f);
    const auto tip = c.getPointOnCircumference (inner - 4.0f, aN);
    const auto base = c.getPointOnCircumference (inner * 0.45f, aN);
    g.setColour (col::txt);
    g.drawLine ({ base, tip }, 2.4f);

    // valor y nombre
    auto text = getLocalBounds().toFloat().withTop (dial.getBottom() + 2.0f);
    if (editor == nullptr)
    {
        g.setColour (col::txt);
        g.setFont (Fonts::mono (compact ? 12.0f : 13.5f));
        g.drawText (format (current), text.removeFromTop (compact ? 15.0f : 18.0f), juce::Justification::centred);
    }
    else text.removeFromTop (compact ? 15.0f : 18.0f);
    g.setColour (dragging ? col::txt : col::txt2);
    g.setFont (Fonts::sans (compact ? 10.5f : 11.5f, true));
    g.drawText (nameOverride.isNotEmpty() ? nameOverride : name, text.removeFromTop (16.0f), juce::Justification::centred);
}

void Knob::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isCommandDown() || e.mods.isCtrlDown())
    {
        setReal (p->convertFrom0to1 (p->getDefaultValue()));
        return;
    }
    dragging = true;
    draggingOther = e.mods.isShiftDown() && shiftTarget && shiftTarget() != nullptr;
    dragStartY = (float) e.position.y;
    lastFine = e.mods.isShiftDown() && ! draggingOther;
    if (draggingOther)
    {
        auto* o = shiftTarget();
        dragStartNorm = o->param().getValue();
        o->param().beginChangeGesture();
    }
    else
    {
        dragStartNorm = norm();
        att->beginGesture();
    }
    repaint();
}

void Knob::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging) return;
    const bool fine = e.mods.isShiftDown() && ! draggingOther;
    const float delta = (dragStartY - (float) e.position.y) / 220.0f * (fine ? 0.1f : 1.0f);
    if (draggingOther)
    {
        auto& op = shiftTarget()->param();
        op.setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, dragStartNorm + delta));
        return;
    }
    // al cambiar entre fino y normal, reanclar para que el valor no salte
    if (fine != lastFine) { dragStartNorm = norm(); dragStartY = (float) e.position.y; lastFine = fine; return; }
    setNorm (dragStartNorm + delta);
}

void Knob::mouseUp (const juce::MouseEvent&)
{
    if (! dragging) return;
    if (draggingOther) shiftTarget()->param().endChangeGesture();
    else att->endGesture();
    dragging = draggingOther = false;
    repaint();
}

void Knob::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const float step = (e.mods.isShiftDown() ? 0.002f : 0.02f) * (w.deltaY > 0 ? 1.0f : -1.0f) * (w.isReversed ? -1.0f : 1.0f);
    att->beginGesture();
    setNorm (norm() + step);
    att->endGesture();
}

void Knob::mouseDoubleClick (const juce::MouseEvent&) { showEditor(); }

void Knob::showEditor()
{
    editor = std::make_unique<juce::TextEditor>();
    editor->setFont (Fonts::mono (13.5f));
    editor->setJustification (juce::Justification::centred);
    editor->setText (p->getCurrentValueAsText().upToFirstOccurrenceOf (" ", false, false), false);
    editor->setBounds (juce::Rectangle<int> (90, 22).withCentre ({ getWidth() / 2, (int) dial.getBottom() + 11 }));
    editor->setIndents (4, 3);
    addAndMakeVisible (*editor);
    editor->selectAll();
    editor->grabKeyboardFocus();
    auto finish = [this] (bool commit) {
        if (editor == nullptr) return;
        if (commit)
        {
            const auto t = editor->getText();
            const float v = p->getValueForText (t);
            setReal (p->convertFrom0to1 (v));
        }
        juce::MessageManager::callAsync ([sp = juce::Component::SafePointer<Knob> (this)] { if (sp) { sp->editor.reset(); sp->repaint(); } });
    };
    editor->onReturnKey = [finish] { finish (true); };
    editor->onEscapeKey = [finish] { finish (false); };
    editor->onFocusLost = [finish] { finish (true); };
}

// =====================================================================================
Fader::Fader (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, const juce::String& n)
    : ParamControl (s, paramId), name (n)
{
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void Fader::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();
    auto top = r.removeFromTop (18);
    auto bottom = r.removeFromBottom (18);
    rail = r.withSizeKeepingCentre (6, r.getHeight() - 12);
    g.setFont (Fonts::mono (12.0f));
    g.setColour (col::txt);
    g.drawText (params::formatFader (current), top, juce::Justification::centred);
    g.setFont (Fonts::sans (10.5f, true));
    g.setColour (dragging ? col::txt : col::txt2);
    g.drawText (name, bottom, juce::Justification::centred);

    g.setColour (col::pista);
    g.fillRoundedRectangle (rail.toFloat(), 3.0f);
    const float n = norm();
    const float y = (float) rail.getBottom() - n * (float) rail.getHeight();
    g.setColour (col::menta55());
    g.fillRoundedRectangle (rail.toFloat().withTop (y), 3.0f);
    // marca de 0 dB
    const float y0 = (float) rail.getBottom() - p->convertTo0to1 (0.0f) * (float) rail.getHeight();
    g.setColour (col::txt2);
    g.drawHorizontalLine ((int) y0, (float) rail.getX() - 6.0f, (float) rail.getX() - 2.0f);
    g.setColour (col::txt);
    g.fillRoundedRectangle (juce::Rectangle<float> (24.0f, 10.0f).withCentre ({ (float) rail.getCentreX(), y }), 3.0f);
}

void Fader::setFromY (float y, bool fine)
{
    juce::ignoreUnused (fine);
    const float n = 1.0f - (y - (float) rail.getY()) / (float) rail.getHeight();
    setNorm (n);
    if (onUserChange) onUserChange (current);
}

void Fader::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isCommandDown() || e.mods.isCtrlDown())
    {
        setReal (p->convertFrom0to1 (p->getDefaultValue()));
        if (onUserChange) onUserChange (current);
        return;
    }
    dragging = true;
    att->beginGesture();
    startNorm = norm();
    startY = (float) e.position.y;
    repaint();
}

void Fader::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging) return;
    const float scale = e.mods.isShiftDown() ? 0.1f : 1.0f;
    setNorm (startNorm + (startY - (float) e.position.y) / (float) std::max (1, rail.getHeight()) * scale);
    if (onUserChange) onUserChange (current);
}

void Fader::mouseUp (const juce::MouseEvent&)
{
    if (! dragging) return;
    dragging = false;
    att->endGesture();
    repaint();
}

void Fader::mouseDoubleClick (const juce::MouseEvent&)
{
    setReal (0.0f);
    if (onUserChange) onUserChange (current);
}

void Fader::setFromLink (float db) { setReal (db); }

// =====================================================================================
Switch::Switch (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, juce::String on, juce::String off)
    : ParamControl (s, paramId), onText (std::move (on)), offText (std::move (off)) {}

void Switch::paint (juce::Graphics& g)
{
    const bool on = current > 0.5f;
    auto r = getLocalBounds().toFloat();
    auto pista = r.removeFromLeft (30.0f).withSizeKeepingCentre (30.0f, 16.0f);
    g.setColour (on ? col::menta55() : col::lin);
    g.fillRoundedRectangle (pista, 8.0f);
    g.setColour (on ? col::txt : col::txt2);
    g.fillEllipse (juce::Rectangle<float> (12, 12).withCentre ({ on ? pista.getRight() - 8.0f : pista.getX() + 8.0f, pista.getCentreY() }));
    g.setFont (Fonts::sans (12.5f));
    g.setColour (col::txt);
    g.drawText (on ? onText : offText, r.withTrimmedLeft (8.0f), juce::Justification::centredLeft);
}

void Switch::mouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasClicked()) setReal (current > 0.5f ? 0.0f : 1.0f);
}

// =====================================================================================
Segmented::Segmented (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, juce::StringArray l)
    : ParamControl (s, paramId), labels (std::move (l)) {}

void Segmented::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (col::lin);
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
    const int n = labels.size();
    const int sel = juce::roundToInt (current);
    const float w = r.getWidth() / (float) n;
    for (int i = 0; i < n; ++i)
    {
        auto cell = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight());
        if (i == sel)
        {
            g.setColour (col::menta12());
            g.fillRoundedRectangle (cell.reduced (1.0f), 5.0f);
        }
        if (i > 0)
        {
            g.setColour (col::lin);
            g.drawVerticalLine ((int) cell.getX(), cell.getY(), cell.getBottom());
        }
        g.setColour (i == sel ? col::menta : col::txt2);
        g.setFont (Fonts::sans (12.0f));
        g.drawText (labels[i], cell, juce::Justification::centred);
    }
}

void Segmented::mouseUp (const juce::MouseEvent& e)
{
    if (! e.mouseWasClicked()) return;
    const int i = juce::jlimit (0, labels.size() - 1, (int) (e.position.x / (float) getWidth() * (float) labels.size()));
    setReal ((float) i);
}

// =====================================================================================
ModeSelector::ModeSelector (juce::AudioProcessorValueTreeState& s) : ParamControl (s, params::id::modo) {}

juce::Rectangle<float> ModeSelector::cell (int i) const
{
    const float gap = 6.0f, w = ((float) getWidth() - gap * 3) / 4.0f;
    return { (w + gap) * (float) i, 0.0f, w, (float) getHeight() };
}

void ModeSelector::setDensity (float db)
{
    if (db != density) { density = db; repaint(); }
}

void ModeSelector::paint (juce::Graphics& g)
{
    static const char* names[] = { "Suave", "Firme", "Duro", "Apagado" };
    const int sel = juce::roundToInt (current);
    for (int i = 0; i < 4; ++i)
    {
        auto c = cell (i).reduced (0.5f);
        const bool on = i == sel;
        if (on) { g.setColour (col::menta12()); g.fillRoundedRectangle (c, 8.0f); }
        g.setColour (on ? col::menta : (i == hover ? col::txt2 : col::lin));
        g.drawRoundedRectangle (c, 8.0f, 1.0f);
        auto plot = c.withTrimmedBottom (24.0f).reduced (14.0f, 10.0f);
        g.setColour (col::lin);
        const float yTop = plot.getY() + plot.getHeight() * 0.18f, yBot = plot.getBottom() - plot.getHeight() * 0.18f;
        g.drawHorizontalLine ((int) yTop, plot.getX(), plot.getRight());
        g.drawHorizontalLine ((int) yBot, plot.getX(), plot.getRight());
        juce::Path path;
        for (int k = 0; k <= 48; ++k)
        {
            const double u = -1.6 + 3.2 * k / 48.0;
            const double y = juce::jlimit (-1.6, 1.6, curves::shaper (u, density, (ClipMode) i));
            const float px = plot.getX() + plot.getWidth() * (float) k / 48.0f;
            const float py = plot.getCentreY() - (float) y / 1.6f * (plot.getHeight() * 0.5f);
            if (k == 0) path.startNewSubPath (px, py); else path.lineTo (px, py);
        }
        g.setColour (on ? col::menta : col::txt2);
        g.strokePath (path, juce::PathStrokeType (1.8f));
        g.setColour (on ? col::txt : col::txt2);
        g.setFont (Fonts::sans (12.0f));
        g.drawText (es (names[i]), c.withTop (c.getBottom() - 26.0f), juce::Justification::centred);
    }
}

void ModeSelector::mouseUp (const juce::MouseEvent& e)
{
    if (! e.mouseWasClicked()) return;
    for (int i = 0; i < 4; ++i)
        if (cell (i).contains (e.position)) setReal ((float) i);
}

void ModeSelector::mouseMove (const juce::MouseEvent& e)
{
    int h = -1;
    for (int i = 0; i < 4; ++i) if (cell (i).contains (e.position)) h = i;
    if (h != hover) { hover = h; repaint(); }
}

void ModeSelector::mouseExit (const juce::MouseEvent&) { hover = -1; repaint(); }

juce::String ModeSelector::getTooltip()
{
    static const char* tips[] = {
        "Suave: rodilla amplia y leve asimetría; agrega armónicos pares.",
        "Firme: rodilla estrecha y simétrica; armónicos impares.",
        "Duro: lineal hasta el techo y recorte duro.",
        "Apagado: sin recorte; solo Densidad y Suavizado, máxima transparencia." };
    return hover >= 0 ? es (tips[hover]) : es ("Modo de recorte");
}
} // namespace clip5::ui
