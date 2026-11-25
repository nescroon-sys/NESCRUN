#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace nescrun
{
namespace Colours_
{
    const juce::Colour bg       { 0xffe9e5df };
    const juce::Colour card     { 0xfff5f2ee };
    const juce::Colour cardEdge { 0xffffffff };
    const juce::Colour text     { 0xff2a2a30 };
    const juce::Colour sub      { 0xff8c8892 };
    const juce::Colour red      { 0xffe5483f };
    const juce::Colour purple   { 0xff7c5cff };
    const juce::Colour screen   { 0xff16161b };
    const juce::Colour track    { 0xffd9d4cd };
}

class NescrunLook : public juce::LookAndFeel_V4
{
public:
    NescrunLook()
    {
        setColour (juce::ComboBox::backgroundColourId, Colours_::card);
        setColour (juce::ComboBox::textColourId, Colours_::text);
        setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
        setColour (juce::ComboBox::arrowColourId, Colours_::sub);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xfffbfaf8));
        setColour (juce::PopupMenu::textColourId, Colours_::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, Colours_::red.withAlpha (0.15f));
        setColour (juce::PopupMenu::highlightedTextColourId, Colours_::text);
        setColour (juce::PopupMenu::headerTextColourId, Colours_::red);
        setColour (juce::Label::textColourId, Colours_::text);
        setColour (juce::TextButton::textColourOffId, Colours_::text);
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    }

    juce::Font font (float h, bool bold = false) const
    {
        return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
    }

    // Perilla neumorfica: sombra suave, cara blanca, arco de valor de color y punto indicador
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startA, float endA, juce::Slider& s) override
    {
        const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId);
        auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
        const float d = std::min (area.getWidth(), area.getHeight());
        auto c = area.withSizeKeepingCentre (d, d);
        const float r = d * 0.5f;
        const auto ctr = c.getCentre();

        // arco de fondo y de valor
        const float arcR = r - 2.f;
        juce::Path bgArc; bgArc.addCentredArc (ctr.x, ctr.y, arcR, arcR, 0.f, startA, endA, true);
        g.setColour (Colours_::track); g.strokePath (bgArc, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
        const float zeroPos = bipolar ? (float) s.valueToProportionOfLength (0.0) : 0.f;
        const float a0 = startA + zeroPos * (endA - startA), a1 = startA + pos * (endA - startA);
        juce::Path valArc; valArc.addCentredArc (ctr.x, ctr.y, arcR, arcR, 0.f, std::min (a0, a1), std::max (a0, a1), true);
        g.setColour (s.isEnabled() ? accent : Colours_::sub);
        g.strokePath (valArc, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // cara
        auto face = c.reduced (r * 0.2f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.18f), (int) (r * 0.35f), { 0, (int) (r * 0.12f) }).drawForPath (g, ellipse (face));
        juce::DropShadow (juce::Colours::white.withAlpha (0.9f), (int) (r * 0.25f), { 0, -(int) (r * 0.08f) }).drawForPath (g, ellipse (face));
        g.setGradientFill (juce::ColourGradient (juce::Colours::white, face.getX(), face.getY(),
                                                 juce::Colour (0xffe4dfd8), face.getRight(), face.getBottom(), false));
        g.fillEllipse (face);
        g.setColour (juce::Colours::white.withAlpha (0.8f)); g.drawEllipse (face.reduced (0.5f), 1.f);

        // indicador
        const float ang = a1 - juce::MathConstants<float>::halfPi;
        const float ir = face.getWidth() * 0.5f - 5.f;
        g.setColour (accent);
        g.fillEllipse (juce::Rectangle<float> (5.f, 5.f).withCentre ({ ctr.x + std::cos (ang) * ir, ctr.y + std::sin (ang) * ir }));

        // valor en el centro
        if (face.getWidth() > 34.f)
        {
            g.setColour (Colours_::text);
            g.setFont (font (std::max (9.f, face.getWidth() * 0.2f), true));
            g.drawFittedText (s.getTextFromValue (s.getValue()), face.reduced (6.f).toNearestInt(), juce::Justification::centred, 1);
        }
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos, float, float,
                           juce::Slider::SliderStyle style, juce::Slider& s) override
    {
        const auto accent = s.findColour (juce::Slider::trackColourId);
        const bool vert = style == juce::Slider::LinearVertical;
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
        const float t = 4.f;
        juce::Rectangle<float> track = vert ? b.withSizeKeepingCentre (t, b.getHeight() - 8.f) : b.withSizeKeepingCentre (b.getWidth() - 8.f, t);
        g.setColour (Colours_::track); g.fillRoundedRectangle (track, t * 0.5f);
        const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
        juce::Rectangle<float> fill;
        if (vert)
        {
            const float zeroY = bipolar ? (float) y + (float) h * (1.f - (float) s.valueToProportionOfLength (0.0)) : track.getBottom();
            fill = juce::Rectangle<float>::leftTopRightBottom (track.getX(), std::min (zeroY, sliderPos), track.getRight(), std::max (zeroY, sliderPos));
        }
        else
        {
            const float zeroX = bipolar ? (float) x + (float) w * (float) s.valueToProportionOfLength (0.0) : track.getX();
            fill = juce::Rectangle<float>::leftTopRightBottom (std::min (zeroX, sliderPos), track.getY(), std::max (zeroX, sliderPos), track.getBottom());
        }
        g.setColour (accent); g.fillRoundedRectangle (fill, t * 0.5f);
        const auto thumbC = vert ? juce::Point<float> (track.getCentreX(), sliderPos) : juce::Point<float> (sliderPos, track.getCentreY());
        auto thumb = juce::Rectangle<float> (16.f, 16.f).withCentre (thumbC);
        juce::DropShadow (juce::Colours::black.withAlpha (0.22f), 6, { 0, 2 }).drawForPath (g, ellipse (thumb));
        g.setColour (juce::Colours::white); g.fillEllipse (thumb);
        g.setColour (accent); g.fillEllipse (thumb.reduced (5.f));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (2.f);
        const bool on = b.getToggleState();
        const auto accent = b.findColour (juce::TextButton::buttonOnColourId);
        juce::Path p; p.addRoundedRectangle (r, r.getHeight() * 0.5f);
        if (on)
        {
            juce::DropShadow (accent.withAlpha (0.45f), 10, { 0, 2 }).drawForPath (g, p);
            g.setColour (accent); g.fillPath (p);
        }
        else
        {
            juce::DropShadow (juce::Colours::black.withAlpha (0.12f), 6, { 0, 2 }).drawForPath (g, p);
            g.setColour (down ? Colours_::track : (over ? juce::Colours::white : Colours_::card)); g.fillPath (p);
            g.setColour (juce::Colours::white); g.strokePath (p, juce::PathStrokeType (1.f));
        }
    }

    juce::Font getTextButtonFont (juce::TextButton&, int h) override { return font (std::min (12.f, (float) h * 0.5f), true); }

    // boton de encendido de cada modulo
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (2.f);
        const float d = std::min (r.getWidth(), r.getHeight());
        auto c = r.withSizeKeepingCentre (d, d);
        const bool on = b.getToggleState();
        const auto accent = b.findColour (juce::ToggleButton::tickColourId);
        g.setColour (on ? accent : Colours_::sub.withAlpha (over ? 0.9f : 0.6f));
        juce::Path arc; arc.addCentredArc (c.getCentreX(), c.getCentreY(), d * 0.36f, d * 0.36f, 0.f, 0.6f, juce::MathConstants<float>::twoPi - 0.6f, true);
        g.strokePath (arc, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.drawLine (c.getCentreX(), c.getY() + d * 0.08f, c.getCentreX(), c.getCentreY(), 1.8f);
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&) override
    {
        auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (2.f);
        juce::Path p; p.addRoundedRectangle (r, 8.f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.10f), 6, { 0, 2 }).drawForPath (g, p);
        g.setColour (Colours_::card); g.fillPath (p);
        g.setColour (juce::Colours::white); g.strokePath (p, juce::PathStrokeType (1.f));
        juce::Path arrow; const float ax = (float) w - 14.f, ay = (float) h * 0.5f;
        arrow.addTriangle (ax - 4.f, ay - 2.f, ax + 4.f, ay - 2.f, ax, ay + 3.f);
        g.setColour (Colours_::sub); g.fillPath (arrow);
    }
    juce::Font getComboBoxFont (juce::ComboBox& b) override { return font (std::min (13.f, (float) b.getHeight() * 0.5f), true); }
    void positionComboBoxText (juce::ComboBox& b, juce::Label& l) override
    {
        l.setBounds (8, 1, b.getWidth() - 26, b.getHeight() - 2);
        l.setFont (getComboBoxFont (b)); l.setJustificationType (juce::Justification::centred);
    }
    juce::Font getPopupMenuFont() override { return font (14.f); }

private:
    static juce::Path ellipse (juce::Rectangle<float> r) { juce::Path p; p.addEllipse (r); return p; }
};
} // namespace nescrun
