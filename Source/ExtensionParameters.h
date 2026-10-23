#pragma once
#include "EffectsParameters.h"
#include "PortamentoParameters.h"
#include "VoiceTuningParameters.h"
namespace talclone {
// Append-only host/preset order. Existing effect indices and IDs stay unchanged.
inline constexpr std::size_t voiceTuningParameterOffset=static_cast<std::size_t>(effectParameterCount)+static_cast<std::size_t>(portamentoParameterCount);
inline constexpr std::size_t voiceTuningLockIndex=voiceTuningParameterOffset+voiceTuningParameterCount;
inline constexpr std::size_t extensionParameterCount=voiceTuningLockIndex+1;
inline constexpr auto extensionSpecs=[] {
 std::array<ParameterSpec,extensionParameterCount> specs{};
 for(std::size_t i=0;i<effectParameterCount;++i)specs[i]=effectSpecs[i];
 for(std::size_t i=0;i<portamentoParameterCount;++i)specs[effectParameterCount+i]=portamentoSpecs[i];
 for(std::size_t i=0;i<voiceTuningParameterCount;++i)specs[voiceTuningParameterOffset+i]=voiceTuningSpecs[i];
 specs[voiceTuningLockIndex]={"voiceTuningLock","Voice Tuning Lock",0,1,1,0};
 return specs;
}();
inline bool isGlobalExtension(std::size_t index) noexcept {return index==fxLock||index==voiceTuningLockIndex;}
inline bool isVoiceTuningExtension(std::size_t index) noexcept {return index>=voiceTuningParameterOffset&&index<voiceTuningLockIndex;}
using ExtensionValues=std::array<float,extensionParameterCount>;
inline bool isLockedExtension(std::size_t index) noexcept {return index<effectParameterCount&&isLockedEffect(index);}
}
