#include "Theme.h"
#include "Clip5Assets.h"

namespace clip5::ui
{
namespace
{
    juce::Typeface::Ptr load (const char* data, int size)
    {
        return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
    }
    juce::Typeface::Ptr interRegular()
    {
        static auto t = load (Clip5Assets::InterRegular_otf, Clip5Assets::InterRegular_otfSize);
        return t;
    }
    juce::Typeface::Ptr interSemi()
    {
        static auto t = load (Clip5Assets::InterSemiBold_otf, Clip5Assets::InterSemiBold_otfSize);
        return t;
    }
    juce::Typeface::Ptr monoRegular()
    {
        static auto t = load (Clip5Assets::JetBrainsMonoRegular_ttf, Clip5Assets::JetBrainsMonoRegular_ttfSize);
        return t;
    }
}

juce::Font Fonts::sans (float h, bool semibold)
{
    return juce::Font (juce::FontOptions (semibold ? interSemi() : interRegular()).withHeight (h));
}

juce::Font Fonts::mono (float h)
{
    return juce::Font (juce::FontOptions (monoRegular()).withHeight (h));
}

LookAndFeel::LookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, col::fondo);
    setColour (juce::PopupMenu::backgroundColourId, col::sup);
    setColour (juce::PopupMenu::textColourId, col::txt);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::menta12());
    setColour (juce::PopupMenu::highlightedTextColourId, col::txt);
    setColour (juce::ComboBox::backgroundColourId, col::sup);
    setColour (juce::ComboBox::textColourId, col::txt);
    setColour (juce::ComboBox::outlineColourId, col::lin);
    setColour (juce::ComboBox::arrowColourId, col::txt2);
    setColour (juce::TextEditor::backgroundColourId, col::sup2);
    setColour (juce::TextEditor::textColourId, col::txt);
    setColour (juce::TextEditor::highlightColourId, col::menta.withAlpha (0.3f));
    setColour (juce::TextEditor::focusedOutlineColourId, col::menta);
    setColour (juce::CaretComponent::caretColourId, col::menta);
    setColour (juce::Label::textColourId, col::txt);
    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff0b0c0d));
    setColour (juce::TooltipWindow::textColourId, col::txt);
    setColour (juce::TooltipWindow::outlineColourId, col::lin);
    setColour (juce::AlertWindow::backgroundColourId, col::sup);
    setColour (juce::AlertWindow::textColourId, col::txt);
    setColour (juce::TextButton::buttonColourId, col::sup);
    setColour (juce::TextButton::textColourOffId, col::txt);
    setDefaultSansSerifTypeface (interRegular());
}

juce::Typeface::Ptr LookAndFeel::getTypefaceForFont (const juce::Font& f)
{
    if (f.getTypefaceName() == juce::Font::getDefaultSansSerifFontName()) return interRegular();
    return LookAndFeel_V4::getTypefaceForFont (f);
}

void LookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (col::sup);
    g.setColour (col::lin);
    g.drawRect (0, 0, w, h);
}

void LookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator, bool isActive,
                                     bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text,
                                     const juce::String& shortcutKeyText, const juce::Drawable*, const juce::Colour*)
{
    if (isSeparator)
    {
        g.setColour (col::lin);
        g.fillRect (area.reduced (8, 0).withHeight (1).withY (area.getCentreY()));
        return;
    }
    auto r = area.reduced (2, 0);
    if (isHighlighted && isActive)
    {
        g.setColour (col::menta12());
        g.fillRoundedRectangle (r.toFloat(), 4.0f);
    }
    g.setColour (! isActive ? col::txt2 : (isTicked ? col::menta : col::txt));
    g.setFont (Fonts::sans (14.0f));
    g.drawText (text, r.withTrimmedLeft (22), juce::Justification::centredLeft);
    if (isTicked)
    {
        g.setColour (col::menta);
        g.fillEllipse (juce::Rectangle<float> (6, 6).withCentre ({ (float) r.getX() + 11.0f, (float) r.getCentreY() }));
    }
    if (shortcutKeyText.isNotEmpty())
    {
        g.setColour (col::txt2);
        g.drawText (shortcutKeyText, r.withTrimmedRight (10), juce::Justification::centredRight);
    }
    if (hasSubMenu)
    {
        g.setColour (col::txt2);
        g.drawText (">", r.withTrimmedRight (10), juce::Justification::centredRight);
    }
}

void LookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (col::sup);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (box.isMouseOver (true) ? col::txt2 : col::lin);
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
    juce::Path p;
    const float cx = (float) w - 14.0f, cy = (float) h * 0.5f;
    p.startNewSubPath (cx - 4, cy - 2);
    p.lineTo (cx, cy + 2);
    p.lineTo (cx + 4, cy - 2);
    g.setColour (col::txt2);
    g.strokePath (p, juce::PathStrokeType (1.4f));
}

void LookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (8, 1, box.getWidth() - 30, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

void LookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int w, int h)
{
    g.fillAll (juce::Colour (0xff0b0c0d));
    g.setColour (col::lin);
    g.drawRect (0, 0, w, h);
    juce::AttributedString s;
    s.append (text, Fonts::sans (13.0f), col::txt);
    s.setWordWrap (juce::AttributedString::byWord);
    juce::TextLayout tl;
    tl.createLayout (s, (float) w - 16.0f);
    tl.draw (g, { 8.0f, 6.0f, (float) w - 16.0f, (float) h - 12.0f });
}

juce::Rectangle<int> LookAndFeel::getTooltipBounds (const juce::String& tipText, juce::Point<int> pos, juce::Rectangle<int> parent)
{
    juce::AttributedString s;
    s.append (tipText, Fonts::sans (13.0f), col::txt);
    s.setWordWrap (juce::AttributedString::byWord);
    juce::TextLayout tl;
    tl.createLayout (s, 260.0f);
    const int w = (int) std::ceil (tl.getWidth()) + 18, h = (int) std::ceil (tl.getHeight()) + 14;
    return juce::Rectangle<int> (pos.x, pos.y + 18, w, h).constrainedWithin (parent);
}

void LookAndFeel::fillTextEditorBackground (juce::Graphics& g, int w, int h, juce::TextEditor&)
{
    g.setColour (col::sup2);
    g.fillRoundedRectangle (0, 0, (float) w, (float) h, 5.0f);
}

void LookAndFeel::drawTextEditorOutline (juce::Graphics& g, int w, int h, juce::TextEditor&)
{
    g.setColour (col::menta);
    g.drawRoundedRectangle (0.5f, 0.5f, (float) w - 1, (float) h - 1, 5.0f, 1.0f);
}

void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    g.setColour (on ? col::menta12() : (down ? col::sup2 : juce::Colours::transparentBlack));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (on ? col::menta : (over ? col::txt2 : col::lin));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
}

void LookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (b.getToggleState() ? col::menta : (b.isEnabled() ? col::txt : col::txt2));
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}
} // namespace clip5::ui
