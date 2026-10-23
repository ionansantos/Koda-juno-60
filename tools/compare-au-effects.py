#!/usr/bin/env python3
"""Offline real-AU effects measurements. No installs, GUI or waveform-equivalence claim.
Raw files are little-endian float32 LR interleaved. Compare each effect to its own
engine's dry control: the two instruments' dry oscillators/envelopes differ.
"""
import argparse, hashlib, json, math, os, pathlib, re, subprocess, time
import numpy as np

ENGINES = [('tal', 'TOGU', 'uno4'), ('koda', 'InAu', 'TuCl')]
NAMES = {
 'tal': ['Unisono Voices','Unisono Detune','Unisono Spread','FX Lock','FX Delay Dry / Wet','FX Delay Time','FX Delay Sync','FX Delay Spread','FX Delay Tone','FX Delay Feedback','FX Reverb Dry / Wet','FX Reverb Size','FX Reverb Delay','FX Reverb Tone'],
 'koda': ['Unison Voices','Unison Detune','Unison Width','FX Lock','Delay Wet','Delay Time','Delay Sync','Delay Spread','Delay Tone','Delay Feedback','Reverb Wet','Reverb Size','Reverb Delay','Reverb Tone']}
KEYS = ['voices','detune','width','lock','dw','dt','sync','spread','tone','feedback','rw','size','predelay','rtone']
DEFAULT = dict(zip(KEYS,[0,.1,0,0,0,.5,0,.5,.5,0,0,.8,0,.5]))
BASE = {
 'tal': {'master octave transpose':.5,'keytranspose':.5,'master fine tune':.5,'master volume':.35,'dco saw enabled':1,'dco pulse enabled':0,'dco sub osc enabled':0,'dco sub osc volume':0,'dco noise volume':0,'filter cutoff':1,'filter resonance':0,'filter env':0,'filter modulation':0,'filter keyboard':0,'vca mode':0,'attack':0,'decay':0,'sustain':1,'release':0,'chorus 1':0,'chorus 2':0,'arp enabled':0},
 'koda': {'TRANSPOSE':.5,'MASTERPITCH':.5,'VOLUME':.35,'SAW':1,'Pulse':0,'SUBOSC':0,'SUBOSCVOLUME':0,'NOISEVOLUME':0,'CUTOFF':1,'RESONANCE':0,'FILTERCONTOUR':0,'FILTERLFO':0,'KEYFOLLOW':0,'AMPATTACK':0,'AMPDECAY':0,'AMPSUSTAIN':1,'AMPRELEASE':0,'ENVGATE':0,'CHORUS1':0,'CHORUS2':0,'Arp On':0}}

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def rms(x): return float(np.sqrt(np.mean(np.square(x,dtype=np.float64)))) if x.size else 0.
def centroid(x,sr):
    if len(x)<16 or rms(x)<1e-12:return None
    y=x.mean(axis=1); sp=np.abs(np.fft.rfft(y*np.hanning(len(y))))**2
    return float(np.sum(np.fft.rfftfreq(len(y),1/sr)*sp)/max(1e-30,sp.sum()))
def bands(x,sr):
    y=x.mean(axis=1); sp=np.abs(np.fft.rfft(y*np.hanning(len(y))))**2; hz=np.fft.rfftfreq(len(y),1/sr)
    return {f'{lo}-{hi}':float(sp[(hz>=lo)&(hz<hi)].sum()/max(1e-30,sp.sum())) for lo,hi in [(20,500),(500,3000),(3000,20000)]}
def metrics(x,sr,gate):
    energy=np.mean(x.astype(np.float64)**2,axis=1); win=max(1,int(sr*.002)); env=np.sqrt(np.convolve(energy,np.ones(win)/win,'same'))
    threshold=max(1e-9,float(env.max())*.01); indices=np.flatnonzero(env>threshold)
    active=x[:int((gate+.05)*sr)]; tail=x[int((gate+.15)*sr):]
    mid=(x[:,0]+x[:,1])*.5;side=(x[:,0]-x[:,1])*.5
    result={'rms':rms(x),'peak':float(np.abs(x).max()),'nonfinite':not bool(np.isfinite(x).all()),'silent':rms(x)<1e-8,
      'onset_seconds':float(indices[0]/sr) if len(indices) else None,'last_above_minus40db_seconds':float(indices[-1]/sr) if len(indices) else None,
      'active_rms':rms(active),'tail_rms':rms(tail),'side_mid_ratio':rms(side)/max(1e-20,rms(mid)),
      'tail_centroid_hz':centroid(tail,sr),'tail_band_energy_fraction':bands(tail,sr),
      'tail_rms_windows':[{ 'start':t,'rms':rms(x[int(t*sr):int((t+.25)*sr)])} for t in np.arange(.25,len(x)/sr-.25,.25)]}
    # A frequency summary around MIDI 60, kept separate from waveform matching.
    segment=x[:min(len(x),int(gate*sr))].mean(axis=1) if gate>=1 else np.zeros(16); fftsize=2**int(math.ceil(math.log2(max(len(segment)*4,16384))))
    sp=np.abs(np.fft.rfft(segment*np.hanning(len(segment)),n=fftsize)); hz=np.fft.rfftfreq(fftsize,1/sr)
    region=np.flatnonzero((hz>175)&(hz<375)); peaks=[i for i in region if sp[i]>sp[i-1] and sp[i]>=sp[i+1]]
    peaks=sorted((i for i in peaks if sp[i]>.08*max(sp[region]) and max(sp[region])>1e-10),key=lambda i:sp[i],reverse=True)[:9]
    result['fundamental_peaks_hz']=[float(hz[i]) for i in peaks]
    result['spectral_peak_note']='Windowed spectral candidates, including possible sidelobes; not a voice-count estimate.'
    result['fundamental_peak_ratios']=[float(hz[i]/261.6255653) for i in peaks]
    return result

def echo_metrics(wet,dry,sr,expected,gate):
    # Full wet cases remove direct signal; no subtraction across independent synth instances.
    # Correlation with the same engine's dry note tolerates oscillator phase differences.
    result=[]
    for channel in range(2):
      signal=wet[:,channel]; ref=dry[:int((gate+.03)*sr),channel]
      size=1<<int(math.ceil(math.log2(len(signal)+len(ref))))
      correlation=np.fft.irfft(np.fft.rfft(signal,size)*np.conj(np.fft.rfft(ref,size)),size)[:len(signal)]
      found=[]
      for repeat in range(1,5):
        want=expected[channel]*repeat; lo=max(int((want-.025)*sr),0);hi=min(int((want+.025)*sr),len(signal))
        if hi<=lo:continue
        index=lo+int(np.argmax(np.abs(correlation[lo:hi])))
        level=rms(signal[max(0,index-int(.01*sr)):min(len(signal),index+len(ref))])
        found.append({'detected_above_minus60db':level>max(1e-9,rms(signal)*.001),'repeat':repeat,'expected_seconds':want,'correlation_peak_seconds':index/sr,'local_rms':level})
      result.append(found)
    return result

def cases():
    result=[('dry',{},.08,6),('unison-dry',{},2,3)]
    for count in [.01,.25,.5,.75,1]:result.append((f'unison-voices-{count}',{'voices':count,'detune':.5,'width':0},2,3))
    for det in [.1,1]:result.append((f'unison-detune-{det}',{'voices':1,'detune':det,'width':0},2,3))
    result.append(('unison-wide',{'voices':1,'detune':.5,'width':1},2,3))
    for t in [0,.3,.6]:result.append((f'delay-time-{t}',{'dw':1,'dt':t},.08,6))
    result += [('delay-sync',{'dw':1,'dt':.3,'sync':1},.08,6),('delay-spread-left',{'dw':1,'dt':.3,'spread':0},.08,6),('delay-spread-right',{'dw':1,'dt':.3,'spread':1},.08,6),('delay-feedback',{'dw':1,'dt':.3,'feedback':.6},.08,6),('delay-dark',{'dw':1,'dt':.3,'tone':0},.08,6),('delay-bright',{'dw':1,'dt':.3,'tone':1},.08,6),('delay-half-wet',{'dw':.5,'dt':.3},.08,6)]
    result += [('reverb',{'rw':1},.08,8),('reverb-predelay',{'rw':1,'predelay':.5},.08,8),('reverb-small',{'rw':1,'size':.2},.08,8),('reverb-large',{'rw':1,'size':1},.08,12),('reverb-dark',{'rw':1,'rtone':0},.08,8),('reverb-bright',{'rw':1,'rtone':1},.08,8),('reverb-half-wet',{'rw':.5},.08,8),('serial',{'voices':.5,'detune':.2,'width':.8,'dw':.5,'dt':.3,'feedback':.4,'rw':.7,'size':.8,'predelay':.2},.08,10)]
    result += [('serial-dry',{'voices':.5,'detune':.2,'width':.8},.08,6),('serial-sustained-dry',{'voices':.5,'detune':.2,'width':.8},2,8),('serial-sustained',{'voices':.5,'detune':.2,'width':.8,'dw':.5,'dt':.3,'feedback':.4,'rw':.7,'size':.8,'predelay':.2},2,10)]
    return result

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--probe',required=True,type=pathlib.Path);ap.add_argument('--output',type=pathlib.Path,default=pathlib.Path('evidence/lx-effects/audio'));ap.add_argument('--bundle',type=pathlib.Path);ap.add_argument('--sample-rate',type=int,default=44100);ap.add_argument('--block-size',type=int,default=256);ap.add_argument('--engine',choices=['both','tal','koda'],default='both');ap.add_argument('--case',action='append',help='Exact case names; dry and unison-dry controls are always rendered');args=ap.parse_args()
    root=pathlib.Path(__file__).resolve().parent.parent;out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    bundle=(args.bundle or root/'build/TalUNo60Clone_artefacts/Release/AU/KODA-JUNO-60.component').resolve()
    bins={'tal':pathlib.Path('/Library/Audio/Plug-Ins/Components/TAL-U-NO-LX-V2.component/Contents/MacOS/TAL-U-NO-LX-V2'),'koda':bundle/'Contents/MacOS/KODA-JUNO-60'}
    selected=[e for e in ENGINES if args.engine in ('both',e[0])];maps={};sr=args.sample_rate
    report={'scope':'Real AU offline render; engine-specific dry baselines. Measures effect timing, spectrum, stereo and tails; does not establish waveform equivalence, DAW GUI operation or preset LOCK semantics. Oscillator initial phase may vary between instances.','sample_rate':sr,'block_size':args.block_size,'preroll_seconds':1,'midi_note':60,'velocity':100,'bpm':120,'format':'float32 little-endian LR interleaved','probe_sha256':sha(args.probe),'identities':{k:{'path':str(bins[k]),'sha256':sha(bins[k])} for k,_,_ in selected},'cases':[]}
    def environment(kind,gate=.08,duration=6):
      env=os.environ.copy();env.pop('KODA_PROBE_BUNDLE',None)
      env.update(KODA_PROBE_RATE=str(sr),KODA_PROBE_BLOCK=str(args.block_size),KODA_PROBE_PREROLL='1',KODA_PROBE_DURATION=str(duration),KODA_PROBE_GATE=str(gate),KODA_PROBE_NOTE='60',KODA_PROBE_BPM='120')
      if kind=='koda':env['KODA_PROBE_BUNDLE']=str(bundle)
      return env
    for kind,mfr,sub in selected:
      process=subprocess.run([str(args.probe),mfr,sub],env=environment(kind),capture_output=True,text=True,check=True)
      (out/f'{kind}-parameters.txt').write_text(process.stdout);maps[kind]={m.group(2):m.group(1) for m in re.finditer(r'^PARAM (\d+) (.*?) \[',process.stdout,re.M)}
      missing=set(BASE[kind])|set(NAMES[kind]);missing-=maps[kind].keys()
      if missing:raise RuntimeError(f'{kind}: missing parameter contracts: {sorted(missing)}')
    controls={};started=time.monotonic()
    for name,changes,gate,duration in cases():
      if args.case and name not in args.case and name not in ('dry','unison-dry'):continue
      print(name,flush=True);item={'name':name,'normalized_effect_parameters':DEFAULT|changes,'gate_seconds':gate,'duration_seconds':duration,'tracks':{}}
      for kind,mfr,sub in selected:
        values=BASE[kind]|{n:(DEFAULT|changes)[key] for key,n in zip(KEYS,NAMES[kind])};path=out/f'{name}-{kind}.raw'
        command=[str(args.probe),mfr,sub]+[f'{maps[kind][n]}={v}' for n,v in values.items()]+['--render',str(path)]
        run=subprocess.run(command,env=environment(kind,gate,duration),capture_output=True,text=True)
        (out/f'{name}-{kind}.log').write_text(run.stdout+run.stderr)
        if run.returncode:raise RuntimeError(f'{name}/{kind}: probe failed: {run.stderr}')
        flat=np.fromfile(path,dtype='<f4')
        if len(flat)!=round(sr*duration)*2 or not np.isfinite(flat).all():raise RuntimeError(f'{name}/{kind}: invalid sample count or nonfinite')
        x=flat.reshape(-1,2);m=metrics(x,sr,gate)
        if m['silent']:raise RuntimeError(f'{name}/{kind}: unexpected silence')
        track={'audio_path':str(path),'audio_sha256':sha(path),'parameters':values,'metrics':m}
        if name in ('dry','unison-dry'):controls[(kind,name)]=x
        dry=controls[(kind,'unison-dry' if name.startswith('unison') else 'dry')]
        track['active_rms_relative_to_own_dry']=m['active_rms']/max(1e-20,rms(dry[:int((gate+.05)*sr)]))
        if name.startswith('delay'):
          v=DEFAULT|changes;r=math.exp(2.9957323*v['dt'])*.052105263-.042105265
          delay=4*r if not v['sync'] else .5
          spread=v['spread'];expected=[max(.04,delay*(spread+.5 if spread<.5 else 1)),max(.04,delay*(1 if spread<.5 else 1.5-spread))]
          track['echo_measurements']=echo_metrics(x,dry,sr,expected,gate)
        if name.startswith('reverb'):
          weton=m['onset_seconds'];dryon=metrics(dry,sr,.08)['onset_seconds'];track['onset_relative_to_dry_seconds']=weton-dryon if weton is not None and dryon is not None else None
        item['tracks'][kind]=track
      if len(item['tracks'])==2:
        a=item['tracks']['tal']['metrics'];b=item['tracks']['koda']['metrics']
        item['comparison']={'onset_delta_seconds':b['onset_seconds']-a['onset_seconds'] if a['onset_seconds'] is not None and b['onset_seconds'] is not None else None,'tail_rms_ratio_koda_tal':b['tail_rms']/max(1e-20,a['tail_rms']),'side_mid_ratio_delta':b['side_mid_ratio']-a['side_mid_ratio'],'tail_centroid_delta_hz':b['tail_centroid_hz']-a['tail_centroid_hz'] if a['tail_centroid_hz'] is not None and b['tail_centroid_hz'] is not None else None}
      report['cases'].append(item);report['elapsed_seconds']=time.monotonic()-started
      (out/'results.json').write_text(json.dumps(report,indent=2,allow_nan=False))
    measurements={c['name']:c for c in report['cases']}
    observations={}
    for kind,_,_ in selected:
      checks={}
      def m(name):return measurements[name]['tracks'][kind]['metrics']
      if all(n in measurements for n in ['delay-time-0','delay-time-0.3','delay-time-0.6']):
        checks['delay_free_time_order']=m('delay-time-0')['onset_seconds']<m('delay-time-0.3')['onset_seconds']<m('delay-time-0.6')['onset_seconds']
      if 'delay-sync' in measurements:checks['quarter_note_sync_onset_within_10ms']=abs(m('delay-sync')['onset_seconds']-.5)<.01
      if all(n in measurements for n in ['unison-wide','unison-voices-1']):checks['unison_width_creates_stereo']=m('unison-wide')['side_mid_ratio']>m('unison-voices-1')['side_mid_ratio']+.1
      if all(n in measurements for n in ['reverb','reverb-predelay']):checks['reverb_predelay_delta_within_15ms']=abs(m('reverb-predelay')['onset_seconds']-m('reverb')['onset_seconds']-.25)<.015
      for effect in ['delay','reverb']:
        if all(n in measurements for n in [effect+'-dark',effect+'-bright']):checks[effect+'_bright_tail_centroid_exceeds_dark']=m(effect+'-bright')['tail_centroid_hz']>m(effect+'-dark')['tail_centroid_hz']
      if all(n in measurements for n in ['reverb-small','reverb-large']):
        def late(name):return sum(w['rms'] for w in m(name)['tail_rms_windows'] if 2<=w['start']<=3)
        checks['reverb_large_longer_late_tail']=late('reverb-large')>late('reverb-small')
      observations[kind]=checks
    report['behavior_checks']=observations
    report['behavior_checks_scope']='Parameter-response checks on each instrument, not pairwise sound equivalence or real DAW integration.'
    (out/'results.json').write_text(json.dumps(report,indent=2,allow_nan=False))
    print(json.dumps({'cases':len(report['cases']),'elapsed_seconds':report['elapsed_seconds'],'output':str(out/'results.json')},indent=2))
if __name__=='__main__':main()
