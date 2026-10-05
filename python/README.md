# adm.interop.python

Python inside an ADM program. The engine is [MicroPython](https://micropython.org) 1.29; its C
sources ship with the library and are compiled into your program, so there is nothing else to
install.

MicroPython implements Python 3 syntax and the core of its library, not all of CPython: there
are no C extension modules (no numpy), and the standard library is the small set listed under
[Limits and safety](#limits-and-safety). Its documentation lists the
[differences from CPython](https://docs.micropython.org/en/latest/genrst/index.html).

## Install

In your project directory:

```bash
adm get admlang:adm.interop.python
```

The library contains C code (the engine). ADM normally asks before installing a package that
brings foreign code; official `admlang` packages are accepted without the question.

## Binding a script to ADM functions

Write the script as a Python file:

```python
# rules.py
def discount(customer):
    if customer["years"] >= 5:
        return 0.15
    return 0.05 if customer["orders"] > 10 else 0.0
```

Declare its functions in ADM, without a body, and call them like any other:

```adm
module shop {
	use adm.interop.python::*

	struct Customer {
		name   string
		years  int
		orders int
	}

	@python("rules.py")
	def discount(customer Customer) !float;
}
```

```adm
let off = try shop.discount(ada)
```

- The file is named relative to the source file and is embedded in the program when it is built;
  nothing is read at run time. It runs once, as a module, when one of its functions is first
  called. Each declaration is the module's function of the same name.
- A script that raises an exception fails the call with a `ScriptError`. Declare the function
  errorable (`!T`) to handle it; a declaration that cannot fail panics instead.
- Arguments: numbers, bools and strings convert; structs become dicts, field by field; arrays
  become lists; `byte[]` becomes `bytes`; `none` is `None`.
- Results: `bool`, any numeric type, `string`, `byte[]`, arrays of those (from a list or a
  tuple), a struct (filled from a dict by key or from an object by attribute: each field takes
  the entry of the same name, nested structs, arrays and maps included), an array of structs, a
  `map<string, V>`, `Value` for the script's value as it is, or nothing. `?T` takes `None` as
  `none`. A number that does not fit the declared type, or a field of the wrong kind, fails the
  call.
- Every declaration runs in one engine, `python.runtime()`, which is the program's only one (see
  [One runtime](#one-runtime)). Give scripts their host functions through its `global()` before
  the first call.
- A script file cannot `import` another file yet.

## Using the engine directly

A `Runtime` is the interpreter: its heap, the `__main__` module and the modules scripts import.
`exec` runs statements in `__main__`; `eval` evaluates an expression there and returns its value:

```adm
use adm.interop.python

let rt = new python.Runtime()

try rt.exec("""
prices = [4, 6, 5]
total = sum(prices)
""")
println(try (try rt.eval("total")).asInt())            // 15
```

Braces start interpolation in ADM strings, so write `\{` and `\}` for the braces of an inline
script (dicts, sets, f-strings).

### Calling a script function

```adm
let shout = try rt.eval("lambda text: text.upper() + '!'")
println(try (try shout.call("hello")).asString())      // HELLO!

let both = try (try rt.eval("divmod")).call(17, 5)     // a tuple
println(try (try both.get(0)).asInt())                 // 3
```

Keyword arguments go in a map, after the positional ones in an array:

```adm
let named map<string, any>
named["sep"] = "-"
let parts any[] = ["a", "b"]
let joined = try (try rt.eval("lambda *parts, sep=' ': sep.join(parts)")).call(parts, named)
```

### Reading and writing values

`get`, `set`, `has` and `remove` take the entry of a dict and the attribute of anything else;
with an integer they index. `attr`/`setAttr` and `item`/`setItem`/`removeItem`/`contains` name
one or the other exactly.

```adm
let user = try rt.eval("\{'name': 'Ada', 'langs': ['adm', 'python']\}")

let name = try user.get("name")
let langs = try user.get("langs")
println("{name} knows {try langs.len()} languages")    // Ada knows 2 languages
println(try (try langs.get(0)).asString())             // adm: Python counts from 0

try langs.append("lua")
try (try rt.global()).set("limit", 10)                 // a global of __main__
```

### Calling ADM from a script

```adm
let log = try rt.function(def (args python.Value[]) !python.Value {
	println("script says: {args[0]}")
	return try rt.value(none)
})
try (try rt.global()).set("log", log)

try rt.exec("log('hello from Python')")                // script says: hello from Python
```

An error the ADM function fails with is raised in the script as an `AdmError`, whose `str()` is
the error's message. If the script does not catch it, the caller in ADM gets the original error
back. ADM functions take positional arguments only.

### Modules

`define` makes a module available to `import`; `load` imports one and returns it. A dotted name is
a module of a package. Modules come only from what the program defines and from MicroPython's
own: scripts cannot read files.

```adm
rt.define("shapes", "def area(w, h):\n    return w * h\n")
let app = try rt.load("app", "from shapes import area\nroom = area(3, 4)\n")
println(try (try app.get("room")).asInt())             // 12

let json = try rt.load("json")
println(try (try json.invoke("dumps", [1, 2])).asString())   // [1, 2]
```

## One runtime

MicroPython keeps its state in the process, so a program has **one runtime open at a time**. A
second `new Runtime()` made while one is open fails every call with a `ScriptError` of kind
`Closed`; `close()` the first, or let it go, and another can be opened.

- `python.runtime()`, where `@python` declarations run, is that runtime for a program that uses
  declarations: drive Python through it instead of making your own.
- An ADM function given to scripts that refers to its runtime keeps the runtime alive, as any
  two objects that refer to each other do. Call `close()` when you are done with such a runtime.

## API

| | |
|---|---|
| `@python("file.py")` | Binds a function declared without a body to the function of the same name in an embedded Python file. `runtime()` is the engine those declarations run in. |
| `Runtime` | The interpreter. `eval`, `exec`, `define`/`load`/`get` for modules, `compile`/`run` for bytecode, `function` for ADM functions scripts can call, `value`/`copy`/`bytes`/`list`/`dict`/`tuple`/`foreign` to make values, `global`, `interrupt`, `timeout`, `memory`, `collect`, `close`. |
| `Value` | A Python value held by ADM. `kind`, `asBool`/`asInt`/`asFloat`/`asString`/`asBytes`, `text`/`repr`, `get`/`set`/`has`/`remove`, `attr`/`setAttr`, `item`/`setItem`/`removeItem`/`contains`, `append`, `keys`, `len`, `call`/`invoke`, `foreign`, `same`/`equals`. |
| `ScriptError` | What a failing script fails with: `kind` (`Thrown`, `Interrupted`, `TimedOut`, `OutOfMemory`, `Closed`, `Conversion`), the Python traceback (`trace`) and the exception (`value`). |
| `HostFunction` | `def(Value[]) !Value`, an ADM function a script calls. |

## How values cross

| ADM | Python |
|---|---|
| `bool`, `string` | `bool`, `str` |
| `int`, `float` | `int`, `float`. `asInt` also reads a float with no fraction and fails for an int beyond 64 bits; `asFloat` reads both |
| `byte[]` | `bytes`; `asBytes` reads `bytes` and `bytearray` |
| other arrays | a `list` (copied) |
| `map<string, V>` | a `dict` (copied) |
| `none` | `None` |
| struct, type object | an opaque object the script can hold and pass back (`Value.foreign()` returns it); `copy` makes a dict of its fields instead |
| an ADM function | a callable, through `Runtime.function` |

## Limits and safety

Scripts get MicroPython's builtins and the `math`, `cmath`, `json`, `re`, `struct`,
`collections`, `array`, `random`, `heapq`, `binascii`, `io` (`StringIO`, `BytesIO`), `errno`,
`gc`, `sys` and `micropython` modules. There is no `os`, `time`, `socket` or `asyncio`, and
`open` fails. `print` writes to the program's standard output. A runtime can also bound what a
script uses:

```adm
let sandbox = new python.Runtime(memoryLimit = 16000000, timeout = 100ms)

sandbox.exec("while True:\n    pass\n") onerror (err error) {
	println(err.message)            // python: the script ran longer than its timeout
	recover none
}
```

- `memoryLimit` is bytes of heap; `stackSize` bounds how deep a script recurses (1 MiB unless
  set); `timeout` bounds each call into the script. `interrupt()` stops a running script from
  another task. A script that catches the stop is stopped again at its next loop. A script is
  stopped where it loops, not inside one long builtin call such as `sorted`.
- These limits contain a script that misbehaves, not one that is hostile: MicroPython is native
  code running in your process. Run untrusted code in a separate process.
- `run` trusts its bytecode. Pass it only what `compile` of the same library version produced.
- Any task may use a runtime; calls run one at a time. A function called by a script must not wait
  for another task that is itself waiting to use the same runtime.

## License

MicroPython is MIT licensed; see `micropython/LICENSE`. Its version is recorded in
`micropython/UPSTREAM`.
