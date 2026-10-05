# adm.interop.lua

Lua inside an ADM program. The engine is [Lua 5.4](https://www.lua.org); its C sources ship with
the library and are compiled into your program, so there is nothing else to install.

## Install

In your project directory:

```bash
adm get admlang:adm.interop.lua
```

The library contains C code (the engine). ADM normally asks before installing a package that
brings foreign code; official `admlang` packages are accepted without the question.

## Binding a script to ADM functions

Write the script as a Lua module, a file that returns a table of functions:

```lua
-- rules.lua
local M = {}

function M.discount(customer)
    if customer.years >= 5 then return 0.15 end
    return customer.orders > 10 and 0.05 or 0
end

return M
```

Declare its functions in ADM, without a body, and call them like any other:

```adm
module shop {
	use adm.interop.lua::*

	struct Customer {
		name   string
		years  int
		orders int
	}

	@lua("rules.lua")
	def discount(customer Customer) !float;
}
```

```adm
let off = try shop.discount(ada)
```

- The file is named relative to the source file and is embedded in the program when it is built;
  nothing is read at run time. Each declaration is the function of the same name in the table the
  file returns.
- A script that raises an error fails the call with a `ScriptError`. Declare the function
  errorable (`!T`) to handle it; a declaration that cannot fail panics instead.
- Arguments: numbers, bools and strings convert; structs become tables, field by field; arrays
  become tables indexed from 1; `byte[]` becomes a string of those bytes; `none` is `nil`.
- Results: `bool`, any numeric type, `string`, `byte[]`, arrays of those, a struct (filled from
  a table: each field takes the table's field of the same name, nested structs, arrays and maps
  included), an array of structs, a `map<string, V>`, `Value` for the script's value as it is, or
  nothing. `?T` takes `nil` as `none`. A number that does not fit the declared type, or a field
  of the wrong kind, fails the call. Only the first value a function returns is used.
- Every declaration runs in one engine, `lua.runtime()`. Give scripts their host functions
  through its table of globals before the first call.
- A script file cannot `require` another file yet.

## Using the engine directly

A `Runtime` is one Lua state with its own globals and modules. `eval` runs a chunk and returns
its first result:

```adm
use adm.interop.lua

let rt = new lua.Runtime()

let total = try rt.eval("""
	local sum = 0
	for _, price in ipairs(\{ 4, 6, 5 \}) do sum = sum + price end
	return sum
""")
println(try total.asInt())          // 15
```

Braces start interpolation in ADM strings, so write `\{` and `\}` for the braces of an inline
script.

### Calling a script function

```adm
let shout = try rt.eval("return function(text) return text:upper() .. '!' end")
println(try (try shout.call("hello")).asString())      // HELLO!

let divmod = try rt.eval("return function(a, b) return a // b, a % b end")
let both = try divmod.callAll(17, 5)                   // every result: 3 and 2
```

### Reading tables

```adm
let user = try rt.eval("return \{ name = 'Ada', langs = \{ 'adm', 'lua' \} \}")

let name = try user.get("name")
let langs = try user.get("langs")
println("{name} knows {try langs.len()} languages")    // Ada knows 2 languages
println(try (try langs.get(1)).asString())             // adm: Lua counts from 1
```

### Calling ADM from a script

```adm
let log = try rt.function(def (args lua.Value[]) !lua.Value {
	println("script says: {args[0]}")
	return try rt.nil()
})
try (try rt.global()).set("log", log)

try rt.eval("log('hello from Lua')")                   // script says: hello from Lua
```

An error the ADM function fails with is raised in the script. If the script does not catch it
with `pcall`, the caller in ADM gets the original error back.

### Modules

`define` makes a module available to `require`; `load` runs one and returns what it returns.
Modules come only from what the program defines: scripts cannot read files.

```adm
rt.define("shapes", "return \{ area = function(w, h) return w * h end \}")
let app = try rt.load("app", "local shapes = require('shapes') return \{ room = shapes.area(3, 4) \}")
println(try (try app.get("room")).asInt())             // 12
```

## API

| | |
|---|---|
| `@lua("file.lua")` | Binds a function declared without a body to the function of the same name in an embedded Lua module. `runtime()` is the engine those declarations run in. |
| `Runtime` | One Lua state with its own heap, globals and modules. `eval`, `define`/`load`/`get` for modules, `compile`/`run` for bytecode, `function` for ADM functions scripts can call, `value`/`copy`/`bytes`/`table`/`foreign`/`nil` to make values, `interrupt`, `timeout`, `memory`, `collect`, `close`. |
| `Value` | A Lua value held by ADM. `kind`, `asBool`/`asInt`/`asFloat`/`asString`/`asBytes`, `text`, `get`/`set`/`has`/`remove`/`keys`/`len`, `call`/`callAll`/`invoke`, `foreign`, `same`. |
| `ScriptError` | What a failing script fails with: `kind` (`Thrown`, `Interrupted`, `TimedOut`, `OutOfMemory`, `Closed`, `Conversion`), the Lua traceback (`trace`) and the error `value`. |
| `HostFunction` | `def(Value[]) !Value`, an ADM function a script calls. |

## How values cross

| ADM | Lua |
|---|---|
| `bool`, `string` | boolean, string |
| `int`, `float` | integer, float. `asInt` also reads a float with no fraction; `asFloat` reads both |
| `byte[]` | a string of those bytes (Lua strings are byte strings); `asBytes` reads them back exactly |
| other arrays | a table indexed from 1 (copied) |
| `map<string, V>` | a table (copied) |
| `none` | `nil` |
| struct, type object | an opaque object the script can hold and pass back (`Value.foreign()` returns it); `copy` makes a table of its fields instead |
| an ADM function | function, through `Runtime.function` |

## Limits and safety

Scripts get the base, coroutine, table, string, math and utf8 libraries. There is no `io`, `os`,
`package` or `debug`, no `dofile` or `loadfile`, and `load` takes source text only. A runtime can
also bound what a script uses:

```adm
let sandbox = new lua.Runtime(memoryLimit = 16000000, timeout = 100ms)

sandbox.eval("while true do end") onerror (err error) {
	println(err.message)            // lua: the script ran longer than its timeout
	recover try sandbox.nil()
}
```

- `memoryLimit` is bytes; `timeout` bounds each call into the script. `interrupt()` stops a
  running script from another task. A script cannot outlast either with `pcall`. A script is
  stopped between its own instructions, not inside one long library call such as `string.rep`.
- These limits contain a script that misbehaves, not one that is hostile: Lua is native code
  running in your process. Run untrusted code in a separate process.
- `run` trusts its bytecode. Pass it only what `compile` of the same library version produced.
- Any task may use a runtime; calls run one at a time. A function called by a script must not wait
  for another task that is itself waiting to use the same runtime.

## License

Lua is MIT licensed; see `lua/LICENSE`. Its version is recorded in `lua/UPSTREAM`.
