# adm.interop.dotnet

.NET inside an ADM program: types, objects and static methods of .NET assemblies (C#, F#, VB),
called from ADM, and ADM functions .NET calls back. The library loads the installed .NET runtime
into the program when it is first used; nothing of .NET is needed to build.

## Install

In your project directory:

```bash
adm get admlang:adm.interop.dotnet
```

The library contains C code (the part that loads the .NET runtime) and writes its bridge assembly
into the program's own folder when the runtime starts, so it asks for the permission
`adm.storage.write`. ADM normally asks before installing a package that brings foreign code;
official `admlang` packages are accepted without the question. Running a program needs a .NET
runtime, version 8 or later: `DOTNET_ROOT`, or `dotnet` on `PATH`.

## Binding .NET methods to ADM functions

```adm
use adm.interop.dotnet::*

@dotnet("Acme.Billing")
def total(prices float[], quantity int32) !float;

@dotnet("Acme.Billing")
def invoiceNumber(customer string) !?string;
```

```adm
try runtime().start(folders = ["billing/bin"])
println(try total([19.90, 5.00], 3))
```

A function declared without a body under `@dotnet("Type")` is a static method of that type: the
one with the declaration's name, or that name with a capital first letter (`total` is `Total`),
whose parameters are exactly the declaration's:

| ADM | .NET |
|-----|------|
| `bool` | `bool` |
| `int8`, `int16`, `int32`, `int` | `sbyte`, `short`, `int`, `long` |
| `byte`, `uint16`, `uint32`, `uint` | `byte`, `ushort`, `uint`, `ulong` |
| `float32`, `float` | `float`, `double` |
| `string` | `string` |
| `byte[]`, `int32[]`, `float[]`, `string[]`, … | `byte[]`, `int[]`, `double[]`, `string[]`, … |
| no result | `void` |

- The type is looked for in the assemblies (`.dll` files) of the folders given to `start`;
  `"Acme.Billing, Billing"` names its assembly. The methods need no attribute and no change: an
  existing assembly is called as it is.
- `method = "Name"` names the method when the declaration cannot have its name.
- Strings and arrays are copied each way.
- `?string` and `?T[]` as a result take `null` as none; without the `?`, a null fails the call.
- A type or method that does not exist fails the first call, naming the C# signature the
  declaration asks for: ``type Acme.Sample has no method `static long Add(long, long)` ``.

## Using the runtime directly

`runtime()` gives the program's .NET runtime. Types are loaded by name; objects, arrays and types
are held as `Value`s.

```adm
let dotnet = runtime()
let list = try (try dotnet.load("System.Collections.Generic.List`1[System.String]")).construct()
try list.invoke("Add", "ADM")
try list.invoke("Add", "interop")
println(try (try list.get("Count")).asInt())      // 2
println(try (try list.get(0)).asString())         // ADM
```

### Types, members and methods

```adm
let math = try dotnet.load("System.Math")
let largest = try (try math.invoke("Max", 3, 9)).asInt()        // a static method
let limit = try (try dotnet.get("System.Int32", "MaxValue")).asInt()

let builder = try (try dotnet.load("System.Text.StringBuilder")).construct("a")
try builder.invoke("Append", 42)
try builder.set("Length", 2)                                     // a property
println(try builder.text())                                      // a4
```

A type is named as .NET names it: a generic type with its arity and arguments
(``System.Collections.Generic.Dictionary`2[System.String,System.Int32]``), a type outside the
runtime's own library optionally with its assembly (`"Acme.Billing, Billing"`); `int[]` and the
other C# keywords work too. `get` and `set` reach fields and properties.

`invoke` and `construct` pick, among the methods of that name, the one whose parameters take the
arguments without losing anything: an integer goes where a `long`, an `int` it fits, a `double` or
an enum is taken, a string where a `string` or a `char` is; `params` and parameters with defaults
work as in C#. When no method fits, the call fails and says what it was given.

### Calling ADM from .NET

```adm
let byLength = try dotnet.function(def (args Value[]) !Value {
	return try dotnet.value((try args[0].asString()).len() - (try args[1].asString()).len())
})
try list.invoke("Sort", byLength)
```

An ADM function becomes a delegate of the type it is passed as (a `Comparison<string>` here), or
an object of the interface a parameter takes; `dotnet.function(fn, "System.Action")` makes it one
from the start. The function runs on the ADM task that called into .NET, may call back into .NET,
and an error it fails with comes back unchanged from the .NET call that let it through.

## API

| | |
|---|---|
| `@dotnet("Type")` | Binds a function declared without a body to the static method of the same name. |
| `runtime()` | The program's .NET runtime. |
| `Runtime` | `start` (assembly folders), `load` (a type as a value), `get` (a static field or property), `function` for ADM functions .NET calls, `value`/`bytes`/`foreign`/`null` to make values, `memory`, `live`, `collect`. |
| `Value` | A .NET value held by ADM. `kind`, `isNone`, `asBool`/`asInt`/`asFloat`/`asString`/`asBytes`, `text`, `get`/`set` by name (fields, properties) and by index (arrays, indexers), `len`, `invoke`, `construct`, `foreign`, `same`, `equals`. |
| `ScriptError` | What a failing call fails with: `kind` (`Thrown`, `Binding`, `Unavailable`, `Conversion`), the exception as .NET prints it (`trace`) and the exception as a `value`. |
| `HostFunction` | `def(Value[]) !Value`, an ADM function .NET calls. |

The other `adm.interop` libraries have the same names. What a script engine has and the .NET
runtime does not is left out: there is no `eval`, `compile` or `define` (.NET code comes compiled,
in assemblies), and no `close`, `interrupt` or `timeout` (the runtime cannot be stopped or
restarted inside a process).

## How values cross

| ADM | .NET |
|---|---|
| `bool`, `string` | `bool`, `string`; a `char` reads as a one-character string |
| `int`, `float` | a `long` or a `double`, or the narrower type a parameter takes when the value fits. Results of every integer and float type read as `int` and `float`; a `ulong` keeps its bits |
| arrays of numbers, bools and strings | the .NET array of the same element type (copied); `asBytes` reads a `byte[]` back |
| other arrays | `object[]` of the converted elements |
| `map<string, V>` | a `Dictionary<string, object>` (copied) |
| `none` | `null` |
| struct, type object | an opaque object .NET can hold and pass back (`Value.foreign()` returns it) |
| an ADM function | a delegate or an object of an interface, through `Runtime.function` |

Where .NET takes any `object`, an integer arrives as an `int` when it fits and a `long` otherwise.

## The .NET runtime

`runtime().start(folders)` starts the runtime and names the folders assemblies are loaded from.
Without it, the first use starts the runtime with the current folder. A process has one .NET
runtime, for as long as it runs: a second `start` fails.

The runtime is the newest one of the .NET installation `DOTNET_ROOT` names, else of the `dotnet`
on `PATH`. It runs on a thread of its own inside the program, and every call waits for that
thread, so calls from several tasks run one after the other.

## Limits

- A .NET exception fails the call with a `ScriptError` of kind `Thrown`; a declaration that cannot
  fail (no `!`) panics with the message instead.
- An ADM function called from a thread .NET started (the thread pool, a timer) runs on the next
  ADM task that calls into .NET, and the .NET thread waits until one does.
- Not there yet: generic methods, `ref` and `out` parameters, events, `async` methods as ADM
  tasks, checking declarations against the assemblies when the program is built.
- Linux is the platform it runs on today; macOS is written and not run, Windows not written.
