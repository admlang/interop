#!/usr/bin/env python3
# pack.py WAST2JSON SPEC_TEST_CORE OUT: packs the WebAssembly spec tests (test/core and
# test/core/simd of the spec repository at its 2.0 tag) into one file for the test suites.
# wast2json (wabt) turns each .wast script into commands and the binary modules they name.
# Modules given as text (assert_malformed of the text format) are left out. Where a script's
# text names a data segment in a module that has none, wast2json writes no data count section,
# which the binary format requires: two assert_invalid modules are malformed for that reason.
#
# Integers are little-endian. A string is a u32 length and its bytes.
# The file is then compressed: gzip -9 -n OUT.
#   file:    u32 scripts, then each script: string name, u32 commands, the commands
#   command: u8 kind, u32 line, then
#     0 module                 string name, string binary
#     1 register               string name, string as
#     2 action                 action
#     3 assert_return          action, u32 count, the expected values
#     4 assert_trap            action, string text
#     5 assert_exhaustion      action, string text
#     6 assert_malformed       string binary
#     7 assert_invalid         string binary, string text
#     8 assert_unlinkable      string binary, string text
#     9 assert_uninstantiable  string binary, string text
#   action:  u8 kind (0 invoke, 1 get), string module, string field, u32 count, the arguments
#   value:   u8 type (0 i32, 1 i64, 2 f32, 3 f64, 4 v128, 5 funcref, 6 externref),
#            u8 lane type (same codes; 7 i8, 8 i16), u8 lanes, then each lane:
#            u8 form (0 these bits, 1 a canonical NaN, 2 an arithmetic NaN, 3 null,
#            4 any reference) and, for form 0, the bits in as many bytes as the lane
#            type has (8 for a reference)
import glob, json, os, struct, subprocess, sys, tempfile

wast2json, core, out = sys.argv[1], sys.argv[2], sys.argv[3]
TYPES = {"i32": 0, "i64": 1, "f32": 2, "f64": 3, "v128": 4, "funcref": 5, "externref": 6, "i8": 7, "i16": 8}
KINDS = {"module": 0, "register": 1, "action": 2, "assert_return": 3, "assert_trap": 4, "assert_exhaustion": 5,
         "assert_malformed": 6, "assert_invalid": 7, "assert_unlinkable": 8, "assert_uninstantiable": 9}

def string(s):
    b = s if isinstance(s, bytes) else s.encode("utf-8", "surrogatepass")
    return struct.pack("<I", len(b)) + b

WIDTHS = {0: 4, 1: 8, 2: 4, 3: 8, 5: 8, 6: 8, 7: 1, 8: 2}

def lane(text, t):
    if text == "nan:canonical":
        return b"\x01"
    if text == "nan:arithmetic":
        return b"\x02"
    if text == "null":
        return b"\x03"
    return b"\x00" + (int(text) & 0xffffffffffffffff).to_bytes(8, "little")[:WIDTHS[t]]

def value(v):
    t = TYPES[v["type"]]
    if t == 4:
        lanes = v["value"]
        kind = TYPES[v["lane_type"]]
        return struct.pack("<BBB", t, kind, len(lanes)) + b"".join(lane(x, kind) for x in lanes)
    if "value" not in v:
        return struct.pack("<BBB", t, t, 1) + b"\x04"
    return struct.pack("<BBB", t, t, 1) + lane(v["value"], t)

def action(a):
    args = a.get("args", [])
    return (struct.pack("<B", 0 if a["type"] == "invoke" else 1) + string(a.get("module", "")) + string(a["field"])
            + struct.pack("<I", len(args)) + b"".join(value(x) for x in args))

scripts = []
total = 0
with tempfile.TemporaryDirectory() as tmp:
    for wast in sorted(glob.glob(os.path.join(core, "*.wast")) + glob.glob(os.path.join(core, "simd", "*.wast"))):
        stem = os.path.basename(wast)[:-5]
        target = os.path.join(tmp, stem + ".json")
        subprocess.run([wast2json, wast, "-o", target], check=True)
        commands = []
        for c in json.load(open(target))["commands"]:
            kind = KINDS[c["type"]]
            head = struct.pack("<BI", kind, c["line"])
            if kind in (0, 6, 7, 8, 9):
                if c.get("module_type", "binary") != "binary":
                    continue
                binary = string(open(os.path.join(tmp, c["filename"]), "rb").read())
                if kind == 0:
                    commands.append(head + string(c.get("name", "")) + binary)
                elif kind == 6:
                    commands.append(head + binary)
                else:
                    commands.append(head + binary + string(c["text"]))
            elif kind == 1:
                commands.append(head + string(c.get("name", "")) + string(c["as"]))
            elif kind == 2:
                commands.append(head + action(c["action"]))
            elif kind == 3:
                expected = c["expected"]
                commands.append(head + action(c["action"]) + struct.pack("<I", len(expected)) + b"".join(value(x) for x in expected))
            else:
                commands.append(head + action(c["action"]) + string(c["text"]))
        total += len(commands)
        scripts.append(string(stem) + struct.pack("<I", len(commands)) + b"".join(commands))
with open(out, "wb") as w:
    w.write(struct.pack("<I", len(scripts)))
    for s in scripts:
        w.write(s)
print(len(scripts), "scripts,", total, "commands")
