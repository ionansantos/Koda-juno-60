#include "PluginProcessor.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <CoreFoundation/CoreFoundation.h>
namespace {
float get(TalCloneProcessor& p,const char* id){return p.parameters.getRawParameterValue(id)->load();}
void set(TalCloneProcessor& p,const char* id,float v){auto* parameter=p.parameters.getParameter(id);assert(parameter);parameter->setValueNotifyingHost(parameter->convertTo0to1(v));}
void pump(){CFRunLoopRunInMode(kCFRunLoopDefaultMode,.02,false);}
void cc(TalCloneProcessor& p,int number,int value,int channel=1){juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;midi.addEvent(juce::MidiMessage::controllerEvent(channel,number,value),113);p.processBlock(audio,midi);for(int c=0;c<2;++c)for(int i=0;i<256;++i)assert(std::isfinite(audio.getSample(c,i)));pump();}
int index(TalCloneProcessor& p,const char* id){return p.parameters.getParameter(id)->getParameterIndex();}
void learn(TalCloneProcessor& p,const char* id,int controller,float touch=.31f){set(p,id,touch);set(p,"p46",1);cc(p,controller,64);set(p,"p46",0);}
void snapshot(juce::AudioProcessorEditor& editor,const char* name){auto image=editor.createComponentSnapshot(editor.getLocalBounds());auto file=juce::File::getCurrentWorkingDirectory().getChildFile(name);file.getParentDirectory().createDirectory();auto stream=file.createOutputStream();assert(stream);stream->setPosition(0);stream->truncate();assert(juce::PNGImageFormat().writeImageToStream(image,*stream));}
}
int main(){juce::ScopedJuceInitialiser_GUI gui;TalCloneProcessor p;p.prepareToPlay(44100,256);p.clearMidiMappings();
 set(p,"p12",.6f);set(p,"p46",1);cc(p,74,64,16);assert(p.getMidiMapping(74)==index(p,"p12"));assert(std::abs(get(p,"p12")-64.f/127)<1.e-6);assert(get(p,"p46")>.5f);
 set(p,"p13",.2f);cc(p,74,127,2);assert(p.getMidiMapping(74)==index(p,"p13"));assert(get(p,"p13")>.999f);assert(std::abs(get(p,"p12")-64.f/127)<1.e-6);cc(p,75,32);assert(p.getMidiMapping(75)==index(p,"p13"));set(p,"p46",0);cc(p,75,16);assert(std::abs(get(p,"p13")-16.f/127)<1.e-6);
 learn(p,"delayTime",76);cc(p,76,32);assert(std::abs(get(p,"delayTime")-32.f/127)<1.e-6);
 learn(p,"vt1Pan",77);cc(p,77,127);assert(get(p,"vt1Pan")>.999f);learn(p,"portamentoMode",78,1);cc(p,78,127);assert(get(p,"portamentoMode")>1.5f);
 learn(p,"p12",64);cc(p,64,127);assert(p.isSustainActive()&&get(p,"p12")>.999f);cc(p,64,0);assert(!p.isSustainActive()&&get(p,"p12")<.001f);
 // A mapped CC changes the DSP at its event offset, independently of deferred UI notification.
 juce::AudioBuffer<float> eventAudio(2,256);juce::MidiBuffer events;
 {
  auto timing=std::make_unique<TalCloneProcessor>();timing->prepareToPlay(44100,256);timing->clearMidiMappings();learn(*timing,"p01",7);
  for(std::size_t i=0;i<talclone::parameterSpecs.size();++i)if(i!=37&&i!=46)set(*timing,talclone::parameterSpecs[i].id,0);
  for(auto [id,value]:std::array<std::pair<const char*,float>,8>{{{"p02",.5f},{"p03",1},{"p12",1},{"p19",1},{"p22",1},{"p30",1},{"p33",.5f},{"p18",1}}})set(*timing,id,value);
  for(int i=0;i<200;++i){juce::MidiBuffer empty;timing->processBlock(eventAudio,empty);}
  events.addEvent(juce::MidiMessage::noteOn(1,69,.9f),0);events.addEvent(juce::MidiMessage::controllerEvent(1,7,127),128);timing->processBlock(eventAudio,events);
  double before=0,after=0;for(int channel=0;channel<2;++channel)for(int i=0;i<256;++i){const double value=eventAudio.getSample(channel,i);(i<110?before:after)+=value*value;}
  assert(before<1.e-9&&after>1.e-6);pump();
 }
 // Each preset has an independent map; LOCK keeps the active map, CLEAR ignores LOCK.
 p.clearMidiMappings();learn(p,"p12",74);p.setCurrentProgram(1);p.clearMidiMappings();learn(p,"p13",74);p.setCurrentProgram(0);assert(p.getMidiMapping(74)==index(p,"p12"));set(p,"p37",1);p.setCurrentProgram(1);assert(get(p,"p37")>.5f&&p.getMidiMapping(74)==index(p,"p12"));p.clearMidiMappings();for(int c=0;c<128;++c)assert(p.getMidiMapping(c)==-1);learn(p,"vt2Detune",79);assert(p.getMidiMapping(79)==index(p,"vt2Detune"));
 // MIDI Program Change followed by CC in the same block must retain the new program's edit and map.
 set(p,"p37",0);p.setCurrentProgram(0);p.clearMidiMappings();learn(p,"p12",84);p.setCurrentProgram(1);p.clearMidiMappings();learn(p,"p13",84);p.setCurrentProgram(0);
 events.clear();events.addEvent(juce::MidiMessage::programChange(1,1),40);events.addEvent(juce::MidiMessage::controllerEvent(1,84,100),160);p.processBlock(eventAudio,events);
 juce::MemoryBlock heardState;p.getStateInformation(heardState);TalCloneProcessor heard;heard.setStateInformation(heardState.getData(),int(heardState.getSize()));assert(heard.getCurrentProgram()==1&&heard.getMidiMapping(84)==index(heard,"p13")&&std::abs(get(heard,"p13")-100.f/127)<1.e-6);
 pump();assert(p.getCurrentProgram()==1&&p.getMidiMapping(84)==index(p,"p13")&&std::abs(get(p,"p13")-100.f/127)<1.e-6);p.setCurrentProgram(0);events.clear();events.addEvent(juce::MidiMessage::programChange(1,1),44);p.processBlock(eventAudio,events);p.setCurrentProgram(2);juce::MidiBuffer noMidi;p.processBlock(eventAudio,noMidi);pump();assert(p.getCurrentProgram()==2);p.setCurrentProgram(1);learn(p,"vt2Detune",79);
 // VT LOCK is independent of MIDI/FX locks and full restoration overrides ambient locks.
 set(p,"voiceTuningLock",0);set(p,"fxLock",1);set(p,"vt1Detune",.9f);p.setCurrentProgram(2);assert(std::abs(get(p,"vt1Detune")-.5f)<1.e-6);set(p,"voiceTuningLock",1);p.setCurrentProgram(1);assert(std::abs(get(p,"vt1Detune")-.5f)<1.e-6);set(p,"vt1Detune",.2f);
 set(p,"p46",1);juce::MemoryBlock state;p.getStateInformation(state);p.clearMidiMappings();set(p,"p37",1);set(p,"vt1Detune",.8f);p.setStateInformation(state.getData(),int(state.getSize()));assert(std::abs(get(p,"vt1Detune")-.2f)<1.e-6&&get(p,"voiceTuningLock")>.5f&&p.getMidiMapping(79)==index(p,"vt2Detune")&&get(p,"p46")<.5f&&get(p,"p37")<.5f);
 // Old sessions without any tuning/map representation restore neutral VT and factory mappings, not ambient edits.
 auto legacy=p.parameters.copyState();for(const auto& spec:talclone::voiceTuningSpecs)legacy.removeChild(legacy.getChildWithProperty("id",spec.id),nullptr);legacy.removeChild(legacy.getChildWithProperty("id","voiceTuningLock"),nullptr);legacy.removeChild(legacy.getChildWithName("PRESET_BANK"),nullptr);legacy.removeChild(legacy.getChildWithName("ACTIVE_MIDI_MAP"),nullptr);auto oldXml=legacy.createXml();juce::MemoryBlock oldPayload;juce::AudioProcessor::copyXmlToBinary(*oldXml,oldPayload);p.setStateInformation(oldPayload.getData(),int(oldPayload.getSize()));for(const auto& spec:talclone::voiceTuningSpecs)assert(std::abs(get(p,spec.id)-.5f)<1.e-6);assert(get(p,"voiceTuningLock")<.5f&&get(p,"p46")<.5f);
 // A malformed explicit map fails closed for unknown IDs/out-of-range controllers, with omitted entries cleared.
 auto malformed=p.parameters.copyState();juce::ValueTree active("ACTIVE_MIDI_MAP");juce::ValueTree bad("MAP");bad.setProperty("cc",999,nullptr);bad.setProperty("id","p12",nullptr);active.addChild(bad,-1,nullptr);malformed.removeChild(malformed.getChildWithName("ACTIVE_MIDI_MAP"),nullptr);malformed.addChild(active,-1,nullptr);auto xml=malformed.createXml();juce::MemoryBlock payload;juce::AudioProcessor::copyXmlToBinary(*xml,payload);p.setStateInformation(payload.getData(),int(payload.getSize()));for(int c=0;c<128;++c)assert(p.getMidiMapping(c)==-1);
 std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());juce::TextButton* vt=nullptr;juce::TextButton* clear=nullptr;juce::Component* panel=nullptr;
 for(auto* child:editor->getChildren()){if(child->getName()=="voiceTuningPanel")panel=child;if(auto* b=dynamic_cast<juce::TextButton*>(child)){if(b->getName()=="voiceTuning")vt=b;if(b->getName()=="presetClear")clear=b;}}
 assert(vt&&clear&&panel&&!panel->isVisible());vt->triggerClick();pump();assert(panel->isVisible());int knobs=0;for(auto* child:panel->getChildren())if(auto* slider=dynamic_cast<juce::Slider*>(child))if(p.parameters.getParameter(slider->getName())){slider->setValue(.67,juce::sendNotificationSync);assert(std::abs(get(p,slider->getName().toRawUTF8())-.67f)<1.e-5);++knobs;}assert(knobs==36);juce::TextButton* random=nullptr;for(auto* child:panel->getChildren()){if(child->getName()=="vtRandom")random=dynamic_cast<juce::TextButton*>(child);if(child->getName()=="vtRandomAmount")if(auto* slider=dynamic_cast<juce::Slider*>(child))slider->setValue(1,juce::sendNotificationSync);}assert(random);const auto oldPan=get(p,"vt1Pan");random->triggerClick();pump();assert(std::abs(get(p,"vt1Pan")-oldPan)<1.e-6);bool randomized=false;for(const auto& spec:talclone::voiceTuningSpecs){const float value=get(p,spec.id);assert(value>=0&&value<=1);if(juce::String(spec.id).contains("Detune")&&std::abs(value-.67f)>.001f)randomized=true;}assert(randomized);for(auto* child:panel->getChildren())if(child->getName()=="vtRandomAmount")if(auto* slider=dynamic_cast<juce::Slider*>(child))slider->setValue(0,juce::sendNotificationSync);random->triggerClick();pump();for(std::size_t i=0;i<talclone::voiceTuningSpecs.size();++i)assert(std::abs(get(p,talclone::voiceTuningSpecs[i].id)-(i%6==talclone::tuningPan?.67f:.5f))<1.e-6);snapshot(*editor,"evidence/toolbar/implementation/vt-panel.png");
 vt->triggerClick();pump();assert(!panel->isVisible());learn(p,"p12",80);clear->triggerClick();pump();assert(p.getMidiMapping(80)==-1);snapshot(*editor,"evidence/toolbar/implementation/toolbar.png");
 // Click selection must work without changing the fader value.
 juce::Component* cutoff=nullptr;juce::ComboBox* selector=nullptr;
 for(auto* child:editor->getChildren()){if(child->getComponentID()=="p12")cutoff=child;if(child->getName()=="presetSelector")selector=dynamic_cast<juce::ComboBox*>(child);}
 assert(cutoff&&selector);set(p,"p46",1);set(p,"p13",.25f);const float untouched=get(p,"p12");
 const auto now=juce::Time::getCurrentTime();juce::MouseEvent click(juce::Desktop::getInstance().getMainMouseSource(),{10,10},juce::ModifierKeys::leftButtonModifier,1,0,0,0,0,cutoff,cutoff,now,{10,10},now,1,false);
 editor->mouseDown(click);assert(get(p,"p12")==untouched);cc(p,85,99);assert(p.getMidiMapping(85)==index(p,"p12"));assert(std::abs(get(p,"p12")-99.f/127)<1.e-6);
 set(p,"p37",1);set(p,"fxLock",1);set(p,"arpLock",1);set(p,"voiceTuningLock",1);set(p,"delayWet",.9f);set(p,"vt1Detune",.8f);set(p,"arpEnabled",1);
 clear->triggerClick();pump();assert(get(p,"p46")==0&&get(p,"p37")==1&&get(p,"fxLock")==1&&get(p,"arpLock")==1&&get(p,"voiceTuningLock")==1);assert(p.getMidiMapping(85)==-1);
 const float resetCutoff=get(p,"p12");cc(p,85,1);assert(get(p,"p12")==resetCutoff);
 juce::MemoryBlock clearedState;p.getStateInformation(clearedState);TalCloneProcessor restoredClear;restoredClear.setStateInformation(clearedState.getData(),int(clearedState.getSize()));assert(restoredClear.getMidiMapping(85)==-1&&get(restoredClear,"p46")==0);
 assert(get(p,"delayWet")==0&&get(p,"vt1Detune")==.5f&&get(p,"arpEnabled")==0);assert(get(p,"p12")==talclone::factoryPresets[static_cast<std::size_t>(p.getCurrentProgram())].parameters[12]);
 const int resetProgram=p.getCurrentProgram();p.setCurrentProgram(5);p.setCurrentProgram(resetProgram);assert(get(p,"p12")==talclone::factoryPresets[static_cast<std::size_t>(resetProgram)].parameters[12]);set(p,"p37",0);p.setCurrentProgram(5);p.setCurrentProgram(resetProgram);assert(p.getMidiMapping(85)==-1);
 assert(p.getNumPrograms()==120&&selector->getNumItems()==73);p.setCurrentProgram(71);assert(p.adjacentVisibleProgram(1)==72);p.setCurrentProgram(0);assert(p.adjacentVisibleProgram(-1)==72);
 p.changeProgramName(72,"My custom sound");p.setCurrentProgram(71);assert(p.adjacentVisibleProgram(1)==72);for(int i=0;i<8;++i)pump();assert(selector->getNumItems()==73);
 p.setCurrentProgram(100);for(int i=0;i<8;++i)pump();assert(selector->getNumItems()==74&&selector->getSelectedId()==101&&selector->getText()=="101 Init");
 p.setCurrentProgram(0);for(int i=0;i<8;++i)pump();assert(selector->getNumItems()==73);snapshot(*editor,"evidence/preset-midi-061/panel.png");
 std::cout<<"Toolbar: MIDI learn/all channels/reassign/reserved CC/extended parameters, maps/locks/CLEAR, VT preset/full state and36panel controls passed\n";
}
