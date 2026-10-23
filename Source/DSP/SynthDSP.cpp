#include "SynthDSP.h"
#include "BlepTable.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
namespace talclone {
namespace {
constexpr float pi=3.14159265358979323846f;
constexpr float syncMultiplier[]={0,5,2,1,0.5f,0.25f,0.125f,7.5f,3,1.5f,0.75f,0.375f,0.1875f};
float unit(float x) noexcept { return std::clamp(x,0.0f,1.0f); }
float fourth(float x) noexcept { float y=x*x;return y*y; }
float cube(float x) noexcept { return x*x*x; }
float saturate(float x) noexcept {
    const float scaled=x*5.749999763793312e-5f;
    const float a=std::abs(scaled), q=(a+3)*a+6;
    return scaled*q/(q*a+12);
}
}
SynthDSP::SynthDSP() { p[1]=0.4f;p[2]=0.5f;p[3]=1;p[4]=1;p[5]=1;p[12]=1;p[19]=1;p[21]=0.7f;p[24]=0.5f;p[30]=1;p[31]=1;p[32]=1;setSampleRate(44100); }
void SynthDSP::setSampleRate(double rate) {
    sampleRate=std::clamp(std::isfinite(rate)?rate:44100.0,8000.0,384000.0);
    const auto length=static_cast<std::size_t>(std::floor(sampleRate*0.007))+2001;
    for(auto& d:delays)d.samples.assign(length,0);
    reset();
}
void SynthDSP::setTempo(double bpm) noexcept { if(std::isfinite(bpm)&&bpm>0)tempo=bpm; }
void SynthDSP::setParameters(const Parameters& values) noexcept { for(int i=0;i<47;++i)setParameter(i,values[static_cast<std::size_t>(i)]); }
void SynthDSP::setParameter(int i,float v) noexcept {
    if(i<0||i>=47||!std::isfinite(v))return;
    p[static_cast<std::size_t>(i)]=i==27?std::clamp(v,0.f,5.f):i==28?std::clamp(v,0.f,12.f):unit(v);
}
void SynthDSP::reset() noexcept {
    voices={};heldNotes={};noteOrder=0;sustainPedals={};sharedFilter={};sharedRightFilter={};decimator={};rightDecimator={};highpass={};rightHighpass={};chorusHighpass={};nextVoice=5;
    syncedFrequency=0;bend=0;lfoPhase=0;pwmPhase=0;heldRandom=previousRandom=filterSmooth=lfoFade=0;lfoDelay=0;
    for(std::size_t i=0;i<delays.size();++i){auto& d=delays[i];std::fill(d.samples.begin(),d.samples.end(),0);d.cursor=d.samples.empty()?0:d.samples.size()-1;d.ramp=(i==0||i==3)?1.f:-1.f;d.direction=d.ramp;d.last=d.lowpass=0;}
}
void SynthDSP::allNotesOff(int channel) noexcept {
    for(int ch=1;ch<=16;++ch)if(channel==0||ch==channel)for(int n=0;n<128;++n){auto& key=heldNotes[static_cast<std::size_t>((ch-1)*128+n)];key.down=false;if(!sustainPedals[static_cast<std::size_t>(ch-1)])key.order=0;}
    for(auto& v:voices)if(!monoPortamento()&&(channel==0||v.channel==channel)){v.keyDown=false;if(!sustainPedals[static_cast<std::size_t>(v.channel-1)])v.gate=false;}
    if(monoPortamento())restoreMonoNote();
}
void SynthDSP::allSoundOff(int channel) noexcept {
    if(channel<1||channel>16)return;
    for(int n=0;n<128;++n)heldNotes[static_cast<std::size_t>((channel-1)*128+n)]={};
    for(auto& v:voices)if(v.channel==channel)v=Voice{};
    if(monoPortamento())restoreMonoNote();
}
void SynthDSP::setSustainPedal(bool down,int channel) noexcept {
    if(channel<1||channel>16)return;
    sustainPedals[static_cast<std::size_t>(channel-1)]=down;
    if(!down){for(int n=0;n<128;++n){auto& key=heldNotes[static_cast<std::size_t>((channel-1)*128+n)];if(!key.down)key.order=0;}
        for(auto& v:voices)if(!monoPortamento()&&v.channel==channel&&!v.keyDown)v.gate=false;
        if(monoPortamento())restoreMonoNote();}
}
bool SynthDSP::handleController(int controller,int value,int channel) noexcept {
    if(channel<1||channel>16)return false;
    switch(controller){
        case 64:setSustainPedal(value>=64,channel);return true;
        case 120:allSoundOff(channel);return true;
        case 121:setSustainPedal(false,channel);pitchWheel(8192);return true;
        case 123:case 124:case 125:case 126:case 127:allNotesOff(channel);return true;
        case 122:return true; // Local Control belongs to the hardware controller.
        default:return false;
    }
}
int SynthDSP::unisonVoiceCount(float v) noexcept {
    if(!std::isfinite(v)||v<=.001f)return 1;
    return static_cast<int>(4*std::clamp(v,0.f,1.f)+3.5f);
}
void SynthDSP::setUnison(float count,float detune,float width) noexcept {
    const int extra=unisonVoiceCount(count)-1;
    const float w=std::isfinite(width)?unit(width):0;
    if((extra>0&&w>0)&&!(unisonExtra>0&&unisonWidthValue>0)){
        if(!stereoEnabled())copyStereoState();else for(auto& v:voices)v.rightLowpass=v.lowpass;
    }
    unisonExtra=extra;unisonWidthValue=w;
    detune=std::isfinite(detune)?unit(detune):.1f;
    if(detune!=unisonDetuneValue){
        unisonDetuneValue=detune;
        for(std::size_t i=0;i<unisonRatios.size();++i){
            const int magnitude=static_cast<int>(i/2)*2+1;
            const float cents=detune*detune*static_cast<float>(i%2==0?magnitude:-magnitude);
            unisonRatios[i]=std::exp2(cents/12.f);
        }
    }
}
void SynthDSP::copyStereoState() noexcept {
    for(auto& v:voices)v.rightLowpass=v.lowpass;
    rightDecimator=decimator;rightHighpass=highpass;sharedRightFilter=sharedFilter;
}
void SynthDSP::setVoiceTuning(const VoiceTunings& tuning) noexcept {
    const bool wasStereo=stereoEnabled();tuningStereo=false;
    for(std::size_t i=0;i<voiceTuning.size();++i){
        const auto& input=tuning[i];auto& output=voiceTuning[i];
        output.cutoff=voiceTuningOffset(input.cutoff);output.detune=voiceTuningOffset(input.detune);
        output.attack=voiceTuningDuration(input.attack);output.decay=voiceTuningDuration(input.decay);output.release=voiceTuningDuration(input.release);
        const float pan=sanitizeVoiceTuning(input.pan);output.left=(1-pan*pan)*1.3333334f;output.right=(1-(1-pan)*(1-pan))*1.3333334f;
        tuningStereo=tuningStereo||pan!=.5f;
    }
    if(!wasStereo&&stereoEnabled())copyStereoState();
}
void SynthDSP::setPortamento(int mode,float amount,bool fixedTime,bool poly,bool retrigger) noexcept {
    mode=std::clamp(mode,0,2);
    const bool nextMono=mode>0&&!poly;
    if(nextMono!=monoPortamento()){
        // Changing voice allocation ends the old routing, retaining each voice's pitch memory.
        heldNotes={};for(auto& v:voices){v.gate=v.keyDown=false;v.amp.level=v.gateEnvelope.level=0;}
    }
    portamentoMode=mode;portamentoAmount=std::isfinite(amount)?unit(amount):0;
    portamentoTime=fixedTime;portamentoPoly=poly;arpRetrigger=retrigger;
}
void SynthDSP::startVoice(Voice& v,int note,float velocity,int channel,bool monoLegato) noexcept {
    const bool wasGated=v.gate;
    const bool active=v.gate||v.amp.level>0||v.gateEnvelope.level>0;
    if(portamentoMode==0||(portamentoMode==1&&!wasGated))v.pitch=static_cast<float>(note);
    if(portamentoMode>0){
        const float a=.11f-.1f*portamentoAmount,square=a*a;
        const float factor=portamentoTime?std::abs(v.pitch-static_cast<float>(note))*.05f:1.f;
        v.glideStep=factor*(square*square*square*10000.f+.0001f)/static_cast<float>(sampleRate*2.2675737e-05);
    }
    const bool retrigger=portamentoMode==0||!wasGated||!monoLegato||arpRetrigger;
    v.note=note;v.channel=channel;v.keyDown=true;v.gate=true;v.tail=0;
    if(retrigger){
        v.velocity=unit(velocity);v.amp.decay=v.filter.decay=v.gateEnvelope.decay=false;v.filter.level=0;
        if(portamentoMode>0)v.amp.level=v.gateEnvelope.level=0;
    }
    if(portamentoMode==0||!active){
        auto start=[](OscillatorStack& osc){osc.saw.phase=0;osc.pulse.phase=osc.pulse.edge=.1f;osc.sub.phase=osc.sub.edge=.25f;};
        start(v.main);for(auto& osc:v.extra)start(osc);
    }
    if(p[26]==0)lfoPhase=0;
    lfoFade=0;lfoDelay=0;
    if(p[28]!=0){lfoPhase=0;syncedFrequency=static_cast<float>(tempo/60)*syncMultiplier[static_cast<std::size_t>(static_cast<int>(p[28]))];}
}
void SynthDSP::restoreMonoNote() noexcept {
    std::size_t newest=heldNotes.size();std::uint64_t order=0;
    for(std::size_t i=0;i<heldNotes.size();++i)if(heldNotes[i].order>order){order=heldNotes[i].order;newest=i;}
    auto& v=voices[0];
    if(newest==heldNotes.size()){v.gate=v.keyDown=false;return;}
    const int note=static_cast<int>(newest%128),channel=static_cast<int>(newest/128)+1;
    if(v.note!=note||v.channel!=channel||!v.gate)startVoice(v,note,heldNotes[newest].velocity,channel,true);
    v.keyDown=heldNotes[newest].down;
}
void SynthDSP::noteOn(int note,float velocity,int channel) noexcept {
    if(note<0||note>127||channel<1||channel>16||!std::isfinite(velocity))return;
    if(velocity<=0){noteOff(note,channel);return;}
    if(monoPortamento()){
        auto& key=heldNotes[static_cast<std::size_t>((channel-1)*128+note)];key={++noteOrder,unit(velocity),true};
        nextVoice=0;startVoice(voices[0],note,velocity,channel,true);return;
    }
    int index=-1;
    for(int i=0;i<6;++i)if(voices[static_cast<std::size_t>(i)].note==note&&voices[static_cast<std::size_t>(i)].channel==channel&&(portamentoMode==0||voices[static_cast<std::size_t>(i)].gate||voices[static_cast<std::size_t>(i)].amp.level>0||voices[static_cast<std::size_t>(i)].gateEnvelope.level>0)){index=i;break;}
    if(index<0){
        if(portamentoMode>0){
            for(int i=0;i<6;++i){const auto& v=voices[static_cast<std::size_t>(i)];if(!v.gate&&v.amp.level<=0&&v.gateEnvelope.level<=0){index=i;break;}}
            if(index<0){index=0;for(int i=1;i<6;++i)if(voices[static_cast<std::size_t>(i)].tail>voices[static_cast<std::size_t>(index)].tail)index=i;}
        }else for(int count=0;count<6;++count){nextVoice=(nextVoice+1)%6;index=nextVoice;if(!voices[static_cast<std::size_t>(index)].gate)break;}
    }
    nextVoice=index;startVoice(voices[static_cast<std::size_t>(index)],note,velocity,channel,false);
}
void SynthDSP::noteOff(int note,int channel) noexcept {
    if(note<0||note>127||channel<1||channel>16)return;
    if(monoPortamento()){
        auto& key=heldNotes[static_cast<std::size_t>((channel-1)*128+note)];key.down=false;
        if(!sustainPedals[static_cast<std::size_t>(channel-1)])key.order=0;
        restoreMonoNote();return;
    }
    for(auto& v:voices)if(v.note==note&&v.channel==channel){v.keyDown=false;if(!sustainPedals[static_cast<std::size_t>(channel-1)])v.gate=false;}
}
void SynthDSP::releaseArpeggiatorNote(int note,int channel) noexcept {
    // Generated notes must end even while the physical sustain pedal is down.
    if(note<0||note>127||channel<1||channel>16)return;
    if(monoPortamento()){heldNotes[static_cast<std::size_t>((channel-1)*128+note)]={};restoreMonoNote();}
    else for(auto& v:voices)if(v.note==note&&v.channel==channel){v.keyDown=false;v.gate=false;}
}
void SynthDSP::pitchWheel(int value) noexcept { bend=(static_cast<float>((std::clamp(value,0,16383)>>7)-64))/64.f; }
float SynthDSP::random() noexcept {
    // Original uses process-global libc rand() with binary constant 2^-31.
    // TODO verificar: libc implementation, initial seed and cross-plugin shared random state differ by host.
    return static_cast<float>(std::rand())*4.656612873077393e-10f;
}
void SynthDSP::Oscillator::addStep(float offset,float sign) noexcept {
    // TODO verificar: saw path in original has no table bound check; safe reconstruction clips at 2048.
    for(int i=0;i<66;++i){int tableIndex=static_cast<int>((i+offset)*32.f+0.5f);if(tableIndex>=0&&tableIndex<2048)correction[static_cast<std::size_t>((cursor+i)%66)]+=sign*(1.f-blepTable[static_cast<std::size_t>(tableIndex)]);}
}
float SynthDSP::Oscillator::tick(float frequency,float width,float rate,bool isSaw) noexcept {
    const float increment=frequency/rate;phase+=increment;
    if(!isSaw)edge+=increment;
    if(phase>=1){phase-=1;addStep(phase/increment,1);}
    if(!isSaw){const float threshold=width*0.8f+0.1f;if(edge>=threshold){const float overshoot=std::max(0.f,edge-threshold);edge-=1;addStep(overshoot/increment,-1);}}
    correction[static_cast<std::size_t>(cursor)]=0;cursor=(cursor+1)%66;
    const float c=correction[static_cast<std::size_t>(cursor)];
    return isSaw?c+phase-0.5f:c+phase-edge-0.5f;
}
float SynthDSP::Envelope::tick(bool gate,float attack,float decayRate,float sustain,float release,float rate,bool instantAttack,float attackFactor,float decayFactor,float releaseFactor) noexcept {
    const float scale=150.f/rate;
    if(!decay){const float a=(1-attack)*0.7f+0.22f;level+=fourth(a)*(scale/attackFactor)*(level+0.2f);
        if(attack!=0){if(level>1||!gate)decay=true;}
        else if(level>=1||instantAttack){decay=true;level=1;}else level+=0.02f;
    }else{
        if(!gate){if(!instantAttack||release!=0)if(level>0)level-=fourth(1-release)*(scale/releaseFactor)*(level+0.05f);}
        else if(level>sustain)level-=fourth(1-decayRate)*(scale/decayFactor)*(fourth(level)+0.2f);
        if(level<0.005f)level=0;
    }
    return level;
}
void SynthDSP::Filter::update(float cutoff,float rate) noexcept {
    const float x=cutoff*std::min(1.f,44100.f/rate)*0.5f;
    normalizedCutoff=x;
    const float x2=x*x;
    const float correction=1.873f*x*x2+0.4955f*x2-0.649f*x+0.9988f;
    resonanceCorrection=-3.5364f*x2+1.8409f*x+0.9968f;
    const float z=-pi*correction*x,z2=z*z,z3=z2*z,z4=z3*z;
    coefficient=(1-(1+z+0.5f*z2+z3/6.f+z4/24.f+z4*z/120.f))*40000.f;
}
float SynthDSP::Filter::tick(float input,float resonance,float randomValue) noexcept {
    // Static translation of Filter3 0x24d3e. Eight nonlinear stages, delayed feedback.
    // TODO verificar: per-instance +0x1c amplitude state is updated but never used by observed process.
    float randomScale=randomValue*0.2f+0.8f;
    if (normalizedCutoff>=0.4f) randomScale*=1.f+(0.4f-normalizedCutoff)*5.f;
    float signal=input-2.6f*resonance*randomScale*output*resonanceCorrection;
    for(std::size_t i=0;i<stage.size();++i){auto& s=stage[i];const float next=s+coefficient*(saturate(signal)-saturate(s))+(i==0?1.e-6f:0.f);s=next;signal=next;}
    output=delay*0.35f;delay=stage[7];
    if(output>0){output=2.5f*output-cube(output)/6.f;output+=cube(output)*-0.16686666f;}
    else{output=2.5f*output+cube(output)*-0.14666627f;output+=cube(output)*-0.15666626f;}
    // TODO verificar: finite-state recovery protects hosts, not present in original pseudocode.
    if(!std::isfinite(output)||std::abs(output)>100){stage={};output=delay=0;}
    return output;
}
float SynthDSP::Highpass::tick(float value,float coefficient) noexcept { output=(value-input)+coefficient*output;input=value+2.3283064365386963e-10f;return output; }
float SynthDSP::lfo(int waveform,float phase) noexcept {
    // Lfo::tick linearly interpolates 257-point tables. Branch byte evidence resolves shapes.
    if(waveform>=4)return waveform==4?heldRandom:previousRandom;
    const float tablePhase=phase*256.f;const int index=static_cast<int>(std::floor(tablePhase));const float blend=tablePhase-index;
    auto point=[&](int i){switch(waveform){
        case 1:return std::sin(static_cast<float>(i)*0.00390625f*6.2831854820251465f);
        case 0:return i<64?i/64.f:i<128?(128-i)/64.f:i<192?-(i-128)/64.f:(i-256)/64.f;
        case 2:return i==256?-1.f:i/128.f-1.f;
        default:return i<128||i==256?1.f:-1.f;
    }};
    return (1-blend)*point(index)+blend*point(index+1);
}
float SynthDSP::cutoffForVoice(Voice& v,float envelope,float modulation) noexcept {
    // The LX reference preset stores a direct normalized cutoff; the original
    // U-No-60 path uses the cubed value recovered from its i386 engine.
    float cutoff=std::max(0.f,referenceMode?filterSmooth:cube(filterSmooth));
    if(p[17]>0){const float semitone=static_cast<float>(std::log(cutoff*static_cast<float>(sampleRate)*0.5f/440.f+1.e-10f)*17.312339782714844);
        const float tracked=440.f*std::pow(2.f,(semitone+v.note+24.f-81.f)/12.f)*p[17]/(0.5f*static_cast<float>(sampleRate));cutoff=cutoff*(1-p[17])+tracked*p[17];}
    cutoff+=0.5f*cube(p[16])*modulation;
    const float amount=p[15]+p[29]*v.velocity;
    const float signedEnv=(p[14]!=0?-1.f:1.f)*envelope*amount*amount;
    cutoff=unit(cutoff+cube(signedEnv)+tuningForVoice(v).cutoff);
    v.drift=((random()-0.5f)+29.f*v.drift)/30.f;
    return unit(cutoff+fourth(cutoff)*v.drift);
}
SynthDSP::StereoOversample SynthDSP::renderVoice(Voice& v,float modulation,float pwm) noexcept {
    StereoOversample out;
    if(!v.gate&&v.amp.level<=0&&v.gateEnvelope.level<=0){if(v.tail<std::numeric_limits<int>::max())++v.tail;return out;}
    const float rate=static_cast<float>(sampleRate);
    const auto& tuning=tuningForVoice(v);
    const float ampEnv=v.amp.tick(v.gate,p[20],p[21],p[22],p[18]!=0?0:p[23],rate,false,tuning.attack,tuning.decay,tuning.release);
    const float fEnv=v.filter.tick(v.gate,p[20],p[21],p[22],p[23],rate,true,tuning.attack,tuning.decay,tuning.release);
    const float target=static_cast<float>(v.note);
    if(portamentoMode==0)v.pitch=target;
    else if(v.pitch<target)v.pitch=std::min(v.pitch+v.glideStep,target);
    else if(v.pitch>target)v.pitch=std::max(v.pitch-v.glideStep,target);
    if(v.tail<std::numeric_limits<int>::max())++v.tail;
    float pitch=v.pitch+tuning.detune+2*(p[2]-0.5f)+bend*p[31]*12;
    pitch+=p[33]<0.25f?-12:p[33]>0.5f?12:0;
    pitch=std::max(0.f,pitch+modulation*cube(p[10])*12);
    const float frequency=std::min(19000.f,440.f*std::pow(2.f,(pitch-81.f)/12.f));
    const float width=p[36]!=0?0.5f+0.5f*p[9]:p[8]!=0?std::min(1.f,ampEnv)*p[9]:p[9]>0?0.5f+0.5f*pwm*p[9]:0.48f;
    auto oscSample=[&](OscillatorStack& osc,float hz){
        float value=0;
        if(p[3]!=0)value+=osc.saw.tick(hz,0,rate,true);
        if(p[7]!=0)value+=osc.pulse.tick(hz,width,rate,false);
        if(p[4]>0)value+=.9f*p[5]*osc.sub.tick(hz*.5f,.5f,rate,false);
        if(p[6]>0){osc.noise=(random()+2*osc.noise)/3.f;value+=osc.noise*p[6];}
        return value;
    };
    const bool stereo=unisonExtra>0&&unisonWidthValue>0;
    for(int subSample=0;subSample<2;++subSample){
        const float central=oscSample(v.main,frequency);
        float left=central,right=central;
        if(unisonExtra>0){
            const float w=unisonWidthValue;left=right=(1-w*w)*central;
            for(int i=0;i<unisonExtra;++i){
                const auto index=static_cast<std::size_t>(i);
                const float sample=oscSample(v.extra[index],std::min(19000.f,frequency*unisonRatios[index]));
                const float nearWeight=1+.5f*w,farWeight=1-w;
                left+=sample*(i%2==0?farWeight:nearWeight);right+=sample*(i%2==0?nearWeight:farWeight);
            }
            left*=.5f;right*=.5f;
        }
        if(subSample==0){out.leftFirst=left;out.rightFirst=right;}
        else {out.leftSecond=left;out.rightSecond=right;}
    }
    if(p[38]==0){
        const float cutoff=cutoffForVoice(v,fEnv,modulation);
        v.lowpass.update(cutoff,rate);
        out.leftFirst=v.lowpass.tick(out.leftFirst,p[13],random());out.leftSecond=v.lowpass.tick(out.leftSecond,p[13],random());
        if(stereo){v.rightLowpass.update(cutoff,rate);out.rightFirst=v.rightLowpass.tick(out.rightFirst,p[13],random());out.rightSecond=v.rightLowpass.tick(out.rightSecond,p[13],random());}
        else {out.rightFirst=out.leftFirst;out.rightSecond=out.leftSecond;}
    }
    const float amplitude=p[18]==0?ampEnv:v.gateEnvelope.tick(v.gate,0,1,1,0,rate,false);
    out.leftFirst*=amplitude;out.leftSecond*=amplitude;out.rightFirst*=amplitude;out.rightSecond*=amplitude;
    out.leftFirst*=tuning.left;out.leftSecond*=tuning.left;out.rightFirst*=tuning.right;out.rightSecond*=tuning.right;return out;
}
float SynthDSP::Delay::tick(float input,float rate,float step) noexcept {
    if(samples.empty())return 0;
    if(ramp>=1)direction=-1;else if(ramp<=-1)direction=1;ramp+=step/rate*direction;
    const float delay=(ramp*0.3f+0.4f)*7.f*rate*0.001f;
    const auto whole=static_cast<std::size_t>(std::floor(delay));const float blend=1-(delay-std::floor(delay));
    const auto a=(cursor+samples.size()-whole%samples.size())%samples.size();const auto b=(a+samples.size()-1)%samples.size();
    last=blend*samples[a]+samples[b]-blend*last;lowpass=last*0.24872541427612305f+0.751274585723877f*lowpass+2.3283064365386963e-10f;
    samples[cursor]=input;cursor=(cursor+1)%samples.size();return lowpass;
}
void SynthDSP::processStereo(float& left,float& right) noexcept {
    const float rate=static_cast<float>(sampleRate);
    const float frequency=p[28]==0?20.f*fourth(p[24]):syncedFrequency;
    // Recovered sync branch slot1 multiplies BPM/60 by FIVE although UI says 1/16.
    // Rate is latched on noteOn, as in 0x1c00.
    if(lfoPhase>1)lfoPhase-=1;if(pwmPhase>1)pwmPhase-=1;
    if(p[27]>=4&&static_cast<int>(lfoPhase*256)==0){previousRandom=heldRandom;heldRandom=(random()-0.5f)*2;}
    const float samplePhase=std::min(lfoPhase,255.99998f/256.f);
    const float samplePwmPhase=std::min(pwmPhase,255.99998f/256.f);
    // TODO verificar: at exactly table index256 original reads the following table's first sample;
    // keep the safe final interpolation inside the current waveform table.
    float modulation=lfo(static_cast<int>(p[27]),samplePhase),pwm=lfo(1,samplePwmPhase);
    lfoPhase+=frequency/rate;pwmPhase+=frequency/rate;
    if(p[25]>0){if(static_cast<float>(lfoDelay)<p[25]*rate){modulation=0;++lfoDelay;}else if(lfoFade<1){modulation*=lfoFade;lfoFade+=5.e-10f*rate;}}
    filterSmooth=(1199.f*filterSmooth+bend*p[32]+p[12])/1200.f+1.e-9f;
    float first=0,second=0,rightFirst=0,rightSecond=0,amplitude=0;
    const bool stereo=stereoEnabled();
    bool anyVoiceActive=false;for(const auto& v:voices)anyVoiceActive=anyVoiceActive||v.gate||v.amp.level>0||v.gateEnvelope.level>0;
    for(auto& v:voices){const auto frame=renderVoice(v,modulation,pwm);first+=frame.leftFirst;second+=frame.leftSecond;rightFirst+=frame.rightFirst;rightSecond+=frame.rightSecond;amplitude+=p[18]==0?v.amp.level:v.gateEnvelope.level;}
    if(p[38]!=0){
        // TODO verificar: original shared-filter cutoff uses engine-selected voice and filter envelope,
        // but audio below is already VCA-scaled. Original then applies summed VCA again.
        auto& v=voices[static_cast<std::size_t>(nextVoice)];
        const float cutoff=cutoffForVoice(v,v.filter.level,modulation);
        sharedFilter.update(cutoff,rate);
        first=2.f*sharedFilter.tick(first*0.5f,p[13],random())*std::min(1.f,amplitude);
        second=2.f*sharedFilter.tick(second*0.5f,p[13],random())*std::min(1.f,amplitude);
        if(stereo){sharedRightFilter.update(cutoff,rate);rightFirst=2.f*sharedRightFilter.tick(rightFirst*.5f,p[13],random())*std::min(1.f,amplitude);rightSecond=2.f*sharedRightFilter.tick(rightSecond*.5f,p[13],random())*std::min(1.f,amplitude);}
    }
    // Observed FIR structure 0x1f16 / ctor0x327c, same two-sample accumulation order.
    constexpr std::array<float,6> c{{0.5f,0.3077392578125f,-0.07794189453125f,0.02618408203125f,-0.007080078125f,0.0010986328125f}};
    auto decimate=[&](std::array<float,9>& state,float a,float b){
        const float filtered=state[8]+a*c[5];
        for(int i=8;i>=5;--i)state[static_cast<std::size_t>(i)]=state[static_cast<std::size_t>(i-1)]+a*c[static_cast<std::size_t>(i-4)];
        state[4]=state[3]+a*c[1]+b*c[0];
        for(int i=3;i>=1;--i)state[static_cast<std::size_t>(i)]=state[static_cast<std::size_t>(i-1)]+a*c[static_cast<std::size_t>(5-i)];
        state[0]=a*c[5];return filtered*p[1]*p[1]*p[19];
    };
    left=anyVoiceActive?decimate(decimator,first,second):0;
    right=stereo&&anyVoiceActive?decimate(rightDecimator,rightFirst,rightSecond):left;
    for(auto& h:highpass)left=h.tick(left,.999f-.4f*cube(p[11]));
    if(stereo)for(auto& h:rightHighpass)right=h.tick(right,.999f-.4f*cube(p[11]));else right=left;
    float wetLeft=0,wetRight=0;
    if(anyVoiceActive&&p[34]!=0){wetRight+=delays[0].tick(right,rate,2);wetLeft+=delays[1].tick(left,rate,2);}
    if(anyVoiceActive&&p[35]!=0){wetRight+=delays[2].tick(right,rate,3.32f);wetLeft+=delays[3].tick(left,rate,3.32f);}
    if(anyVoiceActive&&(p[34]!=0||p[35]!=0)){left+=1.4f*chorusHighpass[0].tick(wetLeft,.995f);right+=1.4f*chorusHighpass[1].tick(wetRight,.995f);}
    // Original skips FIR and chorus when no voice is active; serial HPF state still advances.
    if(!std::isfinite(left))left=0;if(!std::isfinite(right))right=0;
}
}
