#!/usr/bin/env python3
"""Render the selected installed TAL AU and built KODA AU; compare musical steps, not waveforms.
Requires numpy in the caller's Python environment. Does not install or modify either plugin.
"""
import argparse, hashlib, json, math, os, pathlib, re, subprocess
import numpy as np


def digest(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe',required=True,type=pathlib.Path)
    parser.add_argument('--output',type=pathlib.Path,default=pathlib.Path('evidence/arpeggiator/runtime/comparison'))
    parser.add_argument('--sample-rate',type=int,default=44100)
    args=parser.parse_args();sr=args.sample_rate;root=pathlib.Path(__file__).resolve().parent.parent
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    bundle=root/'build/TalUNo60Clone_artefacts/Release/AU/KODA-JUNO-60.component'
    identities={'tal':pathlib.Path('/Library/Audio/Plug-Ins/Components/TAL-U-NO-LX-V2.component/Contents/MacOS/TAL-U-NO-LX-V2'),'koda':bundle/'Contents/MacOS/KODA-JUNO-60'}
    maps={}
    for kind,mfr,sub in [('tal','TOGU','uno4'),('koda','InAu','TuCl')]:
        env=os.environ.copy()
        if kind=='koda':env['KODA_PROBE_BUNDLE']=str(bundle)
        result=subprocess.run([str(args.probe),mfr,sub],env=env,capture_output=True,text=True,check=True)
        (out/f'{kind}-parameters.txt').write_text(result.stdout)
        maps[kind]={m.group(2):m.group(1) for m in re.finditer(r'^PARAM (\d+) (.*?) \[',result.stdout,re.M)}
    cases=[]
    for mode in range(3):
        for octave in range(3):cases.append(dict(name=f'free-mode{mode}-octaves{octave+1}',mode=mode,octaves=octave,rate=.5,scenario='free'))
    for source in ['ext','midi','host']:
        cases.append(dict(name=source,mode=2,octaves=1,rate=9/17,scenario=source))
    cases.append(dict(name='hold',mode=2,octaves=0,rate=.5,scenario='free',hold=True))
    report={'sample_rate':sr,'block_size':256,'preroll_samples':256,'duration_seconds':6,'note_off_seconds':4,
            'scope':'Offline AU audio, matched saw-only settings; pitch sequence and median interval. Not waveform equivalence, real DAW UI, all sync divisions or combinations.',
            'identities':{k:{'path':str(p),'sha256':digest(p)} for k,p in identities.items()},'cases':[]}
    for case in cases:
        print(case['name'],flush=True);tracks={}
        for kind,mfr,sub in [('tal','TOGU','uno4'),('koda','InAu','TuCl')]:
            if kind=='tal':
                values={'arp enabled':1,'arp mode':case['mode']/2,'arp range':case['octaves']/2,'arp rate':case['rate'],
                        'master octave transpose':.5,'keytranspose':.5,'dco sub osc enabled':0,'dco sub osc volume':0,
                        'chorus 1':0,'chorus 2':0,'voice hold':int(case.get('hold',False))}
                if case['scenario']!='free':values.update({'arp sync enabled':1,'trigger arp by midi channel 16':int(case['scenario']=='ext'),'clock sync':int(case['scenario']=='midi'),'host sync':int(case['scenario']=='host')})
            else:
                values={'Arp On':1,'Arp Direction':case['mode']/2,'Arp Octaves':case['octaves']/2,'Arp Tempo':case['rate'],
                        'TRANSPOSE':.5,'SUBOSC':0,'SUBOSCVOLUME':0,'AMPSUSTAIN':1,'ENVGATE':0,
                        'Arp Hold':int(case.get('hold',False))}
                if case['scenario']!='free':values.update({'Arp Sync':1,'Arp EXT 16':int(case['scenario']=='ext'),'Arp MIDI Clock':int(case['scenario']=='midi'),'Arp Host':int(case['scenario']=='host')})
            path=out/f"{case['name']}-{kind}.raw";env=os.environ.copy();env['KODA_PROBE_SCENARIO']=case['scenario'];env['KODA_PROBE_RATE']=str(sr)
            if kind=='koda':env['KODA_PROBE_BUNDLE']=str(bundle)
            command=[str(args.probe),mfr,sub]+[f'{maps[kind][name]}={value}' for name,value in values.items()]+['--render',str(path)]
            result=subprocess.run(command,env=env,capture_output=True,text=True,check=True)
            (out/f"{case['name']}-{kind}.log").write_text(result.stdout+result.stderr)
            x=np.fromfile(path,dtype='<f4');assert len(x)==6*sr and np.isfinite(x).all()
            period=1/(1.2631578*math.exp(2.9957323*case['rate'])-.26315784) if case['scenario']=='free' else .125
            first=256/sr if case['scenario'] in ('free','ext') else (256+round(5*sr/48))/sr if case['scenario']=='midi' else 256/sr
            times=np.arange(first+period*.45,3.9,period)
            pitches=[]
            for t in times:
                size=min(2048,int(period*sr*.5));start=int(t*sr)-size//2
                y=x[start:start+size];spectrum=np.abs(np.fft.rfft(y*np.hanning(size),n=65536))
                indices=np.arange(25,3000);index=indices[np.argmax(spectrum[indices])]
                pitches.append(int(round(69+12*math.log2(index*sr/65536/440))))
            # Sustained stable runs reject short mixed-pitch windows at note transitions.
            runs=[]
            for start in range(1024,int(sr*3.75),int(sr*.005)):
                y=x[start:start+2048];spectrum=np.abs(np.fft.rfft(y*np.hanning(2048),n=16384));index=30+int(np.argmax(spectrum[30:750]))
                pitch=int(round(69+12*math.log2(index*sr/16384/440)))
                if runs and runs[-1]['pitch']==pitch:runs[-1]['frames']+=1
                else:runs.append(dict(time=(start+1024)/sr,pitch=pitch,frames=1))
            stable=[r for r in runs if r['frames']>=8]
            differences=np.diff([r['time'] for r in stable]);interval=float(np.median(differences)) if len(differences) else None
            tracks[kind]={'pitches':pitches,'median_interval_seconds':interval,'tail_rms':float(np.sqrt(np.mean(x[5*sr:]**2))),
                          'ordered_pitches':[r['pitch'] for r in stable[:15]],'audio_sha256':digest(path),'parameters':values}
        match=tracks['tal']['pitches']==tracks['koda']['pitches']
        held=bool(case.get('hold',False))
        release_match=all((track['tail_rms']>1.e-4)==held for track in tracks.values())
        ordered_count=min(len(t['ordered_pitches']) for t in tracks.values())
        order_match=tracks['tal']['ordered_pitches'][:ordered_count]==tracks['koda']['ordered_pitches'][:ordered_count]
        interval_match=all(t['median_interval_seconds'] is not None for t in tracks.values()) and abs(tracks['tal']['median_interval_seconds']-tracks['koda']['median_interval_seconds'])<.01
        report['cases'].append({'case':case,'pitch_sequence_matches':match,'ordered_sequence_matches':order_match,'interval_matches_within_10ms':interval_match,'release_behavior_matches':release_match,'tracks':tracks})
    report['all_pitch_sequences_match']=all(c['pitch_sequence_matches'] and c['interval_matches_within_10ms'] and c['release_behavior_matches'] for c in report['cases'])
    (out/'results.json').write_text(json.dumps(report,indent=2))
    print(json.dumps({'all_pitch_sequences_match':report['all_pitch_sequences_match'],'cases':[(c['case']['name'],c['pitch_sequence_matches'],c['tracks']['tal']['median_interval_seconds'],c['tracks']['koda']['median_interval_seconds']) for c in report['cases']]},indent=2))
    return 0 if report['all_pitch_sequences_match'] else 1

if __name__=='__main__':raise SystemExit(main())
