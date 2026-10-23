#include "../Source/DSP/StereoDelay.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using talclone::StereoDelay;
static void silence(StereoDelay& d,int n) { while(n--) {float l=0,r=0; d.process(l,r);} }
static std::pair<int,int> impulse(StereoDelay& d,int n) {
    int a=-1,b=-1;
    for(int i=0;i<n;++i) {float l=i==0 ? 1.f : 0.f,r=l; d.process(l,r); if(a<0 && std::abs(l)>1.e-6f)a=i; if(b<0 && std::abs(r)>1.e-6f)b=i; assert(std::isfinite(l)&&std::isfinite(r));}
    return {a,b};
}
int main() {
    assert(std::abs(StereoDelay::delaySeconds(0,false,120)-.04)<1.e-6);
    assert(std::abs(StereoDelay::delaySeconds(1,false,120)-4.)<1.e-5);
    assert(std::abs(StereoDelay::delaySeconds(.3f,true,120)-.5)<1.e-7);
    assert(StereoDelay::delaySeconds(1,true,1)==4.);
    for(double rate:{8000.,44100.,48000.,96000.}) {
        StereoDelay d; d.prepare(rate); StereoDelay::Settings s; s.wet=1; s.sync=true; s.time=.3f;
        d.setParameters(s); silence(d,int(rate*2));
        auto hit=impulse(d,int(rate));
        assert(std::abs(hit.first-int(rate*.5))<=int(rate*.001)); assert(hit.first==hit.second);
        d.reset(); s.spread=0; d.setParameters(s); silence(d,int(rate*2));
        hit=impulse(d,int(rate)); assert(std::abs(hit.first-int(rate*.25))<=int(rate*.001)); assert(std::abs(hit.second-int(rate*.5))<=int(rate*.001));
        d.reset(); s.spread=.5f; s.feedback=.7f; d.setParameters(s); silence(d,int(rate*2));
        float energy=0;
        for(int i=0;i<int(rate*2);++i) {float l=i==0 ? 1.f : 0.f,r=l; d.process(l,r); if(i>int(rate*.9))energy+=l*l;}
        assert(energy>1.e-6f);
        s.wet=0; d.setParameters(s); s.wet=1; d.setParameters(s);
        for(int i=0;i<int(rate);++i) {float l=0,r=0; d.process(l,r); assert(l==0&&r==0);}
    }
    StereoDelay d; d.prepare(std::numeric_limits<double>::infinity());
    StereoDelay::Settings s; s.wet=1; s.time=std::numeric_limits<float>::infinity(); s.spread=-100; s.tone=std::numeric_limits<float>::quiet_NaN(); s.feedback=10; d.setParameters(s,-1);
    auto hit=impulse(d,200000); assert(hit.first>=0);
    d.prepare(1.e20); d.prepare(-1); silence(d,1000);
    std::cout << "delay impulse, sync, spread, feedback, clearing and hostile controls passed\n";
}
