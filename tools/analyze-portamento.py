#!/usr/bin/env python3
"""Real installed TAL AU portamento event/pitch measurements; no plugin writes."""
import pathlib, subprocess, os, json, hashlib, time, re, runpy
import numpy as np
ROOT=pathlib.Path(__file__).resolve().parent.parent
OUT=pathlib.Path(os.environ.get('KODA_PORT_OUTPUT',str(ROOT/'evidence/portamento/runtime')));OUT.mkdir(parents=True,exist_ok=True)
PROBE=pathlib.Path('/tmp/koda-portamento-probe');SR=int(os.environ.get('KODA_PORT_SR','44100'))
ENGINE=os.environ.get('KODA_PORT_ENGINE','tal')
BIN=pathlib.Path('/Library/Audio/Plug-Ins/Components/TAL-U-NO-LX-V2.component/Contents/MacOS/TAL-U-NO-LX-V2')
BUNDLE=pathlib.Path(os.environ.get('KODA_PROBE_BUNDLE',str(ROOT/'build/TalUNo60Clone_artefacts/Release/AU/KODA-JUNO-60.component')))
if ENGINE=='koda': BIN=BUNDLE/'Contents/MacOS/KODA-JUNO-60'
assert ENGINE in ('tal','koda')
HOST=('InAu','TuCl') if ENGINE=='koda' else ('TOGU','uno4')
probe_env=os.environ.copy()
if ENGINE=='koda':probe_env['KODA_PROBE_BUNDLE']=str(BUNDLE)
else:probe_env.pop('KODA_PROBE_BUNDLE',None)
listing=subprocess.run([str(PROBE),*HOST],env=probe_env,capture_output=True,text=True,check=True).stdout
PARAM_IDS={m.group(2).strip():int(m.group(1)) for m in re.finditer(r'^PARAM (\d+) (.*?) \[',listing,re.M)}
KODA_BASE=runpy.run_path(str(ROOT/'tools/compare-au-effects.py'))['BASE']['koda']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
BASE={0:0,1:0,2:0,4:0,5:1,6:0,7:0,8:0,9:0,10:1,11:0,13:0,14:0,15:0,16:.35,17:.5,18:.5,19:0,20:0,21:0,22:1,23:0,30:0,31:0,32:0,38:0,39:0,44:0,48:5/11,49:.5,51:0,54:0,55:0,58:0,62:0,69:0,72:0}
def pitch(x):
 result=[];n=1536
 for start in range(0,len(x)-n,220):
  s=x[start:start+n].astype(float);s-=s.mean();ac=np.fft.irfft(abs(np.fft.rfft(s,4096))**2)[:n];ac/=np.arange(n,0,-1)
  inds=np.arange(65,400);peaks=inds[(ac[inds]>ac[inds-1])&(ac[inds]>=ac[inds+1])]
  if not len(peaks) or np.max(ac[peaks])<=0 or np.sqrt(np.mean(s*s))<1e-6:continue
  best=peaks[ac[peaks]>=.85*np.max(ac[peaks])][0];a,b,c=ac[best-1:best+2];lag=best+.5*(a-c)/(a-2*b+c);hz=SR/lag
  result.append([round((start+n/2)/SR,6),round(69+12*np.log2(hz/440),4)])
 return result
cases=[]
for mode in [0,.5,1]:
 for poly in [0,1]:
  for overlap in [False,True]:
   cases.append((f'mode{mode}-poly{poly}-legato{int(overlap)}',mode,poly,0,.5,12,overlap))
for clock in [0,1]:
 for intensity in [.1,.25,.5,.75,1]:
  for interval in [5,12,-5,-12]:cases.append((f'time{clock}-amount{intensity}-interval{interval}',.5,0,clock,intensity,interval,True))
for mode in [.5,1]:cases.append((f'poly-reuse-mode{mode}',mode,1,0,.5,12,False))
for mode in [.5,1]:cases.append((f'mono-return-mode{mode}',mode,0,0,.5,12,True))
for gap in [0,1]:cases.append((f'poly-six-reuse-gap{gap}',1,1,0,.5,12,bool(gap)))
if os.environ.get('KODA_PORT_SELECTED'):cases=[c for c in cases if c[0] in os.environ['KODA_PORT_SELECTED'].split(',')]
report={'engine':ENGINE,'scope':'Real selected AU offline render; saw only, sustained envelope, effects off. Autocorrelation trajectory estimates can confuse simultaneous pitches (poly cases). No inference of internal formulas from measurements alone.','sample_rate':SR,'block':256,'preroll_seconds':1,'binary':{'path':str(BIN),'sha256':sha(BIN)},'probe_sha256':sha(PROBE),'cases':[]}
start=time.monotonic()
for name,mode,poly,clock,intensity,interval,overlap in cases:
 events=[[0,144,60,100],[.98 if not overlap else 1.08,128,60,0],[1,144,60+interval,100],[6.8,128,60+interval,0]];events.sort(key=lambda e:e[0])
 duration=7
 if name.startswith('poly-reuse'):
  events=[]
  for i,n in enumerate([60,62,64,65,67,69,71,72]):events.extend([[i,144,n,100],[i+.8,128,n,0]])
  duration=8.5
 if name.startswith('poly-six-reuse'):
  notes=[48,52,55,60,64,67];events=[[0,144,n,100] for n in notes]
  events.extend([[1.98 if overlap else 2.05,128,n,0] for n in notes]);events.extend([[2,144,72,100],[4,128,72,0]]);events.sort(key=lambda e:e[0]);duration=4.5
 if name.startswith('mono-return'):
  events=[[0,144,60,100],[1,144,72,100],[2,128,72,0],[3.5,128,60,0]];duration=4
 params=BASE|{42:mode,43:intensity,53:poly,56:clock}
 if ENGINE=='koda':
  named=KODA_BASE|{'Portamento Mode':mode,'Portamento Amount':intensity,'Portamento Poly':poly,'Portamento Time':clock,'Unison Voices':0,'Delay Wet':0,'Reverb Wet':0}
  params={PARAM_IDS[n]:v for n,v in named.items()}
 raw=OUT/(name+'.raw');env=probe_env.copy()
 env.update(KODA_PROBE_RATE=str(SR),KODA_PROBE_DURATION=str(duration),KODA_PROBE_PREROLL='1',KODA_PORT_EVENTS=';'.join(','.join(map(str,e)) for e in events))
 run=subprocess.run([str(PROBE),*HOST]+[f'{k}={v}' for k,v in params.items()]+['--render',str(raw)],env=env,capture_output=True,text=True,check=True)
 (OUT/(name+'.log')).write_text(run.stdout+run.stderr)
 audio=np.fromfile(raw,dtype='<f4').reshape(-1,2);assert np.isfinite(audio).all() and np.max(abs(audio))>1e-5
 trajectory=pitch(audio.mean(axis=1));pts=np.array(trajectory);summ={}
 for t in [1.025,1.05,1.1,1.2,1.4,1.8,2.5,3.5,5.5]:
  near=pts[abs(pts[:,0]-t)<.015,1];summ[str(t)]=round(float(np.median(near)),3) if len(near) else None
 after=pts[(pts[:,0]>1.02)&(pts[:,0]<6.8)];target=60+interval
 reached=after[abs(after[:,1]-target)<.1];settle=float(reached[0,0]-1) if len(reached) else None
 if name.startswith('poly-reuse'):settle=None # Multiple scheduled targets; a single arrival time is undefined.
 if name.startswith('poly-six'):
  after=pts[(pts[:,0]>2.10)&(pts[:,0]<4)];reached=after[abs(after[:,1]-72)<.1];settle=float(reached[0,0]-2) if len(reached) else None
 item={'name':name,'parameters':params,'events_seconds_status_note_velocity':events,'audio_path':str(raw),'audio_sha256':sha(raw),'pitch_midi_by_time':summ,'first_within_10cents_seconds_after_second_note':settle,'trajectory':trajectory}
 report['cases'].append(item);report['elapsed_seconds']=time.monotonic()-start
 (OUT/'results.json').write_text(json.dumps(report,indent=2));print(name,summ,'settle',settle,flush=True)
