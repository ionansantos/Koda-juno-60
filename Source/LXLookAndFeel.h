#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

class LXLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    LXLookAndFeel() {
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff383837));
        setColour(juce::ComboBox::textColourId, juce::Colour(0xffc0c0b9));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff474746));
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff252524));
        setColour(juce::TextButton::buttonColourId, juce::Colour(0xff383837));
        setColour(juce::TextButton::textColourOffId, juce::Colour(0xffbabbb2));
        setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffb9bbb2));
    }
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float position,float start,float end,juce::Slider& slider) override {
        if(!static_cast<bool>(slider.getProperties()["fxKnob"])){LookAndFeel_V4::drawRotarySlider(g,x,y,w,h,position,start,end,slider);return;}
        const float cx=static_cast<float>(x)+static_cast<float>(w)*.5f,cy=static_cast<float>(y)+static_cast<float>(h)*.5f;
        const float radius=static_cast<float>(std::min(w,h))*.42f;
        g.setColour(juce::Colour(0xff111211));g.fillEllipse(cx-radius-1,cy-radius-1,2*radius+2,2*radius+2);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff4c4d47),cx-radius,cy-radius,juce::Colour(0xff282925),cx+radius,cy+radius,false));g.fillEllipse(cx-radius,cy-radius,2*radius,2*radius);
        g.setColour(juce::Colour(0xff85877c));g.drawEllipse(cx-radius,cy-radius,2*radius,2*radius,.7f);
        const float angle=start+position*(end-start);g.setColour(juce::Colour(0xffc7cbb9));
        g.drawLine(cx+std::sin(angle)*radius*.28f,cy-std::cos(angle)*radius*.28f,cx+std::sin(angle)*radius*.85f,cy-std::cos(angle)*radius*.85f,1.8f);
    }
    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
                          float pos, float, float, juce::Slider::SliderStyle style,
                          juce::Slider& slider) override {
        if (style != juce::Slider::LinearVertical) {
            LookAndFeel_V4::drawLinearSlider(g,x,y,w,h,pos,0,0,style,slider); return;
        }
        if(static_cast<bool>(slider.getProperties()["arpSelector"])) {
            const float cx=static_cast<float>(x)+static_cast<float>(w)*.5f;
            const float top=static_cast<float>(y)+4, bottom=static_cast<float>(y+h)-4;
            g.setColour(juce::Colour(0xff111211));g.fillRoundedRectangle(cx-5,top,10,bottom-top,5);
            g.setColour(juce::Colour(0xff62625c));g.drawRoundedRectangle(cx-5,top,10,bottom-top,5,.8f);
            g.setColour(juce::Colour(0xffc5c8bd));g.fillEllipse(cx-5,pos-5,10,10);return;
        }
        const float cx=static_cast<float>(x)+static_cast<float>(w)*0.5f;
        const float top=static_cast<float>(y)+5, bottom=static_cast<float>(y+h)-5;
        g.setColour(juce::Colour(0xff565653));
        for(int i=0;i<=10;++i) {
            const float yy=top+(bottom-top)*static_cast<float>(i)/10;
            g.drawLine(cx-20,yy,cx+20,yy,0.6f);
        }
        g.setColour(juce::Colours::black);g.fillRoundedRectangle(cx-2,top-2,4,bottom-top+4,1.3f);
        g.setColour(juce::Colour(0xff4a4a47));g.drawLine(cx+2.5f,top,cx+2.5f,bottom,0.8f);
        const juce::Rectangle<float> cap(cx-13,pos-5,26,10);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff4d4d4b),cx,pos-5,
            juce::Colour(0xff202021),cx,pos+5,false));g.fillRoundedRectangle(cap,1.3f);
        g.setColour(juce::Colour(0xff9a9b95));g.drawRoundedRectangle(cap,1.3f,0.9f);
        g.setColour(juce::Colour(0xffc9ccc2));g.drawLine(cx-12,pos,cx+12,pos,1.1f);
    }
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& b,bool hover,bool down) override {
        const auto bounds=b.getLocalBounds().toFloat();
        const bool on=b.getToggleState();
        if(static_cast<bool>(b.getProperties()["verticalSwitch"])) {
            const auto track=bounds.reduced(6,2);
            g.setColour(juce::Colour(0xff131313));g.fillRoundedRectangle(track,6);
            g.setColour(juce::Colour(0xff62625c));g.drawRoundedRectangle(track,6,0.8f);
            const bool upper=static_cast<bool>(b.getProperties()["invertSwitch"])?!on:on;
            const float cy=upper?track.getY()+7:track.getBottom()-7;
            g.setGradientFill(juce::ColourGradient(juce::Colour(0xffddddcc),track.getCentreX(),cy-5,
                juce::Colour(0xff8b8b83),track.getCentreX(),cy+5,false));
            g.fillEllipse(track.getCentreX()-5,cy-5,10,10);return;
        }
        const float cx=bounds.getCentreX();
        if(on){g.setColour(juce::Colour(0xffee2318).withAlpha(0.12f));g.fillEllipse(cx-9,0,18,16);}
        g.setColour(on?juce::Colour(0xffed2e20):juce::Colour(0xff550e0c));g.fillEllipse(cx-3,3,6,6);
        g.setColour(juce::Colour(0xff090908));g.drawEllipse(cx-3,3,6,6,1);
        auto colour=b.findColour(juce::ToggleButton::tickColourId);
        if(!on)colour=colour.darker(0.28f);if(hover)colour=colour.brighter(0.12f);if(down)colour=colour.darker();
        const auto cap=bounds.withTrimmedTop(13).reduced(1,1);
        g.setColour(juce::Colour(0xff10100f));g.fillRoundedRectangle(cap.expanded(1),2);
        g.setGradientFill(juce::ColourGradient(colour.brighter(0.12f),0,cap.getY(),colour,0,cap.getBottom(),false));
        g.fillRoundedRectangle(cap,1.5f);
        g.setColour(colour.brighter(0.6f));g.drawRoundedRectangle(cap.reduced(1),1.5f,0.8f);
    }
    void drawButtonBackground(juce::Graphics& g,juce::Button& button,const juce::Colour& colour,bool hover,bool down) override {
        if(!static_cast<bool>(button.getProperties()["arpPanel"])){LookAndFeel_V4::drawButtonBackground(g,button,colour,hover,down);return;}
        auto bounds=button.getLocalBounds().toFloat().reduced(.5f);
        auto fill=colour;if(hover)fill=fill.brighter(.08f);if(down)fill=fill.darker(.1f);
        g.setColour(fill);g.fillRect(bounds);g.setColour(juce::Colour(0xff555650));g.drawRect(bounds,.6f);
    }
    juce::Font getTextButtonFont(juce::TextButton& b,int h) override {
        if(static_cast<bool>(b.getProperties()["arpPanel"]))return juce::Font(juce::FontOptions(9.f));
        return LookAndFeel_V4::getTextButtonFont(b,h);
    }
    juce::Font getComboBoxFont(juce::ComboBox&) override { return juce::Font(juce::FontOptions(11.0f)); }
    void drawComboBox(juce::Graphics& g,int w,int h,bool,int,int,int,int,juce::ComboBox& box) override {
        g.setColour(box.findColour(juce::ComboBox::backgroundColourId));g.fillRoundedRectangle(0,0,static_cast<float>(w),static_cast<float>(h),1);
        g.setColour(juce::Colour(0xffb0b1a9));juce::Path p;p.addTriangle(static_cast<float>(w)-12,static_cast<float>(h)/2-2,
            static_cast<float>(w)-4,static_cast<float>(h)/2-2,static_cast<float>(w)-8,static_cast<float>(h)/2+3);g.fillPath(p);
    }
};
