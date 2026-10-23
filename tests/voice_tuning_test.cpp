#include "DSP/SynthDSP.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>
using talclone::SynthDSP;
using talclone::VoiceTunings;
void clean(SynthDSP& synth,double rate){synth.setSampleRate(rate);for(auto [id,value]:std::vector<std::pair<int,float>>{{1,.35f},{3,1},{4,0},{6,0},{7,0},{10,0},{11,0},{12,1},{13,0},{14,0},{15,0},{16,0},{18,0},{19,1},{20,0},{21,0},{22,1},{23,.3f},{24,0},{33,.5f},{34,0},{35,0}})synth.setParameter(id,value);}
std::vector<float> render(SynthDSP& synth,int length,bool right=false){std::vector<float> out;out.reserve(static_cast<std::size_t>(length));for(int i=0;i<length;++i){float l,r;synth.processStereo(l,r);assert(std::isfinite(l)&&std::isfinite(r));out.push_back(right?r:l);}return out;}
double energy(const std::vector<float>& data){double sum=0;for(float v:data)sum+=v*v;return sum;}
double pitch(SynthDSP& synth,double rate){render(synth,int(rate*.1));auto data=render(synth,int(rate*.1));std::vector<double> crossing,periods;for(std::size_t i=1;i<data.size();++i)if(data[i-1]<0&&data[i]>=0)crossing.push_back(double(i)-data[i]/(data[i]-data[i-1]));for(std::size_t i=1;i<crossing.size();++i)periods.push_back(crossing[i]-crossing[i-1]);assert(periods.size()>5);std::sort(periods.begin(),periods.end());return 69+12*std::log2(rate/periods[periods.size()/2]/440);}
int main(){
 for(double rate:{8000.,44100.,48000.,96000.}){
    // Explicit neutral settings are exactly the implicit default, including six-voice allocation.
    std::srand(1);SynthDSP baseline;clean(baseline,rate);for(int n:{48,52,55,60,67,72})baseline.noteOn(n,.8f);auto expected=render(baseline,int(rate*.3));
    std::srand(1);SynthDSP neutral;clean(neutral,rate);neutral.setVoiceTuning(VoiceTunings{});for(int n:{48,52,55,60,67,72})neutral.noteOn(n,.8f);auto actual=render(neutral,int(rate*.3));assert(expected==actual);
    // Detuning each allocated voice independently changes only its pitch by the recovered half-semitone range.
    for(int slot=0;slot<6;++slot){SynthDSP synth;clean(synth,rate);VoiceTunings tuning{};tuning[std::size_t(slot)].detune=1;synth.setVoiceTuning(tuning);for(int i=0;i<slot;++i){synth.noteOn(40+i,.8f);synth.noteOff(40+i);render(synth,int(rate*.15));}synth.noteOn(60,.8f);const double measured=pitch(synth,rate);assert(std::abs(measured-60.5)<(rate<10000?.3:.04));}
    for(float pan:{0.f,.5f,1.f}){SynthDSP synth;clean(synth,rate);VoiceTunings tuning{};tuning[0].pan=pan;synth.setVoiceTuning(tuning);synth.noteOn(60,.8f);double lsum=0,rsum=0;for(int i=0;i<int(rate*.5);++i){float l,r;synth.processStereo(l,r);lsum+=l*l;rsum+=r*r;assert(std::isfinite(l)&&std::isfinite(r));if(pan==.5f)assert(l==r);}if(pan==0){assert(lsum>1.e-4&&rsum<lsum*1.e-8);}if(pan==1)assert(rsum>1.e-4&&lsum<rsum*1.e-8);}
    // Shared-filter Save CPU mode must parse the same unscaled cutoff for both channels.
    // Mirror panning at several cutoffs; compare energies, allowing independent filter noise.
    for(float cutoff:{.4f,.65f,1.f}){double mirrored[2];for(int side=0;side<2;++side){std::srand(1);SynthDSP synth;clean(synth,rate);synth.setParameter(38,1);synth.setParameter(12,cutoff);VoiceTunings tuning{};tuning[0].pan=float(side);synth.setVoiceTuning(tuning);synth.noteOn(60,.8f);render(synth,int(rate*.1));mirrored[side]=energy(render(synth,int(rate*.2),side==1));}assert(mirrored[0]>1.e-6);assert(std::abs(mirrored[1]/mirrored[0]-1)<.015);}
    // Attack and release alter stage duration rather than scaling the finished output.
    double early[2],tail[2];for(int extreme=0;extreme<2;++extreme){SynthDSP synth;clean(synth,rate);synth.setParameter(20,.65f);synth.setParameter(23,.65f);VoiceTunings tuning{};tuning[0].attack=float(extreme);tuning[0].release=float(extreme);synth.setVoiceTuning(tuning);synth.noteOn(60,.8f);early[extreme]=energy(render(synth,int(rate*.08)));render(synth,int(rate*1.5));synth.noteOff(60);tail[extreme]=energy(render(synth,int(rate*.4)));}assert(early[0]>early[1]*1.5);assert(tail[1]>tail[0]*1.2);
    double decay[2];for(int extreme=0;extreme<2;++extreme){SynthDSP synth;clean(synth,rate);synth.setParameter(21,.7f);synth.setParameter(22,.1f);VoiceTunings tuning{};tuning[0].decay=float(extreme);synth.setVoiceTuning(tuning);synth.noteOn(60,.8f);render(synth,int(rate*.15));decay[extreme]=energy(render(synth,int(rate*.2)));}assert(decay[1]>decay[0]*1.2);
    double bright[2];for(int extreme=0;extreme<2;++extreme){SynthDSP synth;clean(synth,rate);synth.setParameter(12,.65f);VoiceTunings tuning{};tuning[0].cutoff=float(extreme);synth.setVoiceTuning(tuning);synth.noteOn(60,.8f);render(synth,int(rate*.1));bright[extreme]=energy(render(synth,int(rate*.1)));}assert(bright[1]>bright[0]*2);
    SynthDSP synth;clean(synth,rate);synth.noteOn(60,.8f);render(synth,100);VoiceTunings hostile{};for(auto& v:hostile)v={std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-10,10,-100,100};synth.setVoiceTuning(hostile);synth.setUnison(1,1,1);render(synth,int(rate*.2));synth.allSoundOff(1);render(synth,int(rate*.1));
 }
 std::cout<<"Voice tuning: neutral rendering, six independent pitches, stereo pan, cutoff, A/D/R duration, hostile values and sample rates passed\n";
}
