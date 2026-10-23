#include "PluginEditor.h"
#include "PluginProcessor.h"
namespace {
struct Position { int parameter,x,y,kind; };
constexpr Position layout[] = {
 {10,36,98,0},{9,77,98,0},
 {7,143,127,2},{3,173,127,2},{4,203,127,2},{5,250,98,0},{6,290,98,0},
 {11,357,98,0},{12,413,98,0},{13,452,98,0},{14,493,121,1},
 {15,531,98,0},{16,572,98,0},{17,613,98,0},{18,665,124,1},
 {20,733,98,0},{21,774,98,0},{22,815,98,0},{23,856,98,0},
 {34,925,127,2},{35,957,127,2},
 {24,207,254,0},{25,249,254,0},{27,337,279,3},{28,337,309,3},
 {31,533,254,0},{32,579,254,0},{29,625,254,0},
 {1,37,444,0},{2,82,444,0},{33,128,444,0}
};
const char* labelFor(int p) {
 switch(p) {
 case 1:return "LEVEL";case 2:return "TUNE";case 3:return "SAW";case 4:return "SUB";
 case 5:return "SUB OSC";case 6:return "NOISE";case 7:return "PULSE";
 case 9:return "PW";case 10:case 16:return "MOD";case 11:return "FILTER";
 case 12:return "FREQ";case 13:return "RES";case 15:return "ENV";case 17:return "KYBD";
 case 20:return "A";case 21:return "D";case 22:return "S";case 23:return "R";
 case 24:return "RATE";case 25:return "DELAY TIME";case 29:return "VEL ENV";
 case 31:return "PW DCO";case 32:return "PW FILTER";case 33:return "OCTAVE";
 case 34:return "I";case 35:return "II";case 38:return "CPU";default:return "";
 }
}
void label(juce::Graphics& g,const juce::String& text,int x,int y,int w,int h=15,float size=9) {
 g.setColour(juce::Colour(0xffb8bab2));g.setFont(juce::Font(juce::FontOptions(size,juce::Font::bold)));
 g.drawText(text,x,y,w,h,juce::Justification::centred);
}
}
TalCloneEditor::TalCloneEditor(TalCloneProcessor& p)
 : AudioProcessorEditor(p),synthProcessor(p),keyboard(p.keyboardState,juce::MidiKeyboardComponent::horizontalKeyboard) {
 setLookAndFeel(&theme);setSize(1000,570);addMouseListener(this,true);
 for(const auto& c:layout) {
  const auto i=static_cast<std::size_t>(c.parameter);const auto& spec=talclone::parameterSpecs[i];
  if(c.kind==1||c.kind==2) {
   auto& b=buttons[i];b.setComponentID(spec.id);b.setTooltip(spec.name);b.getProperties().set("verticalSwitch",c.kind==1);b.getProperties().set("invertSwitch",c.parameter==14||c.parameter==18);
   b.setColour(juce::ToggleButton::tickColourId,c.parameter==3||c.parameter==34?juce::Colour(0xffaaa11b):
    c.parameter==4||c.parameter==35?juce::Colour(0xffba8125):juce::Colour(0xffb6b8ab));
   addAndMakeVisible(b);
   buttonAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,spec.id,b);
  } else if(c.kind==3) {
   auto& menu=menus[static_cast<std::size_t>(c.parameter-27)];
   if(c.parameter==27)menu.addItemList({"SIN","TRI","SAW","REC","NOISE"},1);
   else menu.addItemList({"FREE","1/16","1/8","1/4","1/2","1/1","2/1","1/16T","1/8T","1/4T","1/2T","1/1T","2/1T"},1);
   menu.setComponentID(spec.id);addAndMakeVisible(menu);menu.setTooltip(spec.name);
   menuAttachments[static_cast<std::size_t>(c.parameter-27)]=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters,spec.id,menu);
  } else {
   auto& s=sliders[i];s.setSliderStyle(juce::Slider::LinearVertical);s.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
   s.setComponentID(spec.id);s.setTooltip(spec.name);addAndMakeVisible(s);
   sliderAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,spec.id,s);
  }
 }
 auto setParameter=[&p](const char* id,float value){auto* parameter=p.parameters.getParameter(id);parameter->beginChangeGesture();parameter->setValueNotifyingHost(parameter->convertTo0to1(value));parameter->endChangeGesture();};
 for(auto* selector:{&pwmSelector,&lfoSelector}){selector->setSliderStyle(juce::Slider::LinearVertical);selector->setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);selector->setRange(0,2,1);selector->getProperties().set("arpSelector",true);addAndMakeVisible(*selector);}
 pwmSelector.setName("pwmSource");pwmSelector.setTooltip("PWM source: LFO / MAN / ENV");
 pwmSelector.onValueChange=[this,setParameter]{if(syncingSelectors)return;const int mode=static_cast<int>(pwmSelector.getValue());setParameter("p36",mode==1?1.f:0.f);setParameter("p08",mode==0?1.f:0.f);};
 lfoSelector.setName("lfoTriggerMode");lfoSelector.setTooltip("LFO trigger: KEY / FREE / SYNC; SYNC uses the clock division menu");
 lfoSelector.onValueChange=[this,setParameter]{if(syncingSelectors)return;const int mode=static_cast<int>(lfoSelector.getValue());setParameter("p26",mode==2?0.f:1.f);setParameter("p28",mode==0?3.f:0.f);};
 syncSelectors();
 sliders[19].setSliderStyle(juce::Slider::LinearVertical);sliders[19].setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
 sliders[19].setComponentID("p19");addAndMakeVisible(sliders[19]);sliders[19].setTooltip("ENVVOLUME");
 sliderAttachments[19]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,"p19",sliders[19]);
 presets.setName("presetSelector");refreshPresetMenu();
 presets.setSelectedId(p.getCurrentProgram()+1,juce::dontSendNotification);
 presets.onChange=[this]{if(presets.getSelectedId()>0)synthProcessor.setCurrentProgram(presets.getSelectedId()-1);repaint();};
 previous.onClick=[this]{synthProcessor.setCurrentProgram(synthProcessor.adjacentVisibleProgram(-1));};
 next.onClick=[this]{synthProcessor.setCurrentProgram(synthProcessor.adjacentVisibleProgram(1));};
 panic.onClick=[this]{synthProcessor.keyboardState.allNotesOff(1);synthProcessor.requestPanic();};
 hideKeyboard.onClick=[this]{keyboard.setVisible(!keyboard.isVisible());
  hideKeyboard.setButtonText(keyboard.isVisible()?"HIDE KEYB":"SHOW KEYB");};
 for(auto* b:{&previous,&next,&panic,&hideKeyboard})addAndMakeVisible(*b);
 const std::array<const char*,5> toolbarNames{{"VT","LOCK","MIDI LEARN","LOCK","CLEAR"}};
 const std::array<const char*,5> toolbarIds{{"voiceTuning","voiceTuningLock","midiLearn","midiMappingLock","presetClear"}};
 const std::array<const char*,5> toolbarTips{{"Open individual voice tuning","Keep voice tuning when changing presets","Enable, touch a synth control, move a MIDI CC, then disable to finish","Keep MIDI mappings when changing presets","Restore the preset factory sound and erase its MIDI mappings"}};
 for(std::size_t i=0;i<toolbarButtons.size();++i){auto& button=toolbarButtons[i];button.setButtonText(toolbarNames[i]);button.setName(toolbarIds[i]);button.setTooltip(toolbarTips[i]);button.getProperties().set("arpPanel",true);button.setClickingTogglesState(i<4);button.setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff863a32));addAndMakeVisible(button);}
 const std::array<const char*,3> toolbarParameterIds{{"voiceTuningLock","p46","p37"}};
 for(std::size_t i=0;i<toolbarAttachments.size();++i)toolbarAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,toolbarParameterIds[i],toolbarButtons[i+1]);
 toolbarButtons[0].onClick=[this]{voiceTuningPanel->setVisible(toolbarButtons[0].getToggleState());if(voiceTuningPanel->isVisible())voiceTuningPanel->toFront(true);};
 toolbarButtons[4].onClick=[this]{synthProcessor.resetCurrentPreset();};
 addAndMakeVisible(presets);addAndMakeVisible(keyboard);
 keyboard.setAvailableRange(36,72);keyboard.setLowestVisibleKey(36);keyboard.setOctaveForMiddleC(4);
 keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId,juce::Colour(0xffbdbeb7));
 keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId,juce::Colour(0xff171717));
 keyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId,juce::Colour(0xff70706c));
 keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,juce::Colour(0x8095a96d));
 const std::array<std::size_t,7> ids{{talclone::arpLock,talclone::arpHold,talclone::arpExternal,talclone::arpMidiClock,talclone::arpHost,talclone::arpEnabled,talclone::arpSync}};
 const std::array<const char*,7> titles{{"LOCK","HOLD","EXT 16","MIDI CLK","HOST","ON","SYNC"}};
 for(std::size_t i=0;i<arpButtons.size();++i){auto& b=arpButtons[i];b.setComponentID(talclone::arpSpecs[ids[i]].id);b.setButtonText(titles[i]);b.setClickingTogglesState(true);b.getProperties().set("arpPanel",true);
  b.setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff863a32));
  b.setColour(juce::TextButton::textColourOnId,juce::Colour(0xffffddd0));
  b.setTooltip(i==0?"Keep arpeggiator settings when changing presets":i==1?"Keep the pattern after releasing keys":i==2?"Channel 16 note-on triggers each step (SYNC). Overrides MIDI CLK and HOST":i==3?"Incoming MIDI clock (SYNC). Overrides HOST":i==4?"Follow host position (SYNC); stop with host transport":talclone::arpSpecs[ids[i]].name);
  addAndMakeVisible(b);arpButtonAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,talclone::arpSpecs[ids[i]].id,b);
 }
 const std::array<std::size_t,3> sliderIds{{talclone::arpMode,talclone::arpRange,talclone::arpRate}};
 for(std::size_t i=0;i<arpSliders.size();++i){auto& s=arpSliders[i];s.setSliderStyle(juce::Slider::LinearVertical);
  s.setComponentID(talclone::arpSpecs[sliderIds[i]].id);s.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);s.getProperties().set("arpSelector",i<2);s.setTooltip(talclone::arpSpecs[sliderIds[i]].name);
  addAndMakeVisible(s);arpSliderAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,talclone::arpSpecs[sliderIds[i]].id,s);
 }
 // The extra names describe recovered branches; they are KODA labels, not asserted TAL menu text.
 arpPattern.setComponentID("arpSpecial");arpPattern.addItemList({"NORMAL","AS PLAYED","RANDOM","UP/DOWN REPEAT"},1);addAndMakeVisible(arpPattern);
 arpPatternAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters,"arpSpecial",arpPattern);
 for(std::size_t i=0;i<talclone::effectSpecs.size();++i){const auto& spec=talclone::effectSpecs[i];
  if(i==talclone::fxLock||i==talclone::delaySync){auto& b=effectButtons[i];b.setName(spec.id);b.setButtonText(i==talclone::fxLock?"LOCK":"SYNC");b.setClickingTogglesState(true);b.getProperties().set("arpPanel",true);
   b.setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff863a32));b.setTooltip(i==talclone::fxLock?"Keep delay and reverb settings across presets; Unison follows the preset":"Delay time follows host BPM");addAndMakeVisible(b);
   effectButtonAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,spec.id,b);
  }else{auto& knob=effectSliders[i];knob.setName(spec.id);knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);knob.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);knob.getProperties().set("fxKnob",true);
   knob.setTooltip(spec.name);knob.setMouseDragSensitivity(160);addAndMakeVisible(knob);
   effectSliderAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,spec.id,knob);
  }
 }
 for(std::size_t i=0;i<2;++i){const auto& spec=talclone::portamentoSpecs[i];auto& slider=portamentoSliders[i];
  slider.setName(spec.id);slider.setSliderStyle(juce::Slider::LinearVertical);slider.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
  slider.getProperties().set("arpSelector",i==0);slider.setTooltip(i==0?"OFF: no glide; I: legato; II: always glide": "Portamento rate, or duration when TIME is enabled");
  addAndMakeVisible(slider);portamentoSliderAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,spec.id,slider);
  const auto& buttonSpec=talclone::portamentoSpecs[i+2];auto& button=portamentoButtons[i];button.setName(buttonSpec.id);button.setButtonText(i==0?"TIME":"POLY");button.setClickingTogglesState(true);button.getProperties().set("arpPanel",true);
  button.setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff9a5229));button.setTooltip(i==0?"Constant duration across note intervals":"Independent glide for each voice; disabled uses mono note priority");
  addAndMakeVisible(button);portamentoButtonAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,buttonSpec.id,button);
 }
 voiceTuningPanel=std::make_unique<VoiceTuningPanel>(p.parameters);voiceTuningPanel->setName("voiceTuningPanel");voiceTuningPanel->setBounds(180,75,640,460);addChildComponent(*voiceTuningPanel);
 voiceTuningPanel->onClose=[this]{voiceTuningPanel->setVisible(false);toolbarButtons[0].setToggleState(false,juce::dontSendNotification);};
 startTimerHz(10);
}
TalCloneEditor::~TalCloneEditor(){removeMouseListener(this);stopTimer();setLookAndFeel(nullptr);}
void TalCloneEditor::timerCallback(){syncSelectors();refreshPresetMenu();presets.setSelectedId(synthProcessor.getCurrentProgram()+1,juce::dontSendNotification);repaint(12,207,163,64);repaint(150,245,23,18);repaint(656,40,329,11);repaint(717,225,268,127);repaint(284,353,701,73);repaint(379,225,133,127);}
void TalCloneEditor::syncSelectors() {
 syncingSelectors=true;
 const auto value=[this](const char* id){return synthProcessor.parameters.getRawParameterValue(id)->load();};
 pwmSelector.setValue(value("p36")>=.5f?1:value("p08")>=.5f?0:2,juce::dontSendNotification);
 lfoSelector.setValue(value("p28")>=.5f?0:value("p26")>=.5f?1:2,juce::dontSendNotification);
 syncingSelectors=false;
}
void TalCloneEditor::mouseDown(const juce::MouseEvent& event) {
 if(synthProcessor.parameters.getRawParameterValue("p46")->load()<.5f)return;
 for(auto* control=event.eventComponent;control&&control!=this;control=control->getParentComponent()){
  const auto id=control->getComponentID().isNotEmpty()?control->getComponentID():control->getName();
  if(synthProcessor.selectMidiLearnTarget(id)){repaint(656,40,329,11);break;}
 }
}
void TalCloneEditor::refreshPresetMenu() {
 juce::StringArray names;
 for(int i=0;i<synthProcessor.getNumPrograms();++i)names.add(synthProcessor.getProgramName(i));
 const int current=synthProcessor.getCurrentProgram();
 if(names==presetMenuNames&&menuProgram==current)return;
 presetMenuNames=names;menuProgram=current;presets.clear(juce::dontSendNotification);
 for(int i=0;i<names.size();++i)if(synthProcessor.isProgramVisible(i)||i==current)
  presets.addItem(juce::String(i+1)+" "+(synthProcessor.isProgramVisible(i)?names[i]:"Init"),i+1);
 presets.setSelectedId(current+1,juce::dontSendNotification);
}
void TalCloneEditor::resized() {
 if(voiceTuningPanel)voiceTuningPanel->setBounds(180,75,640,460);
 pwmSelector.setBounds(98,123,20,53);lfoSelector.setBounds(279,273,20,54);
 presets.setBounds(344,17,257,22);previous.setBounds(317,17,24,22);next.setBounds(604,17,24,22);
 const std::array<juce::Rectangle<int>,5> toolbarBounds{{{656,17,35,22},{695,17,45,22},{745,17,91,22},{841,17,45,22},{891,17,54,22}}};
 for(std::size_t i=0;i<toolbarButtons.size();++i)toolbarButtons[i].setBounds(toolbarBounds[i]);
 panic.setBounds(91,313,84,20);panic.setName("panic");panic.getProperties().set("arpPanel",true);hideKeyboard.setBounds(185,375,90,22);
 for(const auto& c:layout) {
  const auto i=static_cast<std::size_t>(c.parameter);
  if(c.kind==1)buttons[i].setBounds(c.x-10,c.y,20,43);
  else if(c.kind==2)buttons[i].setBounds(c.x-13,c.y,26,39);
  else if(c.kind==3)menus[static_cast<std::size_t>(c.parameter-27)].setBounds(c.x-12,c.y,49,21);
  else sliders[i].setBounds(c.x-21,c.y,42,91);
 }
 for(std::size_t i=0;i<5;++i)arpButtons[i].setBounds(725,239+static_cast<int>(i)*20,65,18);
 arpButtons[5].setBounds(798,257,34,30);arpButtons[6].setBounds(835,257,34,30);
 arpSliders[0].setBounds(868,250,21,58);arpSliders[1].setBounds(915,250,21,58);arpSliders[2].setBounds(946,248,33,77);
 arpPattern.setBounds(798,320,138,20);
 const std::array<int,talclone::effectParameterCount> centres{{303,344,385,434,475,518,565,610,653,696,750,803,856,963}};
 for(std::size_t i=0;i<centres.size();++i){
  if(i==talclone::fxLock||i==talclone::delaySync)effectButtons[i].setBounds(centres[i]-19,390,38,21);
  else effectSliders[i].setBounds(centres[i]-18,382,36,36);
 }
 portamentoButtons[0].setBounds(392,242,54,21);portamentoButtons[1].setBounds(392,325,54,20);
 portamentoSliders[0].setBounds(392,269,21,49);portamentoSliders[1].setBounds(459,251,42,91);
 sliders[19].setBounds(670,253,42,91);
 keyboard.setBounds(284,426,699,132);keyboard.setKeyWidth(699.f/22.f);
}
void TalCloneEditor::paint(juce::Graphics& g) {
 g.fillAll(juce::Colour(0xff242523));
 for(int y=0;y<570;y+=2){g.setColour(juce::Colour(0xffb8b8ad).withAlpha(y%6==0?0.025f:0.01f));g.drawHorizontalLine(y,0,1000);}
 g.setColour(juce::Colour(0xff111211));g.drawRect(0,0,1000,570,3);
 label(g,"KODA-JUNO-60",13,8,308,36,30);
 if(synthProcessor.parameters.getRawParameterValue("p46")->load()>=.5f){const auto target=synthProcessor.getMidiLearnTarget();label(g,target.isEmpty()?"MIDI LEARN: touch a control, then move a MIDI CC":"MIDI LEARN: "+target,656,40,329,11,8);}
 g.setColour(juce::Colour(0xff242523));for(int y=15;y<40;y+=4)g.fillRect(15,y,305,1);
 auto panel=[&](int x,int y,int w,int h,const char* title,juce::Colour colour,bool disabled=false){
  g.setColour(juce::Colour(0xff77776e).withAlpha(disabled?0.25f:0.7f));g.drawRect(x,y+18,w,h-18,1);
  g.setColour(colour.withAlpha(disabled?0.42f:1));g.fillRect(x,y,w,17);g.fillRect(x,y+h-3,w,3);
  label(g,title,x,y,w,17,12);
 };
 const juce::Colour red(0xff87352e),blue(0xff285782),orange(0xff9a5229);
 panel(12,53,313,147,"DCO",red);panel(327,53,58,147,"HPF",red);
 panel(387,53,264,147,"VCF",red);panel(653,53,48,147,"VCA",red);
 panel(703,53,181,147,"ADSR",red);panel(886,53,99,147,"CHORUS",red);
 panel(183,207,194,145,"LFO",red);panel(379,207,133,145,"PORTAMENTO",orange);
 panel(514,207,201,145,"CONTROL",blue);panel(717,207,268,145,"ARPEGGIATOR",blue);
 panel(12,376,163,180,"MASTER",juce::Colour(0xffb6b59d));
 g.setColour(juce::Colour(0xff252521));g.setFont(juce::Font(juce::FontOptions(12,juce::Font::bold)));
 g.drawText("MASTER",12,376,163,17,juce::Justification::centred);
 for(const auto& c:layout)if(c.kind==0||c.kind==2){
  if(c.parameter!=3&&c.parameter!=4&&c.parameter!=7)label(g,labelFor(c.parameter),c.x-26,c.y-19,52);
  if(c.kind==0){g.setColour(juce::Colour(0xff7b7d73));g.setFont(juce::Font(juce::FontOptions(6.5f)));
   g.drawText("10",c.x-24,c.y,12,10,juce::Justification::centred);
   g.drawText("5",c.x-24,c.y+39,12,10,juce::Justification::centred);
   g.drawText("0",c.x-24,c.y+79,12,10,juce::Justification::centred);}
 }
 auto wave=[&g](float x,float y,int shape){juce::Path path;
  if(shape==0){path.startNewSubPath(x,y+6);path.lineTo(x,y);path.lineTo(x+5,y);path.lineTo(x+5,y+6);path.lineTo(x+10,y+6);path.lineTo(x+10,y);}
  else if(shape==1){path.startNewSubPath(x,y+6);path.lineTo(x+11,y);path.lineTo(x+11,y+6);}
  else if(shape==2){path.startNewSubPath(x,y+6);path.lineTo(x+3,y);path.lineTo(x+6,y+3);path.lineTo(x+9,y+3);path.lineTo(x+12,y+6);}
  else if(shape==3){path.startNewSubPath(x,y);path.lineTo(x+3,y+6);path.lineTo(x+6,y+3);path.lineTo(x+9,y+3);path.lineTo(x+12,y);}
  else {path.startNewSubPath(x,y+4);path.lineTo(x+3,y);path.lineTo(x+8,y+6);path.lineTo(x+12,y+2);}
  g.setColour(juce::Colour(0xffb8bab2));g.strokePath(path,juce::PathStrokeType(1.1f));
 };
 auto bracket=[&g](float x,float y,float right,float bottom){g.setColour(juce::Colour(0xffa9aba4));g.drawLine(x,y,right,y,1);g.drawLine(x,y,x,bottom,1);g.drawLine(right,y,right,y+6,1);};
 bracket(77,85,143,119);g.drawLine(109,85,109,122,1);bracket(203,85,250,92);
 wave(138,108,0);wave(168,108,1);wave(198,108,0);label(g,"SUB",191,116,24,9,6);
 wave(119,119,4);wave(119,159,2);bracket(493,85,531,115);wave(506,121,2);wave(506,151,3);
 wave(681,116,2);wave(681,143,0);
 label(g,"LFO",119,128,22,10,7);label(g,"MAN",119,146,22,10,7);label(g,"ENV",119,167,22,10,7);
 
 label(g,"ENV",680,124,19,10,7);label(g,"GATE",680,151,19,10,7);
 label(g,"TRIG",271,249,39,10,8);label(g,"MODE",271,259,39,10,8);label(g,"KEY",299,277,23,10,7);label(g,"FREE",299,294,23,10,7);label(g,"SYNC",299,312,23,10,7);
 label(g,"SHAPE",325,254,48);label(g,"CLOCK",325,332,48,10,7);
 label(g,"LEVEL",665,235,42);
 g.setColour(juce::Colour(0xff738a51));g.fillRoundedRectangle(12,207,163,63,1.5f);
 g.setColour(juce::Colour(0xff252c20));g.setFont(juce::Font(juce::FontOptions(16,juce::Font::bold)));
 g.drawFittedText(synthProcessor.getProgramName(synthProcessor.getCurrentProgram()),22,215,142,22,juce::Justification::centredLeft,1);
 g.setFont(juce::Font(juce::FontOptions(9)));g.drawText("6 VOICES / KODA",22,245,125,15,juce::Justification::centredLeft);
 g.setColour(synthProcessor.isSustainActive()?juce::Colour(0xffa4d96a):juce::Colour(0xff414b37));g.fillEllipse(160,249,7,7);
 // Remaining unavailable controls are drawn as inactive placeholders.
 auto dummy=[&](const char* text,int x,int y,int w=48){g.setColour(juce::Colour(0xff363733));g.fillRect(x,y,w,20);
  g.setColour(juce::Colour(0xff73776a));g.setFont(juce::Font(juce::FontOptions(9)));g.drawText(text,x,y,w,20,juce::Justification::centred);};
 dummy("KEY TRANS",12,287,74);dummy("POLY 6",91,287,84);dummy("VOICE N",12,313,74);
 label(g,"TIME",459,235,42,12,8);
 label(g,"II",417,268,27,12,8);label(g,"I",417,287,27,12,8);label(g,"OFF",417,307,27,12,8);
 const float amount=synthProcessor.parameters.getRawParameterValue("portamentoAmount")->load();
 label(g,juce::String(amount*100,0)+"%",459,338,42,10,7);
 label(g,"MODE",869,232,40,12,8);label(g,"RANGE",910,232,36,12,8);label(g,"TEMPO",943,232,38,12,8);
 label(g,"UP",889,249,26,12,7);label(g,"UP&D",889,271,26,12,7);label(g,"DOWN",889,294,27,12,7);
 label(g,"3",936,249,10,12,7);label(g,"2",936,271,10,12,7);label(g,"1",936,294,10,12,7);
 const bool synced=synthProcessor.parameters.getRawParameterValue("arpSync")->load()>=.5f;
 const float rate=synthProcessor.parameters.getRawParameterValue("arpRate")->load();
 const juce::String tempo=synced?juce::String(talclone::Arpeggiator::divisions[static_cast<std::size_t>(talclone::Arpeggiator::division(rate))]):juce::String(talclone::Arpeggiator::freeFrequency(rate),2)+" Hz";
 label(g,tempo,798,300,68,15,10);
 const bool ext=synthProcessor.parameters.getRawParameterValue("arpExternal")->load()>=.5f;
 const bool midi=synthProcessor.parameters.getRawParameterValue("arpMidiClock")->load()>=.5f;
 const bool host=synthProcessor.parameters.getRawParameterValue("arpHost")->load()>=.5f;
 label(g,synced?(ext?"EXT16":midi?"MIDI":host?"HOST":"BPM"):"FREE",798,239,69,12,8);
 label(g,"0.6.1 / KODA",12,357,166,13,9);label(g,"KODA-JUNO-60",12,402,163,15,8);
 label(g,"UNISON",285,355,121,13,10);label(g,"DELAY",455,355,260,13,10);label(g,"REVERB",727,355,257,13,10);
 const std::array<int,talclone::effectParameterCount> centres{{303,344,385,434,475,518,565,610,653,696,750,803,856,963}};
 const std::array<const char*,talclone::effectParameterCount> titles{{"VOICES","DET","WIDTH","","WET","TIME","","SPREAD","TONE","FDB","WET","SIZE","DELAY","TONE"}};
 for(std::size_t i=0;i<centres.size();++i){
  if(i==talclone::fxLock||i==talclone::delaySync)continue;
  label(g,titles[i],centres[i]-24,370,48,12,8);
  const float v=synthProcessor.parameters.getRawParameterValue(talclone::effectSpecs[i].id)->load();
  juce::String value;
  if(i==talclone::unisonVoices)value=juce::String(talclone::SynthDSP::unisonVoiceCount(v));
  else if(i==talclone::delayTime){
   const bool sync=synthProcessor.parameters.getRawParameterValue("delaySync")->load()>=.5f;
   value=sync?juce::String(talclone::StereoDelay::divisions[static_cast<std::size_t>(juce::jlimit(0,20,static_cast<int>(v*20+.5f)))]):juce::String(talclone::StereoDelay::delaySeconds(v,false,120)*1000,0)+"ms";
  }else value=juce::String(v*100,0)+"%";
  label(g,value,centres[i]-24,416,48,10,7);
 }

}
