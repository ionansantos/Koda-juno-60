#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace talclone {
// Musical scheduler independent of JUCE and the voice engine. No heap allocation.
// Direction/range/rate follow the recovered LX 5.1.5 paths; transport stop,
// overflow, invalid pitches and channel identity are explicit KODA policies.
class Arpeggiator {
public:
    struct Settings {
        bool enabled=false, sync=false, hold=false, external=false, midiClock=false, host=true;
        int mode=2, range=0, special=0;
        float rate=0.5f;
    };
    struct Note { int pitch=0, channel=1; float velocity=0; };
    struct Events { std::array<Note,2> notes{}; int count=0; bool release=false; };
    struct Transport { bool valid=false, playing=false; double bpm=120, ppq=0; };
    static constexpr std::size_t capacity=16*128;
    static constexpr std::array<double,18> multipliers {{.125,1./6,.1875,.25,1./3,.375,.5,2./3,.75,1,4./3,1.5,2,8./3,3,3,4,4.5}};
    static constexpr std::array<const char*,18> divisions {{"1/2","1/4.","1/2T","1/4","1/8.","1/4T","1/8","1/16.","1/8T","1/16","1/32.","1/16T","1/32","1/64.","1/32T","1/64","1/128.","1/64T"}};
    static double freeFrequency(float x) noexcept { return 1.2631578*std::exp(2.9957323*std::clamp(x,0.f,1.f))-.26315784; }
    static int division(float x) noexcept { return static_cast<int>(std::floor(std::clamp(x,0.f,1.f)*17+.5f)); }
    void prepare(double rate) noexcept { sampleRate=std::isfinite(rate)&&rate>0?rate:44100; reset();updateRate(); }
    void reset() noexcept { heldCount=baseCount=sequenceCount=cursor=0; active=false; phase=1; pulse=false; clocks=0; hostKnown=false; hostInitial=false; midiRunning=true; pedals={}; randomState=1; }
    const Settings& settings() const noexcept { return config; }
    Events configure(Settings next) noexcept {
        next.mode=std::clamp(next.mode,0,2);next.range=std::clamp(next.range,0,2);next.special=std::clamp(next.special,0,3);
        next.rate=std::isfinite(next.rate)?std::clamp(next.rate,0.f,1.f):.5f;
        Events events;
        const bool routing=next.enabled!=config.enabled;
        const bool clock=next.sync!=config.sync||next.external!=config.external||next.midiClock!=config.midiClock||next.host!=config.host;
        const bool rebuild=next.mode!=config.mode||next.range!=config.range||next.special!=config.special;
        const bool unhold=config.hold&&!next.hold;
        config=next;updateRate();
        if(routing){events=release();reset();return events;}
        if(clock){events=release();phase=1;pulse=false;clocks=0;hostKnown=false;}
        if(unhold){copyHeld();rebuildSequence();events=release();}
        else if(rebuild)rebuildSequence();
        return events;
    }
    Events noteOn(int pitch,int channel,float velocity) noexcept {
        if(!config.enabled||pitch<0||pitch>127||channel<1||channel>16)return {};
        for(std::size_t i=0;i<heldCount;++i)if(held[i].pitch==pitch&&held[i].channel==channel)return {};
        if(heldCount==capacity)return {};
        std::move_backward(held.begin(),held.begin()+static_cast<std::ptrdiff_t>(heldCount),held.begin()+static_cast<std::ptrdiff_t>(heldCount+1));
        held[0]={pitch,channel,velocity};++heldCount;const bool empty=sequenceCount==0;copyHeld();rebuildSequence();hostInitial=hostInitial||empty;return {};
    }
    Events noteOff(int pitch,int channel) noexcept {
        for(std::size_t i=0;i<heldCount;++i)if(held[i].pitch==pitch&&held[i].channel==channel){
            std::move(held.begin()+static_cast<std::ptrdiff_t>(i+1),held.begin()+static_cast<std::ptrdiff_t>(heldCount),held.begin()+static_cast<std::ptrdiff_t>(i));--heldCount;break;
        }
        if(!config.hold&&!pedals[static_cast<std::size_t>(std::clamp(channel,1,16)-1)]) {
            copyHeld();rebuildSequence();
            if(sequenceCount==0){phase=1;cursor=0;return release();}
        }
        return {};
    }
    Events sustain(int channel,bool down) noexcept {
        if(channel<1||channel>16)return {};
        pedals[static_cast<std::size_t>(channel-1)]=down;
        if(!down&&!config.hold){copyHeld();rebuildSequence();if(!sequenceCount)return release();}
        return {};
    }
    Events clearChannel(int channel) noexcept {
        auto end=std::remove_if(held.begin(),held.begin()+static_cast<std::ptrdiff_t>(heldCount),[channel](Note n){return n.channel==channel;});heldCount=static_cast<std::size_t>(end-held.begin());
        // A channel reset also removes latched notes from that channel.
        end=std::remove_if(base.begin(),base.begin()+static_cast<std::ptrdiff_t>(baseCount),[channel](Note n){return n.channel==channel;});baseCount=static_cast<std::size_t>(end-base.begin());
        pedals[static_cast<std::size_t>(channel-1)]=false;rebuildSequence();
        if(active&&current.channel==channel)return release();return {};
    }
    void trigger() noexcept { pulse=true; }
    Events midiStart() noexcept { clocks=0;cursor=0;pulse=false;phase=1;midiRunning=true;return release(); }
    void midiContinue() noexcept { clocks=0;midiRunning=true; }
    Events midiStop() noexcept { midiRunning=false;pulse=false;return release(); }
    void midiTick() noexcept {
        if(!midiRunning)return;
        const int divisor=clockDivisor;
        clocks=(clocks+1)%divisor;if(clocks==0)pulse=true;
    }
    Events beginBlock(Transport t,int samples) noexcept {
        hostValid=t.valid&&std::isfinite(t.ppq);playing=t.playing;
        bpm=std::isfinite(t.bpm)&&t.bpm>0?t.bpm:120;updateRate();
        ppq=t.ppq;
        Events e;
        if(config.enabled&&config.sync&&!config.external&&!config.midiClock&&config.host&&hostValid){
            const bool jumped=hostKnown&&std::abs(ppq-expectedPPQ)>2*bpm/(60*sampleRate);
            if(!playing||jumped){e=release();if(jumped)cursor=0;hostKnown=false;}
            expectedPPQ=ppq+(playing?samples*bpm/(60*sampleRate):0);
        }else hostKnown=false;
        return e;
    }
    Events tick() noexcept {
        if(!config.enabled||sequenceCount==0){pulse=false;ppq+=bpm/(60*sampleRate);return {};}
        bool step=false;
        if(config.sync&&(config.external||config.midiClock)) {step=pulse;pulse=false;}
        else if(config.sync&&config.host&&hostValid){
            if(!playing)return {};
            const double cycles=ppq*4*rateMultiplier;
            const auto beat=static_cast<std::int64_t>(std::floor(cycles+1.e-9));
            step=hostInitial||(hostKnown?beat!=hostBeat:std::abs(cycles-std::round(cycles))<1.e-7);hostInitial=false;
            hostBeat=beat;hostKnown=true;
        }else{
            step=phase>=1;
            if(step)phase-=std::floor(phase);
            phase+=phaseIncrement;
        }
        ppq+=bpm/(60*sampleRate);
        if(!step)return {};
        auto e=release();
        if(cursor>=sequenceCount)cursor=0;
        current=sequence[cursor];active=true;
        e.notes[static_cast<std::size_t>(e.count++)]=current;
        if(config.special==2&&sequenceCount>1){
            randomState=(randomState*16807u)&0x7fffffffu;
            const auto offset=1+static_cast<std::size_t>(randomState)%(sequenceCount-1);cursor=(cursor+offset)%sequenceCount;
        }else ++cursor;
        return e;
    }
    Events release() noexcept { Events e;if(active){e.release=true;e.notes[0]=current;e.count=1;active=false;}return e; }
private:
    void updateRate() noexcept {
        rateMultiplier=multipliers[static_cast<std::size_t>(division(config.rate))];
        phaseIncrement=(config.sync?4*rateMultiplier*bpm/60:freeFrequency(config.rate))/sampleRate;
        clockDivisor=std::max(1,static_cast<int>(6./rateMultiplier));
    }
    void copyHeld() noexcept {
        std::size_t kept=0;
        for(std::size_t i=0;i<baseCount;++i) {
            const auto n=base[i];
            const bool pressed=std::any_of(held.begin(),held.begin()+static_cast<std::ptrdiff_t>(heldCount),[n](Note h){return h.pitch==n.pitch&&h.channel==n.channel;});
            if(pedals[static_cast<std::size_t>(n.channel-1)]&&!pressed)base[kept++]=n;
        }
        std::copy_n(held.begin(),heldCount,base.begin()+static_cast<std::ptrdiff_t>(kept));baseCount=kept+heldCount;
    }
    void rebuildSequence() noexcept {
        std::copy_n(base.begin(),baseCount,sequence.begin());
        if(config.special==0||config.special==3){
            const bool ascending=config.mode!=0||config.special==3;
            std::sort(sequence.begin(),sequence.begin()+static_cast<std::ptrdiff_t>(baseCount),[ascending](Note a,Note b){return ascending?a.pitch<b.pitch:a.pitch>b.pitch;});
        }
        sequenceCount=baseCount;
        const int sign=config.mode==0&&config.special!=3?-1:1;
        for(int octave=1;octave<=config.range;++octave)for(std::size_t i=0;i<baseCount;++i){
            auto note=sequence[i];note.pitch+=sign*12*octave;
            if(note.pitch>=0&&note.pitch<=127)sequence[sequenceCount++]=note;
        }
        const auto count=sequenceCount;
        if(config.special==3){for(std::size_t i=count;i>0;--i)sequence[sequenceCount++]=sequence[i-1];}
        else if(config.mode==1&&count>2){for(std::size_t i=count-1;i>1;--i)sequence[sequenceCount++]=sequence[i-1];}
        if(cursor>=sequenceCount)cursor=0;
    }
    Settings config;
    std::array<Note,capacity> held{},base{};
    std::array<Note,capacity*6> sequence{};
    std::array<bool,16> pedals{};
    std::size_t heldCount=0,baseCount=0,sequenceCount=0,cursor=0;
    Note current;
    bool active=false,pulse=false,hostValid=false,playing=false,hostKnown=false,hostInitial=false,midiRunning=true;
    int clocks=0,clockDivisor=6;
    std::int64_t hostBeat=0;
    std::uint32_t randomState=1;
    double sampleRate=44100,phase=1,bpm=120,ppq=0,expectedPPQ=0,rateMultiplier=1,phaseIncrement=freeFrequency(.5f)/44100;
};
} // namespace talclone
