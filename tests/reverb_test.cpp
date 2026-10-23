#include "DSP/StereoReverb.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>
using R=talclone::StereoReverb;
struct Response { double early=0,late=0,stereo=0;int onset=-1;std::vector<float> samples; };
Response impulse(double rate,float size,float delay=0){R r;r.prepare(rate);r.setParameters({1,size,delay,.5f});Response result;for(int i=0;i<int(rate*8);++i){float l=i==0?1.f:0.f,rr=l;r.process(l,rr);assert(std::isfinite(l)&&std::isfinite(rr));assert(std::abs(l)<8&&std::abs(rr)<8);const double e=double(l)*l+double(rr)*rr;if(e>1e-18&&result.onset<0)result.onset=i;if(i<rate)result.early+=e;if(i>rate*5)result.late+=e;result.stereo+=double(l-rr)*(l-rr);if(i<rate*2)result.samples.push_back(l);}return result;}
int main(){
 for(double rate:{8000.,44100.,48000.,96000.,384000.}){
   auto small=impulse(rate,.2f),large=impulse(rate,.9f),delayed=impulse(rate,.2f,.7f);
   assert(small.onset>0&&large.onset>small.onset);assert(small.early>0&&large.early>0);assert(small.late<small.early*.001);assert(large.late>small.late);assert(small.stereo>0);
   assert(std::abs((delayed.onset-small.onset)/rate-.49)<.002);
 }
 R r;r.prepare(48000);r.setParameters({.5f,.8f,0,.5f});float l=.4f,rr=-.1f;r.process(l,rr);assert(l==.4f&&rr==-.1f);
 // Clearing and bypass must remove an old tail on reactivation.
 for(int i=0;i<12000;++i){l=i==0?1.f:0;rr=l;r.process(l,rr);}r.setParameters({0,.8f,0,.5f});l=.3f;rr=-.2f;r.process(l,rr);assert(l==.3f&&rr==-.2f);r.setParameters({1,.8f,0,.5f});
 for(int i=0;i<20000;++i){l=rr=0;r.process(l,rr);assert(l==0&&rr==0);}
 // Reset restores deterministic network state and silence, including filters/predelay.
 auto first=impulse(48000,.8f);auto second=impulse(48000,.8f);assert(first.samples==second.samples);r.reset();for(int i=0;i<10000;++i){l=rr=0;r.process(l,rr);assert(l==0&&rr==0);}
 // Reset must reproduce a complete impulse on the same instance.
 r.prepare(44100);r.setParameters({1,.8f,.1f,.5f});std::vector<float> replay;
 for(int pass=0;pass<2;++pass){r.reset();for(int i=0;i<88200;++i){l=i==0?1.f:0;rr=l;r.process(l,rr);if(pass==0)replay.push_back(l);else assert(l==replay[std::size_t(i)]);}}
 // Tone endpoints filter away more broad-band impulse energy than the center.
 double toneEnergy[3]={0,0,0};for(int t=0;t<3;++t){r.prepare(48000);r.setParameters({1,.7f,0,float(t)*.5f});for(int i=0;i<96000;++i){l=i==0?1.f:0;rr=l;r.process(l,rr);toneEnergy[t]+=double(l)*l+double(rr)*rr;}}
 assert(toneEnergy[0]<toneEnergy[1]&&toneEnergy[2]<toneEnergy[1]);
 // Invalid host values cannot poison retained state or trigger unbounded allocations.
 for(double rate:{std::numeric_limits<double>::quiet_NaN(),-1.,1.e12}){r.prepare(rate);r.setParameters({std::numeric_limits<float>::infinity(),-10,12,std::numeric_limits<float>::quiet_NaN()});l=std::numeric_limits<float>::infinity();rr=std::numeric_limits<float>::quiet_NaN();r.process(l,rr);assert(std::isfinite(l)&&std::isfinite(rr));}
 // Silence stays silent at the maximum feedback setting.
 r.prepare(48000);r.setParameters({1,1,1,1});for(int i=0;i<200000;++i){l=rr=0;r.process(l,rr);assert(l==0&&rr==0);}
 std::cout<<"Recovered reverb impulse, decay, predelay, stereo, bypass, reset and hostile-input tests passed\n";
}
