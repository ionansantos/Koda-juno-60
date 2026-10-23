#include "Arpeggiator.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using A=talclone::Arpeggiator;
namespace {
std::vector<int> play(A& a,int steps) {
    std::vector<int> notes;
    for(int i=0;i<steps;++i){a.trigger();auto e=a.tick();assert(e.count>0);notes.push_back(e.notes[static_cast<std::size_t>(e.count-1)].pitch);}
    return notes;
}
void chord(A& a){a.noteOn(60,1,1);a.noteOn(64,1,.8f);a.noteOn(67,1,.7f);}
}
int main(){
 A a;A::Settings s;s.enabled=true;s.sync=true;s.external=true;a.prepare(48000);a.configure(s);chord(a);
 assert((play(a,4)==std::vector<int>{60,64,67,60}));
 a.reset();s.mode=1;a.configure(s);chord(a);assert((play(a,8)==std::vector<int>{60,64,67,64,60,64,67,64}));
 a.reset();s.mode=0;s.range=2;a.configure(s);chord(a);assert((play(a,9)==std::vector<int>{67,64,60,55,52,48,43,40,36}));
 a.reset();s.mode=2;s.range=1;a.configure(s);chord(a);assert((play(a,6)==std::vector<int>{60,64,67,72,76,79}));
 a.reset();s.range=0;s.hold=true;a.configure(s);chord(a);a.noteOff(60,1);a.noteOff(64,1);a.noteOff(67,1);assert((play(a,3)==std::vector<int>{60,64,67}));
 // A newly played chord replaces a fully released HOLD pattern.
 a.noteOn(62,2,.5f);assert((play(a,2)==std::vector<int>{62,62}));a.noteOff(62,2);s.hold=false;auto release=a.configure(s);assert(release.release);a.trigger();assert(a.tick().count==0);
 // Sustain latches without losing another channel's identity; channel reset removes latched notes.
 a.reset();a.noteOn(60,1,1);a.noteOn(60,2,.5f);a.sustain(1,true);a.noteOff(60,1);a.noteOff(60,2);
 assert(play(a,1)[0]==60);a.sustain(1,false);a.trigger();assert(a.tick().count==0);
 // Owned note-off precedes the next on, and carries the original channel/velocity.
 a.reset();a.noteOn(60,3,.4f);auto on=play(a,1);(void)on;a.trigger();auto e=a.tick();assert(e.release&&e.count==2&&e.notes[0].channel==3&&e.notes[1].velocity==.4f);
 assert(a.noteOff(60,3).release);a.trigger();assert(a.tick().count==0);
 // Recovered special branches: newest-first, random index changes and repeated endpoints.
 a.reset();s.special=1;a.configure(s);chord(a);assert((play(a,3)==std::vector<int>{67,64,60}));
 a.reset();s.special=3;a.configure(s);chord(a);assert((play(a,6)==std::vector<int>{60,64,67,67,64,60}));
 a.reset();s.special=2;a.configure(s);chord(a);auto randomNotes=play(a,30);
 for(std::size_t i=1;i<randomNotes.size();++i)assert(randomNotes[i]!=randomNotes[i-1]);
 s.special=0;
 // Free-rate timing is independent of sample rate/block size, using the recovered exponential curve.
 for(double sr:{44100.,48000.,96000.}){a.prepare(sr);s.sync=false;s.rate=0;a.configure(s);a.noteOn(60,1,1);
  int attacks=0;for(int i=0;i<static_cast<int>(sr*2.5);++i){auto event=a.tick();if(event.count>(event.release?1:0))++attacks;}assert(attacks==3);
 }
 assert(std::abs(A::freeFrequency(0)-1)<1.e-5&&std::abs(A::freeFrequency(1)-25)<1.e-4);
 // MIDI clock emits one 1/16 step per six F8 pulses, and stop silences the owned note.
 a.prepare(48000);s.sync=true;s.external=false;s.midiClock=true;s.rate=9.f/17;a.configure(s);a.noteOn(60,1,1);
 for(int i=0;i<5;++i){a.midiTick();assert(a.tick().count==0);}a.midiTick();assert(a.tick().count==1);assert(a.midiStop().release);
 for(int i=0;i<12;++i){a.midiTick();assert(a.tick().count==0);}a.midiStart();for(int i=0;i<6;++i)a.midiTick();assert(a.tick().count==1);
 // HOST starts the first note immediately, then follows the PPQ grid (observed TAL behavior).
 a.prepare(48000);s.midiClock=false;s.host=true;a.configure(s);a.beginBlock({true,true,120,.125},3000);
 for(int i=0;i<1000;++i)assert(a.tick().count==0);a.noteOn(60,1,1);
 int position=-1,first=-1;for(int i=1000;i<3001;++i)if(a.tick().count){if(first<0)first=i;position=i;}
 assert(first==1000&&position==3000);assert(a.beginBlock({true,false,120,.25},256).release);assert(a.tick().count==0);
 // Out-of-range expansion is omitted; bounded full MIDI capacity remains safe.
 a.prepare(44100);s.external=true;s.range=2;a.configure(s);a.noteOn(127,16,1);assert(play(a,3)==std::vector<int>({127,127,127}));
 a.reset();for(int c=1;c<=16;++c)for(int n=0;n<128;++n)a.noteOn(n,c,1);for(int n:play(a,20))assert(n>=0&&n<=127);
 a.clearChannel(16);a.reset();a.trigger();assert(a.tick().count==0);
 std::cout<<"Arpeggiator: directions, octaves, HOLD, sustain, channels, ownership, free timing, MIDI clock, PPQ, bounds passed.\n";
}
