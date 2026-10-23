#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Parameters.h"
#include "ArpeggiatorParameters.h"
#include "ExtensionParameters.h"
#include "DSP/StereoDelay.h"
#include "DSP/StereoReverb.h"
#include "PresetManager.h"
#include "MidiMappings.h"
#include "DSP/SynthDSP.h"
#include <atomic>

class TalCloneProcessor final : public juce::AudioProcessor,
                                private juce::AsyncUpdater,
                                private juce::AudioProcessorParameter::Listener {
public:
    TalCloneProcessor();
    ~TalCloneProcessor() override;
    void prepareToPlay(double, int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "KODA-JUNO-60"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 30.0; } // TODO verificar duração máxima do release/chorus original.
    int getNumPrograms() override { return talclone::PresetManager::count; }
    int getCurrentProgram() override { return currentProgram.load(); }
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override;
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorValueTreeState parameters;
    juce::MidiKeyboardState keyboardState;
    void requestPanic() noexcept { pendingPanic.store(true); }
    bool isSustainActive() const noexcept { return sustainChannels.load()!=0; }
    void resetCurrentPreset();
    bool selectMidiLearnTarget(const juce::String&);
    bool isProgramVisible(int);
    int adjacentVisibleProgram(int direction);
    void clearMidiMappings() noexcept { midiMappings.clear(); }
    int getMidiMapping(int controller) const noexcept {return midiMappings.target(controller);}
    juce::String getMidiLearnTarget() const;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
private:
    void handleAsyncUpdate() override;
    void handleMidi(const juce::MidiMessage&);
    void syncProgramParameters(int);
    void captureCurrentProgram();
    void applyArpEvents(const talclone::Arpeggiator::Events&) noexcept;
    void updateArpeggiator() noexcept;
    talclone::Arpeggiator arpeggiator;
    std::array<std::atomic<float>*, talclone::arpParameterCount> arpRaw{};
    std::array<std::array<std::atomic<float>, talclone::arpParameterCount>,120> arpPrograms{};
    talclone::ArpValues lastArpValues{};
    void updateExtensions() noexcept;
    std::array<std::atomic<float>*,talclone::extensionParameterCount> extensionRaw{};
    std::array<std::array<std::atomic<float>,talclone::extensionParameterCount>,120> extensionPrograms{};
    talclone::ExtensionValues lastExtensionValues{};
    talclone::StereoDelay delayEffect;
    talclone::StereoReverb reverbEffect;
    double hostTempo=120;
    talclone::SynthDSP synth;
    talclone::PresetManager presets;
    std::array<std::atomic<float>*, 47> raw{};
    static constexpr std::size_t hostParameterCount=talclone::parameterCount+talclone::arpParameterCount+talclone::extensionParameterCount;
    std::array<std::atomic<float>,hostParameterCount> pendingMidiValues{};
    std::array<juce::RangedAudioParameter*,hostParameterCount> midiParameters{};
    std::array<bool,hostParameterCount> midiLearnable{};
    std::atomic<int> midiLearnTarget{-1};
    std::atomic<bool> suppressMidiSelection{false};
    talclone::MidiMappings midiMappings;
    void parameterValueChanged(int,float) override;
    void parameterGestureChanged(int,bool) override;
    void applyMappedParameter(int,float) noexcept;
    void resetMidiMappingBanks() noexcept;
    bool shouldLoadExtension(std::size_t) const noexcept;
    static bool globalLegacyParameter(std::size_t index) noexcept {return index==37||index==46;}
    juce::ValueTree saveMidiMap(int program) const;
    void loadMidiMap(int program,const juce::ValueTree&);

    std::array<std::array<std::atomic<float>, 47>, 120> audioValues{};
    std::array<float, 47> lastDSPValues{};
    std::atomic<int> currentProgram{0}, pendingProgram{-1};
    std::atomic<int> audioMapProgram{0};
    std::atomic<bool> pendingPanic{false};
    std::atomic<std::uint16_t> sustainChannels{0};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TalCloneProcessor)
};
