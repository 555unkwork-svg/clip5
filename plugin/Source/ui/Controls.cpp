#include "Controls.h"
#include "../Params.h"
#include "clip5/Curves.h"

namespace clip5::ui
{
using params::es;

void drawValueBox (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text, bool focus)
{
    const auto& p = pal();
    g.setColour (p.proc);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (p.txt);
    g.setFont (Fonts::mono (12.0f));
    g.drawText (text, r, juce::Justification::centred);
    if (focus) drawFocusRing (g, r, 5.0f);
}

// ===================================================================================== base
ParamControl::ParamControl (juce::AudioProcessorValueTreeState& s, const juce::String& paramId)
{
    p = s.getParameter (paramId);
    jassert (p != nullptr);
    att = std::make_unique<juce::ParameterAttachment> (*p, [this] (float v) { current = v; valueChanged(); }, s.undoManager);
    att->sendInitialUpdate();
    setWantsKeyboardFocus (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    format = [this] (float v) { return p->getText (p->convertTo0to1 (v), 32); };
}

void ParamControl::focusGained (FocusChangeType cause)
{
    keyboardFocus = cause == focusChangedByTabKey;
    repaint();
}

void ParamControl::focusLost (FocusChangeType)
{
    keyboardFocus = false;
    repaint();
}

void ParamControl::setNorm (float n) { att->setValueAsPartOfGesture (p->convertFrom0to1 (juce::jlimit (0.0f, 1.0f, n))); }
void ParamControl::setReal (float v) { att->setValueAsCompleteGesture (p->getNormalisableRange().snapToLegalValue (v)); }

void ParamControl::stepNorm (float delta)
{
    att->beginGesture();
    setNorm (norm() + delta);
    att->endGesture();
}

juce::String ParamControl::rangeText() const
{
    const auto lo = p->getText (0.0f, 32), hi = p->getText (1.0f, 32);
    return es ("rango ") + lo + es (" a ") + hi;
}

void ParamControl::showEditor (juce::Rectangle<int> area)
{
    editor = std::make_unique<juce::TextEditor>();
    editor->setFont (Fonts::mono (13.0f));
    editor->setJustification (juce::Justification::centred);
    editor->setIndents (4, 3);
    auto text = p->getCurrentValueAsText();
    text = text.upToFirstOccurrenceOf (" ", false, false);
    editor->setText (text, false);
    editor->setBounds (area);
    addAndMakeVisible (*editor);

    hint = std::make_unique<juce::Label>();
    hint->setFont (Fonts::sans (10.5f));
    hint->setText (rangeText() + es (" · Enter aplica · Esc cancela"), juce::dontSendNotification);
    hint->setColour (juce::Label::textColourId, juce::Colours::white);
    hint->setColour (juce::Label::backgroundColourId, juce::Colour (0xe0151e35));
    hint->setJustificationType (juce::Justification::centred);
    const int hw = std::max (area.getWidth() + 40, 190);
    auto hr = juce::Rectangle<int> (hw, 18).withCentre ({ area.getCentreX(), area.getY() - 12 });
    hr.setX (juce::jlimit (0, std::max (0, getWidth() - hw), hr.getX()));
    hint->setBounds (hr);
    addAndMakeVisible (*hint);

    editor->selectAll();
    editor->grabKeyboardFocus();
    editor->onReturnKey = [this] { closeEditor (true, false); };
    editor->onEscapeKey = [this] { closeEditor (false, false); };
    editor->onFocusLost = [this] { closeEditor (true, true); };
}

void ParamControl::closeEditor (bool commit, bool fromFocusLost)
{
    if (editor == nullptr) return;
    if (commit)
    {
        const auto t = editor->getText().trim();
        const bool parseable = t.containsAnyOf ("0123456789") || t.contains (es ("∞")) || t.containsIgnoreCase ("inf");
        if (! parseable)
        {
            // valor inválido: se mantiene el último aplicado y se avisa junto al campo
            hint->setText (es ("Valor no válido · ") + rangeText(), juce::dontSendNotification);
            hint->setColour (juce::Label::backgroundColourId, pal().over.darker (0.4f));
            if (! fromFocusLost) { editor->selectAll(); return; } // sigue abierto para corregir
        }
        else
        {
            const float v = p->getValueForText (t.replace (es ("−"), "-"));
            att->setValueAsCompleteGesture (p->convertFrom0to1 (v));
        }
    }
    juce::MessageManager::callAsync ([sp = juce::Component::SafePointer<ParamControl> (this)] {
        if (sp == nullptr) return;
        sp->editor.reset();
        sp->hint.reset();
        sp->grabKeyboardFocus();
        sp->repaint();
    });
}

// ===================================================================================== perilla
Knob::Knob (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, juce::String n, bool bip,
            std::function<juce::String (float)> fmt, float ns)
    : ParamControl (s, paramId), name (std::move (n)), bipolar (bip), nameSize (ns)
{
    format = std::move (fmt);
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    if (name.isNotEmpty()) r.removeFromTop ((int) nameSize + 8);
    valueBox = r.removeFromBottom (22).withSizeKeepingCentre (std::min (valueBoxWidth, getWidth()), 22);
    r.removeFromBottom (6);
    const float size = (float) std::min (r.getWidth(), r.getHeight());
    dial = juce::Rectangle<float> (size, size).withCentre (r.toFloat().getCentre());
}

void Knob::paint (juce::Graphics& g)
{
    const auto& c = pal();
    if (name.isNotEmpty())
    {
        g.setColour (c.txt);
        g.setFont (Fonts::sans (nameSize, true));
        g.drawText (name, getLocalBounds().removeFromTop ((int) nameSize + 4), juce::Justification::centred);
    }
    const float start = -juce::MathConstants<float>::pi * 0.75f, end = juce::MathConstants<float>::pi * 0.75f;
    const auto ctr = dial.getCentre();
    const float R = dial.getWidth() * 0.5f - 3.0f;
    const float sw = std::max (2.5f, dial.getWidth() * 0.055f);
    juce::Path track;
    track.addCentredArc (ctr.x, ctr.y, R, R, 0, start, end, true);
    g.setColour (c.txt2.withAlpha (0.28f));
    g.strokePath (track, juce::PathStrokeType (sw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const float aN = start + (end - start) * norm();
    const float aC = bipolar ? start + (end - start) * p->convertTo0to1 (0.0f) : start;
    if (std::abs (aN - aC) > 0.001f)
    {
        juce::Path arc;
        arc.addCentredArc (ctr.x, ctr.y, R, R, 0, std::min (aN, aC), std::max (aN, aC), true);
        g.setColour (c.sel);
        g.strokePath (arc, juce::PathStrokeType (sw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    const float body = R - sw * 1.7f;
    auto bodyR = juce::Rectangle<float> (body * 2, body * 2).withCentre (ctr);
    g.setColour (c.perilla);
    g.fillEllipse (bodyR);
    juce::ColourGradient shade (juce::Colours::white.withAlpha (0.16f), bodyR.getX() + body * 0.6f, bodyR.getY() + body * 0.5f,
                                juce::Colours::black.withAlpha (0.20f), bodyR.getRight(), bodyR.getBottom(), true);
    g.setGradientFill (shade);
    g.fillEllipse (bodyR);
    g.setColour (juce::Colours::white);
    g.drawLine ({ ctr.getPointOnCircumference (body * 0.30f, aN), ctr.getPointOnCircumference (body * 0.86f, aN) },
                std::max (2.0f, dial.getWidth() * 0.04f));
    if (keyboardFocus) drawFocusRing (g, dial.reduced (1.0f), dial.getWidth() * 0.5f);
    if (! isEditing()) drawValueBox (g, valueBox.toFloat(), format (current));
}

void Knob::mouseDown (const juce::MouseEvent& e)
{
    if (isEditing()) return;
    if (valueBox.contains (e.getPosition())) { showEditor (valueBox); return; }
    if (e.mods.isCommandDown() || e.mods.isCtrlDown()) { setReal (p->convertFrom0to1 (p->getDefaultValue())); return; }
    dragging = true;
    draggingOther = e.mods.isShiftDown() && shiftTarget && shiftTarget() != nullptr;
    dragStartY = (float) e.position.y;
    lastFine = e.mods.isShiftDown() && ! draggingOther;
    if (draggingOther) { auto* o = shiftTarget(); dragStartNorm = o->param().getValue(); o->param().beginChangeGesture(); }
    else { dragStartNorm = norm(); att->beginGesture(); }
    repaint();
}

void Knob::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging) return;
    const bool fine = e.mods.isShiftDown() && ! draggingOther;
    if (fine != lastFine) { dragStartNorm = norm(); dragStartY = (float) e.position.y; lastFine = fine; return; }
    const float delta = (dragStartY - (float) e.position.y) / 220.0f * (fine ? 0.1f : 1.0f);
    if (draggingOther) { shiftTarget()->param().setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, dragStartNorm + delta)); return; }
    setNorm (dragStartNorm + delta);
}

void Knob::mouseUp (const juce::MouseEvent&)
{
    if (! dragging) return;
    if (draggingOther) shiftTarget()->param().endChangeGesture(); else att->endGesture();
    dragging = draggingOther = false;
    repaint();
}

void Knob::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (! valueBox.contains (e.getPosition())) showEditor (valueBox);
}

void Knob::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    stepNorm ((e.mods.isShiftDown() ? 0.002f : 0.02f) * (w.deltaY > 0 ? 1.0f : -1.0f) * (w.isReversed ? -1.0f : 1.0f));
}

bool Knob::keyPressed (const juce::KeyPress& k)
{
    const float step = k.getModifiers().isShiftDown() ? 0.002f : 0.01f;
    const int code = k.getKeyCode();
    if (code == juce::KeyPress::upKey || code == juce::KeyPress::rightKey) { stepNorm (step); return true; }
    if (code == juce::KeyPress::downKey || code == juce::KeyPress::leftKey) { stepNorm (-step); return true; }
    if (code == juce::KeyPress::pageUpKey) { stepNorm (0.1f); return true; }
    if (code == juce::KeyPress::pageDownKey) { stepNorm (-0.1f); return true; }
    if (code == juce::KeyPress::returnKey) { showEditor (valueBox); return true; }
    if (code == juce::KeyPress::deleteKey || code == juce::KeyPress::backspaceKey) { setReal (p->convertFrom0to1 (p->getDefaultValue())); return true; }
    return false;
}

// ===================================================================================== fader horizontal
HFader::HFader (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, juce::String n)
    : ParamControl (s, paramId), name (std::move (n))
{
    format = [] (float v) { return params::formatFader (v); };
    setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
}

void HFader::resized()
{
    auto r = getLocalBounds();
    auto top = r.removeFromTop (24);
    valueBox = top.removeFromRight (56);
    track = r.withSizeKeepingCentre (r.getWidth() - 14, 14);
}

void HFader::paint (juce::Graphics& g)
{
    const auto& c = pal();
    g.setColour (c.txt);
    g.setFont (Fonts::sans (12.5f));
    g.drawText (name, getLocalBounds().removeFromTop (24), juce::Justification::centredLeft);
    if (! isEditing()) drawValueBox (g, valueBox.toFloat(), format (current));
    const auto t = track.toFloat();
    const float y = t.getCentreY();
    g.setColour (c.txt2.withAlpha (0.25f));
    g.fillRoundedRectangle (t.getX(), y - 2.0f, t.getWidth(), 4.0f, 2.0f);
    const float x = t.getX() + norm() * t.getWidth();
    g.setColour (c.sel);
    g.fillRoundedRectangle (t.getX(), y - 2.0f, x - t.getX(), 4.0f, 2.0f);
    // marca de 0 dB
    const float x0 = t.getX() + p->convertTo0to1 (0.0f) * t.getWidth();
    g.setColour (c.txt2);
    g.fillRect (x0 - 0.5f, y - 7.0f, 1.0f, 5.0f);
    g.setColour (c.sel);
    g.fillEllipse (juce::Rectangle<float> (14, 14).withCentre ({ x, y }));
    if (keyboardFocus) drawFocusRing (g, juce::Rectangle<float> (14, 14).withCentre ({ x, y }), 7.0f);
}

void HFader::changed() { if (onUserChange) onUserChange (current); }

void HFader::mouseDown (const juce::MouseEvent& e)
{
    if (isEditing()) return;
    if (valueBox.contains (e.getPosition())) { showEditor (valueBox.expanded (8, 0)); return; }
    if (e.mods.isCommandDown() || e.mods.isCtrlDown()) { setReal (p->convertFrom0to1 (p->getDefaultValue())); changed(); return; }
    if (e.position.y < 24) return;
    dragging = true;
    att->beginGesture();
    // clic en el riel: salto a la posición
    const auto t = track.toFloat();
    setNorm ((e.position.x - t.getX()) / t.getWidth());
    startNorm = norm();
    startX = e.position.x;
    changed();
}

void HFader::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging) return;
    const float scale = e.mods.isShiftDown() ? 0.1f : 1.0f;
    setNorm (startNorm + (e.position.x - startX) / (float) track.getWidth() * scale);
    changed();
}

void HFader::mouseUp (const juce::MouseEvent&)
{
    if (! dragging) return;
    dragging = false;
    att->endGesture();
}

void HFader::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (valueBox.contains (e.getPosition())) return;
    setReal (0.0f);
    changed();
}

bool HFader::keyPressed (const juce::KeyPress& k)
{
    const float step = k.getModifiers().isShiftDown() ? 0.002f : 0.01f;
    const int code = k.getKeyCode();
    if (code == juce::KeyPress::rightKey || code == juce::KeyPress::upKey) { stepNorm (step); changed(); return true; }
    if (code == juce::KeyPress::leftKey || code == juce::KeyPress::downKey) { stepNorm (-step); changed(); return true; }
    if (code == juce::KeyPress::returnKey) { showEditor (valueBox.expanded (8, 0)); return true; }
    return false;
}

void HFader::setFromLink (float db) { setReal (db); }

// ===================================================================================== píldora
PillToggle::PillToggle (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, juce::String t)
    : ParamControl (s, paramId), text (std::move (t)) {}

void PillToggle::paint (juce::Graphics& g)
{
    const auto& c = pal();
    const bool on = current > 0.5f;
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (on ? c.sel : c.proc);
    g.fillRoundedRectangle (r, 7.0f);
    if (! on) { g.setColour (c.txt2.withAlpha (0.4f)); g.drawRoundedRectangle (r, 7.0f, 1.0f); }
    g.setColour (on ? c.selTxt : c.txt2);
    g.setFont (Fonts::sans (12.5f, true));
    g.drawText (text, getLocalBounds(), juce::Justification::centred);
    if (keyboardFocus) drawFocusRing (g, r, 7.0f);
}

void PillToggle::mouseUp (const juce::MouseEvent& e) { if (e.mouseWasClicked()) setReal (current > 0.5f ? 0.0f : 1.0f); }

bool PillToggle::keyPressed (const juce::KeyPress& k)
{
    if (k.getKeyCode() == juce::KeyPress::returnKey || k.getKeyCode() == juce::KeyPress::spaceKey) { setReal (current > 0.5f ? 0.0f : 1.0f); return true; }
    return false;
}

// ===================================================================================== interruptor
Switch::Switch (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, juce::String on, juce::String off)
    : ParamControl (s, paramId), onText (std::move (on)), offText (std::move (off)) {}

void Switch::paint (juce::Graphics& g)
{
    const auto& c = pal();
    const bool on = current > 0.5f;
    auto r = getLocalBounds().toFloat();
    auto pista = r.removeFromLeft (34.0f).withSizeKeepingCentre (34.0f, 18.0f);
    g.setColour (on ? c.sel : c.txt2.withAlpha (0.35f));
    g.fillRoundedRectangle (pista, 9.0f);
    g.setColour (on ? c.selTxt : c.sup);
    g.fillEllipse (juce::Rectangle<float> (13, 13).withCentre ({ on ? pista.getRight() - 9.0f : pista.getX() + 9.0f, pista.getCentreY() }));
    if (keyboardFocus) drawFocusRing (g, pista, 9.0f);
    g.setFont (Fonts::sans (12.5f));
    g.setColour (c.txt);
    g.drawText (on ? onText : offText, r.withTrimmedLeft (8.0f), juce::Justification::centredLeft);
}

void Switch::mouseUp (const juce::MouseEvent& e) { if (e.mouseWasClicked()) setReal (current > 0.5f ? 0.0f : 1.0f); }

bool Switch::keyPressed (const juce::KeyPress& k)
{
    if (k.getKeyCode() == juce::KeyPress::returnKey || k.getKeyCode() == juce::KeyPress::spaceKey) { setReal (current > 0.5f ? 0.0f : 1.0f); return true; }
    return false;
}

// ===================================================================================== segmentado
Segmented::Segmented (juce::AudioProcessorValueTreeState& s, const juce::String& paramId, juce::StringArray l, float gp)
    : ParamControl (s, paramId), labels (std::move (l)), gap (gp) {}

juce::Rectangle<float> Segmented::cell (int i) const
{
    const int n = labels.size();
    const float w = ((float) getWidth() - gap * (float) (n - 1)) / (float) n;
    return { (w + gap) * (float) i, 0.0f, w, (float) getHeight() };
}

void Segmented::paint (juce::Graphics& g)
{
    const auto& c = pal();
    const int sel = juce::roundToInt (current);
    for (int i = 0; i < labels.size(); ++i)
    {
        auto r = cell (i).reduced (0.5f);
        const bool on = i == sel;
        g.setColour (on ? c.sel : c.proc);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (on ? c.selTxt : c.txt);
        g.setFont (Fonts::sans (fontSize, on));
        g.drawText (labels[i], r, juce::Justification::centred);
        if (on && keyboardFocus) drawFocusRing (g, r, 6.0f);
    }
}

void Segmented::mouseUp (const juce::MouseEvent& e)
{
    if (! e.mouseWasClicked()) return;
    for (int i = 0; i < labels.size(); ++i) if (cell (i).contains (e.position)) setReal ((float) i);
}

bool Segmented::keyPressed (const juce::KeyPress& k)
{
    const int sel = juce::roundToInt (current), code = k.getKeyCode();
    if (code == juce::KeyPress::rightKey || code == juce::KeyPress::downKey) { setReal ((float) std::min (labels.size() - 1, sel + 1)); return true; }
    if (code == juce::KeyPress::leftKey || code == juce::KeyPress::upKey) { setReal ((float) std::max (0, sel - 1)); return true; }
    return false;
}

// ===================================================================================== curva de recorte
CurveSelector::CurveSelector (juce::AudioProcessorValueTreeState& s) : ParamControl (s, params::id::modo) {}

juce::Rectangle<float> CurveSelector::cell (int i) const
{
    const float gap = 5.0f, w = std::min ((float) getHeight(), ((float) getWidth() - gap * 3) / 4.0f);
    return { (w + gap) * (float) i, 0.0f, w, w };
}

void CurveSelector::paint (juce::Graphics& g)
{
    const auto& c = pal();
    const int sel = juce::roundToInt (current);
    for (int i = 0; i < 4; ++i)
    {
        auto r = cell (i).reduced (0.5f);
        const bool on = i == sel;
        g.setColour (on ? c.sel : c.sup);
        g.fillRoundedRectangle (r, 7.0f);
        g.setColour (on ? c.sel : c.txt2.withAlpha (i == hover ? 0.7f : 0.3f));
        g.drawRoundedRectangle (r, 7.0f, 1.0f);
        const auto ink = on ? c.selTxt : c.txt;
        auto plot = r.reduced (r.getWidth() * 0.16f);
        const float yC = plot.getBottom() - plot.getHeight() / 1.5f;
        g.setColour (ink.withAlpha (0.35f));
        const float dashes[] = { 2.0f, 2.0f };
        g.drawDashedLine ({ plot.getX(), yC, plot.getRight(), yC }, dashes, 2, 1.0f);
        juce::Path path;
        for (int k = 0; k <= 40; ++k)
        {
            const double u = 1.5 * k / 40.0;
            const double y = std::min (1.5, curves::clip (u, (ClipMode) i));
            const float px = plot.getX() + plot.getWidth() * (float) (u / 1.5);
            const float py = plot.getBottom() - plot.getHeight() * (float) (y / 1.5);
            if (k == 0) path.startNewSubPath (px, py); else path.lineTo (px, py);
        }
        g.setColour (ink);
        g.strokePath (path, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        if (on && keyboardFocus) drawFocusRing (g, r, 7.0f);
    }
}

void CurveSelector::mouseUp (const juce::MouseEvent& e)
{
    if (! e.mouseWasClicked()) return;
    for (int i = 0; i < 4; ++i) if (cell (i).contains (e.position)) setReal ((float) i);
}

void CurveSelector::mouseMove (const juce::MouseEvent& e)
{
    int h = -1;
    for (int i = 0; i < 4; ++i) if (cell (i).contains (e.position)) h = i;
    if (h != hover) { hover = h; repaint(); }
}

void CurveSelector::mouseExit (const juce::MouseEvent&) { hover = -1; repaint(); }

bool CurveSelector::keyPressed (const juce::KeyPress& k)
{
    const int sel = juce::roundToInt (current), code = k.getKeyCode();
    if (code == juce::KeyPress::rightKey || code == juce::KeyPress::downKey) { setReal ((float) std::min (3, sel + 1)); return true; }
    if (code == juce::KeyPress::leftKey || code == juce::KeyPress::upKey) { setReal ((float) std::max (0, sel - 1)); return true; }
    return false;
}

juce::String CurveSelector::getTooltip()
{
    static const char* tips[] = {
        "Suave: rodilla amplia con leve asimetría; redondea los picos de forma gradual.",
        "Medio: rodilla simétrica más estrecha.",
        "Duro: lineal hasta el Techo y recorte abrupto.",
        "Sin recorte: omite la curva; Densidad, Smooth y True Peak pueden actuar." };
    const int i = hover >= 0 ? hover : juce::roundToInt (current);
    return es ("Curva de recorte · ") + es (tips[juce::jlimit (0, 3, i)]);
}

// ===================================================================================== botón
FlatButton::FlatButton (const juce::String& text, const juce::String& tip, bool onGraph)
{
    setButtonText (text);
    setTooltip (tip);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus (true);
    getProperties().set ("enGrafico", onGraph);
}

void FlatButton::focusGained (FocusChangeType cause)
{
    getProperties().set ("focoTeclado", cause == focusChangedByTabKey);
    repaint();
}

void FlatButton::focusLost (FocusChangeType)
{
    getProperties().set ("focoTeclado", false);
    repaint();
}
} // namespace clip5::ui
