#pragma once
#include <array>
#include <cstddef>

namespace talclone {
inline constexpr std::size_t parameterCount = 47;
struct ParameterSpec { const char* id; const char* name; float minimum; float maximum; float interval; float defaultValue; };
// Names from getParameterName 0x4e0c; defaults from ToguSynthProgram 0x4ae0.
// TODO verificar: physical units/ranges and semantics of reserved parameters.
// p27 and p28 use raw option indices, as in the original editor, not 0..1.
// TODO verificar: Pulse/PWM manual mode/Save CPU are visual aliases; the original
// host returns literal "Param" for p00, p07, and p36..p46.
inline constexpr std::array<ParameterSpec, parameterCount> parameterSpecs {{
    { "p00", "Reserved 00", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p01", "VOLUME", 0.0f, 1.0f, 0.0f, .5f },
    { "p02", "MASTERPITCH", 0.0f, 1.0f, 0.0f, .5f },
    { "p03", "SAW", 0.0f, 1.0f, 0.0f, 1.0f },
    { "p04", "SUBOSC", 0.0f, 1.0f, 0.0f, 1.0f },
    { "p05", "SUBOSCVOLUME", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p06", "NOISEVOLUME", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p07", "Pulse", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p08", "SQUAREPWMODE", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p09", "SQUAREPWINTENSITY", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p10", "OSCLFOINTENSITY", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p11", "HPCUTOFF", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p12", "CUTOFF", 0.0f, 1.0f, 0.0f, 1.0f },
    { "p13", "RESONANCE", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p14", "FILTERCONTOURBIAS", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p15", "FILTERCONTOUR", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p16", "FILTERLFO", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p17", "KEYFOLLOW", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p18", "ENVGATE", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p19", "ENVVOLUME", 0.0f, 1.0f, 0.0f, .5f },
    { "p20", "AMPATTACK", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p21", "AMPDECAY", 0.0f, 1.0f, 0.0f, 1.0f },
    { "p22", "AMPSUSTAIN", 0.0f, 1.0f, 0.0f, 1.0f },
    { "p23", "AMPRELEASE", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p24", "LFO1RATE", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p25", "LFO1DELAY", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p26", "LFO1TRIGGER", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p27", "LFO1WAVEFORM", 0.0f, 4.0f, 1.0f, 0.0f },
    { "p28", "LFO1CLOCK", 0.0f, 12.0f, 1.0f, 0.0f },
    { "p29", "ENVVELOCITY", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p30", "CUTOFFSMOOTH", 0.0f, 1.0f, 0.0f, 1.0f },
    { "p31", "MODDCO", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p32", "MODVCF", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p33", "TRANSPOSE", 0.0f, 1.0f, 0.0f, .5f },
    { "p34", "CHORUS1", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p35", "CHORUS2", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p36", "PWM manual mode", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p37", "MIDI Mapping Lock", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p38", "Save CPU", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p39", "Reserved 39", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p40", "Reserved 40", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p41", "Reserved 41", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p42", "Reserved 42", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p43", "Reserved 43", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p44", "Reserved 44", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p45", "Reserved 45", 0.0f, 1.0f, 0.0f, 0.0f },
    { "p46", "MIDI learn", 0.0f, 1.0f, 0.0f, 0.0f },
}};
} // namespace talclone
