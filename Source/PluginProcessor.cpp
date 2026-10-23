#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ReferencePreset.h"
#include <cmath>
#include <limits>
namespace {
int midiControllerNumber(const juce::var& value) {
    if(!(value.isString()||value.isInt()||value.isInt64()))return -1;
    const auto text=value.toString();if(text.isEmpty())return -1;int number=0;
    for(auto digit:text){if(digit<'0'||digit>'9')return -1;number=number*10+static_cast<int>(digit-'0');if(number>127)return -1;}
    return number;
}
}

juce::AudioProcessorValueTreeState::ParameterLayout TalCloneProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (const auto& spec : talclone::parameterSpecs)
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(spec.id, 1),
            spec.name, juce::NormalisableRange<float>(spec.minimum, spec.maximum, spec.interval),
            spec.defaultValue));
    for (const auto& spec : talclone::arpSpecs)
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(spec.id, 1),
            spec.name, juce::NormalisableRange<float>(spec.minimum,spec.maximum,spec.interval),spec.defaultValue));
    for(const auto& spec:talclone::extensionSpecs)
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(spec.id,1),spec.name,
            juce::NormalisableRange<float>(spec.minimum,spec.maximum,spec.interval),spec.defaultValue));
    return layout;
}

TalCloneProcessor::TalCloneProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "TALCLONE", createParameterLayout()) {
    for (std::size_t program = 0; program < audioValues.size(); ++program)
        for (std::size_t i = 0; i < raw.size(); ++i)
            audioValues[program][i].store(talclone::factoryPresets[program].parameters[i]);
    for (std::size_t i = 0; i < raw.size(); ++i) {
        raw[i] = parameters.getRawParameterValue(talclone::parameterSpecs[i].id);
    }
    for(std::size_t i=0;i<arpRaw.size();++i){
        arpRaw[i]=parameters.getRawParameterValue(talclone::arpSpecs[i].id);
        lastArpValues[i]=talclone::arpSpecs[i].defaultValue;
        for(auto& program:arpPrograms)program[i].store(talclone::arpSpecs[i].defaultValue);
    }
    for(std::size_t i=0;i<extensionRaw.size();++i){
        extensionRaw[i]=parameters.getRawParameterValue(talclone::extensionSpecs[i].id);
        lastExtensionValues[i]=talclone::extensionSpecs[i].defaultValue;
        for(auto& program:extensionPrograms)program[i].store(talclone::extensionSpecs[i].defaultValue);
    }
    // Slot 72 is the KODA reference preset. Keep the immutable table separate
    // from the generated 120-record TAL U-No-60 bank so regeneration cannot
    // shift or overwrite the original programs.
    for(std::size_t i=0;i<raw.size();++i)
        audioValues[talclone::referencePresetProgram][i].store(talclone::referencePresetParameters[i]);
    for(std::size_t i=0;i<arpRaw.size();++i)
        arpPrograms[talclone::referencePresetProgram][i].store(talclone::referencePresetArp[i]);
    for(std::size_t i=0;i<extensionRaw.size();++i)
        extensionPrograms[talclone::referencePresetProgram][i].store(talclone::referencePresetExtensions[i]);
    presets.rename(talclone::referencePresetProgram, talclone::referencePresetName);
    // Original constructor copies its factory bank then loads program 0 (Startup).
    syncProgramParameters(0);
    std::size_t index=0;
    for(auto* parameter:getParameters()){
        auto* ranged=dynamic_cast<juce::RangedAudioParameter*>(parameter);jassert(ranged&&index<hostParameterCount);
        midiParameters[index]=ranged;pendingMidiValues[index].store(std::numeric_limits<float>::quiet_NaN());
        const auto id=ranged->paramID;
        midiLearnable[index]=id!="p37"&&id!="p46"&&id!="arpLock"&&id!="fxLock"&&id!="voiceTuningLock";
        parameter->addListener(this);++index;
    }
    jassert(index==hostParameterCount);resetMidiMappingBanks();midiMappings.select(0,false);
}
TalCloneProcessor::~TalCloneProcessor() { cancelPendingUpdate();for(auto* parameter:midiParameters)if(parameter)parameter->removeListener(this); }
void TalCloneProcessor::prepareToPlay(double rate, int) { synth.setSampleRate(rate); synth.reset(); arpeggiator.prepare(rate); delayEffect.prepare(rate); reverbEffect.prepare(rate); sustainChannels.store(0); }
void TalCloneProcessor::releaseResources() { arpeggiator.reset(); synth.reset(); delayEffect.reset(); reverbEffect.reset(); sustainChannels.store(0); }
bool TalCloneProcessor::isBusesLayoutSupported(const BusesLayout& layout) const {
    return layout.getMainInputChannelSet().isDisabled() &&
           layout.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void TalCloneProcessor::syncProgramParameters(int index) {
    for (std::size_t p = 0; p < raw.size(); ++p) {
        if(globalLegacyParameter(p))continue;
        auto* parameter = parameters.getParameter(talclone::parameterSpecs[p].id);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(audioValues[static_cast<std::size_t>(index)][p].load()));
    }
}
void TalCloneProcessor::applyArpEvents(const talclone::Arpeggiator::Events& events) noexcept {
    for(int i=0;i<events.count;++i){const auto& n=events.notes[static_cast<std::size_t>(i)];
        if(i==0&&events.release)synth.releaseArpeggiatorNote(n.pitch,n.channel);
        else synth.noteOn(n.pitch,n.velocity,n.channel);
    }
}
void TalCloneProcessor::updateArpeggiator() noexcept {
    const auto next=talclone::arpSettings(lastArpValues);
    if(next.enabled!=arpeggiator.settings().enabled){
        // Routing changes release direct or generated voices, without clearing chorus tails.
        for(int c=1;c<=16;++c)synth.allSoundOff(c);
    }
    applyArpEvents(arpeggiator.configure(next));
}
void TalCloneProcessor::updateExtensions() noexcept {
    using namespace talclone;
    constexpr std::size_t offset=effectParameterCount;
    synth.setPortamento(std::isfinite(lastExtensionValues[offset+portamentoMode])?juce::jlimit(0,2,static_cast<int>(std::lround(lastExtensionValues[offset+portamentoMode]))):0,lastExtensionValues[offset+portamentoAmount],lastExtensionValues[offset+portamentoTime]>=.5f,lastExtensionValues[offset+portamentoPoly]>=.5f,lastArpValues[arpEnabled]>=.5f);
    synth.setUnison(lastExtensionValues[unisonVoices],lastExtensionValues[unisonDetune],lastExtensionValues[unisonWidth]);
    delayEffect.setParameters({lastExtensionValues[delayWet],lastExtensionValues[delayTime],lastExtensionValues[delaySpread],lastExtensionValues[delayTone],lastExtensionValues[delayFeedback],lastExtensionValues[delaySync]>=.5f},hostTempo);
    reverbEffect.setParameters({lastExtensionValues[reverbWet],lastExtensionValues[reverbSize],lastExtensionValues[reverbDelay],lastExtensionValues[reverbTone]});
    VoiceTunings tuning{};
    for(std::size_t voice=0;voice<6;++voice){auto& t=tuning[voice];const auto start=voiceTuningParameterOffset+voice*6;
        t={lastExtensionValues[start],lastExtensionValues[start+1],lastExtensionValues[start+2],lastExtensionValues[start+3],lastExtensionValues[start+4],lastExtensionValues[start+5]};}
    synth.setVoiceTuning(tuning);
}
void TalCloneProcessor::captureCurrentProgram() {
    if (pendingProgram.load() >= 0) return;
    const auto program = static_cast<std::size_t>(currentProgram.load());
    for (std::size_t p = 0; p < raw.size(); ++p)if(!globalLegacyParameter(p)){
        const float pending=pendingMidiValues[p].load();audioValues[program][p].store(std::isfinite(pending)?midiParameters[p]->convertFrom0to1(pending):raw[p]->load());
    }
    midiMappings.capture(static_cast<int>(program));
    for(std::size_t i=0;i<arpRaw.size();++i)if(i!=talclone::arpLock){const auto slot=talclone::parameterCount+i;const float pending=pendingMidiValues[slot].load();arpPrograms[program][i].store(std::isfinite(pending)?midiParameters[slot]->convertFrom0to1(pending):arpRaw[i]->load());}
    for(std::size_t i=0;i<extensionRaw.size();++i)if(!talclone::isGlobalExtension(i)){const auto slot=talclone::parameterCount+talclone::arpParameterCount+i;const float pending=pendingMidiValues[slot].load();extensionPrograms[program][i].store(std::isfinite(pending)?midiParameters[slot]->convertFrom0to1(pending):extensionRaw[i]->load());}
}
void TalCloneProcessor::setCurrentProgram(int index) {
    if (index < 0 || index >= getNumPrograms()) return;
    const bool wasSuppressed=suppressMidiSelection.exchange(true);
    const int queued=pendingProgram.load();
    const bool audioAlreadySelected=queued==index;
    if(queued>=0){midiMappings.capture(queued);if(!audioAlreadySelected)pendingProgram.store(-1);}else captureCurrentProgram();
    if(!audioAlreadySelected){for(auto& value:pendingMidiValues)value.store(std::numeric_limits<float>::quiet_NaN());midiMappings.select(index,raw[37]->load()>=.5f);}
    currentProgram.store(index);
    audioMapProgram.store(index);
    syncProgramParameters(index);
    if(arpRaw[talclone::arpLock]->load()<.5f)for(std::size_t i=0;i<arpRaw.size();++i)if(i!=talclone::arpLock){
        auto* p=parameters.getParameter(talclone::arpSpecs[i].id);
        p->setValueNotifyingHost(p->convertTo0to1(arpPrograms[static_cast<std::size_t>(index)][i].load()));
    }
    for(std::size_t i=0;i<extensionRaw.size();++i)if(!talclone::isGlobalExtension(i)&&shouldLoadExtension(i)){
        auto* parameter=parameters.getParameter(talclone::extensionSpecs[i].id);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(extensionPrograms[static_cast<std::size_t>(index)][i].load()));
    }
    midiLearnTarget.store(-1);suppressMidiSelection.store(wasSuppressed);
}
const juce::String TalCloneProcessor::getProgramName(int index) {
    return index >= 0 && index < getNumPrograms() ? juce::String(presets.name(index)) : juce::String{};
}
void TalCloneProcessor::changeProgramName(int index, const juce::String& name) {
    if (index >= 0 && index < getNumPrograms()) presets.rename(index, name.toStdString());
}

bool TalCloneProcessor::isProgramVisible(int index) {
    const auto name=getProgramName(index).trim();
    return name.isNotEmpty()&&!name.equalsIgnoreCase("default");
}
int TalCloneProcessor::adjacentVisibleProgram(int direction) {
    const int current=getCurrentProgram(),count=getNumPrograms();
    for(int step=1;step<=count;++step){const int candidate=(current+(direction<0?-step:step)+count)%count;if(isProgramVisible(candidate))return candidate;}
    return current;
}
bool TalCloneProcessor::selectMidiLearnTarget(const juce::String& id) {
    auto* parameter=parameters.getParameter(id);
    if(!parameter)return false;
    const int index=parameter->getParameterIndex();
    if(index<0||index>=static_cast<int>(hostParameterCount)||!midiLearnable[static_cast<std::size_t>(index)])return false;
    midiLearnTarget.store(index);return true;
}
void TalCloneProcessor::resetCurrentPreset() {
    const bool previous=suppressMidiSelection.exchange(true);
    const auto program=static_cast<std::size_t>(getCurrentProgram());
    pendingProgram.store(-1);
    for(auto& value:pendingMidiValues)value.store(std::numeric_limits<float>::quiet_NaN());
    auto set=[this](const char* id,float value){auto* parameter=parameters.getParameter(id);parameter->setValueNotifyingHost(parameter->convertTo0to1(value));};
    const bool reference=talclone::isReferencePreset(static_cast<int>(program));
    // Explicit reset overrides sound locks, while preserving global switches.
    for(std::size_t i=0;i<raw.size();++i)if(!globalLegacyParameter(i)){
        const float value=reference?talclone::referencePresetParameters[i]:talclone::factoryPresets[program].parameters[i];audioValues[program][i].store(value);set(talclone::parameterSpecs[i].id,value);
    }
    for(std::size_t i=0;i<arpRaw.size();++i)if(i!=talclone::arpLock){const auto& spec=talclone::arpSpecs[i];const float value=reference?talclone::referencePresetArp[i]:spec.defaultValue;arpPrograms[program][i].store(value);set(spec.id,value);}
    for(std::size_t i=0;i<extensionRaw.size();++i)if(!talclone::isGlobalExtension(i)){const auto& spec=talclone::extensionSpecs[i];const float value=reference?talclone::referencePresetExtensions[i]:spec.defaultValue;extensionPrograms[program][i].store(value);set(spec.id,value);}
    set("p46",0);midiLearnTarget.store(-1);midiMappings.clear();midiMappings.capture(static_cast<int>(program));
    keyboardState.allNotesOff(0);pendingPanic.store(true);
    suppressMidiSelection.store(previous);
}

void TalCloneProcessor::handleMidi(const juce::MidiMessage& message) {
    const auto& arp=arpeggiator.settings();
    const bool external=arp.enabled&&arp.sync&&arp.external&&message.getChannel()==16;
    if(external&&(message.isNoteOn()||message.isNoteOff())){if(message.isNoteOn())arpeggiator.trigger();return;}
    if(arp.enabled&&arp.sync&&!arp.external&&arp.midiClock){
        if(message.isMidiClock()){arpeggiator.midiTick();return;}
        if(message.isMidiStart()){applyArpEvents(arpeggiator.midiStart());return;}
        if(message.isMidiContinue()){arpeggiator.midiContinue();return;}
        if(message.isMidiStop()){applyArpEvents(arpeggiator.midiStop());return;}
    }
    if (message.isNoteOn()) {
        if(arp.enabled)applyArpEvents(arpeggiator.noteOn(message.getNoteNumber(),message.getChannel(),message.getFloatVelocity()));
        else synth.noteOn(message.getNoteNumber(),message.getFloatVelocity(),message.getChannel());
    } else if (message.isNoteOff()) {
        if(arp.enabled)applyArpEvents(arpeggiator.noteOff(message.getNoteNumber(),message.getChannel()));
        else synth.noteOff(message.getNoteNumber(),message.getChannel());
    }
    else if (message.isPitchWheel()) synth.pitchWheel(message.getPitchWheelValue());
    else if (message.isProgramChange()) {
        const auto program = message.getProgramChangeNumber();
        if (program < getNumPrograms()) {
            for(auto& value:pendingMidiValues)value.store(std::numeric_limits<float>::quiet_NaN());
            midiMappings.capture(audioMapProgram.load());midiMappings.select(program,raw[37]->load()>=.5f);midiLearnTarget.store(-1);
            // Preserve event sample position; parameter/UI notification is deferred.
            const auto previous = static_cast<std::size_t>(audioMapProgram.load());
            for (std::size_t p = 0; p < raw.size(); ++p) audioValues[previous][p].store(lastDSPValues[p]);
            for (std::size_t p = 0; p < raw.size(); ++p) lastDSPValues[p] = audioValues[static_cast<std::size_t>(program)][p].load();
            synth.setParameters(lastDSPValues);
            for(std::size_t i=0;i<arpRaw.size();++i)if(i!=talclone::arpLock){
                arpPrograms[previous][i].store(lastArpValues[i]);
                if(arpRaw[talclone::arpLock]->load()<.5f)lastArpValues[i]=arpPrograms[static_cast<std::size_t>(program)][i].load();
            }
            for(std::size_t i=0;i<extensionRaw.size();++i)if(!talclone::isGlobalExtension(i)){
                extensionPrograms[previous][i].store(lastExtensionValues[i]);
                if(shouldLoadExtension(i))lastExtensionValues[i]=extensionPrograms[static_cast<std::size_t>(program)][i].load();
            }
            updateExtensions();
            updateArpeggiator();
            audioMapProgram.store(program);
            pendingProgram.store(program);
            currentProgram.store(program);
            triggerAsyncUpdate();
        }
    } else if (message.isController()) {
        const auto cc = message.getControllerNumber();
        if(arp.enabled){
            if(cc==64)applyArpEvents(arpeggiator.sustain(message.getChannel(),message.getControllerValue()>=64));
            else if(cc==121)applyArpEvents(arpeggiator.sustain(message.getChannel(),false));
            else if(cc==120||cc>=123)applyArpEvents(arpeggiator.clearChannel(message.getChannel()));
        }
        // Honor musical sustain/channel actions before applying an explicitly assigned CC mapping.
        if (synth.handleController(cc, message.getControllerValue(), message.getChannel())) {
            const auto bit=static_cast<std::uint16_t>(1u << (message.getChannel()-1));
            if(cc==64 && message.getControllerValue()>=64)sustainChannels.fetch_or(bit);
            else if(cc==64 || cc==121)sustainChannels.fetch_and(static_cast<std::uint16_t>(~bit));
        }
        const int selected=midiLearnTarget.load();
        if(raw[46]->load()>=.5f&&selected>=0&&selected<static_cast<int>(hostParameterCount)&&midiLearnable[static_cast<std::size_t>(selected)])midiMappings.assign(cc,selected);
        const int target=midiMappings.target(cc);
        if(target>=0&&target<static_cast<int>(hostParameterCount))applyMappedParameter(target,static_cast<float>(message.getControllerValue())/127.f);
    }
    // Pitch wheel remains global; note and sustain lifetimes are channel-specific.
}

void TalCloneProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    keyboardState.processNextMidiBuffer(midi, 0, buffer.getNumSamples(), true);
    if (pendingPanic.exchange(false)) { arpeggiator.reset(); synth.reset(); delayEffect.reset(); reverbEffect.reset(); sustainChannels.store(0); }
    buffer.clear();
    if (buffer.getNumChannels() < 2) return;
    std::array<float, 47> values{};
    const auto pending = pendingProgram.load();
    if (pending >= 0) for (std::size_t i = 0; i < values.size(); ++i) values[i] = audioValues[static_cast<std::size_t>(pending)][i].load();
    else for (std::size_t i = 0; i < values.size(); ++i) values[i] = raw[i]->load();
    for (std::size_t i = 0; i < values.size(); ++i) {
        const auto cc = pendingMidiValues[i].load();
        if (std::isfinite(cc)) values[i] = midiParameters[i]->convertFrom0to1(cc);
    }
    synth.setReferenceMode(talclone::isReferencePreset(pending>=0?pending:currentProgram.load()));
    synth.setParameters(values);
    lastDSPValues = values;
    for(std::size_t i=0;i<arpRaw.size();++i)
        lastArpValues[i]=pending>=0&&i!=talclone::arpLock&&arpRaw[talclone::arpLock]->load()<.5f?arpPrograms[static_cast<std::size_t>(pending)][i].load():arpRaw[i]->load();
    for(std::size_t i=0;i<arpRaw.size();++i){const auto index=talclone::parameterCount+i;const float value=pendingMidiValues[index].load();if(std::isfinite(value))lastArpValues[i]=midiParameters[index]->convertFrom0to1(value);}
    updateArpeggiator();
    for(std::size_t i=0;i<extensionRaw.size();++i)
        lastExtensionValues[i]=pending>=0&&!talclone::isGlobalExtension(i)&&shouldLoadExtension(i)?extensionPrograms[static_cast<std::size_t>(pending)][i].load():extensionRaw[i]->load();
    for(std::size_t i=0;i<extensionRaw.size();++i){const auto index=talclone::parameterCount+talclone::arpParameterCount+i;const float value=pendingMidiValues[index].load();if(std::isfinite(value))lastExtensionValues[i]=midiParameters[index]->convertFrom0to1(value);}
    talclone::Arpeggiator::Transport transport;
    if (auto* playHead = getPlayHead())if(const auto position=playHead->getPosition()){
        if(const auto bpm=position->getBpm()){synth.setTempo(*bpm);transport.bpm=*bpm;hostTempo=*bpm;}
        if(const auto ppq=position->getPpqPosition()){transport.ppq=*ppq;transport.valid=true;}
        transport.playing=position->getIsPlaying();
    }
    updateExtensions();
    applyArpEvents(arpeggiator.beginBlock(transport,buffer.getNumSamples()));
    auto event = midi.begin();
    const auto end = midi.end();
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        while (event != end && (*event).samplePosition <= sample) {
            handleMidi((*event).getMessage());
            ++event;
        }
        applyArpEvents(arpeggiator.tick());
        synth.processStereo(left[sample], right[sample]);
        delayEffect.process(left[sample],right[sample]);
        reverbEffect.process(left[sample],right[sample]);
    }
    midi.clear();
}

void TalCloneProcessor::handleAsyncUpdate() {
    const int program = pendingProgram.load();
    if (program >= 0) {
        setCurrentProgram(program);
        int expected = program;
        pendingProgram.compare_exchange_strong(expected, -1);
    }
    const bool wasSuppressed=suppressMidiSelection.exchange(true);
    for(std::size_t i=0;i<pendingMidiValues.size();++i){const float value=pendingMidiValues[i].load();if(!std::isfinite(value))continue;
        midiParameters[i]->setValueNotifyingHost(value);float expected=value;
        pendingMidiValues[i].compare_exchange_strong(expected,std::numeric_limits<float>::quiet_NaN());
    }
    suppressMidiSelection.store(wasSuppressed);
    // TODO verificar concorrência de Program Change/CC simultâneos: notificação
    // assíncrona mantém o DSP por amostra, mas não replica a ordem do host VST2.
}

void TalCloneProcessor::getStateInformation(juce::MemoryBlock& destination) {
    const int queued=pendingProgram.load();
    const int effectiveProgram=queued>=0?queued:currentProgram.load();
    if(queued>=0)midiMappings.capture(effectiveProgram);else captureCurrentProgram();
    auto state = parameters.copyState();
    if(queued>=0){
        const auto program=static_cast<std::size_t>(effectiveProgram);
        auto put=[&](const char* id,float value){auto node=state.getChildWithProperty("id",id);if(node.isValid())node.setProperty("value",value,nullptr);};
        for(std::size_t i=0;i<raw.size();++i)if(!globalLegacyParameter(i))put(talclone::parameterSpecs[i].id,audioValues[program][i].load());
        if(arpRaw[talclone::arpLock]->load()<.5f)for(std::size_t i=0;i<arpRaw.size();++i)if(i!=talclone::arpLock)put(talclone::arpSpecs[i].id,arpPrograms[program][i].load());
        for(std::size_t i=0;i<extensionRaw.size();++i)if(!talclone::isGlobalExtension(i)&&shouldLoadExtension(i))put(talclone::extensionSpecs[i].id,extensionPrograms[program][i].load());
    }
    for(std::size_t i=0;i<pendingMidiValues.size();++i){const float value=pendingMidiValues[i].load();if(std::isfinite(value)){auto node=state.getChildWithProperty("id",midiParameters[i]->paramID);if(node.isValid())node.setProperty("value",midiParameters[i]->convertFrom0to1(value),nullptr);}}
    auto learn=state.getChildWithProperty("id","p46");if(learn.isValid())learn.setProperty("value",0,nullptr);
    state.setProperty("program",effectiveProgram,nullptr);
    juce::ValueTree names("PRESET_BANK");
    for (int i = 0; i < getNumPrograms(); ++i) {
        juce::ValueTree name("NAME");
        name.setProperty("index", i, nullptr);
        name.setProperty("name", getProgramName(i), nullptr);
        for (std::size_t p = 0; p < raw.size(); ++p)if(!globalLegacyParameter(p))
            name.setProperty(talclone::parameterSpecs[p].id, audioValues[static_cast<std::size_t>(i)][p].load(), nullptr);
        for(std::size_t a=0;a<arpRaw.size();++a)if(a!=talclone::arpLock)
            name.setProperty(talclone::arpSpecs[a].id,arpPrograms[static_cast<std::size_t>(i)][a].load(),nullptr);
        for(std::size_t e=0;e<extensionRaw.size();++e)if(!talclone::isGlobalExtension(e))
            name.setProperty(talclone::extensionSpecs[e].id,extensionPrograms[static_cast<std::size_t>(i)][e].load(),nullptr);
        name.addChild(saveMidiMap(i),-1,nullptr);
        names.addChild(name, -1, nullptr);
    }
    state.removeChild(state.getChildWithName("ACTIVE_MIDI_MAP"),nullptr);
    juce::ValueTree active("ACTIVE_MIDI_MAP");for(int cc=0;cc<128;++cc){const int target=midiMappings.target(cc);if(target>=0&&target<static_cast<int>(hostParameterCount)){juce::ValueTree entry("MAP");entry.setProperty("cc",cc,nullptr);entry.setProperty("id",midiParameters[static_cast<std::size_t>(target)]->paramID,nullptr);active.addChild(entry,-1,nullptr);}}
    state.addChild(active,-1,nullptr);
    state.removeChild(state.getChildWithName("PRESET_BANK"), nullptr);
    state.addChild(names, -1, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, destination);
}
void TalCloneProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size)) {
        auto state = juce::ValueTree::fromXml(*xml);
        if (!state.isValid() || state.getType() != parameters.state.getType()) return;
        const int program = juce::jlimit(0, getNumPrograms() - 1, static_cast<int>(state.getProperty("program", 0)));
        for(auto& bank:extensionPrograms)for(std::size_t e=0;e<extensionRaw.size();++e)bank[e].store(talclone::extensionSpecs[e].defaultValue);
        const bool wasSuppressed=suppressMidiSelection.exchange(true);resetMidiMappingBanks();
        const auto names = state.getChildWithName("PRESET_BANK");
        for (int i = 0; i < names.getNumChildren(); ++i) {
            const auto name = names.getChild(i);
            const int index = static_cast<int>(name.getProperty("index", -1));
            if (index < 0 || index >= getNumPrograms()) continue;
            changeProgramName(index, name.getProperty("name").toString());
            const auto map=name.getChildWithName("MIDI_MAP");if(map.isValid())loadMidiMap(index,map);
            for(std::size_t e=0;e<extensionRaw.size();++e){const auto& spec=talclone::extensionSpecs[e];
                const float v=static_cast<float>(name.getProperty(spec.id,spec.defaultValue));
                extensionPrograms[static_cast<std::size_t>(index)][e].store(std::isfinite(v)?juce::jlimit(spec.minimum,spec.maximum,v):spec.defaultValue);
            }
            for(std::size_t a=0;a<arpRaw.size();++a){const auto& spec=talclone::arpSpecs[a];
                const float v=static_cast<float>(name.getProperty(spec.id,spec.defaultValue));
                arpPrograms[static_cast<std::size_t>(index)][a].store(std::isfinite(v)?juce::jlimit(spec.minimum,spec.maximum,v):spec.defaultValue);
            }
            for (std::size_t p = 0; p < raw.size(); ++p) {
                if(globalLegacyParameter(p))continue;
                const auto& spec = talclone::parameterSpecs[p];
                const float value = static_cast<float>(name.getProperty(spec.id, talclone::factoryPresets[static_cast<std::size_t>(index)].parameters[p]));
                if (std::isfinite(value)) audioValues[static_cast<std::size_t>(index)][p].store(juce::jlimit(spec.minimum, spec.maximum, value));
            }
        }
        currentProgram.store(program);
        audioMapProgram.store(program);
        // Old sessions lack arp parameters: load safe defaults instead of retaining ambient values.
        for(const auto& spec:talclone::arpSpecs) {
            auto node=state.getChildWithProperty("id",spec.id);
            if(!node.isValid()){node=juce::ValueTree("PARAM");node.setProperty("id",spec.id,nullptr);state.addChild(node,-1,nullptr);}
            const float v=static_cast<float>(node.getProperty("value",spec.defaultValue));
            node.setProperty("value",std::isfinite(v)?juce::jlimit(spec.minimum,spec.maximum,v):spec.defaultValue,nullptr);
        }
        for(const auto& spec:talclone::extensionSpecs){
            auto node=state.getChildWithProperty("id",spec.id);
            if(!node.isValid()){node=juce::ValueTree("PARAM");node.setProperty("id",spec.id,nullptr);state.addChild(node,-1,nullptr);}
            const float v=static_cast<float>(node.getProperty("value",spec.defaultValue));
            node.setProperty("value",std::isfinite(v)?juce::jlimit(spec.minimum,spec.maximum,v):spec.defaultValue,nullptr);
        }
        auto learn=state.getChildWithProperty("id","p46");if(learn.isValid())learn.setProperty("value",0,nullptr);
        parameters.replaceState(state);
        const auto active=state.getChildWithName("ACTIVE_MIDI_MAP");
        if(active.isValid()){loadMidiMap(program,active);midiMappings.select(program,false);}else midiMappings.select(program,false);
        for(auto& value:pendingMidiValues)value.store(std::numeric_limits<float>::quiet_NaN());
        midiLearnTarget.store(-1);suppressMidiSelection.store(wasSuppressed);
        pendingProgram.store(-1);
        pendingPanic.store(true);
        // TODO verificar chunks VST2 big endian e round-trip no host JUCE real.
    }
}
void TalCloneProcessor::parameterValueChanged(int index,float) {
    if(!suppressMidiSelection.load()&&index>=0&&index<static_cast<int>(hostParameterCount)&&midiLearnable[static_cast<std::size_t>(index)])midiLearnTarget.store(index);
}
void TalCloneProcessor::parameterGestureChanged(int index,bool starting) {if(starting)parameterValueChanged(index,0);}
juce::String TalCloneProcessor::getMidiLearnTarget() const {
    const int index=midiLearnTarget.load();return index>=0&&index<static_cast<int>(hostParameterCount)?midiParameters[static_cast<std::size_t>(index)]->getName(64):juce::String{};
}
bool TalCloneProcessor::shouldLoadExtension(std::size_t index) const noexcept {
    if(talclone::isLockedExtension(index)&&extensionRaw[talclone::fxLock]->load()>=.5f)return false;
    if(talclone::isVoiceTuningExtension(index)&&extensionRaw[talclone::voiceTuningLockIndex]->load()>=.5f)return false;
    return true;
}
void TalCloneProcessor::resetMidiMappingBanks() noexcept {
    for(int program=0;program<120;++program)for(int cc=0;cc<128;++cc){const int target=talclone::factoryPresets[static_cast<std::size_t>(program)].midiMap[static_cast<std::size_t>(cc)];
        midiMappings.setSaved(program,cc,target>0&&target<47&&!globalLegacyParameter(static_cast<std::size_t>(target))?target:-1);
    }
}
void TalCloneProcessor::applyMappedParameter(int index,float normalized) noexcept {
    const auto slot=static_cast<std::size_t>(index);auto* parameter=midiParameters[slot];
    const float domain=parameter->convertFrom0to1(normalized);normalized=parameter->convertTo0to1(domain);
    pendingMidiValues[slot].store(normalized);
    const auto program=static_cast<std::size_t>(audioMapProgram.load());
    if(slot<talclone::parameterCount&&!globalLegacyParameter(slot))audioValues[program][slot].store(domain);
    else if(slot>=talclone::parameterCount&&slot<talclone::parameterCount+talclone::arpParameterCount){const auto i=slot-talclone::parameterCount;if(i!=talclone::arpLock)arpPrograms[program][i].store(domain);}
    else if(slot>=talclone::parameterCount+talclone::arpParameterCount){const auto i=slot-talclone::parameterCount-talclone::arpParameterCount;if(!talclone::isGlobalExtension(i))extensionPrograms[program][i].store(domain);}
    if(slot<talclone::parameterCount){synth.setParameter(index,domain);lastDSPValues[slot]=domain;}
    else if(slot<talclone::parameterCount+talclone::arpParameterCount){lastArpValues[slot-talclone::parameterCount]=domain;updateArpeggiator();}
    else {lastExtensionValues[slot-talclone::parameterCount-talclone::arpParameterCount]=domain;updateExtensions();}
    triggerAsyncUpdate();
}
juce::ValueTree TalCloneProcessor::saveMidiMap(int program) const {
    juce::ValueTree table("MIDI_MAP");
    for(int cc=0;cc<128;++cc){const int target=midiMappings.saved(program,cc);if(target<0||target>=static_cast<int>(hostParameterCount))continue;
        juce::ValueTree entry("MAP");entry.setProperty("cc",cc,nullptr);entry.setProperty("id",midiParameters[static_cast<std::size_t>(target)]->paramID,nullptr);table.addChild(entry,-1,nullptr);
    }
    return table;
}
void TalCloneProcessor::loadMidiMap(int program,const juce::ValueTree& table) {
    for(int cc=0;cc<128;++cc)midiMappings.setSaved(program,cc,-1);
    for(int i=0;i<table.getNumChildren();++i){const auto entry=table.getChild(i);if(!entry.hasType("MAP"))continue;const int cc=midiControllerNumber(entry.getProperty("cc"));if(cc<0)continue;
        auto* parameter=parameters.getParameter(entry.getProperty("id").toString());if(!parameter)continue;
        const int index=parameter->getParameterIndex();if(index>=0&&index<static_cast<int>(hostParameterCount)&&midiLearnable[static_cast<std::size_t>(index)])midiMappings.setSaved(program,cc,index);
    }
}

juce::AudioProcessorEditor* TalCloneProcessor::createEditor() { return new TalCloneEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TalCloneProcessor(); }
