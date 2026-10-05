#!/usr/bin/env python3
# amalgam.py SRC OUT: the Lua core and the libraries adm.interop.lua opens, as one C file.
# The sources are copied unmodified in the order upstream's onelua.c includes them; each
# internal header is inlined where first included, the four public headers stay #includes.
import re, sys, os
src, out = sys.argv[1], sys.argv[2]
PUBLIC = {"lua.h", "luaconf.h", "lualib.h", "lauxlib.h"}
FILES = """lzio lctype lopcodes lmem lundump ldump lstate lgc llex lcode lparser ldebug lfunc
lobject ltm lstring ltable ldo lvm lapi lauxlib lbaselib lcorolib lmathlib lstrlib ltablib
lutf8lib""".split()
seen = set()
inc = re.compile(r'^\s*#\s*include\s+"([^"]+)"\s*$')
def emit(name, w):
    for line in open(os.path.join(src, name), encoding="latin-1"):
        m = inc.match(line)
        if not m or m.group(1) in PUBLIC:
            w.write(line)
            continue
        h = m.group(1)
        if h in seen:
            continue
        seen.add(h)
        w.write("/* %s */\n" % h)
        emit(h, w)
with open(out, "w", encoding="latin-1") as w:
    w.write("""/*
** Lua 5.4.8: the core, the auxiliary library and the base, coroutine, math,
** string, table and utf8 libraries in one file, generated from the release's
** unmodified sources (see UPSTREAM). The io, os, package and debug libraries
** are left out.
*/
#include "lprefix_adm.h"
""".replace('#include "lprefix_adm.h"\n', ""))
    seen.add("lprefix.h")
    emit("lprefix.h", w)
    w.write("""
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* The Lua API stays inside the program that links this file. */
#if defined(__GNUC__)
#pragma GCC visibility push(hidden)
#endif

#define LUA_CORE
#define LUA_LIB
#define ltable_c
#define lvm_c
#include "luaconf.h"

#undef LUAI_FUNC
#undef LUAI_DDEC
#undef LUAI_DDEF
#define LUAI_FUNC	static
#define LUAI_DDEC(def)	/* empty */
#define LUAI_DDEF	static

""")
    for f in FILES:
        w.write("/* ---- %s.c ---- */\n" % f)
        emit(f + ".c", w)
