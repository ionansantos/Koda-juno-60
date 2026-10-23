#include "PluginProcessor.h"
#include <cassert>
#include <cmath>
#include <iostream>
static float get(TalCloneProcessor& p,const char* id){return p.parameters.getRawParameterValue(id)->load();}
static void set(TalCloneProcessor& p,const char* id,float v){auto* q=p.parameters.getParameter(id);assert(q);q->setValueNotifyingHost(q->convertTo0to1(v));}
int main(){juce::ScopedJuceInitialiser_GUI gui;TalCloneProcessor p;p.prepareToPlay(48000,256);
 set(p,"delayWet",.7f);set(p,"reverbSize",.4f);set(p,"unisonVoices",.75f);p.setCurrentProgram(1);assert(std::abs(get(p,"delayWet"))<1.e-6);p.setCurrentProgram(0);assert(std::abs(get(p,"delayWet")-.7f)<1.e-6);
 set(p,"fxLock",1);p.setCurrentProgram(1);assert(std::abs(get(p,"delayWet")-.7f)<1.e-6&&std::abs(get(p,"unisonVoices"))<1.e-6);assert(std::abs(get(p,"reverbSize")-.4f)<1.e-6);
 juce::MemoryBlock state;p.getStateInformation(state);set(p,"delayWet",0);p.setStateInformation(state.getData(),int(state.getSize()));assert(get(p,"fxLock")>.5f&&std::abs(get(p,"delayWet")-.7f)<1.e-6);
 auto legacy=p.parameters.copyState();for(const auto& spec:talclone::effectSpecs){auto node=legacy.getChildWithProperty("id",spec.id);legacy.removeChild(node,nullptr);}auto xml=legacy.createXml();juce::MemoryBlock old;juce::AudioProcessor::copyXmlToBinary(*xml,old);p.setStateInformation(old.getData(),int(old.getSize()));assert(get(p,"fxLock")<.5f&&std::abs(get(p,"delayWet"))<1.e-6&&std::abs(get(p,"reverbWet"))<1.e-6);
 std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());int attached=0;for(auto* c:editor->getChildren())for(const auto& spec:talclone::effectSpecs)if(c->getName()==spec.id){if(auto* s=dynamic_cast<juce::Slider*>(c)){s->setValue(.73,juce::sendNotificationSync);assert(std::abs(get(p,spec.id)-.73f)<1.e-5);set(p,spec.id,.27f);assert(std::abs(s->getValue()-.27)<1.e-5);++attached;}else if(auto* b=dynamic_cast<juce::TextButton*>(c)){b->setToggleState(true,juce::sendNotificationSync);assert(get(p,spec.id)>.5f);set(p,spec.id,0);assert(!b->getToggleState());++attached;}}assert(attached==14);
 auto image=editor->createComponentSnapshot(editor->getLocalBounds());juce::File file=juce::File::getCurrentWorkingDirectory().getChildFile("evidence/lx-effects/runtime/panel.png");file.getParentDirectory().createDirectory();auto stream=file.createOutputStream();assert(stream);stream->setPosition(0);stream->truncate();assert(juce::PNGImageFormat().writeImageToStream(image,*stream));
 std::cout<<"FX presets, global LOCK, full/legacy state and all 14 panel attachments passed\n";
}
