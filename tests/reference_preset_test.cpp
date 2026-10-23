#include "PluginProcessor.h"
#include "ReferencePreset.h"
#include <cassert>
#include <cmath>
#include <iostream>

namespace {
float get(TalCloneProcessor& p, const char* id) {
    auto* value = p.parameters.getRawParameterValue(id);
    assert(value);
    return value->load();
}
void set(TalCloneProcessor& p, const char* id, float value) {
    auto* parameter = p.parameters.getParameter(id);
    assert(parameter);
    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}
}

int main() {
    TalCloneProcessor p;
    p.prepareToPlay(44100, 256);
    assert(p.getNumPrograms() == 120);
    assert(p.getProgramName(72) == talclone::referencePresetName);
    assert(p.isProgramVisible(72));
    p.setCurrentProgram(72);

    for (std::size_t i = 0; i < talclone::referencePresetParameters.size(); ++i) {
        if (i == 37 || i == 46) continue; // global switches are not part of the program payload
        assert(std::abs(get(p, talclone::parameterSpecs[i].id) - talclone::referencePresetParameters[i]) < 1.0e-5f);
    }
    for (std::size_t i = 0; i < talclone::referencePresetArp.size(); ++i)
        assert(std::abs(get(p, talclone::arpSpecs[i].id) - talclone::referencePresetArp[i]) < 1.0e-5f);
    for (std::size_t i = 0; i < talclone::referencePresetExtensions.size(); ++i) {
        if (talclone::isGlobalExtension(i)) continue;
        assert(std::abs(get(p, talclone::extensionSpecs[i].id) - talclone::referencePresetExtensions[i]) < 1.0e-5f);
    }

    // CLEAR must restore the immutable reference values, not generic defaults,
    // and it must ignore ambient locks for this explicit reset.
    set(p, "p12", 0.91f);
    set(p, "arpRate", 0.91f);
    set(p, "delayWet", 0.91f);
    set(p, "vt1Detune", 0.91f);
    set(p, "arpLock", 1.0f);
    set(p, "fxLock", 1.0f);
    set(p, "voiceTuningLock", 1.0f);
    p.resetCurrentPreset();
    assert(std::abs(get(p, "p12") - talclone::referencePresetParameters[12]) < 1.0e-5f);
    assert(std::abs(get(p, "arpRate") - talclone::referencePresetArp[talclone::arpRate]) < 1.0e-5f);
    assert(std::abs(get(p, "delayWet") - talclone::referencePresetExtensions[talclone::delayWet]) < 1.0e-5f);
    assert(std::abs(get(p, "vt1Detune") - talclone::referencePresetExtensions[talclone::voiceTuningParameterOffset + 1]) < 1.0e-5f);
    assert(get(p, "p46") < 0.5f);

    // Full state keeps the reference preset editable and restores it verbatim.
    set(p, "p13", 0.23f);
    set(p, "arpEnabled", 0.0f);
    set(p, "reverbWet", 0.77f);
    juce::MemoryBlock state;
    p.getStateInformation(state);
    TalCloneProcessor restored;
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    assert(restored.getCurrentProgram() == 72);
    assert(std::abs(get(restored, "p13") - 0.23f) < 1.0e-5f);
    assert(get(restored, "arpEnabled") < 0.5f);
    assert(std::abs(get(restored, "reverbWet") - 0.77f) < 1.0e-5f);

    std::cout << "Reference preset slot 72: name, values, arp, FX, voice tuning, CLEAR and state passed\n";
}
