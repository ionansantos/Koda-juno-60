#include "StereoDelay.h"
#include <algorithm>
#include <cmath>

namespace talclone {
namespace {
float control(float v, float fallback) noexcept { return std::isfinite(v) ? std::clamp(v,0.f,1.f) : fallback; }
double validTempo(double v) noexcept { return std::isfinite(v) && v>0 ? v : 120.; }
constexpr std::array<float,21> coefficients {3.75f,2.4999998f,5.625f,7.5f,4.9999995f,11.25f,15,9.999999f,22.5f,30,19.999998f,45,60,39.999996f,90,120,79.99999f,180,240,159.99998f,360};
float fraction(float t, bool sync, double bpm) noexcept {
    t=control(t,.5f);
    if(sync) return float(coefficients[std::size_t(t*20.f+.5f)] / validTempo(bpm));
    return std::exp(t*2.9957323f)*.052105263f-.042105265f;
}
}
const std::array<const char*,21> StereoDelay::divisions {"1/16","1/16T","1/8.","1/8","1/8T","1/8.","1/4","1/4T","1/4.","1/2","1/2T","1/2.","1/1","1/1T","1/1.","2/1","2/1T","2/1.","4/1","4/1T","4/1."};
double StereoDelay::delaySeconds(float t, bool sync, double bpm) noexcept { return 4.*std::clamp(fraction(t,sync,bpm),.01f,1.f); }
void StereoDelay::prepare(double sampleRate) {
    rate=std::isfinite(sampleRate) ? std::clamp(sampleRate,8000.,384000.) : 44100.;
    const auto n=std::size_t(rate*4.+1.);
    for(auto& c:channels) c.ring.assign(n,0.f);
    smoothing=float(rate)*.04535147f;
    smoothingScale=1.f/(smoothing+1.f);
    reset();
    setParameters(settings,tempo);
}
void StereoDelay::reset() noexcept {
    for(auto& c:channels) {
        std::fill(c.ring.begin(),c.ring.end(),0.f);
        c.write=0; c.smoothed=0; c.lpState=0; c.lpHalf=0; c.lpOutput=0; c.hpInput=0; c.hpOutput=0;
    }
}
void StereoDelay::setParameters(Settings s, double bpm) noexcept {
    s.wet=control(s.wet,0); s.time=control(s.time,.5f); s.spread=control(s.spread,.5f);
    s.tone=control(s.tone,.5f); s.feedback=control(s.feedback,0);
    if(s.wet==0 && wet!=0) reset();
    settings=s; tempo=validTempo(bpm); wet=s.wet; dry=wet<.5f ? 1.f : 2.f-2.f*wet;
    const float mL=s.spread<.5f ? s.spread+.5f : 1.f;
    const float mR=s.spread<.5f ? 1.f : 1.5f-s.spread;
    const float r=fraction(s.time,s.sync,tempo);
    channels[0].target=std::clamp(mL*r,.01f,1.f); channels[1].target=std::clamp(mR*r,.01f,1.f);
    const float g=s.feedback*.9949874f;
    channels[0].feedback=g*(.5f*mR+.5f); channels[1].feedback=g*(.5f*mL+.5f);
    const float correction=std::min(44100.f/float(rate),1.f);
    const float p=s.tone<=.5f ? s.tone*1.7f+.14999998f : 1.f;
    const float f=correction*p*p, a=f*.38f;
    const float b=(.9988f-f*.24662f)+(f*.71174f+.4955f)*a*a;
    const float x=f*3.141592741f*-.38f*b;
    lowpass=(-x-x*x*(x*(x*(x*.008333334f+.041666668f)+.16666667f)+.5f))*2.f;
    const float hp=s.tone*1.7f-.85f;
    highpass=s.tone<=.5f ? .999f : .999f-hp*hp*correction;
}
float StereoDelay::tick(Channel& c, float input) noexcept {
    if(std::abs(c.target-c.smoothed)>1.e-7f) c.smoothed=(c.target+smoothing*c.smoothed)*smoothingScale;
    const auto offset=std::size_t(std::floor(float(c.ring.size()-1)*c.smoothed));
    const auto tapIndex=(c.write+c.ring.size()-std::min(offset,c.ring.size()-1))%c.ring.size();
    const float tap=c.ring[tapIndex];
    const float in=input+c.feedback*tap;
    const float lp=(.5f*in-c.lpHalf)*lowpass+c.lpState;
    const float filtered=.875f*lp+.125f*c.lpOutput;
    c.lpState=lp; c.lpHalf=.5f*lp; c.lpOutput=filtered;
    const float hp=filtered-c.hpInput+c.hpOutput*highpass;
    c.hpInput=filtered; c.hpOutput=hp;
    const float u=2.f*hp, a=std::abs(u), q=a*(a+3.f)+6.f;
    c.ring[c.write]=u*q/(a*q+12.f);
    c.write=(c.write+1)%c.ring.size();
    return tap;
}
void StereoDelay::process(float& left, float& right) noexcept {
    if(wet==0 || channels[0].ring.empty()) return;
    left=std::isfinite(left) ? left : 0.f; right=std::isfinite(right) ? right : 0.f;
    left=dry*left+5.f*wet*tick(channels[0],.2f*left);
    right=dry*right+5.f*wet*tick(channels[1],.2f*right);
}
}
