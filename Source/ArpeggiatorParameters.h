#pragma once
#include "Parameters.h"
#include "Arpeggiator.h"
namespace talclone {
enum ArpParameter : std::size_t { arpEnabled,arpSync,arpHold,arpLock,arpExternal,arpMidiClock,arpHost,arpMode,arpRange,arpRate,arpSpecial,arpParameterCount };
inline constexpr std::array<ParameterSpec,arpParameterCount> arpSpecs {{
 {"arpEnabled","Arp On",0,1,1,0},{"arpSync","Arp Sync",0,1,1,0},
 {"arpHold","Arp Hold",0,1,1,0},{"arpLock","Arp Lock",0,1,1,0},
 {"arpExternal","Arp EXT 16",0,1,1,0},{"arpMidiClock","Arp MIDI Clock",0,1,1,0},
 {"arpHost","Arp Host",0,1,1,1},{"arpMode","Arp Direction",0,2,1,2},
 {"arpRange","Arp Octaves",0,2,1,0},{"arpRate","Arp Tempo",0,1,0,.5f},
 {"arpSpecial","Arp Pattern",0,3,1,0}
}};
using ArpValues=std::array<float,arpParameterCount>;
inline Arpeggiator::Settings arpSettings(const ArpValues& v) noexcept {
 return {v[arpEnabled]>=.5f,v[arpSync]>=.5f,v[arpHold]>=.5f,v[arpExternal]>=.5f,v[arpMidiClock]>=.5f,v[arpHost]>=.5f,
 static_cast<int>(v[arpMode]),static_cast<int>(v[arpRange]),static_cast<int>(v[arpSpecial]),v[arpRate]};
}
}
