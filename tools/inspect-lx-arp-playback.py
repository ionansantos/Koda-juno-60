#!/usr/bin/env python3
"""Run focused, static REA analysis of the confirmed LX-V2 ARM64 reference.

Run in the macOS Terminal when the agent sandbox blocks REA's local socket.
No plugin execution, permission changes, or synthesis source edits.
"""
import argparse
import hashlib
import json
import pathlib
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=pathlib.Path)
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parent.parent
    target = root / 'evidence/arpeggiator/TAL-U-NO-LX-V2.arm64'
    if not target.is_file():
        parser.error(f'Missing reference slice: {target}')
    if hashlib.sha256(target.read_bytes()).hexdigest() != 'f42ba995938a5df3a0beed6d28c214b80207c919f692306650222a32b1ef55e0':
        parser.error('Reference hash changed; review the target before using fixed addresses.')
    out = args.output or root / 'evidence/arpeggiator/decompiled'
    out.mkdir(parents=True, exist_ok=True)
    records = []
    with (out / 'rea-server.log').open('w') as log:
        proc = subprocess.Popen(['sh', str(root / 'tools/rea-local.sh'), '--mcp'],
                                cwd=root, stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE, stderr=log, text=True)
        seq = 0

        def rpc(method, params):
            nonlocal seq
            seq += 1
            proc.stdin.write(json.dumps(dict(jsonrpc='2.0', id=seq,
                                             method=method, params=params)) + '\n')
            proc.stdin.flush()
            for line in proc.stdout:
                reply = json.loads(line)
                if reply.get('id') == seq:
                    if 'error' in reply:
                        raise RuntimeError(reply['error'])
                    return reply['result']
            raise RuntimeError('REA server closed its output')

        def call(label, name, arguments):
            print(f'{label}: {name}', flush=True)
            result = rpc('tools/call', dict(name=name, arguments=arguments))
            (out / (label + '.json')).write_text(json.dumps(result, ensure_ascii=False, indent=2))
            evidence = result.get('structuredContent')
            if evidence is None:
                evidence = json.loads(next(c['text'] for c in result['content'] if c['type'] == 'text'))
            if result.get('isError') or 'error' in evidence:
                raise RuntimeError(f'{label}: {evidence.get("error", evidence)}')
            records.append(dict(file=label + '.json', operation=name,
                                evidence_id=evidence.get('evidence_id')))
            value = evidence.get('normalized_result')
            if isinstance(value, str):
                (out / (label + '.cpp.txt')).write_text(value)
            if isinstance(value, dict) and isinstance(value.get('pseudocode'), str):
                (out / (label + '.cpp.txt')).write_text(value['pseudocode'])
            return evidence

        opened = False
        try:
            rpc('initialize', dict(protocolVersion='2024-11-05', capabilities={},
                                   clientInfo=dict(name='koda-arp-analysis', version='1')))
            proc.stdin.write(json.dumps(dict(jsonrpc='2.0', method='notifications/initialized')) + '\n')
            proc.stdin.flush()
            call('01-open', 'open_binary', dict(path=str(target), provider_id='ghidra'))
            opened = True
            for label, address in [('10-process', '0x276ec'), ('11-note-on', '0x548b0'),
                                   ('12-note-off', '0x54b2c'), ('13-sort-up', '0x478ac'),
                                   ('14-sort-down', '0x48268')]:
                call(label, 'procedure_pseudo_code', dict(procedure=address))
            call('98-export', 'export_evidence_bundle', dict(path=str((out / 'evidence-bundle.json').resolve())))
            call('99-close', 'close_binary', {})
            opened = False
            (out / 'index.json').write_text(json.dumps(records, indent=2))
            print(f'Complete REA results: {out}', flush=True)
        finally:
            if opened:
                try:
                    call('99-close-after-error', 'close_binary', {})
                except Exception as error:
                    print(f'REA cleanup diagnostic: {error}', file=sys.stderr)
            proc.terminate()
            try:
                proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                print('REA has not exited; inspect its server log.', file=sys.stderr)


if __name__ == '__main__':
    main()
