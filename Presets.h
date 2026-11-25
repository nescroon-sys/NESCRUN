#pragma once
#include "Params.h"

// Presets de fabrica. Son aproximaciones "de oido" al tipo de mezcla vocal
// asociada a cada estilo/artista; NESCRUN no tiene relacion con esos artistas.
// Valores en unidades reales (dB, %, Hz, indice de opcion, 0/1).
namespace nescrun
{
struct Preset
{
    const char* category;
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

inline const std::vector<Preset>& factoryPresets()
{
    // tune_mode: 0 Off 1 Natural 2 Hard | comp_style: 0 Clean 1 FET 2 Opto 3 Vari-Mu
    // sat_type: 0 Tape 1 Tube 2 Clip | mod_type: 0 Doubler 1 Chorus 2 Flanger 3 Phaser
    // delay_div: 0 1/16 1 1/8 2 1/8D 3 1/4 4 1/4D 5 1/2 6 1/8T 7 1/4T
    static const std::vector<Preset> p = {
        { "Basico", "Default - Lista para mezclar", {} },
        { "Basico", "Afinacion Natural Transparente", {
            {"tune_mode",1},{"tune_speed",90},{"tune_amount",80},{"tune_human",85},
            {"comp_amount",30},{"level_amount",30},{"sat_drive",8},{"boom",5},
            {"delay_on",0},{"rev_mix",8},{"loud",1.5f} } },
        { "Basico", "Vocal Comercial Fuerte (sin saturar)", {
            {"tune_mode",1},{"tune_speed",50},{"reso_amount",60},{"deess_thresh",-30},
            {"comp_style",1},{"comp_amount",60},{"level_amount",45},{"eq_presence",3},{"eq_air",4},
            {"silk_amount",40},{"sat_drive",22},{"boom",25},{"loud",6},{"ceiling",-1} } },

        // ------------------------------------------------ TRAP / URBANO
        { "Trap", "Hook Autotune Duro (estilo Travis Scott)", {
            {"tune_mode",2},{"tune_speed",0},{"tune_amount",100},{"tune_human",0},
            {"hpf_freq",110},{"reso_amount",55},{"comp_style",1},{"comp_amount",65},{"level_amount",40},
            {"eq_body",-1},{"eq_mud",-3},{"eq_presence",3},{"eq_air",5},{"silk_amount",35},
            {"sat_type",0},{"sat_drive",30},{"sat_mix",45},{"boom",20},
            {"mod_on",1},{"mod_type",0},{"mod_amount",35},
            {"delay_div",2},{"delay_fb",38},{"delay_mix",20},{"delay_tone",35},
            {"rev_size",80},{"rev_mix",24},{"rev_predelay",40},{"width",140},{"loud",5} } },
        { "Trap", "Melodico Suave (estilo Gunna)", {
            {"tune_mode",2},{"tune_speed",15},{"tune_amount",95},{"tune_human",20},
            {"comp_style",2},{"comp_amount",55},{"level_amount",45},{"eq_mud",-2.5f},{"eq_presence",2},{"eq_air",4},
            {"silk_amount",45},{"sat_drive",15},{"boom",15},{"delay_div",3},{"delay_fb",25},{"delay_mix",14},
            {"rev_size",60},{"rev_mix",16},{"loud",4} } },
        { "Trap", "Rage Distorsionado (estilo Playboi Carti)", {
            {"tune_mode",2},{"tune_speed",0},{"tune_amount",100},{"tune_human",0},
            {"hpf_freq",150},{"comp_style",1},{"comp_amount",75},{"eq_body",-3},{"eq_mid",3},{"eq_presence",4},{"eq_air",2},
            {"sat_type",2},{"sat_drive",70},{"sat_tone",45},{"sat_mix",75},{"boom",10},
            {"mod_on",1},{"mod_type",0},{"mod_amount",50},{"delay_div",1},{"delay_fb",30},{"delay_mix",14},
            {"rev_size",50},{"rev_mix",12},{"loud",7} } },
        { "Trap", "Emo Rap Melodico (estilo Juice WRLD)", {
            {"tune_mode",2},{"tune_speed",10},{"tune_amount",95},{"tune_human",15},
            {"comp_style",2},{"comp_amount",50},{"eq_mud",-2},{"eq_presence",2.5f},{"eq_air",3.5f},
            {"silk_amount",35},{"sat_drive",20},{"mod_on",1},{"mod_type",0},{"mod_amount",30},
            {"delay_div",4},{"delay_fb",42},{"delay_mix",22},{"delay_tone",30},{"rev_size",75},{"rev_mix",22},{"width",130},{"loud",4} } },
        { "Trap", "Drill Grave y Adelante (estilo Pop Smoke)", {
            {"tune_mode",1},{"tune_speed",40},{"tune_amount",70},
            {"hpf_freq",70},{"comp_style",1},{"comp_amount",70},{"level_amount",40},{"eq_body",3},{"eq_mud",-1},{"eq_presence",2},{"eq_air",1},
            {"sat_type",1},{"sat_drive",35},{"sat_mix",55},{"boom",55},{"delay_on",0},{"rev_mix",8},{"rev_size",30},{"loud",6} } },
        { "Trap", "Trap Argentino (estilo Duki)", {
            {"tune_mode",2},{"tune_speed",8},{"tune_amount",95},{"tune_human",10},
            {"comp_style",1},{"comp_amount",62},{"level_amount",40},{"eq_mud",-3},{"eq_presence",3},{"eq_air",4},
            {"silk_amount",30},{"sat_drive",28},{"boom",25},{"mod_on",1},{"mod_type",0},{"mod_amount",28},
            {"delay_div",2},{"delay_fb",30},{"delay_mix",15},{"rev_size",55},{"rev_mix",14},{"loud",5} } },
        { "Trap", "Freestyle Rap Seco (estilo Trueno)", {
            {"tune_mode",1},{"tune_speed",80},{"tune_amount",60},{"tune_human",90},
            {"comp_style",1},{"comp_amount",68},{"level_amount",50},{"eq_body",1},{"eq_mud",-2},{"eq_presence",3.5f},{"eq_air",2.5f},
            {"sat_type",1},{"sat_drive",25},{"boom",35},{"delay_on",0},{"rev_size",25},{"rev_mix",6},{"loud",5} } },
        { "Trap", "Session Club (estilo BZRP Session)", {
            {"tune_mode",2},{"tune_speed",12},{"tune_amount",90},
            {"comp_style",1},{"comp_amount",66},{"level_amount",45},{"eq_mud",-2.5f},{"eq_presence",3},{"eq_air",5},
            {"silk_amount",40},{"sat_drive",25},{"boom",30},{"delay_div",1},{"delay_fb",22},{"delay_mix",10},
            {"rev_size",40},{"rev_mix",10},{"width",120},{"loud",7} } },
        { "Trap", "Rap Clasico Punch (estilo Kendrick / Eminem)", {
            {"tune_mode",0},{"comp_style",1},{"comp_amount",72},{"level_amount",50},{"eq_body",1.5f},{"eq_mud",-2},{"eq_presence",4},{"eq_air",2},
            {"sat_type",1},{"sat_drive",30},{"boom",40},{"delay_on",0},{"rev_size",20},{"rev_mix",5},{"loud",6} } },
        { "Trap", "Ad-libs Telefono / Radio", {
            {"tune_mode",2},{"tune_speed",0},{"hpf_freq",400},{"eq_body",-12},{"eq_mud",-4},{"eq_mid",6},{"eq_presence",4},{"eq_air",-10},
            {"comp_style",1},{"comp_amount",80},{"sat_type",2},{"sat_drive",55},{"sat_mix",80},{"boom",0},
            {"delay_div",1},{"delay_fb",35},{"delay_mix",22},{"rev_mix",15},{"width",160},{"loud",4} } },

        // ------------------------------------------------ REGGAETON / LATINO
        { "Latino", "Reggaeton Comercial (estilo Bad Bunny)", {
            {"tune_mode",2},{"tune_speed",14},{"tune_amount",92},{"tune_human",20},
            {"comp_style",1},{"comp_amount",58},{"level_amount",45},{"eq_mud",-2},{"eq_presence",2.5f},{"eq_air",4},
            {"silk_amount",35},{"sat_drive",20},{"boom",30},{"mod_on",1},{"mod_type",0},{"mod_amount",22},
            {"delay_div",2},{"delay_fb",28},{"delay_mix",14},{"rev_size",55},{"rev_mix",15},{"loud",6} } },
        { "Latino", "Pop Urbano Brillante (estilo J Balvin)", {
            {"tune_mode",2},{"tune_speed",20},{"tune_amount",90},{"comp_style",2},{"comp_amount",55},
            {"eq_mud",-2},{"eq_presence",2},{"eq_air",5},{"silk_amount",50},{"sat_drive",12},{"boom",20},
            {"delay_div",3},{"delay_fb",20},{"delay_mix",10},{"rev_size",50},{"rev_mix",14},{"loud",6} } },
        { "Latino", "RKT / Cumbia 420 (estilo L-Gante)", {
            {"tune_mode",2},{"tune_speed",0},{"tune_amount",100},{"hpf_freq",120},
            {"comp_style",1},{"comp_amount",70},{"eq_mud",-3},{"eq_mid",2},{"eq_presence",3},{"sat_type",2},{"sat_drive",40},{"sat_mix",60},
            {"boom",25},{"delay_div",1},{"delay_fb",35},{"delay_mix",18},{"rev_mix",12},{"loud",8} } },
        { "Latino", "Pop Latino Femenino (estilo Maria Becerra)", {
            {"tune_mode",1},{"tune_speed",30},{"tune_amount",90},{"tune_human",60},
            {"comp_style",2},{"comp_amount",50},{"level_amount",50},{"eq_mud",-2},{"eq_presence",2},{"eq_air",5},{"silk_amount",55},
            {"deess_thresh",-30},{"sat_drive",10},{"boom",10},{"delay_div",4},{"delay_fb",25},{"delay_mix",12},{"rev_size",65},{"rev_mix",18},{"loud",5} } },

        // ------------------------------------------------ POP / R&B
        { "Pop", "Pop Brillante Radio (estilo Dua Lipa)", {
            {"tune_mode",1},{"tune_speed",35},{"tune_amount",90},{"tune_human",55},
            {"comp_style",2},{"comp_amount",55},{"level_amount",50},{"eq_mud",-2},{"eq_presence",2},{"eq_air",5},{"silk_amount",50},
            {"sat_drive",15},{"boom",15},{"delay_div",3},{"delay_fb",22},{"delay_mix",10},{"rev_size",60},{"rev_mix",16},{"loud",6} } },
        { "Pop", "Intimo Susurrado (estilo Billie Eilish)", {
            {"tune_mode",1},{"tune_speed",60},{"tune_amount",80},{"tune_human",85},{"gate_thresh",-75},
            {"comp_style",2},{"comp_amount",65},{"level_amount",60},{"eq_body",2.5f},{"eq_mud",-1},{"eq_presence",-1},{"eq_air",3},
            {"deess_thresh",-34},{"silk_amount",30},{"silk_blue",1},{"sat_type",0},{"sat_drive",18},{"boom",30},
            {"delay_on",0},{"rev_size",30},{"rev_mix",9},{"rev_predelay",10},{"width",90},{"loud",4} } },
        { "Pop", "R&B 80s Nocturno (estilo The Weeknd)", {
            {"tune_mode",1},{"tune_speed",25},{"tune_amount",92},{"tune_human",45},
            {"comp_style",3},{"comp_amount",55},{"eq_mud",-2},{"eq_presence",2},{"eq_air",4},{"silk_amount",40},
            {"mod_on",1},{"mod_type",1},{"mod_amount",30},{"mod_rate",0.4f},
            {"delay_div",4},{"delay_fb",40},{"delay_mix",18},{"delay_tone",30},{"rev_size",85},{"rev_mix",24},{"rev_predelay",60},{"width",140},{"loud",5} } },
        { "Pop", "Balada Grande (estilo Adele)", {
            {"tune_mode",1},{"tune_speed",70},{"tune_amount",75},{"tune_human",90},
            {"comp_style",2},{"comp_amount",45},{"level_amount",55},{"eq_body",1},{"eq_mud",-1.5f},{"eq_presence",1.5f},{"eq_air",3},
            {"silk_amount",30},{"silk_blue",1},{"sat_type",1},{"sat_drive",12},{"delay_on",0},{"rev_size",80},{"rev_mix",22},{"rev_predelay",50},{"loud",4} } },

        // ------------------------------------------------ ROCK
        { "Rock", "Rock Arena al Frente (estilo CLA)", {
            {"tune_mode",1},{"tune_speed",60},{"tune_amount",70},{"tune_human",90},{"hpf_freq",110},
            {"comp_style",1},{"comp_amount",75},{"level_amount",55},{"eq_body",-1},{"eq_mud",-3},{"eq_mid",1.5f},{"eq_presence",4},{"eq_air",4},
            {"silk_amount",35},{"sat_type",1},{"sat_drive",30},{"sat_mix",50},{"boom",20},
            {"delay_div",3},{"delay_fb",18},{"delay_mix",9},{"rev_size",55},{"rev_mix",12},{"loud",6} } },
        { "Rock", "Grunge Alternativo 90s (estilo Butch Vig)", {
            {"tune_mode",0},{"hpf_freq",100},{"comp_style",2},{"comp_amount",70},{"level_amount",50},
            {"eq_mud",-2},{"eq_mid",2},{"eq_presence",3},{"eq_air",1},{"sat_type",0},{"sat_drive",40},{"sat_tone",50},{"sat_mix",60},
            {"mod_on",1},{"mod_type",0},{"mod_amount",45},{"delay_div",0},{"delay_fb",10},{"delay_mix",6},{"rev_size",35},{"rev_mix",10},{"loud",5} } },
        { "Rock", "Voz Sedosa (estilo Silk Vocal)", {
            {"tune_mode",1},{"tune_speed",60},{"comp_style",3},{"comp_amount",45},{"eq_presence",1},{"eq_air",3},
            {"silk_amount",80},{"silk_blue",0},{"reso_amount",65},{"deess_thresh",-32},{"rev_mix",12},{"loud",4} } },
        { "Rock", "Cerati - Epoca Soda (chorus 80s)", {
            {"tune_mode",1},{"tune_speed",70},{"tune_amount",65},{"tune_human",90},
            {"comp_style",2},{"comp_amount",55},{"eq_body",-1},{"eq_mud",-2},{"eq_mid",2},{"eq_presence",2.5f},{"eq_air",2},
            {"sat_type",0},{"sat_drive",20},{"mod_on",1},{"mod_type",1},{"mod_amount",45},{"mod_rate",0.7f},
            {"delay_div",2},{"delay_fb",30},{"delay_mix",15},{"rev_size",70},{"rev_mix",20},{"rev_predelay",35},{"width",140},{"loud",4} } },
        { "Rock", "Cerati - Bocanada (psicodelico)", {
            {"tune_mode",1},{"tune_speed",80},{"tune_amount",60},{"tune_human",95},
            {"comp_style",3},{"comp_amount",50},{"eq_body",1},{"eq_mud",-1},{"eq_presence",1},{"eq_air",2},
            {"silk_amount",30},{"silk_blue",1},{"sat_type",1},{"sat_drive",25},
            {"mod_on",1},{"mod_type",3},{"mod_amount",40},{"mod_rate",0.25f},
            {"delay_div",4},{"delay_fb",50},{"delay_mix",22},{"delay_tone",25},{"rev_size",88},{"rev_mix",26},{"rev_predelay",70},{"width",160},{"loud",3.5f} } },
        { "Rock", "Cerati - Ahi Vamos (rock crudo)", {
            {"tune_mode",1},{"tune_speed",80},{"tune_amount",55},{"tune_human",95},{"hpf_freq",110},
            {"comp_style",1},{"comp_amount",68},{"eq_mud",-2},{"eq_mid",2.5f},{"eq_presence",3},{"eq_air",1},
            {"sat_type",1},{"sat_drive",45},{"sat_mix",55},{"mod_on",1},{"mod_type",2},{"mod_amount",22},{"mod_rate",0.3f},
            {"delay_div",1},{"delay_fb",22},{"delay_mix",10},{"rev_size",45},{"rev_mix",12},{"loud",5} } },
        { "Rock", "Cerati - Fuerza Natural (acustico calido)", {
            {"tune_mode",1},{"tune_speed",90},{"tune_amount",60},{"tune_human",95},
            {"comp_style",2},{"comp_amount",40},{"level_amount",45},{"eq_body",1.5f},{"eq_mud",-1},{"eq_presence",1},{"eq_air",2.5f},
            {"silk_amount",35},{"silk_blue",1},{"sat_type",0},{"sat_drive",15},{"mod_on",1},{"mod_type",0},{"mod_amount",18},
            {"delay_div",3},{"delay_fb",20},{"delay_mix",9},{"rev_size",60},{"rev_mix",16},{"loud",3} } },
        { "Rock", "Indie Psicodelico (estilo Tame Impala)", {
            {"tune_mode",1},{"tune_speed",50},{"tune_amount",80},
            {"comp_style",3},{"comp_amount",65},{"eq_body",-2},{"eq_mud",-2},{"eq_mid",2},{"eq_air",1},
            {"sat_type",0},{"sat_drive",35},{"sat_tone",40},{"mod_on",1},{"mod_type",3},{"mod_amount",50},{"mod_rate",0.35f},
            {"delay_div",2},{"delay_fb",45},{"delay_mix",20},{"delay_tone",20},{"rev_size",75},{"rev_mix",22},{"width",170},{"loud",4} } },
        { "Rock", "Cerati - Siempre es Hoy (electronico)", {
            {"tune_mode",1},{"tune_speed",30},{"tune_amount",90},{"tune_human",60},{"hpf_freq",120},
            {"comp_style",1},{"comp_amount",60},{"eq_body",-2},{"eq_mud",-2},{"eq_mid",1},{"eq_presence",3},{"eq_air",4},
            {"silk_amount",35},{"sat_type",2},{"sat_drive",25},{"sat_mix",35},
            {"mod_on",1},{"mod_type",2},{"mod_amount",20},{"mod_rate",0.2f},
            {"delay_div",6},{"delay_fb",40},{"delay_mix",16},{"delay_tone",50},{"rev_size",55},{"rev_mix",14},{"width",150},{"loud",5} } },
        { "Rock", "Cerati - Dynamo (voz dentro de la pared)", {
            {"tune_mode",1},{"tune_speed",90},{"tune_amount",55},{"tune_human",95},
            {"comp_style",3},{"comp_amount",60},{"eq_body",-1},{"eq_mud",-1},{"eq_presence",-1},{"eq_air",1},
            {"sat_type",0},{"sat_drive",40},{"sat_tone",35},{"mod_on",1},{"mod_type",1},{"mod_amount",55},{"mod_rate",0.3f},
            {"delay_div",4},{"delay_fb",55},{"delay_mix",24},{"delay_tone",20},{"rev_size",92},{"rev_mix",30},{"rev_predelay",20},{"width",175},{"loud",3} } },
        { "Rock", "Charly - Clics Modernos (80s seco)", {
            {"tune_mode",1},{"tune_speed",80},{"tune_amount",55},{"tune_human",95},{"hpf_freq",110},
            {"comp_style",1},{"comp_amount",62},{"eq_body",-1},{"eq_mud",-2.5f},{"eq_mid",2.5f},{"eq_presence",2.5f},{"eq_air",1.5f},
            {"sat_type",1},{"sat_drive",28},{"mod_on",1},{"mod_type",1},{"mod_amount",18},
            {"delay_div",0},{"delay_fb",15},{"delay_mix",9},{"delay_pp",0},{"rev_size",35},{"rev_damp",30},{"rev_mix",14},{"rev_predelay",5},{"loud",4} } },
        { "Rock", "Charly - Piano Bar (voz al frente)", {
            {"tune_mode",1},{"tune_speed",100},{"tune_amount",50},{"tune_human",100},
            {"comp_style",2},{"comp_amount",50},{"level_amount",50},{"eq_body",1},{"eq_mud",-1.5f},{"eq_mid",1.5f},{"eq_presence",2},{"eq_air",2},
            {"sat_type",0},{"sat_drive",20},{"boom",12},{"delay_div",3},{"delay_fb",18},{"delay_mix",7},
            {"rev_size",55},{"rev_mix",14},{"rev_predelay",30},{"loud",3} } },
        { "Rock", "Charly - Say No More (crudo)", {
            {"tune_mode",0},{"hpf_freq",130},{"comp_style",1},{"comp_amount",75},
            {"eq_body",-2},{"eq_mid",3},{"eq_presence",2},{"eq_air",-1},
            {"sat_type",2},{"sat_drive",50},{"sat_tone",40},{"sat_mix",55},
            {"mod_on",1},{"mod_type",3},{"mod_amount",25},{"mod_rate",0.15f},
            {"delay_div",7},{"delay_fb",35},{"delay_mix",15},{"rev_size",40},{"rev_mix",12},{"loud",4} } },
        { "Rock", "Punk / Pop Punk Doble", {
            {"tune_mode",1},{"tune_speed",40},{"tune_amount",75},{"hpf_freq",130},
            {"comp_style",1},{"comp_amount",78},{"eq_mud",-3},{"eq_mid",2},{"eq_presence",4},{"eq_air",3},
            {"sat_type",1},{"sat_drive",40},{"mod_on",1},{"mod_type",0},{"mod_amount",60},{"delay_on",0},{"rev_size",30},{"rev_mix",8},{"loud",6} } },
        { "Brit", "Liam Gallagher Solista - Al Frente", {
            {"tune_mode",1},{"tune_speed",70},{"tune_amount",65},{"tune_human",95},{"hpf_freq",110},
            {"reso_amount",50},{"comp_style",1},{"comp_amount",68},{"level_amount",50},
            {"eq_body",-1},{"eq_mud",-2.5f},{"eq_mid",2.5f},{"eq_presence",3},{"eq_air",2.5f},
            {"silk_amount",30},{"sat_type",1},{"sat_drive",32},{"sat_mix",50},{"boom",15},
            {"mod_on",1},{"mod_type",0},{"mod_amount",25},
            {"delay_div",3},{"delay_fb",15},{"delay_mix",7},{"rev_size",50},{"rev_mix",11},{"width",125},{"loud",5} } },
        { "Brit", "Liam Gallagher Solista - Fino con Dobles", {
            {"tune_mode",1},{"tune_speed",55},{"tune_amount",80},{"tune_human",85},
            {"reso_amount",60},{"deess_thresh",-30},{"comp_style",2},{"comp_amount",55},{"level_amount",55},
            {"eq_mud",-2},{"eq_mid",1},{"eq_presence",2},{"eq_air",4},{"silk_amount",50},
            {"sat_type",0},{"sat_drive",16},{"mod_on",1},{"mod_type",0},{"mod_amount",45},
            {"delay_div",4},{"delay_fb",25},{"delay_mix",11},{"delay_tone",35},
            {"rev_size",68},{"rev_mix",17},{"rev_predelay",50},{"width",165},{"loud",4.5f} } },
        // ------------------------------------------------ PRODUCTORES / ROCK ARGENTINO / BRIT
        { "Productores", "Hector Castillo - Calido Analogico", {
            {"tune_mode",1},{"tune_speed",90},{"tune_amount",65},{"tune_human",95},{"hpf_freq",75},
            {"reso_amount",40},{"comp_style",2},{"comp_amount",42},{"level_amount",50},{"comp_mix",80},
            {"eq_body",1.5f},{"eq_mud",-1.5f},{"eq_mid",0.5f},{"eq_presence",1.5f},{"eq_air",2.5f},
            {"silk_amount",40},{"silk_blue",1},{"sat_type",0},{"sat_drive",22},{"sat_tone",55},{"sat_mix",45},{"boom",15},
            {"mod_on",1},{"mod_type",0},{"mod_amount",14},
            {"delay_div",4},{"delay_fb",22},{"delay_mix",8},{"delay_tone",30},
            {"rev_size",62},{"rev_damp",60},{"rev_mix",15},{"rev_predelay",45},{"width",125},{"loud",3} } },
        { "Productores", "Hector Castillo - Unplugged Intimo", {
            {"tune_mode",1},{"tune_speed",120},{"tune_amount",55},{"tune_human",100},{"gate_thresh",-75},
            {"comp_style",2},{"comp_amount",35},{"level_amount",45},{"eq_body",1},{"eq_mud",-1},{"eq_presence",1},{"eq_air",2},
            {"silk_amount",30},{"silk_blue",1},{"sat_type",1},{"sat_drive",10},{"boom",8},
            {"delay_on",0},{"rev_size",45},{"rev_damp",65},{"rev_mix",12},{"rev_predelay",30},{"width",105},{"loud",2} } },
        { "Productores", "Tweety Gonzalez - Synth Pop 90s", {
            {"tune_mode",1},{"tune_speed",50},{"tune_amount",80},{"tune_human",80},
            {"comp_style",2},{"comp_amount",55},{"eq_mud",-2},{"eq_mid",1.5f},{"eq_presence",2.5f},{"eq_air",4},{"silk_amount",45},
            {"sat_type",0},{"sat_drive",15},{"mod_on",1},{"mod_type",1},{"mod_amount",38},{"mod_rate",0.55f},
            {"delay_div",2},{"delay_fb",35},{"delay_mix",16},{"delay_tone",40},
            {"rev_size",72},{"rev_mix",19},{"rev_predelay",40},{"width",150},{"loud",4} } },
        { "Productores", "Tweety Gonzalez - Doble Voz Fina", {
            {"tune_mode",1},{"tune_speed",45},{"tune_amount",85},{"tune_human",75},
            {"reso_amount",55},{"comp_style",3},{"comp_amount",50},{"level_amount",50},
            {"eq_mud",-2},{"eq_presence",2},{"eq_air",4.5f},{"silk_amount",55},{"deess_thresh",-31},
            {"sat_drive",12},{"mod_on",1},{"mod_type",0},{"mod_amount",30},
            {"delay_div",4},{"delay_fb",25},{"delay_mix",10},{"rev_size",65},{"rev_mix",16},{"width",160},{"loud",4} } },
    };
    return p;
}
} // namespace nescrun
