#include "PluginProcessor.h"
#include <cassert>
#include <cmath>
#include <iostream>

namespace {
double render(TalCloneProcessor& processor,int blocks=160) {
    double sum=0;juce::AudioBuffer<float> buffer(2,256);
    for(int block=0;block<blocks;++block){juce::MidiBuffer midi;processor.processBlock(buffer,midi);
        for(int c=0;c<2;++c)for(int i=0;i<256;++i){const auto v=buffer.getSample(c,i);assert(std::isfinite(v));sum+=double(v)*v;}}
    return sum;
}
void send(TalCloneProcessor& p,const juce::MidiMessage& message,int offset=0) {
    juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;midi.addEvent(message,offset);p.processBlock(audio,midi);
}
void silent(TalCloneProcessor& p) {render(p);assert(render(p,8)<1.e-5);}
}
int main() {
    juce::ScopedJuceInitialiser_GUI init;
    TalCloneProcessor p;assert(p.getName()=="KODA-JUNO-60");p.prepareToPlay(44100,256);
    // Startup's gate mode gives an audible sustained note independent of ADSR sustain level.
    send(p,juce::MidiMessage::noteOn(1,69,1.f));
    send(p,juce::MidiMessage::controllerEvent(1,64,64),64);
    assert(p.isSustainActive());
    send(p,juce::MidiMessage::noteOff(1,69));assert(render(p)>1.e-3);
    send(p,juce::MidiMessage::controllerEvent(1,64,63),128);silent(p);
    assert(!p.isSustainActive());
    send(p,juce::MidiMessage::controllerEvent(2,64,127));
    send(p,juce::MidiMessage::noteOn(2,72,1.f));
    send(p,juce::MidiMessage::noteOn(2,72,0.f));assert(render(p)>1.e-3);
    send(p,juce::MidiMessage::controllerEvent(1,64,0));assert(render(p)>1.e-3);
    assert(p.isSustainActive());
    send(p,juce::MidiMessage::controllerEvent(2,121,0));silent(p);
    assert(!p.isSustainActive());
    send(p,juce::MidiMessage::controllerEvent(1,64,127));
    send(p,juce::MidiMessage::noteOn(1,69,1.f));send(p,juce::MidiMessage::noteOff(1,69));
    p.requestPanic();silent(p);
    assert(!p.isSustainActive());
    send(p,juce::MidiMessage::noteOn(1,69,1.f));send(p,juce::MidiMessage::noteOff(1,69));silent(p);
    p.releaseResources();
    std::cout<<"Processor MIDI tests passed: CC64, channel isolation, zero-velocity note-off, CC121 and panic.\n";
}
