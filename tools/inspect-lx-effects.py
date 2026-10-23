#!/usr/bin/env python3
"""Focused read-only REA investigation of the identity-pinned TAL LX effects.
JSON command files in the caller-selected queue permit adaptive follow-ups in one
Ghidra session. Create FINISH in that queue to export evidence and close.
"""
import argparse,hashlib,json,pathlib,subprocess,time

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args();root=pathlib.Path(__file__).resolve().parent.parent;out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    queue=out/'queue';queue.mkdir(exist_ok=True)
    target=root/'evidence/arpeggiator/TAL-U-NO-LX-V2.arm64'
    if hashlib.sha256(target.read_bytes()).hexdigest()!='f42ba995938a5df3a0beed6d28c214b80207c919f692306650222a32b1ef55e0':raise RuntimeError('Pinned reference identity changed')
    with (out/'rea-server.log').open('w') as log:
        proc=subprocess.Popen(['sh',str(root/'tools/rea-local.sh'),'--mcp'],cwd=root,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=log,text=True)
        seq=0;records=[];opened=False
        def rpc(method,params):
            nonlocal seq
            seq+=1;proc.stdin.write(json.dumps(dict(jsonrpc='2.0',id=seq,method=method,params=params))+'\n');proc.stdin.flush()
            for line in proc.stdout:
                reply=json.loads(line)
                if reply.get('id')==seq:
                    if 'error' in reply:raise RuntimeError(reply['error'])
                    return reply['result']
            raise RuntimeError('REA output closed')
        def call(label,name,arguments):
            print(label+': '+name,flush=True);result=rpc('tools/call',dict(name=name,arguments=arguments))
            (out/(label+'.json')).write_text(json.dumps(result,ensure_ascii=False,indent=2))
            e=result.get('structuredContent')
            if e is None:e=json.loads(next(c['text'] for c in result['content'] if c['type']=='text'))
            records.append(dict(file=label+'.json',operation=name,evidence_id=e.get('evidence_id'),error=bool(result.get('isError') or 'error' in e)))
            (out/'index.json').write_text(json.dumps(records,indent=2))
            value=e.get('normalized_result')
            if isinstance(value,str):(out/(label+'.cpp.txt')).write_text(value)
            return e
        try:
            rpc('initialize',dict(protocolVersion='2024-11-05',capabilities={},clientInfo=dict(name='koda-lx-effects-analysis',version='1')))
            proc.stdin.write(json.dumps(dict(jsonrpc='2.0',method='notifications/initialized'))+'\n');proc.stdin.flush()
            e=call('001-open','open_binary',dict(path=str(target),provider_id='ghidra'))
            if 'error' in e:raise RuntimeError(e['error'])
            opened=True
            while True:
                jobs=sorted(queue.glob('*.json'))
                if jobs:
                    job=jobs[0];command=json.loads(job.read_text());call(job.stem,command['name'],command['arguments']);job.rename(job.with_suffix('.done'))
                elif (queue/'FINISH').exists():break
                else:time.sleep(.25)
        finally:
            if opened:
                try:call('998-export','export_evidence_bundle',dict(path=str(out/'evidence-bundle.json')))
                finally:call('999-close','close_binary',{})
            proc.stdin.close()
            try:proc.wait(timeout=10)
            except subprocess.TimeoutExpired:proc.terminate();proc.wait(timeout=10)

if __name__=='__main__':main()
