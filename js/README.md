# adm.interop.js

JavaScript inside an ADM program. The engine is [QuickJS-ng](https://github.com/quickjs-ng/quickjs);
its C sources ship with the library and are compiled into your program, so there is nothing else
to install.

## Install

In your project directory:

```bash
adm get admlang:adm.interop.js
```

This downloads the library from the ADM registry, verifies its signature, and records it in
`adm.lock` and under `[deps]` in the project's `adm.toml`. 

The library contains C code (the engine). ADM normally asks before installing a package that
brings foreign code; official `admlang` packages are accepted without the question.

## Binding a script to ADM functions

Write the script as an ordinary ES module:

```js
// billing.js
export function total(items) {
    return items.reduce((sum, item) => sum + item.price * item.quantity, 0)
}
```

Declare its functions in ADM, without a body, and call them like any other:

```adm
module billing {
	use adm.interop.js::*

	struct Item {
		price    float
		quantity int
	}

	@js("billing.js")
	def total(items Item[]) !float;
}
```

```adm
let due = try billing.total(cart)
```

- The file is named relative to the source file and is embedded in the program when it is built;
  nothing is read at run time. Each declaration is the export of the same name.
- A script that throws fails the call with a `ScriptError`. Declare the function errorable (`!T`)
  to handle it; a declaration that cannot fail panics instead.
- Arguments: numbers, bools and strings convert; structs are copied into plain objects, field by
  field; `byte[]` and the numeric arrays `lend` takes arrive as typed arrays over the caller's own
  array, so what the script writes is written there; other arrays and maps are copied.
- Results: `bool`, any numeric type, `string`, `byte[]`, arrays of those, a struct (filled from
  an object: each field takes the property of the same name, nested structs, arrays and maps
  included), an array of structs, a `map<string, V>`, `Value` for the script's value as it is, or
  nothing. `?T` takes `null` and `undefined` as `none`. A number that does not fit the declared
  type, or a property of the wrong kind, fails the call. A promise is waited for, so an `async`
  export works.
- Every declaration runs in one engine, `js.runtime()`. Give scripts their host functions through
  its global object before the first call (see "Calling ADM from a script").
- A script file cannot import another file yet.

## Using the engine directly

A `Runtime` is one JavaScript engine with its own globals and modules. `eval` runs a script and
returns its last value:

```adm
use adm.interop.js

let rt = new js.Runtime()

let total = try rt.eval("""
	const prices = [4, 6, 5]
	prices.reduce((sum, price) => sum + price, 0)
""")
println(try total.asInt())          // 15
```

Braces start interpolation in ADM strings, so write `\{` and `\}` for the braces of an inline
script.

### Calling a script function

A function is a value like any other. `call` converts its arguments and returns the result:

```adm
let slugify = try rt.eval("""
	(title) => title.toLowerCase().replace(/[^a-z0-9]+/g, '-')
""")

let slug = try slugify.call("Hello, ADM World")
println(try slug.asString())        // hello-adm-world
```

### Reading objects

```adm
let user = try rt.eval("(\{ name: 'Ada', langs: ['adm', 'js'] \})")

let name = try user.get("name")
let langs = try user.get("langs")
println("{name} knows {try langs.len()} languages")   // Ada knows 2 languages
```

### Calling ADM from a script

Give the script an ADM function and it can call it like one of its own:

```adm
let log = try rt.function("log", def (args js.Value[]) !js.Value {
	println("script says: {args[0]}")
	return try rt.undefined()
})

let global = try rt.global()
try global.set("log", log)

try rt.eval("log('hello from JavaScript')")   // script says: hello from JavaScript
```

An error the ADM function fails with is thrown in the script as an `Error`. If the script does not
catch it, the caller in ADM gets the original error back.

### Modules

`define` makes a module available to `import`; `load` runs one and returns its exports. Modules
come only from what the program defines: scripts cannot read files.

```adm
rt.define("shapes.js", """
	export const area = (width, height) => width * height
""")

let app = try rt.load("app.js", """
	import * as shapes from 'shapes.js'
	export const room = shapes.area(3, 4)
""")

let room = try app.get("room")
println(try room.asInt())           // 12
```

### Errors

A script that throws fails the call with a `ScriptError`, which carries the JavaScript error's
name, message and stack:

```adm
rt.eval("JSON.parse('not json')") onerror (err error) {
	println(err.message)            // js: SyntaxError: unexpected token: 'not'
	recover try rt.undefined()
}
```

## API

| | |
|---|---|
| `Runtime` | One engine with its own heap, globals and modules. `eval`, `define`/`load`/`get` for ES modules, `compile`/`run` for bytecode, `function` for ADM functions scripts can call, `value`/`copy`/`bytes`/`lend`/`foreign`/`object`/`array`/`parse` to make values, `runJobs`, `interrupt`, `timeout`, `memory`, `collect`, `close`. |
| `Value` | A JavaScript value held by ADM. `kind`, `asBool`/`asInt`/`asFloat`/`asString`/`asBytes`, `text`, `json`, `get`/`set`/`has`/`remove`/`keys`/`len`, `call`/`invoke`/`construct`, `state`/`wait` for promises, `foreign`, `same`, `detach`. |
| `@js("file.js")` | Binds a function declared without a body to the export of the same name in an embedded script file. `runtime()` is the engine those declarations run in. |
| `ScriptError` | What a failing script fails with: `kind` (`Thrown`, `Interrupted`, `TimedOut`, `OutOfMemory`, `Closed`, `Conversion`), the JavaScript error's `name`, its stack (`trace`) and the thrown `value`. |
| `HostFunction` | `def(Value[]) !Value`, an ADM function a script calls. |

Every declaration is documented in the source; `adm doc --module adm.interop.js` lists them.

## How values cross

| ADM | JavaScript |
|---|---|
| `bool`, `float`, `string` | boolean, number, string |
| `int` | number, or BigInt beyond 2^53. `asInt` reads both back and fails on a fraction or an overflow |
| `byte[]` | `Uint8Array`. Copied by `bytes` and `value`. Passed as a call argument it is lent without a copy and is empty again when the call returns; `lend` does the same until `detach` |
| `int8[]`, `int16[]`, `uint16[]`, `int32[]`, `uint32[]`, `float32[]`, `float[]` | array (copied) by `value` and as a call argument; `lend` gives the script the matching typed array (`Int8Array` … `Float64Array`) over the array itself, with no copy, until `detach` or until the returned value is released |
| other arrays, `map<string, V>` | array, object (copied) |
| `none` | `null` |
| struct, type object | an opaque object the script can hold and pass back (`Value.foreign()` returns it); `copy` makes a plain object of its fields instead |
| an ADM function | function, through `Runtime.function` |

## Limits and safety

Scripts get no files, network, clock or environment: only the language itself and the functions
the program gives them. A runtime can also bound what a script uses:

```adm
let sandbox = new js.Runtime(memoryLimit = 16000000, timeout = 100ms)

sandbox.eval("for (;;) ;") onerror (err error) {
	println(err.message)            // js: the script ran longer than its timeout
	recover try sandbox.undefined()
}
```

- `memoryLimit` and `stackSize` are bytes; `timeout` bounds each call into the script.
  `interrupt()` stops a running script from another task.
- These limits contain a script that misbehaves, not one that is hostile: QuickJS is native code
  running in your process. Run untrusted code in a separate process.
- `run` trusts its bytecode. Pass it only what `compile` of the same library version produced.
- Any task may use a runtime; calls run one at a time. A function called by a script must not wait
  for another task that is itself waiting to use the same runtime.
- QuickJS is an interpreter: small and embeddable, but much slower than a JIT engine such as V8 on
  compute-heavy code.

## License

QuickJS-ng is MIT licensed; see `quickjs/LICENSE`. Its version is recorded in `quickjs/UPSTREAM`.
