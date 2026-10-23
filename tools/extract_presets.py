#!/usr/bin/env python3
"""Extract the verified i386 factory bank; never execute the original plugin."""
import argparse
import csv
import hashlib
import json
import math
import pathlib
import struct

EXPECTED_SHA = 'b46668c96111476a3bde38e3367e8ca323178070ced25a51f47f9dd51dc65359'
OFFSET, COUNT, STRIDE = 0x33d20, 120, 0x4d0

def extract(path):
    data = pathlib.Path(path).read_bytes()
    if hashlib.sha256(data).hexdigest() != EXPECTED_SHA:
        raise ValueError('Binary identity differs from the REA evidence')
    if len(data) < OFFSET + COUNT * STRIDE:
        raise ValueError('Truncated bank')
    presets = []
    for i in range(COUNT):
        start = OFFSET + i * STRIDE
        record = data[start:start + STRIDE]
        raw_name = record[:24].split(b'\0', 1)[0]
        name = raw_name.decode('ascii')
        params = list(struct.unpack_from('<47f', record, 24))
        if not name or not all(math.isfinite(p) for p in params):
            raise ValueError(f'Invalid preset record {i}')
        mapping = list(struct.unpack_from('<255i', record, 0xd4))
        presets.append(dict(index=i, name=name, parameters=params,
                            midi_map=mapping, file_offset=start,
                            virtual_address=hex(start)))
    return presets

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('binary')
    parser.add_argument('--output', default=str(pathlib.Path(__file__).resolve().parents[1]))
    args = parser.parse_args()
    out = pathlib.Path(args.output)
    presets = extract(args.binary)
    (out / 'evidence' / 'factory-presets.json').write_text(json.dumps(presets, indent=2) + '\n')
    with (out / 'evidence' / 'preset-names.csv').open('w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(['index', 'name', 'file_offset', 'virtual_address'])
        writer.writerows((p['index'], p['name'], p['file_offset'], p['virtual_address']) for p in presets)
    header = ['#pragma once', '#include <array>', '#include <cstddef>',
              'namespace talclone {',
              'struct FactoryPreset { const char* name; std::array<float, 47> parameters; std::array<int, 255> midiMap; };',
              'inline constexpr std::array<FactoryPreset, 120> factoryPresets {{']
    for p in presets:
        values = ', '.join(format(x, '.9e') + 'f' for x in p['parameters'])
        mapping = ', '.join(str(x) for x in p['midi_map'])
        header.append('    {' + json.dumps(p['name']) + ', {{' + values + '}}, {{' + mapping + '}}},')
    header += ['}};', '} // namespace talclone', '']
    (out / 'Source' / 'FactoryPresets.h').write_text('\n'.join(header))
    print(f'Extracted {len(presets)} validated programs, 47 parameters each')
    print('First:', presets[0]['name'], 'Last:', presets[-1]['name'])

if __name__ == '__main__':
    main()
