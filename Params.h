#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Tabla central de parametros de NESCRUN.
namespace nescrun
{
enum class PType { Float, Choice, Bool };

struct ParamDef
{
    const char* id;
    const char* name;
    PType type;
    float minV, maxV, def, skewCentre; // skewCentre <= 0 -> lineal
    const char* unit;
    juce::StringArray choices;
};

inline const juce::StringArray kTuneModes   { "Off", "Natural", "Hard" };
inline const juce::StringArray kKeys        { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
inline const juce::StringArray kScales      { "Chromatic", "Major", "Minor", "Harm. Minor", "Pent. Minor", "Pent. Major", "Dorian", "Phrygian" };
inline const juce::StringArray kCompStyles  { "Clean", "FET", "Opto", "Vari-Mu" };
inline const juce::StringArray kSatTypes    { "Tape", "Tube", "Clip" };
inline const juce::StringArray kModTypes    { "Doubler", "Chorus", "Flanger", "Phaser" };
inline const juce::StringArray kDelayDivs   { "1/16", "1/8", "1/8 D", "1/4", "1/4 D", "1/2", "1/8 T", "1/4 T" };

inline const std::vector<ParamDef>& paramDefs()
{
    static const std::vector<ParamDef> defs = {
        // Global
        { "in_gain",   "Input",        PType::Float, -24.f, 24.f, 0.f, 0.f, "dB", {} },
        { "out_gain",  "Output",       PType::Float, -24.f, 12.f, 0.f, 0.f, "dB", {} },
        { "mix",       "Dry/Wet",      PType::Float, 0.f, 100.f, 100.f, 0.f, "%", {} },

        // Tune
        { "tune_mode",   "Tune Mode",   PType::Choice, 0, 2, 1, 0, "", kTuneModes },
        { "tune_key",    "Key",         PType::Choice, 0, 11, 0, 0, "", kKeys },
        { "tune_scale",  "Scale",       PType::Choice, 0, 7, 0, 0, "", kScales },
        { "tune_speed",  "Retune Speed",PType::Float, 0.f, 400.f, 60.f, 60.f, "ms", {} },
        { "tune_amount", "Tune Amount", PType::Float, 0.f, 100.f, 85.f, 0.f, "%", {} },
        { "tune_human",  "Humanize",    PType::Float, 0.f, 100.f, 70.f, 0.f, "%", {} },

        // Clean
        { "clean_on",    "Clean On",    PType::Bool, 0, 1, 1, 0, "", {} },
        { "gate_thresh", "Noise Floor", PType::Float, -90.f, -20.f, -65.f, 0.f, "dB", {} },
        { "hpf_freq",    "Low Cut",     PType::Float, 20.f, 400.f, 85.f, 100.f, "Hz", {} },
        { "reso_amount", "Resonance AI",PType::Float, 0.f, 100.f, 45.f, 0.f, "%", {} },
        { "reso_sens",   "AI Focus",    PType::Float, 0.f, 100.f, 50.f, 0.f, "%", {} },
        { "deess_thresh","De-Ess",      PType::Float, -50.f, 0.f, -26.f, 0.f, "dB", {} },
        { "deess_freq",  "De-Ess Freq", PType::Float, 3000.f, 11000.f, 6500.f, 6000.f, "Hz", {} },

        // Dynamics
        { "comp_on",     "Comp On",     PType::Bool, 0, 1, 1, 0, "", {} },
        { "comp_style",  "Comp Style",  PType::Choice, 0, 3, 1, 0, "", kCompStyles },
        { "comp_amount", "Compress",    PType::Float, 0.f, 100.f, 45.f, 0.f, "%", {} },
        { "level_amount","Leveler",     PType::Float, 0.f, 100.f, 35.f, 0.f, "%", {} },
        { "comp_mix",    "Parallel",    PType::Float, 0.f, 100.f, 100.f, 0.f, "%", {} },

        // Tone
        { "eq_on",       "EQ On",       PType::Bool, 0, 1, 1, 0, "", {} },
        { "eq_body",     "Body 120",    PType::Float, -12.f, 12.f, 0.f, 0.f, "dB", {} },
        { "eq_mud",      "Mud 350",     PType::Float, -12.f, 12.f, -1.5f, 0.f, "dB", {} },
        { "eq_mid",      "Mid 1.2k",    PType::Float, -12.f, 12.f, 0.f, 0.f, "dB", {} },
        { "eq_presence", "Presence 4k", PType::Float, -12.f, 12.f, 2.f, 0.f, "dB", {} },
        { "eq_air",      "Air 12k",     PType::Float, -12.f, 12.f, 3.f, 0.f, "dB", {} },
        { "silk_amount", "Silk",        PType::Float, 0.f, 100.f, 30.f, 0.f, "%", {} },
        { "silk_blue",   "Silk Blue",   PType::Bool, 0, 1, 0, 0, "", {} },

        // Color
        { "color_on",    "Color On",    PType::Bool, 0, 1, 1, 0, "", {} },
        { "sat_type",    "Sat Type",    PType::Choice, 0, 2, 0, 0, "", kSatTypes },
        { "sat_drive",   "Drive",       PType::Float, 0.f, 100.f, 20.f, 0.f, "%", {} },
        { "sat_tone",    "Tone",        PType::Float, 0.f, 100.f, 60.f, 0.f, "%", {} },
        { "sat_mix",     "Sat Mix",     PType::Float, 0.f, 100.f, 50.f, 0.f, "%", {} },
        { "boom",        "BOOM",        PType::Float, 0.f, 100.f, 15.f, 0.f, "%", {} },

        // Modulation
        { "mod_on",      "Mod On",      PType::Bool, 0, 1, 0, 0, "", {} },
        { "mod_type",    "Mod Type",    PType::Choice, 0, 3, 0, 0, "", kModTypes },
        { "mod_amount",  "Mod Amount",  PType::Float, 0.f, 100.f, 40.f, 0.f, "%", {} },
        { "mod_rate",    "Mod Rate",    PType::Float, 0.05f, 5.f, 0.6f, 0.8f, "Hz", {} },

        // Delay
        { "delay_on",    "Delay On",    PType::Bool, 0, 1, 1, 0, "", {} },
        { "delay_div",   "Delay Time",  PType::Choice, 0, 7, 3, 0, "", kDelayDivs },
        { "delay_fb",    "Feedback",    PType::Float, 0.f, 95.f, 30.f, 0.f, "%", {} },
        { "delay_mix",   "Delay Mix",   PType::Float, 0.f, 100.f, 12.f, 0.f, "%", {} },
        { "delay_tone",  "Delay Tone",  PType::Float, 0.f, 100.f, 45.f, 0.f, "%", {} },
        { "delay_pp",    "Ping-Pong",   PType::Bool, 0, 1, 1, 0, "", {} },
        { "delay_duck",  "Delay Duck",  PType::Float, 0.f, 100.f, 50.f, 0.f, "%", {} },

        // Reverb
        { "rev_on",      "Reverb On",   PType::Bool, 0, 1, 1, 0, "", {} },
        { "rev_size",    "Size",        PType::Float, 0.f, 100.f, 45.f, 0.f, "%", {} },
        { "rev_damp",    "Damp",        PType::Float, 0.f, 100.f, 50.f, 0.f, "%", {} },
        { "rev_predelay","Pre-Delay",   PType::Float, 0.f, 150.f, 25.f, 0.f, "ms", {} },
        { "rev_mix",     "Reverb Mix",  PType::Float, 0.f, 100.f, 14.f, 0.f, "%", {} },
        { "rev_duck",    "Reverb Duck", PType::Float, 0.f, 100.f, 40.f, 0.f, "%", {} },
        { "width",       "Width",       PType::Float, 0.f, 200.f, 110.f, 0.f, "%", {} },

        // Master
        { "lim_on",      "Limiter On",  PType::Bool, 0, 1, 1, 0, "", {} },
        { "loud",        "Loudness",    PType::Float, 0.f, 18.f, 3.f, 0.f, "dB", {} },
        { "ceiling",     "Ceiling",     PType::Float, -6.f, 0.f, -1.f, 0.f, "dB", {} },
    };
    return defs;
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (auto& d : paramDefs())
    {
        const juce::ParameterID pid { d.id, 1 };
        if (d.type == PType::Float)
        {
            juce::NormalisableRange<float> r (d.minV, d.maxV);
            if (d.skewCentre > 0.f) r.setSkewForCentre (d.skewCentre);
            const juce::String unit (d.unit);
            layout.add (std::make_unique<juce::AudioParameterFloat> (
                pid, d.name, r, d.def,
                juce::AudioParameterFloatAttributes().withLabel (unit)
                    .withStringFromValueFunction ([unit] (float v, int)
                    {
                        if (unit == "Hz" && v >= 1000.f) return juce::String (v / 1000.f, 1) + " kHz";
                        if (unit == "Hz" && v < 10.f) return juce::String (v, 2) + " Hz";
                        if (unit == "Hz") return juce::String ((int) std::round (v)) + " Hz";
                        if (unit == "%")  return juce::String ((int) std::round (v)) + "%";
                        if (unit == "ms") return juce::String ((int) std::round (v)) + " ms";
                        return juce::String (v, 1) + " " + unit;
                    })));
        }
        else if (d.type == PType::Choice)
            layout.add (std::make_unique<juce::AudioParameterChoice> (pid, d.name, d.choices, (int) d.def));
        else
            layout.add (std::make_unique<juce::AudioParameterBool> (pid, d.name, d.def > 0.5f));
    }
    return layout;
}
} // namespace nescrun
