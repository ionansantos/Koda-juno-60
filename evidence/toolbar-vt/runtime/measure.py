import os,re,sys,json,hashlib,subprocess,copy,xml.etree.ElementTree as E
from pathlib import Path
import numpy as np
root=Path(__file__).resolve().parents[3];sys.path.insert(0,str(root/'tools'));from importlib import import_module
fx=import_module('compare-au-effects');out=Path(__file__).parent;probe=Path('/tmp/toolbar-vt-audio-probe');bundle=root/'build/TalUNo60Clone_artefacts/Release/AU/KODA-JUNO-60.component';sr=48000
cases=[('neutral',None,.5),('detune-low','detune',0),('detune-high','detune',1),('pan-left','pan',0),('pan-right','pan',1),('pan-quarter','pan',.25),('pan-threequarter','pan',.75),('cutoff-low','cutoff',0),('cutoff-high','cutoff',1),('attack-low','attack',0),('attack-high','attack',1),('decay-low','decay',0),('decay-high','decay',1),('release-low','release',0),('release-high','release',1)]
report={'scope':'Actual native AU offline rendering. All six voice tuning cells equal per case; mono note69,48000Hz,256frames,preroll1s,gate1.5s,duration3s. Compare control response to own neutral engine, not timbre equivalence. TALstateimport exactXML; KODAparameterAPI.','sample_rate':sr,'cases':[],'identities':{}}
for engine,mfr,sub in fx.ENGINES:
 env=os.environ.copy();env.pop('KODA_PROBE_XML',None);env.pop('KODA_PROBE_BUNDLE',None)
 if engine=='koda':env['KODA_PROBE_BUNDLE']=str(bundle)
 inventory=subprocess.run([str(probe),mfr,sub],env=env,capture_output=True,check=True,text=True).stdout; (out/f'{engine}-parameters.txt').write_text(inventory)
 params={name:int(id) for id,name in re.findall(r'^PARAM (\d+) (.+?) \[',inventory,re.M)}
 binary=Path('/Library/Audio/Plug-Ins/Components/TAL-U-NO-LX-V2.component/Contents/MacOS/TAL-U-NO-LX-V2') if engine=='tal' else bundle/'Contents/MacOS/KODA-JUNO-60';report['identities'][engine]={'path':str(binary),'sha256':fx.sha(binary)}
 for name,key,value in cases:
  env.update(KODA_PROBE_RATE=str(sr),KODA_PROBE_BLOCK='256',KODA_PROBE_PREROLL='1',KODA_PROBE_DURATION='3',KODA_PROBE_GATE='1.5',KODA_PROBE_NOTE='69')
  selected=copy.copy(fx.BASE[engine]);
  if engine=='koda':selected['Save CPU']=0
  selected['vca mode' if engine=='tal' else 'ENVGATE']=1 if engine=='tal' else 0;selected.update({'filter cutoff' if engine=='tal' else 'CUTOFF':.5 if key=='cutoff' else 1})
  for k in ['voices','dw','rw']:selected[fx.NAMES[engine][fx.KEYS.index(k)]]=0
  if key=='attack':selected['attack' if engine=='tal' else 'AMPATTACK']=.5
  if key=='decay':selected['decay' if engine=='tal' else 'AMPDECAY']=.5;selected['sustain' if engine=='tal' else 'AMPSUSTAIN']=0
  if key=='release':selected['release' if engine=='tal' else 'AMPRELEASE']=.5
  if engine=='tal':
   r=E.parse(root/'evidence/toolbar-midi/tal-au-state.xml').getroot()
   for v in r.findall('programs/program/voicetunings/voicetuning'):
    for k in ['cutoff','detune','attack','decay','release','pan']:v.set(k,str(value if k==key else .5))
   file=out/f'{engine}-{name}.xml';file.write_bytes(E.tostring(r));env['KODA_PROBE_XML']=str(file)
  else:
   for voice in range(1,7):
    for k in ['Cutoff','Detune','Attack','Decay','Release','Pan']:selected[f'Voice {voice} {k}']=value if k.lower()==key else .5
  raw=out/f'{engine}-{name}.f32';cmd=[str(probe),mfr,sub]+[f'{params[k]}={v}' for k,v in selected.items()]+['--render',str(raw)];run=subprocess.run(cmd,env=env,capture_output=True,check=True,text=True);(out/f'{engine}-{name}.log').write_text(run.stderr)
  x=np.fromfile(raw,dtype='<f4').reshape(-1,2);segment=x[int(.5*sr):int(1.25*sr)];power=np.mean(segment.astype(float)**2,axis=0);y=segment.mean(axis=1);n=2**19;spec=np.abs(np.fft.rfft(y*np.hanning(len(y)),n=n));freq=np.fft.rfftfreq(n,1/sr);indices=np.flatnonzero((freq>350)&(freq<550));i=indices[np.argmax(spec[indices])];a,b,c=np.log(np.maximum(spec[i-1:i+2],1e-30));d=.5*(a-c)/(a-2*b+c);pitch=(i+d)*sr/n
  spectral=(spec**2);centroid=float(np.sum(freq*spectral)/sum(spectral));ms=np.mean(x.astype(float)**2,axis=1);win=240;envelope=np.sqrt(np.convolve(ms,np.ones(win)/win,'same'));peak=float(envelope.max());above=np.flatnonzero(envelope>peak*.7);decaylast=np.flatnonzero(envelope[:int(1.5*sr)]>peak*.1);releaseabove=np.flatnonzero(envelope[int(1.5*sr):]>peak*.1)
  result={'engine':engine,'case':name,'control':key,'value':value,'parameters':selected,'raw_path':str(raw),'raw_sha256':fx.sha(raw),'finite':bool(np.isfinite(x).all()),'peak':float(np.max(np.abs(x))),'rms_lr':np.sqrt(power).tolist(),'pitch_hz':float(pitch),'centroid_hz':centroid,'attack70_seconds':float(above[0]/sr) if len(above) else None,'decay_minus20db_seconds':float(decaylast[-1]/sr) if len(decaylast) else None,'release_minus20db_seconds':float(releaseabove[-1]/sr) if len(releaseabove) else None};report['cases'].append(result);print(engine,name,round(pitch,4),np.sqrt(power),flush=True)
report['runner_sha256']=fx.sha(Path(__file__));report['probe_binary_sha256']=fx.sha(probe);report['probe_source_sha256']=fx.sha(root/'tools/toolbar-vt-audio-probe.cpp');(out/'results.json').write_text(json.dumps(report,indent=2)+'\n')
