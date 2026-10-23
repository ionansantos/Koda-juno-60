// Isolated TAL AU state/MIDI probe: no audio device or persistent settings writes.
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>
#include <CoreFoundation/CoreFoundation.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
static void check(OSStatus e,const char* s){if(e){std::fprintf(stderr,"%s %d\n",s,int(e));std::exit(2);}}
int main(int argc,char** argv){AudioComponentDescription d{kAudioUnitType_MusicDevice,'uno4','TOGU',0,0};auto c=AudioComponentFindNext(nullptr,&d);if(!c)return 2;AudioUnit u{};check(AudioComponentInstanceNew(c,&u),"instantiate");
 CFPropertyListRef p{};UInt32 n=sizeof(p);check(AudioUnitGetProperty(u,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&p,&n),"state");
 if(argc>1){std::ifstream in(argv[1]);std::string xml((std::istreambuf_iterator<char>(in)),{});if(xml.empty())return 3;std::vector<unsigned char> bytes(8+xml.size()+1);std::memcpy(bytes.data(),"VC2!",4);auto len=static_cast<unsigned>(xml.size());std::memcpy(bytes.data()+4,&len,4);std::memcpy(bytes.data()+8,xml.data(),xml.size());auto data=CFDataCreate(nullptr,bytes.data(),bytes.size());auto copy=CFDictionaryCreateMutableCopy(nullptr,0,(CFDictionaryRef)p);CFDictionarySetValue(copy,CFSTR("jucePluginState"),data);check(AudioUnitSetProperty(u,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&copy,sizeof(copy)),"load");CFRelease(data);CFRelease(copy);}
 if(argc>4){AudioStreamBasicDescription fmt{};fmt.mSampleRate=44100;fmt.mFormatID=kAudioFormatLinearPCM;fmt.mFormatFlags=kAudioFormatFlagIsFloat|kAudioFormatFlagIsPacked|kAudioFormatFlagIsNonInterleaved;fmt.mBytesPerPacket=fmt.mBytesPerFrame=4;fmt.mFramesPerPacket=1;fmt.mChannelsPerFrame=2;fmt.mBitsPerChannel=32;check(AudioUnitSetProperty(u,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Output,0,&fmt,sizeof(fmt)),"format");check(AudioUnitInitialize(u),"init");check(MusicDeviceMIDIEvent(u,0xb0+std::atoi(argv[2])-1,std::atoi(argv[3]),std::atoi(argv[4]),0),"cc");float l[256]{},r[256]{};struct {UInt32 count;AudioBuffer buffers[2];} b{2,{{1,sizeof(l),l},{1,sizeof(r),r}}};AudioTimeStamp t{};t.mFlags=kAudioTimeStampSampleTimeValid;AudioUnitRenderActionFlags flags=0;for(int i=0;i<4;++i){t.mSampleTime=i*256;check(AudioUnitRender(u,&flags,&t,0,256,(AudioBufferList*)&b),"render");}CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.1,false);Float32 v{};check(AudioUnitGetParameter(u,10,kAudioUnitScope_Global,0,&v),"getcutoff");std::fprintf(stderr,"cutoffparam=%g\n",v);AudioUnitUninitialize(u);}
 CFRelease(p);n=sizeof(p);check(AudioUnitGetProperty(u,kAudioUnitProperty_ClassInfo,kAudioUnitScope_Global,0,&p,&n),"result");auto data=CFPropertyListCreateData(nullptr,p,kCFPropertyListXMLFormat_v1_0,0,nullptr);std::fwrite(CFDataGetBytePtr(data),1,CFDataGetLength(data),stdout);CFRelease(data);CFRelease(p);AudioComponentInstanceDispose(u);}
