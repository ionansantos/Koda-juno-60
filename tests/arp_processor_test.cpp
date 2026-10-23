#include "PluginProcessor.h"
#include <cassert>
#include <cmath>
#include <iostream>
namespace {
void set(TalCloneProcessor& p,const char* id,float v){auto* q=p.parameters.getParameter(id);assert(q);q->setValueNotifyingHost(q->convertTo0to1(v));}
float get(TalCloneProcessor& p,const char* id){return p.parameters.getRawParameterValue(id)->load();}
double block(TalCloneProcessor& p,juce::MidiBuffer midi={}){
 juce::AudioBuffer<float> audio(2,256);p.processBlock(audio,midi);double energy=0;
 for(int c=0;c<2;++c)for(int i=0;i<256;++i){auto v=audio.getSample(c,i);assert(std::isfinite(v));energy+=v*v;}return energy;
}
void send(TalCloneProcessor& p,const juce::MidiMessage& m,int offset=0){juce::MidiBuffer midi;midi.addEvent(m,offset);block(p,midi);}
double render(TalCloneProcessor& p,int blocks=100){double sum=0;for(int i=0;i<blocks;++i)sum+=block(p);return sum;}
void silent(TalCloneProcessor& p){render(p);assert(render(p,10)<1.e-7);}
}
int main(int argc,char** argv){juce::ScopedJuceInitialiser_GUI gui;TalCloneProcessor p;p.prepareToPlay(48000,256);
 set(p,"arpEnabled",1);set(p,"arpRate",1);set(p,"arpHold",1);
 send(p,juce::MidiMessage::noteOn(1,60,.8f),123);send(p,juce::MidiMessage::noteOff(1,60));assert(render(p)>1.e-3);
 set(p,"arpHold",0);silent(p);
 // EXT16 waits for actual trigger events, including offsets inside an audio block.
 set(p,"arpSync",1);set(p,"arpExternal",1);send(p,juce::MidiMessage::noteOn(2,64,.7f));silent(p);
 send(p,juce::MidiMessage::noteOn(16,1,1.f),200);assert(render(p)>1.e-3);
 send(p,juce::MidiMessage::noteOff(16,1));assert(render(p)>1.e-3);
 send(p,juce::MidiMessage::noteOff(2,64));silent(p);
 // MIDI clock and channel reset; physical sustain must not prevent arp releases.
 set(p,"arpExternal",0);set(p,"arpMidiClock",1);set(p,"arpRate",9.f/17);
 send(p,juce::MidiMessage::noteOn(1,60,1.f));silent(p);send(p,juce::MidiMessage::midiStart());
 for(int i=0;i<6;++i)send(p,juce::MidiMessage::midiClock());assert(render(p)>1.e-3);
 send(p,juce::MidiMessage::midiStop());silent(p);send(p,juce::MidiMessage::controllerEvent(1,123,0));
 set(p,"arpSync",0);set(p,"arpMidiClock",0);set(p,"arpHold",1);
 send(p,juce::MidiMessage::noteOn(1,60,1.f));send(p,juce::MidiMessage::noteOff(1,60));p.requestPanic();silent(p);
 // Bank values are per program, LOCK is global, full host state restores even when locked.
 set(p,"arpRate",.3f);set(p,"arpRange",2);p.setCurrentProgram(1);assert(std::lround(get(p,"arpEnabled"))==0);
 p.setCurrentProgram(0);assert(std::lround(get(p,"arpEnabled"))==1&&std::lround(get(p,"arpRange"))==2&&std::abs(get(p,"arpRate")-.3f)<1.e-6);
 set(p,"arpLock",1);p.setCurrentProgram(2);assert(std::lround(get(p,"arpEnabled"))==1&&std::lround(get(p,"arpRange"))==2);
 juce::MemoryBlock state;p.getStateInformation(state);TalCloneProcessor restored;restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
 assert(std::lround(get(restored,"arpLock"))==1&&std::lround(get(restored,"arpRange"))==2&&restored.getCurrentProgram()==2);
 // A legacy session must not inherit the current arp's enabled/HOLD state.
 auto legacy=p.parameters.copyState();
 for(const auto& spec:talclone::arpSpecs){auto node=legacy.getChildWithProperty("id",spec.id);legacy.removeChild(node,nullptr);}
 auto xml=legacy.createXml();juce::MemoryBlock oldState;juce::AudioProcessor::copyXmlToBinary(*xml,oldState);
 restored.setStateInformation(oldState.getData(),static_cast<int>(oldState.getSize()));assert(std::lround(get(restored,"arpEnabled"))==0&&std::lround(get(restored,"arpHold"))==0);
 // Verify the visible panel attachment in both directions and render the actual editor.
 std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());bool foundHold=false;
 for(auto* c:editor->getChildren())if(auto* b=dynamic_cast<juce::TextButton*>(c))if(b->getButtonText()=="HOLD"){
  b->setToggleState(false,juce::sendNotificationSync);assert(std::lround(get(p,"arpHold"))==0);
  b->setToggleState(true,juce::sendNotificationSync);assert(std::lround(get(p,"arpHold"))==1);foundHold=true;
 }
 assert(foundHold);
 if(argc>1){auto image=editor->createComponentSnapshot(editor->getLocalBounds());juce::File file=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);auto stream=file.createOutputStream();assert(stream);stream->setPosition(0);stream->truncate();assert(juce::PNGImageFormat().writeImageToStream(image,*stream));}
 std::cout<<"Processor arp: rendered audio, HOLD, EXT16, clock, panic, presets/LOCK, legacy state and panel attachment passed.\n";
}
