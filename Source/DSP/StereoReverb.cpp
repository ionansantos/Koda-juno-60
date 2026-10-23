#include "StereoReverb.h"
#include <algorithm>
#include <cmath>
namespace talclone {
namespace {
float normalized(float x,float fallback=0) noexcept { return std::isfinite(x)?std::clamp(x,0.f,1.f):fallback; }
float finite(float x) noexcept { return std::isfinite(x)?x:0.f; }
std::size_t prime(std::size_t n) { n=std::max(n,std::size_t(2)); for(;;++n){bool ok=true;for(std::size_t i=2;i<=n/i;++i)if(n%i==0){ok=false;break;}if(ok)return n;} }
constexpr float lengths[]={42.70625305f,44.48352051f,46.33474731f,48.26301193f,50.27152634f,52.36363220f,54.54279327f,56.81264877f,59.17696381f,61.63967133f,64.20487213f,66.87681580f,69.65996552f,72.55893707f,75.57855225f,78.72383118f,82.f};
constexpr float damping[]={.696981375f,.686588705f,.675928288f,.665000185f,.653805115f,.642344489f};
}
void StereoReverb::Filter::configure(double rate,double frequency,bool highpass) noexcept {
    const double w=6.283185307179586*std::clamp(frequency,1.,rate*.49)/rate,c=std::cos(w),alpha=std::sin(w)/(2*.68),inv=1/(1+alpha);
    b0=(highpass?(1+c):(1-c))*.5*inv;b1=(highpass?-(1+c):(1-c))*inv;b2=b0;a1=-2*c*inv;a2=(1-alpha)*inv;
}
float StereoReverb::Filter::tick(float input) noexcept {
    // Reference computes from the preceding input before shifting in this sample.
    const double out=b0*previous+z1;z1=b1*previous-a1*out+z2;z2=b2*previous-a2*out;previous=input;
    if(!std::isfinite(out)){clear();return 0;}return static_cast<float>(out);
}
void StereoReverb::Branch::clear() noexcept {
    std::fill(buffer.begin(),buffer.end(),0);position=0;damp=0;seed=initialSeed;ramp=0;step=initialStep;
}
float StereoReverb::Branch::tick(float input,float size,float sign) noexcept {
    if(ramp>=1 || ramp<=-1 || step==0){seed*=16807u;const float magnitude=1.f/((float(seed&0x7fffffffu)*3.4924597e-10f+.25f)*period);step=ramp>=1?-magnitude:magnitude;}
    ramp+=step;
    const float delay=std::max(1.f,float(buffer.size()-2)*(size*.495f+.01f*sign*(ramp*.5f+.5f))+1);
    const auto integer=static_cast<std::size_t>(delay);const auto index=(position+buffer.size()-integer)%buffer.size();const auto previous=(index+buffer.size()-1)%buffer.size();
    const float tap=buffer[index]+(buffer[previous]-buffer[index])*(delay-float(integer));
    damp=finite(tap+(damp-tap)*coefficient*.05f);buffer[position]=finite(input+size*.99f*damp);if(++position==buffer.size())position=0;return tap;
}
float StereoReverb::Allpass::tick(float input) noexcept {const float old=buffer[position];const float next=finite(input+old*.68f);buffer[position]=next;if(++position==buffer.size())position=0;return finite(old-next*.68f);}
void StereoReverb::prepare(double rate) {
    sampleRate_=std::isfinite(rate)?std::clamp(rate,8000.,384000.):44100.;predelay_.assign(prime(static_cast<std::size_t>(sampleRate_)),0);
    for(std::size_t channel=0;channel<2;++channel){for(std::size_t i=0;i<6;++i){auto& b=branches_[channel][i];const double factor=channel==0?(i%2==0?.0020160002:.0020100002):(i%2==0?.002:.0020260802);b.buffer.assign(prime(static_cast<std::size_t>(float(sampleRate_)*float(factor)*lengths[i])),0);b.initialSeed=123321u+std::uint32_t(channel*7919+i*3571);b.period=std::max(1.f,float(int(float(sampleRate_)*(float(i)/60.f+1.f)*( .36f*88200.f+1000.f)/44100.f)));// The initializer sets slope before the first process call changes the
        // period/ramp for fixed q=.4; that call deliberately retains this slope.
        const float initialPeriod=std::max(1.f,float(int(float(sampleRate_)*2.2675737e-05f*(float(i)*1433.3334f+86000.f))));
        b.initialStep=1.f/((float(b.initialSeed&0x7fffffffu)*3.4924597e-10f+.25f)*initialPeriod);b.coefficient=damping[i];}for(std::size_t i=0;i<11;++i)allpasses_[channel][i].buffer.assign(prime(static_cast<std::size_t>(float(sampleRate_)*.00015f*lengths[i+5])),0);}
    configured_=false;setParameters(settings_);reset();
}
void StereoReverb::reset() noexcept {std::fill(predelay_.begin(),predelay_.end(),0);position_=0;highpass_.clear();lowpass_.clear();for(auto& channel:branches_)for(auto& b:channel)b.clear();for(auto& channel:allpasses_)for(auto& a:channel){std::fill(a.buffer.begin(),a.buffer.end(),0);a.position=0;}clearPending_=false;}
void StereoReverb::setParameters(Settings s) noexcept {
    s.wet=normalized(s.wet);s.size=normalized(s.size,.8f);s.delay=normalized(s.delay);s.tone=normalized(s.tone,.5f);
    if(configured_&&settings_.wet==s.wet&&settings_.size==s.size&&settings_.delay==s.delay&&settings_.tone==s.tone)return;
    if(s.wet==0&&settings_.wet>0)clearPending_=true;
    settings_=s;configured_=true;dry_=s.wet<.5f?1:2-2*s.wet;wet_=.5f*s.wet*s.wet;size_=1.0510527f-.051052634f*std::exp(2.9957323f*(1-s.size));
    if(!predelay_.empty())tap_=std::size_t(float(predelay_.size()-2)*s.delay*s.delay+1);
    const float lp=s.tone<=.5f?std::pow(1.5f*s.tone+.25f,4.f):1.f,hp=s.tone>.5f?std::pow(1.5f*s.tone-.75f,4.f):0;
    highpass_.configure(sampleRate_,10+20000*hp,true);lowpass_.configure(sampleRate_,10+20000*lp,false);
}
void StereoReverb::process(float& left,float& right) noexcept {
    left=finite(left);right=finite(right);if(wet_==0||predelay_.empty())return;if(clearPending_)reset();
    const float excitation=predelay_[(position_+predelay_.size()-tap_)%predelay_.size()];predelay_[position_]=(left+right)*.25f;if(++position_==predelay_.size())position_=0;
    const float input=lowpass_.tick(highpass_.tick(excitation));float output[2]={0,0};
    for(std::size_t channel=0;channel<2;++channel){for(std::size_t i=0;i<6;++i){const float sign=i%2==0?1.f:-1.f;output[channel]+=sign*branches_[channel][i].tick(input,size_,sign);}for(auto& a:allpasses_[channel])output[channel]=a.tick(output[channel]);}
    left=finite(dry_*left+wet_*output[0]);right=finite(dry_*right+wet_*output[1]);
}
}
