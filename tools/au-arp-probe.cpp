// Offline probe for the explicitly selected TAL or KODA AU. No audio device or UI.
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
#include <cstdint>
static void check(OSStatus e,const char* where){if(e){std::fprintf(stderr,"%s: %d\n",where,int(e));std::exit(1);}}
static UInt32 four(const char* s){return (UInt32(s[0])<<24)|(UInt32(s[1])<<16)|(UInt32(s[2])<<8)|UInt32(s[3]);}
static void loadXmlPreset(AudioUnit unit,const char* path){
 FILE* file=std::fopen(path,"rb");if(!file){std::perror(path);std::exit(1);}
 std::fseek(file,0,SEEK_END);const long size=std::ftell(file);std::fseek(file,0,SEEK_SET);
 if(size<=0){std::fprintf(stderr,"empty preset\n");std::exit(1);}
 std::string xml(static_cast<std::size_t>(size),'\0');if(std::fread(xml.data(),1,xml.size(),file)!=xml.size()){std::perror(path);std::exit(1);}std::fclose(file);
 std::vector<std::uint8_t> blob(8+xml.size()+1);const std::uint32_t magic=0x21324356u;const std::uint32_t length=static_cast<std::uint32_t>(xml.size()+1);
 std::memcpy(blob.data(),&magic,4);std::memcpy(blob.data()+4,&length,4);std::memcpy(blob.data()+8,xml.data(),xml.size());blob.back()=0;
 CFDataRef data=CFDataCreate(nullptr,blob.data(),static_cast<CFIndex>(blob.size()));
 CFPropertyListRef current=nullptr;UInt32 currentSize=sizeof(current);
 AudioUnitGetProperty(unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&current,&currentSize);
 CFMutableDictionaryRef dict=current?CFDictionaryCreateMutableCopy(nullptr,0,static_cast<CFDictionaryRef>(current)):CFDictionaryCreateMutable(nullptr,1,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
 CFDictionarySetValue(dict,CFSTR("jucePluginState"),data);
 CFPropertyListRef value=dict;
 OSStatus e=AudioUnitSetProperty(unit,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&value,sizeof(value));
 CFRelease(dict);CFRelease(data);if(current)CFRelease(current);
 if(e){std::fprintf(stderr,"preset load failed: %d\n",int(e));std::exit(1);}
}
struct Host { double sample=0,rate=48000; bool playing=true; };
static OSStatus beat(void* data,Float64* position,Float64* bpm){auto& h=*static_cast<Host*>(data);if(position)*position=h.sample*2/h.rate;if(bpm)*bpm=120;return noErr;}
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
 const char* output=nullptr;const char* presetPath=nullptr;bool dumpValues=false;int programChange=-1;
 std::vector<std::pair<UInt32,float>> selected;
 for(int i=3;i<argc;++i){if(std::strcmp(argv[i],"--render")==0&&i+1<argc){output=argv[++i];continue;}if(std::strcmp(argv[i],"--preset")==0&&i+1<argc){presetPath=argv[++i];continue;}if(std::strcmp(argv[i],"--dump")==0){dumpValues=true;continue;}if(std::strcmp(argv[i],"--program")==0&&i+1<argc){programChange=std::atoi(argv[++i]);continue;}
  char* end=nullptr;auto id=std::strtoul(argv[i],&end,10);if(end&&*end=='=')selected.emplace_back(static_cast<UInt32>(id),std::strtof(end+1,nullptr));
 }
 if(output){
  const double rate=std::getenv("KODA_PROBE_RATE")?std::strtod(std::getenv("KODA_PROBE_RATE"),nullptr):48000;
  AudioStreamBasicDescription fmt{};fmt.mSampleRate=rate;fmt.mFormatID=kAudioFormatLinearPCM;fmt.mFormatFlags=kAudioFormatFlagIsFloat|kAudioFormatFlagIsPacked|kAudioFormatFlagIsNonInterleaved;fmt.mBytesPerPacket=fmt.mBytesPerFrame=4;fmt.mFramesPerPacket=1;fmt.mChannelsPerFrame=2;fmt.mBitsPerChannel=32;
  check(AudioUnitSetProperty(unit,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Output,0,&fmt,sizeof(fmt)),"format");
  UInt32 block=256;check(AudioUnitSetProperty(unit,kAudioUnitProperty_MaximumFramesPerSlice,kAudioUnitScope_Global,0,&block,sizeof(block)),"block size");
  Host host;host.rate=rate;HostCallbackInfo callbacks{};callbacks.hostUserData=&host;callbacks.beatAndTempoProc=beat;callbacks.transportStateProc=transport;
  check(AudioUnitSetProperty(unit,kAudioUnitProperty_HostCallbacks,kAudioUnitScope_Global,0,&callbacks,sizeof(callbacks)),"host callbacks");
  check(AudioUnitInitialize(unit),"initialize");
  if(presetPath)loadXmlPreset(unit,presetPath);
  for(const auto& p:selected)check(AudioUnitSetParameter(unit,p.first,kAudioUnitScope_Global,0,p.second,0),"set param");
  if(dumpValues)for(auto id:ids){AudioUnitParameterInfo info{};bytes=sizeof(info);AudioUnitGetProperty(unit,kAudioUnitProperty_ParameterInfo,kAudioUnitScope_Global,id,&info,&bytes);char name[256]{};if(info.flags&kAudioUnitParameterFlag_HasCFNameString)CFStringGetCString(info.cfNameString,name,sizeof(name),kCFStringEncodingUTF8);else std::strncpy(name,info.name,255);Float32 value=0;AudioUnitGetParameter(unit,id,kAudioUnitScope_Global,0,&value);std::printf("VALUE %u %s %g\n",id,name,value);}
  struct StereoBuffers{UInt32 count;AudioBuffer buffers[2];} list{};list.count=2;std::array<float,256> left{},right{};
  for(int c=0;c<2;++c){list.buffers[c].mNumberChannels=1;list.buffers[c].mDataByteSize=1024;list.buffers[c].mData=c?right.data():left.data();}
  auto* f=std::fopen(output,"wb");if(!f){std::perror(output);return 1;}
  const std::string scenario=std::getenv("KODA_PROBE_SCENARIO")?std::getenv("KODA_PROBE_SCENARIO"):"free";
  int clockIndex=0;const int total=static_cast<int>(rate*6),noteOff=static_cast<int>(rate*4);
  for(int b=0;b*256<total;++b){host.sample=b*256.;if(scenario=="host-stop")host.playing=b*256<noteOff;
   if(b==0&&programChange>=0)check(MusicDeviceMIDIEvent(unit,0xc0,static_cast<UInt32>(programChange),0,0),"program change");
   if(b==1){for(int n:{60,64,67})check(MusicDeviceMIDIEvent(unit,0x90,static_cast<UInt32>(n),100,0),"note on");
    if(scenario=="midi")check(MusicDeviceMIDIEvent(unit,0xfa,0,0,0),"MIDI start");}
   if(scenario=="ext"||scenario=="midi"){
    const double interval=rate/(scenario=="ext"?8:48);
    while(true){const int sample=256+static_cast<int>(std::llround(clockIndex*interval));if(sample>=(b+1)*256)break;
     if(sample>=b*256)check(MusicDeviceMIDIEvent(unit,scenario=="ext"?0x9f:0xf8,scenario=="ext"?1:0,scenario=="ext"?100:0,static_cast<UInt32>(sample-b*256)),"clock trigger");
     ++clockIndex;
    }
   }
   AudioTimeStamp t{};t.mFlags=kAudioTimeStampSampleTimeValid;t.mSampleTime=b*256.;AudioUnitRenderActionFlags flags=0;
   if(noteOff>=b*256&&noteOff<(b+1)*256)for(int n:{60,64,67})check(MusicDeviceMIDIEvent(unit,0x80,static_cast<UInt32>(n),0,static_cast<UInt32>(noteOff-b*256)),"note off");
   check(AudioUnitRender(unit,&flags,&t,0,256,reinterpret_cast<AudioBufferList*>(&list)),"render");
   for(int i=0;i<std::min(256,total-b*256);++i){const float v=(left[static_cast<std::size_t>(i)]+right[static_cast<std::size_t>(i)])*.5f;if(!std::isfinite(v)){std::fprintf(stderr,"nonfinite audio\n");return 1;}std::fwrite(&v,sizeof(v),1,f);}
  }
  std::fclose(f);AudioUnitUninitialize(unit);
 }
 check(AudioComponentInstanceDispose(unit),"dispose");
 if(bundle)CFRelease(bundle);
}
