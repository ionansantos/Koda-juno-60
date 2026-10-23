#include "PluginProcessor.h"
#include <cassert>
#include <cmath>
#include <iostream>
namespace {
float get(TalCloneProcessor& p,const char* id){return p.parameters.getRawParameterValue(id)->load();}
void set(TalCloneProcessor& p,const char* id,float value){auto* parameter=p.parameters.getParameter(id);assert(parameter);parameter->setValueNotifyingHost(parameter->convertTo0to1(value));}
double render(TalCloneProcessor& p,juce::MidiBuffer midi={}){juce::AudioBuffer<float> audio(2,256);p.processBlock(audio,midi);double energy=0;for(int c=0;c<2;++c)for(int i=0;i<256;++i){double v=audio.getSample(c,i);assert(std::isfinite(v));energy+=v*v;}return energy;}
}
int main(){juce::ScopedJuceInitialiser_GUI gui;TalCloneProcessor p;p.prepareToPlay(48000,256);
 set(p,"portamentoMode",2);set(p,"portamentoAmount",.73f);set(p,"portamentoTime",1);set(p,"portamentoPoly",1);set(p,"fxLock",1);
 p.setCurrentProgram(1);assert(get(p,"portamentoMode")<.5f);p.setCurrentProgram(0);assert(get(p,"portamentoMode")>1.5f&&std::abs(get(p,"portamentoAmount")-.73f)<1.e-6);
 juce::MemoryBlock state;p.getStateInformation(state);TalCloneProcessor restored;restored.setStateInformation(state.getData(),int(state.getSize()));assert(get(restored,"portamentoMode")>1.5f&&get(restored,"portamentoTime")>.5f&&get(restored,"portamentoPoly")>.5f);
 auto legacy=p.parameters.copyState();for(const auto& spec:talclone::portamentoSpecs)legacy.removeChild(legacy.getChildWithProperty("id",spec.id),nullptr);
 auto bank=legacy.getChildWithName("PRESET_BANK");legacy.removeChild(bank,nullptr);auto xml=legacy.createXml();juce::MemoryBlock old;juce::AudioProcessor::copyXmlToBinary(*xml,old);restored.setStateInformation(old.getData(),int(old.getSize()));for(const auto& spec:talclone::portamentoSpecs)assert(std::abs(get(restored,spec.id)-spec.defaultValue)<1.e-6);restored.setCurrentProgram(1);assert(get(restored,"portamentoMode")<.5f);
 std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());int attached=0;for(auto* child:editor->getChildren())for(std::size_t i=0;i<talclone::portamentoSpecs.size();++i){const auto& spec=talclone::portamentoSpecs[i];if(child->getName()!=spec.id)continue;
  if(auto* slider=dynamic_cast<juce::Slider*>(child)){const double value=i==0?1:.65;slider->setValue(value,juce::sendNotificationSync);assert(std::abs(get(p,spec.id)-value)<1.e-5);set(p,spec.id,i==0?2:.35f);assert(std::abs(slider->getValue()-(i==0?2:.35))<1.e-5);++attached;}
  if(auto* button=dynamic_cast<juce::TextButton*>(child)){button->setToggleState(false,juce::sendNotificationSync);assert(get(p,spec.id)<.5f);set(p,spec.id,1);assert(button->getToggleState());++attached;}
 }assert(attached==4);
 // Portamento + arpeggiator + sustain must release and PANIC must silence retained voices.
 set(p,"arpEnabled",1);set(p,"arpRate",.6f);set(p,"arpHold",1);set(p,"unisonVoices",1);set(p,"unisonWidth",1);
 juce::MidiBuffer midi;midi.addEvent(juce::MidiMessage::noteOn(2,60,.8f),121);midi.addEvent(juce::MidiMessage::noteOn(2,67,.8f),199);double energy=render(p,midi);for(int i=0;i<300;++i)energy+=render(p);assert(energy>1.e-4);
 midi.clear();midi.addEvent(juce::MidiMessage::controllerEvent(2,64,127),80);midi.addEvent(juce::MidiMessage::noteOff(2,60),125);midi.addEvent(juce::MidiMessage::noteOff(2,67),200);render(p,midi);p.requestPanic();for(int i=0;i<200;++i)render(p);assert(render(p)<1.e-7);
 auto image=editor->createComponentSnapshot(editor->getLocalBounds());juce::File file=juce::File::getCurrentWorkingDirectory().getChildFile("evidence/portamento/implementation/panel.png");file.getParentDirectory().createDirectory();auto stream=file.createOutputStream();assert(stream);stream->setPosition(0);stream->truncate();assert(juce::PNGImageFormat().writeImageToStream(image,*stream));
 std::cout<<"Processor Portamento: presets/FX LOCK separation, session/legacy state, four panel controls, arp/sustain/Unison and PANIC passed\n";
}
