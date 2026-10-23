#pragma once
#include "Parameters.h"
namespace talclone {
enum EffectParameter : std::size_t { unisonVoices,unisonDetune,unisonWidth,fxLock,delayWet,delayTime,delaySync,delaySpread,delayTone,delayFeedback,reverbWet,reverbSize,reverbDelay,reverbTone,effectParameterCount };
inline constexpr std::array<ParameterSpec,effectParameterCount> effectSpecs {{
 {"unisonVoices","Unison Voices",0,1,0,0},{"unisonDetune","Unison Detune",0,1,0,.1f},{"unisonWidth","Unison Width",0,1,0,0},
 {"fxLock","FX Lock",0,1,1,0},
 {"delayWet","Delay Wet",0,1,0,0},{"delayTime","Delay Time",0,1,0,.5f},{"delaySync","Delay Sync",0,1,1,0},
 {"delaySpread","Delay Spread",0,1,0,.5f},{"delayTone","Delay Tone",0,1,0,.5f},{"delayFeedback","Delay Feedback",0,1,0,0},
 {"reverbWet","Reverb Wet",0,1,0,0},{"reverbSize","Reverb Size",0,1,0,.8f},{"reverbDelay","Reverb Delay",0,1,0,0},{"reverbTone","Reverb Tone",0,1,0,.5f}
}};
using EffectValues=std::array<float,effectParameterCount>;
inline bool isLockedEffect(std::size_t index) noexcept {return index>fxLock;}
} // namespace talclone
