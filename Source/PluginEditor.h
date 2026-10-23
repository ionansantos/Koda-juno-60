#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Parameters.h"
#include "ArpeggiatorParameters.h"
#include "EffectsParameters.h"
#include "PortamentoParameters.h"
#include "LXLookAndFeel.h"
#include "VoiceTuningPanel.h"
class TalCloneProcessor;
class TalCloneEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit TalCloneEditor(TalCloneProcessor&);
    ~TalCloneEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
private:
    void timerCallback() override;
    void refreshPresetMenu();
    juce::StringArray presetMenuNames;
    int menuProgram=-1;
    TalCloneProcessor& synthProcessor;
    LXLookAndFeel theme;
    juce::MidiKeyboardComponent keyboard;
    juce::TextButton previous { "<" }, next { ">" }, panic { "PANIC" }, hideKeyboard { "HIDE KEYB" };
    // Toolbar actions and their independent lock/learn states.
    std::array<juce::TextButton,5> toolbarButtons;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>,3> toolbarAttachments;
    std::unique_ptr<VoiceTuningPanel> voiceTuningPanel;
    juce::Slider pwmSelector, lfoSelector;
    bool syncingSelectors=false;
    void syncSelectors();
    juce::ComboBox presets;
    std::array<juce::Slider, talclone::parameterCount> sliders;
    std::array<juce::ToggleButton, talclone::parameterCount> buttons;
    std::array<juce::ComboBox, 2> menus;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, talclone::parameterCount> sliderAttachments;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, talclone::parameterCount> buttonAttachments;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>, 2> menuAttachments;
    std::array<juce::TextButton,7> arpButtons;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>,7> arpButtonAttachments;
    std::array<juce::Slider,3> arpSliders;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,3> arpSliderAttachments;
    juce::ComboBox arpPattern;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> arpPatternAttachment;
    std::array<juce::Slider,talclone::effectParameterCount> effectSliders;
    std::array<juce::TextButton,talclone::effectParameterCount> effectButtons;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,talclone::effectParameterCount> effectSliderAttachments;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>,talclone::effectParameterCount> effectButtonAttachments;
    std::array<juce::Slider,2> portamentoSliders;
    std::array<juce::TextButton,2> portamentoButtons;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,2> portamentoSliderAttachments;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>,2> portamentoButtonAttachments;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TalCloneEditor)
};
