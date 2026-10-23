#pragma once
#include "Parameters.h"
#include <algorithm>
#include <cmath>
namespace talclone {
inline constexpr std::size_t voiceTuningVoiceCount=6, voiceTuningControlsPerVoice=6;
inline constexpr std::size_t voiceTuningParameterCount=voiceTuningVoiceCount*voiceTuningControlsPerVoice;
enum VoiceTuningControl : std::size_t { tuningCutoff,tuningDetune,tuningAttack,tuningDecay,tuningRelease,tuningPan };
struct VoiceTuning { float cutoff=.5f,detune=.5f,attack=.5f,decay=.5f,release=.5f,pan=.5f; };
using VoiceTunings=std::array<VoiceTuning,voiceTuningVoiceCount>;
inline constexpr std::array<const char*,6> voiceTuningControlNames{{"Cutoff","Detune","Attack","Decay","Release","Pan"}};
inline constexpr std::array<ParameterSpec,voiceTuningParameterCount> voiceTuningSpecs {{
 {"vt1Cutoff","Voice 1 Cutoff",0,1,0,.5f},
 {"vt1Detune","Voice 1 Detune",0,1,0,.5f},
 {"vt1Attack","Voice 1 Attack",0,1,0,.5f},
 {"vt1Decay","Voice 1 Decay",0,1,0,.5f},
 {"vt1Release","Voice 1 Release",0,1,0,.5f},
 {"vt1Pan","Voice 1 Pan",0,1,0,.5f},
 {"vt2Cutoff","Voice 2 Cutoff",0,1,0,.5f},
 {"vt2Detune","Voice 2 Detune",0,1,0,.5f},
 {"vt2Attack","Voice 2 Attack",0,1,0,.5f},
 {"vt2Decay","Voice 2 Decay",0,1,0,.5f},
 {"vt2Release","Voice 2 Release",0,1,0,.5f},
 {"vt2Pan","Voice 2 Pan",0,1,0,.5f},
 {"vt3Cutoff","Voice 3 Cutoff",0,1,0,.5f},
 {"vt3Detune","Voice 3 Detune",0,1,0,.5f},
 {"vt3Attack","Voice 3 Attack",0,1,0,.5f},
 {"vt3Decay","Voice 3 Decay",0,1,0,.5f},
 {"vt3Release","Voice 3 Release",0,1,0,.5f},
 {"vt3Pan","Voice 3 Pan",0,1,0,.5f},
 {"vt4Cutoff","Voice 4 Cutoff",0,1,0,.5f},
 {"vt4Detune","Voice 4 Detune",0,1,0,.5f},
 {"vt4Attack","Voice 4 Attack",0,1,0,.5f},
 {"vt4Decay","Voice 4 Decay",0,1,0,.5f},
 {"vt4Release","Voice 4 Release",0,1,0,.5f},
 {"vt4Pan","Voice 4 Pan",0,1,0,.5f},
 {"vt5Cutoff","Voice 5 Cutoff",0,1,0,.5f},
 {"vt5Detune","Voice 5 Detune",0,1,0,.5f},
 {"vt5Attack","Voice 5 Attack",0,1,0,.5f},
 {"vt5Decay","Voice 5 Decay",0,1,0,.5f},
 {"vt5Release","Voice 5 Release",0,1,0,.5f},
 {"vt5Pan","Voice 5 Pan",0,1,0,.5f},
 {"vt6Cutoff","Voice 6 Cutoff",0,1,0,.5f},
 {"vt6Detune","Voice 6 Detune",0,1,0,.5f},
 {"vt6Attack","Voice 6 Attack",0,1,0,.5f},
 {"vt6Decay","Voice 6 Decay",0,1,0,.5f},
 {"vt6Release","Voice 6 Release",0,1,0,.5f},
 {"vt6Pan","Voice 6 Pan",0,1,0,.5f},
}};
inline constexpr bool isLockedVoiceTuningParameter(std::size_t index) noexcept { return index<voiceTuningParameterCount; }
// Observed TAL cubic mapping. Nonfinite host values return to the neutral setting.
inline float sanitizeVoiceTuning(float value) noexcept { return std::isfinite(value)?std::clamp(value,0.f,1.f):.5f; }
inline float voiceTuningOffset(float value) noexcept {const float x=2*sanitizeVoiceTuning(value)-1;return .5f*x*x*x;}
inline float voiceTuningDuration(float value) noexcept {return 1+voiceTuningOffset(value);}
}
