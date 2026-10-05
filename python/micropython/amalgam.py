#!/usr/bin/env python3
# amalgam.py TOP GENHDR OUTDIR: MicroPython's core and the modules adm.interop.python
# gives scripts, as micropython.h (every header adm_mpy.c needs, the configuration and the
# generated headers) and micropython-amalgam.c (the sources). TOP is the release tree,
# GENHDR the genhdr folder its embed port generated from mpconfigport.h and
# qstrdefsport.h (see UPSTREAM). The sources are copied unmodified; a header with an
# include guard is inlined where first included, any other include (the qstr and grammar
# tables, read several times under different macros) every time.
import glob, os, re, sys

top, genhdr, outdir = sys.argv[1], sys.argv[2], sys.argv[3]
port = os.path.dirname(os.path.abspath(__file__))

# What adm_mpy.c reaches; everything they include follows.
HEADERS = """py/runtime.h py/compile.h py/gc.h py/cstack.h py/stackctrl.h py/persistentcode.h
py/builtin.h py/lexer.h py/parse.h py/mphal.h py/mperrno.h py/objstr.h py/objlist.h
py/objtuple.h py/objtype.h py/objmodule.h py/objexcept.h py/objint.h py/objfun.h
py/objarray.h py/binary.h py/smallint.h py/stream.h py/mpprint.h
shared/runtime/gchelper.h""".split()

EXTMOD = ["extmod/modjson.c", "extmod/modre.c", "extmod/modheapq.c", "extmod/modbinascii.c",
          "extmod/modrandom.c"]
SOURCES = sorted(os.path.relpath(p, top) for p in glob.glob(os.path.join(top, "py", "*.c")))
# Read by the emitters that use them, under their own macros.
SOURCES = [s for s in SOURCES if s not in ("py/emitnative.c",)]
SOURCES += EXTMOD + ["shared/runtime/gchelper_generic.c"]

# File-local names two sources both use: the later source gets its own.
CLASHES = {"extmod/modbinascii.c": ["bytes_fromhex_obj"]}

SYSTEM = """alloca.h assert.h ctype.h errno.h float.h limits.h math.h setjmp.h stdarg.h
stdbool.h stddef.h stdint.h stdio.h stdlib.h string.h""".split()

include = re.compile(r'^\s*#\s*include\s+(?:"([^"]+)"|<((?:mpconfigport|mphalport|genhdr/)[^>]*)>)\s*(?://.*|/\*.*\*/\s*)?$')
guard = re.compile(r'^\s*#\s*ifndef\s+(MICROPY_INCLUDED_\w+|_INCLUDED_\w+|__\w+_H__?|\w+_H)\s*$', re.M)
define = re.compile(r'^\s*#\s*define\s+(\w+)', re.M)
seen = set()


def find(name, origin):
    roots = [top, os.path.dirname(origin), port, os.path.dirname(genhdr)]
    for root in roots:
        path = os.path.join(root, name)
        if os.path.isfile(path):
            return os.path.normpath(path)
    # Named under a condition this configuration turns off; the line stays.
    return None


def emit(path, w):
    text = open(path, encoding="utf-8").read()
    for line in text.splitlines(keepends=True):
        m = include.match(line)
        if not m:
            w.write(line)
            continue
        target = find(m.group(1) or m.group(2), path)
        if target is None:
            w.write(line)
            continue
        once = target.endswith(".c") or guard.search(open(target, encoding="utf-8").read())
        if once and target in seen:
            continue
        seen.add(target)
        w.write("/* %s */\n" % os.path.relpath(target, top if target.startswith(top) else os.path.dirname(target)))
        emit(target, w)
    if not text.endswith("\n"):
        w.write("\n")


with open(os.path.join(outdir, "micropython.h"), "w", encoding="utf-8") as w:
    w.write("""/*
 * MicroPython 1.29.0: the headers of the core, with the configuration of
 * adm.interop.python and the headers generated from it, in one file made from
 * the release's unmodified sources (see UPSTREAM).
 */
#ifndef ADM_MICROPYTHON_H
#define ADM_MICROPYTHON_H

/* MicroPython's asserts are its debug build; <assert.h> is also read again
   inside the hidden region below, where it must declare nothing. */
#ifndef NDEBUG
#define NDEBUG
#endif

""")
    for h in SYSTEM:
        w.write("#include <%s>\n" % h)
    w.write("""
/* MicroPython stays inside the program that links it. */
#if defined(__GNUC__)
#pragma GCC visibility push(hidden)
#endif

""")
    for h in HEADERS:
        target = find(h, top)
        if target in seen:
            continue
        seen.add(target)
        w.write("/* %s */\n" % h)
        emit(target, w)
    w.write("""
#if defined(__GNUC__)
#pragma GCC visibility pop
#endif

#endif
""")

with open(os.path.join(outdir, "micropython-amalgam.c"), "w", encoding="utf-8") as w:
    w.write("""/*
 * MicroPython 1.29.0: the core and the json, re, heapq, binascii and random
 * modules in one file, generated from the release's unmodified sources (see
 * UPSTREAM).
 */
#include "micropython.h"

#if defined(__GNUC__)
#pragma GCC visibility push(hidden)
#endif

""")
    for s in SOURCES:
        target = find(s, top)
        if target in seen:
            continue
        seen.add(target)
        w.write("/* ---- %s ---- */\n" % s)
        for name in CLASHES.get(s, []):
            w.write("#define %s %s_%s\n" % (name, os.path.basename(s)[:-2], name))
        emit(target, w)
        for name in CLASHES.get(s, []):
            w.write("#undef %s\n" % name)
        # A macro a source defines for itself must not reach the next source.
        for name in sorted(set(define.findall(open(target, encoding="utf-8").read()))):
            w.write("#undef %s\n" % name)
