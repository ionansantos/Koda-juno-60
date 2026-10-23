#include "DSP/SynthDSP.h"
#include "FactoryPresets.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>
using talclone::SynthDSP;
struct Statistics { double energy=0,stereoDifference=0;float peak=0; };
Statistics render(SynthDSP& synth,int samples) {
    Statistics s;
    for(int i=0;i<samples;++i){float l=0,r=0;synth.processStereo(l,r);assert(std::isfinite(l)&&std::isfinite(r));assert(std::abs(l)<20&&std::abs(r)<20);s.energy+=l*l+r*r;s.stereoDifference+=std::abs(l-r);s.peak=std::max(s.peak,std::max(std::abs(l),std::abs(r)));}
    return s;
}
int main() {
    for(double rate:{22050.,44100.,48000.,96000.}){
        SynthDSP synth;synth.setSampleRate(rate);synth.setParameters(talclone::factoryPresets[0].parameters);
        const auto silence=render(synth,256);assert(silence.peak<1.e-6);
        synth.noteOn(69,1);const auto sounding=render(synth,static_cast<int>(rate/4));assert(sounding.energy>0.00001);
        synth.noteOff(69);render(synth,static_cast<int>(rate*2));assert(render(synth,2048).peak<0.005f);
        synth.reset();assert(render(synth,1024).peak<1.e-6f);
    }
    SynthDSP synth;synth.setParameters(talclone::factoryPresets[0].parameters);synth.noteOn(69,1);
    assert(render(synth,8000).stereoDifference==0);
    synth.setParameter(34,1);assert(render(synth,8000).stereoDifference>0.0001);
    // Recovered Unison count mapping, stereo width and reset across host rates.
    assert(SynthDSP::unisonVoiceCount(0)==1);
    assert(SynthDSP::unisonVoiceCount(.01f)==3 && SynthDSP::unisonVoiceCount(1)==7);
    for(double rate:{44100.,48000.,96000.})for(float count:{.01f,.25f,.5f,.75f,1.f}){
        SynthDSP u;u.setSampleRate(rate);u.setParameters(talclone::factoryPresets[0].parameters);u.setParameter(34,0);u.setUnison(count,1,0);u.noteOn(60,.8f);
        auto mono=render(u,4096);assert(mono.energy>0&&mono.stereoDifference==0);
        u.setUnison(count,1,1);assert(render(u,8192).stereoDifference>.001);u.reset();assert(render(u,512).peak<1.e-6f);
    }
    // Factory sweep exercises every recovered program and its unusual dropdown values.
    for(const auto& preset:talclone::factoryPresets){synth.reset();synth.setParameters(preset.parameters);synth.setTempo(137);for(int note:{48,60,67,72,76,84,88})synth.noteOn(note,0.8f);render(synth,4096);synth.pitchWheel(16383);render(synth,512);synth.allNotesOff();render(synth,512);}
    synth.setParameter(27,5);synth.setParameter(28,12);synth.setParameter(13,1);synth.setParameter(38,1);synth.setParameter(24,std::numeric_limits<float>::quiet_NaN());synth.noteOn(127,1);render(synth,20000);
    std::cout << "DSP tests passed: rates, silence, release, chorus stereo, 120 presets, finite output.\n";
}
