#!/usr/bin/env python3
"""Compare measured pitch trajectories in two real-AU Portamento render reports.
Only intervals after the other polyphonic notes have released are evaluated.
This verifies glide behavior, not waveform or timbre equivalence.
"""
import argparse,hashlib,json,pathlib
import numpy as np

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--reference',type=pathlib.Path,required=True);ap.add_argument('--candidate',type=pathlib.Path,required=True);ap.add_argument('--output',type=pathlib.Path,required=True);args=ap.parse_args()
 ref=json.loads(args.reference.read_text());cand=json.loads(args.candidate.read_text());assert ref['sample_rate']==cand['sample_rate'];a={c['name']:c for c in ref['cases']};b={c['name']:c for c in cand['cases']};assert set(a)==set(b)
 report={'scope':__doc__,'sample_rate':ref['sample_rate'],'limits':{'pitch_95th_percentile_cents':10,'pitch_max_cents':20,'arrival_delta_seconds':.01},'rationale':'10 cents at the 95th percentile, maximum20 cents, arrival difference10ms; pitch estimator window1536 samples, stride220; excludes simultaneous chords. These are glide tolerances, not perceptual/timbre equivalence.','inputs':[{'path':str(p.resolve()),'sha256':sha(p)} for p in [args.reference,args.candidate]],'binaries':{'tal':ref['binary'],'koda':cand['binary']},'cases':[]}
 for name,x in a.items():
  y=b[name];assert x['events_seconds_status_note_velocity']==y['events_seconds_status_note_velocity']
  tracks=[]
  for item in [x,y]:
   raw=pathlib.Path(item['audio_path']);assert sha(raw)==item['audio_sha256'];audio=np.fromfile(raw,dtype='<f4').reshape(-1,2);assert np.isfinite(audio).all() and np.max(np.abs(audio))>1e-5;tracks.append(audio)
  assert tracks[0].shape==tracks[1].shape
  windows=[(1.15,3.4 if name.startswith('mono-return') else 6.65)]
  if name.startswith('poly-six'):windows=[(2.15,3.9)]
  if name.startswith('poly-reuse'):windows=[(i+.15,i+.7) for i in range(8)]
  ta=np.array(x['trajectory']);tb=np.array(y['trajectory']);deltas=[]
  for lo,hi in windows:
   at=ta[(ta[:,0]>=lo)&(ta[:,0]<=hi)];bt=tb[(tb[:,0]>=lo)&(tb[:,0]<=hi)];assert len(at)>5 and len(bt)>5
   deltas.extend(np.abs(at[:,1]-np.interp(at[:,0],bt[:,0],bt[:,1]))*100)
  p95=float(np.quantile(deltas,.95));maximum=float(max(deltas));rt=x['first_within_10cents_seconds_after_second_note'];ct=y['first_within_10cents_seconds_after_second_note'];arrival=abs(rt-ct) if rt is not None and ct is not None else None
  arrival_ok=(rt is None and ct is None) or (arrival is not None and arrival<=.01)
  report['cases'].append({'name':name,'windows_seconds':windows,'points':len(deltas),'pitch_95th_percentile_cents':p95,'pitch_max_cents':maximum,'arrival_delta_seconds':arrival,'tal_arrival':rt,'koda_arrival':ct,'pass':p95<=10 and maximum<=20 and arrival_ok})
 report['passed']=sum(c['pass'] for c in report['cases']);report['total']=len(report['cases']);args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2,allow_nan=False));print(json.dumps({k:report[k] for k in ['sample_rate','passed','total']}));
 if report['passed']!=report['total']:raise SystemExit(1)
if __name__=='__main__':main()
