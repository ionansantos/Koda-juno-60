import json,math,hashlib
from pathlib import Path
p=Path(__file__).parent;r=json.loads((p/'results.json').read_text());d={(v['engine'],v['case']):v for v in r['cases']};checks=[];summary={}
for engine in ['tal','koda']:
 neutral=d[engine,'neutral'];s={};base=neutral['pitch_hz'];s['detune_cents']={name:1200*math.log2(d[engine,name]['pitch_hz']/base) for name in ['detune-low','detune-high']}
 for name,expect in [('detune-low',-50),('detune-high',50)]:checks.append({'engine':engine,'check':name,'passed':abs(s['detune_cents'][name]-expect)<1,'measured_cents':s['detune_cents'][name],'expected':expect})
 s['pan_gains_own_neutral']={}
 for name,x in [('pan-left',0),('pan-right',1),('pan-quarter',.25),('pan-threequarter',.75)]:
  measured=[v/neutral['rms_lr'][0] for v in d[engine,name]['rms_lr']];expected=[4/3*(1-x*x),4/3*(1-(1-x)**2)];s['pan_gains_own_neutral'][name]=measured;checks.append({'engine':engine,'check':name,'passed':max(abs(a-b) for a,b in zip(measured,expected))<1e-4,'measured':measured,'expected':expected})
 s['envelope_duration_ratios']={}
 for key,metric in [('attack','attack70_seconds'),('decay','decay_minus20db_seconds'),('release','release_minus20db_seconds')]:
  lo=d[engine,key+'-low'][metric];hi=d[engine,key+'-high'][metric];ratio=hi/lo;s['envelope_duration_ratios'][key]={'low_seconds':lo,'high_seconds':hi,'ratio':ratio};checks.append({'engine':engine,'check':key+'-duration-ratio','passed':abs(ratio-3)<.2,'measured':ratio,'expected':3})
 energyRatio=d[engine,'cutoff-high']['rms_lr'][0]/max(d[engine,'cutoff-low']['rms_lr'][0],1e-20);s['cutoff_high_low_rms_ratio']=energyRatio;checks.append({'engine':engine,'check':'cutoff-direction','passed':energyRatio>1000,'measured':energyRatio})
 checks.append({'engine':engine,'check':'all-audio-finite','passed':all(v['finite'] for v in r['cases'] if v['engine']==engine)});summary[engine]=s
shared=json.loads((p/'shared-pan-results.json').read_text());checks_shared=shared['checks'];binary=Path(r['identities']['koda']['path']);currenthash=hashlib.sha256(binary.read_bytes()).hexdigest();assert currenthash==r['identities']['koda']['sha256']==shared['binary_sha256']
for c in r['cases']+shared['cases']:assert hashlib.sha256(Path(c['raw_path']).read_bytes()).hexdigest()==c['raw_sha256']
record={'scope':'30normalrendercases and5sharedfilterregressionrenders;27checks for controlresponse. Checks are relative within eachengine. Does not establish timbre or absoluteADSRmatching.','summary':summary,'checks':checks,'source_results_sha256':hashlib.sha256((p/'results.json').read_bytes()).hexdigest(),'identities':r['identities'],'normal_matrix_render_count':len(r['cases']),'shared_filter_checks':checks_shared,'shared_filter_render_count':len(shared['cases']),'total_checks':len(checks)+len(checks_shared),'passed_checks':sum(c['passed'] for c in checks+checks_shared),'raw_digests_verified':len(r['cases'])+len(shared['cases']),'tested_binary_still_matches':True};assert record['passed_checks']==record['total_checks'];(p/'summary.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps({'checks':record['total_checks'],'passed':record['passed_checks'],'rawdigests':record['raw_digests_verified'],'AUsha256':currenthash}))
