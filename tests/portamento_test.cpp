#include "DSP/SynthDSP.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>
using talclone::SynthDSP;
void render(SynthDSP& s,double seconds,double rate){for(int i=0;i<int(seconds*rate);++i){float l,r;s.processStereo(l,r);assert(std::isfinite(l)&&std::isfinite(r));}}
double pitch(SynthDSP& s,double rate){std::vector<double> crossings;float last=0;for(int i=0;i<int(.06*rate);++i){float l,r;s.processStereo(l,r);assert(std::isfinite(l)&&std::isfinite(r));if(last<0&&l>=0)crossings.push_back(i-double(l)/(l-last));last=l;}assert(crossings.size()>3);std::vector<double> periods;for(std::size_t i=1;i<crossings.size();++i)periods.push_back(crossings[i]-crossings[i-1]);std::sort(periods.begin(),periods.end());return 69+12*std::log2(rate/periods[periods.size()/2]/440.);}
void clean(SynthDSP& s,double rate){s.setSampleRate(rate);for(auto [id,value]:std::vector<std::pair<int,float>>{{1,.35f},{3,1},{4,0},{6,0},{7,0},{10,0},{11,0},{12,1},{13,0},{14,0},{15,0},{16,0},{18,0},{19,1},{20,0},{21,0},{22,1},{23,0},{24,0},{33,.5f},{34,0},{35,0}})s.setParameter(id,value);}
int main(){for(double rate:{44100.,48000.,96000.}){
 for(int mode:{0,1,2})for(bool overlap:{false,true}){SynthDSP s;clean(s,rate);s.setPortamento(mode,.5f,false,false);s.noteOn(60,.8f);render(s,1,rate);assert(std::abs(pitch(s,rate)-60)<.1);if(!overlap){s.noteOff(60);render(s,.02,rate);}s.noteOn(72,.8f);if(overlap)s.noteOff(60);render(s,.2,rate);double now=pitch(s,rate);assert(std::abs(now-((mode==0||(mode==1&&!overlap))?72:65.7))<.6);s.allNotesOff();render(s,1,rate);}
 for(int interval:{5,12}){SynthDSP s;clean(s,rate);s.setPortamento(1,.5f,true,false);s.noteOn(60,.8f);render(s,1,rate);s.noteOn(60+interval,.8f);render(s,.4,rate);double mid=pitch(s,rate);assert(std::abs(mid-(60+interval*.5375))<.45);render(s,.5,rate);assert(std::abs(pitch(s,rate)-(60+interval))<.1);}
 // Mono last-note priority and smooth return, plus channel-specific sustain cleanup.
 SynthDSP s;clean(s,rate);s.setPortamento(1,.5f,false,false);s.noteOn(60,.8f);render(s,1,rate);s.noteOn(72,.8f);render(s,1,rate);s.noteOff(72);render(s,.2,rate);assert(std::abs(pitch(s,rate)-66.3)<.6);render(s,.5,rate);assert(std::abs(pitch(s,rate)-60)<.1);s.setSustainPedal(true);s.noteOff(60);render(s,.2,rate);assert(std::abs(pitch(s,rate)-60)<.1);s.setSustainPedal(false);render(s,1,rate);float l,r;s.processStereo(l,r);assert(std::abs(l)<1.e-5f);
 // Released voice reuse must ignore unused voices whose default note happens to match64.
 clean(s,rate);s.setPortamento(2,.5f,false,true);s.noteOn(60,.8f);render(s,.8,rate);s.noteOff(60);render(s,.2,rate);s.noteOn(62,.8f);render(s,.8,rate);s.noteOff(62);render(s,.2,rate);s.noteOn(64,.8f);render(s,.02,rate);assert(std::abs(pitch(s,rate)-63.25)<.6);
 // Six simultaneous voices: the oldest voice retains 48 rather than the global last note 67.
 clean(s,rate);s.setPortamento(2,.5f,false,true);for(int n:{48,52,55,60,64,67})s.noteOn(n,.8f);render(s,2,rate);s.noteOn(72,.8f);for(int n:{48,52,55,60,64,67})s.noteOff(n);render(s,.25,rate);assert(std::abs(pitch(s,rate)-55)<.6);
 s.reset();s.setPortamento(2,std::numeric_limits<float>::quiet_NaN(),true,true);s.noteOn(127,1);render(s,.1,rate);s.allSoundOff(1);render(s,.1,rate);
 }
 std::cout<<"Portamento: OFF/I/II, TIME, mono return/sustain, per-voice POLY reuse and sample rates passed\n";
}
