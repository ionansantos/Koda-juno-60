#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "../VoiceTuningParameters.h"
namespace talclone {
// Indices are the original 47-slot Params record. Slots 27/28 are raw dropdown indices.
// Reconstruction from static i386 evidence, not an assertion of sample-identical audio.
class SynthDSP {
public:
    using Parameters = std::array<float, 47>;
    SynthDSP();
    void setSampleRate(double);
    void setTempo(double bpm) noexcept;
    void setParameters(const Parameters&) noexcept;
    void setParameter(int index, float value) noexcept;
    void setReferenceMode(bool enabled) noexcept { referenceMode = enabled; }
    void setUnison(float voices,float detune,float width) noexcept;
    void setVoiceTuning(const VoiceTunings&) noexcept;
    void setPortamento(int mode,float amount,bool fixedTime,bool poly,bool arpRetrigger=false) noexcept;
    static int unisonVoiceCount(float normalized) noexcept;
    void noteOn(int note, float velocity, int channel=1) noexcept;
    void noteOff(int note, int channel=1) noexcept;
    void releaseArpeggiatorNote(int note, int channel) noexcept;
    void allNotesOff(int channel=0) noexcept;
    void allSoundOff(int channel) noexcept;
    void setSustainPedal(bool down, int channel=1) noexcept;
    bool handleController(int controller, int value, int channel=1) noexcept;
    void pitchWheel(int value) noexcept;
    void processStereo(float& left, float& right) noexcept;
    void reset() noexcept;
private:
    struct Oscillator {
        float phase=0, edge=0.1f;
        std::array<float,66> correction{};
        int cursor=0;
        float tick(float frequency,float width,float rate,bool saw) noexcept;
        void addStep(float offset,float sign) noexcept;
    };
    struct Envelope {
        float level=0;
        bool decay=false;
        float tick(bool gate,float attack,float decayRate,float sustain,float release,float rate,bool instantAttack,float attackFactor=1,float decayFactor=1,float releaseFactor=1) noexcept;
    };
    struct Filter {
        std::array<float,8> stage{};
        float delay=0, output=0, coefficient=0, resonanceCorrection=1, normalizedCutoff=0;
        void update(float cutoff,float sampleRate) noexcept;
        float tick(float input,float resonance,float random) noexcept;
    };
    struct OscillatorStack { Oscillator saw,pulse,sub; float noise=0; };
    struct Voice {
        int note=64,tail=100,channel=1;
        bool gate=false,keyDown=false;
        float velocity=0, drift=0, pitch=42, glideStep=0;
        OscillatorStack main;
        std::array<OscillatorStack,6> extra{};
        Envelope amp,filter,gateEnvelope;
        Filter lowpass,rightLowpass;
    };
    struct Highpass { float input=0,output=0; float tick(float,float) noexcept; };
    struct Delay {
        std::vector<float> samples;
        std::size_t cursor=0;
        float ramp=-1,direction=1,last=0,lowpass=0;
        float tick(float input,float rate,float step) noexcept;
    };
    float random() noexcept;
    float lfo(int waveform,float phase) noexcept;
    float cutoffForVoice(Voice&,float envelope,float modulation) noexcept;
    struct StereoOversample {float leftFirst=0,leftSecond=0,rightFirst=0,rightSecond=0;};
    StereoOversample renderVoice(Voice&,float modulation,float pwm) noexcept;
    void startVoice(Voice&,int note,float velocity,int channel,bool monoLegato) noexcept;
    void restoreMonoNote() noexcept;
    struct HeldNote {std::uint64_t order=0;float velocity=0;bool down=false;};
    std::array<HeldNote,16*128> heldNotes{};
    std::uint64_t noteOrder=0;
    int portamentoMode=0;
    float portamentoAmount=0;
    bool portamentoTime=false,portamentoPoly=false,arpRetrigger=false;
    struct TuningCoefficients {float cutoff=0,detune=0,attack=1,decay=1,release=1,left=1,right=1;};
    std::array<TuningCoefficients,6> voiceTuning{};
    bool tuningStereo=false;
    bool stereoEnabled() const noexcept {return tuningStereo||(unisonExtra>0&&unisonWidthValue>0);}
    void copyStereoState() noexcept;
    const TuningCoefficients& tuningForVoice(const Voice& voice) const noexcept {return voiceTuning[static_cast<std::size_t>(&voice-voices.data())];}
    bool monoPortamento() const noexcept {return portamentoMode>0&&!portamentoPoly;}
    Parameters p{};
    bool referenceMode=false;
    double sampleRate=44100,tempo=120;
    float syncedFrequency=0,bend=0,lfoPhase=0,pwmPhase=0,heldRandom=0,previousRandom=0,filterSmooth=0,lfoFade=0;
    std::size_t lfoDelay=0;
    int nextVoice=5;
    std::array<Voice,6> voices{};
    std::array<bool,16> sustainPedals{};
    std::array<float,9> decimator{},rightDecimator{};
    std::array<Highpass,3> highpass{},rightHighpass{};
    std::array<Highpass,2> chorusHighpass{};
    std::array<Delay,4> delays{};
    Filter sharedFilter,sharedRightFilter;
    int unisonExtra=0;
    float unisonDetuneValue=-1,unisonWidthValue=0;
    std::array<float,6> unisonRatios{{1,1,1,1,1,1}};
};
}
