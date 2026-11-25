#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"

namespace nescrun
{
using APVTS = juce::AudioProcessorValueTreeState;

// Perilla con nombre
class Knob : public juce::Component
{
public:
    Knob (APVTS& st, const juce::String& id, const juce::String& label_, juce::Colour accent)
        : label (label_)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setColour (juce::Slider::rotarySliderFillColourId, accent);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        s.setVelocityBasedMode (false);
        s.setMouseDragSensitivity (220);
        s.setPopupDisplayEnabled (true, true, nullptr);
        addAndMakeVisible (s);
        att = std::make_unique<APVTS::SliderAttachment> (st, id, s);
        s.setDoubleClickReturnValue (true, (double) st.getParameter (id)->convertFrom0to1 (st.getParameter (id)->getDefaultValue()));
    }
    void resized() override { auto b = getLocalBounds(); b.removeFromBottom (16); s.setBounds (b); }
    void paint (juce::Graphics& g) override
    {
        g.setColour (Colours_::sub);
        g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
        g.drawFittedText (label.toUpperCase(), getLocalBounds().removeFromBottom (16), juce::Justification::centred, 1);
    }
    juce::Slider s;
private:
    juce::String label;
    std::unique_ptr<APVTS::SliderAttachment> att;
};

// Fader (vertical u horizontal) con nombre
class Fader : public juce::Component
{
public:
    Fader (APVTS& st, const juce::String& id, const juce::String& label_, juce::Colour accent, bool vertical)
        : label (label_), vert (vertical)
    {
        s.setSliderStyle (vertical ? juce::Slider::LinearVertical : juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setColour (juce::Slider::trackColourId, accent);
        s.setPopupDisplayEnabled (true, true, nullptr);
        addAndMakeVisible (s);
        att = std::make_unique<APVTS::SliderAttachment> (st, id, s);
        s.setDoubleClickReturnValue (true, (double) st.getParameter (id)->convertFrom0to1 (st.getParameter (id)->getDefaultValue()));
        s.onValueChange = [this] { repaint(); };
    }
    void resized() override
    {
        auto b = getLocalBounds();
        if (vert) { b.removeFromBottom (30); s.setBounds (b); }
        else { b.removeFromTop (16); s.setBounds (b); }
    }
    void paint (juce::Graphics& g) override
    {
        g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
        auto b = getLocalBounds();
        if (vert)
        {
            auto bot = b.removeFromBottom (30);
            g.setColour (Colours_::text); g.drawFittedText (s.getTextFromValue (s.getValue()), bot.removeFromTop (14), juce::Justification::centred, 1);
            g.setColour (Colours_::sub);  g.drawFittedText (label.toUpperCase(), bot, juce::Justification::centred, 1);
        }
        else
        {
            auto top = b.removeFromTop (16);
            g.setColour (Colours_::sub);  g.drawText (label.toUpperCase(), top, juce::Justification::centredLeft);
            g.setColour (Colours_::text); g.drawText (s.getTextFromValue (s.getValue()), top, juce::Justification::centredRight);
        }
    }
    juce::Slider s;
private:
    juce::String label; bool vert;
    std::unique_ptr<APVTS::SliderAttachment> att;
};

// Botones segmentados para parametros de opciones
class Segments : public juce::Component
{
public:
    Segments (APVTS& st, const juce::String& id, juce::StringArray labels, juce::Colour accent, int cols_ = 0)
        : cols (cols_ > 0 ? cols_ : labels.size())
    {
        auto* prm = st.getParameter (id);
        for (int i = 0; i < labels.size(); ++i)
        {
            auto* b = buttons.add (new juce::TextButton (labels[i].toUpperCase()));
            b->setClickingTogglesState (false);
            b->setColour (juce::TextButton::buttonOnColourId, accent);
            b->onClick = [this, i] { att->setValueAsCompleteGesture ((float) i); };
            addAndMakeVisible (b);
        }
        att = std::make_unique<juce::ParameterAttachment> (*prm, [this] (float v) { setIndex ((int) std::round (v)); });
        att->sendInitialUpdate();
    }
    void setIndex (int idx) { for (int i = 0; i < buttons.size(); ++i) buttons[i]->setToggleState (i == idx, juce::dontSendNotification); }
    void resized() override
    {
        const int rows = (buttons.size() + cols - 1) / cols;
        const int w = getWidth() / cols, h = getHeight() / rows;
        for (int i = 0; i < buttons.size(); ++i) buttons[i]->setBounds ((i % cols) * w, (i / cols) * h, w, h);
    }
private:
    int cols;
    juce::OwnedArray<juce::TextButton> buttons;
    std::unique_ptr<juce::ParameterAttachment> att;
};

// Pantalla del afinador
class TuneScreen : public juce::Component
{
public:
    explicit TuneScreen (NescrunProcessor& p) : proc (p) {}
    void paint (juce::Graphics& g) override;
private:
    NescrunProcessor& proc;
    float shownCents = 0.f;
};

// Pantalla de curva de EQ + reduccion de resonancias
class EqScreen : public juce::Component
{
public:
    explicit EqScreen (NescrunProcessor& p) : proc (p) {}
    void paint (juce::Graphics& g) override;
private:
    NescrunProcessor& proc;
};

// Medidor horizontal simple
class Meter : public juce::Component
{
public:
    Meter (juce::String l, juce::Colour c, bool gr) : label (std::move (l)), col (c), isGr (gr) {}
    void setValue (float db) { value = isGr ? std::min (value * 0.85f + db * 0.15f, db) : std::max (db, value - 1.5f); repaint(); }
    void paint (juce::Graphics& g) override;
private:
    juce::String label; juce::Colour col; bool isGr; float value = isGr ? 0.f : -60.f;
};

struct CardInfo { juce::Rectangle<int> r; juce::String title; juce::Colour accent; };

class Content : public juce::Component
{
public:
    std::vector<CardInfo> cards;
    void paint (juce::Graphics& g) override;
};
} // namespace nescrun

class NescrunEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NescrunEditor (NescrunProcessor&);
    ~NescrunEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int W = 1200, H = 720;

private:
    void timerCallback() override;
    void buildLayout();
    void refreshPresetBox();
    void savePreset();
    void loadPreset();
    juce::File presetDir() const;

    NescrunProcessor& proc;
    nescrun::NescrunLook look;
    nescrun::Content content;

    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, saveBtn { "SAVE" }, loadBtn { "LOAD" }, autoBtn { "AUTO MIX" };
    juce::Array<juce::File> userPresets;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::OwnedArray<juce::Component> owned;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ButtonAttachment> btnAtts;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAtts;

    nescrun::TuneScreen tuneScreen { proc };
    nescrun::EqScreen eqScreen { proc };
    nescrun::Meter inMeter { "IN", nescrun::Colours_::text, false }, outMeter { "OUT", nescrun::Colours_::text, false },
                   limMeter { "LIM", nescrun::Colours_::red, true }, compMeter { "GR", nescrun::Colours_::red, true },
                   deessMeter { "S", nescrun::Colours_::red, true }, resoMeter { "AI", nescrun::Colours_::purple, true };
    juce::Label footer;
    int lastProg = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NescrunEditor)
};
