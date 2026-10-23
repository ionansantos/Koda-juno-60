#pragma once
#include "ArpeggiatorParameters.h"
#include "ExtensionParameters.h"
#include <array>

namespace talclone {
// Slot 72 is reserved for the first KODA reference preset. The values below
// come from TAL-U-NO-LX 5.1.5 "TAL Presets Bank/Arp/Dream Arp.pjunoxl" and are
// mapped onto KODA's stable 47-slot engine plus its append-only extensions.
// The menu label is intentionally explicit about the source until the original
// "Arp Magic Synth" payload is available.
inline constexpr int referencePresetProgram = 72;
inline constexpr const char* referencePresetName = "Arp Magic Synth";
inline constexpr const char* referencePresetSource = "Dream Arp (TAL-U-NO-LX 5.1.5)";

inline constexpr std::array<float, 47> referencePresetParameters {{
    0.0f,                       // p00 Reserved 00
    0.3880000114440918f,        // p01 VOLUME
    0.5f,                       // p02 MASTERPITCH
    1.0f,                       // p03 SAW
    1.0f,                       // p04 SUBOSC
    0.09600000083446503f,       // p05 SUBOSCVOLUME
    0.02000000141561031f,       // p06 NOISEVOLUME
    0.0f,                       // p07 Pulse
    1.0f,                       // p08 SQUAREPWMODE
    0.7040000557899475f,        // p09 SQUAREPWINTENSITY
    0.0f,                       // p10 OSCLFOINTENSITY
    0.0f,                       // p11 HPCUTOFF
    0.2720000147819519f,        // p12 CUTOFF
    0.6200000047683716f,        // p13 RESONANCE
    0.0f,                       // p14 FILTERCONTOURBIAS (positive envelope)
    0.3640000224113464f,        // p15 FILTERCONTOUR
    0.3880000114440918f,        // p16 FILTERLFO
    0.4160000085830688f,        // p17 KEYFOLLOW
    1.0f,                       // p18 ENVGATE (gate VCA)
    1.0f,                       // p19 ENVVOLUME (neutral; not in the LX payload)
    0.0f,                       // p20 AMPATTACK
    0.3960000276565552f,        // p21 AMPDECAY
    0.5240000486373901f,        // p22 AMPSUSTAIN
    0.6880000233650208f,        // p23 AMPRELEASE
    0.09200000762939453f,       // p24 LFO1RATE
    0.0f,                       // p25 LFO1DELAY
    1.0f,                       // p26 LFO1TRIGGER (LX mode 1)
    0.0f,                       // p27 LFO1WAVEFORM (sine)
    0.0f,                       // p28 LFO1CLOCK (free)
    0.0f,                       // p29 ENVVELOCITY
    1.0f,                       // p30 CUTOFFSMOOTH
    0.0f,                       // p31 MODDCO (LX pitch bend DCO = 0)
    0.2400000095367432f,        // p32 MODVCF (LX pitch bend VCF = .24)
    0.3700000047683716f,        // p33 TRANSPOSE (LX octave transpose)
    1.0f,                       // p34 CHORUS1
    0.0f,                       // p35 CHORUS2
    0.0f,                       // p36 PWM manual mode
    0.0f,                       // p37 MIDI Mapping Lock
    0.0f,                       // p38 Save CPU
    0.0f,                       // p39 Reserved 39
    0.0f,                       // p40 Reserved 40
    0.0f,                       // p41 Reserved 41
    0.0f,                       // p42 Reserved 42
    0.0f,                       // p43 Reserved 43
    0.0f,                       // p44 Reserved 44
    0.0f,                       // p45 Reserved 45
    0.0f,                       // p46 MIDI learn
}};

inline constexpr ArpValues referencePresetArp {{
    1.0f,                       // arpEnabled
    1.0f,                       // arpSync
    0.0f,                       // arpHold
    0.0f,                       // arpLock
    0.0f,                       // arpExternal
    0.0f,                       // arpMidiClock
    0.0f,                       // arpHost
    2.0f,                       // arpMode: UP
    2.0f,                       // arpRange: three octaves
    0.3529411852359772f,        // arpRate
    0.0f,                       // arpSpecial: normal
}};

inline constexpr ExtensionValues referencePresetExtensions {{
    0.0f,                       // unisonVoices
    0.1000000014901161f,        // unisonDetune
    0.0f,                       // unisonWidth
    0.0f,                       // fxLock
    0.0f,                       // delayWet
    0.300000011920929f,         // delayTime
    0.0f,                       // delaySync
    0.5f,                       // delaySpread
    0.5f,                       // delayTone
    0.2000000029802322f,        // delayFeedback
    0.0f,                       // reverbWet
    0.800000011920929f,         // reverbSize
    0.0f,                       // reverbDelay
    0.5f,                       // reverbTone
    0.0f,                       // portamentoMode
    0.0f,                       // portamentoAmount
    0.0f,                       // portamentoTime
    0.0f,                       // portamentoPoly
    // Voice 1
    0.5050914883613586f, 0.4726456999778748f, 0.5564788579940796f,
    0.5406449437141418f, 0.4195012450218201f, 0.5f,
    // Voice 2
    0.5572108030319214f, 0.4417783915996552f, 0.5693966150283813f,
    0.4492763876914978f, 0.5881497263908386f, 0.5f,
    // Voice 3
    0.432081937789917f, 0.4013916552066803f, 0.5894989967346191f,
    0.5093703866004944f, 0.5878881216049194f, 0.5f,
    // Voice 4
    0.4352198839187622f, 0.5404750108718872f, 0.5638844966888428f,
    0.4066056609153748f, 0.4211426973342896f, 0.5f,
    // Voice 5
    0.5453815460205078f, 0.4280411303043365f, 0.4870831370353699f,
    0.4065102338790894f, 0.4175102412700653f, 0.5f,
    // Voice 6
    0.4947457015514374f, 0.590796947479248f, 0.5246718525886536f,
    0.5602437853813171f, 0.4174805879592896f, 0.5f,
    0.0f,                       // voiceTuningLock
}};

inline bool isReferencePreset(int program) noexcept {
    return program == referencePresetProgram;
}
} // namespace talclone
