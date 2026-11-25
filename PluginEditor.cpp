#include "PluginEditor.h"

using namespace nescrun;
namespace C = nescrun::Colours_;

static juce::Font F (float h, bool bold = false) { return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain)); }

// ============================================================ Content (fondo, tarjetas y logo)
void Content::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffefebe6), 0, 0, juce::Colour (0xffe2ddd6), 0, (float) getHeight(), false));
    g.fillAll();

    // logo
    {
        const float x = 18.f, y = 20.f;
        g.setColour (C::red);
        const float hs[7] = { 8, 16, 26, 30, 22, 14, 8 };
        for (int i = 0; i < 7; ++i)
            g.fillRoundedRectangle (x + (float) i * 5.f, y + 15.f - hs[i] * 0.5f, 3.f, hs[i], 1.5f);
        g.setColour (C::text);
        g.setFont (F (28.f, true).withExtraKerningFactor (0.12f));
        g.drawText ("NESCRUN", juce::Rectangle<float> (x + 44.f, y - 2.f, 220.f, 34.f), juce::Justification::centredLeft);
        g.setColour (C::sub);
        g.setFont (F (10.f, true).withExtraKerningFactor (0.2f));
        g.drawText ("VOCAL SUITE  ALL-IN-ONE", juce::Rectangle<float> (x + 46.f, y + 30.f, 260.f, 14.f), juce::Justification::centredLeft);
    }

    for (auto& c : cards)
    {
        auto r = c.r.toFloat();
        juce::Path p; p.addRoundedRectangle (r, 16.f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.13f), 18, { 0, 6 }).drawForPath (g, p);
        juce::DropShadow (juce::Colours::white.withAlpha (0.8f), 10, { 0, -3 }).drawForPath (g, p);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff9f7f4), r.getX(), r.getY(), C::card, r.getX(), r.getBottom(), false));
        g.fillPath (p);
        g.setColour (juce::Colours::white); g.strokePath (p, juce::PathStrokeType (1.2f));
        g.setColour (c.accent);
        g.setFont (F (12.5f, true).withExtraKerningFactor (0.18f));
        g.drawText (c.title, c.r.getX() + 36, c.r.getY() + 9, c.r.getWidth() - 46, 22, juce::Justification::centredLeft);
        g.setColour (C::track.withAlpha (0.7f));
        g.drawHorizontalLine (c.r.getY() + 34, r.getX() + 12.f, r.getRight() - 12.f);
    }
}

// ============================================================ Pantalla del afinador
void TuneScreen::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (C::screen); g.fillRoundedRectangle (r, 10.f);
    g.setColour (juce::Colours::white.withAlpha (0.06f)); g.drawRoundedRectangle (r.reduced (0.5f), 10.f, 1.f);

    static const char* names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    auto& t = proc.getTuner();
    const float midi = t.detectedMidi.load();
    const int target = t.targetMidi.load();
    const float cents = t.correctionCents.load();
    shownCents += (cents - shownCents) * 0.35f;
    const int mode = (int) proc.apvts.getRawParameterValue ("tune_mode")->load();

    auto left = r.removeFromLeft (r.getWidth() * 0.42f).reduced (10.f, 6.f);
    g.setFont (F (34.f, true));
    if (midi > 0.f && target >= 0)
    {
        const auto col = mode == 2 ? C::red : C::purple;
        g.setColour (col.brighter (0.2f));
        g.drawText (juce::String (names[target % 12]) + juce::String (target / 12 - 1), left, juce::Justification::centred);
    }
    else { g.setColour (juce::Colours::white.withAlpha (0.25f)); g.drawText ("--", left, juce::Justification::centred); }

    auto right = r.reduced (10.f, 10.f);
    g.setFont (F (10.f, true));
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    const juce::String modeTxt = mode == 0 ? "TUNE OFF" : (mode == 1 ? "NATURAL  /  PITCH CENTER" : "HARD  /  AUTO-TUNE FX");
    g.drawText (modeTxt, right.removeFromTop (14.f), juce::Justification::centredLeft);
    juce::String detTxt = "IN  --";
    if (midi > 0.f)
    {
        const int nn = (int) std::round (midi);
        const int off = (int) std::round ((midi - (float) nn) * 100.f);
        detTxt = "IN  " + juce::String (names[((nn % 12) + 12) % 12]) + juce::String (nn / 12 - 1) + (off >= 0 ? "  +" : "  ") + juce::String (off) + " ct";
    }
    g.drawText (detTxt, right.removeFromTop (16.f), juce::Justification::centredLeft);

    // barra de correccion (-100..+100 cents)
    auto bar = right.removeFromBottom (14.f);
    g.setColour (juce::Colours::white.withAlpha (0.08f)); g.fillRoundedRectangle (bar, 4.f);
    const float cx = bar.getCentreX();
    const float v = juce::jlimit (-1.f, 1.f, shownCents / 100.f);
    auto fill = juce::Rectangle<float>::leftTopRightBottom (std::min (cx, cx + v * bar.getWidth() * 0.5f), bar.getY(), std::max (cx, cx + v * bar.getWidth() * 0.5f), bar.getBottom());
    g.setColour (mode == 2 ? C::red : C::purple); g.fillRoundedRectangle (fill, 4.f);
    g.setColour (juce::Colours::white.withAlpha (0.5f)); g.drawVerticalLine ((int) cx, bar.getY() - 2.f, bar.getBottom() + 2.f);
    g.setFont (F (9.f, true));
    g.drawText ("CORRECCION", right.removeFromBottom (14.f), juce::Justification::centredLeft);
}

// ============================================================ Pantalla de EQ
void EqScreen::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (C::screen); g.fillRoundedRectangle (r, 10.f);
    auto plot = r.reduced (8.f, 8.f);
    const double sr = 48000.0;
    auto xOf = [&] (double f) { return plot.getX() + (float) (std::log (f / 20.0) / std::log (20000.0 / 20.0)) * plot.getWidth(); };
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    for (double f : { 100.0, 1000.0, 10000.0 }) g.drawVerticalLine ((int) xOf (f), plot.getY(), plot.getBottom());
    g.drawHorizontalLine ((int) plot.getCentreY(), plot.getX(), plot.getRight());

    auto pv = [this] (const char* id) { return proc.apvts.getRawParameterValue (id)->load(); };
    using Co = juce::dsp::IIR::Coefficients<float>;
    std::vector<Co::Ptr> cs;
    if (pv ("clean_on") > 0.5f)
    {
        cs.push_back (Co::makeHighPass (sr, pv ("hpf_freq"), 0.707f));
        cs.push_back (Co::makeHighPass (sr, pv ("hpf_freq"), 0.54f));
    }
    if (pv ("eq_on") > 0.5f)
    {
        cs.push_back (Co::makeLowShelf (sr, 120.0, 0.7, linOf (pv ("eq_body"))));
        cs.push_back (Co::makePeakFilter (sr, 350.0, 1.0, linOf (pv ("eq_mud"))));
        cs.push_back (Co::makePeakFilter (sr, 1200.0, 0.8, linOf (pv ("eq_mid"))));
        cs.push_back (Co::makePeakFilter (sr, 4000.0, 0.7, linOf (pv ("eq_presence"))));
        cs.push_back (Co::makeHighShelf (sr, 12000.0, 0.6, linOf (pv ("eq_air"))));
    }
    juce::Path curve;
    const int N = 160;
    for (int i = 0; i <= N; ++i)
    {
        const double f = 20.0 * std::pow (1000.0, (double) i / N);
        double mag = 1.0;
        for (auto& c : cs) mag *= c->getMagnitudeForFrequency (f, sr);
        const float db = juce::jlimit (-18.f, 18.f, (float) juce::Decibels::gainToDecibels (mag));
        const float y = plot.getCentreY() - db / 18.f * plot.getHeight() * 0.5f;
        if (i == 0) curve.startNewSubPath (xOf (f), y); else curve.lineTo (xOf (f), y);
    }
    g.setColour (C::red.withAlpha (0.25f)); g.strokePath (curve, juce::PathStrokeType (5.f));
    g.setColour (C::red); g.strokePath (curve, juce::PathStrokeType (2.f));

    g.setFont (F (9.5f, true));
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.drawText ("100", juce::Rectangle<float> (xOf (100) - 20, plot.getBottom() - 12, 40, 12), juce::Justification::centred);
    g.drawText ("1k", juce::Rectangle<float> (xOf (1000) - 20, plot.getBottom() - 12, 40, 12), juce::Justification::centred);
    g.drawText ("10k", juce::Rectangle<float> (xOf (10000) - 20, plot.getBottom() - 12, 40, 12), juce::Justification::centred);
    const float red = proc.resoGr.load();
    g.setColour (C::purple.brighter (0.3f));
    g.drawText ("AI CLEAN " + juce::String (red, 1) + " dB", plot.removeFromTop (12.f), juce::Justification::centredRight);
}

// ============================================================ Medidor
void Meter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setFont (F (9.5f, true)); g.setColour (C::sub);
    g.drawText (label, r.removeFromLeft (28.f), juce::Justification::centredLeft);
    auto txt = r.removeFromRight (40.f);
    auto bar = r.withSizeKeepingCentre (r.getWidth(), 6.f);
    g.setColour (C::track); g.fillRoundedRectangle (bar, 3.f);
    float frac;
    if (isGr) frac = juce::jlimit (0.f, 1.f, -value / 18.f);
    else frac = juce::jlimit (0.f, 1.f, (value + 60.f) / 60.f);
    auto f = bar.withWidth (bar.getWidth() * frac);
    if (isGr) f = bar.withLeft (bar.getRight() - bar.getWidth() * frac);
    if (! isGr) g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4cc38a), bar.getX(), 0, juce::Colour (0xfff0a33a), bar.getRight(), 0, false));
    else g.setColour (col);
    g.fillRoundedRectangle (f, 3.f);
    g.setColour (C::text);
    g.drawText (juce::String (value, 1), txt, juce::Justification::centredRight);
}

// ============================================================ Editor
NescrunEditor::NescrunEditor (NescrunProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&look);
    addAndMakeVisible (content);
    buildLayout();

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) W / (double) H);
    setResizeLimits (W * 6 / 10, H * 6 / 10, W * 3 / 2, H * 3 / 2);
    setSize (W, H);
    startTimerHz (30);
}

NescrunEditor::~NescrunEditor() { stopTimer(); setLookAndFeel (nullptr); }

void NescrunEditor::paint (juce::Graphics& g) { g.fillAll (C::bg); }

void NescrunEditor::resized()
{
    const float sc = (float) getWidth() / (float) W;
    content.setBounds (0, 0, W, H);
    content.setTransform (juce::AffineTransform::scale (sc));
}

void NescrunEditor::buildLayout()
{
    auto& st = proc.apvts;
    using R = juce::Rectangle<int>;
    auto add = [this] (juce::Component* c, R r) { owned.add (c); content.addAndMakeVisible (c); c->setBounds (r); return c; };
    auto knob = [&] (const char* id, const char* name, juce::Colour a, R r) { add (new Knob (st, id, name, a), r); };
    auto power = [&] (const char* id, R card, juce::Colour a)
    {
        auto* b = new juce::ToggleButton();
        b->setColour (juce::ToggleButton::tickColourId, a);
        add (b, R (card.getX() + 10, card.getY() + 8, 24, 24));
        btnAtts.add (new APVTS::ButtonAttachment (st, id, *b));
    };
    auto combo = [&] (const char* id, const juce::StringArray& items, R r)
    {
        auto* c = new juce::ComboBox();
        c->addItemList (items, 1);
        add (c, r);
        comboAtts.add (new APVTS::ComboBoxAttachment (st, id, *c));
    };

    const auto P = C::purple, Rd = C::red;
    const R tune (16, 78, 320, 318), clean (346, 78, 250, 318), dyn (606, 78, 250, 318), tone (866, 78, 318, 318);
    const R color (16, 406, 236, 282), modR (262, 406, 196, 282), del (468, 406, 246, 282), rev (724, 406, 246, 282), mst (980, 406, 204, 282);
    content.cards = { { tune, "TUNE", P }, { clean, "CLEAN  /  AI", P }, { dyn, "DYNAMICS", Rd }, { tone, "TONE  /  SILK", Rd },
                      { color, "COLOR  /  BOOM", Rd }, { modR, "MOTION", P }, { del, "DELAY", P }, { rev, "SPACE", P }, { mst, "MASTER", Rd } };

    // ---- TUNE
    content.addAndMakeVisible (tuneScreen); tuneScreen.setBounds (28, 120, 296, 84);
    add (new Segments (st, "tune_mode", { "Off", "Natural", "Hard" }, P), R (26, 210, 300, 30));
    combo ("tune_key", kKeys, R (26, 246, 92, 30));
    combo ("tune_scale", kScales, R (122, 246, 204, 30));
    knob ("tune_speed", "Speed", P, R (28, 282, 98, 106));
    knob ("tune_amount", "Amount", P, R (127, 282, 98, 106));
    knob ("tune_human", "Humanize", P, R (226, 282, 98, 106));

    // ---- CLEAN
    power ("clean_on", clean, P);
    knob ("reso_amount", "Resonance AI", P, R (354, 118, 80, 92));
    knob ("reso_sens", "AI Focus", P, R (432, 118, 80, 92));
    knob ("hpf_freq", "Low Cut", P, R (510, 118, 80, 92));
    knob ("deess_thresh", "De-Ess", P, R (393, 210, 80, 92));
    knob ("deess_freq", "S Freq", P, R (471, 210, 80, 92));
    add (new Fader (st, "gate_thresh", "Noise Floor", P, false), R (360, 304, 222, 38));
    content.addAndMakeVisible (resoMeter); resoMeter.setBounds (360, 348, 222, 18);
    content.addAndMakeVisible (deessMeter); deessMeter.setBounds (360, 370, 222, 18);

    // ---- DYNAMICS
    power ("comp_on", dyn, Rd);
    add (new Segments (st, "comp_style", { "Clean", "FET", "Opto", "Vari-Mu" }, Rd, 2), R (616, 116, 230, 60));
    content.addAndMakeVisible (compMeter); compMeter.setBounds (618, 180, 226, 20);
    knob ("comp_amount", "Compress", Rd, R (671, 202, 120, 112));
    knob ("level_amount", "Leveler", Rd, R (630, 308, 90, 84));
    knob ("comp_mix", "Parallel", Rd, R (742, 308, 90, 84));

    // ---- TONE
    power ("eq_on", tone, Rd);
    content.addAndMakeVisible (eqScreen); eqScreen.setBounds (878, 118, 294, 90);
    const char* eqIds[5] = { "eq_body", "eq_mud", "eq_mid", "eq_presence", "eq_air" };
    const char* eqNames[5] = { "Body", "Mud", "Mid", "Pres", "Air" };
    for (int i = 0; i < 5; ++i) add (new Fader (st, eqIds[i], eqNames[i], Rd, true), R (876 + i * 45, 216, 44, 172));
    knob ("silk_amount", "Silk", Rd, R (1104, 222, 72, 88));
    add (new Segments (st, "silk_blue", { "Red", "Blue" }, Rd, 1), R (1106, 316, 68, 60));

    // ---- COLOR
    power ("color_on", color, Rd);
    add (new Segments (st, "sat_type", { "Tape", "Tube", "Clip" }, Rd), R (26, 444, 216, 30));
    knob ("sat_drive", "Drive", Rd, R (28, 480, 70, 84));
    knob ("sat_tone", "Tone", Rd, R (99, 480, 70, 84));
    knob ("sat_mix", "Mix", Rd, R (170, 480, 70, 84));
    knob ("boom", "BOOM", Rd, R (70, 568, 128, 116));

    // ---- MOTION
    power ("mod_on", modR, P);
    add (new Segments (st, "mod_type", { "Doubler", "Chorus", "Flanger", "Phaser" }, P, 2), R (272, 444, 176, 60));
    knob ("mod_amount", "Amount", P, R (272, 510, 88, 88));
    knob ("mod_rate", "Rate", P, R (360, 510, 88, 88));
    knob ("width", "Width", P, R (314, 600, 88, 84));

    // ---- DELAY
    power ("delay_on", del, P);
    combo ("delay_div", kDelayDivs, R (478, 444, 112, 30));
    {
        auto* b = new juce::TextButton ("PING-PONG");
        b->setClickingTogglesState (true);
        b->setColour (juce::TextButton::buttonOnColourId, P);
        add (b, R (596, 444, 108, 30));
        btnAtts.add (new APVTS::ButtonAttachment (st, "delay_pp", *b));
    }
    knob ("delay_fb", "Feedback", P, R (480, 482, 74, 96));
    knob ("delay_mix", "Mix", P, R (554, 482, 74, 96));
    knob ("delay_tone", "Tone", P, R (628, 482, 74, 96));
    knob ("delay_duck", "Duck", P, R (554, 584, 74, 96));

    // ---- SPACE (reverb)
    power ("rev_on", rev, P);
    knob ("rev_size", "Size", P, R (734, 446, 76, 104));
    knob ("rev_damp", "Damp", P, R (809, 446, 76, 104));
    knob ("rev_predelay", "Pre-Delay", P, R (884, 446, 76, 104));
    knob ("rev_mix", "Mix", P, R (760, 566, 86, 110));
    knob ("rev_duck", "Duck", P, R (850, 566, 86, 110));

    // ---- MASTER
    power ("lim_on", mst, Rd);
    knob ("loud", "Loudness", Rd, R (988, 440, 104, 104));
    knob ("ceiling", "Ceiling", Rd, R (1092, 448, 84, 92));
    knob ("in_gain", "Input", Rd, R (988, 546, 62, 76));
    knob ("out_gain", "Output", Rd, R (1051, 546, 62, 76));
    knob ("mix", "Dry/Wet", Rd, R (1114, 546, 62, 76));
    content.addAndMakeVisible (inMeter);  inMeter.setBounds (992, 624, 182, 18);
    content.addAndMakeVisible (outMeter); outMeter.setBounds (992, 644, 182, 18);
    content.addAndMakeVisible (limMeter); limMeter.setBounds (992, 664, 182, 18);

    // ---- barra superior
    content.addAndMakeVisible (prevBtn);   prevBtn.setBounds (372, 18, 38, 36);
    content.addAndMakeVisible (presetBox); presetBox.setBounds (414, 18, 372, 36);
    content.addAndMakeVisible (nextBtn);   nextBtn.setBounds (790, 18, 38, 36);
    content.addAndMakeVisible (saveBtn);   saveBtn.setBounds (850, 18, 74, 36);
    content.addAndMakeVisible (loadBtn);   loadBtn.setBounds (928, 18, 74, 36);
    content.addAndMakeVisible (autoBtn);   autoBtn.setBounds (1014, 18, 170, 36);
    autoBtn.setColour (juce::TextButton::buttonOnColourId, P);
    autoBtn.setToggleState (true, juce::dontSendNotification);
    autoBtn.setTooltip ("Canta o reproduci la voz ~7 segundos: NESCRUN analiza y ajusta ganancia, EQ, de-esser y compresion.");

    presetBox.setTextWhenNothingSelected ("Presets");
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id <= 0) return;
        if (id < 1000) { if (id - 1 != proc.getCurrentProgram() || proc.userPresetName.isNotEmpty()) { proc.setCurrentProgram (id - 1); proc.updateHostDisplay(); } }
        else
        {
            const auto f = userPresets[id - 1000];
            if (auto xml = juce::XmlDocument::parse (f))
                if (xml->hasTagName (proc.apvts.state.getType())) { proc.apvts.replaceState (juce::ValueTree::fromXml (*xml)); proc.userPresetName = f.getFileNameWithoutExtension(); }
        }
    };
    prevBtn.onClick = [this] { const int n = proc.getNumPrograms(); proc.setCurrentProgram ((proc.getCurrentProgram() + n - 1) % n); refreshPresetBox(); };
    nextBtn.onClick = [this] { const int n = proc.getNumPrograms(); proc.setCurrentProgram ((proc.getCurrentProgram() + 1) % n); refreshPresetBox(); };
    saveBtn.onClick = [this] { savePreset(); };
    loadBtn.onClick = [this] { loadPreset(); };
    autoBtn.onClick = [this] { autoBtn.setToggleState (true, juce::dontSendNotification); proc.startAutoMix(); };
    refreshPresetBox();

    footer.setFont (F (10.5f));
    footer.setColour (juce::Label::textColourId, C::sub);
    footer.setJustificationType (juce::Justification::centred);
    content.addAndMakeVisible (footer); footer.setBounds (16, 694, 1168, 20);
    footer.setText ("NESCRUN v1.0  -  latencia " + juce::String (proc.getLatencySamples()) + " muestras (FL Studio la compensa sola)  -  doble clic en un control = valor por defecto",
                    juce::dontSendNotification);
}

void NescrunEditor::refreshPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    const auto& ps = factoryPresets();
    juce::String lastCat;
    for (size_t i = 0; i < ps.size(); ++i)
    {
        if (lastCat != ps[i].category) { lastCat = ps[i].category; presetBox.addSectionHeading (lastCat.toUpperCase()); }
        presetBox.addItem (ps[i].name, (int) i + 1);
    }
    userPresets.clear();
    if (presetDir().isDirectory())
        userPresets = presetDir().findChildFiles (juce::File::findFiles, false, "*.nescrun");
    if (! userPresets.isEmpty())
    {
        presetBox.addSectionHeading ("MIS PRESETS");
        for (int i = 0; i < userPresets.size(); ++i) presetBox.addItem (userPresets[i].getFileNameWithoutExtension(), 1000 + i);
    }
    int sel = proc.getCurrentProgram() + 1;
    if (proc.userPresetName.isNotEmpty())
        for (int i = 0; i < userPresets.size(); ++i) if (userPresets[i].getFileNameWithoutExtension() == proc.userPresetName) sel = 1000 + i;
    presetBox.setSelectedId (sel, juce::dontSendNotification);
}

juce::File NescrunEditor::presetDir() const
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("NESCRUN Presets");
}

void NescrunEditor::savePreset()
{
    presetDir().createDirectory();
    chooser = std::make_unique<juce::FileChooser> ("Guardar preset NESCRUN", presetDir().getChildFile ("Mi preset.nescrun"), "*.nescrun");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
        [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f == juce::File()) return;
            f = f.withFileExtension ("nescrun");
            if (auto xml = proc.apvts.copyState().createXml()) xml->writeTo (f);
            proc.userPresetName = f.getFileNameWithoutExtension();
            refreshPresetBox();
        });
}

void NescrunEditor::loadPreset()
{
    presetDir().createDirectory();
    chooser = std::make_unique<juce::FileChooser> ("Abrir preset NESCRUN", presetDir(), "*.nescrun");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (! f.existsAsFile()) return;
            if (auto xml = juce::XmlDocument::parse (f))
                if (xml->hasTagName (proc.apvts.state.getType()))
                {
                    proc.apvts.replaceState (juce::ValueTree::fromXml (*xml));
                    proc.userPresetName = f.getFileNameWithoutExtension();
                }
            refreshPresetBox();
        });
}

void NescrunEditor::timerCallback()
{
    inMeter.setValue (proc.inLevel.load());
    outMeter.setValue (proc.outLevel.load());
    limMeter.setValue (proc.limGr.load());
    compMeter.setValue (proc.compGr.load());
    deessMeter.setValue (proc.deessGr.load());
    resoMeter.setValue (proc.resoGr.load());
    tuneScreen.repaint();
    eqScreen.repaint();

    const int s = proc.autoMixState.load();
    if (s == 1) autoBtn.setButtonText ("ESCUCHANDO " + juce::String ((int) (proc.autoMixProgress.load() * 100.f)) + "%");
    else autoBtn.setButtonText ("AUTO MIX");

    if (proc.getCurrentProgram() != lastProg) { lastProg = proc.getCurrentProgram(); refreshPresetBox(); }
}
