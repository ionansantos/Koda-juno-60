// VT audio comparison probe. Optional JUCE ClassInfo XML; isolated instance, no audio device.
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>
#include <CoreFoundation/CoreFoundation.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <iterator>
static void check(OSStatus e,const char* where){if(e){std::fprintf(stderr,"%s: %d\n",where,int(e));std::exit(1);}}
static UInt32 four(const char* s){return (UInt32(s[0])<<24)|(UInt32(s[1])<<16)|(UInt32(s[2])<<8)|UInt32(s[3]);}
struct Host { double sample=0,rate=48000,bpm=120; bool playing=true; };
static OSStatus beat(void* data,Float64* position,Float64* bpm){auto& h=*static_cast<Host*>(data);if(position)*position=h.sample*h.bpm/(60*h.rate);if(bpm)*bpm=h.bpm;return noErr;}
static OSStatus transport(void* data,Boolean* playing,Boolean* changed,Float64* sample,Boolean* cycling,Float64* start,Float64* end){
 auto& h=*static_cast<Host*>(data);if(playing)*playing=h.playing;if(changed)*changed=false;if(sample)*sample=h.sample;if(cycling)*cycling=false;if(start)*start=0;if(end)*end=0;return noErr;
}
int main(int argc,char** argv){
 if(argc<3){std::fprintf(stderr,"usage: probe manufacturer subtype [id=value ...] [--render output.raw]\n");return 2;}
 AudioComponentDescription desc{};desc.componentType=kAudioUnitType_MusicDevice;desc.componentManufacturer=four(argv[1]);desc.componentSubType=four(argv[2]);
 AudioComponent component=nullptr;
 CFBundleRef bundle=nullptr;
 if(const auto* path=std::getenv("KODA_PROBE_BUNDLE")){
  const auto url=CFURLCreateFromFileSystemRepresentation(nullptr,reinterpret_cast<const UInt8*>(path),static_cast<CFIndex>(std::strlen(path)),true);
  bundle=CFBundleCreate(nullptr,url);CFRelease(url);
  if(!bundle||!CFBundleLoadExecutable(bundle)){std::fprintf(stderr,"Could not load selected bundle\n");return 1;}
  const auto factory=reinterpret_cast<AudioComponentFactoryFunction>(CFBundleGetFunctionPointerForName(bundle,CFSTR("KODA_JUNO_60AUFactory")));
  if(!factory){std::fprintf(stderr,"KODA AU factory missing\n");return 1;}
  component=AudioComponentRegister(&desc,CFSTR("KODA test build"),0x300,factory);
 }else component=AudioComponentFindNext(nullptr,&desc);
 if(!component){std::fprintf(stderr,"AU not registered\n");return 1;}
 AudioUnit unit=nullptr;check(AudioComponentInstanceNew(component,&unit),"instantiate");
 UInt32 bytes=0;check(AudioUnitGetPropertyInfo(unit,kAudioUnitProperty_ParameterList,kAudioUnitScope_Global,0,&bytes,nullptr),"parameter size");
 std::vector<AudioUnitParameterID> ids(bytes/sizeof(AudioUnitParameterID));check(AudioUnitGetProperty(unit,kAudioUnitProperty_ParameterList,kAudioUnitScope_Global,0,ids.data(),&bytes),"parameters");
 for(auto id:ids){AudioUnitParameterInfo info{};bytes=sizeof(info);check(AudioUnitGetProperty(unit,kAudioUnitProperty_ParameterInfo,kAudioUnitScope_Global,id,&info,&bytes),"info");
  char name[256]{};if(info.flags&kAudioUnitParameterFlag_HasCFNameString)CFStringGetCString(info.cfNameString,name,sizeof(name),kCFStringEncodingUTF8);else std::strncpy(name,info.name,255);
  std::printf("PARAM %u %s [%g,%g] default %g\n",id,name,info.minValue,info.maxValue,info.defaultValue);
  if((info.flags&kAudioUnitParameterFlag_CFNameRelease)&&info.cfNameString)CFRelease(info.cfNameString);
 }
 const char* output=nullptr;
 std::vector<std::pair<UInt32,float>> selected;
 for(int i=3;i<argc;++i){if(std::strcmp(argv[i],"--render")==0&&i+1<argc){output=argv[++i];continue;}
  char* end=nullptr;auto id=std::strtoul(argv[i],&end,10);if(end&&*end=='=')selected.emplace_back(static_cast<UInt32>(id),std::strtof(end+1,nullptr));
 }
 if(output){
  const double rate=std::getenv("KODA_PROBE_RATE")?std::strtod(std::getenv("KODA_PROBE_RATE"),nullptr):48000;
  AudioStreamBasicDescription fmt{};fmt.mSampleRate=rate;fmt.mFormatID=kAudioFormatLinearPCM;fmt.mFormatFlags=kAudioFormatFlagIsFloat|kAudioFormatFlagIsPacked|kAudioFormatFlagIsNonInterleaved;fmt.mBytesPerPacket=fmt.mBytesPerFrame=4;fmt.mFramesPerPacket=1;fmt.mChannelsPerFrame=2;fmt.mBitsPerChannel=32;
  check(AudioUnitSetProperty(unit,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Output,0,&fmt,sizeof(fmt)),"format");
  auto number=[](const char* name,double fallback){const auto* s=std::getenv(name);if(!s)return fallback;char* end=nullptr;const auto value=std::strtod(s,&end);if(end==s||*end||!std::isfinite(value)){std::fprintf(stderr,"invalid %s\n",name);std::exit(2);}return value;};
  const auto blockValue=number("KODA_PROBE_BLOCK",256);if(blockValue<1||blockValue>8192||std::floor(blockValue)!=blockValue){std::fprintf(stderr,"invalid block size\n");return 2;}const auto block=static_cast<UInt32>(blockValue);
  const double preroll=number("KODA_PROBE_PREROLL",1),duration=number("KODA_PROBE_DURATION",6),gate=number("KODA_PROBE_GATE",.08);
  const int note=static_cast<int>(number("KODA_PROBE_NOTE",60));
  if(rate<8000||rate>192000||block<1||block>8192||preroll<0||preroll>120||!std::isfinite(rate)||duration<=gate||duration>120||gate<=0||note<0||note>127){std::fprintf(stderr,"invalid render configuration\n");return 2;}
  check(AudioUnitSetProperty(unit,kAudioUnitProperty_MaximumFramesPerSlice,kAudioUnitScope_Global,0,&block,sizeof(block)),"block size");
  Host host;host.rate=rate;host.bpm=number("KODA_PROBE_BPM",120);HostCallbackInfo callbacks{};callbacks.hostUserData=&host;callbacks.beatAndTempoProc=beat;callbacks.transportStateProc=transport;
  check(AudioUnitSetProperty(unit,kAudioUnitProperty_HostCallbacks,kAudioUnitScope_Global,0,&callbacks,sizeof(callbacks)),"host callbacks");
  check(AudioUnitInitialize(unit),"initialize");
  if(const auto* xmlPath=std::getenv("KODA_PROBE_XML")){
   std::ifstream in(xmlPath);std::string xml((std::istreambuf_iterator<char>(in)),{});if(xml.empty())return 3;
   CFPropertyListRef saved{};UInt32 size=sizeof(saved);check(AudioUnitGetProperty(unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&saved,&size),"readclass");
   auto copy=CFDictionaryCreateMutableCopy(nullptr,0,reinterpret_cast<CFDictionaryRef>(saved));std::vector<unsigned char> encoded(8+xml.size()+1);std::memcpy(encoded.data(),"VC2!",4);const auto length=static_cast<UInt32>(xml.size());std::memcpy(encoded.data()+4,&length,4);std::memcpy(encoded.data()+8,xml.data(),xml.size());auto data=CFDataCreate(nullptr,encoded.data(),encoded.size());CFDictionarySetValue(copy,CFSTR("jucePluginState"),data);check(AudioUnitSetProperty(unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&copy,sizeof(copy)),"writeclass");CFRelease(data);CFRelease(copy);CFRelease(saved);
  }
  for(const auto& p:selected)check(AudioUnitSetParameter(unit,p.first,kAudioUnitScope_Global,0,p.second,0),"set param");
  struct StereoBuffers{UInt32 count;AudioBuffer buffers[2];} list{};list.count=2;std::vector<float> left(block),right(block),interleaved(block*2);
  for(int c=0;c<2;++c){list.buffers[c].mNumberChannels=1;list.buffers[c].mDataByteSize=block*4;list.buffers[c].mData=c?right.data():left.data();}
  auto* f=std::fopen(output,"wb");if(!f){std::perror(output);return 1;}
  const int preBlocks=static_cast<int>(std::ceil(preroll*rate/block));
  const int total=static_cast<int>(std::llround(rate*duration)),off=static_cast<int>(std::llround(rate*gate));
  for(int b=-preBlocks;b*static_cast<int>(block)<total;++b){
   const int start=b*static_cast<int>(block);host.sample=(b+preBlocks)*double(block);
   if(b==0)check(MusicDeviceMIDIEvent(unit,0x90,static_cast<UInt32>(note),100,0),"note on");
   if(off>=start&&off<start+static_cast<int>(block))check(MusicDeviceMIDIEvent(unit,0x80,static_cast<UInt32>(note),0,static_cast<UInt32>(off-start)),"note off");
   AudioTimeStamp t{};t.mFlags=kAudioTimeStampSampleTimeValid;t.mSampleTime=host.sample;AudioUnitRenderActionFlags flags=0;
   check(AudioUnitRender(unit,&flags,&t,0,block,reinterpret_cast<AudioBufferList*>(&list)),"render");
   for(UInt32 i=0;i<block;++i){if(!std::isfinite(left[i])||!std::isfinite(right[i])){std::fprintf(stderr,"nonfinite audio\n");return 1;}interleaved[i*2]=left[i];interleaved[i*2+1]=right[i];}
   if(b>=0){const auto frames=static_cast<std::size_t>(std::min(static_cast<int>(block),total-start));if(std::fwrite(interleaved.data(),sizeof(float)*2,frames,f)!=frames){std::perror("write");return 1;}}
  }
  std::fclose(f);AudioUnitUninitialize(unit);
 }
 check(AudioComponentInstanceDispose(unit),"dispose");
 if(bundle)CFRelease(bundle);
}
