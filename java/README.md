# adm.interop.java

Java inside an ADM program: classes, objects and static methods of the installed Java VM, called
from ADM, and ADM functions Java calls back. The library loads the Java VM of the installed JDK or
JRE into the program when it is first used; nothing of Java is needed to build.

## Install

In your project directory:

```bash
adm get admlang:adm.interop.java
```

The library contains C code (the part that loads the Java VM). ADM normally asks before installing
a package that brings foreign code; official `admlang` packages are accepted without the question.
Running a program needs Java 8 or later: `JAVA_HOME`, or `java` on `PATH`.

## Binding Java methods to ADM functions

```adm
use adm.interop.java::*

@java("com.acme.Billing")
def total(prices float[], quantity int32) !float;

@java("com.acme.Billing")
def invoiceNumber(customer string) !?string;
```

```adm
try runtime().start(classPath = ["billing.jar"])
println(try total([19.90, 5.00], 3))
```

A function declared without a body under `@java("Class")` is the static method of the same name
in that class. The method is the one whose parameters are exactly the declaration's:

| ADM | Java |
|-----|------|
| `bool` | `boolean` |
| `int8`, `int16`, `int32`, `int` | `byte`, `short`, `int`, `long` |
| `float32`, `float` | `float`, `double` |
| `string` | `String` |
| `byte[]`, `int32[]`, `float[]`, `string[]`, … | `byte[]`, `int[]`, `double[]`, `String[]`, … |
| no result | `void` |

- Strings and arrays are copied each way. A string keeps every character: ADM text is UTF-8,
  Java's UTF-16.
- `?string` and `?T[]` as a result take Java's `null` as none; without the `?`, a null fails the
  call.
- `method = "name"` names the Java method when the declaration cannot have its name
  (`@java("Orders", method = "check")`: `check` is a keyword in ADM).
- A class or method that does not exist fails the first call, naming the Java signature the
  declaration asks for: ``class Sample has no method `static long subtract(long, long)` ``.

## Using the VM directly

`runtime()` gives the program's Java VM. Classes are loaded by name; objects, arrays and classes
are held as `Value`s.

```adm
let java = runtime()
let list = try (try java.load("java.util.ArrayList")).construct()
try list.invoke("add", "ADM")
try list.invoke("add", 42)
println(try (try list.invoke("size")).asInt())    // 2
println(try (try list.get(0)).asString())         // ADM
println("{list}")                                 // [ADM, 42]
```

### Classes, fields and methods

```adm
let math = try java.load("java.lang.Math")
let largest = try (try math.invoke("max", 3, 9)).asInt()       // a static method
let limit = try (try java.get("java.lang.Integer", "MAX_VALUE")).asInt()

let builder = try (try java.load("java.lang.StringBuilder")).construct("a")
try builder.invoke("append", 42)
println(try builder.text())                                     // a42
```

`get` and `set` by name reach public fields: static ones on a class value, instance ones on an
object.

`invoke` and `construct` pick, among the methods of that name, the one whose parameters take the
arguments without losing anything: an integer goes where a `long`, an `int` it fits or a `double`
is taken, a string where a `String` or a `char` is, and variable-arity methods take the rest of
the arguments. When no method fits, the call fails and says what it was given.

### Calling ADM from Java

```adm
let byLength = try java.function(def (args Value[]) !Value {
	return try java.value((try args[0].asString()).len() - (try args[1].asString()).len())
})
try list.invoke("sort", byLength)
```

An ADM function becomes an object of the interface it is passed as (a `Comparator` here), or of
the interface named in `java.function(fn, "java.lang.Runnable")`. Every method of the interface
runs the function with the call's arguments. The function runs on the ADM task that called into
Java, may call back into Java, and an error it fails with comes back unchanged from the Java call
that let it through.

## API

| | |
|---|---|
| `@java("Class")` | Binds a function declared without a body to the static method of the same name. |
| `runtime()` | The program's Java VM. |
| `Runtime` | `start` (class path and VM options), `load` (a class as a value), `get` (a static field), `function` for ADM functions Java calls, `value`/`bytes`/`foreign`/`null` to make values, `memory`, `live`, `collect`. |
| `Value` | A Java value held by ADM. `kind`, `isNone`, `asBool`/`asInt`/`asFloat`/`asString`/`asBytes`, `text`, `get`/`set` by name (fields) and by index (arrays, lists), `len`, `invoke`, `construct`, `foreign`, `same`, `equals`. |
| `ScriptError` | What a failing call fails with: `kind` (`Thrown`, `Binding`, `Unavailable`, `Conversion`), the Java stack trace (`trace`) and the exception as a `value`. |
| `HostFunction` | `def(Value[]) !Value`, an ADM function Java calls. |

The other `adm.interop` libraries have the same names. What a script engine has and a Java VM
does not is left out: there is no `eval`, `compile` or `define` (Java code comes compiled, from the
class path), and no `close`, `interrupt` or `timeout` (a Java VM cannot be stopped or restarted
inside a process).

## How values cross

| ADM | Java |
|---|---|
| `bool`, `string` | `boolean`, `String`; a `char` reads as a one-character string |
| `int`, `float` | a `long` or a `double`, or the narrower type a parameter takes when the value fits. Results of every integer and float type read as `int` and `float` |
| `byte[]`, `int8[]` | `byte[]`; `asBytes` reads one back |
| `int16[]`, `int32[]`, `int[]`, `float32[]`, `float[]`, `bool[]`, `string[]` | `short[]`, `int[]`, `long[]`, `float[]`, `double[]`, `boolean[]`, `String[]` (copied) |
| other arrays | `Object[]` of the converted elements |
| `map<string, V>` | a `java.util.LinkedHashMap` (copied) |
| `none` | `null` |
| struct, type object | an opaque object Java can hold and pass back (`Value.foreign()` returns it) |
| an ADM function | an object of an interface, through `Runtime.function` |

Where Java takes any `Object`, an integer arrives as an `Integer` when it fits and a `Long`
otherwise.

## The Java VM

`runtime().start(classPath, options)` starts the VM with folders and `.jar` files as its class
path and options such as `-Xmx256m` or `-Dname=value`. Without it, the first use starts the VM
with the class path of the `CLASSPATH` variable, or the current folder. A process has one Java VM,
for as long as it runs: a second `start` fails.

The VM is the one of the Java installation `JAVA_HOME` names, else of the `java` on `PATH`. It
runs on a thread of its own inside the program, and every call waits for that thread, so calls
from several tasks run one after the other.

## Limits

- A Java exception fails the call with a `ScriptError` of kind `Thrown`; a declaration that cannot
  fail (no `!`) panics with the message instead.
- An ADM function called from a thread Java started (a thread pool, a timer) runs on the next ADM
  task that calls into Java, and the Java thread waits until one does.
- Default methods of an interface an ADM function stands in for run from Java 16 on.
- Not there yet: generic type checks beyond what the VM does at run time, classes embedded in the
  program, checking declarations against the class files when the program is built.
- Linux is the platform it runs on today; macOS is written and not run, Windows not written.
