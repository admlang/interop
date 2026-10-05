/*
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

#include <alloca.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* MicroPython stays inside the program that links it. */
#if defined(__GNUC__)
#pragma GCC visibility push(hidden)
#endif

/* py/runtime.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_RUNTIME_H
#define MICROPY_INCLUDED_PY_RUNTIME_H

/* py/mpstate.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_MPSTATE_H
#define MICROPY_INCLUDED_PY_MPSTATE_H

#include <stdint.h>

/* py/mpconfig.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_MPCONFIG_H
#define MICROPY_INCLUDED_PY_MPCONFIG_H

#include <stdint.h>

#if defined(__cplusplus) // Required on at least one compiler to get ULLONG_MAX
#include <climits>
#else
#include <limits.h>
#endif


// Current version of MicroPython. This is used by sys.implementation.version
// as well as a fallback to generate MICROPY_GIT_TAG if the git repo or tags
// are unavailable.
#define MICROPY_VERSION_MAJOR 1
#define MICROPY_VERSION_MINOR 29
#define MICROPY_VERSION_MICRO 0
#define MICROPY_VERSION_PRERELEASE 0

// Combined version as a 32-bit number for convenience to allow version
// comparison. Doesn't include prerelease state.
// e.g. #if MICROPY_VERSION < MICROPY_MAKE_VERSION(1, 22, 0)
#define MICROPY_MAKE_VERSION(major, minor, patch) (major << 16 | minor << 8 | patch)
#define MICROPY_VERSION MICROPY_MAKE_VERSION(MICROPY_VERSION_MAJOR, MICROPY_VERSION_MINOR, MICROPY_VERSION_MICRO)

// String version. This is only used directly for platform.platform and
// os.uname().release. All other version info available in the firmware (e.g.
// the REPL banner) comes from MICROPY_GIT_TAG.
#define MICROPY_VERSION_STRING_BASE \
    MP_STRINGIFY(MICROPY_VERSION_MAJOR) "." \
    MP_STRINGIFY(MICROPY_VERSION_MINOR) "." \
    MP_STRINGIFY(MICROPY_VERSION_MICRO)
#if MICROPY_VERSION_PRERELEASE
#define MICROPY_VERSION_STRING MICROPY_VERSION_STRING_BASE "-preview"
#else
#define MICROPY_VERSION_STRING MICROPY_VERSION_STRING_BASE
#endif

// If this is enabled, then in-progress/breaking changes slated for the 2.x
// release will be enabled.
#ifndef MICROPY_PREVIEW_VERSION_2
#define MICROPY_PREVIEW_VERSION_2 (0)
#endif

// This file contains default configuration settings for MicroPython.
// You can override any of the options below using mpconfigport.h file
// located in a directory of your port.

// mpconfigport.h is a file containing configuration settings for a
// particular port. mpconfigport.h is actually a default name for
// such config, and it can be overridden using MP_CONFIGFILE preprocessor
// define (you can do that by passing CFLAGS_EXTRA='-DMP_CONFIGFILE="<file.h>"'
// argument to make when using standard MicroPython makefiles).
// This is useful to have more than one config per port, for example,
// release vs debug configs, etc. Note that if you switch from one config
// to another, you must rebuild from scratch using "-B" switch to make.

// Disable all optional features (i.e. minimal port).
#define MICROPY_CONFIG_ROM_LEVEL_MINIMUM (0)
// Only enable core features (constrained flash, e.g. STM32L072)
#define MICROPY_CONFIG_ROM_LEVEL_CORE_FEATURES (10)
// Enable most common features (small on-device flash, e.g. STM32F411)
#define MICROPY_CONFIG_ROM_LEVEL_BASIC_FEATURES (20)
// Enable convenience features (medium on-device flash, e.g. STM32F405)
#define MICROPY_CONFIG_ROM_LEVEL_EXTRA_FEATURES (30)
// Enable all common features (large/external flash, rp2, unix)
#define MICROPY_CONFIG_ROM_LEVEL_FULL_FEATURES (40)
// Enable everything (e.g. coverage)
#define MICROPY_CONFIG_ROM_LEVEL_EVERYTHING (50)

#ifdef MP_CONFIGFILE
#include MP_CONFIGFILE
#else
/* mpconfigport.h */
// MicroPython's configuration for adm.interop.python. The headers generated
// from it (qstrs, module table, root pointers) are part of micropython.h, so
// a change here means regenerating: see UPSTREAM.

#include <stddef.h>
#include <stdint.h>
#include <alloca.h>

#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_EXTRA_FEATURES)

#define MICROPY_OBJ_REPR (MICROPY_OBJ_REPR_A)
#define MICROPY_LONGINT_IMPL (MICROPY_LONGINT_IMPL_MPZ)
#define MICROPY_FLOAT_IMPL (MICROPY_FLOAT_IMPL_DOUBLE)
#define MICROPY_ERROR_REPORTING (MICROPY_ERROR_REPORTING_DETAILED)
#define MICROPY_WARNINGS (0)
#define MICROPY_ENABLE_SOURCE_LINE (1)
#define MICROPY_ENABLE_DOC_STRING (1)

#define MICROPY_ENABLE_COMPILER (1)
#define MICROPY_ENABLE_EXTERNAL_IMPORT (1)
#define MICROPY_PERSISTENT_CODE_LOAD (1)
#define MICROPY_PERSISTENT_CODE_SAVE (1)
#define MICROPY_PY_BUILTINS_COMPILE (1)
#define MICROPY_PY_BUILTINS_EXECFILE (0)
#define MICROPY_PY_BUILTINS_INPUT (0)
#define MICROPY_PY_BUILTINS_HELP (0)
#define MICROPY_HELPER_REPL (0)
#define MICROPY_REPL_EVENT_DRIVEN (0)
#define MICROPY_KBD_EXCEPTION (1)

// The heap grows in areas the shim allocates and counts.
#define MICROPY_ENABLE_GC (1)
#define MICROPY_ENABLE_FINALISER (1)
#define MICROPY_GC_SPLIT_HEAP (1)
#define MICROPY_GC_SPLIT_HEAP_AUTO (1)
#define MICROPY_GCREGS_SETJMP (1)
void* ampy_heap_alloc(size_t size);
void ampy_heap_free(void* area);
#define MP_PLAT_ALLOC_HEAP(size) ampy_heap_alloc(size)
#define MP_PLAT_FREE_HEAP(ptr) ampy_heap_free(ptr)

// Errors jump with setjmp/longjmp on every target; the C stack is measured
// from the entry the shim marks.
#define MICROPY_NLR_SETJMP (1)
#define MICROPY_STACK_CHECK (1)
#define MICROPY_ENABLE_SCHEDULER (0)

// Runs where the interpreter jumps backwards: the interrupt flag and the
// deadline are looked at there.
extern int ampy_ticks;
void ampy_poll(void);
#define MICROPY_VM_HOOK_LOOP \
    if (--ampy_ticks <= 0) ampy_poll();

// No files, clock, process or devices: scripts import the modules the program
// defined (mp_import_stat and mp_lexer_new_from_file in the shim).
#define MICROPY_VFS (0)
#define MICROPY_READER_POSIX (0)
#define MICROPY_READER_VFS (0)
#define MICROPY_PY_IO (1)
#define MICROPY_PY_SYS (1)
#define MICROPY_PY_SYS_PATH (0)
#define MICROPY_PY_SYS_ARGV (0)
#define MICROPY_PY_SYS_EXIT (0)
#define MICROPY_PY_SYS_STDFILES (0)
#define MICROPY_PY_SYS_STDIO_BUFFER (0)
#define MICROPY_PY_SYS_PS1_PS2 (0)
#define MICROPY_PY_SYS_PLATFORM "adm"
#define MICROPY_PY_TIME (0)
#define MICROPY_PY_OS (0)
#define MICROPY_PY_SELECT (0)
#define MICROPY_PY_SELECT_SELECT (0)
#define MICROPY_PY_ASYNCIO (0)
#define MICROPY_PY_UCTYPES (0)
#define MICROPY_PY_DEFLATE (0)
#define MICROPY_PY_FRAMEBUF (0)
#define MICROPY_PY_HASHLIB (0)
#define MICROPY_PY_PLATFORM (0)
#define MICROPY_PY_MACHINE (0)
#define MICROPY_PY_MICROPYTHON_RINGIO (0)
#define MICROPY_PY_MICROPYTHON_MEM_INFO (0)

#define MICROPY_PY_JSON (1)
#define MICROPY_PY_RE (1)
#define MICROPY_PY_RE_SUB (1)
#define MICROPY_PY_RE_MATCH_GROUPS (1)
#define MICROPY_PY_RE_MATCH_SPAN_START_END (1)
#define MICROPY_PY_HEAPQ (1)
#define MICROPY_PY_BINASCII (1)
#define MICROPY_PY_BINASCII_CRC32 (0)
#define MICROPY_PY_RANDOM (1)
#define MICROPY_PY_RANDOM_EXTRA_FUNCS (1)
#define MICROPY_PY_COLLECTIONS_NAMEDTUPLE__ASDICT (1)
#define MICROPY_PY_ALL_INPLACE_SPECIAL_METHODS (1)
#define MICROPY_PY_BUILTINS_RANGE_BINOP (1)
#define MICROPY_PY_SYS_GETSIZEOF (1)
#define MICROPY_PY_SYS_TRACEBACKLIMIT (0)

typedef long mp_off_t;

#define MICROPY_HW_BOARD_NAME "adm"
#define MICROPY_HW_MCU_NAME "adm.interop.python"
#define MICROPY_BANNER_MACHINE "adm.interop.python"

// print() and tracebacks write through the shim.
void ampy_print(const char* str, size_t len);
#define MP_PLAT_PRINT_STRN(str, len) ampy_print(str, len)

// No pins.
#define mp_hal_pin_obj_t
#endif

// Ports/boards should set this, but default to level=core.
#ifndef MICROPY_CONFIG_ROM_LEVEL
#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_CORE_FEATURES)
#endif

// Helper macros for "have at least this level".
#define MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES (MICROPY_CONFIG_ROM_LEVEL >= MICROPY_CONFIG_ROM_LEVEL_CORE_FEATURES)
#define MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_BASIC_FEATURES (MICROPY_CONFIG_ROM_LEVEL >= MICROPY_CONFIG_ROM_LEVEL_BASIC_FEATURES)
#define MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES (MICROPY_CONFIG_ROM_LEVEL >= MICROPY_CONFIG_ROM_LEVEL_EXTRA_FEATURES)
#define MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_FULL_FEATURES (MICROPY_CONFIG_ROM_LEVEL >= MICROPY_CONFIG_ROM_LEVEL_FULL_FEATURES)
#define MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EVERYTHING (MICROPY_CONFIG_ROM_LEVEL >= MICROPY_CONFIG_ROM_LEVEL_EVERYTHING)

// Any options not explicitly set in mpconfigport.h will get default
// values below.

/*****************************************************************************/
/* Object representation                                                     */

// A MicroPython object is a machine word having the following form:
//  - xxxx...xxx1 : a small int, bits 1 and above are the value
//  - xxxx...x010 : a qstr, bits 3 and above are the value
//  - xxxx...x110 : an immediate object, bits 3 and above are the value
//  - xxxx...xx00 : a pointer to an mp_obj_base_t (unless a fake object)
#define MICROPY_OBJ_REPR_A (0)

// A MicroPython object is a machine word having the following form:
//  - xxxx...xx01 : a small int, bits 2 and above are the value
//  - xxxx...x011 : a qstr, bits 3 and above are the value
//  - xxxx...x111 : an immediate object, bits 3 and above are the value
//  - xxxx...xxx0 : a pointer to an mp_obj_base_t (unless a fake object)
#define MICROPY_OBJ_REPR_B (1)

// A MicroPython object is a machine word having the following form (called R):
//  - iiiiiiii iiiiiiii iiiiiiii iiiiiii1 small int with 31-bit signed value
//  - 01111111 1qqqqqqq qqqqqqqq qqqq0110 str with 19-bit qstr value
//  - 01111111 10000000 00000000 ssss1110 immediate object with 4-bit value
//  - s1111111 10000000 00000000 00000010 +/- inf
//  - s1111111 1xxxxxxx xxxxxxxx xxxxx010 nan, x != 0
//  - seeeeeee efffffff ffffffff ffffff10 30-bit fp, e != 0xff
//  - pppppppp pppppppp pppppppp pppppp00 ptr (4 byte alignment)
// Str, immediate and float stored as O = R + 0x80800000, retrieved as R = O - 0x80800000.
// This makes strs/immediates easier to encode/decode as they have zeros in the top 9 bits.
// This scheme only works with 32-bit word size and float enabled.
#define MICROPY_OBJ_REPR_C (2)

// A MicroPython object is a 64-bit word having the following form (called R):
//  - seeeeeee eeeeffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff 64-bit fp, e != 0x7ff
//  - s1111111 11110000 00000000 00000000 00000000 00000000 00000000 00000000 +/- inf
//  - 01111111 11111000 00000000 00000000 00000000 00000000 00000000 00000000 normalised nan
//  - 01111111 11111101 iiiiiiii iiiiiiii iiiiiiii iiiiiiii iiiiiiii iiiiiii1 small int
//  - 01111111 11111110 00000000 00000000 qqqqqqqq qqqqqqqq qqqqqqqq qqqqqqq1 str
//  - 01111111 11111111 ss000000 00000000 00000000 00000000 00000000 00000000 immediate object
//  - 01111111 11111100 00000000 00000000 pppppppp pppppppp pppppppp pppppp00 ptr (4 byte alignment)
// Stored as O = R + 0x8004000000000000, retrieved as R = O - 0x8004000000000000.
// This makes pointers have all zeros in the top 32 bits.
// Small-ints and strs have 1 as LSB to make sure they don't look like pointers
// to the garbage collector.
#define MICROPY_OBJ_REPR_D (3)

#ifndef MICROPY_OBJ_REPR
#define MICROPY_OBJ_REPR (MICROPY_OBJ_REPR_A)
#endif

// Whether to encode None/False/True as immediate objects instead of pointers to
// real objects.  Reduces code size by a decent amount without hurting
// performance, for all representations except D on some architectures.
#ifndef MICROPY_OBJ_IMMEDIATE_OBJS
#define MICROPY_OBJ_IMMEDIATE_OBJS (MICROPY_OBJ_REPR != MICROPY_OBJ_REPR_D)
#endif

// Definition of the `mp_int_t` and `mp_uint_t` types and associated macros.
// Normally, it suffices for the platform to do nothing: A type as wide
// as a pointer is chosen, unless nanboxing (REPR_D) is selected, in
// which case a 64-bit type is chosen to match the assumed size of
// double-precision floats.
//
// In the case of exceptions, the port, board, or variant must define
// MP_INT_TYPE as MP_INT_TYPE_OTHER and provide all the typedefs and
// defines.
#define MP_INT_TYPE_INTPTR (0)
#define MP_INT_TYPE_INT64 (1)
#define MP_INT_TYPE_OTHER (2)

#if !defined(MP_INT_TYPE)
#if MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_D
#define MP_INT_TYPE (MP_INT_TYPE_INT64)
#else
#define MP_INT_TYPE (MP_INT_TYPE_INTPTR)
#endif
#endif

#if MP_INT_TYPE == MP_INT_TYPE_INTPTR
typedef intptr_t mp_int_t;
typedef uintptr_t mp_uint_t;
#define MP_INT_MAX INTPTR_MAX
#define MP_INT_MIN INTPTR_MIN
#define MP_UINT_MAX INTPTR_UMAX
#elif MP_INT_TYPE == MP_INT_TYPE_INT64
typedef int64_t mp_int_t;
typedef uint64_t mp_uint_t;
#define MP_INT_MAX INT64_MAX
#define MP_INT_MIN INT64_MIN
#define MP_UINT_MAX INT64_UMAX
#endif

// mp_printf format support for mp_int_t. In the unusual case that MP_INT_MAX doesn't
// match any of the standard C types (int/long/long long), provide all 3
// macros. Otherwise, rely on these automatic definitions.
#if !defined(INT_FMT)
#if MP_INT_MAX == INT_MAX
#define INT_FMT "%d"
#define UINT_FMT "%u"
#define HEX_FMT "%x"
#elif MP_INT_MAX == LONG_MAX
#define INT_FMT "%ld"
#define UINT_FMT "%lu"
#define HEX_FMT "%lx"
#elif MP_INT_MAX == LLONG_MAX
#define INT_FMT "%lld"
#define UINT_FMT "%llu"
#define HEX_FMT "%llx"
#else
#error Unexpected MP_INT_MAX value
#endif
#endif

// mp_printf format support for size_t. In the unusual case that SIZE_MAX doesn't
// match any of the standard C types (int/long/long long), provide a
// macro. Otherwise, rely on these automatic definitions.
#if !defined(SIZE_FMT)
#if SIZE_MAX == UINT_MAX
#define SIZE_FMT "%u"
#elif SIZE_MAX == ULONG_MAX
#define SIZE_FMT "%lu"
#elif SIZE_MAX == ULLONG_MAX
#define SIZE_FMT "%llu"
#else
#error Unexpected SIZE_MAX value
#endif
#endif


/*****************************************************************************/
/* Memory allocation policy                                                  */

// Number of bytes in memory allocation/GC block. Any size allocated will be
// rounded up to be multiples of this.
#ifndef MICROPY_BYTES_PER_GC_BLOCK
#define MICROPY_BYTES_PER_GC_BLOCK (4 * MP_BYTES_PER_OBJ_WORD)
#endif

// Number of words allocated (in BSS) to the GC stack (minimum is 1)
#ifndef MICROPY_ALLOC_GC_STACK_SIZE
#define MICROPY_ALLOC_GC_STACK_SIZE (64)
#endif

// The C-type to use for entries in the GC stack.  By default it allows the
// heap to be as large as the address space, but the bit-width of this type can
// be reduced to save memory when the heap is small enough.  The type must be
// big enough to index all blocks in the heap, which is set by
// heap-size-in-bytes / MICROPY_BYTES_PER_GC_BLOCK.
#ifndef MICROPY_GC_STACK_ENTRY_TYPE
#define MICROPY_GC_STACK_ENTRY_TYPE size_t
#endif

// Be conservative and always clear to zero newly (re)allocated memory in the GC.
// This helps eliminate stray pointers that hold on to memory that's no longer
// used.  It decreases performance due to unnecessary memory clearing.
// A memory manager which always clears memory can set this to 0.
// TODO Do analysis to understand why some memory is not properly cleared and
// find a more efficient way to clear it.
#ifndef MICROPY_GC_CONSERVATIVE_CLEAR
#define MICROPY_GC_CONSERVATIVE_CLEAR (MICROPY_ENABLE_GC)
#endif

// Support automatic GC when reaching allocation threshold,
// configurable by gc.threshold().
#ifndef MICROPY_GC_ALLOC_THRESHOLD
#define MICROPY_GC_ALLOC_THRESHOLD (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Number of bytes to allocate initially when creating new chunks to store
// interned string data.  Smaller numbers lead to more chunks being needed
// and more wastage at the end of the chunk.  Larger numbers lead to wasted
// space at the end when no more strings need interning.
#ifndef MICROPY_ALLOC_QSTR_CHUNK_INIT
#define MICROPY_ALLOC_QSTR_CHUNK_INIT (128)
#endif

// Initial amount for lexer indentation level
#ifndef MICROPY_ALLOC_LEXER_INDENT_INIT
#define MICROPY_ALLOC_LEXER_INDENT_INIT (10)
#endif

// Increment for lexer indentation level
#ifndef MICROPY_ALLOC_LEXEL_INDENT_INC
#define MICROPY_ALLOC_LEXEL_INDENT_INC (8)
#endif

// Initial amount for parse rule stack
#ifndef MICROPY_ALLOC_PARSE_RULE_INIT
#define MICROPY_ALLOC_PARSE_RULE_INIT (64)
#endif

// Increment for parse rule stack
#ifndef MICROPY_ALLOC_PARSE_RULE_INC
#define MICROPY_ALLOC_PARSE_RULE_INC (16)
#endif

// Initial amount for parse result stack
#ifndef MICROPY_ALLOC_PARSE_RESULT_INIT
#define MICROPY_ALLOC_PARSE_RESULT_INIT (32)
#endif

// Increment for parse result stack
#ifndef MICROPY_ALLOC_PARSE_RESULT_INC
#define MICROPY_ALLOC_PARSE_RESULT_INC (16)
#endif

// Strings this length or less will be interned by the parser
#ifndef MICROPY_ALLOC_PARSE_INTERN_STRING_LEN
#define MICROPY_ALLOC_PARSE_INTERN_STRING_LEN (10)
#endif

// Number of bytes to allocate initially when creating new chunks to store
// parse nodes.  Small leads to fragmentation, large leads to excess use.
#ifndef MICROPY_ALLOC_PARSE_CHUNK_INIT
#define MICROPY_ALLOC_PARSE_CHUNK_INIT (128)
#endif

// Initial amount for ids in a scope
#ifndef MICROPY_ALLOC_SCOPE_ID_INIT
#define MICROPY_ALLOC_SCOPE_ID_INIT (4)
#endif

// Increment for ids in a scope
#ifndef MICROPY_ALLOC_SCOPE_ID_INC
#define MICROPY_ALLOC_SCOPE_ID_INC (6)
#endif

// Maximum length of a path in the filesystem
// So we can allocate a buffer on the stack for path manipulation in import
#ifndef MICROPY_ALLOC_PATH_MAX
#define MICROPY_ALLOC_PATH_MAX (512)
#endif

// Initial size of module dict
#ifndef MICROPY_MODULE_DICT_SIZE
#define MICROPY_MODULE_DICT_SIZE (1)
#endif

// Initial size of sys.modules dict
#ifndef MICROPY_LOADED_MODULES_DICT_SIZE
#define MICROPY_LOADED_MODULES_DICT_SIZE (3)
#endif

// Whether realloc/free should be passed allocated memory region size
// You must enable this if MICROPY_MEM_STATS is enabled
#ifndef MICROPY_MALLOC_USES_ALLOCATED_SIZE
#define MICROPY_MALLOC_USES_ALLOCATED_SIZE (0)
#endif

// Number of bytes used to store qstr length
// Dictates hard limit on maximum Python identifier length, but 1 byte
// (limit of 255 bytes in an identifier) should be enough for everyone
#ifndef MICROPY_QSTR_BYTES_IN_LEN
#define MICROPY_QSTR_BYTES_IN_LEN (1)
#endif

// Number of bytes used to store qstr hash
#ifndef MICROPY_QSTR_BYTES_IN_HASH
#if MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES
#define MICROPY_QSTR_BYTES_IN_HASH (2)
#elif MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES
#define MICROPY_QSTR_BYTES_IN_HASH (1)
#else
#define MICROPY_QSTR_BYTES_IN_HASH (0)
#endif
#endif

// Avoid using C stack when making Python function calls. C stack still
// may be used if there's no free heap.
#ifndef MICROPY_STACKLESS
#define MICROPY_STACKLESS (0)
#endif

// Never use C stack when making Python function calls. This may break
// testsuite as will subtly change which exception is thrown in case
// of too deep recursion and other similar cases.
#ifndef MICROPY_STACKLESS_STRICT
#define MICROPY_STACKLESS_STRICT (0)
#endif

// Don't use alloca calls. As alloca() is not part of ANSI C, this
// workaround option is provided for compilers lacking this de-facto
// standard function. The way it works is allocating from heap, and
// relying on garbage collection to free it eventually. This is of
// course much less optimal than real alloca().
#if defined(MICROPY_NO_ALLOCA) && MICROPY_NO_ALLOCA
#undef alloca
#define alloca(x) m_malloc(x)
#endif

/*****************************************************************************/
/* MicroPython emitters                                                     */

// Whether to support loading of persistent code
#ifndef MICROPY_PERSISTENT_CODE_LOAD
#define MICROPY_PERSISTENT_CODE_LOAD (0)
#endif

// Whether to support loading of persistent native code
#ifndef MICROPY_PERSISTENT_CODE_LOAD_NATIVE
#define MICROPY_PERSISTENT_CODE_LOAD_NATIVE (MICROPY_EMIT_MACHINE_CODE)
#endif

// Whether to support saving of persistent code, i.e. for mpy-cross to
// generate .mpy files. Enabling this enables additional metadata on raw code
// objects which is also required for sys.settrace.
#ifndef MICROPY_PERSISTENT_CODE_SAVE
#define MICROPY_PERSISTENT_CODE_SAVE (MICROPY_PY_SYS_SETTRACE)
#endif

// Whether to support saving persistent code to a file via mp_raw_code_save_file
#ifndef MICROPY_PERSISTENT_CODE_SAVE_FILE
#define MICROPY_PERSISTENT_CODE_SAVE_FILE (0)
#endif

// Whether to support converting functions to persistent code (bytes)
#ifndef MICROPY_PERSISTENT_CODE_SAVE_FUN
#define MICROPY_PERSISTENT_CODE_SAVE_FUN (MICROPY_PY_MARSHAL)
#endif

// Whether generated code can persist independently of the VM/runtime instance
// This is enabled automatically when needed by other features
#ifndef MICROPY_PERSISTENT_CODE
#define MICROPY_PERSISTENT_CODE (MICROPY_PERSISTENT_CODE_LOAD || MICROPY_PERSISTENT_CODE_LOAD_NATIVE || MICROPY_PERSISTENT_CODE_SAVE || MICROPY_MODULE_FROZEN_MPY)
#endif

// Whether bytecode uses a qstr_table to map internal qstr indices in the bytecode
// to global qstr values in the runtime (behaviour when feature is enabled), or
// just stores global qstr values directly in the bytecode.  This must be enabled
// if MICROPY_PERSISTENT_CODE is enabled.
#ifndef MICROPY_EMIT_BYTECODE_USES_QSTR_TABLE
#define MICROPY_EMIT_BYTECODE_USES_QSTR_TABLE (MICROPY_PERSISTENT_CODE)
#endif

// Whether to emit x64 native code
#ifndef MICROPY_EMIT_X64
#define MICROPY_EMIT_X64 (0)
#endif

// Whether to emit x86 native code
#ifndef MICROPY_EMIT_X86
#define MICROPY_EMIT_X86 (0)
#endif

// Whether to emit thumb native code
#ifndef MICROPY_EMIT_THUMB
#define MICROPY_EMIT_THUMB (0)
#endif

// Whether to emit ARMv7-M instruction support in thumb native code
#ifndef MICROPY_EMIT_THUMB_ARMV7M
#if defined(__ARM_ARCH_ISA_THUMB) && __ARM_ARCH_ISA_THUMB == 2
#define MICROPY_EMIT_THUMB_ARMV7M (1)
#else
#define MICROPY_EMIT_THUMB_ARMV7M (0)
#endif
#endif

// Whether to enable the thumb inline assembler
#ifndef MICROPY_EMIT_INLINE_THUMB
#define MICROPY_EMIT_INLINE_THUMB (0)
#endif

// Whether to enable float support in the Thumb2 inline assembler
#ifndef MICROPY_EMIT_INLINE_THUMB_FLOAT
#if defined(__ARM_ARCH_ISA_THUMB) && __ARM_ARCH_ISA_THUMB == 2 && defined(__ARM_FP)
#define MICROPY_EMIT_INLINE_THUMB_FLOAT (1)
#else
#define MICROPY_EMIT_INLINE_THUMB_FLOAT (0)
#endif
#endif

// Whether to emit ARM native code
#ifndef MICROPY_EMIT_ARM
#define MICROPY_EMIT_ARM (0)
#endif

// Whether to emit Xtensa native code
#ifndef MICROPY_EMIT_XTENSA
#define MICROPY_EMIT_XTENSA (0)
#endif

// Whether to enable the Xtensa inline assembler
#ifndef MICROPY_EMIT_INLINE_XTENSA
#define MICROPY_EMIT_INLINE_XTENSA (0)
#endif

// Whether to support uncommon Xtensa inline assembler opcodes
#ifndef MICROPY_EMIT_INLINE_XTENSA_UNCOMMON_OPCODES
#define MICROPY_EMIT_INLINE_XTENSA_UNCOMMON_OPCODES (0)
#endif

// Whether to emit Xtensa-Windowed native code
#ifndef MICROPY_EMIT_XTENSAWIN
#define MICROPY_EMIT_XTENSAWIN (0)
#endif

// Whether to emit RISC-V RV32 native code
#ifndef MICROPY_EMIT_RV32
#define MICROPY_EMIT_RV32 (0)
#endif

// Whether to emit RISC-V RV32 Zba opcodes in native code
#ifndef MICROPY_EMIT_RV32_ZBA
#define MICROPY_EMIT_RV32_ZBA (0)
#endif

// Whether to emit RISC-V RV32 Zcmp opcodes in native code
#ifndef MICROPY_EMIT_RV32_ZCMP
#define MICROPY_EMIT_RV32_ZCMP (0)
#endif

// Whether to enable the RISC-V RV32 inline assembler
#ifndef MICROPY_EMIT_INLINE_RV32
#define MICROPY_EMIT_INLINE_RV32 (0)
#endif

// Whether to enable the human-readable native instructions emitter
#ifndef MICROPY_EMIT_NATIVE_DEBUG
#define MICROPY_EMIT_NATIVE_DEBUG (0)
#endif

// Convenience definition for whether any native emitter is enabled
#define MICROPY_EMIT_NATIVE (MICROPY_EMIT_X64 || MICROPY_EMIT_X86 || MICROPY_EMIT_THUMB || MICROPY_EMIT_ARM || MICROPY_EMIT_XTENSA || MICROPY_EMIT_XTENSAWIN || MICROPY_EMIT_RV32 || MICROPY_EMIT_NATIVE_DEBUG)

// Some architectures cannot read byte-wise from executable memory.  In this case
// the prelude for a native function (which usually sits after the machine code)
// must be separated and placed somewhere where it can be read byte-wise.
#define MICROPY_EMIT_NATIVE_PRELUDE_SEPARATE_FROM_MACHINE_CODE (MICROPY_EMIT_XTENSAWIN)

// Convenience definition for whether any inline assembler emitter is enabled
#define MICROPY_EMIT_INLINE_ASM (MICROPY_EMIT_INLINE_THUMB || MICROPY_EMIT_INLINE_XTENSA || MICROPY_EMIT_INLINE_RV32)

// Convenience definition for whether any native or inline assembler emitter is enabled
#define MICROPY_EMIT_MACHINE_CODE (MICROPY_EMIT_NATIVE || MICROPY_EMIT_INLINE_ASM)

// Convenience definition for whether native code has to be dealt with (either
// generated or loaded from a file).  This does not cover inline asm code.
#define MICROPY_ENABLE_NATIVE_CODE (MICROPY_EMIT_NATIVE || MICROPY_PERSISTENT_CODE_LOAD_NATIVE)

/*****************************************************************************/
/* Compiler configuration                                                    */

// Whether to include the compiler
#ifndef MICROPY_ENABLE_COMPILER
#define MICROPY_ENABLE_COMPILER (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether the compiler is dynamically configurable (ie at runtime)
// This will disable the ability to execute native/viper code
#ifndef MICROPY_DYNAMIC_COMPILER
#define MICROPY_DYNAMIC_COMPILER (0)
#endif

// Whether the compiler allows compiling top-level await expressions
#ifndef MICROPY_COMP_ALLOW_TOP_LEVEL_AWAIT
#define MICROPY_COMP_ALLOW_TOP_LEVEL_AWAIT (0)
#endif

// Whether to enable constant folding; eg 1+2 rewritten as 3
#ifndef MICROPY_COMP_CONST_FOLDING
#define MICROPY_COMP_CONST_FOLDING (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to compile constant tuples immediately to their respective objects; eg (1, True)
// Otherwise the tuple will be built at runtime
#ifndef MICROPY_COMP_CONST_TUPLE
#define MICROPY_COMP_CONST_TUPLE (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to enable optimisations for constant literals, eg OrderedDict
#ifndef MICROPY_COMP_CONST_LITERAL
#define MICROPY_COMP_CONST_LITERAL (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to enable lookup of constants in modules; eg module.CONST
#ifndef MICROPY_COMP_MODULE_CONST
#define MICROPY_COMP_MODULE_CONST (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to enable constant optimisation; id = const(value)
#ifndef MICROPY_COMP_CONST
#define MICROPY_COMP_CONST (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to enable float constant folding like 1.2+3.4 (when MICROPY_COMP_CONST_FOLDING is also enabled)
// and constant optimisation like id = const(1.2) (when MICROPY_COMP_CONST is also enabled)
// and constant lookup like math.inf (when MICROPY_COMP_MODULE_CONST is also enabled)
#ifndef MICROPY_COMP_CONST_FLOAT
#define MICROPY_COMP_CONST_FLOAT (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to enable optimisation of: a, b = c, d
// Costs 124 bytes (Thumb2)
#ifndef MICROPY_COMP_DOUBLE_TUPLE_ASSIGN
#define MICROPY_COMP_DOUBLE_TUPLE_ASSIGN (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to enable optimisation of: a, b, c = d, e, f
// Requires MICROPY_COMP_DOUBLE_TUPLE_ASSIGN and costs 68 bytes (Thumb2)
#ifndef MICROPY_COMP_TRIPLE_TUPLE_ASSIGN
#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to enable optimisation of: return a if b else c
// Costs about 80 bytes (Thumb2) and saves 2 bytes of bytecode for each use
#ifndef MICROPY_COMP_RETURN_IF_EXPR
#define MICROPY_COMP_RETURN_IF_EXPR (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

/*****************************************************************************/
/* Internal debugging stuff                                                  */

// Whether to collect memory allocation stats
#ifndef MICROPY_MEM_STATS
#define MICROPY_MEM_STATS (0)
#endif

// The mp_print_t printer used for debugging output
#ifndef MICROPY_DEBUG_PRINTER
#define MICROPY_DEBUG_PRINTER (&mp_plat_print)
#endif

// Whether to build functions that print debugging info:
//   mp_bytecode_print
//   mp_parse_node_print
#ifndef MICROPY_DEBUG_PRINTERS
#define MICROPY_DEBUG_PRINTERS (0)
#endif

// Whether to enable all debugging outputs (it will be extremely verbose)
#ifndef MICROPY_DEBUG_VERBOSE
#define MICROPY_DEBUG_VERBOSE (0)
#endif

// Whether to enable debugging versions of MP_OBJ_NULL/STOP_ITERATION/SENTINEL
#ifndef MICROPY_DEBUG_MP_OBJ_SENTINELS
#define MICROPY_DEBUG_MP_OBJ_SENTINELS (0)
#endif

// Whether to print parse rule names (rather than integers) in mp_parse_node_print
#ifndef MICROPY_DEBUG_PARSE_RULE_NAME
#define MICROPY_DEBUG_PARSE_RULE_NAME (0)
#endif

// Whether to enable a simple VM stack overflow check
#ifndef MICROPY_DEBUG_VM_STACK_OVERFLOW
#define MICROPY_DEBUG_VM_STACK_OVERFLOW (0)
#endif

// Whether to enable extra instrumentation for valgrind
#ifndef MICROPY_DEBUG_VALGRIND
#define MICROPY_DEBUG_VALGRIND (0)
#endif

/*****************************************************************************/
/* Optimisations                                                             */

// Whether to use computed gotos in the VM, or a switch
// Computed gotos are roughly 10% faster, and increase VM code size by a little,
// e.g. ~1kiB on Cortex M4.
// Note: enabling this will use the gcc-specific extensions of ranged designated
// initialisers and addresses of labels, which are not part of the C99 standard.
#ifndef MICROPY_OPT_COMPUTED_GOTO
#define MICROPY_OPT_COMPUTED_GOTO (0)
#endif

// Optimise the fast path for loading attributes from instance types. Increases
// Thumb2 code size by about 48 bytes.
#ifndef MICROPY_OPT_LOAD_ATTR_FAST_PATH
#define MICROPY_OPT_LOAD_ATTR_FAST_PATH (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Use extra RAM to cache map lookups by remembering the likely location of
// the index. Avoids the hash computation on unordered maps, and avoids the
// linear search on ordered (especially in-ROM) maps. Can provide a +10-15%
// performance improvement on benchmarks involving lots of attribute access
// or dictionary lookup.
#ifndef MICROPY_OPT_MAP_LOOKUP_CACHE
#define MICROPY_OPT_MAP_LOOKUP_CACHE (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// How much RAM (in bytes) to use for the map lookup cache.
#ifndef MICROPY_OPT_MAP_LOOKUP_CACHE_SIZE
#define MICROPY_OPT_MAP_LOOKUP_CACHE_SIZE (128)
#endif

// Whether to use fast versions of bitwise operations (and, or, xor) when the
// arguments are both positive.  Increases Thumb2 code size by about 250 bytes.
#ifndef MICROPY_OPT_MPZ_BITWISE
#define MICROPY_OPT_MPZ_BITWISE (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif


// Whether math.factorial is large, fast and recursive (1) or small and slow (0).
#ifndef MICROPY_OPT_MATH_FACTORIAL
#define MICROPY_OPT_MATH_FACTORIAL (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

/*****************************************************************************/
/* Python internal features                                                  */

// Use a special long jump in nlrthumb.c, which may be necessary if nlr.o and
// nlrthumb.o are linked far apart from each other.
#ifndef MICROPY_NLR_THUMB_USE_LONG_JUMP
#define MICROPY_NLR_THUMB_USE_LONG_JUMP (0)
#endif

// Whether to enable import of external modules
// When disabled, only importing of built-in modules is supported
// When enabled, a port must implement mp_import_stat (among other things)
#ifndef MICROPY_ENABLE_EXTERNAL_IMPORT
#define MICROPY_ENABLE_EXTERNAL_IMPORT (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to use the POSIX reader for importing files
#ifndef MICROPY_READER_POSIX
#define MICROPY_READER_POSIX (0)
#endif

// Whether to use the VFS reader for importing files
#ifndef MICROPY_READER_VFS
#define MICROPY_READER_VFS (0)
#endif

// Whether any readers have been defined
#ifndef MICROPY_HAS_FILE_READER
#define MICROPY_HAS_FILE_READER (MICROPY_READER_POSIX || MICROPY_READER_VFS)
#endif

// Hook for the VM at the start of the opcode loop (can contain variable
// definitions usable by the other hook functions)
#ifndef MICROPY_VM_HOOK_INIT
#define MICROPY_VM_HOOK_INIT
#endif

// Hook for the VM during the opcode loop (but only after jump opcodes)
#ifndef MICROPY_VM_HOOK_LOOP
#define MICROPY_VM_HOOK_LOOP
#endif

// Hook for the VM just before return opcode is finished being interpreted
#ifndef MICROPY_VM_HOOK_RETURN
#define MICROPY_VM_HOOK_RETURN
#endif

// Hook for mp_sched_schedule when a function gets scheduled on sched_queue
// (this macro executes within an atomic section)
#ifndef MICROPY_SCHED_HOOK_SCHEDULED
#define MICROPY_SCHED_HOOK_SCHEDULED
#endif

// Whether to include the garbage collector
#ifndef MICROPY_ENABLE_GC
#define MICROPY_ENABLE_GC (0)
#endif

// Whether the garbage-collected heap can be split over multiple memory areas.
#ifndef MICROPY_GC_SPLIT_HEAP
#define MICROPY_GC_SPLIT_HEAP (0)
#endif

// Whether regions should be added/removed from the split heap as needed.
#ifndef MICROPY_GC_SPLIT_HEAP_AUTO
#define MICROPY_GC_SPLIT_HEAP_AUTO (0)
#endif

// Hook to run code during time consuming garbage collector operations
// *i* is the loop index variable (e.g. can be used to run every x loops)
#ifndef MICROPY_GC_HOOK_LOOP
#define MICROPY_GC_HOOK_LOOP(i)
#endif

// Whether to provide m_tracked_calloc, m_tracked_free functions
#ifndef MICROPY_TRACKED_ALLOC
#define MICROPY_TRACKED_ALLOC (0)
#endif

// Whether to enable finalisers in the garbage collector (ie call __del__)
#ifndef MICROPY_ENABLE_FINALISER
#define MICROPY_ENABLE_FINALISER (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to enable a separate allocator for the Python stack.
// If enabled then the code must call mp_pystack_init before mp_init.
#ifndef MICROPY_ENABLE_PYSTACK
#define MICROPY_ENABLE_PYSTACK (0)
#endif

// Number of bytes that memory returned by mp_pystack_alloc will be aligned by.
#ifndef MICROPY_PYSTACK_ALIGN
#define MICROPY_PYSTACK_ALIGN (8)
#endif

// Whether to check C stack usage. C stack used for calling Python functions,
// etc. Not checking means segfault on overflow.
#ifndef MICROPY_STACK_CHECK
#define MICROPY_STACK_CHECK (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Additional margin between the places in the runtime where Python stack is
// checked and the actual end of the C stack. Needs to be large enough to avoid
// overflows from function calls made between checks.
#ifndef MICROPY_STACK_CHECK_MARGIN
#define MICROPY_STACK_CHECK_MARGIN (0)
#endif

// The size of a separate stack used for hard IRQ handlers, which should be
// checked instead of the main stack when running a hard callback. 0 implies
// there is no separate ISR stack to check.
#ifndef MICROPY_STACK_SIZE_HARD_IRQ
#define MICROPY_STACK_SIZE_HARD_IRQ (0)
#endif

// Whether to have an emergency exception buffer
#ifndef MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF
#define MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF (0)
#endif
#if MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF
#ifndef MICROPY_EMERGENCY_EXCEPTION_BUF_SIZE
#define MICROPY_EMERGENCY_EXCEPTION_BUF_SIZE (0)      // 0 - implies dynamic allocation
#endif
#endif

// Whether to provide the mp_kbd_exception object, and micropython.kbd_intr function
#ifndef MICROPY_KBD_EXCEPTION
#define MICROPY_KBD_EXCEPTION (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Prefer to raise KeyboardInterrupt asynchronously (from signal or interrupt
// handler) - if supported by a particular port.
#ifndef MICROPY_ASYNC_KBD_INTR
#define MICROPY_ASYNC_KBD_INTR (0)
#endif

// Whether to include REPL helper function
#ifndef MICROPY_HELPER_REPL
#define MICROPY_HELPER_REPL (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Allow enabling debug prints after each REPL line
#ifndef MICROPY_REPL_INFO
#define MICROPY_REPL_INFO (0)
#endif

// Whether to include emacs-style readline behavior in REPL
#ifndef MICROPY_REPL_EMACS_KEYS
#define MICROPY_REPL_EMACS_KEYS (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to include emacs-style word movement/kill readline behavior in REPL.
// This adds Alt+F, Alt+B, Alt+D and Alt+Backspace for forward-word, backward-word, forward-kill-word
// and backward-kill-word, respectively.
#ifndef MICROPY_REPL_EMACS_WORDS_MOVE
#define MICROPY_REPL_EMACS_WORDS_MOVE (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EVERYTHING)
#endif

// Whether to include extra convenience keys for word movement/kill in readline REPL.
// This adds Ctrl+Right, Ctrl+Left and Ctrl+W for forward-word, backward-word and backward-kill-word
// respectively. Ctrl+Delete is not implemented because it's a very different escape sequence.
// Depends on MICROPY_REPL_EMACS_WORDS_MOVE.
#ifndef MICROPY_REPL_EMACS_EXTRA_WORDS_MOVE
#define MICROPY_REPL_EMACS_EXTRA_WORDS_MOVE (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EVERYTHING)
#endif

// Whether to implement auto-indent in REPL
#ifndef MICROPY_REPL_AUTO_INDENT
#define MICROPY_REPL_AUTO_INDENT (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether port requires event-driven REPL functions
#ifndef MICROPY_REPL_EVENT_DRIVEN
#define MICROPY_REPL_EVENT_DRIVEN (0)
#endif

// The number of items to keep in the readline history.
#ifndef MICROPY_READLINE_HISTORY_SIZE
#define MICROPY_READLINE_HISTORY_SIZE (8)
#endif

// Whether to include lexer helper function for unix
#ifndef MICROPY_HELPER_LEXER_UNIX
#define MICROPY_HELPER_LEXER_UNIX (0)
#endif

// Long int implementation
#define MICROPY_LONGINT_IMPL_NONE (0)
#define MICROPY_LONGINT_IMPL_LONGLONG (1)
#define MICROPY_LONGINT_IMPL_MPZ (2)

#ifndef MICROPY_LONGINT_IMPL
#define MICROPY_LONGINT_IMPL (MICROPY_LONGINT_IMPL_NONE)
#endif

#if MICROPY_LONGINT_IMPL == MICROPY_LONGINT_IMPL_LONGLONG
typedef long long mp_longint_impl_t;
#endif

// Whether to include information in the byte code to determine source
// line number (increases RAM usage, but doesn't slow byte code execution)
#ifndef MICROPY_ENABLE_SOURCE_LINE
#define MICROPY_ENABLE_SOURCE_LINE (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to include doc strings (increases RAM usage)
#ifndef MICROPY_ENABLE_DOC_STRING
#define MICROPY_ENABLE_DOC_STRING (0)
#endif

// Exception messages are removed (requires disabling MICROPY_ROM_TEXT_COMPRESSION)
#define MICROPY_ERROR_REPORTING_NONE     (0)
// Exception messages are short static strings
#define MICROPY_ERROR_REPORTING_TERSE    (1)
// Exception messages provide basic error details
#define MICROPY_ERROR_REPORTING_NORMAL   (2)
// Exception messages provide full info, e.g. object names
#define MICROPY_ERROR_REPORTING_DETAILED (3)

#ifndef MICROPY_ERROR_REPORTING
#if MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_FULL_FEATURES
#define MICROPY_ERROR_REPORTING (MICROPY_ERROR_REPORTING_DETAILED)
#elif MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES
#define MICROPY_ERROR_REPORTING (MICROPY_ERROR_REPORTING_NORMAL)
#else
#define MICROPY_ERROR_REPORTING (MICROPY_ERROR_REPORTING_TERSE)
#endif
#endif

// Whether issue warnings during compiling/execution
#ifndef MICROPY_WARNINGS
#define MICROPY_WARNINGS (0)
#endif

// Whether to support warning categories
#ifndef MICROPY_WARNINGS_CATEGORY
#define MICROPY_WARNINGS_CATEGORY (0)
#endif

// This macro is used when printing runtime warnings and errors
#ifndef MICROPY_ERROR_PRINTER
#define MICROPY_ERROR_PRINTER (&mp_plat_print)
#endif

// Float and complex implementation
#define MICROPY_FLOAT_IMPL_NONE (0)
#define MICROPY_FLOAT_IMPL_FLOAT (1)
#define MICROPY_FLOAT_IMPL_DOUBLE (2)

#ifndef MICROPY_FLOAT_IMPL
#define MICROPY_FLOAT_IMPL (MICROPY_FLOAT_IMPL_NONE)
#endif

#if MICROPY_FLOAT_IMPL == MICROPY_FLOAT_IMPL_FLOAT
#define MICROPY_PY_BUILTINS_FLOAT (1)
#define MICROPY_FLOAT_CONST(x) x##F
#define MICROPY_FLOAT_C_FUN(fun) fun##f
typedef float mp_float_t;
#elif MICROPY_FLOAT_IMPL == MICROPY_FLOAT_IMPL_DOUBLE
#define MICROPY_PY_BUILTINS_FLOAT (1)
#define MICROPY_FLOAT_CONST(x) x
#define MICROPY_FLOAT_C_FUN(fun) fun
typedef double mp_float_t;
#else
#define MICROPY_PY_BUILTINS_FLOAT (0)
#endif

#ifndef MICROPY_PY_BUILTINS_COMPLEX
#define MICROPY_PY_BUILTINS_COMPLEX (MICROPY_PY_BUILTINS_FLOAT)
#endif

// Float to string conversion implementations
//
// Note that the EXACT method is only available if the compiler supports
// floating points larger than mp_float_t:
// - with MICROPY_FLOAT_IMPL_FLOAT, the compiler needs to support `double`
// - with MICROPY_FLOAT_IMPL_DOUBLE, the compiler needs to support `long double`
//
#define MICROPY_FLOAT_FORMAT_IMPL_BASIC (0)  // smallest code, but inexact
#define MICROPY_FLOAT_FORMAT_IMPL_APPROX (1) // slightly bigger, almost perfect
#define MICROPY_FLOAT_FORMAT_IMPL_EXACT (2)  // bigger code, and 100% exact repr

#ifndef MICROPY_FLOAT_FORMAT_IMPL
#if MICROPY_FLOAT_IMPL == MICROPY_FLOAT_IMPL_FLOAT
#define MICROPY_FLOAT_FORMAT_IMPL (MICROPY_FLOAT_FORMAT_IMPL_APPROX)
#elif defined(__SIZEOF_LONG_DOUBLE__) && __SIZEOF_LONG_DOUBLE__ > __SIZEOF_DOUBLE__
#define MICROPY_FLOAT_FORMAT_IMPL (MICROPY_FLOAT_FORMAT_IMPL_EXACT)
#else
#define MICROPY_FLOAT_FORMAT_IMPL (MICROPY_FLOAT_FORMAT_IMPL_APPROX)
#endif
#endif

// Whether to use the native _Float16 for 16-bit float support
#ifndef MICROPY_FLOAT_USE_NATIVE_FLT16
#ifdef __FLT16_MAX__
#define MICROPY_FLOAT_USE_NATIVE_FLT16 (1)
#else
#define MICROPY_FLOAT_USE_NATIVE_FLT16 (0)
#endif
#endif

// Whether to provide a high-quality hash for float and complex numbers.
// Otherwise the default is a very simple but correct hashing function.
#ifndef MICROPY_FLOAT_HIGH_QUALITY_HASH
#define MICROPY_FLOAT_HIGH_QUALITY_HASH (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EVERYTHING)
#endif

// Enable features which improve CPython compatibility
// but may lead to more code size/memory usage.
// TODO: Originally intended as generic category to not
// add bunch of once-off options. May need refactoring later
#ifndef MICROPY_CPYTHON_COMPAT
#define MICROPY_CPYTHON_COMPAT (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Perform full checks as done by CPython. Disabling this
// may produce incorrect results, if incorrect data is fed,
// but should not lead to MicroPython crashes or similar
// grave issues (in other words, only user app should be,
// affected, not system).
#ifndef MICROPY_FULL_CHECKS
#define MICROPY_FULL_CHECKS (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Ports can choose to use timestamps based on 2000-01-01 or 1970-01-01
// Default is timestamps based on 2000-01-01
#if !defined(MICROPY_EPOCH_IS_2000) && !defined(MICROPY_EPOCH_IS_1970)
#define MICROPY_EPOCH_IS_2000 (1)
#define MICROPY_EPOCH_IS_1970 (0)
#elif !defined(MICROPY_EPOCH_IS_1970)
#define MICROPY_EPOCH_IS_1970 (1 - (MICROPY_EPOCH_IS_2000))
#elif !defined(MICROPY_EPOCH_IS_2000)
#define MICROPY_EPOCH_IS_2000 (1 - (MICROPY_EPOCH_IS_1970))
#endif

// To maintain reasonable compatibility with CPython on embedded systems,
// and avoid breaking anytime soon, time functions are defined to work
// at least between 1970 and 2099 (included) on any machine.
//
// Specific ports can enable extended date support
// - after 2099 using MICROPY_TIME_SUPPORT_Y2100_AND_BEYOND
// - before 1970 using MICROPY_TIME_SUPPORT_Y1969_AND_BEFORE
// The largest possible range is year 1600 to year 3000
//
// By default, extended date support is only enabled for machines using 64 bit pointers,
// but it can be enabled by specific ports
#ifndef MICROPY_TIME_SUPPORT_Y1969_AND_BEFORE
#if MP_SSIZE_MAX > 2147483647
#define MICROPY_TIME_SUPPORT_Y1969_AND_BEFORE (1)
#else
#define MICROPY_TIME_SUPPORT_Y1969_AND_BEFORE (0)
#endif
#endif

// When support for dates <1970 is enabled, supporting >=2100 does not cost anything
#ifndef MICROPY_TIME_SUPPORT_Y2100_AND_BEYOND
#define MICROPY_TIME_SUPPORT_Y2100_AND_BEYOND (MICROPY_TIME_SUPPORT_Y1969_AND_BEFORE)
#endif

// The type to be used to represent platform-specific timestamps depends on the choices above
#define MICROPY_TIMESTAMP_IMPL_LONG_LONG (0)
#define MICROPY_TIMESTAMP_IMPL_UINT (1)
#define MICROPY_TIMESTAMP_IMPL_TIME_T (2)

#ifndef MICROPY_TIMESTAMP_IMPL
#if MICROPY_TIME_SUPPORT_Y2100_AND_BEYOND || MICROPY_TIME_SUPPORT_Y1969_AND_BEFORE || MICROPY_EPOCH_IS_2000
#define MICROPY_TIMESTAMP_IMPL (MICROPY_TIMESTAMP_IMPL_LONG_LONG)
#else
#define MICROPY_TIMESTAMP_IMPL (MICROPY_TIMESTAMP_IMPL_UINT)
#endif
#endif

// `mp_timestamp_t` is the type that should be used by the port
// to represent timestamps, and is referenced to the platform epoch
#if MICROPY_TIMESTAMP_IMPL == MICROPY_TIMESTAMP_IMPL_LONG_LONG
typedef long long mp_timestamp_t;
#elif MICROPY_TIMESTAMP_IMPL == MICROPY_TIMESTAMP_IMPL_UINT
typedef mp_uint_t mp_timestamp_t;
#elif MICROPY_TIMESTAMP_IMPL == MICROPY_TIMESTAMP_IMPL_TIME_T
typedef time_t mp_timestamp_t;
#endif

// Whether POSIX-semantics non-blocking streams are supported
#ifndef MICROPY_STREAMS_NON_BLOCK
#define MICROPY_STREAMS_NON_BLOCK (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to provide stream functions with POSIX-like signatures
// (useful for porting existing libraries to MicroPython).
#ifndef MICROPY_STREAMS_POSIX_API
#define MICROPY_STREAMS_POSIX_API (0)
#endif

// Whether to delegate error raising to stream implementations using the
// MP_STREAM_RAISE_ERROR ioctl to support raising more detailed messages.
#ifndef MICROPY_STREAMS_DELEGATE_ERROR
#define MICROPY_STREAMS_DELEGATE_ERROR (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to process __all__ when importing all public symbols from a module.
#ifndef MICROPY_MODULE___ALL__
#define MICROPY_MODULE___ALL__ (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_BASIC_FEATURES)
#endif

// Whether to set __file__ on imported modules.
#ifndef MICROPY_MODULE___FILE__
#define MICROPY_MODULE___FILE__ (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether modules can use MP_REGISTER_MODULE_DELEGATION() to delegate failed
// attribute lookups to a custom handler function.
#ifndef MICROPY_MODULE_ATTR_DELEGATION
#define MICROPY_MODULE_ATTR_DELEGATION (MICROPY_PY_SYS_ATTR_DELEGATION || MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to call __init__ when importing builtin modules for the first time.
// Modules using this need to handle the possibility that __init__ might be
// called multiple times.
#ifndef MICROPY_MODULE_BUILTIN_INIT
#define MICROPY_MODULE_BUILTIN_INIT (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to allow built-in modules to have sub-packages (by making the
// sub-package a member of their locals dict). Sub-packages should not be
// registered with MP_REGISTER_MODULE, instead they should be added as
// members of the parent's globals dict. To match CPython behavior,
// their __name__ should be "foo.bar"(i.e. QSTR_foo_dot_bar) which will
// require an entry in qstrdefs, although it does also work to just call
// it "bar". Also, because subpackages can be accessed without being
// imported (e.g. as foo.bar after `import foo`), they should not
// have __init__ methods. Instead, the top-level package's __init__ should
// initialise all sub-packages.
#ifndef MICROPY_MODULE_BUILTIN_SUBPACKAGES
#define MICROPY_MODULE_BUILTIN_SUBPACKAGES (MICROPY_PY_TSTRINGS || MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EVERYTHING)
#endif

// Whether to support module-level __getattr__ (see PEP 562)
#ifndef MICROPY_MODULE_GETATTR
#define MICROPY_MODULE_GETATTR (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to enable importing foo.py with __name__ set to '__main__'
// Used by the unix port for the -m flag.
#ifndef MICROPY_MODULE_OVERRIDE_MAIN_IMPORT
#define MICROPY_MODULE_OVERRIDE_MAIN_IMPORT (0)
#endif

// Whether frozen modules are supported in the form of strings
#ifndef MICROPY_MODULE_FROZEN_STR
#define MICROPY_MODULE_FROZEN_STR (0)
#endif

// Whether frozen modules are supported in the form of .mpy files
#ifndef MICROPY_MODULE_FROZEN_MPY
#define MICROPY_MODULE_FROZEN_MPY (0)
#endif

// Convenience macro for whether frozen modules are supported
#ifndef MICROPY_MODULE_FROZEN
#define MICROPY_MODULE_FROZEN (MICROPY_MODULE_FROZEN_STR || MICROPY_MODULE_FROZEN_MPY)
#endif

// Whether you can override builtins in the builtins module
#ifndef MICROPY_CAN_OVERRIDE_BUILTINS
#define MICROPY_CAN_OVERRIDE_BUILTINS (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to check that the "self" argument of a builtin method has the
// correct type.  Such an explicit check is only needed if a builtin
// method escapes to Python land without a first argument, eg
// list.append([], 1).  Without this check such calls will have undefined
// behaviour (usually segfault) if the first argument is the wrong type.
#ifndef MICROPY_BUILTIN_METHOD_CHECK_SELF_ARG
#define MICROPY_BUILTIN_METHOD_CHECK_SELF_ARG (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to use internally defined errno's (otherwise system provided ones)
#ifndef MICROPY_USE_INTERNAL_ERRNO
#define MICROPY_USE_INTERNAL_ERRNO (0)
#endif

// Whether to use internally defined *printf() functions (otherwise external ones)
#ifndef MICROPY_USE_INTERNAL_PRINTF
#define MICROPY_USE_INTERNAL_PRINTF (1)
#endif

// The mp_print_t printer used for printf output when MICROPY_USE_INTERNAL_PRINTF is enabled
#ifndef MICROPY_INTERNAL_PRINTF_PRINTER
#define MICROPY_INTERNAL_PRINTF_PRINTER (&mp_plat_print)
#endif

// Whether to support mp_sched_vm_abort to asynchronously abort to the top level.
#ifndef MICROPY_ENABLE_VM_ABORT
#define MICROPY_ENABLE_VM_ABORT (0)
#endif

// Whether to handle abort behavior in pyexec code
#ifndef MICROPY_PYEXEC_ENABLE_VM_ABORT
#define MICROPY_PYEXEC_ENABLE_VM_ABORT (0)
#endif

// Whether to set exit codes according to the exit reason (keyboard interrupt, crash, normal exit, ...)
#ifndef MICROPY_PYEXEC_ENABLE_EXIT_CODE_HANDLING
#define MICROPY_PYEXEC_ENABLE_EXIT_CODE_HANDLING (0)
#endif

// Support for internal scheduler
#ifndef MICROPY_ENABLE_SCHEDULER
#define MICROPY_ENABLE_SCHEDULER (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether the scheduler supports scheduling static nodes with C callbacks
#ifndef MICROPY_SCHEDULER_STATIC_NODES
#define MICROPY_SCHEDULER_STATIC_NODES (0)
#endif

// Maximum number of entries in the scheduler
#ifndef MICROPY_SCHEDULER_DEPTH
#define MICROPY_SCHEDULER_DEPTH (4)
#endif

// Support for generic VFS sub-system
#ifndef MICROPY_VFS
#define MICROPY_VFS (0)
#endif

// Whether to include support for fast native block devices.
#ifndef MICROPY_VFS_BLOCKDEV_NATIVE
#define MICROPY_VFS_BLOCKDEV_NATIVE (0)
#endif

// Whether to include support for writable filesystems.
#ifndef MICROPY_VFS_WRITABLE
#define MICROPY_VFS_WRITABLE (1)
#endif

// Whether to enable the mp_vfs_rom_ioctl C function, and vfs.rom_ioctl Python function
#ifndef MICROPY_VFS_ROM_IOCTL
#define MICROPY_VFS_ROM_IOCTL (MICROPY_VFS_ROM)
#endif

// Support for VFS POSIX component, to mount a POSIX filesystem within VFS
#ifndef MICROPY_VFS_POSIX
#define MICROPY_VFS_POSIX (0)
#endif

// Whether to include support for writable POSIX filesystems.
#ifndef MICROPY_VFS_POSIX_WRITABLE
#define MICROPY_VFS_POSIX_WRITABLE (1)
#endif

// Support for VFS FAT component, to mount a FAT filesystem within VFS
#ifndef MICROPY_VFS_FAT
#define MICROPY_VFS_FAT (0)
#endif

// Support for VFS LittleFS v1 component, to mount a LFSv1 filesystem within VFS
#ifndef MICROPY_VFS_LFS1
#define MICROPY_VFS_LFS1 (0)
#endif

// Support for VFS LittleFS v2 component, to mount a LFSv2 filesystem within VFS
#ifndef MICROPY_VFS_LFS2
#define MICROPY_VFS_LFS2 (0)
#endif

// Support for ROMFS.
#ifndef MICROPY_VFS_ROM
#define MICROPY_VFS_ROM (0)
#endif

/*****************************************************************************/
/* Fine control over Python builtins, classes, modules, etc                  */

// Whether to support multiple inheritance of Python classes.  Multiple
// inheritance makes some C functions inherently recursive, and adds a bit of
// code overhead.
#ifndef MICROPY_MULTIPLE_INHERITANCE
#define MICROPY_MULTIPLE_INHERITANCE (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to implement attributes on functions
#ifndef MICROPY_PY_FUNCTION_ATTRS
#define MICROPY_PY_FUNCTION_ATTRS (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to implement the __code__ attribute on functions, and function constructor
#ifndef MICROPY_PY_FUNCTION_ATTRS_CODE
#define MICROPY_PY_FUNCTION_ATTRS_CODE (MICROPY_PY_MARSHAL || MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_FULL_FEATURES)
#endif

// Whether bound_method can just use == (feature disabled), or requires a call to
// mp_obj_equal (feature enabled), to test equality of the self and meth entities.
// This is only needed if objects and functions can be identical without being the
// same thing, eg when using an object proxy.
#ifndef MICROPY_PY_BOUND_METHOD_FULL_EQUALITY_CHECK
#define MICROPY_PY_BOUND_METHOD_FULL_EQUALITY_CHECK (0)
#endif

// Whether to support the descriptors __get__, __set__, __delete__, __set_name__
// This costs some code size and makes load/store/delete of instance
// attributes slower for the classes that use this feature
#ifndef MICROPY_PY_DESCRIPTORS
#define MICROPY_PY_DESCRIPTORS (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to support class __delattr__ and __setattr__ methods
// This costs some code size and makes store/delete of instance
// attributes slower for the classes that use this feature
#ifndef MICROPY_PY_DELATTR_SETATTR
#define MICROPY_PY_DELATTR_SETATTR (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Support for async/await/async for/async with
#ifndef MICROPY_PY_ASYNC_AWAIT
#define MICROPY_PY_ASYNC_AWAIT (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Support for literal string interpolation, f-strings (see PEP 498, Python 3.6+)
#ifndef MICROPY_PY_FSTRINGS
#define MICROPY_PY_FSTRINGS (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Support for template strings, t-strings (see PEP 750, Python 3.14+)
// Requires MICROPY_PY_FSTRINGS to be enabled.
#ifndef MICROPY_PY_TSTRINGS
#define MICROPY_PY_TSTRINGS (MICROPY_PY_FSTRINGS && MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_FULL_FEATURES)
#endif

// Support for assignment expressions with := (see PEP 572, Python 3.8+)
#ifndef MICROPY_PY_ASSIGN_EXPR
#define MICROPY_PY_ASSIGN_EXPR (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Non-standard .pend_throw() method for generators, allowing for
// Future-like behavior with respect to exception handling: an
// exception set with .pend_throw() will activate on the next call
// to generator's .send() or .__next__(). (This is useful to implement
// async schedulers.)
#ifndef MICROPY_PY_GENERATOR_PEND_THROW
#define MICROPY_PY_GENERATOR_PEND_THROW (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Issue a warning when comparing str and bytes objects
#ifndef MICROPY_PY_STR_BYTES_CMP_WARN
#define MICROPY_PY_STR_BYTES_CMP_WARN (0)
#endif

// Add bytes.hex and bytes.fromhex
#ifndef MICROPY_PY_BUILTINS_BYTES_HEX
#define MICROPY_PY_BUILTINS_BYTES_HEX (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether str object is proper unicode
#ifndef MICROPY_PY_BUILTINS_STR_UNICODE
#define MICROPY_PY_BUILTINS_STR_UNICODE (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_BASIC_FEATURES)
#endif

// Whether to check for valid UTF-8 when converting bytes to str
#ifndef MICROPY_PY_BUILTINS_STR_UNICODE_CHECK
#define MICROPY_PY_BUILTINS_STR_UNICODE_CHECK (MICROPY_PY_BUILTINS_STR_UNICODE)
#endif

// Whether bytes.decode() supports the 'ignore' and 'replace' error handlers
#ifndef MICROPY_PY_BUILTINS_BYTES_DECODE_ERRORS
#define MICROPY_PY_BUILTINS_BYTES_DECODE_ERRORS (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether str.center() method provided
#ifndef MICROPY_PY_BUILTINS_STR_CENTER
#define MICROPY_PY_BUILTINS_STR_CENTER (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether str.count() method provided
#ifndef MICROPY_PY_BUILTINS_STR_COUNT
#define MICROPY_PY_BUILTINS_STR_COUNT (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether str % (...) formatting operator provided
#ifndef MICROPY_PY_BUILTINS_STR_OP_MODULO
#define MICROPY_PY_BUILTINS_STR_OP_MODULO (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether str.partition()/str.rpartition() method provided
#ifndef MICROPY_PY_BUILTINS_STR_PARTITION
#define MICROPY_PY_BUILTINS_STR_PARTITION (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether str.splitlines() method provided
#ifndef MICROPY_PY_BUILTINS_STR_SPLITLINES
#define MICROPY_PY_BUILTINS_STR_SPLITLINES (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to support bytearray object
#ifndef MICROPY_PY_BUILTINS_BYTEARRAY
#define MICROPY_PY_BUILTINS_BYTEARRAY (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to support code objects, and how many features they have
#define MICROPY_PY_BUILTINS_CODE_NONE       (0)
#define MICROPY_PY_BUILTINS_CODE_MINIMUM    (1)
#define MICROPY_PY_BUILTINS_CODE_BASIC      (2)
#define MICROPY_PY_BUILTINS_CODE_FULL       (3)
#ifndef MICROPY_PY_BUILTINS_CODE
#define MICROPY_PY_BUILTINS_CODE            (MICROPY_PY_SYS_SETTRACE ? MICROPY_PY_BUILTINS_CODE_FULL : (MICROPY_PY_FUNCTION_ATTRS_CODE ? MICROPY_PY_BUILTINS_CODE_BASIC : (MICROPY_PY_BUILTINS_COMPILE ? MICROPY_PY_BUILTINS_CODE_MINIMUM : MICROPY_PY_BUILTINS_CODE_NONE)))
#endif

// Whether to support dict.fromkeys() class method
#ifndef MICROPY_PY_BUILTINS_DICT_FROMKEYS
#define MICROPY_PY_BUILTINS_DICT_FROMKEYS (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to support memoryview object
#ifndef MICROPY_PY_BUILTINS_MEMORYVIEW
#define MICROPY_PY_BUILTINS_MEMORYVIEW (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to support memoryview.itemsize attribute
#ifndef MICROPY_PY_BUILTINS_MEMORYVIEW_ITEMSIZE
#define MICROPY_PY_BUILTINS_MEMORYVIEW_ITEMSIZE (MICROPY_PY_MACHINE_MEM_BACKUP || MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_BASIC_FEATURES)
#endif

// Whether to support set object
#ifndef MICROPY_PY_BUILTINS_SET
#define MICROPY_PY_BUILTINS_SET (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to support slice subscript operators and slice object
#ifndef MICROPY_PY_BUILTINS_SLICE
#define MICROPY_PY_BUILTINS_SLICE (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to support slice attribute read access,
// i.e. slice.start, slice.stop, slice.step
#ifndef MICROPY_PY_BUILTINS_SLICE_ATTRS
#define MICROPY_PY_BUILTINS_SLICE_ATTRS (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to support the .indices(len) method on slice objects
#ifndef MICROPY_PY_BUILTINS_SLICE_INDICES
#define MICROPY_PY_BUILTINS_SLICE_INDICES (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to support frozenset object
#ifndef MICROPY_PY_BUILTINS_FROZENSET
#define MICROPY_PY_BUILTINS_FROZENSET (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to support property object
#ifndef MICROPY_PY_BUILTINS_PROPERTY
#define MICROPY_PY_BUILTINS_PROPERTY (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to implement the start/stop/step attributes (readback) on
// the "range" builtin type. Rarely used, and costs ~60 bytes (x86).
#ifndef MICROPY_PY_BUILTINS_RANGE_ATTRS
#define MICROPY_PY_BUILTINS_RANGE_ATTRS (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to support binary ops [only (in)equality is defined] between range
// objects.  With this option disabled all range objects that are not exactly
// the same object will compare as not-equal.  With it enabled the semantics
// match CPython and ranges are equal if they yield the same sequence of items.
#ifndef MICROPY_PY_BUILTINS_RANGE_BINOP
#define MICROPY_PY_BUILTINS_RANGE_BINOP (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EVERYTHING)
#endif

// Support for calling next() with second argument
#ifndef MICROPY_PY_BUILTINS_NEXT2
#define MICROPY_PY_BUILTINS_NEXT2 (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_BASIC_FEATURES)
#endif

// Whether to support rounding of integers (incl bignum); eg round(123,-1)=120
#ifndef MICROPY_PY_BUILTINS_ROUND_INT
#define MICROPY_PY_BUILTINS_ROUND_INT (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to implement dir() to enumerate object fields.
#ifndef MICROPY_PY_BUILTINS_DIR
#define MICROPY_PY_BUILTINS_DIR (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to support complete set of special methods for user
// classes, or only the most used ones. "Inplace" methods are
// controlled by MICROPY_PY_ALL_INPLACE_SPECIAL_METHODS below.
// "Reverse" methods are controlled by
// MICROPY_PY_REVERSE_SPECIAL_METHODS below.
#ifndef MICROPY_PY_ALL_SPECIAL_METHODS
#define MICROPY_PY_ALL_SPECIAL_METHODS (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to support all inplace arithmetic operation methods
// (__imul__, etc.)
#ifndef MICROPY_PY_ALL_INPLACE_SPECIAL_METHODS
#define MICROPY_PY_ALL_INPLACE_SPECIAL_METHODS (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EVERYTHING)
#endif

// Whether to support reverse arithmetic operation methods
// (__radd__, etc.). Additionally gated by
// MICROPY_PY_ALL_SPECIAL_METHODS.
#ifndef MICROPY_PY_REVERSE_SPECIAL_METHODS
#define MICROPY_PY_REVERSE_SPECIAL_METHODS (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to support compile function
#ifndef MICROPY_PY_BUILTINS_COMPILE
#define MICROPY_PY_BUILTINS_COMPILE (MICROPY_ENABLE_COMPILER && MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to support enumerate function(type)
#ifndef MICROPY_PY_BUILTINS_ENUMERATE
#define MICROPY_PY_BUILTINS_ENUMERATE (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to support eval and exec functions
// By default they are supported if the compiler is enabled
#ifndef MICROPY_PY_BUILTINS_EVAL_EXEC
#define MICROPY_PY_BUILTINS_EVAL_EXEC (MICROPY_ENABLE_COMPILER)
#endif

// Whether to support the Python 2 execfile function
#ifndef MICROPY_PY_BUILTINS_EXECFILE
#define MICROPY_PY_BUILTINS_EXECFILE (MICROPY_ENABLE_COMPILER && MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to support filter function(type)
#ifndef MICROPY_PY_BUILTINS_FILTER
#define MICROPY_PY_BUILTINS_FILTER (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to support reversed function(type)
#ifndef MICROPY_PY_BUILTINS_REVERSED
#define MICROPY_PY_BUILTINS_REVERSED (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to define "NotImplemented" special constant
#ifndef MICROPY_PY_BUILTINS_NOTIMPLEMENTED
#define MICROPY_PY_BUILTINS_NOTIMPLEMENTED (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to provide the built-in input() function. The implementation of this
// uses shared/readline, so can only be enabled if the port uses this readline.
#ifndef MICROPY_PY_BUILTINS_INPUT
#define MICROPY_PY_BUILTINS_INPUT (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to support min/max functions
#ifndef MICROPY_PY_BUILTINS_MIN_MAX
#define MICROPY_PY_BUILTINS_MIN_MAX (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Support for calls to pow() with 3 integer arguments
#ifndef MICROPY_PY_BUILTINS_POW3
#define MICROPY_PY_BUILTINS_POW3 (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to provide the help function
#ifndef MICROPY_PY_BUILTINS_HELP
#define MICROPY_PY_BUILTINS_HELP (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Use this to configure the help text shown for help().  It should be a
// variable with the type "const char*".  A sensible default is provided.
#ifndef MICROPY_PY_BUILTINS_HELP_TEXT
#define MICROPY_PY_BUILTINS_HELP_TEXT mp_help_default_text
#endif

// Add the ability to list the available modules when executing help('modules')
#ifndef MICROPY_PY_BUILTINS_HELP_MODULES
#define MICROPY_PY_BUILTINS_HELP_MODULES (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Use this to configure output of help('modules')
#ifndef MICROPY_PY_BUILTINS_HELP_NUM_COLUMNS
#define MICROPY_PY_BUILTINS_HELP_NUM_COLUMNS (4)
#endif

// Use this to configure output of help('modules')
#ifndef MICROPY_PY_BUILTINS_HELP_COLUMN_WIDTH
#define MICROPY_PY_BUILTINS_HELP_COLUMN_WIDTH (18)
#endif

// Whether to provide mem-info related functions in micropython module
#ifndef MICROPY_PY_MICROPYTHON_MEM_INFO
#define MICROPY_PY_MICROPYTHON_MEM_INFO (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to provide "micropython.stack_use" function
#ifndef MICROPY_PY_MICROPYTHON_STACK_USE
#define MICROPY_PY_MICROPYTHON_STACK_USE (MICROPY_PY_MICROPYTHON_MEM_INFO)
#endif

// Whether to provide the "micropython.heap_locked" function
#ifndef MICROPY_PY_MICROPYTHON_HEAP_LOCKED
#define MICROPY_PY_MICROPYTHON_HEAP_LOCKED (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EVERYTHING)
#endif

// Support for micropython.RingIO()
#ifndef MICROPY_PY_MICROPYTHON_RINGIO
#define MICROPY_PY_MICROPYTHON_RINGIO (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to provide "array" module. Note that large chunk of the
// underlying code is shared with "bytearray" builtin type, so to
// get real savings, it should be disabled too.
#ifndef MICROPY_PY_ARRAY
#define MICROPY_PY_ARRAY (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to support slice assignments for array (and bytearray).
// This is rarely used, but adds ~0.5K of code.
#ifndef MICROPY_PY_ARRAY_SLICE_ASSIGN
#define MICROPY_PY_ARRAY_SLICE_ASSIGN (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to support attrtuple type (MicroPython extension)
// It provides space-efficient tuples with attribute access
#ifndef MICROPY_PY_ATTRTUPLE
#define MICROPY_PY_ATTRTUPLE (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to provide "collections" module
#ifndef MICROPY_PY_COLLECTIONS
#define MICROPY_PY_COLLECTIONS (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to provide "collections.deque" type
#ifndef MICROPY_PY_COLLECTIONS_DEQUE
#define MICROPY_PY_COLLECTIONS_DEQUE (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether "collections.deque" supports iteration
#ifndef MICROPY_PY_COLLECTIONS_DEQUE_ITER
#define MICROPY_PY_COLLECTIONS_DEQUE_ITER (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether "collections.deque" supports subscription
#ifndef MICROPY_PY_COLLECTIONS_DEQUE_SUBSCR
#define MICROPY_PY_COLLECTIONS_DEQUE_SUBSCR (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to provide "collections.OrderedDict" type
#ifndef MICROPY_PY_COLLECTIONS_ORDEREDDICT
#define MICROPY_PY_COLLECTIONS_ORDEREDDICT (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to provide the _asdict function for namedtuple
#ifndef MICROPY_PY_COLLECTIONS_NAMEDTUPLE__ASDICT
#define MICROPY_PY_COLLECTIONS_NAMEDTUPLE__ASDICT (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EVERYTHING)
#endif

// Whether to provide "marshal" module
#ifndef MICROPY_PY_MARSHAL
#define MICROPY_PY_MARSHAL (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EVERYTHING)
#endif

// Whether to provide "math" module
#ifndef MICROPY_PY_MATH
#define MICROPY_PY_MATH (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to provide all math module constants (Python 3.5+), or just pi and e.
#ifndef MICROPY_PY_MATH_CONSTANTS
#define MICROPY_PY_MATH_CONSTANTS (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to provide special math functions: math.{erf,erfc,gamma,lgamma}
#ifndef MICROPY_PY_MATH_SPECIAL_FUNCTIONS
#define MICROPY_PY_MATH_SPECIAL_FUNCTIONS (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to provide math.factorial function
#ifndef MICROPY_PY_MATH_FACTORIAL
#define MICROPY_PY_MATH_FACTORIAL (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to provide math.isclose function
#ifndef MICROPY_PY_MATH_ISCLOSE
#define MICROPY_PY_MATH_ISCLOSE (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to provide fix for atan2 Inf handling.
#ifndef MICROPY_PY_MATH_ATAN2_FIX_INFNAN
#define MICROPY_PY_MATH_ATAN2_FIX_INFNAN (0)
#endif

// Whether to provide fix for fmod Inf handling.
#ifndef MICROPY_PY_MATH_FMOD_FIX_INFNAN
#define MICROPY_PY_MATH_FMOD_FIX_INFNAN (0)
#endif

// Whether to provide fix for modf negative zero handling.
#ifndef MICROPY_PY_MATH_MODF_FIX_NEGZERO
#define MICROPY_PY_MATH_MODF_FIX_NEGZERO (0)
#endif

// Whether to provide fix for pow(1, NaN) and pow(NaN, 0), which both should be 1 not NaN.
// Also fixes pow(base, NaN) to return NaN for other values of base.
#ifndef MICROPY_PY_MATH_POW_FIX_NAN
#define MICROPY_PY_MATH_POW_FIX_NAN (0)
#endif

// Whether to provide fix for gamma(-inf) to raise ValueError
#ifndef MICROPY_PY_MATH_GAMMA_FIX_NEGINF
#define MICROPY_PY_MATH_GAMMA_FIX_NEGINF (0)
#endif

// Whether to provide "cmath" module
#ifndef MICROPY_PY_CMATH
#define MICROPY_PY_CMATH (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to provide "micropython" module
#ifndef MICROPY_PY_MICROPYTHON
#define MICROPY_PY_MICROPYTHON (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to provide "gc" module
#ifndef MICROPY_PY_GC
#define MICROPY_PY_GC (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to return number of collected objects from gc.collect()
#ifndef MICROPY_PY_GC_COLLECT_RETVAL
#define MICROPY_PY_GC_COLLECT_RETVAL (0)
#endif

// Whether to provide "io" module
#ifndef MICROPY_PY_IO
#define MICROPY_PY_IO (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to provide "io.IOBase" class to support user streams
#ifndef MICROPY_PY_IO_IOBASE
#define MICROPY_PY_IO_IOBASE (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether to provide "io.BytesIO" class
#ifndef MICROPY_PY_IO_BYTESIO
#define MICROPY_PY_IO_BYTESIO (1)
#endif

// Whether to provide "io.BufferedWriter" class
#ifndef MICROPY_PY_IO_BUFFEREDWRITER
#define MICROPY_PY_IO_BUFFEREDWRITER (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EVERYTHING)
#endif

// Whether to provide "struct" module
#ifndef MICROPY_PY_STRUCT
#define MICROPY_PY_STRUCT (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES)
#endif

// Whether struct module provides unsafe and non-standard typecodes O, P, S.
// These typecodes are not in CPython and can cause crashes by accessing arbitrary
// memory.
#ifndef MICROPY_PY_STRUCT_UNSAFE_TYPECODES
#define MICROPY_PY_STRUCT_UNSAFE_TYPECODES (1)
#endif

// Whether to provide "sys" module
#ifndef MICROPY_PY_SYS
#define MICROPY_PY_SYS (1)
#endif

// Whether to initialise "sys.path" and "sys.argv" to their defaults in mp_init()
#ifndef MICROPY_PY_SYS_PATH_ARGV_DEFAULTS
#define MICROPY_PY_SYS_PATH_ARGV_DEFAULTS (MICROPY_PY_SYS)
#endif

// Whether to provide "sys.maxsize" constant
#ifndef MICROPY_PY_SYS_MAXSIZE
#define MICROPY_PY_SYS_MAXSIZE (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to provide "sys.modules" dictionary
#ifndef MICROPY_PY_SYS_MODULES
#define MICROPY_PY_SYS_MODULES (1)
#endif

// Whether to provide "sys.exc_info" function
// Avoid enabling this, this function is Python2 heritage
#ifndef MICROPY_PY_SYS_EXC_INFO
#define MICROPY_PY_SYS_EXC_INFO (0)
#endif

// Whether to provide "sys.executable", which is the absolute path to the
// micropython binary
// Intended for use on the "OS" ports (e.g. Unix)
#ifndef MICROPY_PY_SYS_EXECUTABLE
#define MICROPY_PY_SYS_EXECUTABLE (0)
#endif

// Whether to provide "sys.intern"
#ifndef MICROPY_PY_SYS_INTERN
#define MICROPY_PY_SYS_INTERN (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EVERYTHING)
#endif

// Whether to provide "sys.exit" function
#ifndef MICROPY_PY_SYS_EXIT
#define MICROPY_PY_SYS_EXIT (1)
#endif

// Whether to provide "sys.atexit" function (MicroPython extension)
#ifndef MICROPY_PY_SYS_ATEXIT
#define MICROPY_PY_SYS_ATEXIT (0)
#endif

// Whether to provide the "sys.path" attribute (which forces module delegation
// and mutable sys attributes to be enabled).
// If MICROPY_PY_SYS_PATH_ARGV_DEFAULTS is enabled, this is initialised in
// mp_init to an empty list. Otherwise the port must initialise it using
// `mp_sys_path = mp_obj_new_list(...)`.
#ifndef MICROPY_PY_SYS_PATH
#define MICROPY_PY_SYS_PATH (1)
#endif

// Whether to provide the "sys.argv" attribute.
// If MICROPY_PY_SYS_PATH_ARGV_DEFAULTS is enabled, this is initialised in
// mp_init to an empty list. Otherwise the port must initialise it using
// `mp_obj_list_init(MP_OBJ_TO_PTR(mp_sys_argv), ...);`
#ifndef MICROPY_PY_SYS_ARGV
#define MICROPY_PY_SYS_ARGV (1)
#endif

// Whether to provide sys.{ps1,ps2} mutable attributes, to control REPL prompts
#ifndef MICROPY_PY_SYS_PS1_PS2
#define MICROPY_PY_SYS_PS1_PS2 (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to provide "sys.settrace" function
#ifndef MICROPY_PY_SYS_SETTRACE
#define MICROPY_PY_SYS_SETTRACE (0)
#endif

// Whether to provide "sys.getsizeof" function
#ifndef MICROPY_PY_SYS_GETSIZEOF
#define MICROPY_PY_SYS_GETSIZEOF (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EVERYTHING)
#endif

// Whether to provide sys.{stdin,stdout,stderr} objects
#ifndef MICROPY_PY_SYS_STDFILES
#define MICROPY_PY_SYS_STDFILES (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to provide sys.{stdin,stdout,stderr}.buffer object
// This is implemented per-port
#ifndef MICROPY_PY_SYS_STDIO_BUFFER
#define MICROPY_PY_SYS_STDIO_BUFFER (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to provide sys.tracebacklimit mutable attribute
#ifndef MICROPY_PY_SYS_TRACEBACKLIMIT
#define MICROPY_PY_SYS_TRACEBACKLIMIT (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EVERYTHING)
#endif

// Whether the sys module supports attribute delegation
// This is enabled automatically when needed by other features
#ifndef MICROPY_PY_SYS_ATTR_DELEGATION
#define MICROPY_PY_SYS_ATTR_DELEGATION (MICROPY_PY_SYS_PATH || MICROPY_PY_SYS_PS1_PS2 || MICROPY_PY_SYS_TRACEBACKLIMIT)
#endif

// Whether to provide "errno" module
#ifndef MICROPY_PY_ERRNO
#define MICROPY_PY_ERRNO (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to provide the errno.errorcode dict
#ifndef MICROPY_PY_ERRNO_ERRORCODE
#define MICROPY_PY_ERRNO_ERRORCODE (1)
#endif

// Whether to provide "select" module
#ifndef MICROPY_PY_SELECT
#define MICROPY_PY_SELECT (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to enable POSIX optimisations in the "select" module (requires system poll)
#ifndef MICROPY_PY_SELECT_POSIX_OPTIMISATIONS
#define MICROPY_PY_SELECT_POSIX_OPTIMISATIONS (0)
#endif

// Whether to enable the select() function in the "select" module (baremetal
// implementation). This is present for compatibility but can be disabled to
// save space.
#ifndef MICROPY_PY_SELECT_SELECT
#define MICROPY_PY_SELECT_SELECT (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to provide the "time" module
#ifndef MICROPY_PY_TIME
#define MICROPY_PY_TIME (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_BASIC_FEATURES)
#endif

// Whether to provide time.gmtime/localtime/mktime functions
#ifndef MICROPY_PY_TIME_GMTIME_LOCALTIME_MKTIME
#define MICROPY_PY_TIME_GMTIME_LOCALTIME_MKTIME (0)
#endif

// Whether to provide time.time/time_ns functions
#ifndef MICROPY_PY_TIME_TIME_TIME_NS
#define MICROPY_PY_TIME_TIME_TIME_NS (0)
#endif

// Period of values returned by time.ticks_ms(), ticks_us(), ticks_cpu()
// functions. Should be power of two. All functions above use the same
// period, so if underlying hardware/API has different periods, the
// minimum of them should be used. The value below is the maximum value
// this parameter can take (corresponding to 30 bit tick values on 32-bit
// system).
#ifndef MICROPY_PY_TIME_TICKS_PERIOD
#define MICROPY_PY_TIME_TICKS_PERIOD (MP_SMALL_INT_POSITIVE_MASK + 1)
#endif

// Whether to provide "_thread" module
#ifndef MICROPY_PY_THREAD
#define MICROPY_PY_THREAD (0)
#endif

// Whether to make the VM/runtime thread-safe using a global lock
// If not enabled then thread safety must be provided at the Python level
#ifndef MICROPY_PY_THREAD_GIL
#define MICROPY_PY_THREAD_GIL (MICROPY_PY_THREAD)
#endif

// Number of VM jump-loops to do before releasing the GIL.
// Set this to 0 to disable the divisor.
#ifndef MICROPY_PY_THREAD_GIL_VM_DIVISOR
#define MICROPY_PY_THREAD_GIL_VM_DIVISOR (32)
#endif

// Is a recursive mutex type in use?
#ifndef MICROPY_PY_THREAD_RECURSIVE_MUTEX
#define MICROPY_PY_THREAD_RECURSIVE_MUTEX (MICROPY_PY_THREAD && !MICROPY_PY_THREAD_GIL)
#endif

// Whether to provide the "weakref" module.
#ifndef MICROPY_PY_WEAKREF
#define MICROPY_PY_WEAKREF (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EVERYTHING)
#endif

// Extended modules

#ifndef MICROPY_PY_ASYNCIO
#define MICROPY_PY_ASYNCIO (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

#ifndef MICROPY_PY_ASYNCIO_TASK_QUEUE_PUSH_CALLBACK
#define MICROPY_PY_ASYNCIO_TASK_QUEUE_PUSH_CALLBACK (0)
#endif

#ifndef MICROPY_PY_UCTYPES
#define MICROPY_PY_UCTYPES (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to provide SHORT, INT, LONG, etc. types in addition to
// exact-bitness types like INT16, INT32, etc.
#ifndef MICROPY_PY_UCTYPES_NATIVE_C_TYPES
#define MICROPY_PY_UCTYPES_NATIVE_C_TYPES (1)
#endif

// Whether to provide "deflate" module (decompression-only by default)
#ifndef MICROPY_PY_DEFLATE
#define MICROPY_PY_DEFLATE (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to provide compression support in "deflate" module
#ifndef MICROPY_PY_DEFLATE_COMPRESS
#define MICROPY_PY_DEFLATE_COMPRESS (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_FULL_FEATURES)
#endif

#ifndef MICROPY_PY_JSON
#define MICROPY_PY_JSON (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to support the "separators" argument to dump, dumps
#ifndef MICROPY_PY_JSON_SEPARATORS
#define MICROPY_PY_JSON_SEPARATORS (1)
#endif

#ifndef MICROPY_PY_OS
#define MICROPY_PY_OS (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

#ifndef MICROPY_PY_OS_STATVFS
#define MICROPY_PY_OS_STATVFS (MICROPY_PY_OS)
#endif

#ifndef MICROPY_PY_RE
#define MICROPY_PY_RE (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

#ifndef MICROPY_PY_RE_DEBUG
#define MICROPY_PY_RE_DEBUG (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EVERYTHING)
#endif

#ifndef MICROPY_PY_RE_MATCH_GROUPS
#define MICROPY_PY_RE_MATCH_GROUPS (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EVERYTHING)
#endif

#ifndef MICROPY_PY_RE_MATCH_SPAN_START_END
#define MICROPY_PY_RE_MATCH_SPAN_START_END (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EVERYTHING)
#endif

#ifndef MICROPY_PY_RE_SUB
#define MICROPY_PY_RE_SUB (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

#ifndef MICROPY_PY_HEAPQ
#define MICROPY_PY_HEAPQ (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

#ifndef MICROPY_PY_HASHLIB
#define MICROPY_PY_HASHLIB (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

#ifndef MICROPY_PY_HASHLIB_MD5
#define MICROPY_PY_HASHLIB_MD5 (MICROPY_PY_SSL)
#endif

#ifndef MICROPY_PY_HASHLIB_SHA1
#define MICROPY_PY_HASHLIB_SHA1  (MICROPY_PY_SSL)
#endif

#ifndef MICROPY_PY_HASHLIB_SHA256
#define MICROPY_PY_HASHLIB_SHA256 (1)
#endif

#ifndef MICROPY_PY_CRYPTOLIB
#define MICROPY_PY_CRYPTOLIB (MICROPY_PY_SSL)
#endif

// Depends on MICROPY_PY_CRYPTOLIB
#ifndef MICROPY_PY_CRYPTOLIB_CTR
#define MICROPY_PY_CRYPTOLIB_CTR (0)
#endif

#ifndef MICROPY_PY_CRYPTOLIB_CONSTS
#define MICROPY_PY_CRYPTOLIB_CONSTS (0)
#endif

#ifndef MICROPY_PY_BINASCII
#define MICROPY_PY_BINASCII (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Depends on MICROPY_PY_DEFLATE
#ifndef MICROPY_PY_BINASCII_CRC32
#define MICROPY_PY_BINASCII_CRC32 (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

#ifndef MICROPY_PY_RANDOM
#define MICROPY_PY_RANDOM (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to include: randrange, randint, choice, random, uniform
#ifndef MICROPY_PY_RANDOM_EXTRA_FUNCS
#define MICROPY_PY_RANDOM_EXTRA_FUNCS (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

#ifndef MICROPY_PY_MACHINE
#define MICROPY_PY_MACHINE (0)
#endif

// Whether to include: reset, reset_cause
#ifndef MICROPY_PY_MACHINE_RESET
#define MICROPY_PY_MACHINE_RESET (0)
#endif

// Maximum number of arguments for machine.freq()
#ifndef MICROPY_PY_MACHINE_FREQ_NUM_ARGS_MAX
#define MICROPY_PY_MACHINE_FREQ_NUM_ARGS_MAX (1)
#endif

// Whether to include: bitstream
#ifndef MICROPY_PY_MACHINE_BITSTREAM
#define MICROPY_PY_MACHINE_BITSTREAM (0)
#endif

// Whether to include: time_pulse_us
#ifndef MICROPY_PY_MACHINE_PULSE
#define MICROPY_PY_MACHINE_PULSE (0)
#endif

// Whether to provide the "machine.mem8/16/32" objects
#ifndef MICROPY_PY_MACHINE_MEMX
#define MICROPY_PY_MACHINE_MEMX (MICROPY_PY_MACHINE)
#endif

// Whether to provide the "machine.mem_backup" function
#ifndef MICROPY_PY_MACHINE_MEM_BACKUP
#define MICROPY_PY_MACHINE_MEM_BACKUP (0)
#endif

// Whether to provide the "machine.Signal" class
#ifndef MICROPY_PY_MACHINE_SIGNAL
#define MICROPY_PY_MACHINE_SIGNAL (MICROPY_PY_MACHINE)
#endif

#ifndef MICROPY_PY_MACHINE_I2C
#define MICROPY_PY_MACHINE_I2C (0)
#endif

// Whether the low-level I2C transfer function supports a separate write as the first transfer
#ifndef MICROPY_PY_MACHINE_I2C_TRANSFER_WRITE1
#define MICROPY_PY_MACHINE_I2C_TRANSFER_WRITE1 (0)
#endif

// Whether to provide the "machine.SoftI2C" class
#ifndef MICROPY_PY_MACHINE_SOFTI2C
#define MICROPY_PY_MACHINE_SOFTI2C (0)
#endif

#ifndef MICROPY_PY_MACHINE_SPI
#define MICROPY_PY_MACHINE_SPI (0)
#endif

// Whether to provide the "machine.SoftSPI" class
#ifndef MICROPY_PY_MACHINE_SOFTSPI
#define MICROPY_PY_MACHINE_SOFTSPI (0)
#endif

// Values of SPI.MSB and SPI.LSB constants
#ifndef MICROPY_PY_MACHINE_SPI_MSB
#define MICROPY_PY_MACHINE_SPI_MSB (0)
#define MICROPY_PY_MACHINE_SPI_LSB (1)
#endif

// Whether to provide the "machine.Timer" class
#ifndef MICROPY_PY_MACHINE_TIMER
#define MICROPY_PY_MACHINE_TIMER (0)
#endif

// The default backlog value for socket.listen(backlog)
#ifndef MICROPY_PY_SOCKET_LISTEN_BACKLOG_DEFAULT
#define MICROPY_PY_SOCKET_LISTEN_BACKLOG_DEFAULT (2)
#endif

// Whether to enable lwIP bindings to be used as the implementation of the `socket` module
#ifndef MICROPY_PY_LWIP
#define MICROPY_PY_LWIP (0)
#endif

// Whether to support raw sockets via the `socket.SOCK_RAW` constant
#ifndef MICROPY_PY_LWIP_SOCK_RAW
#define MICROPY_PY_LWIP_SOCK_RAW (MICROPY_PY_LWIP)
#endif

#ifndef MICROPY_PY_SSL
#define MICROPY_PY_SSL (0)
#endif

// Whether to add finaliser code to ssl objects
#ifndef MICROPY_PY_SSL_FINALISER
#define MICROPY_PY_SSL_FINALISER (MICROPY_ENABLE_FINALISER)
#endif

// Whether to add a root pointer for the current ssl object
#ifndef MICROPY_PY_SSL_MBEDTLS_NEED_ACTIVE_CONTEXT
#define MICROPY_PY_SSL_MBEDTLS_NEED_ACTIVE_CONTEXT (MICROPY_PY_SSL_ECDSA_SIGN_ALT)
#endif

// Whether to support DTLS protocol (non-CPython feature)
#ifndef MICROPY_PY_SSL_DTLS
#define MICROPY_PY_SSL_DTLS (MICROPY_SSL_MBEDTLS && MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

// Whether to provide the "vfs" module
#ifndef MICROPY_PY_VFS
#define MICROPY_PY_VFS (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_CORE_FEATURES && MICROPY_VFS)
#endif

#ifndef MICROPY_PY_WEBSOCKET
#define MICROPY_PY_WEBSOCKET (0)
#endif

#ifndef MICROPY_PY_FRAMEBUF
#define MICROPY_PY_FRAMEBUF (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

#ifndef MICROPY_PY_BTREE
#define MICROPY_PY_BTREE (0)
#endif

// Whether to provide the low-level "_onewire" module
#ifndef MICROPY_PY_ONEWIRE
#define MICROPY_PY_ONEWIRE (0)
#endif

// Whether to provide the "platform" module
#ifndef MICROPY_PY_PLATFORM
#define MICROPY_PY_PLATFORM (MICROPY_CONFIG_ROM_LEVEL_AT_LEAST_EXTRA_FEATURES)
#endif

/*****************************************************************************/
/* Hooks for a port to add builtins                                          */

// Additional builtin function definitions - see modbuiltins.c:mp_module_builtins_globals_table for format.
#ifndef MICROPY_PORT_BUILTINS
#define MICROPY_PORT_BUILTINS
#endif

// Additional builtin function definitions for extension by command-line, boards or variants.
// See modbuiltins.c:mp_module_builtins_globals_table for format.
#ifndef MICROPY_PORT_EXTRA_BUILTINS
#define MICROPY_PORT_EXTRA_BUILTINS
#endif

// Additional constant definitions for the compiler - see compile.c:mp_constants_table.
#ifndef MICROPY_PORT_CONSTANTS
#define MICROPY_PORT_CONSTANTS
#endif

/*****************************************************************************/
/* Hooks for a port to wrap functions with attributes                        */

#ifndef MICROPY_WRAP_MP_BINARY_OP
#define MICROPY_WRAP_MP_BINARY_OP(f) f
#endif

#ifndef MICROPY_WRAP_MP_EXECUTE_BYTECODE
#define MICROPY_WRAP_MP_EXECUTE_BYTECODE(f) f
#endif

#ifndef MICROPY_WRAP_MP_LOAD_GLOBAL
#define MICROPY_WRAP_MP_LOAD_GLOBAL(f) f
#endif

#ifndef MICROPY_WRAP_MP_LOAD_NAME
#define MICROPY_WRAP_MP_LOAD_NAME(f) f
#endif

#ifndef MICROPY_WRAP_MP_MAP_LOOKUP
#define MICROPY_WRAP_MP_MAP_LOOKUP(f) f
#endif

#ifndef MICROPY_WRAP_MP_OBJ_GET_TYPE
#define MICROPY_WRAP_MP_OBJ_GET_TYPE(f) f
#endif

#ifndef MICROPY_WRAP_MP_SCHED_EXCEPTION
#define MICROPY_WRAP_MP_SCHED_EXCEPTION(f) f
#endif

#ifndef MICROPY_WRAP_MP_SCHED_KEYBOARD_INTERRUPT
#define MICROPY_WRAP_MP_SCHED_KEYBOARD_INTERRUPT(f) f
#endif

#ifndef MICROPY_WRAP_MP_SCHED_SCHEDULE
#define MICROPY_WRAP_MP_SCHED_SCHEDULE(f) f
#endif

#ifndef MICROPY_WRAP_MP_SCHED_VM_ABORT
#define MICROPY_WRAP_MP_SCHED_VM_ABORT(f) f
#endif

/*****************************************************************************/
/* Miscellaneous settings                                                    */

// All MicroPython objects in ROM must be aligned on at least a 4 byte boundary
// so that the small-int/qstr/pointer distinction can be made.  For machines
// that don't do this (eg 16-bit CPU), define the following macro to something
// like __attribute__((aligned(4))).
#ifndef MICROPY_OBJ_BASE_ALIGNMENT
#define MICROPY_OBJ_BASE_ALIGNMENT
#endif

// String used for the banner, and sys.version additional information
#ifndef MICROPY_BANNER_NAME_AND_VERSION
#if MICROPY_PREVIEW_VERSION_2
#define MICROPY_BANNER_NAME_AND_VERSION "MicroPython (with v2.0 preview) " MICROPY_GIT_TAG " on " MICROPY_BUILD_DATE
#else
#define MICROPY_BANNER_NAME_AND_VERSION "MicroPython " MICROPY_GIT_TAG " on " MICROPY_BUILD_DATE
#endif
#endif

// String used for the second part of the banner, and sys.implementation._machine
#ifndef MICROPY_BANNER_MACHINE
#ifdef MICROPY_HW_BOARD_NAME
#define MICROPY_BANNER_MACHINE MICROPY_HW_BOARD_NAME " with " MICROPY_HW_MCU_NAME
#else
#define MICROPY_BANNER_MACHINE MICROPY_PY_SYS_PLATFORM " [" MICROPY_PLATFORM_COMPILER "] version"
#endif
#endif

// Number of bytes in an object word: mp_obj_t, mp_uint_t, mp_uint_t
#ifndef MP_BYTES_PER_OBJ_WORD
#define MP_BYTES_PER_OBJ_WORD (sizeof(mp_uint_t))
#endif

// Number of bits in a byte
#ifndef MP_BITS_PER_BYTE
#define MP_BITS_PER_BYTE (8)
#endif
// mp_int_t value with most significant bit set
#define MP_OBJ_WORD_MSBIT_HIGH (((mp_uint_t)1) << (MP_BYTES_PER_OBJ_WORD * MP_BITS_PER_BYTE - 1))

// Make sure both MP_ENDIANNESS_LITTLE and MP_ENDIANNESS_BIG are
// defined and that they are the opposite of each other.
#if defined(MP_ENDIANNESS_LITTLE)
#define MP_ENDIANNESS_BIG (!MP_ENDIANNESS_LITTLE)
#elif defined(MP_ENDIANNESS_BIG)
#define MP_ENDIANNESS_LITTLE (!MP_ENDIANNESS_BIG)
#else
// Endianness not defined by port so try to autodetect it.
  #if defined(__BYTE_ORDER__)
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
      #define MP_ENDIANNESS_LITTLE (1)
    #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
      #define MP_ENDIANNESS_LITTLE (0)
    #endif
  #else
    #include <endian.h>
      #if defined(__BYTE_ORDER)
        #if __BYTE_ORDER == __LITTLE_ENDIAN
          #define MP_ENDIANNESS_LITTLE (1)
        #elif __BYTE_ORDER == __BIG_ENDIAN
          #define MP_ENDIANNESS_LITTLE (0)
        #endif
      #endif
  #endif
  #ifndef MP_ENDIANNESS_LITTLE
    #error endianness not defined and cannot detect it
  #endif
  #define MP_ENDIANNESS_BIG (!MP_ENDIANNESS_LITTLE)
#endif

// Make a pointer to RAM callable (eg set lower bit for Thumb code)
// (This scheme won't work if we want to mix Thumb and normal ARM code.)
#ifndef MICROPY_MAKE_POINTER_CALLABLE
#define MICROPY_MAKE_POINTER_CALLABLE(p) (p)
#endif

// Whether native text/BSS/rodata memory loaded from .mpy files is explicitly tracked
// so that the GC cannot reclaim it.
//
// In general a port should let these options have their defaults, but the defaults here
// can be overridden if needed by defining both MICROPY_PERSISTENT_CODE_TRACK_FUN_DATA
// and MICROPY_PERSISTENT_CODE_TRACK_BSS_RODATA.
#ifndef MICROPY_PERSISTENT_CODE_TRACK_FUN_DATA
#if (MICROPY_EMIT_INLINE_ASM || MICROPY_ENABLE_NATIVE_CODE) && MICROPY_PERSISTENT_CODE_LOAD
// Pointer tracking is required when loading native code is enabled.
#if defined(MP_PLAT_ALLOC_EXEC) || defined(MP_PLAT_COMMIT_EXEC)
// If a port defined a custom allocator or commit function for native text, then the
// text does not need to be tracked (its allocation is managed by the port).  But the
// BSS/rodata must be tracked (if there is any) because if there are any pointers to it
// in the function data, they aren't traced by the GC.
#define MICROPY_PERSISTENT_CODE_TRACK_FUN_DATA (0)
#define MICROPY_PERSISTENT_CODE_TRACK_BSS_RODATA (1)
#else
// If a port uses the default allocator (the GC heap) then all native text is allocated
// on the GC heap.  But it's not guaranteed that a pointer to the head of the block of
// native text (which may contain multiple native functions) will be retained for the GC
// to trace.  This is because native functions can start inside the big block of text
// and so it's possible that the only GC-reachable pointers are pointers inside.
// Therefore the big block is explicitly tracked. If there is any BSS/rodata memory,
// then it does not need to be explicitly tracked because a pointer to it is stored into
// the function text via `mp_native_relocate()`.
#define MICROPY_PERSISTENT_CODE_TRACK_FUN_DATA (1)
#define MICROPY_PERSISTENT_CODE_TRACK_BSS_RODATA (0)
#endif
#else // (MICROPY_EMIT_INLINE_ASM || MICROPY_ENABLE_NATIVE_CODE) && MICROPY_PERSISTENT_CODE_LOAD
// Pointer tracking not needed when loading native code is disabled.
#define MICROPY_PERSISTENT_CODE_TRACK_FUN_DATA (0)
#define MICROPY_PERSISTENT_CODE_TRACK_BSS_RODATA (0)
#endif
#endif

// If these macros are defined then the memory allocated by them does not need to be
// traced by the GC.  But if they are left undefined then the GC heap will be used as
// the allocator and the memory must be traced by the GC.  See also above logic for
// enabling MICROPY_PERSISTENT_CODE_TRACK_FUN_DATA and
// MICROPY_PERSISTENT_CODE_TRACK_BSS_RODATA.
#ifndef MP_PLAT_ALLOC_EXEC
#define MP_PLAT_ALLOC_EXEC(min_size, ptr, size) do { *ptr = m_new(byte, min_size); *size = min_size; } while (0)
#define MP_PLAT_FREE_EXEC(ptr, size) m_del(byte, ptr, size)
#endif

// Allocating new heap area at runtime requires port to be able to allocate from system heap
#if MICROPY_GC_SPLIT_HEAP_AUTO
#ifndef MP_PLAT_ALLOC_HEAP
#define MP_PLAT_ALLOC_HEAP(size) malloc(size)
#endif
#ifndef MP_PLAT_FREE_HEAP
#define MP_PLAT_FREE_HEAP(ptr) free(ptr)
#endif
#endif

// This macro is used to do all output (except when MICROPY_PY_IO is defined)
#ifndef MP_PLAT_PRINT_STRN
#define MP_PLAT_PRINT_STRN(str, len) mp_hal_stdout_tx_strn_cooked(str, len)
#endif

#ifndef MP_SSIZE_MAX
#define MP_SSIZE_MAX SSIZE_MAX
#endif

// Modifier for function which doesn't return
#ifndef MP_NORETURN
#define MP_NORETURN __attribute__((noreturn))
#endif

#if !MICROPY_PREVIEW_VERSION_2
#define NORETURN MP_NORETURN
#endif

// Modifier for weak functions
#ifndef MP_WEAK
#define MP_WEAK __attribute__((weak))
#endif

// Modifier for functions which should be never inlined
#ifndef MP_NOINLINE
#define MP_NOINLINE __attribute__((noinline))
#endif

// Modifier for functions which should be always inlined
#ifndef MP_ALWAYSINLINE
#define MP_ALWAYSINLINE __attribute__((always_inline))
#endif

// Condition is likely to be true, to help branch prediction
#ifndef MP_LIKELY
#define MP_LIKELY(x) __builtin_expect((x), 1)
#endif

// Condition is likely to be false, to help branch prediction
#ifndef MP_UNLIKELY
#define MP_UNLIKELY(x) __builtin_expect((x), 0)
#endif

// To annotate that code is unreachable
#ifndef MP_UNREACHABLE
#if defined(__GNUC__)
#define MP_UNREACHABLE __builtin_unreachable();
#else
#define MP_UNREACHABLE for (;;);
#endif
#endif

// Explicitly annotate switch case fall throughs
#if defined(__GNUC__) && __GNUC__ >= 7
#define MP_FALLTHROUGH __attribute__((fallthrough));
#else
#define MP_FALLTHROUGH
#endif

#ifndef MP_HTOBE16
#if MP_ENDIANNESS_LITTLE
#define MP_HTOBE16(x) MP_BSWAP16(x)
#define MP_BE16TOH(x) MP_HTOBE16(x)
#else
#define MP_HTOBE16(x) (x)
#define MP_BE16TOH(x) (x)
#endif
#endif

#ifndef MP_HTOBE32
#if MP_ENDIANNESS_LITTLE
#define MP_HTOBE32(x) MP_BSWAP32(x)
#define MP_BE32TOH(x) MP_HTOBE32(x)
#else
#define MP_HTOBE32(x) (x)
#define MP_BE32TOH(x) (x)
#endif
#endif

// Warning categories are by default implemented as strings, though
// hook is left for a port to define them as something else.
#if MICROPY_WARNINGS_CATEGORY
#ifndef MP_WARN_CAT
#define MP_WARN_CAT(x) #x
#endif
#else
#undef MP_WARN_CAT
#define MP_WARN_CAT(x) (NULL)
#endif

// If true, use __builtin_mul_overflow (a gcc intrinsic supported by clang) for
// overflow checking when multiplying two small ints. Otherwise, use a portable
// algorithm.
//
// Most MCUs have a 32x32->64 bit multiply instruction, in which case the
// intrinsic is likely to be faster and generate smaller code. The main exception is
// cortex-m0 with __ARM_ARCH_ISA_THUMB == 1.
//
// The intrinsic is in GCC starting with version 5.
#ifndef MICROPY_USE_GCC_MUL_OVERFLOW_INTRINSIC
#if defined(__ARM_ARCH_ISA_THUMB) && (__GNUC__ >= 5)
#define MICROPY_USE_GCC_MUL_OVERFLOW_INTRINSIC (__ARM_ARCH_ISA_THUMB >= 2)
#elif (__GNUC__ >= 5)
#define MICROPY_USE_GCC_MUL_OVERFLOW_INTRINSIC (1)
#else
#define MICROPY_USE_GCC_MUL_OVERFLOW_INTRINSIC (0)
#endif
#endif

#endif // MICROPY_INCLUDED_PY_MPCONFIG_H
/* py/mpthread.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George on behalf of Pycom Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_MPTHREAD_H
#define MICROPY_INCLUDED_PY_MPTHREAD_H


#if MICROPY_PY_THREAD

struct _mp_state_thread_t;

#ifdef MICROPY_MPTHREADPORT_H
#include MICROPY_MPTHREADPORT_H
#else
#include <mpthreadport.h>
#endif

struct _mp_state_thread_t *mp_thread_get_state(void);
void mp_thread_set_state(struct _mp_state_thread_t *state);
mp_uint_t mp_thread_create(void *(*entry)(void *), void *arg, size_t *stack_size);
mp_uint_t mp_thread_get_id(void);
void mp_thread_start(void);
void mp_thread_finish(void);
void mp_thread_mutex_init(mp_thread_mutex_t *mutex);
int mp_thread_mutex_lock(mp_thread_mutex_t *mutex, int wait);
void mp_thread_mutex_unlock(mp_thread_mutex_t *mutex);

#if MICROPY_PY_THREAD_RECURSIVE_MUTEX
void mp_thread_recursive_mutex_init(mp_thread_recursive_mutex_t *mutex);
int mp_thread_recursive_mutex_lock(mp_thread_recursive_mutex_t *mutex, int wait);
void mp_thread_recursive_mutex_unlock(mp_thread_recursive_mutex_t *mutex);
#endif

#endif // MICROPY_PY_THREAD

#if MICROPY_PY_THREAD && MICROPY_PY_THREAD_GIL
#define MP_THREAD_GIL_ENTER() mp_thread_mutex_lock(&MP_STATE_VM(gil_mutex), 1)
#define MP_THREAD_GIL_EXIT() mp_thread_mutex_unlock(&MP_STATE_VM(gil_mutex))
#else
#define MP_THREAD_GIL_ENTER()
#define MP_THREAD_GIL_EXIT()
#endif

#endif // MICROPY_INCLUDED_PY_MPTHREAD_H
/* py/misc.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_MISC_H
#define MICROPY_INCLUDED_PY_MISC_H

#include <string.h>

// a mini library of useful types and functions

/** types *******************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#if __cplusplus // Required on at least one compiler to get ULLONG_MAX
#include <climits>
#else
#include <limits.h>
#endif

typedef unsigned char byte;
typedef unsigned int uint;

#ifndef __has_builtin
#define __has_builtin(x) (0)
#endif
#ifndef __has_feature
// This macro is supported by Clang and gcc>=14
#define __has_feature(x) (0)
#endif


/** generic ops *************************************************/

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

// Classical double-indirection stringification of preprocessor macro's value
#define MP_STRINGIFY_HELPER(x) #x
#define MP_STRINGIFY(x) MP_STRINGIFY_HELPER(x)

// Static assertion macro
#if __cplusplus
#define MP_STATIC_ASSERT(cond) static_assert((cond), #cond)
#elif __GNUC__ >= 5 || __STDC_VERSION__ >= 201112L
#define MP_STATIC_ASSERT(cond) _Static_assert((cond), #cond)
#else
#define MP_STATIC_ASSERT(cond) ((void)sizeof(char[1 - 2 * !(cond)]))
#endif

// In C++ things like comparing extern const pointers are not constant-expressions so cannot be used
// in MP_STATIC_ASSERT. Note that not all possible compiler versions will reject this. Some gcc versions
// do, others only with -Werror=vla, msvc always does.
// The (void) is needed to avoid "left operand of comma operator has no effect [-Werror=unused-value]"
// when using this macro on the left-hand side of a comma.
#if defined(_MSC_VER) || defined(__cplusplus)
#define MP_STATIC_ASSERT_NONCONSTEXPR(cond) ((void)1)
#else
#if __clang__
#pragma GCC diagnostic ignored "-Wgnu-folding-constant"
#endif
#define MP_STATIC_ASSERT_NONCONSTEXPR(cond) ((void)sizeof(char[1 - 2 * !(cond)]))
#endif

// Round-up integer division
#define MP_CEIL_DIVIDE(a, b) (((a) + (b) - 1) / (b))
#define MP_ROUND_DIVIDE(a, b) (((a) + (b) / 2) / (b))

/** memory allocation ******************************************/

// TODO make a lazy m_renew that can increase by a smaller amount than requested (but by at least 1 more element)

#define m_new(type, num) ((type *)(m_malloc(sizeof(type) * (num))))
#define m_new_maybe(type, num) ((type *)(m_malloc_maybe(sizeof(type) * (num))))
#define m_new0(type, num) ((type *)(m_malloc0(sizeof(type) * (num))))
#define m_new_obj(type) (m_new(type, 1))
#define m_new_obj_maybe(type) (m_new_maybe(type, 1))
#define m_new_obj_var(obj_type, var_field, var_type, var_num) ((obj_type *)m_malloc(offsetof(obj_type, var_field) + sizeof(var_type) * (var_num)))
#define m_new_obj_var0(obj_type, var_field, var_type, var_num) ((obj_type *)m_malloc0(offsetof(obj_type, var_field) + sizeof(var_type) * (var_num)))
#define m_new_obj_var_maybe(obj_type, var_field, var_type, var_num) ((obj_type *)m_malloc_maybe(offsetof(obj_type, var_field) + sizeof(var_type) * (var_num)))
#if MICROPY_MALLOC_USES_ALLOCATED_SIZE
#define m_renew(type, ptr, old_num, new_num) ((type *)(m_realloc((ptr), sizeof(type) * (old_num), sizeof(type) * (new_num))))
#define m_renew_maybe(type, ptr, old_num, new_num, allow_move) ((type *)(m_realloc_maybe((ptr), sizeof(type) * (old_num), sizeof(type) * (new_num), (allow_move))))
#define m_del(type, ptr, num) m_free(ptr, sizeof(type) * (num))
#define m_del_var(obj_type, var_field, var_type, var_num, ptr) (m_free(ptr, offsetof(obj_type, var_field) + sizeof(var_type) * (var_num)))
#else
#define m_renew(type, ptr, old_num, new_num) ((type *)(m_realloc((ptr), sizeof(type) * (new_num))))
#define m_renew_maybe(type, ptr, old_num, new_num, allow_move) ((type *)(m_realloc_maybe((ptr), sizeof(type) * (new_num), (allow_move))))
#define m_del(type, ptr, num) ((void)(num), m_free(ptr))
#define m_del_var(obj_type, var_field, var_type, var_num, ptr) ((void)(var_num), m_free(ptr))
#endif
#define m_del_obj(type, ptr) (m_del(type, ptr, 1))

void *m_malloc(size_t num_bytes);
void *m_malloc_maybe(size_t num_bytes);
void *m_malloc_with_finaliser(size_t num_bytes);
void *m_malloc0(size_t num_bytes);
#if MICROPY_MALLOC_USES_ALLOCATED_SIZE
void *m_realloc(void *ptr, size_t old_num_bytes, size_t new_num_bytes);
void *m_realloc_maybe(void *ptr, size_t old_num_bytes, size_t new_num_bytes, bool allow_move);
void m_free(void *ptr, size_t num_bytes);
#else
void *m_realloc(void *ptr, size_t new_num_bytes);
void *m_realloc_maybe(void *ptr, size_t new_num_bytes, bool allow_move);
void m_free(void *ptr);
#endif
MP_NORETURN void m_malloc_fail(size_t num_bytes);

#if MICROPY_TRACKED_ALLOC
// These alloc/free functions track the pointers in a linked list so the GC does not reclaim
// them.  They can be used by code that requires traditional C malloc/free semantics.
void *m_tracked_calloc(size_t nmemb, size_t size);
void *m_tracked_realloc(void *ptr_in, size_t n_bytes);
void m_tracked_free(void *ptr_in);
#endif

#if MICROPY_MEM_STATS
size_t m_get_total_bytes_allocated(void);
size_t m_get_current_bytes_allocated(void);
size_t m_get_peak_bytes_allocated(void);
#endif

/** array helpers ***********************************************/

// get the number of elements in a fixed-size array
#define MP_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

// align ptr to the nearest multiple of "alignment"
#define MP_ALIGN(ptr, alignment) (void *)(((uintptr_t)(ptr) + ((alignment) - 1)) & ~((alignment) - 1))

/** unichar / UTF-8 *********************************************/

#if MICROPY_PY_BUILTINS_STR_UNICODE
// with unicode enabled we need a type which can fit chars up to 0x10ffff
typedef uint32_t unichar;
#else
// without unicode enabled we can only need to fit chars up to 0xff
// (on 16-bit archs uint is 16-bits and more efficient than uint32_t)
typedef uint unichar;
#endif

#if MICROPY_PY_BUILTINS_STR_UNICODE
unichar utf8_get_char(const byte *s);
const byte *utf8_next_char(const byte *s);
size_t utf8_charlen(const byte *str, size_t len);
#else
static inline unichar utf8_get_char(const byte *s) {
    return *s;
}
static inline const byte *utf8_next_char(const byte *s) {
    return s + 1;
}
static inline size_t utf8_charlen(const byte *str, size_t len) {
    (void)str;
    return len;
}
#endif

bool unichar_isspace(unichar c);
bool unichar_isalpha(unichar c);
bool unichar_isprint(unichar c);
bool unichar_isdigit(unichar c);
bool unichar_isxdigit(unichar c);
bool unichar_isident(unichar c);
bool unichar_isalnum(unichar c);
bool unichar_isupper(unichar c);
bool unichar_islower(unichar c);
unichar unichar_tolower(unichar c);
unichar unichar_toupper(unichar c);
mp_uint_t unichar_xdigit_value(unichar c);
#define UTF8_IS_NONASCII(ch) ((ch) & 0x80)
#define UTF8_IS_CONT(ch) (((ch) & 0xC0) == 0x80)

/** variable string *********************************************/

typedef struct _vstr_t {
    size_t alloc;
    size_t len;
    char *buf;
    bool fixed_buf;
} vstr_t;

// convenience macro to declare a vstr with a fixed size buffer on the stack
#define VSTR_FIXED(vstr, alloc) vstr_t vstr; char vstr##_buf[(alloc)]; vstr_init_fixed_buf(&vstr, (alloc), vstr##_buf);

void vstr_init(vstr_t *vstr, size_t alloc);
void vstr_init_len(vstr_t *vstr, size_t len);
void vstr_init_fixed_buf(vstr_t *vstr, size_t alloc, char *buf);
struct _mp_print_t;
void vstr_init_print(vstr_t *vstr, size_t alloc, struct _mp_print_t *print);
void vstr_clear(vstr_t *vstr);
vstr_t *vstr_new(size_t alloc);
void vstr_free(vstr_t *vstr);
static inline void vstr_reset(vstr_t *vstr) {
    vstr->len = 0;
}
static inline char *vstr_str(vstr_t *vstr) {
    return vstr->buf;
}
static inline size_t vstr_len(vstr_t *vstr) {
    return vstr->len;
}
void vstr_hint_size(vstr_t *vstr, size_t size);
char *vstr_extend(vstr_t *vstr, size_t size);
char *vstr_add_len(vstr_t *vstr, size_t len);
char *vstr_null_terminated_str(vstr_t *vstr);
void vstr_add_byte(vstr_t *vstr, byte v);
void vstr_add_char(vstr_t *vstr, unichar chr);
void vstr_add_str(vstr_t *vstr, const char *str);
void vstr_add_strn(vstr_t *vstr, const char *str, size_t len);
char *vstr_ins_blank_bytes(vstr_t *vstr, size_t byte_pos, size_t byte_len);
static inline void vstr_ins_byte(vstr_t *vstr, size_t byte_pos, byte b) {
    char *s = vstr_ins_blank_bytes(vstr, byte_pos, 1);
    *s = b;
}
static inline void vstr_ins_char(vstr_t *vstr, size_t char_pos, unichar chr) {
    // TODO UNICODE
    char *s = vstr_ins_blank_bytes(vstr, char_pos, 1);
    *s = (char)chr;
}
static inline void vstr_ins_strn(vstr_t *vstr, size_t byte_pos, const char *str, size_t len) {
    char *s = vstr_ins_blank_bytes(vstr, byte_pos, len);
    memcpy(s, str, len);
}
void vstr_cut_head_bytes(vstr_t *vstr, size_t bytes_to_cut);
void vstr_cut_tail_bytes(vstr_t *vstr, size_t bytes_to_cut);
void vstr_cut_out_bytes(vstr_t *vstr, size_t byte_pos, size_t bytes_to_cut);
void vstr_printf(vstr_t *vstr, const char *fmt, ...);
#ifdef va_start
void vstr_vprintf(vstr_t *vstr, const char *fmt, va_list ap);
#endif

/** debugging helpers *******************************************/

int DEBUG_printf(const char *fmt, ...);

/** float internals *************/

#if MICROPY_PY_BUILTINS_FLOAT

#if MICROPY_FLOAT_IMPL == MICROPY_FLOAT_IMPL_DOUBLE
#define MP_FLOAT_EXP_BITS (11)
#define MP_FLOAT_EXP_OFFSET (1023)
#define MP_FLOAT_FRAC_BITS (52)
typedef uint64_t mp_float_uint_t;
#elif MICROPY_FLOAT_IMPL == MICROPY_FLOAT_IMPL_FLOAT
#define MP_FLOAT_EXP_BITS (8)
#define MP_FLOAT_EXP_OFFSET (127)
#define MP_FLOAT_FRAC_BITS (23)
typedef uint32_t mp_float_uint_t;
#endif

#define MP_FLOAT_EXP_BIAS ((1 << (MP_FLOAT_EXP_BITS - 1)) - 1)

typedef union _mp_float_union_t {
    mp_float_t f;
    #if MP_ENDIANNESS_LITTLE
    struct {
        mp_float_uint_t frc : MP_FLOAT_FRAC_BITS;
        mp_float_uint_t exp : MP_FLOAT_EXP_BITS;
        mp_float_uint_t sgn : 1;
    } p;
    #else
    struct {
        mp_float_uint_t sgn : 1;
        mp_float_uint_t exp : MP_FLOAT_EXP_BITS;
        mp_float_uint_t frc : MP_FLOAT_FRAC_BITS;
    } p;
    #endif
    mp_float_uint_t i;
} mp_float_union_t;

#if MICROPY_FLOAT_FORMAT_IMPL == MICROPY_FLOAT_FORMAT_IMPL_EXACT

#if MICROPY_FLOAT_IMPL == MICROPY_FLOAT_IMPL_FLOAT
// Exact float conversion requires using internally a bigger sort of floating point
typedef double mp_large_float_t;
#elif MICROPY_FLOAT_IMPL == MICROPY_FLOAT_IMPL_DOUBLE
typedef long double mp_large_float_t;
#endif
// Always use a 64 bit mantissa for formatting and parsing
typedef uint64_t mp_large_float_uint_t;

#else // MICROPY_FLOAT_FORMAT_IMPL != MICROPY_FLOAT_FORMAT_IMPL_EXACT

// No bigger floating points
typedef mp_float_t mp_large_float_t;
typedef mp_float_uint_t mp_large_float_uint_t;

#endif

#endif // MICROPY_PY_BUILTINS_FLOAT

/** ROM string compression *************/

#if MICROPY_ROM_TEXT_COMPRESSION

#if MICROPY_ERROR_REPORTING == MICROPY_ERROR_REPORTING_NONE
#error "MICROPY_ERROR_REPORTING_NONE requires MICROPY_ROM_TEXT_COMPRESSION disabled"
#endif

#ifdef NO_QSTR

// Compression enabled but doing QSTR extraction.
// So leave MP_COMPRESSED_ROM_TEXT in place for makeqstrdefs.py / makecompresseddata.py to find them.

#else

// Compression enabled and doing a regular build.
// Map MP_COMPRESSED_ROM_TEXT to the compressed strings.

// Force usage of the MP_ERROR_TEXT macro by requiring an opaque type.
typedef struct {
    #if defined(__clang__) || defined(_MSC_VER)
    // Fix "error: empty struct has size 0 in C, size 1 in C++", and the msvc counterpart
    // "C requires that a struct or union have at least one member"
    char dummy;
    #endif
} *mp_rom_error_text_t;

#include <string.h>

inline MP_ALWAYSINLINE const char *MP_COMPRESSED_ROM_TEXT(const char *msg) {
    // "genhdr/compressed.data.h" contains an invocation of the MP_MATCH_COMPRESSED macro for each compressed string.
    // The giant if(strcmp) tree is optimized by the compiler, which turns this into a direct return of the compressed data.
    #define MP_MATCH_COMPRESSED(a, b) if (strcmp(msg, a) == 0) { return b; } else

    // It also contains a single invocation of the MP_COMPRESSED_DATA macro, we don't need that here.
    #define MP_COMPRESSED_DATA(x)

    #include "genhdr/compressed.data.h"

#undef MP_COMPRESSED_DATA
#undef MP_MATCH_COMPRESSED

    return msg;
}

#endif

#else

// Compression not enabled, just make it a no-op.

typedef const char *mp_rom_error_text_t;
#define MP_COMPRESSED_ROM_TEXT(x) x

#endif // MICROPY_ROM_TEXT_COMPRESSION

// Might add more types of compressed text in the future.
// For now, forward directly to MP_COMPRESSED_ROM_TEXT.
#define MP_ERROR_TEXT(x) (mp_rom_error_text_t)MP_COMPRESSED_ROM_TEXT(x)

// Portable implementations of CLZ and CTZ intrinsics
#ifdef _MSC_VER
#include <intrin.h>

static inline uint32_t mp_clz(uint32_t x) {
    unsigned long lz = 0;
    return _BitScanReverse(&lz, x) ? (sizeof(x) * 8 - 1) - lz : 0;
}

static inline uint32_t mp_clzl(unsigned long x) {
    unsigned long lz = 0;
    return _BitScanReverse(&lz, x) ? (sizeof(x) * 8 - 1) - lz : 0;
}

#ifdef _WIN64
static inline uint32_t mp_clzll(unsigned long long x) {
    unsigned long lz = 0;
    return _BitScanReverse64(&lz, x) ? (sizeof(x) * 8 - 1) - lz : 0;
}
#else
// Microsoft don't ship _BitScanReverse64 on Win32, so emulate it
static inline uint32_t mp_clzll(unsigned long long x) {
    unsigned long h = x >> 32;
    return h ? mp_clzl(h) : (mp_clzl((unsigned long)x) + 32);
}
#endif

static inline uint32_t mp_ctz(uint32_t x) {
    unsigned long tz = 0;
    return _BitScanForward(&tz, x) ? tz : 0;
}

static inline uint32_t mp_popcount(uint32_t x) {
    return __popcnt(x);
}
#else // _MSC_VER
#define mp_clz(x) __builtin_clz(x)
#define mp_clzl(x) __builtin_clzl(x)
#define mp_clzll(x) __builtin_clzll(x)
#define mp_ctz(x) __builtin_ctz(x)
#if __has_builtin(__builtin_popcount)
#define mp_popcount(x) __builtin_popcount(x)
#else
static inline uint32_t mp_popcount(uint32_t x) {
    x = x - ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x + (x >> 4)) & 0x0F0F0F0F;
    return (x * 0x01010101) >> 24;
}
#endif // __has_builtin(__builtin_popcount)
#endif // _MSC_VER

#define MP_FIT_UNSIGNED(bits, value) (((value) & (~0U << (bits))) == 0)
#define MP_FIT_SIGNED(bits, value) \
    (MP_FIT_UNSIGNED(((bits) - 1), (value)) || \
    (((value) & (~0U << ((bits) - 1))) == (~0U << ((bits) - 1))))

// mp_int_t can be larger than long, i.e. Windows 64-bit, nan-box variants
static inline uint32_t mp_clz_mpi(mp_int_t x) {
    #ifdef __XC16__
    mp_uint_t mask = MP_OBJ_WORD_MSBIT_HIGH;
    mp_uint_t zeroes = 0;
    while (mask != 0) {
        if (mask & (mp_uint_t)x) {
            break;
        }
        zeroes++;
        mask >>= 1;
    }
    return zeroes;
    #else
    #if MP_INT_MAX == INT_MAX
    return mp_clz((unsigned)x);
    #elif MP_INT_MAX == LONG_MAX
    return mp_clzl((unsigned long)x);
    #elif MP_INT_MAX == LLONG_MAX
    return mp_clzll((unsigned long long)x);
    #else
    #error Unexpected MP_INT_MAX value
    #endif
    #endif
}

// Overflow-checked operations

// Integer overflow builtins were added to GCC 5, but __has_builtin only in GCC 10
//
// Note that the builtins has a defined result when overflow occurs, whereas the custom
// functions below don't update the result if an overflow would occur (to avoid UB).
#define MP_GCC_HAS_BUILTIN_OVERFLOW (__GNUC__ >= 5)

#if MICROPY_USE_GCC_MUL_OVERFLOW_INTRINSIC

#define mp_mul_ull_overflow __builtin_umulll_overflow
#define mp_mul_ll_overflow __builtin_smulll_overflow
static inline bool mp_mul_mp_int_t_overflow(mp_int_t x, mp_int_t y, mp_int_t *res) {
    // __builtin_mul_overflow is a type-generic function, this inline ensures the argument
    // types are checked to match mp_int_t.
    return __builtin_mul_overflow(x, y, res);
}

#else

bool mp_mul_ll_overflow(long long int x, long long int y, long long int *res);
bool mp_mul_mp_int_t_overflow(mp_int_t x, mp_int_t y, mp_int_t *res);
static inline bool mp_mul_ull_overflow(unsigned long long int x, unsigned long long int y, unsigned long long int *res) {
    if (y > 0 && x > (ULLONG_MAX / y)) {
        return true; // overflow
    }
    *res = x * y;
    return false;
}

#endif

#if __has_builtin(__builtin_saddll_overflow) || MP_GCC_HAS_BUILTIN_OVERFLOW
#define mp_add_ll_overflow __builtin_saddll_overflow
#else
static inline bool mp_add_ll_overflow(long long int lhs, long long int rhs, long long int *res) {
    bool overflow;

    if (rhs > 0) {
        overflow = (lhs > LLONG_MAX - rhs);
    } else {
        overflow = (lhs < LLONG_MIN - rhs);
    }

    if (!overflow) {
        *res = lhs + rhs;
    }

    return overflow;
}
#endif

#if __has_builtin(__builtin_ssubll_overflow) || MP_GCC_HAS_BUILTIN_OVERFLOW
#define mp_sub_ll_overflow __builtin_ssubll_overflow
#else
static inline bool mp_sub_ll_overflow(long long int lhs, long long int rhs, long long int *res) {
    bool overflow;

    if (rhs > 0) {
        overflow = (lhs < LLONG_MIN + rhs);
    } else {
        overflow = (lhs > LLONG_MAX + rhs);
    }

    if (!overflow) {
        *res = lhs - rhs;
    }

    return overflow;
}
#endif


// Helper macros for detecting if sanitizers are enabled
//
// Use sparingly, not for masking issues reported by sanitizers!
//
// Can be detected automatically in Clang and gcc>=14, need to be
// set manually otherwise.
#ifndef MP_UBSAN
#define MP_UBSAN __has_feature(undefined_behavior_sanitizer)
#endif

#ifndef MP_ASAN
#define MP_ASAN __has_feature(address_sanitizer)
#endif

#ifndef MP_SANITIZER_BUILD
#define MP_SANITIZER_BUILD (MP_UBSAN || MP_ASAN)
#endif

// halfword/word/longword swapping macros

#if __has_builtin(__builtin_bswap16)
#define MP_BSWAP16(x) __builtin_bswap16(x)
#else
#define MP_BSWAP16(x) ((uint16_t)((((x) & 0xFF) << 8) | (((x) >> 8) & 0xFF)))
#endif

#if __has_builtin(__builtin_bswap32)
#define MP_BSWAP32(x) __builtin_bswap32(x)
#else
#define MP_BSWAP32(x) \
    ((uint32_t)((((x) & 0xFF) << 24) | (((x) & 0xFF00) << 8) | \
    (((x) >> 8) & 0xFF00) | (((x) >> 24) & 0xFF)))
#endif

#if __has_builtin(__builtin_bswap64)
#define MP_BSWAP64(x) __builtin_bswap64(x)
#else
#define MP_BSWAP64(x) \
    ((uint64_t)((((x) & 0xFF) << 56) | (((x) & 0xFF00) << 40) | \
    (((x) & 0xFF0000) << 24) | (((x) & 0xFF000000) << 8) | \
    (((x) >> 8) & 0xFF000000) | (((x) >> 24) & 0xFF0000) | \
    (((x) >> 40) & 0xFF00) | (((x) >> 56) & 0xFF)))
#endif

#endif // MICROPY_INCLUDED_PY_MISC_H
/* py/nlr.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2023 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_NLR_H
#define MICROPY_INCLUDED_PY_NLR_H

// non-local return
// exception handling, basically a stack of setjmp/longjmp buffers

#include <limits.h>
#include <assert.h>
#include <stdbool.h>


#define MICROPY_NLR_NUM_REGS_X86            (6)
#define MICROPY_NLR_NUM_REGS_X64            (8)
#define MICROPY_NLR_NUM_REGS_X64_WIN        (10)
#define MICROPY_NLR_NUM_REGS_ARM_THUMB      (10)
#define MICROPY_NLR_NUM_REGS_ARM_THUMB_FP   (10 + 6)
#define MICROPY_NLR_NUM_REGS_AARCH64        (13)
#define MICROPY_NLR_NUM_REGS_MIPS           (13)
#define MICROPY_NLR_NUM_REGS_XTENSA         (10)
#define MICROPY_NLR_NUM_REGS_XTENSAWIN      (17)
#define MICROPY_NLR_NUM_REGS_RV32I          (14)
#define MICROPY_NLR_NUM_REGS_RV64I          (14)
#define MICROPY_NLR_NUM_REGS_LOONG64        (13)

// *FORMAT-OFF*

// If MICROPY_NLR_SETJMP is not enabled then auto-detect the machine arch
#if !MICROPY_NLR_SETJMP
// A lot of nlr-related things need different treatment on Windows
#if defined(_WIN32) || defined(__CYGWIN__)
#define MICROPY_NLR_OS_WINDOWS 1
#else
#define MICROPY_NLR_OS_WINDOWS 0
#endif
#if defined(__i386__)
    #define MICROPY_NLR_X86 (1)
    #define MICROPY_NLR_NUM_REGS (MICROPY_NLR_NUM_REGS_X86)
#elif defined(__x86_64__)
    #define MICROPY_NLR_X64 (1)
    #if MICROPY_NLR_OS_WINDOWS
        #define MICROPY_NLR_NUM_REGS (MICROPY_NLR_NUM_REGS_X64_WIN)
    #else
        #define MICROPY_NLR_NUM_REGS (MICROPY_NLR_NUM_REGS_X64)
    #endif
#elif defined(__thumb2__) || defined(__thumb__) || defined(__arm__)
    #define MICROPY_NLR_THUMB (1)
    #if defined(__SOFTFP__)
        #define MICROPY_NLR_NUM_REGS (MICROPY_NLR_NUM_REGS_ARM_THUMB)
    #else
        // With hardware FP registers s16-s31 are callee save so in principle
        // should be saved and restored by the NLR code.  gcc only uses s16-s21
        // so only save/restore those as an optimisation.
        #define MICROPY_NLR_NUM_REGS (MICROPY_NLR_NUM_REGS_ARM_THUMB_FP)
    #endif
#elif defined(__aarch64__)
    #define MICROPY_NLR_AARCH64 (1)
    #define MICROPY_NLR_NUM_REGS (MICROPY_NLR_NUM_REGS_AARCH64)
#elif defined(__xtensa__)
    #define MICROPY_NLR_XTENSA (1)
    #define MICROPY_NLR_NUM_REGS (MICROPY_NLR_NUM_REGS_XTENSA)
#elif defined(__powerpc__)
    #define MICROPY_NLR_POWERPC (1)
    // this could be less but using 128 for safety
    #define MICROPY_NLR_NUM_REGS (128)
#elif defined(__mips__)
    #define MICROPY_NLR_MIPS (1)
    #define MICROPY_NLR_NUM_REGS (MICROPY_NLR_NUM_REGS_MIPS)
#elif defined(__riscv)
    #if __riscv_xlen == 32
        #define MICROPY_NLR_NUM_REGS (MICROPY_NLR_NUM_REGS_RV32I)
        #define MICROPY_NLR_RV32I (1)
    #elif __riscv_xlen == 64
        #define MICROPY_NLR_NUM_REGS (MICROPY_NLR_NUM_REGS_RV64I)
        #define MICROPY_NLR_RV64I (1)
    #else
        #error Unsupported RISC-V variant.
    #endif
#elif defined(__loongarch__)
    #if defined(__loongarch64)
        #define MICROPY_NLR_LOONG64 (1)
        #define MICROPY_NLR_NUM_REGS (MICROPY_NLR_NUM_REGS_LOONG64)
    #else
        #error Unsupported Loongarch variant.
    #endif
#else
    #define MICROPY_NLR_SETJMP (1)
    //#warning "No native NLR support for this arch, using setjmp implementation"
#endif
#endif

// *FORMAT-ON*

#if MICROPY_NLR_SETJMP
#include <setjmp.h>
#endif

typedef struct _nlr_buf_t nlr_buf_t;
struct _nlr_buf_t {
    // The entries in this struct must all be machine word size.

    // Pointer to the previous nlr_buf_t in the chain.
    // Or NULL if it's the top-level one.
    nlr_buf_t *prev;

    // The exception that is being raised:
    // - NULL means the jump is because of a VM abort (only if MICROPY_ENABLE_VM_ABORT enabled)
    // - otherwise it's always a concrete object (an exception instance)
    void *ret_val;

    #if MICROPY_NLR_SETJMP
    jmp_buf jmpbuf;
    #else
    void *regs[MICROPY_NLR_NUM_REGS];
    #endif

    #if MICROPY_ENABLE_PYSTACK
    void *pystack;
    #endif
};

typedef void (*nlr_jump_callback_fun_t)(void *ctx);

typedef struct _nlr_jump_callback_node_t nlr_jump_callback_node_t;

struct _nlr_jump_callback_node_t {
    nlr_jump_callback_node_t *prev;
    nlr_jump_callback_fun_t fun;
};

// Helper macros to save/restore the pystack state
#if MICROPY_ENABLE_PYSTACK
#define MP_NLR_SAVE_PYSTACK(nlr_buf) (nlr_buf)->pystack = MP_STATE_THREAD(pystack_cur)
#define MP_NLR_RESTORE_PYSTACK(nlr_buf) MP_STATE_THREAD(pystack_cur) = (nlr_buf)->pystack
#else
#define MP_NLR_SAVE_PYSTACK(nlr_buf) (void)nlr_buf
#define MP_NLR_RESTORE_PYSTACK(nlr_buf) (void)nlr_buf
#endif

// Helper macro to use at the start of a specific nlr_jump implementation
#define MP_NLR_JUMP_HEAD(val, top) \
    nlr_buf_t **_top_ptr = &MP_STATE_THREAD(nlr_top); \
    nlr_buf_t *top = *_top_ptr; \
    if (top == NULL) { \
        nlr_jump_fail(val); \
    } \
    top->ret_val = val; \
    nlr_call_jump_callbacks(top); \
    MP_NLR_RESTORE_PYSTACK(top); \
    *_top_ptr = top->prev; \

#if MICROPY_NLR_SETJMP
// nlr_push() must be defined as a macro, because "The stack context will be
// invalidated if the function which called setjmp() returns."
// For this case it is safe to call nlr_push_tail() first.
#define nlr_push(buf) (nlr_push_tail(buf), setjmp((buf)->jmpbuf))
#else
unsigned int nlr_push(nlr_buf_t *);
#endif

unsigned int nlr_push_tail(nlr_buf_t *top);
void nlr_pop(void);
MP_NORETURN void nlr_jump(void *val);

#if MICROPY_ENABLE_VM_ABORT
#define nlr_set_abort(buf) MP_STATE_VM(nlr_abort) = buf
#define nlr_get_abort() MP_STATE_VM(nlr_abort)
MP_NORETURN void nlr_jump_abort(void);
#endif

// This must be implemented by a port.  It's called by nlr_jump
// if no nlr buf has been pushed.  It must not return, but rather
// should bail out with a fatal error.
MP_NORETURN void nlr_jump_fail(void *val);

// use nlr_raise instead of nlr_jump so that debugging is easier
#ifndef MICROPY_DEBUG_NLR
#define nlr_raise(val) nlr_jump(MP_OBJ_TO_PTR(val))
#else

#define nlr_raise(val) \
    do { \
        void *_val = MP_OBJ_TO_PTR(val); \
        assert(_val != NULL); \
        assert(mp_obj_is_exception_instance(val)); \
        nlr_jump(_val); \
    } while (0)

#if !MICROPY_NLR_SETJMP
#define nlr_push(val) \
    assert(MP_STATE_THREAD(nlr_top) != val), nlr_push(val)
#endif

#endif

// Push a callback on to the linked-list of NLR jump callbacks.  The `node` pointer must
// be on the C stack.  The `fun` callback will be executed if an NLR jump is taken which
// unwinds the C stack through this `node`.
void nlr_push_jump_callback(nlr_jump_callback_node_t *node, nlr_jump_callback_fun_t fun);

// Pop a callback from the linked-list of NLR jump callbacks.  The corresponding function
// will be called if `run_callback` is true.
void nlr_pop_jump_callback(bool run_callback);

// Pop and call all NLR jump callbacks that were registered after `nlr` buffer was pushed.
void nlr_call_jump_callbacks(nlr_buf_t *nlr);

#endif // MICROPY_INCLUDED_PY_NLR_H
/* py/obj.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_OBJ_H
#define MICROPY_INCLUDED_PY_OBJ_H

#include <assert.h>

/* py/qstr.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_QSTR_H
#define MICROPY_INCLUDED_PY_QSTR_H


// See qstrdefs.h for a list of qstr's that are available as constants.
// Reference them as MP_QSTR_xxxx.
//
// Note: it would be possible to define MP_QSTR_xxx as qstr_from_str("xxx")
// for qstrs that are referenced this way, but you don't want to have them in ROM.

// first entry in enum will be MP_QSTRnull=0, which indicates invalid/no qstr
enum {
    #ifndef NO_QSTR
#define QDEF0(id, hash, len, str) id,
#define QDEF1(id, hash, len, str)
/* qstrdefs.generated.h */
// This file was automatically generated by makeqstrdata.py

QDEF0(MP_QSTRnull, 0, 0, "")
QDEF0(MP_QSTR_, 5381, 0, "")
QDEF0(MP_QSTR___dir__, 36730, 7, "__dir__")
QDEF0(MP_QSTR__0x0a_, 46511, 1, "\x0a")
QDEF0(MP_QSTR__space_, 46469, 1, " ")
QDEF0(MP_QSTR__star_, 46479, 1, "*")
QDEF0(MP_QSTR__slash_, 46474, 1, "/")
QDEF0(MP_QSTR__lt_module_gt_, 38077, 8, "<module>")
QDEF0(MP_QSTR__, 46586, 1, "_")
QDEF0(MP_QSTR___call__, 63911, 8, "__call__")
QDEF0(MP_QSTR___class__, 50475, 9, "__class__")
QDEF0(MP_QSTR___delitem__, 13821, 11, "__delitem__")
QDEF0(MP_QSTR___enter__, 47725, 9, "__enter__")
QDEF0(MP_QSTR___exit__, 63557, 8, "__exit__")
QDEF0(MP_QSTR___getattr__, 63552, 11, "__getattr__")
QDEF0(MP_QSTR___getitem__, 14630, 11, "__getitem__")
QDEF0(MP_QSTR___hash__, 51447, 8, "__hash__")
QDEF0(MP_QSTR___init__, 42335, 8, "__init__")
QDEF0(MP_QSTR___int__, 6934, 7, "__int__")
QDEF0(MP_QSTR___iter__, 13007, 8, "__iter__")
QDEF0(MP_QSTR___len__, 45282, 7, "__len__")
QDEF0(MP_QSTR___main__, 5006, 8, "__main__")
QDEF0(MP_QSTR___module__, 12543, 10, "__module__")
QDEF0(MP_QSTR___name__, 14562, 8, "__name__")
QDEF0(MP_QSTR___new__, 5497, 7, "__new__")
QDEF0(MP_QSTR___next__, 29442, 8, "__next__")
QDEF0(MP_QSTR___qualname__, 107, 12, "__qualname__")
QDEF0(MP_QSTR___repr__, 2832, 8, "__repr__")
QDEF0(MP_QSTR___setitem__, 15922, 11, "__setitem__")
QDEF0(MP_QSTR___str__, 52688, 7, "__str__")
QDEF0(MP_QSTR_ArithmeticError, 35885, 15, "ArithmeticError")
QDEF0(MP_QSTR_AssertionError, 23191, 14, "AssertionError")
QDEF0(MP_QSTR_AttributeError, 56865, 14, "AttributeError")
QDEF0(MP_QSTR_BaseException, 37383, 13, "BaseException")
QDEF0(MP_QSTR_EOFError, 49041, 8, "EOFError")
QDEF0(MP_QSTR_Ellipsis, 57584, 8, "Ellipsis")
QDEF0(MP_QSTR_Exception, 10738, 9, "Exception")
QDEF0(MP_QSTR_GeneratorExit, 25110, 13, "GeneratorExit")
QDEF0(MP_QSTR_ImportError, 39968, 11, "ImportError")
QDEF0(MP_QSTR_IndentationError, 8284, 16, "IndentationError")
QDEF0(MP_QSTR_IndexError, 44419, 10, "IndexError")
QDEF0(MP_QSTR_KeyError, 234, 8, "KeyError")
QDEF0(MP_QSTR_KeyboardInterrupt, 58031, 17, "KeyboardInterrupt")
QDEF0(MP_QSTR_LookupError, 27135, 11, "LookupError")
QDEF0(MP_QSTR_MemoryError, 33756, 11, "MemoryError")
QDEF0(MP_QSTR_NameError, 11706, 9, "NameError")
QDEF0(MP_QSTR_NoneType, 26647, 8, "NoneType")
QDEF0(MP_QSTR_NotImplementedError, 39110, 19, "NotImplementedError")
QDEF0(MP_QSTR_OSError, 26017, 7, "OSError")
QDEF0(MP_QSTR_OverflowError, 57729, 13, "OverflowError")
QDEF0(MP_QSTR_RuntimeError, 61793, 12, "RuntimeError")
QDEF0(MP_QSTR_StopIteration, 7402, 13, "StopIteration")
QDEF0(MP_QSTR_SyntaxError, 36756, 11, "SyntaxError")
QDEF0(MP_QSTR_SystemExit, 65312, 10, "SystemExit")
QDEF0(MP_QSTR_TypeError, 38437, 9, "TypeError")
QDEF0(MP_QSTR_ValueError, 34710, 10, "ValueError")
QDEF0(MP_QSTR_ZeroDivisionError, 10166, 17, "ZeroDivisionError")
QDEF0(MP_QSTR_abs, 12949, 3, "abs")
QDEF0(MP_QSTR_all, 13124, 3, "all")
QDEF0(MP_QSTR_any, 13075, 3, "any")
QDEF0(MP_QSTR_append, 38763, 6, "append")
QDEF0(MP_QSTR_args, 50882, 4, "args")
QDEF0(MP_QSTR_bool, 15595, 4, "bool")
QDEF0(MP_QSTR_builtins, 12791, 8, "builtins")
QDEF0(MP_QSTR_bytearray, 41846, 9, "bytearray")
QDEF0(MP_QSTR_bytecode, 32034, 8, "bytecode")
QDEF0(MP_QSTR_bytes, 45660, 5, "bytes")
QDEF0(MP_QSTR_callable, 28685, 8, "callable")
QDEF0(MP_QSTR_chr, 19676, 3, "chr")
QDEF0(MP_QSTR_classmethod, 36020, 11, "classmethod")
QDEF0(MP_QSTR_clear, 41084, 5, "clear")
QDEF0(MP_QSTR_close, 26419, 5, "close")
QDEF0(MP_QSTR_const, 65472, 5, "const")
QDEF0(MP_QSTR_copy, 56288, 4, "copy")
QDEF0(MP_QSTR_count, 19878, 5, "count")
QDEF0(MP_QSTR_dict, 64575, 4, "dict")
QDEF0(MP_QSTR_dir, 7930, 3, "dir")
QDEF0(MP_QSTR_divmod, 1208, 6, "divmod")
QDEF0(MP_QSTR_end, 8970, 3, "end")
QDEF0(MP_QSTR_endswith, 41755, 8, "endswith")
QDEF0(MP_QSTR_eval, 42651, 4, "eval")
QDEF0(MP_QSTR_exec, 49182, 4, "exec")
QDEF0(MP_QSTR_extend, 59491, 6, "extend")
QDEF0(MP_QSTR_find, 13312, 4, "find")
QDEF0(MP_QSTR_format, 13094, 6, "format")
QDEF0(MP_QSTR_from_bytes, 29749, 10, "from_bytes")
QDEF0(MP_QSTR_get, 15155, 3, "get")
QDEF0(MP_QSTR_getattr, 6080, 7, "getattr")
QDEF0(MP_QSTR_globals, 18845, 7, "globals")
QDEF0(MP_QSTR_hasattr, 45196, 7, "hasattr")
QDEF0(MP_QSTR_hash, 28855, 4, "hash")
QDEF0(MP_QSTR_id, 28456, 2, "id")
QDEF0(MP_QSTR_index, 10363, 5, "index")
QDEF0(MP_QSTR_insert, 21522, 6, "insert")
QDEF0(MP_QSTR_int, 21270, 3, "int")
QDEF0(MP_QSTR_isalpha, 14315, 7, "isalpha")
QDEF0(MP_QSTR_isdigit, 39592, 7, "isdigit")
QDEF0(MP_QSTR_isinstance, 48822, 10, "isinstance")
QDEF0(MP_QSTR_islower, 33020, 7, "islower")
QDEF0(MP_QSTR_isspace, 63579, 7, "isspace")
QDEF0(MP_QSTR_issubclass, 32693, 10, "issubclass")
QDEF0(MP_QSTR_isupper, 42973, 7, "isupper")
QDEF0(MP_QSTR_items, 21475, 5, "items")
QDEF0(MP_QSTR_iter, 8591, 4, "iter")
QDEF0(MP_QSTR_join, 23719, 4, "join")
QDEF0(MP_QSTR_key, 27954, 3, "key")
QDEF0(MP_QSTR_keys, 4865, 4, "keys")
QDEF0(MP_QSTR_len, 16482, 3, "len")
QDEF0(MP_QSTR_list, 7463, 4, "list")
QDEF0(MP_QSTR_little, 27273, 6, "little")
QDEF0(MP_QSTR_locals, 41275, 6, "locals")
QDEF0(MP_QSTR_lower, 52166, 5, "lower")
QDEF0(MP_QSTR_lstrip, 47589, 6, "lstrip")
QDEF0(MP_QSTR_main, 47054, 4, "main")
QDEF0(MP_QSTR_map, 17337, 3, "map")
QDEF0(MP_QSTR_micropython, 31755, 11, "micropython")
QDEF0(MP_QSTR_next, 34882, 4, "next")
QDEF0(MP_QSTR_object, 36240, 6, "object")
QDEF0(MP_QSTR_open, 15057, 4, "open")
QDEF0(MP_QSTR_ord, 24092, 3, "ord")
QDEF0(MP_QSTR_pop, 29482, 3, "pop")
QDEF0(MP_QSTR_popitem, 11455, 7, "popitem")
QDEF0(MP_QSTR_pow, 29485, 3, "pow")
QDEF0(MP_QSTR_print, 50772, 5, "print")
QDEF0(MP_QSTR_range, 24090, 5, "range")
QDEF0(MP_QSTR_read, 63927, 4, "read")
QDEF0(MP_QSTR_readinto, 48971, 8, "readinto")
QDEF0(MP_QSTR_readline, 6649, 8, "readline")
QDEF0(MP_QSTR_remove, 35427, 6, "remove")
QDEF0(MP_QSTR_replace, 9545, 7, "replace")
QDEF0(MP_QSTR_repr, 63440, 4, "repr")
QDEF0(MP_QSTR_reverse, 10789, 7, "reverse")
QDEF0(MP_QSTR_rfind, 40146, 5, "rfind")
QDEF0(MP_QSTR_rindex, 11241, 6, "rindex")
QDEF0(MP_QSTR_round, 9703, 5, "round")
QDEF0(MP_QSTR_rsplit, 165, 6, "rsplit")
QDEF0(MP_QSTR_rstrip, 38203, 6, "rstrip")
QDEF0(MP_QSTR_self, 30585, 4, "self")
QDEF0(MP_QSTR_send, 30393, 4, "send")
QDEF0(MP_QSTR_sep, 36643, 3, "sep")
QDEF0(MP_QSTR_set, 36647, 3, "set")
QDEF0(MP_QSTR_setattr, 43220, 7, "setattr")
QDEF0(MP_QSTR_setdefault, 41836, 10, "setdefault")
QDEF0(MP_QSTR_sort, 40383, 4, "sort")
QDEF0(MP_QSTR_sorted, 5470, 6, "sorted")
QDEF0(MP_QSTR_split, 13239, 5, "split")
QDEF0(MP_QSTR_start, 61317, 5, "start")
QDEF0(MP_QSTR_startswith, 59508, 10, "startswith")
QDEF0(MP_QSTR_staticmethod, 44898, 12, "staticmethod")
QDEF0(MP_QSTR_step, 13911, 4, "step")
QDEF0(MP_QSTR_stop, 13981, 4, "stop")
QDEF0(MP_QSTR_str, 36176, 3, "str")
QDEF0(MP_QSTR_strip, 7721, 5, "strip")
QDEF0(MP_QSTR_sum, 36142, 3, "sum")
QDEF0(MP_QSTR_super, 45764, 5, "super")
QDEF0(MP_QSTR_throw, 17587, 5, "throw")
QDEF0(MP_QSTR_to_bytes, 16088, 8, "to_bytes")
QDEF0(MP_QSTR_tuple, 16893, 5, "tuple")
QDEF0(MP_QSTR_type, 32669, 4, "type")
QDEF0(MP_QSTR_update, 30388, 6, "update")
QDEF0(MP_QSTR_upper, 37927, 5, "upper")
QDEF0(MP_QSTR_utf_hyphen_8, 33463, 5, "utf-8")
QDEF0(MP_QSTR_value, 13390, 5, "value")
QDEF0(MP_QSTR_values, 48765, 6, "values")
QDEF0(MP_QSTR_write, 43160, 5, "write")
QDEF0(MP_QSTR_zip, 44262, 3, "zip")
QDEF1(MP_QSTR__percent__hash_o, 6764, 3, "%#o")
QDEF1(MP_QSTR__percent__hash_x, 6779, 3, "%#x")
QDEF0(MP_QSTR__lt_dictcomp_gt_, 36300, 10, "<dictcomp>")
QDEF0(MP_QSTR__lt_genexpr_gt_, 27188, 9, "<genexpr>")
QDEF0(MP_QSTR__lt_lambda_gt_, 35968, 8, "<lambda>")
QDEF0(MP_QSTR__lt_listcomp_gt_, 5588, 10, "<listcomp>")
QDEF0(MP_QSTR__lt_setcomp_gt_, 20820, 9, "<setcomp>")
QDEF1(MP_QSTR__lt_stdin_gt_, 25571, 7, "<stdin>")
QDEF1(MP_QSTR__lt_string_gt_, 21330, 8, "<string>")
QDEF1(MP_QSTR_AdmError, 24117, 8, "AdmError")
QDEF1(MP_QSTR_AdmObject, 59672, 9, "AdmObject")
QDEF1(MP_QSTR_BytesIO, 46874, 7, "BytesIO")
QDEF1(MP_QSTR_EACCES, 49719, 6, "EACCES")
QDEF1(MP_QSTR_EADDRINUSE, 4375, 10, "EADDRINUSE")
QDEF1(MP_QSTR_EAGAIN, 60448, 6, "EAGAIN")
QDEF1(MP_QSTR_EALREADY, 5446, 8, "EALREADY")
QDEF1(MP_QSTR_EBADF, 41825, 5, "EBADF")
QDEF1(MP_QSTR_ECONNABORTED, 43815, 12, "ECONNABORTED")
QDEF1(MP_QSTR_ECONNREFUSED, 11322, 12, "ECONNREFUSED")
QDEF1(MP_QSTR_ECONNRESET, 64281, 10, "ECONNRESET")
QDEF1(MP_QSTR_EEXIST, 44371, 6, "EEXIST")
QDEF1(MP_QSTR_EHOSTUNREACH, 9606, 12, "EHOSTUNREACH")
QDEF1(MP_QSTR_EINPROGRESS, 41114, 11, "EINPROGRESS")
QDEF1(MP_QSTR_EINVAL, 65372, 6, "EINVAL")
QDEF1(MP_QSTR_EIO, 42630, 3, "EIO")
QDEF1(MP_QSTR_EISDIR, 20389, 6, "EISDIR")
QDEF1(MP_QSTR_ENOBUFS, 34787, 7, "ENOBUFS")
QDEF1(MP_QSTR_ENODEV, 26550, 6, "ENODEV")
QDEF1(MP_QSTR_ENOENT, 25950, 6, "ENOENT")
QDEF1(MP_QSTR_ENOMEM, 34212, 6, "ENOMEM")
QDEF1(MP_QSTR_ENOTCONN, 55161, 8, "ENOTCONN")
QDEF1(MP_QSTR_EOPNOTSUPP, 38828, 10, "EOPNOTSUPP")
QDEF1(MP_QSTR_EPERM, 32746, 5, "EPERM")
QDEF1(MP_QSTR_ETIMEDOUT, 63743, 9, "ETIMEDOUT")
QDEF1(MP_QSTR_IOBase, 48694, 6, "IOBase")
QDEF1(MP_QSTR_NotImplemented, 50750, 14, "NotImplemented")
QDEF1(MP_QSTR_OrderedDict, 32496, 11, "OrderedDict")
QDEF1(MP_QSTR_StopAsyncIteration, 61676, 18, "StopAsyncIteration")
QDEF1(MP_QSTR_StringIO, 30326, 8, "StringIO")
QDEF1(MP_QSTR_UnicodeError, 53538, 12, "UnicodeError")
QDEF0(MP_QSTR___abs__, 54933, 7, "__abs__")
QDEF0(MP_QSTR___add__, 33476, 7, "__add__")
QDEF1(MP_QSTR___aenter__, 33868, 10, "__aenter__")
QDEF1(MP_QSTR___aexit__, 53188, 9, "__aexit__")
QDEF1(MP_QSTR___aiter__, 11086, 9, "__aiter__")
QDEF1(MP_QSTR___all__, 50116, 7, "__all__")
QDEF0(MP_QSTR___and__, 56078, 7, "__and__")
QDEF1(MP_QSTR___anext__, 46211, 9, "__anext__")
QDEF1(MP_QSTR___bases__, 12291, 9, "__bases__")
QDEF0(MP_QSTR___bool__, 25899, 8, "__bool__")
QDEF1(MP_QSTR___build_class__, 34882, 15, "__build_class__")
QDEF0(MP_QSTR___complex__, 58053, 11, "__complex__")
QDEF0(MP_QSTR___contains__, 24518, 12, "__contains__")
QDEF1(MP_QSTR___del__, 14184, 7, "__del__")
QDEF1(MP_QSTR___delattr__, 30555, 11, "__delattr__")
QDEF0(MP_QSTR___delete__, 60892, 10, "__delete__")
QDEF1(MP_QSTR___dict__, 21631, 8, "__dict__")
QDEF0(MP_QSTR___divmod__, 4472, 10, "__divmod__")
QDEF1(MP_QSTR___doc__, 44077, 7, "__doc__")
QDEF0(MP_QSTR___eq__, 15985, 6, "__eq__")
QDEF1(MP_QSTR___file__, 21507, 8, "__file__")
QDEF0(MP_QSTR___float__, 28725, 9, "__float__")
QDEF0(MP_QSTR___floordiv__, 24390, 12, "__floordiv__")
QDEF0(MP_QSTR___ge__, 18087, 6, "__ge__")
QDEF0(MP_QSTR___get__, 36787, 7, "__get__")
QDEF1(MP_QSTR___globals__, 44701, 11, "__globals__")
QDEF0(MP_QSTR___gt__, 33462, 6, "__gt__")
QDEF0(MP_QSTR___iadd__, 19053, 8, "__iadd__")
QDEF0(MP_QSTR___iand__, 8615, 8, "__iand__")
QDEF0(MP_QSTR___ifloordiv__, 3919, 13, "__ifloordiv__")
QDEF0(MP_QSTR___ilshift__, 41600, 11, "__ilshift__")
QDEF0(MP_QSTR___imatmul__, 28352, 11, "__imatmul__")
QDEF0(MP_QSTR___imod__, 34378, 8, "__imod__")
QDEF1(MP_QSTR___import__, 15928, 10, "__import__")
QDEF0(MP_QSTR___imul__, 52760, 8, "__imul__")
QDEF0(MP_QSTR___invert__, 30711, 10, "__invert__")
QDEF0(MP_QSTR___ior__, 36849, 7, "__ior__")
QDEF0(MP_QSTR___ipow__, 54660, 8, "__ipow__")
QDEF0(MP_QSTR___irshift__, 63710, 11, "__irshift__")
QDEF0(MP_QSTR___isub__, 30728, 8, "__isub__")
QDEF0(MP_QSTR___itruediv__, 545, 12, "__itruediv__")
QDEF0(MP_QSTR___ixor__, 47369, 8, "__ixor__")
QDEF0(MP_QSTR___le__, 5068, 6, "__le__")
QDEF0(MP_QSTR___lshift__, 34825, 10, "__lshift__")
QDEF0(MP_QSTR___lt__, 26717, 6, "__lt__")
QDEF0(MP_QSTR___matmul__, 62281, 10, "__matmul__")
QDEF0(MP_QSTR___mod__, 14179, 7, "__mod__")
QDEF0(MP_QSTR___mul__, 16945, 7, "__mul__")
QDEF0(MP_QSTR___ne__, 2830, 6, "__ne__")
QDEF0(MP_QSTR___neg__, 54633, 7, "__neg__")
QDEF0(MP_QSTR___or__, 47928, 6, "__or__")
QDEF1(MP_QSTR___path__, 9160, 8, "__path__")
QDEF0(MP_QSTR___pos__, 61481, 7, "__pos__")
QDEF0(MP_QSTR___pow__, 45, 7, "__pow__")
QDEF0(MP_QSTR___radd__, 50454, 8, "__radd__")
QDEF0(MP_QSTR___rand__, 11996, 8, "__rand__")
QDEF1(MP_QSTR___repl_print__, 47872, 14, "__repl_print__")
QDEF1(MP_QSTR___reversed__, 65377, 12, "__reversed__")
QDEF0(MP_QSTR___rfloordiv__, 38996, 13, "__rfloordiv__")
QDEF0(MP_QSTR___rlshift__, 53531, 11, "__rlshift__")
QDEF0(MP_QSTR___rmatmul__, 987, 11, "__rmatmul__")
QDEF0(MP_QSTR___rmod__, 35377, 8, "__rmod__")
QDEF0(MP_QSTR___rmul__, 355, 8, "__rmul__")
QDEF0(MP_QSTR___ror__, 56362, 7, "__ror__")
QDEF0(MP_QSTR___rpow__, 39679, 8, "__rpow__")
QDEF0(MP_QSTR___rrshift__, 40389, 11, "__rrshift__")
QDEF0(MP_QSTR___rshift__, 38999, 10, "__rshift__")
QDEF0(MP_QSTR___rsub__, 30579, 8, "__rsub__")
QDEF0(MP_QSTR___rtruediv__, 55130, 12, "__rtruediv__")
QDEF0(MP_QSTR___rxor__, 34546, 8, "__rxor__")
QDEF0(MP_QSTR___set__, 45991, 7, "__set__")
QDEF1(MP_QSTR___set_name__, 21695, 12, "__set_name__")
QDEF1(MP_QSTR___setattr__, 6484, 11, "__setattr__")
QDEF0(MP_QSTR___sizeof__, 15113, 10, "__sizeof__")
QDEF0(MP_QSTR___sub__, 2337, 7, "__sub__")
QDEF1(MP_QSTR___traceback__, 53071, 13, "__traceback__")
QDEF0(MP_QSTR___truediv__, 61320, 11, "__truediv__")
QDEF0(MP_QSTR___xor__, 60448, 7, "__xor__")
QDEF1(MP_QSTR__asdict, 18482, 7, "_asdict")
QDEF1(MP_QSTR__machine, 19391, 8, "_machine")
QDEF1(MP_QSTR__mpy, 10590, 4, "_mpy")
QDEF1(MP_QSTR_a2b_base64, 2876, 10, "a2b_base64")
QDEF1(MP_QSTR_abs_tol, 28029, 7, "abs_tol")
QDEF1(MP_QSTR_acos, 40987, 4, "acos")
QDEF1(MP_QSTR_acosh, 41747, 5, "acosh")
QDEF1(MP_QSTR_add, 12868, 3, "add")
QDEF1(MP_QSTR_appendleft, 7856, 10, "appendleft")
QDEF1(MP_QSTR_array, 29308, 5, "array")
QDEF1(MP_QSTR_ascii, 22356, 5, "ascii")
QDEF1(MP_QSTR_asin, 58704, 4, "asin")
QDEF1(MP_QSTR_asinh, 36664, 5, "asinh")
QDEF1(MP_QSTR_atan, 48671, 4, "atan")
QDEF1(MP_QSTR_atan2, 33229, 5, "atan2")
QDEF1(MP_QSTR_atanh, 33175, 5, "atanh")
QDEF1(MP_QSTR_b2a_base64, 36668, 10, "b2a_base64")
QDEF1(MP_QSTR_big, 18665, 3, "big")
QDEF1(MP_QSTR_bin, 18656, 3, "bin")
QDEF1(MP_QSTR_binascii, 15505, 8, "binascii")
QDEF1(MP_QSTR_bound_method, 41623, 12, "bound_method")
QDEF1(MP_QSTR_byteorder, 39265, 9, "byteorder")
QDEF1(MP_QSTR_calcsize, 14413, 8, "calcsize")
QDEF1(MP_QSTR_ceil, 45062, 4, "ceil")
QDEF1(MP_QSTR_center, 48974, 6, "center")
QDEF1(MP_QSTR_choice, 13102, 6, "choice")
QDEF1(MP_QSTR_closure, 51828, 7, "closure")
QDEF1(MP_QSTR_cmath, 62646, 5, "cmath")
QDEF1(MP_QSTR_code, 55912, 4, "code")
QDEF1(MP_QSTR_collect, 26011, 7, "collect")
QDEF1(MP_QSTR_collections, 51424, 11, "collections")
QDEF1(MP_QSTR_compile, 51700, 7, "compile")
QDEF1(MP_QSTR_complex, 40389, 7, "complex")
QDEF1(MP_QSTR_copysign, 5171, 8, "copysign")
QDEF1(MP_QSTR_cos, 19578, 3, "cos")
QDEF1(MP_QSTR_cosh, 56274, 4, "cosh")
QDEF1(MP_QSTR_decode, 22953, 6, "decode")
QDEF1(MP_QSTR_default, 32206, 7, "default")
QDEF1(MP_QSTR_degrees, 16642, 7, "degrees")
QDEF1(MP_QSTR_delattr, 51419, 7, "delattr")
QDEF1(MP_QSTR_deleter, 56174, 7, "deleter")
QDEF1(MP_QSTR_deque, 39173, 5, "deque")
QDEF1(MP_QSTR_dict_view, 43309, 9, "dict_view")
QDEF1(MP_QSTR_difference, 9330, 10, "difference")
QDEF1(MP_QSTR_difference_update, 64156, 17, "difference_update")
QDEF1(MP_QSTR_disable, 30353, 7, "disable")
QDEF1(MP_QSTR_discard, 28943, 7, "discard")
QDEF1(MP_QSTR_doc, 7981, 3, "doc")
QDEF1(MP_QSTR_dump, 12265, 4, "dump")
QDEF1(MP_QSTR_dumps, 11642, 5, "dumps")
QDEF1(MP_QSTR_e, 46528, 1, "e")
QDEF1(MP_QSTR_enable, 56836, 6, "enable")
QDEF1(MP_QSTR_encode, 51779, 6, "encode")
QDEF1(MP_QSTR_enumerate, 47729, 9, "enumerate")
QDEF1(MP_QSTR_erf, 9108, 3, "erf")
QDEF1(MP_QSTR_erfc, 38519, 4, "erfc")
QDEF1(MP_QSTR_errno, 4545, 5, "errno")
QDEF1(MP_QSTR_errorcode, 56592, 9, "errorcode")
QDEF1(MP_QSTR_exp, 9416, 3, "exp")
QDEF1(MP_QSTR_expm1, 29300, 5, "expm1")
QDEF1(MP_QSTR_fabs, 4755, 4, "fabs")
QDEF1(MP_QSTR_factorial, 13004, 9, "factorial")
QDEF1(MP_QSTR_filter, 48677, 6, "filter")
QDEF1(MP_QSTR_float, 17461, 5, "float")
QDEF1(MP_QSTR_floor, 18045, 5, "floor")
QDEF1(MP_QSTR_flush, 49505, 5, "flush")
QDEF1(MP_QSTR_fmod, 17637, 4, "fmod")
QDEF1(MP_QSTR_frexp, 38940, 5, "frexp")
QDEF1(MP_QSTR_fromhex, 14598, 7, "fromhex")
QDEF1(MP_QSTR_fromkeys, 48439, 8, "fromkeys")
QDEF1(MP_QSTR_frozenset, 40173, 9, "frozenset")
QDEF1(MP_QSTR_function, 551, 8, "function")
QDEF1(MP_QSTR_gamma, 36866, 5, "gamma")
QDEF1(MP_QSTR_gc, 28257, 2, "gc")
QDEF1(MP_QSTR_generator, 50070, 9, "generator")
QDEF1(MP_QSTR_getrandbits, 32102, 11, "getrandbits")
QDEF1(MP_QSTR_getsizeof, 9215, 9, "getsizeof")
QDEF1(MP_QSTR_getter, 45712, 6, "getter")
QDEF1(MP_QSTR_getvalue, 44152, 8, "getvalue")
QDEF1(MP_QSTR_group, 45242, 5, "group")
QDEF1(MP_QSTR_groups, 51081, 6, "groups")
QDEF1(MP_QSTR_heap_lock, 36013, 9, "heap_lock")
QDEF1(MP_QSTR_heap_unlock, 11606, 11, "heap_unlock")
QDEF1(MP_QSTR_heapify, 11695, 7, "heapify")
QDEF1(MP_QSTR_heappop, 10198, 7, "heappop")
QDEF1(MP_QSTR_heappush, 27527, 8, "heappush")
QDEF1(MP_QSTR_heapq, 7528, 5, "heapq")
QDEF1(MP_QSTR_hex, 20592, 3, "hex")
QDEF1(MP_QSTR_hexlify, 32554, 7, "hexlify")
QDEF1(MP_QSTR_ignore, 6717, 6, "ignore")
QDEF1(MP_QSTR_imag, 46919, 4, "imag")
QDEF1(MP_QSTR_implementation, 11543, 14, "implementation")
QDEF1(MP_QSTR_indices, 18522, 7, "indices")
QDEF1(MP_QSTR_inf, 21252, 3, "inf")
QDEF1(MP_QSTR_intersection, 10792, 12, "intersection")
QDEF1(MP_QSTR_intersection_update, 56582, 19, "intersection_update")
QDEF1(MP_QSTR_io, 28451, 2, "io")
QDEF1(MP_QSTR_ioctl, 49784, 5, "ioctl")
QDEF1(MP_QSTR_isclose, 20041, 7, "isclose")
QDEF1(MP_QSTR_isdisjoint, 26871, 10, "isdisjoint")
QDEF1(MP_QSTR_isenabled, 58778, 9, "isenabled")
QDEF1(MP_QSTR_isfinite, 43942, 8, "isfinite")
QDEF1(MP_QSTR_isinf, 4414, 5, "isinf")
QDEF1(MP_QSTR_isnan, 926, 5, "isnan")
QDEF1(MP_QSTR_issubset, 49593, 8, "issubset")
QDEF1(MP_QSTR_issuperset, 60668, 10, "issuperset")
QDEF1(MP_QSTR_itemsize, 30581, 8, "itemsize")
QDEF1(MP_QSTR_iterable, 37413, 8, "iterable")
QDEF1(MP_QSTR_iterator, 48711, 8, "iterator")
QDEF1(MP_QSTR_json, 53757, 4, "json")
QDEF1(MP_QSTR_kbd_intr, 5110, 8, "kbd_intr")
QDEF1(MP_QSTR_keepends, 35682, 8, "keepends")
QDEF1(MP_QSTR_ldexp, 28480, 5, "ldexp")
QDEF1(MP_QSTR_length, 34649, 6, "length")
QDEF1(MP_QSTR_lgamma, 27854, 6, "lgamma")
QDEF1(MP_QSTR_load, 9315, 4, "load")
QDEF1(MP_QSTR_loads, 45232, 5, "loads")
QDEF1(MP_QSTR_log, 16161, 3, "log")
QDEF1(MP_QSTR_log10, 37184, 5, "log10")
QDEF1(MP_QSTR_log2, 9075, 4, "log2")
QDEF1(MP_QSTR_match, 8854, 5, "match")
QDEF1(MP_QSTR_math, 47925, 4, "math")
QDEF1(MP_QSTR_max, 17329, 3, "max")
QDEF1(MP_QSTR_maximum_space_recursion_space_depth_space_exceeded, 7795, 32, "maximum recursion depth exceeded")
QDEF1(MP_QSTR_maxsize, 28884, 7, "maxsize")
QDEF1(MP_QSTR_mem_alloc, 11090, 9, "mem_alloc")
QDEF1(MP_QSTR_mem_free, 25291, 8, "mem_free")
QDEF1(MP_QSTR_memoryview, 17513, 10, "memoryview")
QDEF1(MP_QSTR_min, 17071, 3, "min")
QDEF1(MP_QSTR_modf, 49189, 4, "modf")
QDEF1(MP_QSTR_module, 39359, 6, "module")
QDEF1(MP_QSTR_modules, 53740, 7, "modules")
QDEF1(MP_QSTR_name, 30114, 4, "name")
QDEF1(MP_QSTR_namedtuple, 5662, 10, "namedtuple")
QDEF1(MP_QSTR_nan, 22756, 3, "nan")
QDEF1(MP_QSTR_newline, 35127, 7, "newline")
QDEF1(MP_QSTR_oct, 23805, 3, "oct")
QDEF1(MP_QSTR_opt_level, 26503, 9, "opt_level")
QDEF1(MP_QSTR_pack, 53692, 4, "pack")
QDEF1(MP_QSTR_pack_into, 43295, 9, "pack_into")
QDEF1(MP_QSTR_partition, 58759, 9, "partition")
QDEF1(MP_QSTR_pend_throw, 29939, 10, "pend_throw")
QDEF1(MP_QSTR_phase, 54634, 5, "phase")
QDEF1(MP_QSTR_pi, 28700, 2, "pi")
QDEF1(MP_QSTR_platform, 6458, 8, "platform")
QDEF1(MP_QSTR_polar, 3077, 5, "polar")
QDEF1(MP_QSTR_popleft, 39537, 7, "popleft")
QDEF1(MP_QSTR_print_exception, 8732, 15, "print_exception")
QDEF1(MP_QSTR_property, 10690, 8, "property")
QDEF1(MP_QSTR_radians, 16263, 7, "radians")
QDEF1(MP_QSTR_randint, 56495, 7, "randint")
QDEF1(MP_QSTR_random, 11454, 6, "random")
QDEF1(MP_QSTR_randrange, 16035, 9, "randrange")
QDEF1(MP_QSTR_re, 28882, 2, "re")
QDEF1(MP_QSTR_real, 63935, 4, "real")
QDEF1(MP_QSTR_rect, 63973, 4, "rect")
QDEF1(MP_QSTR_rel_tol, 18006, 7, "rel_tol")
QDEF1(MP_QSTR_reversed, 28321, 8, "reversed")
QDEF1(MP_QSTR_rpartition, 53269, 10, "rpartition")
QDEF1(MP_QSTR_search, 49579, 6, "search")
QDEF1(MP_QSTR_seed, 30098, 4, "seed")
QDEF1(MP_QSTR_seek, 30109, 4, "seek")
QDEF1(MP_QSTR_separators, 3307, 10, "separators")
QDEF1(MP_QSTR_setter, 22788, 6, "setter")
QDEF1(MP_QSTR_signed, 311, 6, "signed")
QDEF1(MP_QSTR_sin, 37041, 3, "sin")
QDEF1(MP_QSTR_single, 8255, 6, "single")
QDEF1(MP_QSTR_sinh, 42681, 4, "sinh")
QDEF1(MP_QSTR_slice, 62645, 5, "slice")
QDEF1(MP_QSTR_span, 18121, 4, "span")
QDEF1(MP_QSTR_splitlines, 54122, 10, "splitlines")
QDEF1(MP_QSTR_sqrt, 17441, 4, "sqrt")
QDEF1(MP_QSTR_struct, 36882, 6, "struct")
QDEF1(MP_QSTR_sub, 36129, 3, "sub")
QDEF1(MP_QSTR_symmetric_difference, 26574, 20, "symmetric_difference")
QDEF1(MP_QSTR_symmetric_difference_update, 63584, 27, "symmetric_difference_update")
QDEF1(MP_QSTR_sys, 36540, 3, "sys")
QDEF1(MP_QSTR_tan, 25086, 3, "tan")
QDEF1(MP_QSTR_tanh, 41430, 4, "tanh")
QDEF1(MP_QSTR_tau, 25061, 3, "tau")
QDEF1(MP_QSTR_tell, 45332, 4, "tell")
QDEF1(MP_QSTR_threshold, 12274, 9, "threshold")
QDEF1(MP_QSTR_trunc, 39259, 5, "trunc")
QDEF1(MP_QSTR_unhexlify, 47537, 9, "unhexlify")
QDEF1(MP_QSTR_uniform, 62721, 7, "uniform")
QDEF1(MP_QSTR_union, 31990, 5, "union")
QDEF1(MP_QSTR_unpack, 15367, 6, "unpack")
QDEF1(MP_QSTR_unpack_from, 27918, 11, "unpack_from")
QDEF1(MP_QSTR_usys, 62409, 4, "usys")
QDEF1(MP_QSTR_utf8, 60602, 4, "utf8")
QDEF1(MP_QSTR_version, 54207, 7, "version")
QDEF1(MP_QSTR_version_info, 2670, 12, "version_info")
QDEF1(MP_QSTR__brace_open__colon__hash_b_brace_close_, 14168, 5, "{:#b}")
#undef QDEF0
#undef QDEF1
    #endif
    MP_QSTRnumber_of_static,
    MP_QSTRstart_of_main = MP_QSTRnumber_of_static - 1, // unused but shifts the enum counter back one

    #ifndef NO_QSTR
#define QDEF0(id, hash, len, str)
#define QDEF1(id, hash, len, str) id,
/* qstrdefs.generated.h */
// This file was automatically generated by makeqstrdata.py

QDEF0(MP_QSTRnull, 0, 0, "")
QDEF0(MP_QSTR_, 5381, 0, "")
QDEF0(MP_QSTR___dir__, 36730, 7, "__dir__")
QDEF0(MP_QSTR__0x0a_, 46511, 1, "\x0a")
QDEF0(MP_QSTR__space_, 46469, 1, " ")
QDEF0(MP_QSTR__star_, 46479, 1, "*")
QDEF0(MP_QSTR__slash_, 46474, 1, "/")
QDEF0(MP_QSTR__lt_module_gt_, 38077, 8, "<module>")
QDEF0(MP_QSTR__, 46586, 1, "_")
QDEF0(MP_QSTR___call__, 63911, 8, "__call__")
QDEF0(MP_QSTR___class__, 50475, 9, "__class__")
QDEF0(MP_QSTR___delitem__, 13821, 11, "__delitem__")
QDEF0(MP_QSTR___enter__, 47725, 9, "__enter__")
QDEF0(MP_QSTR___exit__, 63557, 8, "__exit__")
QDEF0(MP_QSTR___getattr__, 63552, 11, "__getattr__")
QDEF0(MP_QSTR___getitem__, 14630, 11, "__getitem__")
QDEF0(MP_QSTR___hash__, 51447, 8, "__hash__")
QDEF0(MP_QSTR___init__, 42335, 8, "__init__")
QDEF0(MP_QSTR___int__, 6934, 7, "__int__")
QDEF0(MP_QSTR___iter__, 13007, 8, "__iter__")
QDEF0(MP_QSTR___len__, 45282, 7, "__len__")
QDEF0(MP_QSTR___main__, 5006, 8, "__main__")
QDEF0(MP_QSTR___module__, 12543, 10, "__module__")
QDEF0(MP_QSTR___name__, 14562, 8, "__name__")
QDEF0(MP_QSTR___new__, 5497, 7, "__new__")
QDEF0(MP_QSTR___next__, 29442, 8, "__next__")
QDEF0(MP_QSTR___qualname__, 107, 12, "__qualname__")
QDEF0(MP_QSTR___repr__, 2832, 8, "__repr__")
QDEF0(MP_QSTR___setitem__, 15922, 11, "__setitem__")
QDEF0(MP_QSTR___str__, 52688, 7, "__str__")
QDEF0(MP_QSTR_ArithmeticError, 35885, 15, "ArithmeticError")
QDEF0(MP_QSTR_AssertionError, 23191, 14, "AssertionError")
QDEF0(MP_QSTR_AttributeError, 56865, 14, "AttributeError")
QDEF0(MP_QSTR_BaseException, 37383, 13, "BaseException")
QDEF0(MP_QSTR_EOFError, 49041, 8, "EOFError")
QDEF0(MP_QSTR_Ellipsis, 57584, 8, "Ellipsis")
QDEF0(MP_QSTR_Exception, 10738, 9, "Exception")
QDEF0(MP_QSTR_GeneratorExit, 25110, 13, "GeneratorExit")
QDEF0(MP_QSTR_ImportError, 39968, 11, "ImportError")
QDEF0(MP_QSTR_IndentationError, 8284, 16, "IndentationError")
QDEF0(MP_QSTR_IndexError, 44419, 10, "IndexError")
QDEF0(MP_QSTR_KeyError, 234, 8, "KeyError")
QDEF0(MP_QSTR_KeyboardInterrupt, 58031, 17, "KeyboardInterrupt")
QDEF0(MP_QSTR_LookupError, 27135, 11, "LookupError")
QDEF0(MP_QSTR_MemoryError, 33756, 11, "MemoryError")
QDEF0(MP_QSTR_NameError, 11706, 9, "NameError")
QDEF0(MP_QSTR_NoneType, 26647, 8, "NoneType")
QDEF0(MP_QSTR_NotImplementedError, 39110, 19, "NotImplementedError")
QDEF0(MP_QSTR_OSError, 26017, 7, "OSError")
QDEF0(MP_QSTR_OverflowError, 57729, 13, "OverflowError")
QDEF0(MP_QSTR_RuntimeError, 61793, 12, "RuntimeError")
QDEF0(MP_QSTR_StopIteration, 7402, 13, "StopIteration")
QDEF0(MP_QSTR_SyntaxError, 36756, 11, "SyntaxError")
QDEF0(MP_QSTR_SystemExit, 65312, 10, "SystemExit")
QDEF0(MP_QSTR_TypeError, 38437, 9, "TypeError")
QDEF0(MP_QSTR_ValueError, 34710, 10, "ValueError")
QDEF0(MP_QSTR_ZeroDivisionError, 10166, 17, "ZeroDivisionError")
QDEF0(MP_QSTR_abs, 12949, 3, "abs")
QDEF0(MP_QSTR_all, 13124, 3, "all")
QDEF0(MP_QSTR_any, 13075, 3, "any")
QDEF0(MP_QSTR_append, 38763, 6, "append")
QDEF0(MP_QSTR_args, 50882, 4, "args")
QDEF0(MP_QSTR_bool, 15595, 4, "bool")
QDEF0(MP_QSTR_builtins, 12791, 8, "builtins")
QDEF0(MP_QSTR_bytearray, 41846, 9, "bytearray")
QDEF0(MP_QSTR_bytecode, 32034, 8, "bytecode")
QDEF0(MP_QSTR_bytes, 45660, 5, "bytes")
QDEF0(MP_QSTR_callable, 28685, 8, "callable")
QDEF0(MP_QSTR_chr, 19676, 3, "chr")
QDEF0(MP_QSTR_classmethod, 36020, 11, "classmethod")
QDEF0(MP_QSTR_clear, 41084, 5, "clear")
QDEF0(MP_QSTR_close, 26419, 5, "close")
QDEF0(MP_QSTR_const, 65472, 5, "const")
QDEF0(MP_QSTR_copy, 56288, 4, "copy")
QDEF0(MP_QSTR_count, 19878, 5, "count")
QDEF0(MP_QSTR_dict, 64575, 4, "dict")
QDEF0(MP_QSTR_dir, 7930, 3, "dir")
QDEF0(MP_QSTR_divmod, 1208, 6, "divmod")
QDEF0(MP_QSTR_end, 8970, 3, "end")
QDEF0(MP_QSTR_endswith, 41755, 8, "endswith")
QDEF0(MP_QSTR_eval, 42651, 4, "eval")
QDEF0(MP_QSTR_exec, 49182, 4, "exec")
QDEF0(MP_QSTR_extend, 59491, 6, "extend")
QDEF0(MP_QSTR_find, 13312, 4, "find")
QDEF0(MP_QSTR_format, 13094, 6, "format")
QDEF0(MP_QSTR_from_bytes, 29749, 10, "from_bytes")
QDEF0(MP_QSTR_get, 15155, 3, "get")
QDEF0(MP_QSTR_getattr, 6080, 7, "getattr")
QDEF0(MP_QSTR_globals, 18845, 7, "globals")
QDEF0(MP_QSTR_hasattr, 45196, 7, "hasattr")
QDEF0(MP_QSTR_hash, 28855, 4, "hash")
QDEF0(MP_QSTR_id, 28456, 2, "id")
QDEF0(MP_QSTR_index, 10363, 5, "index")
QDEF0(MP_QSTR_insert, 21522, 6, "insert")
QDEF0(MP_QSTR_int, 21270, 3, "int")
QDEF0(MP_QSTR_isalpha, 14315, 7, "isalpha")
QDEF0(MP_QSTR_isdigit, 39592, 7, "isdigit")
QDEF0(MP_QSTR_isinstance, 48822, 10, "isinstance")
QDEF0(MP_QSTR_islower, 33020, 7, "islower")
QDEF0(MP_QSTR_isspace, 63579, 7, "isspace")
QDEF0(MP_QSTR_issubclass, 32693, 10, "issubclass")
QDEF0(MP_QSTR_isupper, 42973, 7, "isupper")
QDEF0(MP_QSTR_items, 21475, 5, "items")
QDEF0(MP_QSTR_iter, 8591, 4, "iter")
QDEF0(MP_QSTR_join, 23719, 4, "join")
QDEF0(MP_QSTR_key, 27954, 3, "key")
QDEF0(MP_QSTR_keys, 4865, 4, "keys")
QDEF0(MP_QSTR_len, 16482, 3, "len")
QDEF0(MP_QSTR_list, 7463, 4, "list")
QDEF0(MP_QSTR_little, 27273, 6, "little")
QDEF0(MP_QSTR_locals, 41275, 6, "locals")
QDEF0(MP_QSTR_lower, 52166, 5, "lower")
QDEF0(MP_QSTR_lstrip, 47589, 6, "lstrip")
QDEF0(MP_QSTR_main, 47054, 4, "main")
QDEF0(MP_QSTR_map, 17337, 3, "map")
QDEF0(MP_QSTR_micropython, 31755, 11, "micropython")
QDEF0(MP_QSTR_next, 34882, 4, "next")
QDEF0(MP_QSTR_object, 36240, 6, "object")
QDEF0(MP_QSTR_open, 15057, 4, "open")
QDEF0(MP_QSTR_ord, 24092, 3, "ord")
QDEF0(MP_QSTR_pop, 29482, 3, "pop")
QDEF0(MP_QSTR_popitem, 11455, 7, "popitem")
QDEF0(MP_QSTR_pow, 29485, 3, "pow")
QDEF0(MP_QSTR_print, 50772, 5, "print")
QDEF0(MP_QSTR_range, 24090, 5, "range")
QDEF0(MP_QSTR_read, 63927, 4, "read")
QDEF0(MP_QSTR_readinto, 48971, 8, "readinto")
QDEF0(MP_QSTR_readline, 6649, 8, "readline")
QDEF0(MP_QSTR_remove, 35427, 6, "remove")
QDEF0(MP_QSTR_replace, 9545, 7, "replace")
QDEF0(MP_QSTR_repr, 63440, 4, "repr")
QDEF0(MP_QSTR_reverse, 10789, 7, "reverse")
QDEF0(MP_QSTR_rfind, 40146, 5, "rfind")
QDEF0(MP_QSTR_rindex, 11241, 6, "rindex")
QDEF0(MP_QSTR_round, 9703, 5, "round")
QDEF0(MP_QSTR_rsplit, 165, 6, "rsplit")
QDEF0(MP_QSTR_rstrip, 38203, 6, "rstrip")
QDEF0(MP_QSTR_self, 30585, 4, "self")
QDEF0(MP_QSTR_send, 30393, 4, "send")
QDEF0(MP_QSTR_sep, 36643, 3, "sep")
QDEF0(MP_QSTR_set, 36647, 3, "set")
QDEF0(MP_QSTR_setattr, 43220, 7, "setattr")
QDEF0(MP_QSTR_setdefault, 41836, 10, "setdefault")
QDEF0(MP_QSTR_sort, 40383, 4, "sort")
QDEF0(MP_QSTR_sorted, 5470, 6, "sorted")
QDEF0(MP_QSTR_split, 13239, 5, "split")
QDEF0(MP_QSTR_start, 61317, 5, "start")
QDEF0(MP_QSTR_startswith, 59508, 10, "startswith")
QDEF0(MP_QSTR_staticmethod, 44898, 12, "staticmethod")
QDEF0(MP_QSTR_step, 13911, 4, "step")
QDEF0(MP_QSTR_stop, 13981, 4, "stop")
QDEF0(MP_QSTR_str, 36176, 3, "str")
QDEF0(MP_QSTR_strip, 7721, 5, "strip")
QDEF0(MP_QSTR_sum, 36142, 3, "sum")
QDEF0(MP_QSTR_super, 45764, 5, "super")
QDEF0(MP_QSTR_throw, 17587, 5, "throw")
QDEF0(MP_QSTR_to_bytes, 16088, 8, "to_bytes")
QDEF0(MP_QSTR_tuple, 16893, 5, "tuple")
QDEF0(MP_QSTR_type, 32669, 4, "type")
QDEF0(MP_QSTR_update, 30388, 6, "update")
QDEF0(MP_QSTR_upper, 37927, 5, "upper")
QDEF0(MP_QSTR_utf_hyphen_8, 33463, 5, "utf-8")
QDEF0(MP_QSTR_value, 13390, 5, "value")
QDEF0(MP_QSTR_values, 48765, 6, "values")
QDEF0(MP_QSTR_write, 43160, 5, "write")
QDEF0(MP_QSTR_zip, 44262, 3, "zip")
QDEF1(MP_QSTR__percent__hash_o, 6764, 3, "%#o")
QDEF1(MP_QSTR__percent__hash_x, 6779, 3, "%#x")
QDEF0(MP_QSTR__lt_dictcomp_gt_, 36300, 10, "<dictcomp>")
QDEF0(MP_QSTR__lt_genexpr_gt_, 27188, 9, "<genexpr>")
QDEF0(MP_QSTR__lt_lambda_gt_, 35968, 8, "<lambda>")
QDEF0(MP_QSTR__lt_listcomp_gt_, 5588, 10, "<listcomp>")
QDEF0(MP_QSTR__lt_setcomp_gt_, 20820, 9, "<setcomp>")
QDEF1(MP_QSTR__lt_stdin_gt_, 25571, 7, "<stdin>")
QDEF1(MP_QSTR__lt_string_gt_, 21330, 8, "<string>")
QDEF1(MP_QSTR_AdmError, 24117, 8, "AdmError")
QDEF1(MP_QSTR_AdmObject, 59672, 9, "AdmObject")
QDEF1(MP_QSTR_BytesIO, 46874, 7, "BytesIO")
QDEF1(MP_QSTR_EACCES, 49719, 6, "EACCES")
QDEF1(MP_QSTR_EADDRINUSE, 4375, 10, "EADDRINUSE")
QDEF1(MP_QSTR_EAGAIN, 60448, 6, "EAGAIN")
QDEF1(MP_QSTR_EALREADY, 5446, 8, "EALREADY")
QDEF1(MP_QSTR_EBADF, 41825, 5, "EBADF")
QDEF1(MP_QSTR_ECONNABORTED, 43815, 12, "ECONNABORTED")
QDEF1(MP_QSTR_ECONNREFUSED, 11322, 12, "ECONNREFUSED")
QDEF1(MP_QSTR_ECONNRESET, 64281, 10, "ECONNRESET")
QDEF1(MP_QSTR_EEXIST, 44371, 6, "EEXIST")
QDEF1(MP_QSTR_EHOSTUNREACH, 9606, 12, "EHOSTUNREACH")
QDEF1(MP_QSTR_EINPROGRESS, 41114, 11, "EINPROGRESS")
QDEF1(MP_QSTR_EINVAL, 65372, 6, "EINVAL")
QDEF1(MP_QSTR_EIO, 42630, 3, "EIO")
QDEF1(MP_QSTR_EISDIR, 20389, 6, "EISDIR")
QDEF1(MP_QSTR_ENOBUFS, 34787, 7, "ENOBUFS")
QDEF1(MP_QSTR_ENODEV, 26550, 6, "ENODEV")
QDEF1(MP_QSTR_ENOENT, 25950, 6, "ENOENT")
QDEF1(MP_QSTR_ENOMEM, 34212, 6, "ENOMEM")
QDEF1(MP_QSTR_ENOTCONN, 55161, 8, "ENOTCONN")
QDEF1(MP_QSTR_EOPNOTSUPP, 38828, 10, "EOPNOTSUPP")
QDEF1(MP_QSTR_EPERM, 32746, 5, "EPERM")
QDEF1(MP_QSTR_ETIMEDOUT, 63743, 9, "ETIMEDOUT")
QDEF1(MP_QSTR_IOBase, 48694, 6, "IOBase")
QDEF1(MP_QSTR_NotImplemented, 50750, 14, "NotImplemented")
QDEF1(MP_QSTR_OrderedDict, 32496, 11, "OrderedDict")
QDEF1(MP_QSTR_StopAsyncIteration, 61676, 18, "StopAsyncIteration")
QDEF1(MP_QSTR_StringIO, 30326, 8, "StringIO")
QDEF1(MP_QSTR_UnicodeError, 53538, 12, "UnicodeError")
QDEF0(MP_QSTR___abs__, 54933, 7, "__abs__")
QDEF0(MP_QSTR___add__, 33476, 7, "__add__")
QDEF1(MP_QSTR___aenter__, 33868, 10, "__aenter__")
QDEF1(MP_QSTR___aexit__, 53188, 9, "__aexit__")
QDEF1(MP_QSTR___aiter__, 11086, 9, "__aiter__")
QDEF1(MP_QSTR___all__, 50116, 7, "__all__")
QDEF0(MP_QSTR___and__, 56078, 7, "__and__")
QDEF1(MP_QSTR___anext__, 46211, 9, "__anext__")
QDEF1(MP_QSTR___bases__, 12291, 9, "__bases__")
QDEF0(MP_QSTR___bool__, 25899, 8, "__bool__")
QDEF1(MP_QSTR___build_class__, 34882, 15, "__build_class__")
QDEF0(MP_QSTR___complex__, 58053, 11, "__complex__")
QDEF0(MP_QSTR___contains__, 24518, 12, "__contains__")
QDEF1(MP_QSTR___del__, 14184, 7, "__del__")
QDEF1(MP_QSTR___delattr__, 30555, 11, "__delattr__")
QDEF0(MP_QSTR___delete__, 60892, 10, "__delete__")
QDEF1(MP_QSTR___dict__, 21631, 8, "__dict__")
QDEF0(MP_QSTR___divmod__, 4472, 10, "__divmod__")
QDEF1(MP_QSTR___doc__, 44077, 7, "__doc__")
QDEF0(MP_QSTR___eq__, 15985, 6, "__eq__")
QDEF1(MP_QSTR___file__, 21507, 8, "__file__")
QDEF0(MP_QSTR___float__, 28725, 9, "__float__")
QDEF0(MP_QSTR___floordiv__, 24390, 12, "__floordiv__")
QDEF0(MP_QSTR___ge__, 18087, 6, "__ge__")
QDEF0(MP_QSTR___get__, 36787, 7, "__get__")
QDEF1(MP_QSTR___globals__, 44701, 11, "__globals__")
QDEF0(MP_QSTR___gt__, 33462, 6, "__gt__")
QDEF0(MP_QSTR___iadd__, 19053, 8, "__iadd__")
QDEF0(MP_QSTR___iand__, 8615, 8, "__iand__")
QDEF0(MP_QSTR___ifloordiv__, 3919, 13, "__ifloordiv__")
QDEF0(MP_QSTR___ilshift__, 41600, 11, "__ilshift__")
QDEF0(MP_QSTR___imatmul__, 28352, 11, "__imatmul__")
QDEF0(MP_QSTR___imod__, 34378, 8, "__imod__")
QDEF1(MP_QSTR___import__, 15928, 10, "__import__")
QDEF0(MP_QSTR___imul__, 52760, 8, "__imul__")
QDEF0(MP_QSTR___invert__, 30711, 10, "__invert__")
QDEF0(MP_QSTR___ior__, 36849, 7, "__ior__")
QDEF0(MP_QSTR___ipow__, 54660, 8, "__ipow__")
QDEF0(MP_QSTR___irshift__, 63710, 11, "__irshift__")
QDEF0(MP_QSTR___isub__, 30728, 8, "__isub__")
QDEF0(MP_QSTR___itruediv__, 545, 12, "__itruediv__")
QDEF0(MP_QSTR___ixor__, 47369, 8, "__ixor__")
QDEF0(MP_QSTR___le__, 5068, 6, "__le__")
QDEF0(MP_QSTR___lshift__, 34825, 10, "__lshift__")
QDEF0(MP_QSTR___lt__, 26717, 6, "__lt__")
QDEF0(MP_QSTR___matmul__, 62281, 10, "__matmul__")
QDEF0(MP_QSTR___mod__, 14179, 7, "__mod__")
QDEF0(MP_QSTR___mul__, 16945, 7, "__mul__")
QDEF0(MP_QSTR___ne__, 2830, 6, "__ne__")
QDEF0(MP_QSTR___neg__, 54633, 7, "__neg__")
QDEF0(MP_QSTR___or__, 47928, 6, "__or__")
QDEF1(MP_QSTR___path__, 9160, 8, "__path__")
QDEF0(MP_QSTR___pos__, 61481, 7, "__pos__")
QDEF0(MP_QSTR___pow__, 45, 7, "__pow__")
QDEF0(MP_QSTR___radd__, 50454, 8, "__radd__")
QDEF0(MP_QSTR___rand__, 11996, 8, "__rand__")
QDEF1(MP_QSTR___repl_print__, 47872, 14, "__repl_print__")
QDEF1(MP_QSTR___reversed__, 65377, 12, "__reversed__")
QDEF0(MP_QSTR___rfloordiv__, 38996, 13, "__rfloordiv__")
QDEF0(MP_QSTR___rlshift__, 53531, 11, "__rlshift__")
QDEF0(MP_QSTR___rmatmul__, 987, 11, "__rmatmul__")
QDEF0(MP_QSTR___rmod__, 35377, 8, "__rmod__")
QDEF0(MP_QSTR___rmul__, 355, 8, "__rmul__")
QDEF0(MP_QSTR___ror__, 56362, 7, "__ror__")
QDEF0(MP_QSTR___rpow__, 39679, 8, "__rpow__")
QDEF0(MP_QSTR___rrshift__, 40389, 11, "__rrshift__")
QDEF0(MP_QSTR___rshift__, 38999, 10, "__rshift__")
QDEF0(MP_QSTR___rsub__, 30579, 8, "__rsub__")
QDEF0(MP_QSTR___rtruediv__, 55130, 12, "__rtruediv__")
QDEF0(MP_QSTR___rxor__, 34546, 8, "__rxor__")
QDEF0(MP_QSTR___set__, 45991, 7, "__set__")
QDEF1(MP_QSTR___set_name__, 21695, 12, "__set_name__")
QDEF1(MP_QSTR___setattr__, 6484, 11, "__setattr__")
QDEF0(MP_QSTR___sizeof__, 15113, 10, "__sizeof__")
QDEF0(MP_QSTR___sub__, 2337, 7, "__sub__")
QDEF1(MP_QSTR___traceback__, 53071, 13, "__traceback__")
QDEF0(MP_QSTR___truediv__, 61320, 11, "__truediv__")
QDEF0(MP_QSTR___xor__, 60448, 7, "__xor__")
QDEF1(MP_QSTR__asdict, 18482, 7, "_asdict")
QDEF1(MP_QSTR__machine, 19391, 8, "_machine")
QDEF1(MP_QSTR__mpy, 10590, 4, "_mpy")
QDEF1(MP_QSTR_a2b_base64, 2876, 10, "a2b_base64")
QDEF1(MP_QSTR_abs_tol, 28029, 7, "abs_tol")
QDEF1(MP_QSTR_acos, 40987, 4, "acos")
QDEF1(MP_QSTR_acosh, 41747, 5, "acosh")
QDEF1(MP_QSTR_add, 12868, 3, "add")
QDEF1(MP_QSTR_appendleft, 7856, 10, "appendleft")
QDEF1(MP_QSTR_array, 29308, 5, "array")
QDEF1(MP_QSTR_ascii, 22356, 5, "ascii")
QDEF1(MP_QSTR_asin, 58704, 4, "asin")
QDEF1(MP_QSTR_asinh, 36664, 5, "asinh")
QDEF1(MP_QSTR_atan, 48671, 4, "atan")
QDEF1(MP_QSTR_atan2, 33229, 5, "atan2")
QDEF1(MP_QSTR_atanh, 33175, 5, "atanh")
QDEF1(MP_QSTR_b2a_base64, 36668, 10, "b2a_base64")
QDEF1(MP_QSTR_big, 18665, 3, "big")
QDEF1(MP_QSTR_bin, 18656, 3, "bin")
QDEF1(MP_QSTR_binascii, 15505, 8, "binascii")
QDEF1(MP_QSTR_bound_method, 41623, 12, "bound_method")
QDEF1(MP_QSTR_byteorder, 39265, 9, "byteorder")
QDEF1(MP_QSTR_calcsize, 14413, 8, "calcsize")
QDEF1(MP_QSTR_ceil, 45062, 4, "ceil")
QDEF1(MP_QSTR_center, 48974, 6, "center")
QDEF1(MP_QSTR_choice, 13102, 6, "choice")
QDEF1(MP_QSTR_closure, 51828, 7, "closure")
QDEF1(MP_QSTR_cmath, 62646, 5, "cmath")
QDEF1(MP_QSTR_code, 55912, 4, "code")
QDEF1(MP_QSTR_collect, 26011, 7, "collect")
QDEF1(MP_QSTR_collections, 51424, 11, "collections")
QDEF1(MP_QSTR_compile, 51700, 7, "compile")
QDEF1(MP_QSTR_complex, 40389, 7, "complex")
QDEF1(MP_QSTR_copysign, 5171, 8, "copysign")
QDEF1(MP_QSTR_cos, 19578, 3, "cos")
QDEF1(MP_QSTR_cosh, 56274, 4, "cosh")
QDEF1(MP_QSTR_decode, 22953, 6, "decode")
QDEF1(MP_QSTR_default, 32206, 7, "default")
QDEF1(MP_QSTR_degrees, 16642, 7, "degrees")
QDEF1(MP_QSTR_delattr, 51419, 7, "delattr")
QDEF1(MP_QSTR_deleter, 56174, 7, "deleter")
QDEF1(MP_QSTR_deque, 39173, 5, "deque")
QDEF1(MP_QSTR_dict_view, 43309, 9, "dict_view")
QDEF1(MP_QSTR_difference, 9330, 10, "difference")
QDEF1(MP_QSTR_difference_update, 64156, 17, "difference_update")
QDEF1(MP_QSTR_disable, 30353, 7, "disable")
QDEF1(MP_QSTR_discard, 28943, 7, "discard")
QDEF1(MP_QSTR_doc, 7981, 3, "doc")
QDEF1(MP_QSTR_dump, 12265, 4, "dump")
QDEF1(MP_QSTR_dumps, 11642, 5, "dumps")
QDEF1(MP_QSTR_e, 46528, 1, "e")
QDEF1(MP_QSTR_enable, 56836, 6, "enable")
QDEF1(MP_QSTR_encode, 51779, 6, "encode")
QDEF1(MP_QSTR_enumerate, 47729, 9, "enumerate")
QDEF1(MP_QSTR_erf, 9108, 3, "erf")
QDEF1(MP_QSTR_erfc, 38519, 4, "erfc")
QDEF1(MP_QSTR_errno, 4545, 5, "errno")
QDEF1(MP_QSTR_errorcode, 56592, 9, "errorcode")
QDEF1(MP_QSTR_exp, 9416, 3, "exp")
QDEF1(MP_QSTR_expm1, 29300, 5, "expm1")
QDEF1(MP_QSTR_fabs, 4755, 4, "fabs")
QDEF1(MP_QSTR_factorial, 13004, 9, "factorial")
QDEF1(MP_QSTR_filter, 48677, 6, "filter")
QDEF1(MP_QSTR_float, 17461, 5, "float")
QDEF1(MP_QSTR_floor, 18045, 5, "floor")
QDEF1(MP_QSTR_flush, 49505, 5, "flush")
QDEF1(MP_QSTR_fmod, 17637, 4, "fmod")
QDEF1(MP_QSTR_frexp, 38940, 5, "frexp")
QDEF1(MP_QSTR_fromhex, 14598, 7, "fromhex")
QDEF1(MP_QSTR_fromkeys, 48439, 8, "fromkeys")
QDEF1(MP_QSTR_frozenset, 40173, 9, "frozenset")
QDEF1(MP_QSTR_function, 551, 8, "function")
QDEF1(MP_QSTR_gamma, 36866, 5, "gamma")
QDEF1(MP_QSTR_gc, 28257, 2, "gc")
QDEF1(MP_QSTR_generator, 50070, 9, "generator")
QDEF1(MP_QSTR_getrandbits, 32102, 11, "getrandbits")
QDEF1(MP_QSTR_getsizeof, 9215, 9, "getsizeof")
QDEF1(MP_QSTR_getter, 45712, 6, "getter")
QDEF1(MP_QSTR_getvalue, 44152, 8, "getvalue")
QDEF1(MP_QSTR_group, 45242, 5, "group")
QDEF1(MP_QSTR_groups, 51081, 6, "groups")
QDEF1(MP_QSTR_heap_lock, 36013, 9, "heap_lock")
QDEF1(MP_QSTR_heap_unlock, 11606, 11, "heap_unlock")
QDEF1(MP_QSTR_heapify, 11695, 7, "heapify")
QDEF1(MP_QSTR_heappop, 10198, 7, "heappop")
QDEF1(MP_QSTR_heappush, 27527, 8, "heappush")
QDEF1(MP_QSTR_heapq, 7528, 5, "heapq")
QDEF1(MP_QSTR_hex, 20592, 3, "hex")
QDEF1(MP_QSTR_hexlify, 32554, 7, "hexlify")
QDEF1(MP_QSTR_ignore, 6717, 6, "ignore")
QDEF1(MP_QSTR_imag, 46919, 4, "imag")
QDEF1(MP_QSTR_implementation, 11543, 14, "implementation")
QDEF1(MP_QSTR_indices, 18522, 7, "indices")
QDEF1(MP_QSTR_inf, 21252, 3, "inf")
QDEF1(MP_QSTR_intersection, 10792, 12, "intersection")
QDEF1(MP_QSTR_intersection_update, 56582, 19, "intersection_update")
QDEF1(MP_QSTR_io, 28451, 2, "io")
QDEF1(MP_QSTR_ioctl, 49784, 5, "ioctl")
QDEF1(MP_QSTR_isclose, 20041, 7, "isclose")
QDEF1(MP_QSTR_isdisjoint, 26871, 10, "isdisjoint")
QDEF1(MP_QSTR_isenabled, 58778, 9, "isenabled")
QDEF1(MP_QSTR_isfinite, 43942, 8, "isfinite")
QDEF1(MP_QSTR_isinf, 4414, 5, "isinf")
QDEF1(MP_QSTR_isnan, 926, 5, "isnan")
QDEF1(MP_QSTR_issubset, 49593, 8, "issubset")
QDEF1(MP_QSTR_issuperset, 60668, 10, "issuperset")
QDEF1(MP_QSTR_itemsize, 30581, 8, "itemsize")
QDEF1(MP_QSTR_iterable, 37413, 8, "iterable")
QDEF1(MP_QSTR_iterator, 48711, 8, "iterator")
QDEF1(MP_QSTR_json, 53757, 4, "json")
QDEF1(MP_QSTR_kbd_intr, 5110, 8, "kbd_intr")
QDEF1(MP_QSTR_keepends, 35682, 8, "keepends")
QDEF1(MP_QSTR_ldexp, 28480, 5, "ldexp")
QDEF1(MP_QSTR_length, 34649, 6, "length")
QDEF1(MP_QSTR_lgamma, 27854, 6, "lgamma")
QDEF1(MP_QSTR_load, 9315, 4, "load")
QDEF1(MP_QSTR_loads, 45232, 5, "loads")
QDEF1(MP_QSTR_log, 16161, 3, "log")
QDEF1(MP_QSTR_log10, 37184, 5, "log10")
QDEF1(MP_QSTR_log2, 9075, 4, "log2")
QDEF1(MP_QSTR_match, 8854, 5, "match")
QDEF1(MP_QSTR_math, 47925, 4, "math")
QDEF1(MP_QSTR_max, 17329, 3, "max")
QDEF1(MP_QSTR_maximum_space_recursion_space_depth_space_exceeded, 7795, 32, "maximum recursion depth exceeded")
QDEF1(MP_QSTR_maxsize, 28884, 7, "maxsize")
QDEF1(MP_QSTR_mem_alloc, 11090, 9, "mem_alloc")
QDEF1(MP_QSTR_mem_free, 25291, 8, "mem_free")
QDEF1(MP_QSTR_memoryview, 17513, 10, "memoryview")
QDEF1(MP_QSTR_min, 17071, 3, "min")
QDEF1(MP_QSTR_modf, 49189, 4, "modf")
QDEF1(MP_QSTR_module, 39359, 6, "module")
QDEF1(MP_QSTR_modules, 53740, 7, "modules")
QDEF1(MP_QSTR_name, 30114, 4, "name")
QDEF1(MP_QSTR_namedtuple, 5662, 10, "namedtuple")
QDEF1(MP_QSTR_nan, 22756, 3, "nan")
QDEF1(MP_QSTR_newline, 35127, 7, "newline")
QDEF1(MP_QSTR_oct, 23805, 3, "oct")
QDEF1(MP_QSTR_opt_level, 26503, 9, "opt_level")
QDEF1(MP_QSTR_pack, 53692, 4, "pack")
QDEF1(MP_QSTR_pack_into, 43295, 9, "pack_into")
QDEF1(MP_QSTR_partition, 58759, 9, "partition")
QDEF1(MP_QSTR_pend_throw, 29939, 10, "pend_throw")
QDEF1(MP_QSTR_phase, 54634, 5, "phase")
QDEF1(MP_QSTR_pi, 28700, 2, "pi")
QDEF1(MP_QSTR_platform, 6458, 8, "platform")
QDEF1(MP_QSTR_polar, 3077, 5, "polar")
QDEF1(MP_QSTR_popleft, 39537, 7, "popleft")
QDEF1(MP_QSTR_print_exception, 8732, 15, "print_exception")
QDEF1(MP_QSTR_property, 10690, 8, "property")
QDEF1(MP_QSTR_radians, 16263, 7, "radians")
QDEF1(MP_QSTR_randint, 56495, 7, "randint")
QDEF1(MP_QSTR_random, 11454, 6, "random")
QDEF1(MP_QSTR_randrange, 16035, 9, "randrange")
QDEF1(MP_QSTR_re, 28882, 2, "re")
QDEF1(MP_QSTR_real, 63935, 4, "real")
QDEF1(MP_QSTR_rect, 63973, 4, "rect")
QDEF1(MP_QSTR_rel_tol, 18006, 7, "rel_tol")
QDEF1(MP_QSTR_reversed, 28321, 8, "reversed")
QDEF1(MP_QSTR_rpartition, 53269, 10, "rpartition")
QDEF1(MP_QSTR_search, 49579, 6, "search")
QDEF1(MP_QSTR_seed, 30098, 4, "seed")
QDEF1(MP_QSTR_seek, 30109, 4, "seek")
QDEF1(MP_QSTR_separators, 3307, 10, "separators")
QDEF1(MP_QSTR_setter, 22788, 6, "setter")
QDEF1(MP_QSTR_signed, 311, 6, "signed")
QDEF1(MP_QSTR_sin, 37041, 3, "sin")
QDEF1(MP_QSTR_single, 8255, 6, "single")
QDEF1(MP_QSTR_sinh, 42681, 4, "sinh")
QDEF1(MP_QSTR_slice, 62645, 5, "slice")
QDEF1(MP_QSTR_span, 18121, 4, "span")
QDEF1(MP_QSTR_splitlines, 54122, 10, "splitlines")
QDEF1(MP_QSTR_sqrt, 17441, 4, "sqrt")
QDEF1(MP_QSTR_struct, 36882, 6, "struct")
QDEF1(MP_QSTR_sub, 36129, 3, "sub")
QDEF1(MP_QSTR_symmetric_difference, 26574, 20, "symmetric_difference")
QDEF1(MP_QSTR_symmetric_difference_update, 63584, 27, "symmetric_difference_update")
QDEF1(MP_QSTR_sys, 36540, 3, "sys")
QDEF1(MP_QSTR_tan, 25086, 3, "tan")
QDEF1(MP_QSTR_tanh, 41430, 4, "tanh")
QDEF1(MP_QSTR_tau, 25061, 3, "tau")
QDEF1(MP_QSTR_tell, 45332, 4, "tell")
QDEF1(MP_QSTR_threshold, 12274, 9, "threshold")
QDEF1(MP_QSTR_trunc, 39259, 5, "trunc")
QDEF1(MP_QSTR_unhexlify, 47537, 9, "unhexlify")
QDEF1(MP_QSTR_uniform, 62721, 7, "uniform")
QDEF1(MP_QSTR_union, 31990, 5, "union")
QDEF1(MP_QSTR_unpack, 15367, 6, "unpack")
QDEF1(MP_QSTR_unpack_from, 27918, 11, "unpack_from")
QDEF1(MP_QSTR_usys, 62409, 4, "usys")
QDEF1(MP_QSTR_utf8, 60602, 4, "utf8")
QDEF1(MP_QSTR_version, 54207, 7, "version")
QDEF1(MP_QSTR_version_info, 2670, 12, "version_info")
QDEF1(MP_QSTR__brace_open__colon__hash_b_brace_close_, 14168, 5, "{:#b}")
#undef QDEF0
#undef QDEF1
    #endif
    MP_QSTRnumber_of, // no underscore so it can't clash with any of the above
};

typedef size_t qstr;
typedef uint16_t qstr_short_t;

#if MICROPY_QSTR_BYTES_IN_HASH == 0
// No qstr_hash_t type needed.
#elif MICROPY_QSTR_BYTES_IN_HASH == 1
typedef uint8_t qstr_hash_t;
#elif MICROPY_QSTR_BYTES_IN_HASH == 2
typedef uint16_t qstr_hash_t;
#else
#error unimplemented qstr hash decoding
#endif

#if MICROPY_QSTR_BYTES_IN_LEN == 1
typedef uint8_t qstr_len_t;
#elif MICROPY_QSTR_BYTES_IN_LEN == 2
typedef uint16_t qstr_len_t;
#else
#error unimplemented qstr length decoding
#endif

typedef struct _qstr_pool_t {
    const struct _qstr_pool_t *prev;
    size_t total_prev_len : (8 * sizeof(size_t) - 1);
    size_t is_sorted : 1;
    size_t alloc;
    size_t len;
    #if MICROPY_QSTR_BYTES_IN_HASH
    qstr_hash_t *hashes;
    #endif
    qstr_len_t *lengths;
    const char *qstrs[];
} qstr_pool_t;

#define QSTR_TOTAL() (MP_STATE_VM(last_pool)->total_prev_len + MP_STATE_VM(last_pool)->len)

void qstr_init(void);

size_t qstr_compute_hash(const byte *data, size_t len);

qstr qstr_find_strn(const char *str, size_t str_len); // returns MP_QSTRnull if not found

qstr qstr_from_str(const char *str);
qstr qstr_from_strn(const char *str, size_t len);
#if MICROPY_VFS_ROM
qstr qstr_from_strn_static(const char *str, size_t len);
#endif

mp_uint_t qstr_hash(qstr q);
const char *qstr_str(qstr q);
size_t qstr_len(qstr q);
const byte *qstr_data(qstr q, size_t *len);

void qstr_pool_info(size_t *n_pool, size_t *n_qstr, size_t *n_str_data_bytes, size_t *n_total_bytes);
void qstr_dump_data(void);

#if MICROPY_ROM_TEXT_COMPRESSION
void mp_decompress_rom_string(byte *dst, const mp_rom_error_text_t src);
#define MP_IS_COMPRESSED_ROM_STRING(s) (*(byte *)(s) == 0xff)
#endif

#endif // MICROPY_INCLUDED_PY_QSTR_H
/* py/mpprint.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_MPPRINT_H
#define MICROPY_INCLUDED_PY_MPPRINT_H


#define PF_FLAG_LEFT_ADJUST       (0x001)
#define PF_FLAG_SHOW_SIGN         (0x002)
#define PF_FLAG_SPACE_SIGN        (0x004)
#define PF_FLAG_SHOW_PREFIX       (0x008)
#define PF_FLAG_PAD_AFTER_SIGN    (0x010)
#define PF_FLAG_CENTER_ADJUST     (0x020)
#define PF_FLAG_ADD_PERCENT       (0x040)
#define PF_FLAG_SHOW_OCTAL_LETTER (0x080)
#define PF_FLAG_ALWAYS_DECIMAL    (0x100)
#define PF_FLAG_SEP_POS           (9) // must be above all the above PF_FLAGs

#if MICROPY_PY_IO && MICROPY_PY_SYS_STDFILES
#define MP_PYTHON_PRINTER &mp_sys_stdout_print
#else
#define MP_PYTHON_PRINTER &mp_plat_print
#endif

typedef void (*mp_print_strn_t)(void *data, const char *str, size_t len);

typedef struct _mp_print_t {
    void *data;
    mp_print_strn_t print_strn;
} mp_print_t;

typedef struct _mp_print_ext_t {
    mp_print_t base;
    const char *item_separator;
    const char *key_separator;
} mp_print_ext_t;

#define MP_PRINT_GET_EXT(print) ((mp_print_ext_t *)print)

// All (non-debug) prints go through one of the two interfaces below.
// 1) Wrapper for platform print function, which wraps MP_PLAT_PRINT_STRN.
extern const mp_print_t mp_plat_print;
#if MICROPY_PY_IO && MICROPY_PY_SYS_STDFILES
// 2) Wrapper for printing to sys.stdout.
extern const mp_print_t mp_sys_stdout_print;
#endif

int mp_print_str(const mp_print_t *print, const char *str);
int mp_print_strn(const mp_print_t *print, const char *str, size_t len, unsigned int flags, char fill, int width);
#if MICROPY_PY_BUILTINS_FLOAT
int mp_print_float(const mp_print_t *print, mp_float_t f, char fmt, unsigned int flags, char fill, int width, int prec);
#endif

int mp_printf(const mp_print_t *print, const char *fmt, ...);
#ifdef va_start
int mp_vprintf(const mp_print_t *print, const char *fmt, va_list args);
#endif

#endif // MICROPY_INCLUDED_PY_MPPRINT_H
/* py/runtime0.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_RUNTIME0_H
#define MICROPY_INCLUDED_PY_RUNTIME0_H

// These constants are used by:
// - mp_raw_code_t::is_generator (only MP_SCOPE_FLAG_GENERATOR)
// - scope_t::scope_flags (16 bits)
// - MP_BC_PRELUDE_SIG_ENCODE macro, masked by MP_SCOPE_FLAG_ALL_SIG (4 bits)
// - tools/mpy_ld.py, when generating mpy files (maximum 7 bits)
#define MP_SCOPE_FLAG_ALL_SIG      (0x0f)
#define MP_SCOPE_FLAG_GENERATOR    (0x01)
#define MP_SCOPE_FLAG_VARKEYWORDS  (0x02)
#define MP_SCOPE_FLAG_VARARGS      (0x04)
#define MP_SCOPE_FLAG_DEFKWARGS    (0x08)
#define MP_SCOPE_FLAG_REFGLOBALS   (0x10) // used only if native emitter enabled
#define MP_SCOPE_FLAG_HASCONSTS    (0x20) // used only if native emitter enabled
#define MP_SCOPE_FLAG_VIPERRET_POS    (6) // 3 bits used for viper return type, to pass from compiler to native emitter
#define MP_SCOPE_FLAG_VIPERRELOC   (0x10) // used only when loading viper from .mpy
#define MP_SCOPE_FLAG_VIPERRODATA  (0x20) // used only when loading viper from .mpy
#define MP_SCOPE_FLAG_VIPERBSS     (0x40) // used only when loading viper from .mpy

// types for native (viper) function signature
#define MP_NATIVE_TYPE_OBJ  (0x00)
#define MP_NATIVE_TYPE_BOOL (0x01)
#define MP_NATIVE_TYPE_INT  (0x02)
#define MP_NATIVE_TYPE_UINT (0x03)
#define MP_NATIVE_TYPE_PTR  (0x04)
#define MP_NATIVE_TYPE_PTR8 (0x05)
#define MP_NATIVE_TYPE_PTR16 (0x06)
#define MP_NATIVE_TYPE_PTR32 (0x07)

// Not use for viper, but for dynamic native modules
#define MP_NATIVE_TYPE_QSTR (0x08)

// Bytecode and runtime boundaries for unary ops
#define MP_UNARY_OP_NUM_BYTECODE    (MP_UNARY_OP_NOT + 1)
#define MP_UNARY_OP_NUM_RUNTIME     (MP_UNARY_OP_SIZEOF + 1)

// Bytecode and runtime boundaries for binary ops
#define MP_BINARY_OP_NUM_BYTECODE   (MP_BINARY_OP_POWER + 1)
#if MICROPY_PY_REVERSE_SPECIAL_METHODS
#define MP_BINARY_OP_NUM_RUNTIME    (MP_BINARY_OP_REVERSE_POWER + 1)
#else
#define MP_BINARY_OP_NUM_RUNTIME    (MP_BINARY_OP_CONTAINS + 1)
#endif

typedef enum {
    // These ops may appear in the bytecode. Changing this group
    // in any way requires changing the bytecode version.
    MP_UNARY_OP_POSITIVE,
    MP_UNARY_OP_NEGATIVE,
    MP_UNARY_OP_INVERT,
    MP_UNARY_OP_NOT,

    // Following ops cannot appear in the bytecode
    MP_UNARY_OP_BOOL, // __bool__
    MP_UNARY_OP_LEN, // __len__
    MP_UNARY_OP_HASH, // __hash__; must return a small int
    MP_UNARY_OP_ABS, // __abs__
    MP_UNARY_OP_INT_MAYBE, // __int__; must return MP_OBJ_NULL, or an object satisfying mp_obj_is_int()
    MP_UNARY_OP_FLOAT_MAYBE, // __float__
    MP_UNARY_OP_COMPLEX_MAYBE, // __complex__
    MP_UNARY_OP_SIZEOF, // for sys.getsizeof()
} mp_unary_op_t;

typedef enum {
    // The following 9+13+13 ops are used in bytecode and changing
    // them requires changing the bytecode version.

    // 9 relational operations, should return a bool; order of first 6 matches corresponding mp_token_kind_t
    MP_BINARY_OP_LESS,
    MP_BINARY_OP_MORE,
    MP_BINARY_OP_EQUAL,
    MP_BINARY_OP_LESS_EQUAL,
    MP_BINARY_OP_MORE_EQUAL,
    MP_BINARY_OP_NOT_EQUAL,
    MP_BINARY_OP_IN,
    MP_BINARY_OP_IS,
    MP_BINARY_OP_EXCEPTION_MATCH,

    // 13 inplace arithmetic operations; order matches corresponding mp_token_kind_t
    MP_BINARY_OP_INPLACE_OR,
    MP_BINARY_OP_INPLACE_XOR,
    MP_BINARY_OP_INPLACE_AND,
    MP_BINARY_OP_INPLACE_LSHIFT,
    MP_BINARY_OP_INPLACE_RSHIFT,
    MP_BINARY_OP_INPLACE_ADD,
    MP_BINARY_OP_INPLACE_SUBTRACT,
    MP_BINARY_OP_INPLACE_MULTIPLY,
    MP_BINARY_OP_INPLACE_MAT_MULTIPLY,
    MP_BINARY_OP_INPLACE_FLOOR_DIVIDE,
    MP_BINARY_OP_INPLACE_TRUE_DIVIDE,
    MP_BINARY_OP_INPLACE_MODULO,
    MP_BINARY_OP_INPLACE_POWER,

    // 13 normal arithmetic operations; order matches corresponding mp_token_kind_t
    MP_BINARY_OP_OR,
    MP_BINARY_OP_XOR,
    MP_BINARY_OP_AND,
    MP_BINARY_OP_LSHIFT,
    MP_BINARY_OP_RSHIFT,
    MP_BINARY_OP_ADD,
    MP_BINARY_OP_SUBTRACT,
    MP_BINARY_OP_MULTIPLY,
    MP_BINARY_OP_MAT_MULTIPLY,
    MP_BINARY_OP_FLOOR_DIVIDE,
    MP_BINARY_OP_TRUE_DIVIDE,
    MP_BINARY_OP_MODULO,
    MP_BINARY_OP_POWER,

    // Operations below this line don't appear in bytecode, they
    // just identify special methods.

    // This is not emitted by the compiler but is supported by the runtime.
    // It must follow immediately after MP_BINARY_OP_POWER.
    MP_BINARY_OP_DIVMOD,

    // The runtime will convert MP_BINARY_OP_IN to this operator with swapped args.
    // A type should implement this containment operator instead of MP_BINARY_OP_IN.
    MP_BINARY_OP_CONTAINS,

    // 13 MP_BINARY_OP_REVERSE_* operations must be in the same order as MP_BINARY_OP_*,
    // and be the last ones supported by the runtime.
    MP_BINARY_OP_REVERSE_OR,
    MP_BINARY_OP_REVERSE_XOR,
    MP_BINARY_OP_REVERSE_AND,
    MP_BINARY_OP_REVERSE_LSHIFT,
    MP_BINARY_OP_REVERSE_RSHIFT,
    MP_BINARY_OP_REVERSE_ADD,
    MP_BINARY_OP_REVERSE_SUBTRACT,
    MP_BINARY_OP_REVERSE_MULTIPLY,
    MP_BINARY_OP_REVERSE_MAT_MULTIPLY,
    MP_BINARY_OP_REVERSE_FLOOR_DIVIDE,
    MP_BINARY_OP_REVERSE_TRUE_DIVIDE,
    MP_BINARY_OP_REVERSE_MODULO,
    MP_BINARY_OP_REVERSE_POWER,

    // These 2 are not supported by the runtime and must be synthesised by the emitter
    MP_BINARY_OP_NOT_IN,
    MP_BINARY_OP_IS_NOT,
} mp_binary_op_t;

#endif // MICROPY_INCLUDED_PY_RUNTIME0_H

// This is the definition of the opaque MicroPython object type.
// All concrete objects have an encoding within this type and the
// particular encoding is specified by MICROPY_OBJ_REPR.
#if MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_D
typedef uint64_t mp_obj_t;
typedef uint64_t mp_const_obj_t;
#else
typedef void *mp_obj_t;
typedef const void *mp_const_obj_t;
#endif

// This mp_obj_type_t struct is a concrete MicroPython object which holds info
// about a type.  See below for actual definition of the struct.
typedef struct _mp_obj_type_t mp_obj_type_t;

// Anything that wants to be a concrete MicroPython object must have mp_obj_base_t
// as its first member (small ints, qstr objs and inline floats are not concrete).
struct _mp_obj_base_t {
    const mp_obj_type_t *type MICROPY_OBJ_BASE_ALIGNMENT;
};
typedef struct _mp_obj_base_t mp_obj_base_t;

// These fake objects are used to indicate certain things in arguments or return
// values, and should only be used when explicitly allowed.
//
//  - MP_OBJ_NULL : used to indicate the absence of an object, or unsupported operation.
//  - MP_OBJ_STOP_ITERATION : used instead of throwing a StopIteration, for efficiency.
//  - MP_OBJ_SENTINEL : used for various internal purposes where one needs
//    an object which is unique from all other objects, including MP_OBJ_NULL.
//
// For debugging purposes they are all different.  For non-debug mode, we alias
// as many as we can to MP_OBJ_NULL because it's cheaper to load/compare 0.

#if MICROPY_DEBUG_MP_OBJ_SENTINELS
#define MP_OBJ_NULL             (MP_OBJ_FROM_PTR((void *)0))
#define MP_OBJ_STOP_ITERATION   (MP_OBJ_FROM_PTR((void *)4))
#define MP_OBJ_SENTINEL         (MP_OBJ_FROM_PTR((void *)8))
#else
#define MP_OBJ_NULL             (MP_OBJ_FROM_PTR((void *)0))
#define MP_OBJ_STOP_ITERATION   (MP_OBJ_FROM_PTR((void *)0))
#define MP_OBJ_SENTINEL         (MP_OBJ_FROM_PTR((void *)4))
#endif

// These macros/inline functions operate on objects and depend on the
// particular object representation.  They are used to query, pack and
// unpack small ints, qstrs and full object pointers.

#if MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_A

static inline bool mp_obj_is_small_int(mp_const_obj_t o) {
    return (((mp_int_t)(o)) & 1) != 0;
}
#define MP_OBJ_SMALL_INT_VALUE(o) (((mp_int_t)(o)) >> 1)
#define MP_OBJ_NEW_SMALL_INT(small_int) ((mp_obj_t)((((mp_uint_t)(small_int)) << 1) | 1))

static inline bool mp_obj_is_qstr(mp_const_obj_t o) {
    return (((mp_int_t)(o)) & 7) == 2;
}
#define MP_OBJ_QSTR_VALUE(o) (((mp_uint_t)(o)) >> 3)
#define MP_OBJ_NEW_QSTR(qst) ((mp_obj_t)((((mp_uint_t)(qst)) << 3) | 2))

static inline bool mp_obj_is_immediate_obj(mp_const_obj_t o) {
    return (((mp_int_t)(o)) & 7) == 6;
}
#define MP_OBJ_IMMEDIATE_OBJ_VALUE(o) (((mp_uint_t)(o)) >> 3)
#define MP_OBJ_NEW_IMMEDIATE_OBJ(val) ((mp_obj_t)(((val) << 3) | 6))

#if MICROPY_PY_BUILTINS_FLOAT
#define mp_const_float_e MP_ROM_PTR(&mp_const_float_e_obj)
#define mp_const_float_pi MP_ROM_PTR(&mp_const_float_pi_obj)
#if MICROPY_PY_MATH_CONSTANTS
#define mp_const_float_tau MP_ROM_PTR(&mp_const_float_tau_obj)
#define mp_const_float_inf MP_ROM_PTR(&mp_const_float_inf_obj)
#define mp_const_float_nan MP_ROM_PTR(&mp_const_float_nan_obj)
#endif
extern const struct _mp_obj_float_t mp_const_float_e_obj;
extern const struct _mp_obj_float_t mp_const_float_pi_obj;
#if MICROPY_PY_MATH_CONSTANTS
extern const struct _mp_obj_float_t mp_const_float_tau_obj;
extern const struct _mp_obj_float_t mp_const_float_inf_obj;
extern const struct _mp_obj_float_t mp_const_float_nan_obj;
#endif

#define mp_obj_is_float(o) mp_obj_is_exact_type((o), &mp_type_float)
mp_float_t mp_obj_float_get(mp_obj_t self_in);
mp_obj_t mp_obj_new_float(mp_float_t value);
#endif

static inline bool mp_obj_is_obj(mp_const_obj_t o) {
    return (((mp_int_t)(o)) & 3) == 0;
}

#elif MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_B

static inline bool mp_obj_is_small_int(mp_const_obj_t o) {
    return (((mp_int_t)(o)) & 3) == 1;
}
#define MP_OBJ_SMALL_INT_VALUE(o) (((mp_int_t)(o)) >> 2)
#define MP_OBJ_NEW_SMALL_INT(small_int) ((mp_obj_t)((((mp_uint_t)(small_int)) << 2) | 1))

static inline bool mp_obj_is_qstr(mp_const_obj_t o) {
    return (((mp_int_t)(o)) & 7) == 3;
}
#define MP_OBJ_QSTR_VALUE(o) (((mp_uint_t)(o)) >> 3)
#define MP_OBJ_NEW_QSTR(qst) ((mp_obj_t)((((mp_uint_t)(qst)) << 3) | 3))

static inline bool mp_obj_is_immediate_obj(mp_const_obj_t o) {
    return (((mp_int_t)(o)) & 7) == 7;
}
#define MP_OBJ_IMMEDIATE_OBJ_VALUE(o) (((mp_uint_t)(o)) >> 3)
#define MP_OBJ_NEW_IMMEDIATE_OBJ(val) ((mp_obj_t)(((val) << 3) | 7))

#if MICROPY_PY_BUILTINS_FLOAT
#define mp_const_float_e MP_ROM_PTR(&mp_const_float_e_obj)
#define mp_const_float_pi MP_ROM_PTR(&mp_const_float_pi_obj)
#if MICROPY_PY_MATH_CONSTANTS
#define mp_const_float_tau MP_ROM_PTR(&mp_const_float_tau_obj)
#define mp_const_float_inf MP_ROM_PTR(&mp_const_float_inf_obj)
#define mp_const_float_nan MP_ROM_PTR(&mp_const_float_nan_obj)
#endif
extern const struct _mp_obj_float_t mp_const_float_e_obj;
extern const struct _mp_obj_float_t mp_const_float_pi_obj;
#if MICROPY_PY_MATH_CONSTANTS
extern const struct _mp_obj_float_t mp_const_float_tau_obj;
extern const struct _mp_obj_float_t mp_const_float_inf_obj;
extern const struct _mp_obj_float_t mp_const_float_nan_obj;
#endif

#define mp_obj_is_float(o) mp_obj_is_exact_type((o), &mp_type_float)
mp_float_t mp_obj_float_get(mp_obj_t self_in);
mp_obj_t mp_obj_new_float(mp_float_t value);
#endif

static inline bool mp_obj_is_obj(mp_const_obj_t o) {
    return (((mp_int_t)(o)) & 1) == 0;
}

#elif MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_C

#if MICROPY_FLOAT_IMPL == MICROPY_FLOAT_IMPL_NONE
#error "MICROPY_OBJ_REPR_C requires float to be enabled."
#endif

static inline bool mp_obj_is_small_int(mp_const_obj_t o) {
    return (((mp_int_t)(o)) & 1) != 0;
}
#define MP_OBJ_SMALL_INT_VALUE(o) (((mp_int_t)(o)) >> 1)
#define MP_OBJ_NEW_SMALL_INT(small_int) ((mp_obj_t)((((mp_uint_t)(small_int)) << 1) | 1))

#if MICROPY_PY_BUILTINS_FLOAT
#include <math.h>
// note: MP_OBJ_NEW_CONST_FLOAT should be a MP_ROM_PTR but that macro isn't available yet
#define MP_OBJ_NEW_CONST_FLOAT(f) ((mp_obj_t)((((((uint64_t)f) & ~3) | 2) + 0x80800000) & 0xffffffff))
#define mp_const_float_e  MP_OBJ_NEW_CONST_FLOAT(0x402df854)
#define mp_const_float_pi MP_OBJ_NEW_CONST_FLOAT(0x40490fdb)
#define mp_const_float_nan MP_OBJ_NEW_CONST_FLOAT(0x7fc00000)
#if MICROPY_PY_MATH_CONSTANTS
#define mp_const_float_tau MP_OBJ_NEW_CONST_FLOAT(0x40c90fdb)
#define mp_const_float_inf MP_OBJ_NEW_CONST_FLOAT(0x7f800000)
#endif

static inline bool mp_obj_is_float(mp_const_obj_t o) {
    // Ensure that 32-bit arch can only use single precision.
    MP_STATIC_ASSERT(sizeof(mp_float_t) <= sizeof(mp_obj_t));

    return (((mp_uint_t)(o)) & 3) == 2 && (((mp_uint_t)(o)) & 0xff800007) != 0x00000006;
}
static inline mp_float_t mp_obj_float_get(mp_const_obj_t o) {
    union {
        mp_float_t f;
        mp_uint_t u;
    } num = {.u = ((mp_uint_t)o - 0x80800000u) & ~3u};
    // Rather than always truncating toward zero, which creates a strong
    // bias, copy the two previous bits to fill in the two missing bits.
    // This appears to be a pretty good heuristic.
    num.u |= (num.u >> 2) & 3u;
    return num.f;
}
static inline mp_obj_t mp_obj_new_float(mp_float_t f) {
    if (isnan(f)) {
        // prevent creation of bad nanboxed pointers via array.array or struct
        return mp_const_float_nan;
    }
    union {
        mp_float_t f;
        mp_uint_t u;
    } num = {.f = f};
    return (mp_obj_t)(((num.u & ~0x3u) | 2u) + 0x80800000u);
}
#endif

static inline bool mp_obj_is_qstr(mp_const_obj_t o) {
    return (((mp_uint_t)(o)) & 0xff80000f) == 0x00000006;
}
#define MP_OBJ_QSTR_VALUE(o) (((mp_uint_t)(o)) >> 4)
#define MP_OBJ_NEW_QSTR(qst) ((mp_obj_t)((((mp_uint_t)(qst)) << 4) | 0x00000006))

static inline bool mp_obj_is_immediate_obj(mp_const_obj_t o) {
    return (((mp_uint_t)(o)) & 0xff80000f) == 0x0000000e;
}
#define MP_OBJ_IMMEDIATE_OBJ_VALUE(o) (((mp_uint_t)(o)) >> 4)
#define MP_OBJ_NEW_IMMEDIATE_OBJ(val) ((mp_obj_t)(((val) << 4) | 0xe))

static inline bool mp_obj_is_obj(mp_const_obj_t o) {
    return (((mp_int_t)(o)) & 3) == 0;
}

#elif MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_D

static inline bool mp_obj_is_small_int(mp_const_obj_t o) {
    return (((uint64_t)(o)) & 0xffff000000000000) == 0x0001000000000000;
}
#define MP_OBJ_SMALL_INT_VALUE(o) (((mp_int_t)((o) << 16)) >> 17)
#define MP_OBJ_NEW_SMALL_INT(small_int) (((((uint64_t)(small_int)) & 0x7fffffffffff) << 1) | 0x0001000000000001)

static inline bool mp_obj_is_qstr(mp_const_obj_t o) {
    return (((uint64_t)(o)) & 0xffff000000000000) == 0x0002000000000000;
}
#define MP_OBJ_QSTR_VALUE(o) ((((uint32_t)(o)) >> 1) & 0xffffffff)
#define MP_OBJ_NEW_QSTR(qst) ((mp_obj_t)(((uint64_t)(((uint32_t)(qst)) << 1)) | 0x0002000000000001))

static inline bool mp_obj_is_immediate_obj(mp_const_obj_t o) {
    return (((uint64_t)(o)) & 0xffff000000000000) == 0x0003000000000000;
}
#define MP_OBJ_IMMEDIATE_OBJ_VALUE(o) ((((uint32_t)(o)) >> 46) & 3)
#define MP_OBJ_NEW_IMMEDIATE_OBJ(val) (((uint64_t)(val) << 46) | 0x0003000000000000)

#if MICROPY_PY_BUILTINS_FLOAT

#if MICROPY_FLOAT_IMPL != MICROPY_FLOAT_IMPL_DOUBLE
#error MICROPY_OBJ_REPR_D requires MICROPY_FLOAT_IMPL_DOUBLE
#endif

#include <math.h>
#define mp_const_float_e {((mp_obj_t)((uint64_t)0x4005bf0a8b145769 + 0x8004000000000000))}
#define mp_const_float_pi {((mp_obj_t)((uint64_t)0x400921fb54442d18 + 0x8004000000000000))}
#define mp_const_float_nan {((mp_obj_t)((uint64_t)0x7ff8000000000000 + 0x8004000000000000))}
#if MICROPY_PY_MATH_CONSTANTS
#define mp_const_float_tau {((mp_obj_t)((uint64_t)0x401921fb54442d18 + 0x8004000000000000))}
#define mp_const_float_inf {((mp_obj_t)((uint64_t)0x7ff0000000000000 + 0x8004000000000000))}
#endif

static inline bool mp_obj_is_float(mp_const_obj_t o) {
    return ((uint64_t)(o) & 0xfffc000000000000) != 0;
}
static inline mp_float_t mp_obj_float_get(mp_const_obj_t o) {
    union {
        mp_float_t f;
        uint64_t r;
    } num = {.r = o - 0x8004000000000000};
    return num.f;
}
static inline mp_obj_t mp_obj_new_float(mp_float_t f) {
    if (isnan(f)) {
        // prevent creation of bad nanboxed pointers via array.array or struct
        struct {
            uint64_t r;
        } num = mp_const_float_nan;
        return num.r;
    }
    union {
        mp_float_t f;
        uint64_t r;
    } num = {.f = f};
    return num.r + 0x8004000000000000;
}
#endif

static inline bool mp_obj_is_obj(mp_const_obj_t o) {
    return (((uint64_t)(o)) & 0xffff000000000000) == 0x0000000000000000;
}
#define MP_OBJ_TO_PTR(o) ((void *)(uintptr_t)(o))
#define MP_OBJ_FROM_PTR(p) ((mp_obj_t)((uintptr_t)(p)))

// rom object storage needs special handling to widen 32-bit pointer to 64-bits
typedef union _mp_rom_obj_t {
    uint64_t u64;
    struct {
        const void *lo, *hi;
    } u32;
} mp_rom_obj_t;
#define MP_ROM_INT(i) {MP_OBJ_NEW_SMALL_INT(i)}
#define MP_ROM_QSTR(q) {MP_OBJ_NEW_QSTR(q)}
#if MP_ENDIANNESS_LITTLE
#define MP_ROM_PTR(p) {.u32 = {.lo = (p), .hi = NULL}}
#else
#define MP_ROM_PTR(p) {.u32 = {.lo = NULL, .hi = (p)}}
#endif

#endif

// Macros to convert between mp_obj_t and concrete object types.
// These are identity operations in MicroPython, but ability to override
// these operations are provided to experiment with other methods of
// object representation and memory management.

// Cast mp_obj_t to object pointer
#ifndef MP_OBJ_TO_PTR
#define MP_OBJ_TO_PTR(o) ((void *)(o))
#endif

// Cast object pointer to mp_obj_t
#ifndef MP_OBJ_FROM_PTR
#define MP_OBJ_FROM_PTR(p) ((mp_obj_t)(p))
#endif

// Macros to create objects that are stored in ROM.

#ifndef MP_ROM_NONE
#if MICROPY_OBJ_IMMEDIATE_OBJS
#define MP_ROM_NONE mp_const_none
#else
#define MP_ROM_NONE MP_ROM_PTR(&mp_const_none_obj)
#endif
#endif

#ifndef MP_ROM_FALSE
#if MICROPY_OBJ_IMMEDIATE_OBJS
#define MP_ROM_FALSE mp_const_false
#define MP_ROM_TRUE mp_const_true
#else
#define MP_ROM_FALSE MP_ROM_PTR(&mp_const_false_obj)
#define MP_ROM_TRUE MP_ROM_PTR(&mp_const_true_obj)
#endif
#endif

#ifndef MP_ROM_INT
typedef mp_const_obj_t mp_rom_obj_t;
#define MP_ROM_INT(i) MP_OBJ_NEW_SMALL_INT(i)
#define MP_ROM_QSTR(q) MP_OBJ_NEW_QSTR(q)
#define MP_ROM_PTR(p) (p)
/* for testing
typedef struct _mp_rom_obj_t { mp_const_obj_t o; } mp_rom_obj_t;
#define MP_ROM_INT(i) {MP_OBJ_NEW_SMALL_INT(i)}
#define MP_ROM_QSTR(q) {MP_OBJ_NEW_QSTR(q)}
#define MP_ROM_PTR(p) {.o = p}
*/
#endif

// These macros are used to declare and define constant function objects
// You can put "static" in front of the definitions to make them local

#define MP_DECLARE_CONST_FUN_OBJ_0(obj_name) extern const mp_obj_fun_builtin_fixed_t obj_name
#define MP_DECLARE_CONST_FUN_OBJ_1(obj_name) extern const mp_obj_fun_builtin_fixed_t obj_name
#define MP_DECLARE_CONST_FUN_OBJ_2(obj_name) extern const mp_obj_fun_builtin_fixed_t obj_name
#define MP_DECLARE_CONST_FUN_OBJ_3(obj_name) extern const mp_obj_fun_builtin_fixed_t obj_name
#define MP_DECLARE_CONST_FUN_OBJ_VAR(obj_name) extern const mp_obj_fun_builtin_var_t obj_name
#define MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(obj_name) extern const mp_obj_fun_builtin_var_t obj_name
#define MP_DECLARE_CONST_FUN_OBJ_KW(obj_name) extern const mp_obj_fun_builtin_var_t obj_name

#define MP_OBJ_FUN_ARGS_MAX (0xffff) // to set maximum value in n_args_max below
#define MP_OBJ_FUN_MAKE_SIG(n_args_min, n_args_max, takes_kw) ((uint32_t)((((uint32_t)(n_args_min)) << 17) | (((uint32_t)(n_args_max)) << 1) | ((takes_kw) ? 1 : 0)))

#define MP_DEFINE_CONST_FUN_OBJ_0(obj_name, fun_name) \
    const mp_obj_fun_builtin_fixed_t obj_name = \
    {.base = {.type = &mp_type_fun_builtin_0}, .fun = {._0 = fun_name}}
#define MP_DEFINE_CONST_FUN_OBJ_1(obj_name, fun_name) \
    const mp_obj_fun_builtin_fixed_t obj_name = \
    {.base = {.type = &mp_type_fun_builtin_1}, .fun = {._1 = fun_name}}
#define MP_DEFINE_CONST_FUN_OBJ_2(obj_name, fun_name) \
    const mp_obj_fun_builtin_fixed_t obj_name = \
    {.base = {.type = &mp_type_fun_builtin_2}, .fun = {._2 = fun_name}}
#define MP_DEFINE_CONST_FUN_OBJ_3(obj_name, fun_name) \
    const mp_obj_fun_builtin_fixed_t obj_name = \
    {.base = {.type = &mp_type_fun_builtin_3}, .fun = {._3 = fun_name}}
#define MP_DEFINE_CONST_FUN_OBJ_VAR(obj_name, n_args_min, fun_name) \
    const mp_obj_fun_builtin_var_t obj_name = \
    {.base = {.type = &mp_type_fun_builtin_var}, .sig = MP_OBJ_FUN_MAKE_SIG(n_args_min, MP_OBJ_FUN_ARGS_MAX, false), .fun = {.var = fun_name}}
#define MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(obj_name, n_args_min, n_args_max, fun_name) \
    const mp_obj_fun_builtin_var_t obj_name = \
    {.base = {.type = &mp_type_fun_builtin_var}, .sig = MP_OBJ_FUN_MAKE_SIG(n_args_min, n_args_max, false), .fun = {.var = fun_name}}
#define MP_DEFINE_CONST_FUN_OBJ_KW(obj_name, n_args_min, fun_name) \
    const mp_obj_fun_builtin_var_t obj_name = \
    {.base = {.type = &mp_type_fun_builtin_var}, .sig = MP_OBJ_FUN_MAKE_SIG(n_args_min, MP_OBJ_FUN_ARGS_MAX, true), .fun = {.kw = fun_name}}

// These macros are used to define constant map/dict objects
// You can put "static" in front of the definition to make it local

#define MP_DEFINE_CONST_MAP(map_name, table_name) \
    const mp_map_t map_name = { \
        .all_keys_are_qstrs = 1, \
        .is_fixed = 1, \
        .is_ordered = 1, \
        .used = MP_ARRAY_SIZE(table_name), \
        .alloc = MP_ARRAY_SIZE(table_name), \
        .table = (mp_map_elem_t *)(mp_rom_map_elem_t *)table_name, \
    }

#define MP_DEFINE_CONST_DICT_WITH_SIZE(dict_name, table_name, n) \
    const mp_obj_dict_t dict_name = { \
        .base = {&mp_type_dict}, \
        .map = { \
            .all_keys_are_qstrs = 1, \
            .is_fixed = 1, \
            .is_ordered = 1, \
            .used = n, \
            .alloc = n, \
            .table = (mp_map_elem_t *)(mp_rom_map_elem_t *)table_name, \
        }, \
    }

#define MP_DEFINE_CONST_DICT(dict_name, table_name) MP_DEFINE_CONST_DICT_WITH_SIZE(dict_name, table_name, MP_ARRAY_SIZE(table_name))

// These macros are used to declare and define constant staticmethod and classmethod objects
// You can put "static" in front of the definitions to make them local

#define MP_DECLARE_CONST_STATICMETHOD_OBJ(obj_name) extern const mp_rom_obj_static_class_method_t obj_name
#define MP_DECLARE_CONST_CLASSMETHOD_OBJ(obj_name) extern const mp_rom_obj_static_class_method_t obj_name

#define MP_DEFINE_CONST_STATICMETHOD_OBJ(obj_name, fun_name) const mp_rom_obj_static_class_method_t obj_name = {{&mp_type_staticmethod}, fun_name}
#define MP_DEFINE_CONST_CLASSMETHOD_OBJ(obj_name, fun_name) const mp_rom_obj_static_class_method_t obj_name = {{&mp_type_classmethod}, fun_name}

#ifndef NO_QSTR

// Declare a module as a builtin, processed by makemoduledefs.py
// param module_name: MP_QSTR_<module name>
// param obj_module: mp_obj_module_t instance
#define MP_REGISTER_MODULE(module_name, obj_module)

// As above, but allow this module to be extended from the filesystem.
#define MP_REGISTER_EXTENSIBLE_MODULE(module_name, obj_module)

// Add a custom handler for a builtin module that will be called to delegate
// failed attribute lookups.
#define MP_REGISTER_MODULE_DELEGATION(obj_module, fun_name)

// Declare a root pointer (to avoid garbage collection of a global static variable).
// param variable_declaration: a valid C variable declaration
#define MP_REGISTER_ROOT_POINTER(variable_declaration)

#endif // NO_QSTR

// Underlying map/hash table implementation (not dict object or map function)

typedef struct _mp_map_elem_t {
    mp_obj_t key;
    mp_obj_t value;
} mp_map_elem_t;

typedef struct _mp_rom_map_elem_t {
    mp_rom_obj_t key;
    mp_rom_obj_t value;
} mp_rom_map_elem_t;

typedef struct _mp_map_t {
    size_t all_keys_are_qstrs : 1;
    size_t is_fixed : 1;    // if set, table is fixed/read-only and can't be modified
    size_t is_ordered : 1;  // if set, table is an ordered array, not a hash map
    size_t used : (8 * sizeof(size_t) - 3);
    size_t alloc;
    mp_map_elem_t *table;
} mp_map_t;

// mp_set_lookup requires these constants to have the values they do
typedef enum _mp_map_lookup_kind_t {
    MP_MAP_LOOKUP = 0,
    MP_MAP_LOOKUP_ADD_IF_NOT_FOUND = 1,
    MP_MAP_LOOKUP_REMOVE_IF_FOUND = 2,
    MP_MAP_LOOKUP_ADD_IF_NOT_FOUND_OR_REMOVE_IF_FOUND = 3, // only valid for mp_set_lookup
} mp_map_lookup_kind_t;

static inline bool mp_map_slot_is_filled(const mp_map_t *map, size_t pos) {
    assert(pos < map->alloc);
    return (map)->table[pos].key != MP_OBJ_NULL && (map)->table[pos].key != MP_OBJ_SENTINEL;
}

void mp_map_init(mp_map_t *map, size_t n);
void mp_map_init_fixed_table(mp_map_t *map, size_t n, const mp_obj_t *table);
void mp_map_deinit(mp_map_t *map);
mp_map_elem_t *mp_map_lookup(mp_map_t *map, mp_obj_t index, mp_map_lookup_kind_t lookup_kind);
void mp_map_clear(mp_map_t *map);
void mp_map_dump(mp_map_t *map);

// Underlying set implementation (not set object)

typedef struct _mp_set_t {
    size_t alloc;
    size_t used;
    mp_obj_t *table;
} mp_set_t;

static inline bool mp_set_slot_is_filled(const mp_set_t *set, size_t pos) {
    return (set)->table[pos] != MP_OBJ_NULL && (set)->table[pos] != MP_OBJ_SENTINEL;
}

void mp_set_init(mp_set_t *set, size_t n);
mp_obj_t mp_set_lookup(mp_set_t *set, mp_obj_t index, mp_map_lookup_kind_t lookup_kind);
mp_obj_t mp_set_remove_first(mp_set_t *set);
void mp_set_clear(mp_set_t *set);

// Type definitions for methods

typedef mp_obj_t (*mp_fun_0_t)(void);
typedef mp_obj_t (*mp_fun_1_t)(mp_obj_t);
typedef mp_obj_t (*mp_fun_2_t)(mp_obj_t, mp_obj_t);
typedef mp_obj_t (*mp_fun_3_t)(mp_obj_t, mp_obj_t, mp_obj_t);
typedef mp_obj_t (*mp_fun_var_t)(size_t n, const mp_obj_t *);
// mp_fun_kw_t takes mp_map_t* (and not const mp_map_t*) to ease passing
// this arg to mp_map_lookup().
// Note that the mp_obj_t* array will contain all arguments, positional and keyword, with the keyword
// ones starting at offset n, like: arg0 arg1 ... arg<n> key0 value0 key1 value1 ..., and the mp_map_t*
// gets those same keyword arguments but as a map for convenience; see fun_builtin_var_call.
typedef mp_obj_t (*mp_fun_kw_t)(size_t n, const mp_obj_t *, mp_map_t *);

// Flags for type behaviour (mp_obj_type_t.flags)
// If MP_TYPE_FLAG_IS_SUBCLASSED is set, then subclasses of this class have been created.
//   Mutations to this class that would require updating all subclasses must be rejected.
// If MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS is set, then attribute lookups involving this
//   class need to additionally check for special accessor methods, such as from descriptors.
// If MP_TYPE_FLAG_EQ_NOT_REFLEXIVE is clear then __eq__ is reflexive (A==A returns True).
// If MP_TYPE_FLAG_EQ_CHECKS_OTHER_TYPE is clear then the type can't be equal to an
//   instance of any different class that also clears this flag.  If this flag is set
//   then the type may check for equality against a different type.
// If MP_TYPE_FLAG_EQ_HAS_NEQ_TEST is clear then the type only implements the __eq__
//   operator and not the __ne__ operator.  If it's set then __ne__ may be implemented.
// If MP_TYPE_FLAG_BINDS_SELF is set then the type as a method binds self as the first arg.
// If MP_TYPE_FLAG_BUILTIN_FUN is set then the type is a built-in function type.
// MP_TYPE_FLAG_ITER_IS_GETITER is a no-op flag that means the default behaviour for the
//   iter slot and it's the getiter function.
// If MP_TYPE_FLAG_ITER_IS_ITERNEXT is set then the "iter" slot is the iternext
//   function and getiter will be automatically implemented as "return self".
// If MP_TYPE_FLAG_ITER_IS_CUSTOM is set then the "iter" slot is a pointer to a
//   mp_getiter_iternext_custom_t struct instance (with both .getiter and .iternext set).
// If MP_TYPE_FLAG_ITER_IS_STREAM is set then the type implicitly gets a "return self"
//   getiter, and mp_stream_unbuffered_iter for iternext.
// If MP_TYPE_FLAG_INSTANCE_TYPE is set then this is an instance type (i.e. defined in Python).
// If MP_TYPE_FLAG_SUBSCR_ALLOWS_STACK_SLICE is set then the "subscr" slot allows a stack
//   allocated slice to be passed in (no references to it will be retained after the call).
#define MP_TYPE_FLAG_NONE (0x0000)
#define MP_TYPE_FLAG_IS_SUBCLASSED (0x0001)
#define MP_TYPE_FLAG_HAS_SPECIAL_ACCESSORS (0x0002)
#define MP_TYPE_FLAG_EQ_NOT_REFLEXIVE (0x0004)
#define MP_TYPE_FLAG_EQ_CHECKS_OTHER_TYPE (0x0008)
#define MP_TYPE_FLAG_EQ_HAS_NEQ_TEST (0x0010)
#define MP_TYPE_FLAG_BINDS_SELF (0x0020)
#define MP_TYPE_FLAG_BUILTIN_FUN (0x0040)
#define MP_TYPE_FLAG_ITER_IS_GETITER (0x0000)
#define MP_TYPE_FLAG_ITER_IS_ITERNEXT (0x0080)
#define MP_TYPE_FLAG_ITER_IS_CUSTOM (0x0100)
#define MP_TYPE_FLAG_ITER_IS_STREAM (MP_TYPE_FLAG_ITER_IS_ITERNEXT | MP_TYPE_FLAG_ITER_IS_CUSTOM)
#define MP_TYPE_FLAG_INSTANCE_TYPE (0x0200)
#define MP_TYPE_FLAG_SUBSCR_ALLOWS_STACK_SLICE (0x0400)

typedef enum {
    PRINT_STR = 0,
    PRINT_REPR = 1,
    PRINT_EXC = 2, // Special format for printing exception in unhandled exception message
    PRINT_JSON = 3,
    PRINT_RAW = 4, // Special format for printing bytes as an undercorated string
    PRINT_EXC_SUBCLASS = 0x80, // Internal flag for printing exception subclasses
} mp_print_kind_t;

typedef struct _mp_obj_iter_buf_t {
    mp_obj_base_t base;
    mp_obj_t buf[3];
} mp_obj_iter_buf_t;

// The number of slots that an mp_obj_iter_buf_t needs on the Python value stack.
// It's rounded up in case mp_obj_base_t is smaller than mp_obj_t (eg for OBJ_REPR_D).
#define MP_OBJ_ITER_BUF_NSLOTS ((sizeof(mp_obj_iter_buf_t) + sizeof(mp_obj_t) - 1) / sizeof(mp_obj_t))

typedef void (*mp_print_fun_t)(const mp_print_t *print, mp_obj_t o, mp_print_kind_t kind);
typedef mp_obj_t (*mp_make_new_fun_t)(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
typedef mp_obj_t (*mp_call_fun_t)(mp_obj_t fun, size_t n_args, size_t n_kw, const mp_obj_t *args);
typedef mp_obj_t (*mp_unary_op_fun_t)(mp_unary_op_t op, mp_obj_t);
typedef mp_obj_t (*mp_binary_op_fun_t)(mp_binary_op_t op, mp_obj_t, mp_obj_t);
typedef void (*mp_attr_fun_t)(mp_obj_t self_in, qstr attr, mp_obj_t *dest);
typedef mp_obj_t (*mp_subscr_fun_t)(mp_obj_t self_in, mp_obj_t index, mp_obj_t value);
typedef mp_obj_t (*mp_getiter_fun_t)(mp_obj_t self_in, mp_obj_iter_buf_t *iter_buf);
typedef mp_fun_1_t mp_iternext_fun_t;

// For MP_TYPE_FLAG_ITER_IS_CUSTOM, the "getiter" slot points to an instance of this type.
typedef struct _mp_getiter_iternext_custom_t {
    mp_getiter_fun_t getiter;
    mp_iternext_fun_t iternext;
} mp_getiter_iternext_custom_t;

// Buffer protocol

typedef struct _mp_buffer_info_t {
    void *buf;      // can be NULL if len == 0
    size_t len;     // in bytes
    int typecode;   // as per binary.h
} mp_buffer_info_t;

#define MP_BUFFER_READ  (1)
#define MP_BUFFER_WRITE (2)
#define MP_BUFFER_RW (MP_BUFFER_READ | MP_BUFFER_WRITE)
#define MP_BUFFER_RAISE_IF_UNSUPPORTED (4)

typedef mp_int_t (*mp_buffer_fun_t)(mp_obj_t obj, mp_buffer_info_t *bufinfo, mp_uint_t flags);

bool mp_get_buffer(mp_obj_t obj, mp_buffer_info_t *bufinfo, mp_uint_t flags);

static inline void mp_get_buffer_raise(mp_obj_t obj, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    mp_get_buffer(obj, bufinfo, flags | MP_BUFFER_RAISE_IF_UNSUPPORTED);
}

// This struct will be updated to become a variable sized struct. In order to
// use this as a member, or allocate dynamically, use the mp_obj_empty_type_t
// or mp_obj_full_type_t structs below (which must be kept in sync).
struct _mp_obj_type_t {
    // A type is an object so must start with this entry, which points to mp_type_type.
    mp_obj_base_t base;

    // Flags associated with this type.
    uint16_t flags;

    // The name of this type, a qstr.
    uint16_t name;

    // Slots: For the rest of the fields, the slot index points to the
    // relevant function in the variable-length "slots" field. Ideally these
    // would be only 4 bits, but the extra overhead of accessing them adds
    // more code, and we also need to be able to take the address of them for
    // mp_obj_class_lookup.

    // Corresponds to __new__ and __init__ special methods, to make an instance of the type.
    uint8_t slot_index_make_new;

    // Corresponds to __repr__ and __str__ special methods.
    uint8_t slot_index_print;

    // Corresponds to __call__ special method, ie T(...).
    uint8_t slot_index_call;

    // Implements unary and binary operations.
    // Can return MP_OBJ_NULL if the operation is not supported.
    uint8_t slot_index_unary_op;
    uint8_t slot_index_binary_op;

    // Implements load, store and delete attribute.
    //
    // dest[0] = MP_OBJ_NULL means load
    //  return: for fail, do nothing
    //          for fail but continue lookup in locals_dict, dest[1] = MP_OBJ_SENTINEL
    //          for attr, dest[0] = value
    //          for method, dest[0] = method, dest[1] = self
    //
    // dest[0,1] = {MP_OBJ_SENTINEL, MP_OBJ_NULL} means delete
    // dest[0,1] = {MP_OBJ_SENTINEL, object} means store
    //  return: for fail, do nothing
    //          for success set dest[0] = MP_OBJ_NULL
    uint8_t slot_index_attr;

    // Implements load, store and delete subscripting:
    //  - value = MP_OBJ_SENTINEL means load
    //  - value = MP_OBJ_NULL means delete
    //  - all other values mean store the value
    // Can return MP_OBJ_NULL if operation not supported.
    uint8_t slot_index_subscr;

    // This slot's behaviour depends on the MP_TYPE_FLAG_ITER_IS_* flags above.
    // - If MP_TYPE_FLAG_ITER_IS_GETITER flag is set, then this corresponds to the __iter__
    //   special method (of type mp_getiter_fun_t). Can use the given mp_obj_iter_buf_t
    //   to store the iterator object, otherwise can return a pointer to an object on the heap.
    // - If MP_TYPE_FLAG_ITER_IS_ITERNEXT is set, then this corresponds to __next__ special method.
    //   May return MP_OBJ_STOP_ITERATION as an optimisation instead of raising StopIteration()
    //   with no args. The type will implicitly implement getiter as "return self".
    // - If MP_TYPE_FLAG_ITER_IS_CUSTOM is set, then this slot must point to an
    //   mp_getiter_iternext_custom_t instance with both the getiter and iternext fields set.
    // - If MP_TYPE_FLAG_ITER_IS_STREAM is set, this this slot should be unset.
    uint8_t slot_index_iter;

    // Implements the buffer protocol if supported by this type.
    uint8_t slot_index_buffer;

    // One of disjoint protocols (interfaces), like mp_stream_p_t, etc.
    uint8_t slot_index_protocol;

    // A pointer to the parents of this type:
    //  - 0 parents: pointer is NULL (object is implicitly the single parent)
    //  - 1 parent: a pointer to the type of that parent
    //  - 2 or more parents: pointer to a tuple object containing the parent types
    uint8_t slot_index_parent;

    // A dict mapping qstrs to objects local methods/constants/etc.
    uint8_t slot_index_locals_dict;

    const void *slots[];
};

// Non-variable sized versions of mp_obj_type_t to be used as a member
// in other structs or for dynamic allocation. The fields are exactly
// as in mp_obj_type_t, but with a fixed size for the flexible array
// members.
typedef struct _mp_obj_empty_type_t {
    mp_obj_base_t base;
    uint16_t flags;
    uint16_t name;

    uint8_t slot_index_make_new;
    uint8_t slot_index_print;
    uint8_t slot_index_call;
    uint8_t slot_index_unary_op;
    uint8_t slot_index_binary_op;
    uint8_t slot_index_attr;
    uint8_t slot_index_subscr;
    uint8_t slot_index_iter;
    uint8_t slot_index_buffer;
    uint8_t slot_index_protocol;
    uint8_t slot_index_parent;
    uint8_t slot_index_locals_dict;

    // No slots member.
} mp_obj_empty_type_t;

typedef struct _mp_obj_full_type_t {
    mp_obj_base_t base;
    uint16_t flags;
    uint16_t name;

    uint8_t slot_index_make_new;
    uint8_t slot_index_print;
    uint8_t slot_index_call;
    uint8_t slot_index_unary_op;
    uint8_t slot_index_binary_op;
    uint8_t slot_index_attr;
    uint8_t slot_index_subscr;
    uint8_t slot_index_iter;
    uint8_t slot_index_buffer;
    uint8_t slot_index_protocol;
    uint8_t slot_index_parent;
    uint8_t slot_index_locals_dict;

    // Explicitly add 12 slots.
    const void *slots[11];
} mp_obj_full_type_t;

#define _MP_OBJ_TYPE_SLOT_TYPE_make_new (mp_make_new_fun_t)
#define _MP_OBJ_TYPE_SLOT_TYPE_print (mp_print_fun_t)
#define _MP_OBJ_TYPE_SLOT_TYPE_call (mp_call_fun_t)
#define _MP_OBJ_TYPE_SLOT_TYPE_unary_op (mp_unary_op_fun_t)
#define _MP_OBJ_TYPE_SLOT_TYPE_binary_op (mp_binary_op_fun_t)
#define _MP_OBJ_TYPE_SLOT_TYPE_attr (mp_attr_fun_t)
#define _MP_OBJ_TYPE_SLOT_TYPE_subscr (mp_subscr_fun_t)
#define _MP_OBJ_TYPE_SLOT_TYPE_iter (const void *)
#define _MP_OBJ_TYPE_SLOT_TYPE_buffer (mp_buffer_fun_t)
#define _MP_OBJ_TYPE_SLOT_TYPE_protocol (const void *)
#define _MP_OBJ_TYPE_SLOT_TYPE_parent (const void *)
#define _MP_OBJ_TYPE_SLOT_TYPE_locals_dict (struct _mp_obj_dict_t *)

// Because the mp_obj_type_t instances are in (zero-initialised) ROM, we take
// slot_index_foo=0 to mean that the slot is unset. This also simplifies checking
// if the slot is set. That means that we need to store index+1 in slot_index_foo
// though and then access it as slots[slot_index_foo - 1]. This is an implementation
// detail, the user of these macros doesn't need to be aware of it, and when using
// MP_OBJ_TYPE_OFFSETOF_SLOT you should use zero-based indexing.
#define MP_OBJ_TYPE_HAS_SLOT(t, f) ((t)->slot_index_##f)
#define MP_OBJ_TYPE_GET_SLOT(t, f) (_MP_OBJ_TYPE_SLOT_TYPE_##f(t)->slots[(t)->slot_index_##f - 1])
#define MP_OBJ_TYPE_GET_SLOT_OR_NULL(t, f) (_MP_OBJ_TYPE_SLOT_TYPE_##f(MP_OBJ_TYPE_HAS_SLOT(t, f) ? MP_OBJ_TYPE_GET_SLOT(t, f) : NULL))
#define MP_OBJ_TYPE_SET_SLOT(t, f, v, n) ((t)->slot_index_##f = (n) + 1, (t)->slots[(n)] = (void *)v)
#define MP_OBJ_TYPE_OFFSETOF_SLOT(f) (offsetof(mp_obj_type_t, slot_index_##f))
#define MP_OBJ_TYPE_HAS_SLOT_BY_OFFSET(t, offset) (*(uint8_t *)((char *)(t) + (offset)) != 0)

// Implementation of MP_DEFINE_CONST_OBJ_TYPE for each number of arguments.
// Do not use these directly, instead use MP_DEFINE_CONST_OBJ_TYPE.
// Generated with:
// for i in range(13):
//     args = ['_struct_type', '_typename', '_name', '_flags']
//     struct = ['.base = { &mp_type_type }', '.flags = _flags', '.name = _name']
//     slots = []
//     for j in range(i):
//         args += [f"f{j+1}", f"v{j+1}"]
//         struct += [f".slot_index_##f{j+1} = {j+1}"]
//         slots += [f"(const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f{j+1} v{j+1}"]
//     if slots:
//         struct += [f".slots = {{ {', '.join(slots)} }}"]
//     print(f"#define MP_DEFINE_CONST_OBJ_TYPE_NARGS_{i}({', '.join(args)}) const _struct_type _typename = {{ {', '.join(struct)} }}")
#define MP_DEFINE_CONST_OBJ_TYPE_NARGS_0(_struct_type, _typename, _name, _flags) const _struct_type _typename = { .base = { &mp_type_type }, .flags = _flags, .name = _name }
#define MP_DEFINE_CONST_OBJ_TYPE_NARGS_1(_struct_type, _typename, _name, _flags, f1, v1) const _struct_type _typename = { .base = { &mp_type_type }, .flags = _flags, .name = _name, .slot_index_##f1 = 1, .slots = { (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f1 v1 } }
#define MP_DEFINE_CONST_OBJ_TYPE_NARGS_2(_struct_type, _typename, _name, _flags, f1, v1, f2, v2) const _struct_type _typename = { .base = { &mp_type_type }, .flags = _flags, .name = _name, .slot_index_##f1 = 1, .slot_index_##f2 = 2, .slots = { (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f1 v1, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f2 v2 } }
#define MP_DEFINE_CONST_OBJ_TYPE_NARGS_3(_struct_type, _typename, _name, _flags, f1, v1, f2, v2, f3, v3) const _struct_type _typename = { .base = { &mp_type_type }, .flags = _flags, .name = _name, .slot_index_##f1 = 1, .slot_index_##f2 = 2, .slot_index_##f3 = 3, .slots = { (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f1 v1, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f2 v2, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f3 v3 } }
#define MP_DEFINE_CONST_OBJ_TYPE_NARGS_4(_struct_type, _typename, _name, _flags, f1, v1, f2, v2, f3, v3, f4, v4) const _struct_type _typename = { .base = { &mp_type_type }, .flags = _flags, .name = _name, .slot_index_##f1 = 1, .slot_index_##f2 = 2, .slot_index_##f3 = 3, .slot_index_##f4 = 4, .slots = { (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f1 v1, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f2 v2, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f3 v3, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f4 v4 } }
#define MP_DEFINE_CONST_OBJ_TYPE_NARGS_5(_struct_type, _typename, _name, _flags, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5) const _struct_type _typename = { .base = { &mp_type_type }, .flags = _flags, .name = _name, .slot_index_##f1 = 1, .slot_index_##f2 = 2, .slot_index_##f3 = 3, .slot_index_##f4 = 4, .slot_index_##f5 = 5, .slots = { (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f1 v1, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f2 v2, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f3 v3, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f4 v4, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f5 v5 } }
#define MP_DEFINE_CONST_OBJ_TYPE_NARGS_6(_struct_type, _typename, _name, _flags, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6) const _struct_type _typename = { .base = { &mp_type_type }, .flags = _flags, .name = _name, .slot_index_##f1 = 1, .slot_index_##f2 = 2, .slot_index_##f3 = 3, .slot_index_##f4 = 4, .slot_index_##f5 = 5, .slot_index_##f6 = 6, .slots = { (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f1 v1, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f2 v2, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f3 v3, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f4 v4, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f5 v5, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f6 v6 } }
#define MP_DEFINE_CONST_OBJ_TYPE_NARGS_7(_struct_type, _typename, _name, _flags, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6, f7, v7) const _struct_type _typename = { .base = { &mp_type_type }, .flags = _flags, .name = _name, .slot_index_##f1 = 1, .slot_index_##f2 = 2, .slot_index_##f3 = 3, .slot_index_##f4 = 4, .slot_index_##f5 = 5, .slot_index_##f6 = 6, .slot_index_##f7 = 7, .slots = { (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f1 v1, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f2 v2, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f3 v3, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f4 v4, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f5 v5, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f6 v6, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f7 v7 } }
#define MP_DEFINE_CONST_OBJ_TYPE_NARGS_8(_struct_type, _typename, _name, _flags, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6, f7, v7, f8, v8) const _struct_type _typename = { .base = { &mp_type_type }, .flags = _flags, .name = _name, .slot_index_##f1 = 1, .slot_index_##f2 = 2, .slot_index_##f3 = 3, .slot_index_##f4 = 4, .slot_index_##f5 = 5, .slot_index_##f6 = 6, .slot_index_##f7 = 7, .slot_index_##f8 = 8, .slots = { (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f1 v1, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f2 v2, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f3 v3, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f4 v4, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f5 v5, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f6 v6, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f7 v7, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f8 v8 } }
#define MP_DEFINE_CONST_OBJ_TYPE_NARGS_9(_struct_type, _typename, _name, _flags, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6, f7, v7, f8, v8, f9, v9) const _struct_type _typename = { .base = { &mp_type_type }, .flags = _flags, .name = _name, .slot_index_##f1 = 1, .slot_index_##f2 = 2, .slot_index_##f3 = 3, .slot_index_##f4 = 4, .slot_index_##f5 = 5, .slot_index_##f6 = 6, .slot_index_##f7 = 7, .slot_index_##f8 = 8, .slot_index_##f9 = 9, .slots = { (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f1 v1, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f2 v2, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f3 v3, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f4 v4, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f5 v5, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f6 v6, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f7 v7, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f8 v8, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f9 v9 } }
#define MP_DEFINE_CONST_OBJ_TYPE_NARGS_10(_struct_type, _typename, _name, _flags, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6, f7, v7, f8, v8, f9, v9, f10, v10) const _struct_type _typename = { .base = { &mp_type_type }, .flags = _flags, .name = _name, .slot_index_##f1 = 1, .slot_index_##f2 = 2, .slot_index_##f3 = 3, .slot_index_##f4 = 4, .slot_index_##f5 = 5, .slot_index_##f6 = 6, .slot_index_##f7 = 7, .slot_index_##f8 = 8, .slot_index_##f9 = 9, .slot_index_##f10 = 10, .slots = { (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f1 v1, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f2 v2, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f3 v3, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f4 v4, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f5 v5, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f6 v6, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f7 v7, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f8 v8, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f9 v9, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f10 v10 } }
#define MP_DEFINE_CONST_OBJ_TYPE_NARGS_11(_struct_type, _typename, _name, _flags, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6, f7, v7, f8, v8, f9, v9, f10, v10, f11, v11) const _struct_type _typename = { .base = { &mp_type_type }, .flags = _flags, .name = _name, .slot_index_##f1 = 1, .slot_index_##f2 = 2, .slot_index_##f3 = 3, .slot_index_##f4 = 4, .slot_index_##f5 = 5, .slot_index_##f6 = 6, .slot_index_##f7 = 7, .slot_index_##f8 = 8, .slot_index_##f9 = 9, .slot_index_##f10 = 10, .slot_index_##f11 = 11, .slots = { (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f1 v1, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f2 v2, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f3 v3, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f4 v4, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f5 v5, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f6 v6, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f7 v7, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f8 v8, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f9 v9, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f10 v10, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f11 v11 } }
#define MP_DEFINE_CONST_OBJ_TYPE_NARGS_12(_struct_type, _typename, _name, _flags, f1, v1, f2, v2, f3, v3, f4, v4, f5, v5, f6, v6, f7, v7, f8, v8, f9, v9, f10, v10, f11, v11, f12, v12) const _struct_type _typename = { .base = { &mp_type_type }, .flags = _flags, .name = _name, .slot_index_##f1 = 1, .slot_index_##f2 = 2, .slot_index_##f3 = 3, .slot_index_##f4 = 4, .slot_index_##f5 = 5, .slot_index_##f6 = 6, .slot_index_##f7 = 7, .slot_index_##f8 = 8, .slot_index_##f9 = 9, .slot_index_##f10 = 10, .slot_index_##f11 = 11, .slot_index_##f12 = 12, .slots = { (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f1 v1, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f2 v2, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f3 v3, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f4 v4, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f5 v5, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f6 v6, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f7 v7, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f8 v8, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f9 v9, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f10 v10, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f11 v11, (const void *)_MP_OBJ_TYPE_SLOT_TYPE_##f12 v12 } }

// Workaround for https://docs.microsoft.com/en-us/cpp/preprocessor/preprocessor-experimental-overview?view=msvc-160#macro-arguments-are-unpacked
#define MP_DEFINE_CONST_OBJ_TYPE_EXPAND(x) x

// This macro evaluates to MP_DEFINE_CONST_OBJ_TYPE_NARGS_##N, where N is the value
// of the 29th argument (29 is 13*2 + 3).
#define MP_DEFINE_CONST_OBJ_TYPE_NARGS(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, N, ...) MP_DEFINE_CONST_OBJ_TYPE_NARGS_##N

// This macros is used to define a object type in ROM.
// Invoke as MP_DEFINE_CONST_OBJ_TYPE(_typename, _name, _flags, [, slot, func]*)
// It uses the number of arguments to select which MP_DEFINE_CONST_OBJ_TYPE_*
// macro to use based on the number of arguments. It works by shifting the
// numeric values 12, 11, ... 0 by the number of arguments, such that the
// 29th argument ends up being the number to use. The _INV values are
// placeholders because the slot arguments come in pairs.
#define MP_DEFINE_CONST_OBJ_TYPE(...) MP_DEFINE_CONST_OBJ_TYPE_EXPAND(MP_DEFINE_CONST_OBJ_TYPE_NARGS(__VA_ARGS__, _INV, 12, _INV, 11, _INV, 10, _INV, 9, _INV, 8, _INV, 7, _INV, 6, _INV, 5, _INV, 4, _INV, 3, _INV, 2, _INV, 1, _INV, 0)(mp_obj_type_t, __VA_ARGS__))

// Constant types, globally accessible
extern const mp_obj_type_t mp_type_type;
extern const mp_obj_type_t mp_type_object;
extern const mp_obj_type_t mp_type_NoneType;
extern const mp_obj_type_t mp_type_bool;
extern const mp_obj_type_t mp_type_int;
extern const mp_obj_type_t mp_type_str;
extern const mp_obj_type_t mp_type_template;
extern const mp_obj_type_t mp_type_interpolation;
extern const mp_obj_type_t mp_type_bytes;
extern const mp_obj_type_t mp_type_bytearray;
extern const mp_obj_type_t mp_type_memoryview;
extern const mp_obj_type_t mp_type_float;
extern const mp_obj_type_t mp_type_complex;
extern const mp_obj_type_t mp_type_tuple;
extern const mp_obj_type_t mp_type_list;
extern const mp_obj_type_t mp_type_map; // map (the python builtin, not the dict implementation detail)
extern const mp_obj_type_t mp_type_enumerate;
extern const mp_obj_type_t mp_type_filter;
extern const mp_obj_type_t mp_type_deque;
extern const mp_obj_type_t mp_type_dict;
extern const mp_obj_type_t mp_type_ordereddict;
extern const mp_obj_type_t mp_type_range;
extern const mp_obj_type_t mp_type_set;
extern const mp_obj_type_t mp_type_frozenset;
extern const mp_obj_type_t mp_type_slice;
extern const mp_obj_type_t mp_type_zip;
extern const mp_obj_type_t mp_type_array;
extern const mp_obj_type_t mp_type_super;
extern const mp_obj_type_t mp_type_gen_wrap;
extern const mp_obj_type_t mp_type_native_gen_wrap;
extern const mp_obj_type_t mp_type_gen_instance;
extern const mp_obj_type_t mp_type_fun_builtin_0;
extern const mp_obj_type_t mp_type_fun_builtin_1;
extern const mp_obj_type_t mp_type_fun_builtin_2;
extern const mp_obj_type_t mp_type_fun_builtin_3;
extern const mp_obj_type_t mp_type_fun_builtin_var;
extern const mp_obj_type_t mp_type_fun_bc;
extern const mp_obj_type_t mp_type_fun_native;
extern const mp_obj_type_t mp_type_fun_viper;
extern const mp_obj_type_t mp_type_fun_asm;
extern const mp_obj_type_t mp_type_code;
extern const mp_obj_type_t mp_type_module;
extern const mp_obj_type_t mp_type_staticmethod;
extern const mp_obj_type_t mp_type_classmethod;
extern const mp_obj_type_t mp_type_bound_meth;
extern const mp_obj_type_t mp_type_property;
extern const mp_obj_type_t mp_type_stringio;
extern const mp_obj_type_t mp_type_bytesio;
extern const mp_obj_type_t mp_type_ringio;
extern const mp_obj_type_t mp_type_reversed;
extern const mp_obj_type_t mp_type_polymorph_iter;
#if MICROPY_ENABLE_FINALISER
extern const mp_obj_type_t mp_type_polymorph_iter_with_finaliser;
#endif

// Exceptions
extern const mp_obj_type_t mp_type_BaseException;
extern const mp_obj_type_t mp_type_ArithmeticError;
extern const mp_obj_type_t mp_type_AssertionError;
extern const mp_obj_type_t mp_type_AttributeError;
extern const mp_obj_type_t mp_type_EOFError;
extern const mp_obj_type_t mp_type_Exception;
extern const mp_obj_type_t mp_type_GeneratorExit;
extern const mp_obj_type_t mp_type_ImportError;
extern const mp_obj_type_t mp_type_IndentationError;
extern const mp_obj_type_t mp_type_IndexError;
extern const mp_obj_type_t mp_type_KeyboardInterrupt;
extern const mp_obj_type_t mp_type_KeyError;
extern const mp_obj_type_t mp_type_LookupError;
extern const mp_obj_type_t mp_type_MemoryError;
extern const mp_obj_type_t mp_type_NameError;
extern const mp_obj_type_t mp_type_NotImplementedError;
extern const mp_obj_type_t mp_type_OSError;
extern const mp_obj_type_t mp_type_OverflowError;
extern const mp_obj_type_t mp_type_RuntimeError;
extern const mp_obj_type_t mp_type_StopAsyncIteration;
extern const mp_obj_type_t mp_type_StopIteration;
extern const mp_obj_type_t mp_type_SyntaxError;
extern const mp_obj_type_t mp_type_SystemExit;
extern const mp_obj_type_t mp_type_TypeError;
extern const mp_obj_type_t mp_type_UnicodeError;
extern const mp_obj_type_t mp_type_ValueError;
extern const mp_obj_type_t mp_type_ViperTypeError;
extern const mp_obj_type_t mp_type_ZeroDivisionError;

// Constant objects, globally accessible: None, False, True
// These should always be accessed via the below macros.
#if MICROPY_OBJ_IMMEDIATE_OBJS
// None is even while False/True are odd so their types can be distinguished with 1 bit.
#define mp_const_none MP_OBJ_NEW_IMMEDIATE_OBJ(0)
#define mp_const_false MP_OBJ_NEW_IMMEDIATE_OBJ(1)
#define mp_const_true MP_OBJ_NEW_IMMEDIATE_OBJ(3)
#else
#define mp_const_none (MP_OBJ_FROM_PTR(&mp_const_none_obj))
#define mp_const_false (MP_OBJ_FROM_PTR(&mp_const_false_obj))
#define mp_const_true (MP_OBJ_FROM_PTR(&mp_const_true_obj))
extern const struct _mp_obj_none_t mp_const_none_obj;
extern const struct _mp_obj_bool_t mp_const_false_obj;
extern const struct _mp_obj_bool_t mp_const_true_obj;
#endif

// Constant objects, globally accessible: b'', (), {}, Ellipsis, NotImplemented, GeneratorExit()
// The below macros are for convenience only.
#define mp_const_empty_bytes (MP_OBJ_FROM_PTR(&mp_const_empty_bytes_obj))
#define mp_const_empty_tuple (MP_OBJ_FROM_PTR(&mp_const_empty_tuple_obj))
#define mp_const_notimplemented (MP_OBJ_FROM_PTR(&mp_const_notimplemented_obj))
extern const struct _mp_obj_str_t mp_const_empty_bytes_obj;
extern const struct _mp_obj_tuple_t mp_const_empty_tuple_obj;
extern const struct _mp_obj_dict_t mp_const_empty_dict_obj;
extern const struct _mp_obj_singleton_t mp_const_ellipsis_obj;
extern const struct _mp_obj_singleton_t mp_const_notimplemented_obj;
extern const struct _mp_obj_exception_t mp_const_GeneratorExit_obj;

// Fixed empty map. Useful when calling keyword-receiving functions
// without any keywords from C, etc.
#define mp_const_empty_map (mp_const_empty_dict_obj.map)

// General API for objects

// Helper versions of m_new_obj when you need to immediately set base.type.
// Implementing this as a call rather than inline saves 8 bytes per usage.
#define mp_obj_malloc(struct_type, obj_type) ((struct_type *)mp_obj_malloc_helper(sizeof(struct_type), obj_type))
#define mp_obj_malloc_var(struct_type, var_field, var_type, var_num, obj_type) ((struct_type *)mp_obj_malloc_helper(offsetof(struct_type, var_field) + sizeof(var_type) * (var_num), obj_type))
void *mp_obj_malloc_helper(size_t num_bytes, const mp_obj_type_t *type);

// Object allocation macros for allocating objects that have a finaliser.
#if MICROPY_ENABLE_FINALISER
#define mp_obj_malloc_with_finaliser(struct_type, obj_type) ((struct_type *)mp_obj_malloc_with_finaliser_helper(sizeof(struct_type), obj_type))
#define mp_obj_malloc_var_with_finaliser(struct_type, var_field, var_type, var_num, obj_type) ((struct_type *)mp_obj_malloc_with_finaliser_helper(offsetof(struct_type, var_field) + sizeof(var_type) * (var_num), obj_type))
void *mp_obj_malloc_with_finaliser_helper(size_t num_bytes, const mp_obj_type_t *type);
#else
#define mp_obj_malloc_with_finaliser(struct_type, obj_type) mp_obj_malloc(struct_type, obj_type)
#define mp_obj_malloc_var_with_finaliser(struct_type, var_field, var_type, var_num, obj_type) mp_obj_malloc_var(struct_type, var_field, var_type, var_num, obj_type)
#endif

// These macros are derived from more primitive ones and are used to
// check for more specific object types.
// Note: these are kept as macros because inline functions sometimes use much
// more code space than the equivalent macros, depending on the compiler.
// don't use mp_obj_is_exact_type directly; use mp_obj_is_type which provides additional safety checks.
// use the former only if you need to bypass these checks (because you've already checked everything else)
#define mp_obj_is_exact_type(o, t) (mp_obj_is_obj(o) && (((mp_obj_base_t *)MP_OBJ_TO_PTR(o))->type == (t)))

// Type checks are split to a separate, constant result macro. This is so it doesn't hinder the compilers's
// optimizations (other tricks like using ({ expr; exper; }) or (exp, expr, expr) in mp_obj_is_type() result
// in missed optimizations)
#define mp_type_assert_not_bool_int_str_nonetype(t) (                                     \
    MP_STATIC_ASSERT_NONCONSTEXPR((t) != &mp_type_bool), assert((t) != &mp_type_bool),         \
    MP_STATIC_ASSERT_NONCONSTEXPR((t) != &mp_type_int), assert((t) != &mp_type_int),           \
    MP_STATIC_ASSERT_NONCONSTEXPR((t) != &mp_type_str), assert((t) != &mp_type_str),           \
    MP_STATIC_ASSERT_NONCONSTEXPR((t) != &mp_type_NoneType), assert((t) != &mp_type_NoneType), \
    1)
#if MICROPY_PY_BUILTINS_FLOAT
#define mp_type_assert_not_float(t) (MP_STATIC_ASSERT_NONCONSTEXPR((t) != &mp_type_float), assert((t) != &mp_type_float), 1)
#else
#define mp_type_assert_not_float(t) (1)
#endif

#define mp_obj_is_type(o, t) (mp_type_assert_not_bool_int_str_nonetype(t) && mp_type_assert_not_float(t) && mp_obj_is_exact_type(o, t))
#if MICROPY_OBJ_IMMEDIATE_OBJS
// bool's are immediates, not real objects, so test for the 2 possible values.
#define mp_obj_is_bool(o) ((o) == mp_const_false || (o) == mp_const_true)
#else
#define mp_obj_is_bool(o) mp_obj_is_exact_type(o, &mp_type_bool)
#endif
#define mp_obj_is_int(o) (mp_obj_is_small_int(o) || mp_obj_is_exact_type(o, &mp_type_int))
#define mp_obj_is_str(o) (mp_obj_is_qstr(o) || mp_obj_is_exact_type(o, &mp_type_str))
#define mp_obj_is_str_or_bytes(o) (mp_obj_is_qstr(o) || (mp_obj_is_obj(o) && MP_OBJ_TYPE_GET_SLOT_OR_NULL(((mp_obj_base_t *)MP_OBJ_TO_PTR(o))->type, binary_op) == mp_obj_str_binary_op))
bool mp_obj_is_dict_or_ordereddict(mp_obj_t o);
#define mp_obj_is_fun(o) (mp_obj_is_obj(o) && (((mp_obj_base_t *)MP_OBJ_TO_PTR(o))->type->name == MP_QSTR_function))

static inline mp_obj_t mp_obj_new_bool(mp_int_t x) {
    return x ? mp_const_true : mp_const_false;
}
mp_obj_t mp_obj_new_cell(mp_obj_t obj);
mp_obj_t mp_obj_new_int(mp_int_t value);
mp_obj_t mp_obj_new_int_from_uint(mp_uint_t value);
mp_obj_t mp_obj_new_int_from_str_len(const char **str, size_t len, bool neg, unsigned int base);
mp_obj_t mp_obj_new_int_from_ll(long long val); // this must return a multi-precision integer object (or raise an overflow exception)
mp_obj_t mp_obj_new_int_from_ull(unsigned long long val); // this must return a multi-precision integer object (or raise an overflow exception)
mp_obj_t mp_obj_new_str(const char *data, size_t len); // will check utf-8 (raises UnicodeError)
mp_obj_t mp_obj_new_str_from_cstr(const char *str); // // accepts null-terminated string, will check utf-8 (raises UnicodeError)
mp_obj_t mp_obj_new_str_via_qstr(const char *data, size_t len); // input data must be valid utf-8
mp_obj_t mp_obj_new_str_from_vstr(vstr_t *vstr); // will check utf-8 (raises UnicodeError)
#if MICROPY_PY_BUILTINS_STR_UNICODE && MICROPY_PY_BUILTINS_STR_UNICODE_CHECK
mp_obj_t mp_obj_new_str_from_utf8_vstr(vstr_t *vstr); // input data must be valid utf-8
#else
#define mp_obj_new_str_from_utf8_vstr mp_obj_new_str_from_vstr
#endif
mp_obj_t mp_obj_new_bytes_from_vstr(vstr_t *vstr);
mp_obj_t mp_obj_new_bytes(const byte *data, size_t len);
mp_obj_t mp_obj_new_bytearray(size_t n, const void *items);
mp_obj_t mp_obj_new_bytearray_by_ref(size_t n, void *items);
#if MICROPY_PY_TSTRINGS
mp_obj_t mp_obj_new_template(size_t n_args, const mp_obj_t *args);
#endif
#if MICROPY_PY_BUILTINS_FLOAT
mp_obj_t mp_obj_new_int_from_float(mp_float_t val);
mp_obj_t mp_obj_new_complex(mp_float_t real, mp_float_t imag);
#endif
mp_obj_t mp_obj_new_exception(const mp_obj_type_t *exc_type);
mp_obj_t mp_obj_new_exception_args(const mp_obj_type_t *exc_type, size_t n_args, const mp_obj_t *args);
#if MICROPY_ERROR_REPORTING == MICROPY_ERROR_REPORTING_NONE
#define mp_obj_new_exception_msg(exc_type, msg) mp_obj_new_exception(exc_type)
#define mp_obj_new_exception_msg_varg(exc_type, ...) mp_obj_new_exception(exc_type)
#else
mp_obj_t mp_obj_new_exception_msg(const mp_obj_type_t *exc_type, mp_rom_error_text_t msg);
mp_obj_t mp_obj_new_exception_msg_varg(const mp_obj_type_t *exc_type, mp_rom_error_text_t fmt, ...); // counts args by number of % symbols in fmt, excluding %%; can only handle void* sizes (ie no float/double!)
#endif
#ifdef va_start
mp_obj_t mp_obj_new_exception_msg_vlist(const mp_obj_type_t *exc_type, mp_rom_error_text_t fmt, va_list arg); // same fmt restrictions as above
#endif
mp_obj_t mp_obj_new_gen_wrap(mp_obj_t fun);
mp_obj_t mp_obj_new_closure(mp_obj_t fun, size_t n_closed, const mp_obj_t *closed);
mp_obj_t mp_obj_new_tuple(size_t n, const mp_obj_t *items);
mp_obj_t mp_obj_new_list(size_t n, mp_obj_t *items);
mp_obj_t mp_obj_new_dict(size_t n_args);
mp_obj_t mp_obj_new_set(size_t n_args, mp_obj_t *items);
mp_obj_t mp_obj_new_slice(mp_obj_t start, mp_obj_t stop, mp_obj_t step);
mp_obj_t mp_obj_new_bound_meth(mp_obj_t meth, mp_obj_t self);
mp_obj_t mp_obj_new_getitem_iter(mp_obj_t *args, mp_obj_iter_buf_t *iter_buf);
mp_obj_t mp_obj_new_module(qstr module_name);
mp_obj_t mp_obj_new_memoryview(byte typecode, size_t nitems, void *items);

const mp_obj_type_t *mp_obj_get_type(mp_const_obj_t o_in);
const char *mp_obj_get_type_str(mp_const_obj_t o_in);
bool mp_obj_is_subclass_fast(mp_const_obj_t object, mp_const_obj_t classinfo); // arguments should be type objects
mp_obj_t mp_obj_cast_to_native_base(mp_obj_t self_in, mp_const_obj_t native_type);

void mp_obj_print_helper(const mp_print_t *print, mp_obj_t o_in, mp_print_kind_t kind);
void mp_obj_print(mp_obj_t o, mp_print_kind_t kind);
void mp_obj_print_exception(const mp_print_t *print, mp_obj_t exc);

bool mp_obj_is_true(mp_obj_t arg);
bool mp_obj_is_callable(mp_obj_t o_in);
mp_obj_t mp_obj_equal_not_equal(mp_binary_op_t op, mp_obj_t o1, mp_obj_t o2);
bool mp_obj_equal(mp_obj_t o1, mp_obj_t o2);

// returns true if o is bool, small int or long int
static inline bool mp_obj_is_integer(mp_const_obj_t o) {
    return mp_obj_is_int(o) || mp_obj_is_bool(o);
}

mp_int_t mp_obj_get_int(mp_const_obj_t arg);
mp_uint_t mp_obj_get_uint(mp_const_obj_t arg);
long long mp_obj_get_ll(mp_const_obj_t arg);
mp_int_t mp_obj_get_int_truncated(mp_const_obj_t arg);
bool mp_obj_get_int_maybe(mp_const_obj_t arg, mp_int_t *value);
#if MICROPY_PY_BUILTINS_FLOAT
mp_float_t mp_obj_get_float(mp_obj_t self_in);
bool mp_obj_get_float_maybe(mp_obj_t arg, mp_float_t *value);
void mp_obj_get_complex(mp_obj_t self_in, mp_float_t *real, mp_float_t *imag);
bool mp_obj_get_complex_maybe(mp_obj_t self_in, mp_float_t *real, mp_float_t *imag);
#endif
void mp_obj_get_array(mp_obj_t o, size_t *len, mp_obj_t **items); // *items may point inside a GC block
void mp_obj_get_array_fixed_n(mp_obj_t o, size_t len, mp_obj_t **items); // *items may point inside a GC block
size_t mp_get_index(const mp_obj_type_t *type, size_t len, mp_obj_t index, bool is_slice);
mp_obj_t mp_obj_id(mp_obj_t o_in);
mp_obj_t mp_obj_len(mp_obj_t o_in);
mp_obj_t mp_obj_len_maybe(mp_obj_t o_in); // may return MP_OBJ_NULL
mp_obj_t mp_obj_subscr(mp_obj_t base, mp_obj_t index, mp_obj_t val);

// cell

typedef struct _mp_obj_cell_t {
    mp_obj_base_t base;
    mp_obj_t obj;
} mp_obj_cell_t;

static inline mp_obj_t mp_obj_cell_get(mp_obj_t self_in) {
    mp_obj_cell_t *self = (mp_obj_cell_t *)MP_OBJ_TO_PTR(self_in);
    return self->obj;
}

static inline void mp_obj_cell_set(mp_obj_t self_in, mp_obj_t obj) {
    mp_obj_cell_t *self = (mp_obj_cell_t *)MP_OBJ_TO_PTR(self_in);
    self->obj = obj;
}

// int
// For long int, returns value truncated to mp_int_t
mp_int_t mp_obj_int_get_truncated(mp_const_obj_t self_in);
// Will raise exception if value doesn't fit into mp_int_t
mp_int_t mp_obj_int_get_checked(mp_const_obj_t self_in);
// Will raise exception if value is negative or doesn't fit into mp_uint_t
mp_uint_t mp_obj_int_get_uint_checked(mp_const_obj_t self_in);

// exception
bool mp_obj_is_native_exception_instance(mp_obj_t self_in);
bool mp_obj_is_exception_type(mp_obj_t self_in);
bool mp_obj_is_exception_instance(mp_obj_t self_in);
bool mp_obj_exception_match(mp_obj_t exc, mp_const_obj_t exc_type);
void mp_obj_exception_clear_traceback(mp_obj_t self_in);
void mp_obj_exception_add_traceback(mp_obj_t self_in, qstr file, size_t line, qstr block);
void mp_obj_exception_get_traceback(mp_obj_t self_in, size_t *n, size_t **values);
mp_obj_t mp_obj_exception_get_value(mp_obj_t self_in);
mp_obj_t mp_obj_exception_make_new(const mp_obj_type_t *type_in, size_t n_args, size_t n_kw, const mp_obj_t *args);
mp_obj_t mp_alloc_emergency_exception_buf(mp_obj_t size_in);
void mp_init_emergency_exception_buf(void);
static inline mp_obj_t mp_obj_new_exception_arg1(const mp_obj_type_t *exc_type, mp_obj_t arg) {
    assert(MP_OBJ_TYPE_GET_SLOT_OR_NULL(exc_type, make_new) == mp_obj_exception_make_new);
    return mp_obj_exception_make_new(exc_type, 1, 0, &arg);
}

// str
bool mp_obj_str_equal(mp_obj_t s1, mp_obj_t s2);
qstr mp_obj_str_get_qstr(mp_obj_t self_in); // use this if you will anyway convert the string to a qstr
const char *mp_obj_str_get_str(mp_obj_t self_in); // use this only if you need the string to be null terminated
const char *mp_obj_str_get_data(mp_obj_t self_in, size_t *len);
mp_obj_t mp_obj_str_intern(mp_obj_t str);
mp_obj_t mp_obj_str_intern_checked(mp_obj_t obj);
void mp_str_print_quoted(const mp_print_t *print, const byte *str_data, size_t str_len, bool is_bytes);

#if MICROPY_PY_BUILTINS_FLOAT
// float
#if MICROPY_FLOAT_IMPL == MICROPY_FLOAT_IMPL_FLOAT
static inline float mp_obj_get_float_to_f(mp_obj_t o) {
    return mp_obj_get_float(o);
}

static inline double mp_obj_get_float_to_d(mp_obj_t o) {
    return (double)mp_obj_get_float(o);
}

static inline mp_obj_t mp_obj_new_float_from_f(float o) {
    return mp_obj_new_float(o);
}

static inline mp_obj_t mp_obj_new_float_from_d(double o) {
    return mp_obj_new_float((mp_float_t)o);
}
#elif MICROPY_FLOAT_IMPL == MICROPY_FLOAT_IMPL_DOUBLE
static inline float mp_obj_get_float_to_f(mp_obj_t o) {
    return (float)mp_obj_get_float(o);
}

static inline double mp_obj_get_float_to_d(mp_obj_t o) {
    return mp_obj_get_float(o);
}

static inline mp_obj_t mp_obj_new_float_from_f(float o) {
    return mp_obj_new_float((mp_float_t)o);
}

static inline mp_obj_t mp_obj_new_float_from_d(double o) {
    return mp_obj_new_float(o);
}
#endif
#if MICROPY_FLOAT_HIGH_QUALITY_HASH
mp_int_t mp_float_hash(mp_float_t val);
#else
static inline mp_int_t mp_float_hash(mp_float_t val) {
    return (mp_int_t)val;
}
#endif
mp_obj_t mp_obj_float_binary_op(mp_binary_op_t op, mp_float_t lhs_val, mp_obj_t rhs); // can return MP_OBJ_NULL if op not supported

// complex
void mp_obj_complex_get(mp_obj_t self_in, mp_float_t *real, mp_float_t *imag);
mp_obj_t mp_obj_complex_binary_op(mp_binary_op_t op, mp_float_t lhs_real, mp_float_t lhs_imag, mp_obj_t rhs_in); // can return MP_OBJ_NULL if op not supported
#else
#define mp_obj_is_float(o) (false)
#endif

// dict
typedef struct _mp_obj_dict_t {
    mp_obj_base_t base;
    mp_map_t map;
} mp_obj_dict_t;
mp_obj_t mp_obj_dict_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
void mp_obj_dict_init(mp_obj_dict_t *dict, size_t n_args);
size_t mp_obj_dict_len(mp_obj_t self_in);
mp_obj_t mp_obj_dict_get(mp_obj_t self_in, mp_obj_t index);
mp_obj_t mp_obj_dict_store(mp_obj_t self_in, mp_obj_t key, mp_obj_t value);
mp_obj_t mp_obj_dict_delete(mp_obj_t self_in, mp_obj_t key);
mp_obj_t mp_obj_dict_copy(mp_obj_t self_in);
static inline mp_map_t *mp_obj_dict_get_map(mp_obj_t dict) {
    return &((mp_obj_dict_t *)MP_OBJ_TO_PTR(dict))->map;
}

// set
void mp_obj_set_store(mp_obj_t self_in, mp_obj_t item);

// slice indexes resolved to particular sequence
typedef struct {
    mp_int_t start;
    mp_int_t stop;
    mp_int_t step;
} mp_bound_slice_t;

// slice
typedef struct _mp_obj_slice_t {
    mp_obj_base_t base;
    mp_obj_t start;
    mp_obj_t stop;
    mp_obj_t step;
} mp_obj_slice_t;
void mp_obj_slice_indices(mp_obj_t self_in, mp_int_t length, mp_bound_slice_t *result);

// functions

typedef struct _mp_obj_fun_builtin_fixed_t {
    mp_obj_base_t base;
    union {
        mp_fun_0_t _0;
        mp_fun_1_t _1;
        mp_fun_2_t _2;
        mp_fun_3_t _3;
    } fun;
} mp_obj_fun_builtin_fixed_t;

typedef struct _mp_obj_fun_builtin_var_t {
    mp_obj_base_t base;
    uint32_t sig; // see MP_OBJ_FUN_MAKE_SIG
    union {
        mp_fun_var_t var;
        mp_fun_kw_t kw;
    } fun;
} mp_obj_fun_builtin_var_t;

mp_obj_t mp_identity(mp_obj_t self);
MP_DECLARE_CONST_FUN_OBJ_1(mp_identity_obj);

// module
typedef struct _mp_obj_module_t {
    mp_obj_base_t base;
    mp_obj_dict_t *globals;
} mp_obj_module_t;

// staticmethod and classmethod types; defined here so we can make const versions
// this structure is used for instances of both staticmethod and classmethod
typedef struct _mp_obj_static_class_method_t {
    mp_obj_base_t base;
    mp_obj_t fun;
} mp_obj_static_class_method_t;
typedef struct _mp_rom_obj_static_class_method_t {
    mp_obj_base_t base;
    mp_rom_obj_t fun;
} mp_rom_obj_static_class_method_t;

// property
const mp_obj_t *mp_obj_property_get(mp_obj_t self_in);

// sequence helpers

void mp_seq_multiply(const void *items, size_t item_sz, size_t len, size_t times, void *dest);
#if MICROPY_PY_BUILTINS_SLICE
bool mp_seq_get_fast_slice_indexes(mp_uint_t len, mp_obj_t slice, mp_bound_slice_t *indexes);
#endif
#define mp_seq_copy(dest, src, len, item_t) memcpy(dest, src, len * sizeof(item_t))
#define mp_seq_cat(dest, src1, len1, src2, len2, item_t) { memcpy(dest, src1, (len1) * sizeof(item_t)); memcpy(dest + (len1), src2, (len2) * sizeof(item_t)); }
bool mp_seq_cmp_bytes(mp_uint_t op, const byte *data1, size_t len1, const byte *data2, size_t len2);
bool mp_seq_cmp_objs(mp_uint_t op, const mp_obj_t *items1, size_t len1, const mp_obj_t *items2, size_t len2);
mp_obj_t mp_seq_index_obj(const mp_obj_t *items, size_t len, size_t n_args, const mp_obj_t *args);
mp_obj_t mp_seq_count_obj(const mp_obj_t *items, size_t len, mp_obj_t value);
mp_obj_t mp_seq_extract_slice(const mp_obj_t *seq, mp_bound_slice_t *indexes);

// Helper to clear stale pointers from allocated, but unused memory, to preclude GC problems
#define mp_seq_clear(start, len, alloc_len, item_sz) memset((byte *)(start) + (len) * (item_sz), 0, ((alloc_len) - (len)) * (item_sz))

// Note: dest and slice regions may overlap
#define mp_seq_replace_slice_no_grow(dest, dest_len, beg, end, slice, slice_len, item_sz) \
    memmove(((char *)dest) + (beg) * (item_sz), slice, slice_len * (item_sz)); \
    memmove(((char *)dest) + (beg + slice_len) * (item_sz), ((char *)dest) + (end) * (item_sz), (dest_len - end) * (item_sz));

// Note: dest and slice regions may overlap
#define mp_seq_replace_slice_grow_inplace(dest, dest_len, beg, end, slice, slice_len, len_adj, item_sz) \
    memmove(((char *)dest) + (beg + slice_len) * (item_sz), ((char *)dest) + (end) * (item_sz), ((dest_len) + (len_adj) - ((beg) + (slice_len))) * (item_sz)); \
    memmove(((char *)dest) + (beg) * (item_sz), slice, slice_len * (item_sz));

#if !MICROPY_PREVIEW_VERSION_2

// Provide translation for legacy API
#define MP_OBJ_IS_SMALL_INT mp_obj_is_small_int
#define MP_OBJ_IS_QSTR mp_obj_is_qstr
#define MP_OBJ_IS_OBJ mp_obj_is_obj
#define MP_OBJ_IS_INT mp_obj_is_int
#define MP_OBJ_IS_TYPE mp_obj_is_type
#define MP_OBJ_IS_STR mp_obj_is_str
#define MP_OBJ_IS_STR_OR_BYTES mp_obj_is_str_or_bytes
#define MP_OBJ_IS_FUN mp_obj_is_fun
#define MP_MAP_SLOT_IS_FILLED mp_map_slot_is_filled
#define MP_SET_SLOT_IS_FILLED mp_set_slot_is_filled

#endif

#endif // MICROPY_INCLUDED_PY_OBJ_H
/* py/objlist.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_OBJLIST_H
#define MICROPY_INCLUDED_PY_OBJLIST_H


typedef struct _mp_obj_list_t {
    mp_obj_base_t base;
    size_t alloc;
    size_t len;
    mp_obj_t *items;
} mp_obj_list_t;

void mp_obj_list_init(mp_obj_list_t *o, size_t n);
mp_obj_t mp_obj_list_make_new(const mp_obj_type_t *type_in, size_t n_args, size_t n_kw, const mp_obj_t *args);
mp_obj_t mp_obj_list_append(mp_obj_t self_in, mp_obj_t arg);
mp_obj_t mp_obj_list_sort(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs);
mp_obj_t mp_obj_list_remove(mp_obj_t self_in, mp_obj_t value);

static inline void mp_obj_list_get(mp_obj_t self_in, size_t *len, mp_obj_t **items) {
    mp_obj_list_t *self = (mp_obj_list_t *)MP_OBJ_TO_PTR(self_in);
    *len = self->len;
    *items = self->items;
}

static inline void mp_obj_list_set_len(mp_obj_t self_in, size_t len) {
    // trust that the caller knows what it's doing
    // TODO realloc if len got much smaller than alloc
    mp_obj_list_t *self = (mp_obj_list_t *)MP_OBJ_TO_PTR(self_in);
    self->len = len;
}

static inline void mp_obj_list_store(mp_obj_t self_in, mp_obj_t index, mp_obj_t value) {
    mp_obj_list_t *self = (mp_obj_list_t *)MP_OBJ_TO_PTR(self_in);
    size_t i = mp_get_index(self->base.type, self->len, index, false);
    self->items[i] = value;
}

// Helper function for pattern of an optional argument which can be a list of a specified size, and is
// allocated on-demand otherwise
mp_obj_list_t *mp_obj_list_optional_arg(mp_obj_t arg_in, size_t min_len);

// Ensure provided object is a list of minimum length min_len. Raises TypeError & ValueError otherwise.
mp_obj_list_t *mp_obj_list_ensure(mp_obj_t in, size_t min_len);

#endif // MICROPY_INCLUDED_PY_OBJLIST_H
/* py/objexcept.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_OBJEXCEPT_H
#define MICROPY_INCLUDED_PY_OBJEXCEPT_H

/* py/objtuple.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_OBJTUPLE_H
#define MICROPY_INCLUDED_PY_OBJTUPLE_H


// type check is done on getiter method to allow tuple, namedtuple, attrtuple
#define mp_obj_is_tuple_compatible(o) (MP_OBJ_TYPE_GET_SLOT_OR_NULL(mp_obj_get_type(o), iter) == mp_obj_tuple_getiter)

typedef struct _mp_obj_tuple_t {
    mp_obj_base_t base;
    size_t len;
    mp_obj_t items[];
} mp_obj_tuple_t;

typedef struct _mp_rom_obj_tuple_t {
    mp_obj_base_t base;
    size_t len;
    mp_rom_obj_t items[];
} mp_rom_obj_tuple_t;

void mp_obj_tuple_print(const mp_print_t *print, mp_obj_t o_in, mp_print_kind_t kind);
mp_obj_t mp_obj_tuple_unary_op(mp_unary_op_t op, mp_obj_t self_in);
mp_obj_t mp_obj_tuple_binary_op(mp_binary_op_t op, mp_obj_t lhs, mp_obj_t rhs);
mp_obj_t mp_obj_tuple_subscr(mp_obj_t base, mp_obj_t index, mp_obj_t value);
mp_obj_t mp_obj_tuple_getiter(mp_obj_t o_in, mp_obj_iter_buf_t *iter_buf);

static inline void mp_obj_tuple_get(mp_obj_t self_in, size_t *len, mp_obj_t **items) {
    assert(mp_obj_is_tuple_compatible(self_in));
    mp_obj_tuple_t *self = (mp_obj_tuple_t *)MP_OBJ_TO_PTR(self_in);
    *len = self->len;
    *items = &self->items[0];
}

extern const mp_obj_type_t mp_type_attrtuple;

#define MP_DEFINE_ATTRTUPLE(tuple_obj_name, fields, nitems, ...) \
    const mp_rom_obj_tuple_t tuple_obj_name = { \
        .base = {.type = &mp_type_attrtuple}, \
        .len = nitems, \
        .items = { __VA_ARGS__, MP_ROM_PTR((void *)fields) } \
    }

#if MICROPY_PY_COLLECTIONS
void mp_obj_attrtuple_print_helper(const mp_print_t *print, const qstr *fields, mp_obj_tuple_t *o);
#endif

mp_obj_t mp_obj_new_attrtuple(const qstr *fields, size_t n, const mp_obj_t *items);

#endif // MICROPY_INCLUDED_PY_OBJTUPLE_H

typedef struct _mp_obj_exception_t {
    mp_obj_base_t base;
    size_t traceback_alloc : (8 * sizeof(size_t) / 2);
    size_t traceback_len : (8 * sizeof(size_t) / 2);
    size_t *traceback_data;
    mp_obj_tuple_t *args;
} mp_obj_exception_t;

void mp_obj_exception_print(const mp_print_t *print, mp_obj_t o_in, mp_print_kind_t kind);
void mp_obj_exception_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest);

#define MP_DEFINE_EXCEPTION(exc_name, base_name) \
    MP_DEFINE_CONST_OBJ_TYPE(mp_type_##exc_name, MP_QSTR_##exc_name, MP_TYPE_FLAG_NONE, \
    make_new, mp_obj_exception_make_new, \
    print, mp_obj_exception_print, \
    attr, mp_obj_exception_attr, \
    parent, &mp_type_##base_name \
    );

#endif // MICROPY_INCLUDED_PY_OBJEXCEPT_H

// This file contains structures defining the state of the MicroPython
// memory system, runtime and virtual machine.  The state is a global
// variable, but in the future it is hoped that the state can become local.

#if MICROPY_PY_SYS_ATTR_DELEGATION
// Must be kept in sync with sys_mutable_keys in modsys.c.
enum {
    #if MICROPY_PY_SYS_PATH
    MP_SYS_MUTABLE_PATH,
    #endif
    #if MICROPY_PY_SYS_PS1_PS2
    MP_SYS_MUTABLE_PS1,
    MP_SYS_MUTABLE_PS2,
    #endif
    #if MICROPY_PY_SYS_TRACEBACKLIMIT
    MP_SYS_MUTABLE_TRACEBACKLIMIT,
    #endif
    MP_SYS_MUTABLE_NUM,
};
#endif // MICROPY_PY_SYS_ATTR_DELEGATION

// This structure contains dynamic configuration for the compiler.
#if MICROPY_DYNAMIC_COMPILER
typedef struct mp_dynamic_compiler_t {
    // This is used to let mpy-cross pass options to the emitter chosen with
    // `native_arch`.  The main use case for the time being is to give the
    // RV32 emitter extended information about which extensions can be
    // optionally used, in order to generate code that's better suited for the
    // hardware platform the code will run on.
    void *backend_options;
    uint8_t small_int_bits; // must be <= host small_int_bits
    uint8_t native_arch;
    uint8_t nlr_buf_num_regs;
    #if MICROPY_ENABLE_SOURCE_LINE
    bool include_source_lines;
    #endif
} mp_dynamic_compiler_t;
extern mp_dynamic_compiler_t mp_dynamic_compiler;
#endif

// These are the values for sched_state
#define MP_SCHED_IDLE (1)
#define MP_SCHED_LOCKED (-1)
#define MP_SCHED_PENDING (0) // 0 so it's a quick check in the VM

typedef struct _mp_sched_item_t {
    mp_obj_t func;
    mp_obj_t arg;
} mp_sched_item_t;

// gc_lock_depth field is a combination of the GC_COLLECT_FLAG
// bit and a lock depth shifted GC_LOCK_DEPTH_SHIFT bits left.
#if MICROPY_ENABLE_FINALISER
#define GC_COLLECT_FLAG 1
#define GC_LOCK_DEPTH_SHIFT 1
#else
// If finalisers are disabled then this check doesn't matter, as gc_lock()
// is called anywhere else that heap can't be changed. So save some code size.
#define GC_COLLECT_FLAG 0
#define GC_LOCK_DEPTH_SHIFT 0
#endif

// This structure holds information about a single contiguous area of
// memory reserved for the memory manager.
typedef struct _mp_state_mem_area_t {
    #if MICROPY_GC_SPLIT_HEAP
    struct _mp_state_mem_area_t *next;
    #endif

    byte *gc_alloc_table_start;
    size_t gc_alloc_table_byte_len;
    #if MICROPY_ENABLE_FINALISER
    byte *gc_finaliser_table_start;
    #endif
    #if MICROPY_PY_WEAKREF
    byte *gc_weakref_table_start;
    #endif
    byte *gc_pool_start;
    byte *gc_pool_end;

    size_t gc_last_free_atb_index;
    size_t gc_last_used_block; // The block ID of the highest block allocated in the area
} mp_state_mem_area_t;

// This structure hold information about the memory allocation system.
typedef struct _mp_state_mem_t {
    #if MICROPY_MEM_STATS
    size_t total_bytes_allocated;
    size_t current_bytes_allocated;
    size_t peak_bytes_allocated;
    #endif

    mp_state_mem_area_t area;
    #if MICROPY_GC_SPLIT_HEAP
    byte *area_pool_min;  // Min of all gc_pool_start values across all areas
    byte *area_pool_max;  // Max of all gc_pool_end values across all areas
    #endif

    int gc_stack_overflow;
    MICROPY_GC_STACK_ENTRY_TYPE gc_block_stack[MICROPY_ALLOC_GC_STACK_SIZE];
    #if MICROPY_GC_SPLIT_HEAP
    // Array that tracks the area for each block on gc_block_stack.
    mp_state_mem_area_t *gc_area_stack[MICROPY_ALLOC_GC_STACK_SIZE];
    #endif

    // This variable controls auto garbage collection.  If set to 0 then the
    // GC won't automatically run when gc_alloc can't find enough blocks.  But
    // you can still allocate/free memory and also explicitly call gc_collect.
    uint16_t gc_auto_collect_enabled;

    #if MICROPY_GC_ALLOC_THRESHOLD
    size_t gc_alloc_amount;
    size_t gc_alloc_threshold;
    #endif

    #if MICROPY_GC_SPLIT_HEAP
    mp_state_mem_area_t *gc_last_free_area;
    #endif

    #if MICROPY_PY_GC_COLLECT_RETVAL
    size_t gc_collected;
    #endif

    #if MICROPY_PY_THREAD && !MICROPY_PY_THREAD_GIL
    // This is a global mutex used to make the GC thread-safe.
    mp_thread_recursive_mutex_t gc_mutex;
    #endif
} mp_state_mem_t;

// This structure hold runtime and VM information.  It includes a section
// which contains root pointers that must be scanned by the GC.
typedef struct _mp_state_vm_t {
    //
    // CONTINUE ROOT POINTER SECTION
    // This must start at the start of this structure and follows
    // the state in the mp_state_thread_t structure, continuing
    // the root pointer section from there.
    //

    qstr_pool_t *last_pool;

    #if MICROPY_TRACKED_ALLOC
    struct _m_tracked_node_t *m_tracked_head;
    #endif

    // non-heap memory for creating an exception if we can't allocate RAM
    mp_obj_exception_t mp_emergency_exception_obj;

    // memory for exception arguments if we can't allocate RAM
    #if MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF
    #if MICROPY_EMERGENCY_EXCEPTION_BUF_SIZE > 0
    // statically allocated buf (needs to be aligned to mp_obj_t)
    mp_obj_t mp_emergency_exception_buf[MICROPY_EMERGENCY_EXCEPTION_BUF_SIZE / sizeof(mp_obj_t)];
    #else
    // dynamically allocated buf
    byte *mp_emergency_exception_buf;
    #endif
    #endif

    #if MICROPY_KBD_EXCEPTION
    // exception object of type KeyboardInterrupt
    mp_obj_exception_t mp_kbd_exception;
    #endif

    // dictionary with loaded modules (may be exposed as sys.modules)
    mp_obj_dict_t mp_loaded_modules_dict;

    // dictionary for the __main__ module
    mp_obj_dict_t dict_main;

    // dictionary for overridden builtins
    #if MICROPY_CAN_OVERRIDE_BUILTINS
    mp_obj_dict_t *mp_module_builtins_override_dict;
    #endif

    // Include any root pointers registered with MP_REGISTER_ROOT_POINTER().
    #ifndef NO_QSTR
    // Only include root pointer definitions when not doing qstr extraction, because
    // the qstr extraction stage also generates the root pointers header file.
/* root_pointers.h */
// Automatically generated by make_root_pointers.py.

    #endif

    //
    // END ROOT POINTER SECTION
    ////////////////////////////////////////////////////////////

    // pointer and sizes to store interned string data
    // (qstr_last_chunk can be root pointer but is also stored in qstr pool)
    char *qstr_last_chunk;
    size_t qstr_last_alloc;
    size_t qstr_last_used;

    #if MICROPY_PY_THREAD && !MICROPY_PY_THREAD_GIL
    // This is a global mutex used to make qstr interning thread-safe.
    mp_thread_mutex_t qstr_mutex;
    #endif

    #if MICROPY_ENABLE_COMPILER
    mp_uint_t mp_optimise_value;
    #if MICROPY_EMIT_NATIVE
    uint8_t default_emit_opt; // one of MP_EMIT_OPT_xxx
    #endif
    #if MICROPY_DEBUG_PRINTERS
    mp_uint_t mp_verbose_flag;
    #endif
    #endif

    // size of the emergency exception buf, if it's dynamically allocated
    #if MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF && MICROPY_EMERGENCY_EXCEPTION_BUF_SIZE == 0
    mp_int_t mp_emergency_exception_buf_size;
    #endif

    #if MICROPY_ENABLE_SCHEDULER
    volatile int16_t sched_state;

    #if MICROPY_SCHEDULER_STATIC_NODES
    // These will usually point to statically allocated memory.  They are not
    // traced by the GC.  They are assumed to be zero'd out before mp_init() is
    // called (usually because this struct lives in the BSS).
    struct _mp_sched_node_t *sched_head;
    struct _mp_sched_node_t *sched_tail;
    #endif

    // These index sched_queue.
    uint8_t sched_len;
    uint8_t sched_idx;
    #endif

    #if MICROPY_ENABLE_VM_ABORT
    bool vm_abort;
    nlr_buf_t *nlr_abort;
    #endif

    #if MICROPY_PY_THREAD_GIL
    // This is a global mutex used to make the VM/runtime thread-safe.
    mp_thread_mutex_t gil_mutex;
    #endif

    #if MICROPY_OPT_MAP_LOOKUP_CACHE
    // See mp_map_lookup.
    uint8_t map_lookup_cache[MICROPY_OPT_MAP_LOOKUP_CACHE_SIZE];
    #endif
} mp_state_vm_t;

// This structure holds state that is specific to a given thread. Everything
// in this structure is scanned for root pointers.  Anything added to this
// structure must have corresponding initialisation added to thread_entry (in
// py/modthread.c).
typedef struct _mp_state_thread_t {
    // Stack top at the start of program
    char *stack_top;

    #if MICROPY_STACK_CHECK
    size_t stack_limit;
    #endif

    #if MICROPY_ENABLE_PYSTACK
    uint8_t *pystack_start;
    uint8_t *pystack_end;
    uint8_t *pystack_cur;
    #endif

    // Locking of the GC is done per thread.
    // See GC_LOCK_DEPTH_SHIFT for an explanation of this field.
    uint16_t gc_lock_depth;

    ////////////////////////////////////////////////////////////
    // START ROOT POINTER SECTION
    // Everything that needs GC scanning must start here, and
    // is followed by state in the mp_state_vm_t structure.
    //

    mp_obj_dict_t *dict_locals;
    mp_obj_dict_t *dict_globals;

    nlr_buf_t *nlr_top;
    nlr_jump_callback_node_t *nlr_jump_callback_top;

    // pending exception object (MP_OBJ_NULL if not pending)
    volatile mp_obj_t mp_pending_exception;

    // If MP_OBJ_STOP_ITERATION is propagated then this holds its argument.
    mp_obj_t stop_iteration_arg;

    #if MICROPY_PY_SYS_SETTRACE
    mp_obj_t prof_trace_callback;
    bool prof_callback_is_executing;
    struct _mp_code_state_t *current_code_state;
    #endif

    #if MICROPY_PY_SSL_MBEDTLS_NEED_ACTIVE_CONTEXT
    struct _mp_obj_ssl_context_t *tls_ssl_context;
    #endif
} mp_state_thread_t;

// This structure combines the above 3 structures.
// The order of the entries are important for root pointer scanning in the GC to work.
typedef struct _mp_state_ctx_t {
    mp_state_thread_t thread;
    mp_state_vm_t vm;
    mp_state_mem_t mem;
} mp_state_ctx_t;

extern mp_state_ctx_t mp_state_ctx;

#define MP_STATE_VM(x) (mp_state_ctx.vm.x)
#define MP_STATE_MEM(x) (mp_state_ctx.mem.x)
#define MP_STATE_MAIN_THREAD(x) (mp_state_ctx.thread.x)

#if MICROPY_PY_THREAD
#define MP_STATE_THREAD(x) (mp_thread_get_state()->x)
#define mp_thread_is_main_thread() (mp_thread_get_state() == &mp_state_ctx.thread)
#else
#define MP_STATE_THREAD(x)  MP_STATE_MAIN_THREAD(x)
#define mp_thread_is_main_thread() (true)
#endif

#endif // MICROPY_INCLUDED_PY_MPSTATE_H
/* py/pystack.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_PYSTACK_H
#define MICROPY_INCLUDED_PY_PYSTACK_H


// Enable this debugging option to check that the amount of memory freed is
// consistent with amounts that were previously allocated.
#define MP_PYSTACK_DEBUG (0)

#if MICROPY_ENABLE_PYSTACK

void mp_pystack_init(void *start, void *end);
void *mp_pystack_alloc(size_t n_bytes);

// This function can free multiple continuous blocks at once: just pass the
// pointer to the block that was allocated first and it and all subsequently
// allocated blocks will be freed.
static inline void mp_pystack_free(void *ptr) {
    assert((uint8_t *)ptr >= MP_STATE_THREAD(pystack_start));
    assert((uint8_t *)ptr <= MP_STATE_THREAD(pystack_cur));
    #if MP_PYSTACK_DEBUG
    size_t n_bytes_to_free = MP_STATE_THREAD(pystack_cur) - (uint8_t *)ptr;
    size_t n_bytes = *(size_t *)(MP_STATE_THREAD(pystack_cur) - MICROPY_PYSTACK_ALIGN);
    while (n_bytes < n_bytes_to_free) {
        n_bytes += *(size_t *)(MP_STATE_THREAD(pystack_cur) - n_bytes - MICROPY_PYSTACK_ALIGN);
    }
    if (n_bytes != n_bytes_to_free) {
        mp_printf(&mp_plat_print, "mp_pystack_free() failed: %u != %u\n", (uint)n_bytes_to_free,
            (uint)*(size_t *)(MP_STATE_THREAD(pystack_cur) - MICROPY_PYSTACK_ALIGN));
        assert(0);
    }
    #endif
    MP_STATE_THREAD(pystack_cur) = (uint8_t *)ptr;
}

static inline void mp_pystack_realloc(void *ptr, size_t n_bytes) {
    mp_pystack_free(ptr);
    mp_pystack_alloc(n_bytes);
}

static inline size_t mp_pystack_usage(void) {
    return MP_STATE_THREAD(pystack_cur) - MP_STATE_THREAD(pystack_start);
}

static inline size_t mp_pystack_limit(void) {
    return MP_STATE_THREAD(pystack_end) - MP_STATE_THREAD(pystack_start);
}

#endif

#if !MICROPY_ENABLE_PYSTACK

#define mp_local_alloc(n_bytes) alloca(n_bytes)

static inline void mp_local_free(void *ptr) {
    (void)ptr;
}

static inline void *mp_nonlocal_alloc(size_t n_bytes) {
    return m_new(uint8_t, n_bytes);
}

static inline void *mp_nonlocal_realloc(void *ptr, size_t old_n_bytes, size_t new_n_bytes) {
    return m_renew(uint8_t, ptr, old_n_bytes, new_n_bytes);
}

static inline void mp_nonlocal_free(void *ptr, size_t n_bytes) {
    m_del(uint8_t, ptr, n_bytes);
}

#else

static inline void *mp_local_alloc(size_t n_bytes) {
    return mp_pystack_alloc(n_bytes);
}

static inline void mp_local_free(void *ptr) {
    mp_pystack_free(ptr);
}

static inline void *mp_nonlocal_alloc(size_t n_bytes) {
    return mp_pystack_alloc(n_bytes);
}

static inline void *mp_nonlocal_realloc(void *ptr, size_t old_n_bytes, size_t new_n_bytes) {
    (void)old_n_bytes;
    mp_pystack_realloc(ptr, new_n_bytes);
    return ptr;
}

static inline void mp_nonlocal_free(void *ptr, size_t n_bytes) {
    (void)n_bytes;
    mp_pystack_free(ptr);
}

#endif

#endif // MICROPY_INCLUDED_PY_PYSTACK_H
/* py/cstack.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Paul Sokolovsky
 * Copyright (c) 2024 Angus Gratton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_CSTACK_H
#define MICROPY_INCLUDED_PY_CSTACK_H


// Both init functions below accept the full stack size. Set the
// MICROPY_STACK_CHECK_MARGIN to the number of bytes subtracted to account
// for stack usage between checks.

void mp_cstack_init_with_sp_here(size_t stack_size);

static inline void mp_cstack_init_with_top(void *top, size_t stack_size) {
    MP_STATE_THREAD(stack_top) = (char *)top;

    #if MICROPY_STACK_CHECK
    assert(stack_size > MICROPY_STACK_CHECK_MARGIN); // Should be enforced by port
    MP_STATE_THREAD(stack_limit) = stack_size - MICROPY_STACK_CHECK_MARGIN;
    #else
    (void)stack_size;
    #endif
}

mp_uint_t mp_cstack_usage(void);

#if MICROPY_STACK_CHECK

void mp_cstack_check(void);

#else

static inline void mp_cstack_check(void) {
    // No-op when stack checking is disabled
}

#endif

#endif // MICROPY_INCLUDED_PY_CSTACK_H

// Initialize an nlr_jump_callback_node_call_function_1_t struct for use with
// nlr_push_jump_callback(&ctx.callback, mp_call_function_1_from_nlr_jump_callback);
#define MP_DEFINE_NLR_JUMP_CALLBACK_FUNCTION_1(ctx, f, a) \
    nlr_jump_callback_node_call_function_1_t ctx; \
    ctx.func = (void (*)(void *))(f); \
    ctx.arg = (a)

typedef enum {
    MP_VM_RETURN_NORMAL,
    MP_VM_RETURN_YIELD,
    MP_VM_RETURN_EXCEPTION,
} mp_vm_return_kind_t;

typedef enum {
    MP_ARG_BOOL      = 0x001,
    MP_ARG_INT       = 0x002,
    MP_ARG_OBJ       = 0x003,
    MP_ARG_KIND_MASK = 0x0ff,
    MP_ARG_REQUIRED  = 0x100,
    MP_ARG_KW_ONLY   = 0x200,
} mp_arg_flag_t;

// These first two enum values match the original signature of `mp_handle_pending(bool)`.
typedef enum {
    MP_HANDLE_PENDING_CALLBACKS_AND_CLEAR_EXCEPTIONS = false,
    MP_HANDLE_PENDING_CALLBACKS_AND_EXCEPTIONS = true,
    MP_HANDLE_PENDING_CALLBACKS_ONLY,
} mp_handle_pending_behaviour_t;

typedef union _mp_arg_val_t {
    bool u_bool;
    mp_int_t u_int;
    mp_obj_t u_obj;
    mp_rom_obj_t u_rom_obj;
} mp_arg_val_t;

typedef struct _mp_arg_t {
    uint16_t qst;
    uint16_t flags;
    mp_arg_val_t defval;
} mp_arg_t;

struct _mp_sched_node_t;

typedef void (*mp_sched_callback_t)(struct _mp_sched_node_t *);

typedef struct _mp_sched_node_t {
    mp_sched_callback_t callback;
    struct _mp_sched_node_t *next;
} mp_sched_node_t;

// For use with mp_globals_locals_set_from_nlr_jump_callback.
typedef struct _nlr_jump_callback_node_globals_locals_t {
    nlr_jump_callback_node_t callback;
    mp_obj_dict_t *globals;
    mp_obj_dict_t *locals;
} nlr_jump_callback_node_globals_locals_t;

// For use with mp_call_function_1_from_nlr_jump_callback.
typedef struct _nlr_jump_callback_node_call_function_1_t {
    nlr_jump_callback_node_t callback;
    void (*func)(void *);
    void *arg;
} nlr_jump_callback_node_call_function_1_t;

// Tables mapping operator enums to qstrs, defined in objtype.c
extern const byte mp_unary_op_method_name[];
extern const byte mp_binary_op_method_name[];

void mp_init(void);
void mp_deinit(void);

void mp_sched_exception(mp_obj_t exc);
void mp_sched_keyboard_interrupt(void);
#if MICROPY_ENABLE_VM_ABORT
void mp_sched_vm_abort(void);
#endif

void mp_handle_pending(mp_handle_pending_behaviour_t behavior);

#if MICROPY_ENABLE_SCHEDULER
void mp_sched_lock(void);
void mp_sched_unlock(void);
#define mp_sched_num_pending() (MP_STATE_VM(sched_len))
bool mp_sched_schedule(mp_obj_t function, mp_obj_t arg);
bool mp_sched_schedule_node(mp_sched_node_t *node, mp_sched_callback_t callback);
#endif

// Handles any pending MicroPython events without waiting for an interrupt or event.
void mp_event_handle_nowait(void);

// Handles any pending MicroPython events and then suspends execution until the
// next interrupt or event.
//
// Note: on "tickless" ports this can suspend execution for a long time,
// don't call unless you know an interrupt is coming to continue execution.
// On "ticked" ports it may return early due to the tick interrupt.
void mp_event_wait_indefinite(void);

// Handle any pending MicroPython events and then suspends execution until the
// next interrupt or event, or until timeout_ms milliseconds have elapsed.
//
// On "ticked" ports it may return early due to the tick interrupt.
void mp_event_wait_ms(mp_uint_t timeout_ms);

// extra printing method specifically for mp_obj_t's which are integral type
int mp_print_mp_int(const mp_print_t *print, mp_obj_t x, unsigned base, int base_char, int flags, char fill, int width, int prec);

void mp_arg_check_num_sig(size_t n_args, size_t n_kw, uint32_t sig);
static inline void mp_arg_check_num(size_t n_args, size_t n_kw, size_t n_args_min, size_t n_args_max, bool takes_kw) {
    mp_arg_check_num_sig(n_args, n_kw, MP_OBJ_FUN_MAKE_SIG(n_args_min, n_args_max, takes_kw));
}
void mp_arg_parse_all(size_t n_pos, const mp_obj_t *pos, mp_map_t *kws, size_t n_allowed, const mp_arg_t *allowed, mp_arg_val_t *out_vals);
void mp_arg_parse_all_kw_array(size_t n_pos, size_t n_kw, const mp_obj_t *args, size_t n_allowed, const mp_arg_t *allowed, mp_arg_val_t *out_vals);
MP_NORETURN void mp_arg_error_terse_mismatch(void);
MP_NORETURN void mp_arg_error_unimpl_kw(void);

static inline mp_obj_dict_t *mp_locals_get(void) {
    return MP_STATE_THREAD(dict_locals);
}
static inline void mp_locals_set(mp_obj_dict_t *d) {
    MP_STATE_THREAD(dict_locals) = d;
}
static inline mp_obj_dict_t *mp_globals_get(void) {
    return MP_STATE_THREAD(dict_globals);
}
static inline void mp_globals_set(mp_obj_dict_t *d) {
    MP_STATE_THREAD(dict_globals) = d;
}

void mp_globals_locals_set_from_nlr_jump_callback(void *ctx_in);
void mp_call_function_1_from_nlr_jump_callback(void *ctx_in);

#if MICROPY_PY_THREAD
static inline void mp_thread_init_state(mp_state_thread_t *ts, size_t stack_size, mp_obj_dict_t *locals, mp_obj_dict_t *globals) {
    mp_thread_set_state(ts);

    mp_cstack_init_with_top(ts + 1, stack_size); // need to include ts in root-pointer scan

    // GC starts off unlocked
    ts->gc_lock_depth = 0;

    // There are no pending jump callbacks or exceptions yet
    ts->nlr_top = NULL;
    ts->nlr_jump_callback_top = NULL;
    ts->mp_pending_exception = MP_OBJ_NULL;

    #if MICROPY_PY_SYS_SETTRACE
    ts->prof_trace_callback = MP_OBJ_NULL;
    ts->prof_callback_is_executing = false;
    ts->current_code_state = NULL;
    #endif

    // If locals/globals are not given, inherit from main thread
    if (locals == NULL) {
        locals = mp_state_ctx.thread.dict_locals;
    }
    if (globals == NULL) {
        globals = mp_state_ctx.thread.dict_globals;
    }
    mp_locals_set(locals);
    mp_globals_set(globals);
}
#endif

mp_obj_t mp_load_name(qstr qst);
mp_obj_t mp_load_global(qstr qst);
mp_obj_t mp_load_build_class(void);
void mp_store_name(qstr qst, mp_obj_t obj);
void mp_store_global(qstr qst, mp_obj_t obj);
void mp_delete_name(qstr qst);
void mp_delete_global(qstr qst);

mp_obj_t mp_unary_op(mp_unary_op_t op, mp_obj_t arg);
mp_obj_t mp_binary_op(mp_binary_op_t op, mp_obj_t lhs, mp_obj_t rhs);

mp_obj_t mp_call_function_0(mp_obj_t fun);
mp_obj_t mp_call_function_1(mp_obj_t fun, mp_obj_t arg);
mp_obj_t mp_call_function_2(mp_obj_t fun, mp_obj_t arg1, mp_obj_t arg2);
mp_obj_t mp_call_function_n_kw(mp_obj_t fun, size_t n_args, size_t n_kw, const mp_obj_t *args);
mp_obj_t mp_call_method_n_kw(size_t n_args, size_t n_kw, const mp_obj_t *args);
mp_obj_t mp_call_method_n_kw_var(bool have_self, size_t n_args_n_kw, const mp_obj_t *args);
mp_obj_t mp_call_method_self_n_kw(mp_obj_t meth, mp_obj_t self, size_t n_args, size_t n_kw, const mp_obj_t *args);
// Call function and catch/dump exception - for Python callbacks from C code
// (return MP_OBJ_NULL in case of exception).
mp_obj_t mp_call_function_1_protected(mp_obj_t fun, mp_obj_t arg);
mp_obj_t mp_call_function_2_protected(mp_obj_t fun, mp_obj_t arg1, mp_obj_t arg2);

typedef struct _mp_call_args_t {
    mp_obj_t fun;
    size_t n_args, n_kw, n_alloc;
    mp_obj_t *args;
} mp_call_args_t;

#if MICROPY_STACKLESS
// Takes arguments which are the most general mix of Python arg types, and
// prepares argument array suitable for passing to ->call() method of a
// function object (and mp_call_function_n_kw()).
// (Only needed in stackless mode.)
void mp_call_prepare_args_n_kw_var(bool have_self, size_t n_args_n_kw, const mp_obj_t *args, mp_call_args_t *out_args);
#endif

void mp_unpack_sequence(mp_obj_t seq, size_t num, mp_obj_t *items);
void mp_unpack_ex(mp_obj_t seq, size_t num, mp_obj_t *items);
mp_obj_t mp_store_map(mp_obj_t map, mp_obj_t key, mp_obj_t value);
mp_obj_t mp_load_attr(mp_obj_t base, qstr attr);
void mp_convert_member_lookup(mp_obj_t obj, const mp_obj_type_t *type, mp_obj_t member, mp_obj_t *dest);
void mp_load_method(mp_obj_t base, qstr attr, mp_obj_t *dest);
void mp_load_method_maybe(mp_obj_t base, qstr attr, mp_obj_t *dest);
void mp_load_method_protected(mp_obj_t obj, qstr attr, mp_obj_t *dest, bool catch_all_exc);
void mp_load_super_method(qstr attr, mp_obj_t *dest);
void mp_store_attr(mp_obj_t base, qstr attr, mp_obj_t val);

mp_obj_t mp_getiter(mp_obj_t o, mp_obj_iter_buf_t *iter_buf);
mp_obj_t mp_iternext_allow_raise(mp_obj_t o); // may return MP_OBJ_STOP_ITERATION instead of raising StopIteration()
mp_obj_t mp_iternext(mp_obj_t o); // will always return MP_OBJ_STOP_ITERATION instead of raising StopIteration(...)
mp_vm_return_kind_t mp_resume(mp_obj_t self_in, mp_obj_t send_value, mp_obj_t throw_value, mp_obj_t *ret_val);

static inline mp_obj_t mp_make_stop_iteration(mp_obj_t o) {
    MP_STATE_THREAD(stop_iteration_arg) = o;
    return MP_OBJ_STOP_ITERATION;
}

mp_obj_t mp_make_raise_obj(mp_obj_t o);

mp_obj_t mp_import_name(qstr name, mp_obj_t fromlist, mp_obj_t level);
mp_obj_t mp_import_from(mp_obj_t module, qstr name);
void mp_import_all(mp_obj_t module);

#if MICROPY_ERROR_REPORTING == MICROPY_ERROR_REPORTING_NONE
MP_NORETURN void mp_raise_type(const mp_obj_type_t *exc_type);
MP_NORETURN void mp_raise_ValueError_no_msg(void);
MP_NORETURN void mp_raise_TypeError_no_msg(void);
MP_NORETURN void mp_raise_NotImplementedError_no_msg(void);
#define mp_raise_msg(exc_type, msg) mp_raise_type(exc_type)
#define mp_raise_msg_varg(exc_type, ...) mp_raise_type(exc_type)
#define mp_raise_ValueError(msg) mp_raise_ValueError_no_msg()
#define mp_raise_TypeError(msg) mp_raise_TypeError_no_msg()
#define mp_raise_NotImplementedError(msg) mp_raise_NotImplementedError_no_msg()
#else
#define mp_raise_type(exc_type) mp_raise_msg(exc_type, NULL)
MP_NORETURN void mp_raise_msg(const mp_obj_type_t *exc_type, mp_rom_error_text_t msg);
MP_NORETURN void mp_raise_msg_varg(const mp_obj_type_t *exc_type, mp_rom_error_text_t fmt, ...);
MP_NORETURN void mp_raise_ValueError(mp_rom_error_text_t msg);
MP_NORETURN void mp_raise_TypeError(mp_rom_error_text_t msg);
MP_NORETURN void mp_raise_NotImplementedError(mp_rom_error_text_t msg);
#endif

MP_NORETURN void mp_raise_type_arg(const mp_obj_type_t *exc_type, mp_obj_t arg);
MP_NORETURN void mp_raise_StopIteration(mp_obj_t arg);
MP_NORETURN void mp_raise_TypeError_int_conversion(mp_const_obj_t arg);
MP_NORETURN void mp_raise_OSError(int errno_);
MP_NORETURN void mp_raise_OSError_with_filename(int errno_, const char *filename);
MP_NORETURN void mp_raise_recursion_depth(void);

#if MICROPY_BUILTIN_METHOD_CHECK_SELF_ARG
#undef mp_check_self
#define mp_check_self(pred)
#else
// A port may define to raise TypeError for example
#ifndef mp_check_self
#define mp_check_self(pred) assert(pred)
#endif
#endif

// helper functions for native/viper code
int mp_native_type_from_qstr(qstr qst);
mp_uint_t mp_native_from_obj(mp_obj_t obj, mp_uint_t type);
mp_obj_t mp_native_to_obj(mp_uint_t val, mp_uint_t type);

#if MICROPY_PY_SYS_PATH
#define mp_sys_path (MP_STATE_VM(sys_mutable[MP_SYS_MUTABLE_PATH]))
#endif

#if MICROPY_PY_SYS_ARGV
#define mp_sys_argv (MP_OBJ_FROM_PTR(&MP_STATE_VM(mp_sys_argv_obj)))
#endif

#if MICROPY_WARNINGS
#ifndef mp_warning
void mp_warning(const char *category, const char *msg, ...);
#endif
#else
#define mp_warning(...)
#endif

#endif // MICROPY_INCLUDED_PY_RUNTIME_H
/* py/compile.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_COMPILE_H
#define MICROPY_INCLUDED_PY_COMPILE_H

/* py/lexer.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_LEXER_H
#define MICROPY_INCLUDED_PY_LEXER_H

#include <stdint.h>

/* py/reader.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_READER_H
#define MICROPY_INCLUDED_PY_READER_H


// Pass to the `free_len` argument to `mp_reader_new_mem` to indicate that the data is in ROM.
// This means that the data is addressable and will remain valid at least until a soft reset.
#define MP_READER_IS_ROM ((size_t)-1)

// the readbyte function must return the next byte in the input stream
// it must return MP_READER_EOF if end of stream
// it can be called again after returning MP_READER_EOF, and in that case must return MP_READER_EOF
#define MP_READER_EOF ((mp_uint_t)(-1))

typedef struct _mp_reader_t {
    void *data;
    mp_uint_t (*readbyte)(void *data);
    void (*close)(void *data);
} mp_reader_t;

void mp_reader_new_mem(mp_reader_t *reader, const byte *buf, size_t len, size_t free_len);
void mp_reader_new_file(mp_reader_t *reader, qstr filename);
void mp_reader_new_file_from_fd(mp_reader_t *reader, int fd, bool close_fd);

// Try to efficiently read the given number of bytes from a ROM-based reader.
// Returns a valid, non-NULL pointer to the requested data if the reader points to ROM.
// Returns NULL if the reader does not point to ROM.
const uint8_t *mp_reader_try_read_rom(mp_reader_t *reader, size_t len);

#endif // MICROPY_INCLUDED_PY_READER_H

/* lexer.h -- simple tokeniser for MicroPython
 *
 * Uses (byte) length instead of null termination.
 * Tokens are the same - UTF-8 with (byte) length.
 */

typedef enum _mp_token_kind_t {
    MP_TOKEN_END,

    MP_TOKEN_INVALID,
    MP_TOKEN_DEDENT_MISMATCH,
    MP_TOKEN_LONELY_STRING_OPEN,
    #if MICROPY_PY_FSTRINGS
    MP_TOKEN_MALFORMED_FSTRING,
    #endif

    MP_TOKEN_NEWLINE,
    MP_TOKEN_INDENT,
    MP_TOKEN_DEDENT,

    MP_TOKEN_NAME,
    MP_TOKEN_INTEGER,
    MP_TOKEN_FLOAT_OR_IMAG,
    MP_TOKEN_STRING,
    MP_TOKEN_BYTES,

    MP_TOKEN_ELLIPSIS,

    MP_TOKEN_KW_FALSE,
    MP_TOKEN_KW_NONE,
    MP_TOKEN_KW_TRUE,
    MP_TOKEN_KW___DEBUG__,
    MP_TOKEN_KW_AND,
    MP_TOKEN_KW_AS,
    MP_TOKEN_KW_ASSERT,
    #if MICROPY_PY_ASYNC_AWAIT
    MP_TOKEN_KW_ASYNC,
    MP_TOKEN_KW_AWAIT,
    #endif
    MP_TOKEN_KW_BREAK,
    MP_TOKEN_KW_CLASS,
    MP_TOKEN_KW_CONTINUE,
    MP_TOKEN_KW_DEF,
    MP_TOKEN_KW_DEL,
    MP_TOKEN_KW_ELIF,
    MP_TOKEN_KW_ELSE,
    MP_TOKEN_KW_EXCEPT,
    MP_TOKEN_KW_FINALLY,
    MP_TOKEN_KW_FOR,
    MP_TOKEN_KW_FROM,
    MP_TOKEN_KW_GLOBAL,
    MP_TOKEN_KW_IF,
    MP_TOKEN_KW_IMPORT,
    MP_TOKEN_KW_IN,
    MP_TOKEN_KW_IS,
    MP_TOKEN_KW_LAMBDA,
    MP_TOKEN_KW_NONLOCAL,
    MP_TOKEN_KW_NOT,
    MP_TOKEN_KW_OR,
    MP_TOKEN_KW_PASS,
    MP_TOKEN_KW_RAISE,
    MP_TOKEN_KW_RETURN,
    MP_TOKEN_KW_TRY,
    MP_TOKEN_KW_WHILE,
    MP_TOKEN_KW_WITH,
    MP_TOKEN_KW_YIELD,

    MP_TOKEN_OP_ASSIGN,
    MP_TOKEN_OP_TILDE,

    // Order of these 6 matches corresponding mp_binary_op_t operator
    MP_TOKEN_OP_LESS,
    MP_TOKEN_OP_MORE,
    MP_TOKEN_OP_DBL_EQUAL,
    MP_TOKEN_OP_LESS_EQUAL,
    MP_TOKEN_OP_MORE_EQUAL,
    MP_TOKEN_OP_NOT_EQUAL,

    // Order of these 13 matches corresponding mp_binary_op_t operator
    MP_TOKEN_OP_PIPE,
    MP_TOKEN_OP_CARET,
    MP_TOKEN_OP_AMPERSAND,
    MP_TOKEN_OP_DBL_LESS,
    MP_TOKEN_OP_DBL_MORE,
    MP_TOKEN_OP_PLUS,
    MP_TOKEN_OP_MINUS,
    MP_TOKEN_OP_STAR,
    MP_TOKEN_OP_AT,
    MP_TOKEN_OP_DBL_SLASH,
    MP_TOKEN_OP_SLASH,
    MP_TOKEN_OP_PERCENT,
    MP_TOKEN_OP_DBL_STAR,

    // Order of these 13 matches corresponding mp_binary_op_t operator
    MP_TOKEN_DEL_PIPE_EQUAL,
    MP_TOKEN_DEL_CARET_EQUAL,
    MP_TOKEN_DEL_AMPERSAND_EQUAL,
    MP_TOKEN_DEL_DBL_LESS_EQUAL,
    MP_TOKEN_DEL_DBL_MORE_EQUAL,
    MP_TOKEN_DEL_PLUS_EQUAL,
    MP_TOKEN_DEL_MINUS_EQUAL,
    MP_TOKEN_DEL_STAR_EQUAL,
    MP_TOKEN_DEL_AT_EQUAL,
    MP_TOKEN_DEL_DBL_SLASH_EQUAL,
    MP_TOKEN_DEL_SLASH_EQUAL,
    MP_TOKEN_DEL_PERCENT_EQUAL,
    MP_TOKEN_DEL_DBL_STAR_EQUAL,

    MP_TOKEN_DEL_PAREN_OPEN,
    MP_TOKEN_DEL_PAREN_CLOSE,
    MP_TOKEN_DEL_BRACKET_OPEN,
    MP_TOKEN_DEL_BRACKET_CLOSE,
    MP_TOKEN_DEL_BRACE_OPEN,
    MP_TOKEN_DEL_BRACE_CLOSE,
    MP_TOKEN_DEL_COMMA,
    MP_TOKEN_DEL_COLON,
    MP_TOKEN_DEL_PERIOD,
    MP_TOKEN_DEL_SEMICOLON,
    MP_TOKEN_DEL_EQUAL,
    MP_TOKEN_DEL_MINUS_MORE,

    MP_TOKEN_NUMBER_OF,
} mp_token_kind_t;

// this data structure is exposed for efficiency
// public members are: source_name, tok_line, tok_column, tok_kind, vstr
typedef struct _mp_lexer_t {
    qstr source_name;           // name of source
    mp_reader_t reader;         // stream source

    uint32_t chr0;              // first cached byte from source (32-bits for efficient access)
    uint8_t chr1, chr2;         // subsequent cached bytes from source

    size_t line;                // current source line
    size_t column;              // current source column

    mp_int_t emit_dent;             // non-zero when there are INDENT/DEDENT tokens to emit
    mp_int_t nested_bracket_level;  // >0 when there are nested brackets over multiple lines

    size_t alloc_indent_level;
    size_t num_indent_level;
    uint16_t *indent_level;

    size_t tok_line;            // token source line
    size_t tok_column;          // token source column
    mp_token_kind_t tok_kind;   // token kind
    vstr_t vstr;                // token data
    #if MICROPY_PY_FSTRINGS
    vstr_t inject_chrs;         // characters currently being injected into the stream
    size_t inject_chrs_idx;     // current index into inject_chrs
    vstr_t fstring_args;        // extracted arguments to pass to .format()
    #endif
} mp_lexer_t;

mp_lexer_t *mp_lexer_new(qstr src_name, mp_reader_t reader);
mp_lexer_t *mp_lexer_new_from_str_len(qstr src_name, const char *str, size_t len, size_t free_len);

// If MICROPY_READER_POSIX or MICROPY_READER_VFS aren't enabled then
// this function must be implemented by the port.
mp_lexer_t *mp_lexer_new_from_file(qstr filename);

#if MICROPY_HELPER_LEXER_UNIX
mp_lexer_t *mp_lexer_new_from_fd(qstr filename, int fd, bool close_fd);
#endif

void mp_lexer_free(mp_lexer_t *lex);
void mp_lexer_to_next(mp_lexer_t *lex);

#endif // MICROPY_INCLUDED_PY_LEXER_H
/* py/parse.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_PARSE_H
#define MICROPY_INCLUDED_PY_PARSE_H

#include <stddef.h>
#include <stdint.h>


struct _mp_lexer_t;

// a mp_parse_node_t is:
//  - 0000...0000: no node
//  - xxxx...xxx1: a small integer; bits 1 and above are the signed value, 2's complement
//  - xxxx...xx00: pointer to mp_parse_node_struct_t
//  - xx...xx0010: an identifier; bits 4 and above are the qstr
//  - xx...xx0110: a string; bits 4 and above are the qstr holding the value
//  - xx...xx1010: a token; bits 4 and above are mp_token_kind_t

#define MP_PARSE_NODE_NULL      (0)
#define MP_PARSE_NODE_SMALL_INT (0x1)
#define MP_PARSE_NODE_ID        (0x02)
#define MP_PARSE_NODE_STRING    (0x06)
#define MP_PARSE_NODE_TOKEN     (0x0a)

typedef uintptr_t mp_parse_node_t; // must be pointer size

typedef struct _mp_parse_node_struct_t {
    uint32_t source_line;       // line number in source file
    uint32_t kind_num_nodes;    // parse node kind, and number of nodes
    mp_parse_node_t nodes[];    // nodes
} mp_parse_node_struct_t;

// macros for mp_parse_node_t usage
// some of these evaluate their argument more than once

#define MP_PARSE_NODE_IS_NULL(pn) ((pn) == MP_PARSE_NODE_NULL)
#define MP_PARSE_NODE_IS_LEAF(pn) ((pn) & 3)
#define MP_PARSE_NODE_IS_STRUCT(pn) ((pn) != MP_PARSE_NODE_NULL && ((pn) & 3) == 0)
#define MP_PARSE_NODE_IS_STRUCT_KIND(pn, k) ((pn) != MP_PARSE_NODE_NULL && ((pn) & 3) == 0 && MP_PARSE_NODE_STRUCT_KIND((mp_parse_node_struct_t *)(pn)) == (k))

#define MP_PARSE_NODE_IS_SMALL_INT(pn) (((pn) & 0x1) == MP_PARSE_NODE_SMALL_INT)
#define MP_PARSE_NODE_IS_ID(pn) (((pn) & 0x0f) == MP_PARSE_NODE_ID)
#define MP_PARSE_NODE_IS_TOKEN(pn) (((pn) & 0x0f) == MP_PARSE_NODE_TOKEN)
#define MP_PARSE_NODE_IS_TOKEN_KIND(pn, k) ((pn) == (MP_PARSE_NODE_TOKEN | ((k) << 4)))

#define MP_PARSE_NODE_LEAF_KIND(pn) ((pn) & 0x0f)
#define MP_PARSE_NODE_LEAF_ARG(pn) (((uintptr_t)(pn)) >> 4)
#define MP_PARSE_NODE_LEAF_SMALL_INT(pn) (((mp_int_t)(intptr_t)(pn)) >> 1)
#define MP_PARSE_NODE_STRUCT_KIND(pns) ((pns)->kind_num_nodes & 0xff)
#define MP_PARSE_NODE_STRUCT_NUM_NODES(pns) ((pns)->kind_num_nodes >> 8)

static inline mp_parse_node_t mp_parse_node_new_small_int(mp_int_t val) {
    return (mp_parse_node_t)(MP_PARSE_NODE_SMALL_INT | ((mp_uint_t)val << 1));
}

static inline mp_parse_node_t mp_parse_node_new_leaf(size_t kind, mp_int_t arg) {
    return (mp_parse_node_t)(kind | ((mp_uint_t)arg << 4));
}

static inline mp_obj_t mp_parse_node_extract_const_object(mp_parse_node_struct_t *pns) {
    #if MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_D
    // nodes are 32-bit pointers, but need to extract 64-bit object
    return (uint64_t)pns->nodes[0] | ((uint64_t)pns->nodes[1] << 32);
    #else
    return (mp_obj_t)pns->nodes[0];
    #endif
}

bool mp_parse_node_is_const_false(mp_parse_node_t pn);
bool mp_parse_node_is_const_true(mp_parse_node_t pn);
bool mp_parse_node_get_int_maybe(mp_parse_node_t pn, mp_obj_t *o);
size_t mp_parse_node_extract_list(mp_parse_node_t *pn, size_t pn_kind, mp_parse_node_t **nodes);
void mp_parse_node_print(const mp_print_t *print, mp_parse_node_t pn, size_t indent);

typedef enum {
    MP_PARSE_SINGLE_INPUT,
    MP_PARSE_FILE_INPUT,
    MP_PARSE_EVAL_INPUT,
} mp_parse_input_kind_t;

typedef struct _mp_parse_t {
    mp_parse_node_t root;
    struct _mp_parse_chunk_t *chunk;
} mp_parse_tree_t;

// the parser will raise an exception if an error occurred
// the parser will free the lexer before it returns
mp_parse_tree_t mp_parse(struct _mp_lexer_t *lex, mp_parse_input_kind_t input_kind);
void mp_parse_tree_clear(mp_parse_tree_t *tree);

#endif // MICROPY_INCLUDED_PY_PARSE_H
/* py/emitglue.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_EMITGLUE_H
#define MICROPY_INCLUDED_PY_EMITGLUE_H

/* py/bc.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 * Copyright (c) 2014 Paul Sokolovsky
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_BC_H
#define MICROPY_INCLUDED_PY_BC_H


// bytecode layout:
//
//  func signature  : var uint
//      contains six values interleaved bit-wise as: xSSSSEAA [xFSSKAED repeated]
//          x = extension           another byte follows
//          S = n_state - 1         number of entries in Python value stack
//          E = n_exc_stack         number of entries in exception stack
//          F = scope_flags         four bits of flags, MP_SCOPE_FLAG_xxx
//          A = n_pos_args          number of arguments this function takes
//          K = n_kwonly_args       number of keyword-only arguments this function takes
//          D = n_def_pos_args      number of default positional arguments
//
//  prelude size    : var uint
//      contains two values interleaved bit-wise as: xIIIIIIC repeated
//          x = extension           another byte follows
//          I = n_info              number of bytes in source info section (always > 0)
//          C = n_cells             number of bytes/cells in closure section
//
//  source info section:
//      simple_name : var qstr      always exists
//      argname0    : var qstr
//      ...         : var qstr
//      argnameN    : var qstr      N = num_pos_args + num_kwonly_args - 1
//      <line number info>
//
//  closure section:
//      local_num0  : byte
//      ...         : byte
//      local_numN  : byte          N = n_cells-1
//
//  <bytecode>
//
//
// constant table layout:
//
//  const0          : obj
//  constN          : obj

#define MP_ENCODE_UINT_MAX_BYTES ((MP_BYTES_PER_OBJ_WORD * 8 + 6) / 7)

#define MP_BC_PRELUDE_SIG_ENCODE(S, E, scope, out_byte, out_env) \
    do {                                                            \
        /*// Get values to store in prelude */                      \
        size_t F = scope->scope_flags & MP_SCOPE_FLAG_ALL_SIG;      \
        size_t A = scope->num_pos_args;                             \
        size_t K = scope->num_kwonly_args;                          \
        size_t D = scope->num_def_pos_args;                         \
                                                                \
        /* Adjust S to shrink range, to compress better */          \
        S -= 1;                                                     \
                                                                \
        /* Encode prelude */                                        \
        /* xSSSSEAA */                                              \
        uint8_t z = (S & 0xf) << 3 | (E & 1) << 2 | (A & 3);        \
        S >>= 4;                                                    \
        E >>= 1;                                                    \
        A >>= 2;                                                    \
        while (S | E | F | A | K | D) {                             \
            out_byte(out_env, 0x80 | z);                            \
            /* xFSSKAED */                                          \
            z = (F & 1) << 6 | (S & 3) << 4 | (K & 1) << 3          \
                | (A & 1) << 2 | (E & 1) << 1 | (D & 1);            \
            S >>= 2;                                                \
            E >>= 1;                                                \
            F >>= 1;                                                \
            A >>= 1;                                                \
            K >>= 1;                                                \
            D >>= 1;                                                \
        }                                                           \
        out_byte(out_env, z);                                       \
    } while (0)

#define MP_BC_PRELUDE_SIG_DECODE_INTO(ip, S, E, F, A, K, D)     \
    do {                                                            \
        uint8_t z = *(ip)++;                                        \
        /* xSSSSEAA */                                              \
        S = (z >> 3) & 0xf;                                         \
        E = (z >> 2) & 0x1;                                         \
        F = 0;                                                      \
        A = z & 0x3;                                                \
        K = 0;                                                      \
        D = 0;                                                      \
        for (unsigned n = 0; z & 0x80; ++n) {                       \
            z = *(ip)++;                                            \
            /* xFSSKAED */                                          \
            S |= (z & 0x30) << (2 * n);                             \
            E |= (z & 0x02) << n;                                   \
            F |= ((z & 0x40) >> 6) << n;                            \
            A |= (z & 0x4) << n;                                    \
            K |= ((z & 0x08) >> 3) << n;                            \
            D |= (z & 0x1) << n;                                    \
        }                                                           \
        S += 1;                                                     \
    } while (0)

#define MP_BC_PRELUDE_SIG_DECODE(ip) \
    size_t n_state, n_exc_stack, scope_flags, n_pos_args, n_kwonly_args, n_def_pos_args; \
    MP_BC_PRELUDE_SIG_DECODE_INTO(ip, n_state, n_exc_stack, scope_flags, n_pos_args, n_kwonly_args, n_def_pos_args); \
    (void)n_state; (void)n_exc_stack; (void)scope_flags; \
    (void)n_pos_args; (void)n_kwonly_args; (void)n_def_pos_args

#define MP_BC_PRELUDE_SIZE_ENCODE(I, C, out_byte, out_env)      \
    do {                                                            \
        /* Encode bit-wise as: xIIIIIIC */                          \
        uint8_t z = 0;                                              \
        do {                                                        \
            z = (I & 0x3f) << 1 | (C & 1);                          \
            C >>= 1;                                                \
            I >>= 6;                                                \
            if (C | I) {                                            \
                z |= 0x80;                                          \
            }                                                       \
            out_byte(out_env, z);                                   \
        } while (C | I);                                            \
    } while (0)

#define MP_BC_PRELUDE_SIZE_DECODE_INTO(ip, I, C)                \
    do {                                                            \
        uint8_t z;                                                  \
        C = 0;                                                      \
        I = 0;                                                      \
        for (unsigned n = 0;; ++n) {                                \
            z = *(ip)++;                                            \
            /* xIIIIIIC */                                          \
            C |= (z & 1) << n;                                      \
            I |= ((z & 0x7e) >> 1) << (6 * n);                      \
            if (!(z & 0x80)) {                                      \
                break;                                              \
            }                                                       \
        }                                                           \
    } while (0)

#define MP_BC_PRELUDE_SIZE_DECODE(ip) \
    size_t n_info, n_cell; \
    MP_BC_PRELUDE_SIZE_DECODE_INTO(ip, n_info, n_cell); \
    (void)n_info; (void)n_cell

// Sentinel value for mp_code_state_t.exc_sp_idx
#define MP_CODE_STATE_EXC_SP_IDX_SENTINEL ((uint16_t)-1)

// To convert mp_code_state_t.exc_sp_idx to/from a pointer to mp_exc_stack_t
#define MP_CODE_STATE_EXC_SP_IDX_FROM_PTR(exc_stack, exc_sp) ((exc_sp) + 1 - (exc_stack))
#define MP_CODE_STATE_EXC_SP_IDX_TO_PTR(exc_stack, exc_sp_idx) ((exc_stack) + (exc_sp_idx) - 1)

typedef struct _mp_bytecode_prelude_t {
    uint n_state;
    uint n_exc_stack;
    uint scope_flags;
    uint n_pos_args;
    uint n_kwonly_args;
    uint n_def_pos_args;
    qstr qstr_block_name_idx;
    const byte *line_info;
    const byte *line_info_top;
    const byte *opcodes;
} mp_bytecode_prelude_t;

// Exception stack entry
typedef struct _mp_exc_stack_t {
    const byte *handler;
    // bit 0 is currently unused
    // bit 1 is whether the opcode was SETUP_WITH or SETUP_FINALLY
    mp_obj_t *val_sp;
    // Saved exception
    mp_obj_base_t *prev_exc;
} mp_exc_stack_t;

// Constants associated with a module, to interface bytecode with runtime.
typedef struct _mp_module_constants_t {
    #if MICROPY_EMIT_BYTECODE_USES_QSTR_TABLE
    qstr_short_t *qstr_table;
    #else
    qstr source_file;
    #endif
    mp_obj_t *obj_table;
} mp_module_constants_t;

// State associated with a module.
typedef struct _mp_module_context_t {
    mp_obj_module_t module;
    mp_module_constants_t constants;
} mp_module_context_t;

// Outer level struct defining a compiled module.
typedef struct _mp_compiled_module_t {
    mp_module_context_t *context;
    const struct _mp_raw_code_t *rc;
    #if MICROPY_PERSISTENT_CODE_SAVE
    bool has_native;
    size_t n_qstr;
    size_t n_obj;
    size_t arch_flags;
    #endif
} mp_compiled_module_t;

// Outer level struct defining a frozen module.
typedef struct _mp_frozen_module_t {
    const mp_module_constants_t constants;
    const void *proto_fun;
} mp_frozen_module_t;

// State for an executing function.
typedef struct _mp_code_state_t {
    // The fun_bc entry points to the underlying function object that is being executed.
    // It is needed to access the start of bytecode and the const_table.
    // It is also needed to prevent the GC from reclaiming the bytecode during execution,
    // because the ip pointer below will always point to the interior of the bytecode.
    struct _mp_obj_fun_bc_t *fun_bc;
    const byte *ip;
    mp_obj_t *sp;
    uint16_t n_state;
    uint16_t exc_sp_idx;
    mp_obj_dict_t *old_globals;
    #if MICROPY_STACKLESS
    struct _mp_code_state_t *prev;
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    struct _mp_code_state_t *prev_state;
    struct _mp_obj_frame_t *frame;
    #endif
    // Variable-length
    mp_obj_t state[0];
    // Variable-length, never accessed by name, only as (void*)(state + n_state)
    // mp_exc_stack_t exc_state[0];
} mp_code_state_t;

// State for an executing native function (based on mp_code_state_t).
typedef struct _mp_code_state_native_t {
    struct _mp_obj_fun_bc_t *fun_bc;
    const byte *ip;
    mp_obj_t *sp;
    uint16_t n_state;
    uint16_t exc_sp_idx;
    mp_obj_dict_t *old_globals;
    mp_obj_t state[0];
} mp_code_state_native_t;

// Allocator may return NULL, in which case data is not stored (can be used to compute size).
typedef uint8_t *(*mp_encode_uint_allocator_t)(void *env, size_t nbytes);

void mp_encode_uint(void *env, mp_encode_uint_allocator_t allocator, mp_uint_t val);
mp_uint_t mp_decode_uint(const byte **ptr);
mp_uint_t mp_decode_uint_value(const byte *ptr);
const byte *mp_decode_uint_skip(const byte *ptr);

mp_vm_return_kind_t mp_execute_bytecode(mp_code_state_t *code_state,
#ifndef __cplusplus
    volatile
#endif
    mp_obj_t inject_exc);
mp_code_state_t *mp_obj_fun_bc_prepare_codestate(mp_obj_t func, size_t n_args, size_t n_kw, const mp_obj_t *args);
void mp_setup_code_state(mp_code_state_t *code_state, size_t n_args, size_t n_kw, const mp_obj_t *args);
void mp_setup_code_state_native(mp_code_state_native_t *code_state, size_t n_args, size_t n_kw, const mp_obj_t *args);
void mp_bytecode_print(const mp_print_t *print, const struct _mp_raw_code_t *rc, size_t fun_data_len, const mp_module_constants_t *cm);
void mp_bytecode_print2(const mp_print_t *print, const byte *ip, size_t len, struct _mp_raw_code_t *const *child_table, const mp_module_constants_t *cm);
const byte *mp_bytecode_print_str(const mp_print_t *print, const byte *ip_start, const byte *ip, struct _mp_raw_code_t *const *child_table, const mp_module_constants_t *cm);
#define mp_bytecode_print_inst(print, code, x_table) mp_bytecode_print2(print, code, 1, x_table)

// Helper macros to access pointer with least significant bits holding flags
#define MP_TAGPTR_PTR(x) ((void *)((uintptr_t)(x) & ~((uintptr_t)3)))
#define MP_TAGPTR_TAG0(x) ((uintptr_t)(x) & 1)
#define MP_TAGPTR_TAG1(x) ((uintptr_t)(x) & 2)
#define MP_TAGPTR_MAKE(ptr, tag) ((void *)((uintptr_t)(ptr) | (tag)))

static inline void mp_module_context_alloc_tables(mp_module_context_t *context, size_t n_qstr, size_t n_obj) {
    #if MICROPY_EMIT_BYTECODE_USES_QSTR_TABLE
    size_t nq = (n_qstr * sizeof(qstr_short_t) + sizeof(mp_uint_t) - 1) / sizeof(mp_uint_t);
    size_t no = n_obj;
    mp_uint_t *mem = m_new(mp_uint_t, nq + no);
    context->constants.qstr_table = (qstr_short_t *)mem;
    context->constants.obj_table = (mp_obj_t *)(mem + nq);
    #else
    if (n_obj == 0) {
        context->constants.obj_table = NULL;
    } else {
        context->constants.obj_table = m_new(mp_obj_t, n_obj);
    }
    #endif
}

typedef struct _mp_code_lineinfo_t {
    size_t bc_increment;
    size_t line_increment;
} mp_code_lineinfo_t;

static inline mp_code_lineinfo_t mp_bytecode_decode_lineinfo(const byte **line_info) {
    mp_code_lineinfo_t result;
    size_t c = (*line_info)[0];
    if ((c & 0x80) == 0) {
        // 0b0LLBBBBB encoding
        result.bc_increment = c & 0x1f;
        result.line_increment = c >> 5;
        *line_info += 1;
    } else {
        // 0b1LLLBBBB 0bLLLLLLLL encoding (l's LSB in second byte)
        result.bc_increment = c & 0xf;
        result.line_increment = ((c << 4) & 0x700) | (*line_info)[1];
        *line_info += 2;
    }
    return result;
}

static inline size_t mp_bytecode_get_source_line(const byte *line_info, const byte *line_info_top, size_t bc_offset) {
    size_t source_line = 1;
    while (line_info < line_info_top) {
        mp_code_lineinfo_t decoded = mp_bytecode_decode_lineinfo(&line_info);
        if (bc_offset >= decoded.bc_increment) {
            bc_offset -= decoded.bc_increment;
            source_line += decoded.line_increment;
        } else {
            // found source line corresponding to bytecode offset
            break;
        }
    }
    return source_line;
}

#endif // MICROPY_INCLUDED_PY_BC_H

// These variables and functions glue the code emitters to the runtime.

// Used with mp_raw_code_t::proto_fun_indicator to detect if a mp_proto_fun_t is a
// mp_raw_code_t struct or a direct pointer to bytecode.
#define MP_PROTO_FUN_INDICATOR_RAW_CODE_0 (0)
#define MP_PROTO_FUN_INDICATOR_RAW_CODE_1 (0)

// These must fit in 8 bits; see scope.h
enum {
    MP_EMIT_OPT_NONE,
    MP_EMIT_OPT_BYTECODE,
    MP_EMIT_OPT_NATIVE_PYTHON,
    MP_EMIT_OPT_VIPER,
    MP_EMIT_OPT_ASM,
};

typedef enum {
    MP_CODE_UNUSED,
    MP_CODE_RESERVED,
    MP_CODE_BYTECODE,
    MP_CODE_NATIVE_PY,
    MP_CODE_NATIVE_VIPER,
    MP_CODE_NATIVE_ASM,
} mp_raw_code_kind_t;

// An mp_proto_fun_t points to static information about a non-instantiated function.
// A function object is created from this information, and that object can then be executed.
// It points either to bytecode, or an mp_raw_code_t struct.
typedef const void *mp_proto_fun_t;

// Bytecode is distinguished from an mp_raw_code_t struct by the first two bytes: bytecode
// is guaranteed to have either its first or second byte non-zero.  So if both bytes are
// zero then the mp_proto_fun_t pointer must be an mp_raw_code_t.
static inline bool mp_proto_fun_is_bytecode(mp_proto_fun_t proto_fun) {
    const uint8_t *header = (const uint8_t *)proto_fun;
    return (header[0] | (header[1] << 8)) != (MP_PROTO_FUN_INDICATOR_RAW_CODE_0 | (MP_PROTO_FUN_INDICATOR_RAW_CODE_1 << 8));
}

// The mp_raw_code_t struct appears in the following places:
// compiled bytecode: instance in RAM, referenced by outer scope, usually freed after first (and only) use
// mpy file: instance in RAM, created when .mpy file is loaded (same comments as above)
// frozen: instance in ROM
typedef struct _mp_raw_code_t {
    uint8_t proto_fun_indicator[2];
    uint8_t kind; // of type mp_raw_code_kind_t; only 3 bits used
    bool is_generator;
    const void *fun_data;
    struct _mp_raw_code_t **children;
    #if MICROPY_PERSISTENT_CODE_SAVE
    uint32_t fun_data_len; // for mp_raw_code_save
    uint16_t n_children;
    #if MICROPY_EMIT_INLINE_ASM || MICROPY_ENABLE_NATIVE_CODE
    uint16_t prelude_offset;
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    // line_of_definition is a Python source line where the raw_code was
    // created e.g. MP_BC_MAKE_FUNCTION. This is different from lineno info
    // stored in prelude, which provides line number for first statement of
    // a function. Required to properly implement "call" trace event.
    uint32_t line_of_definition;
    mp_bytecode_prelude_t prelude;
    #endif
    #endif
    #if MICROPY_EMIT_INLINE_ASM
    uint32_t asm_n_pos_args : 8;
    uint32_t asm_type_sig : 24; // compressed as 2-bit types; ret is MSB, then arg0, arg1, etc
    #endif
} mp_raw_code_t;

// Version of mp_raw_code_t but without the asm_n_pos_args/asm_type_sig entries, which are
// only needed when the kind is MP_CODE_NATIVE_ASM.  So this struct can be used when the
// kind is MP_CODE_BYTECODE, MP_CODE_NATIVE_PY or MP_CODE_NATIVE_VIPER, to reduce its size.
typedef struct _mp_raw_code_truncated_t {
    uint8_t proto_fun_indicator[2];
    uint8_t kind;
    bool is_generator;
    const void *fun_data;
    struct _mp_raw_code_t **children;
    #if MICROPY_PERSISTENT_CODE_SAVE
    uint32_t fun_data_len;
    uint16_t n_children;
    #if MICROPY_EMIT_INLINE_ASM || MICROPY_ENABLE_NATIVE_CODE
    uint16_t prelude_offset;
    #endif
    #if MICROPY_PY_SYS_SETTRACE
    uint32_t line_of_definition;
    mp_bytecode_prelude_t prelude;
    #endif
    #endif
} mp_raw_code_truncated_t;

mp_raw_code_t *mp_emit_glue_new_raw_code(void);

void mp_emit_glue_assign_bytecode(mp_raw_code_t *rc, const byte *code,
    mp_raw_code_t **children,
    #if MICROPY_PERSISTENT_CODE_SAVE
    size_t len,
    uint16_t n_children,
    #endif
    uint16_t scope_flags);

void mp_emit_glue_assign_native(mp_raw_code_t *rc, mp_raw_code_kind_t kind, const void *fun_data, mp_uint_t fun_len,
    mp_raw_code_t **children,
    #if MICROPY_PERSISTENT_CODE_SAVE
    uint16_t n_children,
    uint16_t prelude_offset,
    #endif
    uint16_t scope_flags, uint32_t asm_n_pos_args, uint32_t asm_type_sig);

mp_obj_t mp_make_function_from_proto_fun(mp_proto_fun_t proto_fun, const mp_module_context_t *context, const mp_obj_t *def_args);
mp_obj_t mp_make_closure_from_proto_fun(mp_proto_fun_t proto_fun, const mp_module_context_t *context, mp_uint_t n_closed_over, const mp_obj_t *args);

#endif // MICROPY_INCLUDED_PY_EMITGLUE_H

// Whether mp_compile_to_raw_code is exposed as a public function.
#define MICROPY_EXPOSE_MP_COMPILE_TO_RAW_CODE (MICROPY_PY_BUILTINS_CODE >= MICROPY_PY_BUILTINS_CODE_BASIC || MICROPY_PERSISTENT_CODE_SAVE)

#if MICROPY_COMP_ALLOW_TOP_LEVEL_AWAIT
// set to `true` to allow top-level await expressions
extern bool mp_compile_allow_top_level_await;
#endif

// the compiler will raise an exception if an error occurred
// the compiler will clear the parse tree before it returns
// mp_globals_get() will be used for the context
mp_obj_t mp_compile(mp_parse_tree_t *parse_tree, qstr source_file, bool is_repl);

#if MICROPY_EXPOSE_MP_COMPILE_TO_RAW_CODE
// this has the same semantics as mp_compile
void mp_compile_to_raw_code(mp_parse_tree_t *parse_tree, qstr source_file, bool is_repl, mp_compiled_module_t *cm);
#endif

// this is implemented in runtime.c
mp_obj_t mp_parse_compile_execute(mp_lexer_t *lex, mp_parse_input_kind_t parse_input_kind, mp_obj_dict_t *globals, mp_obj_dict_t *locals);

#endif // MICROPY_INCLUDED_PY_COMPILE_H
/* py/gc.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_GC_H
#define MICROPY_INCLUDED_PY_GC_H

#include <stdbool.h>
#include <stddef.h>

void gc_init(void *start, void *end);

#if MICROPY_GC_SPLIT_HEAP
// Used to add additional memory areas to the heap.
void gc_add(void *start, void *end);

#if MICROPY_GC_SPLIT_HEAP_AUTO
// Port must implement this function to return the maximum available block of
// RAM to allocate a new heap area into using MP_PLAT_ALLOC_HEAP.
size_t gc_get_max_new_split(void);
#endif // MICROPY_GC_SPLIT_HEAP_AUTO
#endif // MICROPY_GC_SPLIT_HEAP

// These lock/unlock functions can be nested.
// They can be used to prevent the GC from allocating/freeing.
void gc_lock(void);
void gc_unlock(void);
bool gc_is_locked(void);

// A given port must implement gc_collect by using the other collect functions.
void gc_collect(void);
void gc_collect_start(void);
void gc_collect_root(void **ptrs, size_t len);
void gc_collect_end(void);

// Use this function to sweep the whole heap and run all finalisers
void gc_sweep_all(void);

// These functions are used to manage weakrefs.
void gc_weakref_mark(void *ptr);
void gc_weakref_about_to_be_freed(void *ptr);
void gc_weakref_sweep(void);

enum {
    GC_ALLOC_FLAG_HAS_FINALISER = 1,
};

void *gc_alloc(size_t n_bytes, unsigned int alloc_flags);
void gc_free(void *ptr); // does not call finaliser
size_t gc_nbytes(const void *ptr);
void *gc_realloc(void *ptr, size_t n_bytes, bool allow_move);

typedef struct _gc_info_t {
    size_t total;
    size_t used;
    size_t free;
    size_t max_free;
    size_t num_1block;
    size_t num_2block;
    size_t max_block;
    #if MICROPY_GC_SPLIT_HEAP_AUTO
    size_t max_new_split;
    #endif
} gc_info_t;

void gc_info(gc_info_t *info);
void gc_info_fast(gc_info_t *info);
void gc_dump_info(const mp_print_t *print);
void gc_dump_alloc_table(const mp_print_t *print);

#endif // MICROPY_INCLUDED_PY_GC_H
/* py/stackctrl.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Paul Sokolovsky
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_STACKCTRL_H
#define MICROPY_INCLUDED_PY_STACKCTRL_H

// This API is deprecated, please use py/cstack.h instead


#if !MICROPY_PREVIEW_VERSION_2

void mp_stack_ctrl_init(void);
void mp_stack_set_top(void *top);
mp_uint_t mp_stack_usage(void);

#if MICROPY_STACK_CHECK

void mp_stack_set_limit(mp_uint_t limit);
void mp_stack_check(void);
#define MP_STACK_CHECK() mp_stack_check()

#else

#define mp_stack_set_limit(limit) (void)(limit)
#define MP_STACK_CHECK()

#endif // MICROPY_STACK_CHECK

#endif // !MICROPY_PREVIEW_VERSION_2

#endif // MICROPY_INCLUDED_PY_STACKCTRL_H
/* py/persistentcode.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_PERSISTENTCODE_H
#define MICROPY_INCLUDED_PY_PERSISTENTCODE_H


// The current version of .mpy files. A bytecode-only .mpy file can be loaded
// as long as MPY_VERSION matches, but a native .mpy (i.e. one with an arch
// set) must also match MPY_SUB_VERSION. This allows 3 additional updates to
// the native ABI per bytecode revision.
#define MPY_VERSION 6
#define MPY_SUB_VERSION 3

// Macros to encode/decode sub-version to/from the feature byte. This replaces
// the bits previously used to encode the flags (map caching and unicode)
// which are no longer used starting at .mpy version 6.
#define MPY_FEATURE_ENCODE_SUB_VERSION(version) (version)
#define MPY_FEATURE_DECODE_SUB_VERSION(feat) ((feat) & 3)

// Macros to encode/decode native architecture to/from the feature byte
#define MPY_FEATURE_ENCODE_ARCH(arch) ((arch) << 2)
#define MPY_FEATURE_DECODE_ARCH(feat) (((feat) >> 2) & 0x2F)

// Define the host architecture
#if MICROPY_PERSISTENT_CODE_LOAD_NATIVE
    #if defined(__i386__) || defined(_M_IX86)
        #define MPY_FEATURE_ARCH (MP_NATIVE_ARCH_X86)
    #elif defined(__x86_64__) || defined(_M_X64)
        #define MPY_FEATURE_ARCH (MP_NATIVE_ARCH_X64)
    #elif defined(__thumb2__) || defined(__thumb__)
        #if defined(__thumb2__)
            #if defined(__ARM_FP) && (__ARM_FP & 8) == 8
                #define MPY_FEATURE_ARCH (MP_NATIVE_ARCH_ARMV7EMDP)
            #elif defined(__ARM_FP) && (__ARM_FP & 4) == 4
                #define MPY_FEATURE_ARCH (MP_NATIVE_ARCH_ARMV7EMSP)
            #else
                #define MPY_FEATURE_ARCH (MP_NATIVE_ARCH_ARMV7EM)
            #endif
        #else
            #define MPY_FEATURE_ARCH (MP_NATIVE_ARCH_ARMV6M)
        #endif
        #define MPY_FEATURE_ARCH_TEST(x) (MP_NATIVE_ARCH_ARMV6M <= (x) && (x) <= MPY_FEATURE_ARCH)
    #elif defined(__arm__)
        #define MPY_FEATURE_ARCH (MP_NATIVE_ARCH_ARMV6)
    #elif defined(__xtensa__)
        #include <xtensa/config/core-isa.h>
        #if XCHAL_HAVE_WINDOWED
            #define MPY_FEATURE_ARCH (MP_NATIVE_ARCH_XTENSAWIN)
        #else
            #define MPY_FEATURE_ARCH (MP_NATIVE_ARCH_XTENSA)
        #endif
    #elif defined(__riscv)
        #if __riscv_xlen == 32
            #define MPY_FEATURE_ARCH (MP_NATIVE_ARCH_RV32IMC)
        #elif __riscv_xlen == 64
            #define MPY_FEATURE_ARCH (MP_NATIVE_ARCH_RV64IMC)
        #else
            #error "Unsupported RISC-V architecture."
        #endif
    #else
        #error "Unsupported native architecture."
    #endif
#else
    #define MPY_FEATURE_ARCH (MP_NATIVE_ARCH_NONE)
#endif

#ifndef MPY_FEATURE_ARCH_TEST
#define MPY_FEATURE_ARCH_TEST(x) ((x) == MPY_FEATURE_ARCH)
#endif

// 16-bit little-endian integer with the second and third bytes of supported .mpy files
#define MPY_FILE_HEADER_INT (MPY_VERSION \
    | (MPY_FEATURE_ENCODE_SUB_VERSION(MPY_SUB_VERSION) | MPY_FEATURE_ENCODE_ARCH(MPY_FEATURE_ARCH)) << 8)

// Architecture-specific flags are present in the .mpy file
#define MPY_FEATURE_ARCH_FLAGS (0x40)
#define MPY_FEATURE_ARCH_FLAGS_TEST(x) (((x) & MPY_FEATURE_ARCH_FLAGS) == MPY_FEATURE_ARCH_FLAGS)

enum {
    MP_NATIVE_ARCH_NONE = 0,
    MP_NATIVE_ARCH_X86,
    MP_NATIVE_ARCH_X64,
    MP_NATIVE_ARCH_ARMV6,
    MP_NATIVE_ARCH_ARMV6M,
    MP_NATIVE_ARCH_ARMV7M,
    MP_NATIVE_ARCH_ARMV7EM,
    MP_NATIVE_ARCH_ARMV7EMSP,
    MP_NATIVE_ARCH_ARMV7EMDP,
    MP_NATIVE_ARCH_XTENSA,
    MP_NATIVE_ARCH_XTENSAWIN,
    MP_NATIVE_ARCH_RV32IMC,
    MP_NATIVE_ARCH_RV64IMC,
    MP_NATIVE_ARCH_DEBUG, // this entry should always be last
};

enum {
    MP_PERSISTENT_OBJ_FUN_TABLE = 0,
    MP_PERSISTENT_OBJ_NONE,
    MP_PERSISTENT_OBJ_FALSE,
    MP_PERSISTENT_OBJ_TRUE,
    MP_PERSISTENT_OBJ_ELLIPSIS,
    MP_PERSISTENT_OBJ_STR,
    MP_PERSISTENT_OBJ_BYTES,
    MP_PERSISTENT_OBJ_INT,
    MP_PERSISTENT_OBJ_FLOAT,
    MP_PERSISTENT_OBJ_COMPLEX,
    MP_PERSISTENT_OBJ_TUPLE,
};

void mp_raw_code_load(mp_reader_t *reader, mp_compiled_module_t *ctx);
void mp_raw_code_load_mem(const byte *buf, size_t len, mp_compiled_module_t *ctx);
void mp_raw_code_load_file(qstr filename, mp_compiled_module_t *ctx);

void mp_raw_code_save(mp_compiled_module_t *cm, mp_print_t *print);
void mp_raw_code_save_file(mp_compiled_module_t *cm, qstr filename);
mp_obj_t mp_raw_code_save_fun_to_bytes(const mp_module_constants_t *consts, mp_proto_fun_t proto_fun);

void mp_native_relocate(void *reloc, uint8_t *text, uintptr_t reloc_text);

#endif // MICROPY_INCLUDED_PY_PERSISTENTCODE_H
/* py/builtin.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_BUILTIN_H
#define MICROPY_INCLUDED_PY_BUILTIN_H


typedef enum {
    MP_IMPORT_STAT_NO_EXIST,
    MP_IMPORT_STAT_DIR,
    MP_IMPORT_STAT_FILE,
} mp_import_stat_t;

#if MICROPY_VFS

// Delegate to the VFS for import stat and builtin open.

#define mp_builtin_open_obj mp_vfs_open_obj

mp_import_stat_t mp_vfs_import_stat(const char *path);
mp_obj_t mp_vfs_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs);

MP_DECLARE_CONST_FUN_OBJ_KW(mp_vfs_open_obj);

static inline mp_import_stat_t mp_import_stat(const char *path) {
    return mp_vfs_import_stat(path);
}

static inline mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    return mp_vfs_open(n_args, args, kwargs);
}

#else

// A port can provide implementations of these functions.
mp_import_stat_t mp_import_stat(const char *path);
mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs);

// A port can provide this object.
MP_DECLARE_CONST_FUN_OBJ_KW(mp_builtin_open_obj);

#endif

// A port can provide its own import handler by defining mp_builtin___import__.
#ifndef mp_builtin___import__
#define mp_builtin___import__ mp_builtin___import___default
#endif
mp_obj_t mp_builtin___import__(size_t n_args, const mp_obj_t *args);
mp_obj_t mp_builtin___import___default(size_t n_args, const mp_obj_t *args);

mp_obj_t mp_micropython_mem_info(size_t n_args, const mp_obj_t *args);

MP_DECLARE_CONST_FUN_OBJ_VAR(mp_builtin___build_class___obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin___import___obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin___repl_print___obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_abs_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_all_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_any_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_bin_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_callable_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_compile_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_chr_obj);
MP_DECLARE_CONST_FUN_OBJ_2(mp_builtin_delattr_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_dir_obj);
MP_DECLARE_CONST_FUN_OBJ_2(mp_builtin_divmod_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_eval_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_exec_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_execfile_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_getattr_obj);
MP_DECLARE_CONST_FUN_OBJ_3(mp_builtin_setattr_obj);
MP_DECLARE_CONST_FUN_OBJ_0(mp_builtin_globals_obj);
MP_DECLARE_CONST_FUN_OBJ_2(mp_builtin_hasattr_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_hash_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_help_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_hex_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_id_obj);
MP_DECLARE_CONST_FUN_OBJ_2(mp_builtin_isinstance_obj);
MP_DECLARE_CONST_FUN_OBJ_2(mp_builtin_issubclass_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_iter_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_len_obj);
MP_DECLARE_CONST_FUN_OBJ_0(mp_builtin_locals_obj);
MP_DECLARE_CONST_FUN_OBJ_KW(mp_builtin_max_obj);
MP_DECLARE_CONST_FUN_OBJ_KW(mp_builtin_min_obj);
#if MICROPY_PY_BUILTINS_NEXT2
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_next_obj);
#else
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_next_obj);
#endif
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_oct_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_ord_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_pow_obj);
MP_DECLARE_CONST_FUN_OBJ_KW(mp_builtin_print_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_builtin_repr_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_round_obj);
MP_DECLARE_CONST_FUN_OBJ_KW(mp_builtin_sorted_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_sum_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_input_obj);

MP_DECLARE_CONST_FUN_OBJ_2(mp_namedtuple_obj);

MP_DECLARE_CONST_FUN_OBJ_2(mp_op_contains_obj);
MP_DECLARE_CONST_FUN_OBJ_2(mp_op_getitem_obj);
MP_DECLARE_CONST_FUN_OBJ_3(mp_op_setitem_obj);
MP_DECLARE_CONST_FUN_OBJ_2(mp_op_delitem_obj);

MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_sys_exit_obj);

// Modules needed by the runtime.
extern const mp_obj_dict_t mp_module_builtins_globals;
extern const mp_obj_module_t mp_module___main__;
extern const mp_obj_module_t mp_module_builtins;
extern const mp_obj_module_t mp_module_sys;

// Modules needed by the parser when MICROPY_COMP_MODULE_CONST is enabled.
extern const mp_obj_module_t mp_module_errno;
extern const mp_obj_module_t mp_module_uctypes;
extern const mp_obj_module_t mp_module_machine;
extern const mp_obj_module_t mp_module_math;

extern const char MICROPY_PY_BUILTINS_HELP_TEXT[];

#endif // MICROPY_INCLUDED_PY_BUILTIN_H
/* py/mphal.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_MPHAL_H
#define MICROPY_INCLUDED_PY_MPHAL_H

#include <stdint.h>
#include <stddef.h>

#ifdef MICROPY_MPHALPORT_H
#include MICROPY_MPHALPORT_H
#else
/* mphalport.h */
// MicroPython asks its host for nothing here: output goes through
// MP_PLAT_PRINT_STRN (mpconfigport.h).
void mp_hal_set_interrupt_char(int c);
#endif

// On embedded platforms, these will typically enable/disable irqs.
#ifndef MICROPY_BEGIN_ATOMIC_SECTION
#define MICROPY_BEGIN_ATOMIC_SECTION() (0)
#endif
#ifndef MICROPY_END_ATOMIC_SECTION
#define MICROPY_END_ATOMIC_SECTION(state) (void)(state)
#endif

#ifndef mp_hal_stdio_poll
uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags);
#endif

#ifndef mp_hal_stdin_rx_chr
int mp_hal_stdin_rx_chr(void);
#endif

#ifndef mp_hal_stdout_tx_str
void mp_hal_stdout_tx_str(const char *str);
#endif

#ifndef mp_hal_stdout_tx_strn
mp_uint_t mp_hal_stdout_tx_strn(const char *str, size_t len);
#endif

#ifndef mp_hal_stdout_tx_strn_cooked
void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len);
#endif

#ifndef mp_hal_delay_ms
void mp_hal_delay_ms(mp_uint_t ms);
#endif

#ifndef mp_hal_delay_us
void mp_hal_delay_us(mp_uint_t us);
#endif

#ifndef mp_hal_ticks_ms
mp_uint_t mp_hal_ticks_ms(void);
#endif

#ifndef mp_hal_ticks_us
mp_uint_t mp_hal_ticks_us(void);
#endif

#ifndef mp_hal_ticks_cpu
mp_uint_t mp_hal_ticks_cpu(void);
#endif

#ifndef mp_hal_time_ns
// Nanoseconds since the Epoch.
uint64_t mp_hal_time_ns(void);
#endif

// If port HAL didn't define its own pin API, use generic
// "virtual pin" API from the core.
#ifndef mp_hal_pin_obj_t
#define mp_hal_pin_obj_t mp_obj_t
#define mp_hal_get_pin_obj(pin) (pin)
#define mp_hal_pin_read(pin) mp_virtual_pin_read(pin)
#define mp_hal_pin_write(pin, v) mp_virtual_pin_write(pin, v)
/* extmod/virtpin.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Paul Sokolovsky
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_EXTMOD_VIRTPIN_H
#define MICROPY_INCLUDED_EXTMOD_VIRTPIN_H


#define MP_PIN_READ   (1)
#define MP_PIN_WRITE  (2)
#define MP_PIN_INPUT  (3)
#define MP_PIN_OUTPUT (4)

// Pin protocol
typedef struct _mp_pin_p_t {
    mp_uint_t (*ioctl)(mp_obj_t obj, mp_uint_t request, uintptr_t arg, int *errcode);
} mp_pin_p_t;

int mp_virtual_pin_read(mp_obj_t pin);
void mp_virtual_pin_write(mp_obj_t pin, int value);

// If a port exposes a Pin object, it's constructor should be like this
mp_obj_t mp_pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);

#endif // MICROPY_INCLUDED_EXTMOD_VIRTPIN_H
#endif

// Event handling and wait-for-event functions.

#ifndef MICROPY_INTERNAL_WFE
// Fallback definition for ports that don't need to suspend the CPU.
#define MICROPY_INTERNAL_WFE(TIMEOUT_MS) (void)0
#endif

#ifndef MICROPY_INTERNAL_EVENT_HOOK
// Fallback definition for ports that don't need any port-specific
// non-blocking event processing.
#define MICROPY_INTERNAL_EVENT_HOOK (void)0
#endif

#endif // MICROPY_INCLUDED_PY_MPHAL_H
/* py/mperrno.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_MPERRNO_H
#define MICROPY_INCLUDED_PY_MPERRNO_H


#if MICROPY_USE_INTERNAL_ERRNO

// MP_Exxx errno's are defined directly as numeric values
// (Linux constants are used as a reference)

#define MP_EPERM              (1) // Operation not permitted
#define MP_ENOENT             (2) // No such file or directory
#define MP_ESRCH              (3) // No such process
#define MP_EINTR              (4) // Interrupted system call
#define MP_EIO                (5) // I/O error
#define MP_ENXIO              (6) // No such device or address
#define MP_E2BIG              (7) // Argument list too long
#define MP_ENOEXEC            (8) // Exec format error
#define MP_EBADF              (9) // Bad file number
#define MP_ECHILD            (10) // No child processes
#define MP_EAGAIN            (11) // Try again
#define MP_ENOMEM            (12) // Out of memory
#define MP_EACCES            (13) // Permission denied
#define MP_EFAULT            (14) // Bad address
#define MP_ENOTBLK           (15) // Block device required
#define MP_EBUSY             (16) // Device or resource busy
#define MP_EEXIST            (17) // File exists
#define MP_EXDEV             (18) // Cross-device link
#define MP_ENODEV            (19) // No such device
#define MP_ENOTDIR           (20) // Not a directory
#define MP_EISDIR            (21) // Is a directory
#define MP_EINVAL            (22) // Invalid argument
#define MP_ENFILE            (23) // File table overflow
#define MP_EMFILE            (24) // Too many open files
#define MP_ENOTTY            (25) // Not a typewriter
#define MP_ETXTBSY           (26) // Text file busy
#define MP_EFBIG             (27) // File too large
#define MP_ENOSPC            (28) // No space left on device
#define MP_ESPIPE            (29) // Illegal seek
#define MP_EROFS             (30) // Read-only file system
#define MP_EMLINK            (31) // Too many links
#define MP_EPIPE             (32) // Broken pipe
#define MP_EDOM              (33) // Math argument out of domain of func
#define MP_ERANGE            (34) // Math result not representable
#define MP_EWOULDBLOCK  MP_EAGAIN // Operation would block
#define MP_EOPNOTSUPP        (95) // Operation not supported on transport endpoint
#define MP_EAFNOSUPPORT      (97) // Address family not supported by protocol
#define MP_EADDRINUSE        (98) // Address already in use
#define MP_ECONNABORTED     (103) // Software caused connection abort
#define MP_ECONNRESET       (104) // Connection reset by peer
#define MP_ENOBUFS          (105) // No buffer space available
#define MP_EISCONN          (106) // Transport endpoint is already connected
#define MP_ENOTCONN         (107) // Transport endpoint is not connected
#define MP_ETIMEDOUT        (110) // Connection timed out
#define MP_ECONNREFUSED     (111) // Connection refused
#define MP_EHOSTUNREACH     (113) // No route to host
#define MP_EALREADY         (114) // Operation already in progress
#define MP_EINPROGRESS      (115) // Operation now in progress
#define MP_ECANCELED        (125) // Operation canceled

#else

// MP_Exxx errno's are defined in terms of system supplied ones

#include <errno.h>

#define MP_EPERM            EPERM
#define MP_ENOENT           ENOENT
#define MP_ESRCH            ESRCH
#define MP_EINTR            EINTR
#define MP_EIO              EIO
#define MP_ENXIO            ENXIO
#define MP_E2BIG            E2BIG
#define MP_ENOEXEC          ENOEXEC
#define MP_EBADF            EBADF
#define MP_ECHILD           ECHILD
#define MP_EAGAIN           EAGAIN
#define MP_ENOMEM           ENOMEM
#define MP_EACCES           EACCES
#define MP_EFAULT           EFAULT
#define MP_ENOTBLK          ENOTBLK
#define MP_EBUSY            EBUSY
#define MP_EEXIST           EEXIST
#define MP_EXDEV            EXDEV
#define MP_ENODEV           ENODEV
#define MP_ENOTDIR          ENOTDIR
#define MP_EISDIR           EISDIR
#define MP_EINVAL           EINVAL
#define MP_ENFILE           ENFILE
#define MP_EMFILE           EMFILE
#define MP_ENOTTY           ENOTTY
#define MP_ETXTBSY          ETXTBSY
#define MP_EFBIG            EFBIG
#define MP_ENOSPC           ENOSPC
#define MP_ESPIPE           ESPIPE
#define MP_EROFS            EROFS
#define MP_EMLINK           EMLINK
#define MP_EPIPE            EPIPE
#define MP_EDOM             EDOM
#define MP_ERANGE           ERANGE
#define MP_EWOULDBLOCK      EWOULDBLOCK
#define MP_EOPNOTSUPP       EOPNOTSUPP
#define MP_EAFNOSUPPORT     EAFNOSUPPORT
#define MP_EADDRINUSE       EADDRINUSE
#define MP_ECONNABORTED     ECONNABORTED
#define MP_ECONNRESET       ECONNRESET
#define MP_ENOBUFS          ENOBUFS
#define MP_EISCONN          EISCONN
#define MP_ENOTCONN         ENOTCONN
#define MP_ETIMEDOUT        ETIMEDOUT
#define MP_ECONNREFUSED     ECONNREFUSED
#define MP_EHOSTUNREACH     EHOSTUNREACH
#define MP_EALREADY         EALREADY
#define MP_EINPROGRESS      EINPROGRESS
#define MP_ECANCELED        ECANCELED

#endif

#if MICROPY_PY_ERRNO


qstr mp_errno_to_str(mp_obj_t errno_val);

#endif

#endif // MICROPY_INCLUDED_PY_MPERRNO_H
/* py/objstr.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_OBJSTR_H
#define MICROPY_INCLUDED_PY_OBJSTR_H

/* py/objarray.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 * Copyright (c) 2014 Paul Sokolovsky
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_OBJARRAY_H
#define MICROPY_INCLUDED_PY_OBJARRAY_H


// Used only for memoryview types, set in "typecode" to indicate a writable memoryview
#define MP_OBJ_ARRAY_TYPECODE_FLAG_RW (0x80)

// Bit size used for mp_obj_array_t.free member.
#define MP_OBJ_ARRAY_FREE_SIZE_BITS (8 * sizeof(size_t) - 8)

// This structure is used for all of bytearray, array.array, memoryview
// objects.  Note that memoryview has different meaning for some fields,
// see comment at the beginning of objarray.c.
typedef struct _mp_obj_array_t {
    mp_obj_base_t base;
    size_t typecode : 8;
    // free is number of unused elements after len used elements
    // alloc size = len + free
    // But for memoryview, 'free' is reused as offset (in elements) into the
    // parent object. (Union is not used to not go into a complication of
    // union-of-bitfields with different toolchains). See comments in
    // objarray.c.
    size_t free : MP_OBJ_ARRAY_FREE_SIZE_BITS;
    size_t len; // in elements
    void *items;
} mp_obj_array_t;

#if MICROPY_PY_BUILTINS_MEMORYVIEW

#define MP_DEFINE_MEMORYVIEW_OBJ(obj_name, typecode, offset, len, ptr) \
    mp_obj_array_t obj_name = {{&mp_type_memoryview}, (typecode), (offset), (len), (ptr)}

static inline void mp_obj_memoryview_init(mp_obj_array_t *self, size_t typecode, size_t offset, size_t len, void *items) {
    self->base.type = &mp_type_memoryview;
    self->typecode = typecode;
    self->free = offset;
    self->len = len;
    self->items = items;
}

#endif

#if MICROPY_PY_ARRAY || MICROPY_PY_BUILTINS_BYTEARRAY
MP_DECLARE_CONST_FUN_OBJ_2(mp_obj_array_append_obj);
MP_DECLARE_CONST_FUN_OBJ_2(mp_obj_array_extend_obj);
#endif

#endif // MICROPY_INCLUDED_PY_OBJARRAY_H

typedef struct _mp_obj_str_t {
    mp_obj_base_t base;
    size_t hash;
    // len == number of bytes used in data, alloc = len + 1 because (at the moment) we also append a null byte
    size_t len;
    const byte *data;
} mp_obj_str_t;

// This static assert is used to ensure that mp_obj_str_t and mp_obj_array_t are compatible,
// meaning that their len and data/items entries are at the same offsets in the struct.
// This allows the same code to be used for str/bytes and bytearray.
#define MP_STATIC_ASSERT_STR_ARRAY_COMPATIBLE \
    MP_STATIC_ASSERT(offsetof(mp_obj_str_t, len) == offsetof(mp_obj_array_t, len) \
    && offsetof(mp_obj_str_t, data) == offsetof(mp_obj_array_t, items))

#define MP_DEFINE_STR_OBJ(obj_name, str) mp_obj_str_t obj_name = {{&mp_type_str}, 0, sizeof(str) - 1, (const byte *)str}

// use this macro to extract the string hash
// warning: the hash can be 0, meaning invalid, and must then be explicitly computed from the data
#define GET_STR_HASH(str_obj_in, str_hash) \
    size_t str_hash; \
    if (mp_obj_is_qstr(str_obj_in)) { \
        str_hash = qstr_hash(MP_OBJ_QSTR_VALUE(str_obj_in)); \
    } else { \
        str_hash = ((mp_obj_str_t *)MP_OBJ_TO_PTR(str_obj_in))->hash; \
    }

// use this macro to extract the string length
#define GET_STR_LEN(str_obj_in, str_len) \
    size_t str_len; \
    if (mp_obj_is_qstr(str_obj_in)) { \
        str_len = qstr_len(MP_OBJ_QSTR_VALUE(str_obj_in)); \
    } else { \
        str_len = ((mp_obj_str_t *)MP_OBJ_TO_PTR(str_obj_in))->len; \
    }

// use this macro to extract the string data and length
#if MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_C || MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_D
const byte *mp_obj_str_get_data_no_check(mp_obj_t self_in, size_t *len);
#define GET_STR_DATA_LEN(str_obj_in, str_data, str_len) \
    size_t str_len; \
    const byte *str_data = mp_obj_str_get_data_no_check(str_obj_in, &str_len);
#else
#define GET_STR_DATA_LEN(str_obj_in, str_data, str_len) \
    const byte *str_data; \
    size_t str_len; \
    if (mp_obj_is_qstr(str_obj_in)) { \
        str_data = qstr_data(MP_OBJ_QSTR_VALUE(str_obj_in), &str_len); \
    } else { \
        MP_STATIC_ASSERT_STR_ARRAY_COMPATIBLE; \
        str_len = ((mp_obj_str_t *)MP_OBJ_TO_PTR(str_obj_in))->len; \
        str_data = ((mp_obj_str_t *)MP_OBJ_TO_PTR(str_obj_in))->data; \
    }
#endif

mp_obj_t mp_obj_str_make_new(const mp_obj_type_t *type_in, size_t n_args, size_t n_kw, const mp_obj_t *args);
void mp_str_print_json(const mp_print_t *print, const byte *str_data, size_t str_len);
mp_obj_t mp_obj_str_format(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs);
mp_obj_t mp_obj_str_split(size_t n_args, const mp_obj_t *args);
mp_obj_t mp_obj_new_str_copy(const mp_obj_type_t *type, const byte *data, size_t len); // for type=str, input data must be valid utf-8
mp_obj_t mp_obj_new_str_of_type(const mp_obj_type_t *type, const byte *data, size_t len); // for type=str, will check utf-8 (raises UnicodeError)

mp_obj_t mp_obj_str_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in);
mp_int_t mp_obj_str_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags);

void mp_obj_str_set_data(mp_obj_str_t *str, const byte *data, size_t len);

const byte *str_index_to_ptr(const mp_obj_type_t *type, const byte *self_data, size_t self_len,
    mp_obj_t index, bool is_slice);
const byte *find_subbytes(const byte *haystack, size_t hlen, const byte *needle, size_t nlen, int direction);

#define MP_DEFINE_BYTES_OBJ(obj_name, target, len) mp_obj_str_t obj_name = {{&mp_type_bytes}, 0, (len), (const byte *)(target)}

mp_obj_t mp_obj_bytes_hex(size_t n_args, const mp_obj_t *args, const mp_obj_type_t *type);
mp_obj_t mp_obj_bytes_fromhex(mp_obj_t type_in, mp_obj_t data);

extern const mp_obj_dict_t mp_obj_str_locals_dict;

#if MICROPY_PY_BUILTINS_MEMORYVIEW && MICROPY_PY_BUILTINS_BYTES_HEX
extern const mp_obj_dict_t mp_obj_memoryview_locals_dict;
#endif

#if MICROPY_PY_BUILTINS_BYTEARRAY
extern const mp_obj_dict_t mp_obj_bytearray_locals_dict;
#endif

#if MICROPY_PY_ARRAY
extern const mp_obj_dict_t mp_obj_array_locals_dict;
#endif

#endif // MICROPY_INCLUDED_PY_OBJSTR_H
/* py/objtype.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_OBJTYPE_H
#define MICROPY_INCLUDED_PY_OBJTYPE_H


// instance object
// creating an instance of a class makes one of these objects
typedef struct _mp_obj_instance_t {
    mp_obj_base_t base;
    mp_map_t members;
    mp_obj_t subobj[];
    // TODO maybe cache __getattr__ and __setattr__ for efficient lookup of them
} mp_obj_instance_t;

#if MICROPY_CPYTHON_COMPAT
// this is needed for object.__new__
mp_obj_instance_t *mp_obj_new_instance(const mp_obj_type_t *cls, const mp_obj_type_t **native_base);
#endif

// these need to be exposed so mp_obj_is_callable can work correctly
bool mp_obj_instance_is_callable(mp_obj_t self_in);
mp_obj_t mp_obj_instance_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args);

#define mp_obj_is_instance_type(type) ((type)->flags & MP_TYPE_FLAG_INSTANCE_TYPE)
#define mp_obj_is_native_type(type) (!((type)->flags & MP_TYPE_FLAG_INSTANCE_TYPE))

// this needs to be exposed for mp_getiter
mp_obj_t mp_obj_instance_getiter(mp_obj_t self_in, mp_obj_iter_buf_t *iter_buf);

MP_DECLARE_CONST_FUN_OBJ_KW(mp_native_base_init_wrapper_obj);

#endif // MICROPY_INCLUDED_PY_OBJTYPE_H
/* py/objmodule.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2019 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_OBJMODULE_H
#define MICROPY_INCLUDED_PY_OBJMODULE_H


#ifndef NO_QSTR
// Only include module definitions when not doing qstr extraction, because the
// qstr extraction stage also generates this module definition header file.
/* moduledefs.h */
// Automatically generated by makemoduledefs.py.

extern const struct _mp_obj_module_t mp_module_array;
#undef MODULE_DEF_ARRAY
#define MODULE_DEF_ARRAY { MP_ROM_QSTR(MP_QSTR_array), MP_ROM_PTR(&mp_module_array) },

extern const struct _mp_obj_module_t mp_module_binascii;
#undef MODULE_DEF_BINASCII
#define MODULE_DEF_BINASCII { MP_ROM_QSTR(MP_QSTR_binascii), MP_ROM_PTR(&mp_module_binascii) },

extern const struct _mp_obj_module_t mp_module_collections;
#undef MODULE_DEF_COLLECTIONS
#define MODULE_DEF_COLLECTIONS { MP_ROM_QSTR(MP_QSTR_collections), MP_ROM_PTR(&mp_module_collections) },

extern const struct _mp_obj_module_t mp_module_errno;
#undef MODULE_DEF_ERRNO
#define MODULE_DEF_ERRNO { MP_ROM_QSTR(MP_QSTR_errno), MP_ROM_PTR(&mp_module_errno) },

extern const struct _mp_obj_module_t mp_module_heapq;
#undef MODULE_DEF_HEAPQ
#define MODULE_DEF_HEAPQ { MP_ROM_QSTR(MP_QSTR_heapq), MP_ROM_PTR(&mp_module_heapq) },

extern const struct _mp_obj_module_t mp_module_io;
#undef MODULE_DEF_IO
#define MODULE_DEF_IO { MP_ROM_QSTR(MP_QSTR_io), MP_ROM_PTR(&mp_module_io) },

extern const struct _mp_obj_module_t mp_module_json;
#undef MODULE_DEF_JSON
#define MODULE_DEF_JSON { MP_ROM_QSTR(MP_QSTR_json), MP_ROM_PTR(&mp_module_json) },

extern const struct _mp_obj_module_t mp_module_random;
#undef MODULE_DEF_RANDOM
#define MODULE_DEF_RANDOM { MP_ROM_QSTR(MP_QSTR_random), MP_ROM_PTR(&mp_module_random) },

extern const struct _mp_obj_module_t mp_module_re;
#undef MODULE_DEF_RE
#define MODULE_DEF_RE { MP_ROM_QSTR(MP_QSTR_re), MP_ROM_PTR(&mp_module_re) },

extern const struct _mp_obj_module_t mp_module_struct;
#undef MODULE_DEF_STRUCT
#define MODULE_DEF_STRUCT { MP_ROM_QSTR(MP_QSTR_struct), MP_ROM_PTR(&mp_module_struct) },

extern const struct _mp_obj_module_t mp_module___main__;
#undef MODULE_DEF___MAIN__
#define MODULE_DEF___MAIN__ { MP_ROM_QSTR(MP_QSTR___main__), MP_ROM_PTR(&mp_module___main__) },

extern const struct _mp_obj_module_t mp_module_builtins;
#undef MODULE_DEF_BUILTINS
#define MODULE_DEF_BUILTINS { MP_ROM_QSTR(MP_QSTR_builtins), MP_ROM_PTR(&mp_module_builtins) },

extern const struct _mp_obj_module_t mp_module_cmath;
#undef MODULE_DEF_CMATH
#define MODULE_DEF_CMATH { MP_ROM_QSTR(MP_QSTR_cmath), MP_ROM_PTR(&mp_module_cmath) },

extern const struct _mp_obj_module_t mp_module_gc;
#undef MODULE_DEF_GC
#define MODULE_DEF_GC { MP_ROM_QSTR(MP_QSTR_gc), MP_ROM_PTR(&mp_module_gc) },

extern const struct _mp_obj_module_t mp_module_math;
#undef MODULE_DEF_MATH
#define MODULE_DEF_MATH { MP_ROM_QSTR(MP_QSTR_math), MP_ROM_PTR(&mp_module_math) },

extern const struct _mp_obj_module_t mp_module_micropython;
#undef MODULE_DEF_MICROPYTHON
#define MODULE_DEF_MICROPYTHON { MP_ROM_QSTR(MP_QSTR_micropython), MP_ROM_PTR(&mp_module_micropython) },

extern const struct _mp_obj_module_t mp_module_sys;
#undef MODULE_DEF_SYS
#define MODULE_DEF_SYS { MP_ROM_QSTR(MP_QSTR_sys), MP_ROM_PTR(&mp_module_sys) },


#define MICROPY_REGISTERED_MODULES \
    MODULE_DEF_BUILTINS \
    MODULE_DEF_CMATH \
    MODULE_DEF_GC \
    MODULE_DEF_MATH \
    MODULE_DEF_MICROPYTHON \
    MODULE_DEF_SYS \
    MODULE_DEF___MAIN__ \
// MICROPY_REGISTERED_MODULES

#define MICROPY_HAVE_REGISTERED_EXTENSIBLE_MODULES  10

#define MICROPY_REGISTERED_EXTENSIBLE_MODULES \
    MODULE_DEF_ARRAY \
    MODULE_DEF_BINASCII \
    MODULE_DEF_COLLECTIONS \
    MODULE_DEF_ERRNO \
    MODULE_DEF_HEAPQ \
    MODULE_DEF_IO \
    MODULE_DEF_JSON \
    MODULE_DEF_RANDOM \
    MODULE_DEF_RE \
    MODULE_DEF_STRUCT \
// MICROPY_REGISTERED_EXTENSIBLE_MODULES
#endif

extern const mp_map_t mp_builtin_module_map;

#if MICROPY_HAVE_REGISTERED_EXTENSIBLE_MODULES
extern const mp_map_t mp_builtin_extensible_module_map;
#endif

mp_obj_t mp_module_get_builtin(qstr module_name, bool extensible);

void mp_module_generic_attr(qstr attr, mp_obj_t *dest, const uint16_t *keys, mp_obj_t *values);

static inline mp_obj_dict_t *mp_obj_module_get_globals(mp_obj_t module) {
    return ((mp_obj_module_t *)MP_OBJ_TO_PTR(module))->globals;
}

#endif // MICROPY_INCLUDED_PY_OBJMODULE_H
/* py/objint.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_OBJINT_H
#define MICROPY_INCLUDED_PY_OBJINT_H

/* py/mpz.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_MPZ_H
#define MICROPY_INCLUDED_PY_MPZ_H

#include <stdint.h>


// This mpz module implements arbitrary precision integers.
//
// The storage for each digit is defined by mpz_dig_t.  The actual number of
// bits in mpz_dig_t that are used is defined by MPZ_DIG_SIZE.  The machine must
// also provide a type that is twice as wide as mpz_dig_t, in both signed and
// unsigned versions.
//
// MPZ_DIG_SIZE can be between 4 and 8*sizeof(mpz_dig_t), but it makes most
// sense to have it as large as possible.  If MPZ_DIG_SIZE is not already
// defined then it is auto-detected below, depending on the machine.  The types
// are then set based on the value of MPZ_DIG_SIZE (although they can be freely
// changed so long as the constraints mentioned above are met).

#ifndef MPZ_DIG_SIZE
  #if defined(__x86_64__) || defined(_WIN64)
// 64-bit machine, using 32-bit storage for digits
    #define MPZ_DIG_SIZE (32)
  #else
// default: 32-bit machine, using 16-bit storage for digits
    #define MPZ_DIG_SIZE (16)
  #endif
#endif

#if MPZ_DIG_SIZE > 16
#define MPZ_DBL_DIG_SIZE (64)
typedef uint32_t mpz_dig_t;
typedef uint64_t mpz_dbl_dig_t;
typedef int64_t mpz_dbl_dig_signed_t;
#elif MPZ_DIG_SIZE > 8
#define MPZ_DBL_DIG_SIZE (32)
typedef uint16_t mpz_dig_t;
typedef uint32_t mpz_dbl_dig_t;
typedef int32_t mpz_dbl_dig_signed_t;
#elif MPZ_DIG_SIZE > 4
#define MPZ_DBL_DIG_SIZE (16)
typedef uint8_t mpz_dig_t;
typedef uint16_t mpz_dbl_dig_t;
typedef int16_t mpz_dbl_dig_signed_t;
#else
#define MPZ_DBL_DIG_SIZE (8)
typedef uint8_t mpz_dig_t;
typedef uint8_t mpz_dbl_dig_t;
typedef int8_t mpz_dbl_dig_signed_t;
#endif

#ifdef _WIN64
  #ifdef __MINGW32__
    #define MPZ_LONG_1 1LL
  #else
    #define MPZ_LONG_1 1i64
  #endif
#else
  #define MPZ_LONG_1 1L
#endif

// these define the maximum storage needed to hold an int or long long
#define MPZ_NUM_DIG_FOR_INT ((sizeof(mp_int_t) * 8 + MPZ_DIG_SIZE - 1) / MPZ_DIG_SIZE)
#define MPZ_NUM_DIG_FOR_LL ((sizeof(long long) * 8 + MPZ_DIG_SIZE - 1) / MPZ_DIG_SIZE)

typedef struct _mpz_t {
    // Zero has neg=0, len=0.  Negative zero is not allowed.
    size_t neg : 1;
    size_t fixed_dig : 1; // flag, 'dig' buffer cannot be reallocated
    size_t alloc : (8 * sizeof(size_t) - 2); // number of entries allocated in 'dig'
    size_t len; // number of entries used in 'dig'
    mpz_dig_t *dig;
} mpz_t;

// convenience macro to declare an mpz with a digit array from the stack, initialised by an integer
#define MPZ_CONST_INT(z, val) mpz_t z; mpz_dig_t z##_digits[MPZ_NUM_DIG_FOR_INT]; mpz_init_fixed_from_int(&z, z_digits, MPZ_NUM_DIG_FOR_INT, val);

void mpz_init_zero(mpz_t *z);
void mpz_init_from_int(mpz_t *z, mp_int_t val);
void mpz_init_fixed_from_int(mpz_t *z, mpz_dig_t *dig, size_t dig_alloc, mp_int_t val);
void mpz_deinit(mpz_t *z);

void mpz_set(mpz_t *dest, const mpz_t *src);
void mpz_set_from_int(mpz_t *z, mp_int_t src);
void mpz_set_from_ll(mpz_t *z, long long i, bool is_signed);
#if MICROPY_PY_BUILTINS_FLOAT
void mpz_set_from_float(mpz_t *z, mp_float_t src);
#endif
size_t mpz_set_from_str(mpz_t *z, const char *str, size_t len, bool neg, unsigned int base);
void mpz_set_from_bytes(mpz_t *z, bool big_endian, size_t len, const byte *buf);

static inline bool mpz_is_zero(const mpz_t *z) {
    return z->len == 0;
}
static inline bool mpz_is_neg(const mpz_t *z) {
    return z->neg != 0;
}
int mpz_cmp(const mpz_t *lhs, const mpz_t *rhs);

void mpz_abs_inpl(mpz_t *dest, const mpz_t *z);
void mpz_neg_inpl(mpz_t *dest, const mpz_t *z);
void mpz_not_inpl(mpz_t *dest, const mpz_t *z);
void mpz_shl_inpl(mpz_t *dest, const mpz_t *lhs, mp_uint_t rhs);
void mpz_shr_inpl(mpz_t *dest, const mpz_t *lhs, mp_uint_t rhs);
void mpz_add_inpl(mpz_t *dest, const mpz_t *lhs, const mpz_t *rhs);
void mpz_sub_inpl(mpz_t *dest, const mpz_t *lhs, const mpz_t *rhs);
void mpz_mul_inpl(mpz_t *dest, const mpz_t *lhs, const mpz_t *rhs);
void mpz_pow_inpl(mpz_t *dest, const mpz_t *lhs, const mpz_t *rhs);
void mpz_pow3_inpl(mpz_t *dest, const mpz_t *lhs, const mpz_t *rhs, const mpz_t *mod);
void mpz_and_inpl(mpz_t *dest, const mpz_t *lhs, const mpz_t *rhs);
void mpz_or_inpl(mpz_t *dest, const mpz_t *lhs, const mpz_t *rhs);
void mpz_xor_inpl(mpz_t *dest, const mpz_t *lhs, const mpz_t *rhs);
void mpz_divmod_inpl(mpz_t *dest_quo, mpz_t *dest_rem, const mpz_t *lhs, const mpz_t *rhs);

static inline size_t mpz_max_num_bits(const mpz_t *z) {
    return z->len * MPZ_DIG_SIZE;
}
mp_int_t mpz_hash(const mpz_t *z);
bool mpz_as_int_checked(const mpz_t *z, mp_int_t *value);
bool mpz_as_uint_checked(const mpz_t *z, mp_uint_t *value);
// Returns true if 'z' fit into 'len' bytes of 'buf' without overflowing, 'buf' is truncated otherwise.
bool mpz_as_bytes(const mpz_t *z, bool big_endian, bool as_signed, size_t len, byte *buf);
#if MICROPY_PY_BUILTINS_FLOAT
mp_float_t mpz_as_float(const mpz_t *z);
#endif
size_t mpz_as_str_inpl(const mpz_t *z, unsigned int base, const char *prefix, char base_char, char comma, char *str);

#endif // MICROPY_INCLUDED_PY_MPZ_H

typedef struct _mp_obj_int_t {
    mp_obj_base_t base;
    #if MICROPY_LONGINT_IMPL == MICROPY_LONGINT_IMPL_LONGLONG
    mp_longint_impl_t val;
    #elif MICROPY_LONGINT_IMPL == MICROPY_LONGINT_IMPL_MPZ
    mpz_t mpz;
    #endif
} mp_obj_int_t;

extern const mp_obj_int_t mp_sys_maxsize_obj;

#if MICROPY_PY_BUILTINS_FLOAT
mp_float_t mp_obj_int_as_float_impl(mp_obj_t self_in);
#endif

size_t mp_int_format_size(size_t num_bits, int base, const char *prefix, char comma);

mp_obj_int_t *mp_obj_int_new_mpz(void);

void mp_obj_int_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind);
char *mp_obj_int_formatted(char **buf, size_t *buf_size, size_t *fmt_size, mp_const_obj_t self_in,
    int base, const char *prefix, char base_char, char comma);
char *mp_obj_int_formatted_impl(char **buf, size_t *buf_size, size_t *fmt_size, mp_const_obj_t self_in,
    int base, const char *prefix, char base_char, char comma);

mp_int_t mp_obj_int_hash(mp_obj_t self_in);
mp_obj_t mp_obj_int_from_bytes_impl(bool big_endian, size_t len, const byte *buf);
// Write an integer to a byte sequence.
// If overflow_check is true, raises OverflowError if 'self_in' doesn't fit. If false, truncate to fit.
void mp_obj_int_to_bytes(mp_obj_t self_in, size_t buf_len, byte *buf, bool big_endian, bool is_signed, bool overflow_check);
int mp_obj_int_sign(mp_obj_t self_in);
mp_obj_t mp_obj_int_unary_op(mp_unary_op_t op, mp_obj_t o_in);
mp_obj_t mp_obj_int_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in);
mp_obj_t mp_obj_int_binary_op_extra_cases(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in);
mp_obj_t mp_obj_int_pow3(mp_obj_t base, mp_obj_t exponent,  mp_obj_t modulus);

#endif // MICROPY_INCLUDED_PY_OBJINT_H
/* py/objfun.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_OBJFUN_H
#define MICROPY_INCLUDED_PY_OBJFUN_H


typedef struct _mp_obj_fun_bc_t {
    mp_obj_base_t base;
    const mp_module_context_t *context;         // context within which this function was defined
    struct _mp_raw_code_t *const *child_table;  // table of children
    const byte *bytecode;                       // bytecode for the function
    #if MICROPY_PY_SYS_SETTRACE
    const struct _mp_raw_code_t *rc;
    #endif
    // the following extra_args array is allocated space to take (in order):
    //  - values of positional default args (if any)
    //  - a single slot for default kw args dict (if it has them)
    mp_obj_t extra_args[];
} mp_obj_fun_bc_t;

typedef struct _mp_obj_fun_asm_t {
    mp_obj_base_t base;
    size_t n_args;
    const void *fun_data; // GC must be able to trace this pointer
    mp_uint_t type_sig;
} mp_obj_fun_asm_t;

mp_obj_t mp_obj_new_fun_bc(const mp_obj_t *def_args, const byte *code, const mp_module_context_t *cm, struct _mp_raw_code_t *const *raw_code_table);
qstr mp_obj_fun_bc_get_name(const mp_obj_fun_bc_t *fun);
void mp_obj_fun_bc_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest);

#if MICROPY_ENABLE_NATIVE_CODE

static inline mp_obj_t mp_obj_new_fun_native(const mp_obj_t *def_args, const void *fun_data, const mp_module_context_t *mc, struct _mp_raw_code_t *const *child_table) {
    mp_obj_fun_bc_t *o = (mp_obj_fun_bc_t *)MP_OBJ_TO_PTR(mp_obj_new_fun_bc(def_args, (const byte *)fun_data, mc, child_table));
    o->base.type = &mp_type_fun_native;
    return MP_OBJ_FROM_PTR(o);
}

static inline mp_obj_t mp_obj_new_fun_viper(const void *fun_data, const mp_module_context_t *mc, struct _mp_raw_code_t *const *child_table) {
    mp_obj_fun_bc_t *o = mp_obj_malloc(mp_obj_fun_bc_t, &mp_type_fun_viper);
    o->bytecode = (const byte *)fun_data;
    o->context = mc;
    o->child_table = child_table;
    return MP_OBJ_FROM_PTR(o);
}

static inline const uint8_t *mp_obj_fun_native_get_prelude_ptr(const mp_obj_fun_bc_t *fun_native) {
    // Obtain a pointer to the start of the function prelude, based on prelude_ptr_index.
    uintptr_t prelude_ptr_index = ((uintptr_t *)fun_native->bytecode)[0];
    const uint8_t *prelude_ptr;
    if (prelude_ptr_index == 0) {
        prelude_ptr = (const uint8_t *)fun_native->child_table;
    } else {
        prelude_ptr = (const uint8_t *)fun_native->child_table[prelude_ptr_index];
    }
    return prelude_ptr;
}

static inline void *mp_obj_fun_native_get_function_start(const mp_obj_fun_bc_t *fun_native) {
    // Obtain a pointer to the start of the function executable machine code.
    return MICROPY_MAKE_POINTER_CALLABLE((void *)(fun_native->bytecode + sizeof(uintptr_t)));
}

static inline void *mp_obj_fun_native_get_generator_start(const mp_obj_fun_bc_t *fun_native) {
    // Obtain a pointer to the start of the generator executable machine code.
    uintptr_t start_offset = ((uintptr_t *)fun_native->bytecode)[1];
    return MICROPY_MAKE_POINTER_CALLABLE((void *)(fun_native->bytecode + start_offset));
}

static inline void *mp_obj_fun_native_get_generator_resume(const mp_obj_fun_bc_t *fun_native) {
    // Obtain a pointer to the resume location of the generator executable machine code.
    return MICROPY_MAKE_POINTER_CALLABLE((void *)&((uintptr_t *)fun_native->bytecode)[2]);
}

#endif

#if MICROPY_EMIT_INLINE_ASM
static inline mp_obj_t mp_obj_new_fun_asm(size_t n_args, const void *fun_data, mp_uint_t type_sig) {
    mp_obj_fun_asm_t *o = (mp_obj_fun_asm_t *)mp_obj_malloc(mp_obj_fun_asm_t, &mp_type_fun_asm);
    o->n_args = n_args;
    o->fun_data = (const byte *)fun_data;
    o->type_sig = type_sig;
    return MP_OBJ_FROM_PTR(o);
}
#endif

#endif // MICROPY_INCLUDED_PY_OBJFUN_H
/* py/binary.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Paul Sokolovsky
 * Copyright (c) 2014-2017 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_BINARY_H
#define MICROPY_INCLUDED_PY_BINARY_H


// Use special typecode to differentiate repr() of bytearray vs array.array('B')
// (underlyingly they're same).  Can't use 0 here because that's used to detect
// type-specification errors due to end-of-string.
#define BYTEARRAY_TYPECODE 1

size_t mp_binary_get_size(char struct_type, char val_type, size_t *palign);
mp_obj_t mp_binary_get_val_array(char typecode, void *p, size_t index);
void mp_binary_set_val_array(char typecode, void *p, size_t index, mp_obj_t val_in);
mp_obj_t mp_binary_get_val(char struct_type, char val_type, byte *p_base, byte **ptr);
void mp_binary_set_val(char struct_type, char val_type, mp_obj_t val_in, byte *p_base, byte **ptr);
long long mp_binary_get_int(size_t size, bool is_signed, bool big_endian, const byte *src);
void mp_binary_set_int(size_t dest_sz, byte *dest, size_t val_sz, mp_uint_t val, bool big_endian);

#endif // MICROPY_INCLUDED_PY_BINARY_H
/* py/smallint.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_SMALLINT_H
#define MICROPY_INCLUDED_PY_SMALLINT_H


// Functions for small integer arithmetic

#ifndef MP_SMALL_INT_MIN

// In SMALL_INT, next-to-highest bits is used as sign, so both must match for value in range
#if MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_A || MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_C

#define MP_SMALL_INT_MIN ((mp_int_t)(((mp_int_t)MP_OBJ_WORD_MSBIT_HIGH) >> 1))
#define MP_SMALL_INT_FITS(n) ((((n) ^ ((mp_uint_t)(n) << 1)) & MP_OBJ_WORD_MSBIT_HIGH) == 0)
// Mask to truncate mp_int_t to positive value
#define MP_SMALL_INT_POSITIVE_MASK ~(MP_OBJ_WORD_MSBIT_HIGH | (MP_OBJ_WORD_MSBIT_HIGH >> 1))

#elif MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_B

#define MP_SMALL_INT_MIN ((mp_int_t)(((mp_int_t)MP_OBJ_WORD_MSBIT_HIGH) >> 2))
#define MP_SMALL_INT_FITS(n) ((((n) & MP_SMALL_INT_MIN) == 0) || (((n) & MP_SMALL_INT_MIN) == MP_SMALL_INT_MIN))
// Mask to truncate mp_int_t to positive value
#define MP_SMALL_INT_POSITIVE_MASK ~(MP_OBJ_WORD_MSBIT_HIGH | (MP_OBJ_WORD_MSBIT_HIGH >> 1) | (MP_OBJ_WORD_MSBIT_HIGH >> 2))

#elif MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_D

#define MP_SMALL_INT_MIN ((mp_int_t)(((mp_int_t)0xffff800000000000) >> 1))
#define MP_SMALL_INT_FITS(n) ((((n) ^ ((n) << 1)) & 0xffff800000000000) == 0)
// Mask to truncate mp_int_t to positive value
#define MP_SMALL_INT_POSITIVE_MASK ~(0xffff800000000000 | (0xffff800000000000 >> 1))

#endif

#endif

#define MP_SMALL_INT_MAX ((mp_int_t)(~(MP_SMALL_INT_MIN)))

// https://stackoverflow.com/a/4589384/1976323
// Number of bits in inttype_MAX, or in any (1<<k)-1 where 0 <= k < 2040
#define MP_IMAX_BITS(m) ((m) / ((m) % 255 + 1) / 255 % 255 * 8 + 7 - 86 / ((m) % 255 + 12))

// The number of bits in a MP_SMALL_INT including the sign bit.
#define MP_SMALL_INT_BITS (MP_IMAX_BITS(MP_SMALL_INT_MAX) + 1)

mp_int_t mp_small_int_modulo(mp_int_t dividend, mp_int_t divisor);
mp_int_t mp_small_int_floor_divide(mp_int_t num, mp_int_t denom);

#endif // MICROPY_INCLUDED_PY_SMALLINT_H
/* py/stream.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 * Copyright (c) 2014-2016 Paul Sokolovsky
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_PY_STREAM_H
#define MICROPY_INCLUDED_PY_STREAM_H


#define MP_STREAM_ERROR ((mp_uint_t)-1)

// Stream ioctl request codes
#define MP_STREAM_FLUSH             (1)
#define MP_STREAM_SEEK              (2)
#define MP_STREAM_POLL              (3)
#define MP_STREAM_CLOSE             (4)
#define MP_STREAM_TIMEOUT           (5)  // Get/set timeout (single op)
#define MP_STREAM_GET_OPTS          (6)  // Get stream options
#define MP_STREAM_SET_OPTS          (7)  // Set stream options
#define MP_STREAM_GET_DATA_OPTS     (8)  // Get data/message options
#define MP_STREAM_SET_DATA_OPTS     (9)  // Set data/message options
#define MP_STREAM_GET_FILENO        (10) // Get fileno of underlying file
#define MP_STREAM_GET_BUFFER_SIZE   (11) // Get preferred buffer size for file
#if MICROPY_STREAMS_DELEGATE_ERROR
#define MP_STREAM_RAISE_ERROR       (12) // Raise an error with detailed error string
#endif

// These poll ioctl values are compatible with Linux
#define MP_STREAM_POLL_RD       (0x0001)
#define MP_STREAM_POLL_WR       (0x0004)
#define MP_STREAM_POLL_ERR      (0x0008)
#define MP_STREAM_POLL_HUP      (0x0010)
#define MP_STREAM_POLL_NVAL     (0x0020)

// Argument structure for MP_STREAM_SEEK
struct mp_stream_seek_t {
    // If whence == MP_SEEK_SET, offset should be treated as unsigned.
    // This allows dealing with full-width stream sizes (16, 32, 64,
    // etc. bits). For other seek types, should be treated as signed.
    mp_off_t offset;
    int whence;
};

// seek ioctl "whence" values
#define MP_SEEK_SET (0)
#define MP_SEEK_CUR (1)
#define MP_SEEK_END (2)

// Stream protocol
typedef struct _mp_stream_p_t {
    // On error, functions should return MP_STREAM_ERROR and fill in *errcode
    // (values are are implementation-dependent, but will be exposed to user).
    // If MICROPY_STREAMS_DELEGATE_ERROR is enabled and ioctl is not null the
    // stream will receive a MP_STREAM_RAISE_ERROR ioctl request with the arg
    // containing the errcode, so it may raise a more detailed error. If that
    // ioctl returns without raising, an OSError with the errcode will raised.
    mp_uint_t (*read)(mp_obj_t obj, void *buf, mp_uint_t size, int *errcode);
    mp_uint_t (*write)(mp_obj_t obj, const void *buf, mp_uint_t size, int *errcode);
    mp_uint_t (*ioctl)(mp_obj_t obj, mp_uint_t request, uintptr_t arg, int *errcode);
    mp_uint_t is_text : 1; // default is bytes, set this for text stream
} mp_stream_p_t;

MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_stream_read_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_stream_read1_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_stream_readinto_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_stream_readinto1_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_stream_unbuffered_readline_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_stream_unbuffered_readlines_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_stream_write_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_stream_write1_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_stream_close_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_stream___exit___obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_stream_seek_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_stream_tell_obj);
MP_DECLARE_CONST_FUN_OBJ_1(mp_stream_flush_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_stream_ioctl_obj);

// these are for mp_get_stream_raise and can be or'd together
#define MP_STREAM_OP_READ (1)
#define MP_STREAM_OP_WRITE (2)
#define MP_STREAM_OP_IOCTL (4)

// Object is assumed to have a non-NULL stream protocol with valid r/w/ioctl methods
static inline const mp_stream_p_t *mp_get_stream(mp_const_obj_t self) {
    return (const mp_stream_p_t *)MP_OBJ_TYPE_GET_SLOT(((const mp_obj_base_t *)MP_OBJ_TO_PTR(self))->type, protocol);
}

const mp_stream_p_t *mp_get_stream_raise(mp_obj_t self_in, int flags);
mp_obj_t mp_stream_close(mp_obj_t stream);

// Iterator which uses mp_stream_unbuffered_readline_obj
mp_obj_t mp_stream_unbuffered_iter(mp_obj_t self);

mp_obj_t mp_stream_write(mp_obj_t self_in, const void *buf, size_t len, byte flags);

// C-level helper functions
#define MP_STREAM_RW_READ  0
#define MP_STREAM_RW_WRITE 2
#define MP_STREAM_RW_ONCE  1
mp_uint_t mp_stream_rw(mp_obj_t stream, void *buf, mp_uint_t size, int *errcode, byte flags);
#define mp_stream_write_exactly(stream, buf, size, err) mp_stream_rw(stream, (byte *)buf, size, err, MP_STREAM_RW_WRITE)
#define mp_stream_read_exactly(stream, buf, size, err) mp_stream_rw(stream, buf, size, err, MP_STREAM_RW_READ)
mp_off_t mp_stream_seek(mp_obj_t stream, mp_off_t offset, int whence, int *errcode);

void mp_stream_write_adaptor(void *self, const char *buf, size_t len);

#if MICROPY_STREAMS_POSIX_API
#include <sys/types.h>
// Functions with POSIX-compatible signatures
// "stream" is assumed to be a pointer to a concrete object with the stream protocol
ssize_t mp_stream_posix_write(void *stream, const void *buf, size_t len);
ssize_t mp_stream_posix_read(void *stream, void *buf, size_t len);
off_t mp_stream_posix_lseek(void *stream, off_t offset, int whence);
int mp_stream_posix_fsync(void *stream);
#endif

#if MICROPY_STREAMS_NON_BLOCK
#define mp_is_nonblocking_error(errno) ((errno) == MP_EAGAIN || (errno) == MP_EWOULDBLOCK)
#else
#define mp_is_nonblocking_error(errno) (0)
#endif

#endif // MICROPY_INCLUDED_PY_STREAM_H
/* shared/runtime/gchelper.h */
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_LIB_UTILS_GCHELPER_H
#define MICROPY_INCLUDED_LIB_UTILS_GCHELPER_H

#include <stdint.h>

#if MICROPY_GCREGS_SETJMP
#include <setjmp.h>
typedef jmp_buf gc_helper_regs_t;
#else

#if defined(__x86_64__)
typedef uintptr_t gc_helper_regs_t[6];
#elif defined(__i386__)
typedef uintptr_t gc_helper_regs_t[4];
#elif defined(__thumb2__) || defined(__thumb__) || defined(__arm__)
typedef uintptr_t gc_helper_regs_t[10];
#elif defined(__aarch64__)
typedef uintptr_t gc_helper_regs_t[11]; // x19-x29
#elif defined(__riscv) && (__riscv_xlen <= 64)
typedef uintptr_t gc_helper_regs_t[12]; // S0-S11
#elif defined(__loongarch__) && defined(__loongarch64)
typedef uintptr_t gc_helper_regs_t[10]; // S0-S9
#elif defined(__powerpc__) && defined(__powerpc64__)
typedef uintptr_t gc_helper_regs_t[18]; // r14-r31
#endif

#endif

void gc_helper_collect_regs_and_stack(void);

#endif // MICROPY_INCLUDED_LIB_UTILS_GCHELPER_H

#if defined(__GNUC__)
#pragma GCC visibility pop
#endif

#endif
