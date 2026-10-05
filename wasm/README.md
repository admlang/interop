# adm.interop.wasm

WebAssembly inside an ADM program, written in ADM: no C, nothing else to install.

**In progress.** Modules of WebAssembly 2.0, the 128-bit vector instructions included, decode,
validate and run. The API below is provisional.

```adm
use adm.interop.wasm

let loaded = try wasm.decode(bytes)
let store = new wasm.Store()
let instance = try store.instantiate(loaded)
let results = try instance.invoke("add", wasm.Value[
	wasm.Value{kind: wasm.ValueType.I32, bits: 2},
	wasm.Value{kind: wasm.ValueType.I32, bits: 3}
])
println(results[0].bits)
```

- `decode` reads a binary into a `Module` and fails with a `ModuleError` naming the byte where
  it stops following the format. `Module.validate()` checks it without running anything;
  `imports` and `exports` list what it asks for and offers.
- A `Store` owns what instances share: functions, tables, memories and globals, which modules
  pass to each other through imports and exports. `provide` makes a host function, global, table
  or memory importable, `register` does it for every export of an instance.
- `instantiate` links, compiles and starts a module. Each function is compiled once to register
  instructions; calls keep their frames on the heap, so deep recursion in a module ends in a
  `Trap`, never in a crash of the program.
- `store.fuel(units)` limits what the code may run: each function entry and loop iteration
  takes one unit, and code that runs out stops with a `Trap`; `store.fuel()` is what is left.
- A trap (division by zero, an access out of bounds, `unreachable`, a wrong indirect call, an
  exhausted call stack) is an ADM error of type `Trap`.

## Conformance

The library runs the WebAssembly 2.0 specification tests: all 52915 commands of the 148
scripts pass (`adm test .`), 90 for the core instructions and 58 for the vector ones.

## Not there yet

Typed calls and memory views, WASI, the text format.
