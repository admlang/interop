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
