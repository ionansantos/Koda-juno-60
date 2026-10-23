"""Reproduce derived names from the retained REA read_bytes record."""
import json, struct
from pathlib import Path
folder = Path(__file__).parent
record = json.loads((folder / "parameter_name_raw.json").read_text())
raw = bytes.fromhex(record["normalized_result"]["bytes_hex"])
base = 0x4e14
names = []
for index in range(47):
    if index >= 36:
        names.append("Param")
        continue
    offset = struct.unpack_from("<i", raw, 0x28 + 4 * index)[0]
    if offset == 0xc5:
        names.append("Param")
        continue
    k, output = offset, bytearray(64)
    while raw[k] != 0xc3:
        width, displacement = 4, 0
        if raw[k] == 0x66:
            width, k = 2, k + 1
        if raw[k] == 0xc6:
            width = 1
        if raw[k] in (0xc6, 0xc7) and raw[k+1] == 2:
            k += 2
        elif raw[k] in (0xc6, 0xc7) and raw[k+1] == 0x42:
            displacement, k = raw[k+2], k + 3
        else:
            k += 1
            continue
        output[displacement:displacement+width] = raw[k:k+width]
        k += width
    names.append(output.split(b"\0", 1)[0].decode("ascii"))
metadata = json.loads((folder / "parameters.json").read_text())
assert names == [parameter["name"] for parameter in metadata]
print("47 parameter names match retained REA bytes.")
