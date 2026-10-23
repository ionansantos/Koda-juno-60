#include "DSP/SynthDSP.h"
#include "FactoryPresets.h"
#include <cassert>
#include <cmath>
#include <iostream>

using talclone::SynthDSP;
double energy(SynthDSP& synth,int samples=12000) {
    double sum=0;
    for(int i=0;i<samples;++i){float l=0,r=0;synth.processStereo(l,r);
        assert(std::isfinite(l)&&std::isfinite(r));sum+=double(l)*l+double(r)*r;}
    return sum;
}
void silence(SynthDSP& synth) { energy(synth,44100);assert(energy(synth,2048)<1.e-5); }
SynthDSP pad() {
    SynthDSP synth;auto p=talclone::factoryPresets[0].parameters;
    p[18]=0;p[20]=0;p[21]=0.5f;p[22]=1;p[23]=0;p[38]=0;
    synth.setParameters(p);return synth;
}
int main() {
    for(int down:{64,127}) {
        auto synth=pad();assert(synth.handleController(64,down));
        synth.noteOn(69,1);energy(synth,2048);synth.noteOff(69);
        assert(energy(synth)>1.e-3); // Must continue long after the normal release.
        assert(synth.handleController(64,63));silence(synth);
    }
    {
        auto synth=pad();synth.handleController(64,63);synth.noteOn(69,1);
        energy(synth,2048);synth.noteOff(69);silence(synth);
        synth.handleController(64,127);synth.noteOn(69,1);synth.noteOff(69);
        synth.noteOn(69,1);synth.handleController(64,0);
        assert(energy(synth)>1.e-3); // Re-articulated key is still physically down.
        synth.noteOff(69);silence(synth);
    }
    {
        auto synth=pad();synth.handleController(64,127,1);
        synth.noteOn(69,1,1);synth.noteOn(69,1,2);energy(synth,2048);
        synth.noteOff(69,1);synth.handleController(64,0,1);
        assert(energy(synth)>1.e-3); // Same pitch on channel2 remains held.
        synth.noteOff(69,2);silence(synth);
        synth.handleController(64,127,1);synth.noteOn(72,1,1);synth.noteOff(72,1);
        synth.handleController(64,0,2);assert(energy(synth)>1.e-3);
        synth.handleController(64,0,1);silence(synth);
    }
    {
        auto synth=pad();synth.handleController(64,127,1);synth.noteOn(36,1,1);synth.noteOff(36,1);
        for(int n=72;n<78;++n)synth.noteOn(n,1,2); // Steal a pedal-held voice.
        synth.handleController(64,0,1);assert(energy(synth)>1.e-3);
        for(int n=72;n<78;++n)synth.noteOff(n,2);
        silence(synth); // No stale note-off releases or leaves the stolen voice stuck.
    }
    {
        auto synth=pad();synth.handleController(64,127);synth.noteOn(69,1);synth.noteOff(69);
        synth.handleController(121,0);silence(synth);
        synth.handleController(64,127);synth.noteOn(72,1);synth.handleController(121,0);
        assert(energy(synth)>1.e-3);synth.noteOff(72);silence(synth);
        synth.handleController(64,127);synth.noteOn(69,1);synth.handleController(123,0);
        assert(energy(synth)>1.e-3);synth.handleController(64,0);silence(synth);
        synth.handleController(64,127);synth.noteOn(69,1);synth.handleController(120,0);silence(synth);
        synth.reset();synth.noteOn(69,1);synth.noteOff(69);silence(synth);
    }
    {
        auto synth=pad();synth.setParameter(34,1);synth.setParameter(35,1);
        synth.handleController(64,127);synth.noteOn(69,1);energy(synth,2048);synth.noteOn(69,0);
        assert(energy(synth)>1.e-3);synth.handleController(64,0);silence(synth);
        assert(!synth.handleController(64,127,0));assert(!synth.handleController(64,127,17));
        synth.noteOn(200,1);synth.noteOn(69,1,0);synth.noteOff(69,17);silence(synth);
    }
    std::cout<<"Sustain tests passed: thresholds, pedal-up, retrigger, channels, voice stealing, resets and chorus.\n";
}
