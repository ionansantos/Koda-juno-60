#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "VoiceTuningParameters.h"
// A dedicated overlay: the main synth keyboard and control layout are retained.
class VoiceTuningPanel final : public juce::Component {
public:
    explicit VoiceTuningPanel(juce::AudioProcessorValueTreeState& state) : parameters(state) {
        title.setText("VOICE TUNING",juce::dontSendNotification);title.setJustificationType(juce::Justification::centredLeft);addAndMakeVisible(title);
        randomAmount.setName("vtRandomAmount");randomAmount.setTooltip("Voice variation amount. Pan is preserved.");randomAmount.setRange(0,1);randomAmount.setNumDecimalPlacesToDisplay(2);randomAmount.setValue(.2);randomAmount.setSliderStyle(juce::Slider::LinearHorizontal);randomAmount.setTextBoxStyle(juce::Slider::TextBoxRight,false,48,18);addAndMakeVisible(randomAmount);
        randomize.setButtonText("RANDOM");randomize.setName("vtRandom");randomize.onClick=[this]{randomizeTuning();};addAndMakeVisible(randomize);
        close.setButtonText("CLOSE");close.setName("vtClose");close.onClick=[this]{if(onClose)onClose();};addAndMakeVisible(close);
        for(std::size_t i=0;i<sliders.size();++i){
            const auto& spec=talclone::voiceTuningSpecs[i];auto& slider=sliders[i];
            slider.setName(spec.id);slider.setComponentID(spec.id);slider.setTooltip(spec.name);
            slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,66,17);
            slider.getProperties().set("fxKnob",true);slider.setRange(0,1);slider.setDoubleClickReturnValue(true,.5);slider.setNumDecimalPlacesToDisplay(3);
            addAndMakeVisible(slider);attachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state,spec.id,slider);
            slider.textFromValueFunction=[](double value){return juce::String(value,3);};
            slider.valueFromTextFunction=[](const juce::String& text){return text.getDoubleValue();};
        }
        for(std::size_t i=0;i<columns.size();++i){columns[i].setText(talclone::voiceTuningControlNames[i],juce::dontSendNotification);columns[i].setJustificationType(juce::Justification::centred);addAndMakeVisible(columns[i]);}
        for(std::size_t i=0;i<rows.size();++i){rows[i].setText("VOICE "+juce::String(i+1),juce::dontSendNotification);rows[i].setJustificationType(juce::Justification::centred);addAndMakeVisible(rows[i]);}
        setSize(640,460);
    }
    std::function<void()> onClose;
    void paint(juce::Graphics& g) override {g.fillAll(juce::Colour(0xff242624));g.setColour(juce::Colour(0xffb3b494));g.drawRect(getLocalBounds(),2);g.setColour(juce::Colour(0xff444844));for(int row=0;row<6;++row)g.drawHorizontalLine(67+row*60,12.f,float(getWidth()-12));g.setColour(juce::Colour(0xffb3b494));g.drawText("RANDOM AMOUNT",180,getHeight()-32,115,20,juce::Justification::centredRight);}
    void resized() override {
        title.setBounds(18,8,400,30);randomAmount.setBounds(300,getHeight()-28,160,24);randomize.setBounds(475,getHeight()-28,90,24);close.setBounds(getWidth()-90,10,72,24);
        const int cell=(getWidth()-84)/6;
        for(std::size_t col=0;col<6;++col)columns[col].setBounds(76+int(col)*cell,45,cell,22);
        for(std::size_t row=0;row<6;++row){rows[row].setBounds(5,72+int(row)*60,68,46);for(std::size_t col=0;col<6;++col)sliders[row*6+col].setBounds(76+int(col)*cell,68+int(row)*60,cell,59);}
    }
private:
    void randomizeTuning(){
        for(std::size_t i=0;i<talclone::voiceTuningParameterCount;++i){
            if(i%talclone::voiceTuningControlsPerVoice==talclone::tuningPan)continue;
            if(auto* parameter=parameters.getParameter(talclone::voiceTuningSpecs[i].id)){
                parameter->beginChangeGesture();parameter->setValueNotifyingHost(std::clamp(.5f+float(randomAmount.getValue())*(random.nextFloat()-.5f),0.f,1.f));parameter->endChangeGesture();
            }
        }
    }
    juce::AudioProcessorValueTreeState& parameters;
    juce::Random random;
    juce::Label title;
    juce::TextButton close,randomize;
    juce::Slider randomAmount;
    std::array<juce::Label,6> columns,rows;
    std::array<juce::Slider,talclone::voiceTuningParameterCount> sliders;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,talclone::voiceTuningParameterCount> attachments;
};
