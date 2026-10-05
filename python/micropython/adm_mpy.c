// The C side of adm.interop.python: MicroPython behind functions that take and
// return integers.
//
// A Python value held by ADM is a slot in its runtime's table. A function that
// returns one returns the slot number with the value's kind in the top byte,
// or 0 after putting the exception aside for ampyException.
//
// MicroPython reports errors with longjmp, which must never cross an ADM
// frame: every entry runs its Python calls inside nlr_push (protect), and a
// host function's error is raised only after the ADM function has returned.
//
// MicroPython keeps its state in globals, so a program has one runtime at a
// time. It has one caller at a time too: every entry takes the runtime's lock,
// which belongs to an ADM task and counts, because a host function called from
// Python calls back in.
//
// The collector scans the C stack between the outermost protect and the
// collection, so a Python object is held only in frames below a protect, in a
// slot, or in one of the runtime's roots.

// clock_gettime and strdup.
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <pthread.h>
#include <stdatomic.h>
#include <time.h>

#include "micropython.h"

// The running ADM task, NULL on a thread that runs none (ADM runtime).
extern void* adm_sched_current(void);

// Calls ADM host function `fn` with `argc` arguments, the first four in
// a0..a3 and the rest through ampyArgs. Returns a slot to hand to Python, -1
// for None, or 0 after ampyThrow.
typedef int64_t (*AmpyHost)(int64_t fn, int64_t argc, int64_t a0, int64_t a1, int64_t a2, int64_t a3);

enum {
    K_NONE, K_BOOL, K_INT, K_FLOAT, K_STRING, K_BYTES, K_LIST, K_TUPLE, K_DICT, K_FUNCTION, K_MODULE, K_OBJECT, K_FOREIGN,
};

#define KIND_SHIFT 56
#define SLOT_MASK ((INT64_C(1) << KIND_SHIFT) - 1)

enum { STOP_NONE, STOP_INTERRUPTED, STOP_TIMED_OUT, STOP_OUT_OF_MEMORY };

// Backward jumps between two looks at the interrupt flag and the deadline.
#define POLL_COUNT 1000

// The first heap area; later ones double the heap.
#define FIRST_AREA (256 * 1024)

// C stack a script may use below its entry, unless the program says.
#define DEFAULT_STACK (1024 * 1024)

typedef struct {
    char* name;
    char* source;
    size_t len;
} AmpySource;

typedef struct {
    void* start;
    size_t size;
} AmpyArea;

typedef struct Ampy {
    bool open;
    AmpyHost host;

    // The values ADM holds; a free slot holds MP_OBJ_NULL.
    mp_obj_t* slots;
    int64_t* freeSlots;
    int64_t slotCap, freeLen, live;

    // The exception the last failed call put aside, and the one the host
    // call in progress raises when it returns 0.
    mp_obj_t exception, thrown;
    bool hasException;
    int failStop;

    // Arguments of the host call in progress beyond the first four.
    int64_t* args;
    int64_t argCount;
    // Host calls in progress. While one runs, slots its task releases wait in
    // the queue: the slot it returns is released before C reads it.
    int64_t hostDepth;
    int64_t protectDepth;

    AmpySource* sources;
    int64_t sourceLen, sourceCap;

    // The lock: owner is the task inside, depth its nesting.
    _Atomic(void*) owner;
    int64_t depth;
    atomic_int waiters;
    pthread_mutex_t mu;
    pthread_cond_t cv;

    // Slots released by tasks outside the lock, freed at the next entry.
    pthread_mutex_t queueMu;
    int64_t* released;
    int64_t releasedLen, releasedCap;
    atomic_bool hasReleased;
    int64_t* drained;
    int64_t drainedCap;

    // Ids of foreign objects that were collected.
    int64_t* dead;
    int64_t deadLen, deadCap;

    // The heap's areas and their bytes; the heap stops growing at memoryLimit
    // (0 for none).
    AmpyArea* areas;
    int64_t areaLen, areaCap;
    size_t heapBytes, memoryLimit;

    size_t stackSize;
    atomic_int interrupt;
    int stop;
    int64_t timeoutNs, deadlineNs;
} Ampy;

// The program's runtime; MicroPython's state is its.
static Ampy* current;
static pthread_mutex_t currentMu = PTHREAD_MUTEX_INITIALIZER;

// Counts down to the next ampy_poll (MICROPY_VM_HOOK_LOOP).
int ampy_ticks = POLL_COUNT;

// A length-prefixed block handed to ADM, which copies it out with ampyBlobTake.
typedef struct {
    int64_t len;
    uint8_t data[];
} AmpyBlob;

static AmpyBlob* blob_new(const void* data, size_t len) {
    AmpyBlob* b = malloc(sizeof(AmpyBlob) + len + 1);
    if (!b) return NULL;
    b->len = (int64_t)len;
    if (len) memcpy(b->data, data, len);
    b->data[len] = 0;
    return b;
}

int64_t ampyBlobLen(AmpyBlob* b) { return b ? b->len : 0; }

void ampyBlobTake(AmpyBlob* b, uint8_t* out, int64_t cap) {
    if (!b) return;
    int64_t n = b->len < cap ? b->len : cap;
    if (n > 0) memcpy(out, b->data, (size_t)n);
    free(b);
}

static int64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
}

static void* self_id(void) {
    void* task = adm_sched_current();
    if (task) return task;
    return (void*)((uintptr_t)pthread_self() | 1);
}

// --- What MicroPython asks of its host ---------------------------------------

void* ampy_heap_alloc(size_t size) {
    Ampy* r = current;
    if (!r) return NULL;
    if (r->areaLen == r->areaCap) {
        int64_t cap = r->areaCap ? r->areaCap * 2 : 8;
        AmpyArea* grown = realloc(r->areas, (size_t)cap * sizeof(AmpyArea));
        if (!grown) return NULL;
        r->areas = grown;
        r->areaCap = cap;
    }
    void* area = malloc(size);
    if (!area) return NULL;
    r->areas[r->areaLen++] = (AmpyArea){area, size};
    r->heapBytes += size;
    return area;
}

void ampy_heap_free(void* area) {
    Ampy* r = current;
    if (!r) return;
    for (int64_t i = 0; i < r->areaLen; i++) {
        if (r->areas[i].start != area) continue;
        r->heapBytes -= r->areas[i].size;
        r->areas[i] = r->areas[--r->areaLen];
        break;
    }
    free(area);
}

// Bytes the heap may still grow by.
size_t gc_get_max_new_split(void) {
    Ampy* r = current;
    if (!r) return 0;
    if (!r->memoryLimit) return SIZE_MAX / 4;
    return r->heapBytes >= r->memoryLimit ? 0 : r->memoryLimit - r->heapBytes;
}

void gc_collect(void) {
    Ampy* r = current;
    gc_collect_start();
    gc_helper_collect_regs_and_stack();
    if (r) {
        gc_collect_root((void**)r->slots, (size_t)r->slotCap);
        gc_collect_root((void**)&r->exception, 1);
        gc_collect_root((void**)&r->thrown, 1);
    }
    gc_collect_end();
}

void ampy_print(const char* str, size_t len) { fwrite(str, 1, len, stdout); }

void mp_hal_set_interrupt_char(int c) { (void)c; }

MP_NORETURN void nlr_jump_fail(void* val) {
    (void)val;
    fprintf(stderr, "adm.interop.python: a Python exception escaped its entry\n");
    abort();
}

// Stops the script when it was interrupted or ran past its deadline. The
// reason stays set until the outermost entry returns, and from then on the
// script is stopped again at every backward jump: one that catches the
// exception does not outlast its loop.
void ampy_poll(void) {
    Ampy* r = current;
    ampy_ticks = POLL_COUNT;
    if (!r) return;
    if (r->stop == STOP_NONE) {
        if (atomic_exchange(&r->interrupt, 0))
            r->stop = STOP_INTERRUPTED;
        else if (r->deadlineNs && now_ns() > r->deadlineNs)
            r->stop = STOP_TIMED_OUT;
    }
    if (r->stop != STOP_INTERRUPTED && r->stop != STOP_TIMED_OUT) return;
    ampy_ticks = 1;
    mp_sched_keyboard_interrupt();
}

static AmpySource* find_source(Ampy* r, const char* name, size_t len) {
    for (int64_t i = 0; i < r->sourceLen; i++)
        if (strlen(r->sources[i].name) == len && memcmp(r->sources[i].name, name, len) == 0) return &r->sources[i];
    return NULL;
}

// Whether a module is defined under a name that starts with these characters
// and a dot: the name is a package.
static bool has_package(Ampy* r, const char* name, size_t len) {
    for (int64_t i = 0; i < r->sourceLen; i++) {
        const char* s = r->sources[i].name;
        if (strlen(s) > len && memcmp(s, name, len) == 0 && s[len] == '.') return true;
    }
    return false;
}

// The module a path of the import machinery names: "a/b.py" and
// "a/b/__init__.py" are the module defined as "a.b". Writes the dotted name
// to out and returns its length, or -1 when the path is neither.
static int64_t path_module(const char* path, char* out, size_t cap, bool* init) {
    size_t len = strlen(path);
    *init = false;
    if (len < 4 || strcmp(path + len - 3, ".py") != 0 || len - 3 >= cap) return -1;
    len -= 3;
    static const char INIT[] = "/__init__";
    size_t initLen = sizeof(INIT) - 1;
    if (len > initLen && memcmp(path + len - initLen, INIT, initLen) == 0) {
        len -= initLen;
        *init = true;
    }
    for (size_t i = 0; i < len; i++) out[i] = path[i] == '/' ? '.' : path[i];
    out[len] = 0;
    return (int64_t)len;
}

// What `import` finds: the modules the program defined.
mp_import_stat_t mp_import_stat(const char* path) {
    Ampy* r = current;
    if (!r) return MP_IMPORT_STAT_NO_EXIST;
    char name[512];
    bool init;
    int64_t n = path_module(path, name, sizeof(name), &init);
    if (n >= 0) {
        if (!find_source(r, name, (size_t)n)) return MP_IMPORT_STAT_NO_EXIST;
        // "a.py" is the module only when "a" is not a package, whose code is
        // its "__init__.py".
        if (!init && has_package(r, name, (size_t)n)) return MP_IMPORT_STAT_NO_EXIST;
        return MP_IMPORT_STAT_FILE;
    }
    size_t len = strlen(path);
    if (len == 0 || len >= sizeof(name)) return MP_IMPORT_STAT_NO_EXIST;
    for (size_t i = 0; i < len; i++) name[i] = path[i] == '/' ? '.' : path[i];
    return has_package(r, name, len) ? MP_IMPORT_STAT_DIR : MP_IMPORT_STAT_NO_EXIST;
}

mp_lexer_t* mp_lexer_new_from_file(qstr filename) {
    Ampy* r = current;
    char name[512];
    bool init;
    int64_t n = r ? path_module(qstr_str(filename), name, sizeof(name), &init) : -1;
    AmpySource* s = n >= 0 ? find_source(r, name, (size_t)n) : NULL;
    if (!s) mp_raise_OSError(MP_ENOENT);
    return mp_lexer_new_from_str_len(filename, s->source, s->len, 0);
}

// open(): scripts have no files.
static mp_obj_t script_open(size_t n_args, const mp_obj_t* args, mp_map_t* kwargs) {
    (void)n_args;
    (void)args;
    (void)kwargs;
    mp_raise_OSError(MP_EPERM);
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, script_open);

// --- The lock ----------------------------------------------------------------

static void free_slot(Ampy* r, int64_t h) {
    h &= SLOT_MASK;
    if (h <= 0 || h >= r->slotCap || r->slots[h] == MP_OBJ_NULL) return;
    r->slots[h] = MP_OBJ_NULL;
    r->freeSlots[r->freeLen++] = h;
    r->live--;
}

static void drain_released(Ampy* r) {
    pthread_mutex_lock(&r->queueMu);
    int64_t n = r->releasedLen;
    int64_t* list = r->released;
    int64_t cap = r->releasedCap;
    r->released = r->drained;
    r->releasedCap = r->drainedCap;
    r->releasedLen = 0;
    atomic_store(&r->hasReleased, false);
    pthread_mutex_unlock(&r->queueMu);
    r->drained = list;
    r->drainedCap = cap;
    if (r->open)
        for (int64_t i = 0; i < n; i++) free_slot(r, list[i]);
}

// Takes the lock. False when the runtime is closed; the lock is held either way.
static bool enter(Ampy* r) {
    // A runtime that could not be made is a closed one.
    if (!r) return false;
    void* me = self_id();
    if (atomic_load(&r->owner) == me) {
        r->depth++;
        return r->open;
    }
    void* expected = NULL;
    if (!atomic_compare_exchange_strong(&r->owner, &expected, me)) {
        pthread_mutex_lock(&r->mu);
        atomic_fetch_add(&r->waiters, 1);
        for (;;) {
            expected = NULL;
            if (atomic_compare_exchange_strong(&r->owner, &expected, me)) break;
            pthread_cond_wait(&r->cv, &r->mu);
        }
        atomic_fetch_sub(&r->waiters, 1);
        pthread_mutex_unlock(&r->mu);
    }
    r->depth = 1;
    if (!r->open) return false;
    if (atomic_load(&r->hasReleased)) drain_released(r);
    r->stop = STOP_NONE;
    r->deadlineNs = r->timeoutNs > 0 ? now_ns() + r->timeoutNs : 0;
    ampy_ticks = POLL_COUNT;
    MP_STATE_THREAD(mp_pending_exception) = MP_OBJ_NULL;
    return true;
}

static void leave(Ampy* r) {
    if (!r || --r->depth > 0) return;
    atomic_store(&r->owner, NULL);
    if (atomic_load(&r->waiters)) {
        pthread_mutex_lock(&r->mu);
        pthread_cond_broadcast(&r->cv);
        pthread_mutex_unlock(&r->mu);
    }
}

// --- Values ------------------------------------------------------------------

typedef struct {
    mp_obj_base_t base;
    int64_t id;
} AmpyForeign;

typedef struct {
    mp_obj_base_t base;
    int64_t fn;
} AmpyHostFunction;

static const mp_obj_type_t ampy_type_foreign;
static const mp_obj_type_t ampy_type_host;
static const mp_obj_type_t ampy_type_AdmError;

// An int as 64 bits. False when the value is not an int or does not fit.
static bool int_value(mp_obj_t o, int64_t* out) {
    if (mp_obj_is_small_int(o)) {
        *out = MP_OBJ_SMALL_INT_VALUE(o);
        return true;
    }
    if (!mp_obj_is_exact_type(o, &mp_type_int)) return false;
    // Two's complement, least significant byte first.
    byte b[8];
    if (!mpz_as_bytes(&((mp_obj_int_t*)MP_OBJ_TO_PTR(o))->mpz, false, true, sizeof(b), b)) return false;
    uint64_t v = 0;
    for (int i = 7; i >= 0; i--) v = (v << 8) | b[i];
    *out = (int64_t)v;
    return true;
}

static int kind_of(mp_obj_t o) {
    if (o == mp_const_none) return K_NONE;
    if (mp_obj_is_bool(o)) return K_BOOL;
    if (mp_obj_is_int(o)) return K_INT;
    if (mp_obj_is_float(o)) return K_FLOAT;
    if (mp_obj_is_str(o)) return K_STRING;
    if (!mp_obj_is_obj(o)) return K_OBJECT;
    const mp_obj_type_t* t = mp_obj_get_type(o);
    if (t == &mp_type_bytes || t == &mp_type_bytearray) return K_BYTES;
    if (t == &mp_type_list) return K_LIST;
    if (t == &mp_type_tuple) return K_TUPLE;
    if (t == &mp_type_dict || t == &mp_type_ordereddict) return K_DICT;
    if (t == &mp_type_module) return K_MODULE;
    if (t == &ampy_type_foreign) return K_FOREIGN;
    if (mp_obj_is_callable(o)) return K_FUNCTION;
    return K_OBJECT;
}

// Moves a value into a slot and returns it with its kind. Raises MemoryError
// when the table cannot grow: call it protected.
static int64_t put(Ampy* r, mp_obj_t o) {
    if (r->freeLen == 0) {
        int64_t cap = r->slotCap ? r->slotCap * 2 : 256;
        int64_t* grownFree = realloc(r->freeSlots, (size_t)cap * sizeof(int64_t));
        if (!grownFree) mp_raise_type(&mp_type_MemoryError);
        r->freeSlots = grownFree;
        mp_obj_t* grown = realloc(r->slots, (size_t)cap * sizeof(mp_obj_t));
        if (!grown) mp_raise_type(&mp_type_MemoryError);
        r->slots = grown;
        for (int64_t i = r->slotCap; i < cap; i++) r->slots[i] = MP_OBJ_NULL;
        // Slot 0 stays unused: 0 means failure.
        for (int64_t i = cap - 1; i >= (r->slotCap ? r->slotCap : 1); i--) r->freeSlots[r->freeLen++] = i;
        r->slotCap = cap;
    }
    int64_t h = r->freeSlots[--r->freeLen];
    r->slots[h] = o;
    r->live++;
    return h | ((int64_t)kind_of(o) << KIND_SHIFT);
}

// The value in slot h; None for a slot that holds nothing.
static mp_obj_t at(Ampy* r, int64_t h) {
    h &= SLOT_MASK;
    if (h <= 0 || h >= r->slotCap || r->slots[h] == MP_OBJ_NULL) return mp_const_none;
    return r->slots[h];
}

typedef struct {
    int64_t a, b, c, n, kw, out;
    double d;
    const char* s;
    const char* file;
    const void* p;
    const int64_t* list;
    int64_t* outList;
    AmpyBlob* blob;
    bool flag;
} Op;

// Puts an exception aside for ampyException.
static void set_exception(Ampy* r, mp_obj_t exc) {
    if (r->stop == STOP_NONE && mp_obj_is_subclass_fast(MP_OBJ_FROM_PTR(mp_obj_get_type(exc)), MP_OBJ_FROM_PTR(&mp_type_MemoryError)))
        r->stop = STOP_OUT_OF_MEMORY;
    r->failStop = r->stop;
    r->exception = exc;
    r->hasException = true;
}

// Runs f(r, op) protected. False after putting its exception aside.
static bool protect(Ampy* r, void (*f)(Ampy*, Op*), Op* op) {
    nlr_buf_t nlr;
    mp_obj_dict_t* globals = mp_globals_get();
    mp_obj_dict_t* locals = mp_locals_get();
    // Everything f calls lies below this frame: the collector scans up to it,
    // and the stack limit is measured from it.
    if (r->protectDepth++ == 0) mp_cstack_init_with_top(&nlr, r->stackSize);
    bool ok = false;
    if (nlr_push(&nlr) == 0) {
        f(r, op);
        nlr_pop();
        ok = true;
    } else {
        mp_globals_set(globals);
        mp_locals_set(locals);
        MP_STATE_THREAD(mp_pending_exception) = MP_OBJ_NULL;
        set_exception(r, MP_OBJ_FROM_PTR(nlr.ret_val));
    }
    r->protectDepth--;
    return ok;
}

// Bytes printed by MicroPython, collected outside its heap.
typedef struct {
    uint8_t* data;
    size_t len, cap;
    bool failed;
} ByteBuf;

static void buf_add(void* env, const char* p, size_t n) {
    ByteBuf* b = env;
    if (b->failed) return;
    if (b->len + n > b->cap) {
        size_t cap = b->cap ? b->cap * 2 : 256;
        while (cap < b->len + n) cap *= 2;
        uint8_t* grown = realloc(b->data, cap);
        if (!grown) {
            b->failed = true;
            return;
        }
        b->data = grown;
        b->cap = cap;
    }
    memcpy(b->data + b->len, p, n);
    b->len += n;
}

static void foreign_dead(Ampy* r, int64_t id) {
    if (!id) return;
    if (r->deadLen == r->deadCap) {
        int64_t cap = r->deadCap ? r->deadCap * 2 : 64;
        int64_t* grown = realloc(r->dead, (size_t)cap * sizeof(int64_t));
        if (!grown) return;
        r->dead = grown;
        r->deadCap = cap;
    }
    r->dead[r->deadLen++] = id;
}

static mp_obj_t foreign_del(mp_obj_t self_in) {
    AmpyForeign* self = MP_OBJ_TO_PTR(self_in);
    if (current) foreign_dead(current, self->id);
    self->id = 0;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(foreign_del_obj, foreign_del);

static const mp_rom_map_elem_t foreign_locals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&foreign_del_obj)},
};
static MP_DEFINE_CONST_DICT(foreign_locals, foreign_locals_table);

static MP_DEFINE_CONST_OBJ_TYPE(ampy_type_foreign, MP_QSTR_AdmObject, MP_TYPE_FLAG_NONE, locals_dict, &foreign_locals);

static mp_obj_t new_foreign(int64_t id) {
    AmpyForeign* o = mp_obj_malloc_with_finaliser(AmpyForeign, &ampy_type_foreign);
    o->id = id;
    return MP_OBJ_FROM_PTR(o);
}

// str() of an error from ADM is its message; its second argument, when it
// has one, is the foreign object that stands for the ADM error.
static void adm_error_print(const mp_print_t* print, mp_obj_t o_in, mp_print_kind_t kind) {
    mp_obj_exception_t* o = MP_OBJ_TO_PTR(o_in);
    if (kind == PRINT_STR && o->args && o->args->len > 0) {
        mp_obj_print_helper(print, o->args->items[0], PRINT_STR);
        return;
    }
    if (kind == PRINT_EXC && o->args && o->args->len > 0) {
        mp_print_str(print, "AdmError: ");
        mp_obj_print_helper(print, o->args->items[0], PRINT_STR);
        return;
    }
    mp_obj_exception_print(print, o_in, kind);
}

static MP_DEFINE_CONST_OBJ_TYPE(ampy_type_AdmError, MP_QSTR_AdmError, MP_TYPE_FLAG_NONE, make_new, mp_obj_exception_make_new, print, adm_error_print, attr, mp_obj_exception_attr, parent, &mp_type_Exception);

static mp_obj_t host_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t* args) {
    Ampy* r = current;
    AmpyHostFunction* self = MP_OBJ_TO_PTR(self_in);
    if (n_kw) mp_raise_TypeError(MP_ERROR_TEXT("an ADM function takes no keyword arguments"));
    if (atomic_load(&r->hasReleased)) drain_released(r);

    int64_t first[4] = {0, 0, 0, 0};
    // The collector owns the block, so an error below does not leak it.
    int64_t* rest = n_args > 4 ? m_new(int64_t, n_args - 4) : NULL;
    for (size_t i = 0; i < n_args; i++) {
        int64_t h = put(r, args[i]);
        if (i < 4)
            first[i] = h;
        else
            rest[i - 4] = h;
    }
    int64_t* outerArgs = r->args;
    int64_t outerCount = r->argCount;
    r->args = rest;
    r->argCount = n_args > 4 ? (int64_t)n_args - 4 : 0;

    r->hostDepth++;
    int64_t res = r->host(self->fn, (int64_t)n_args, first[0], first[1], first[2], first[3]);
    r->hostDepth--;

    r->args = outerArgs;
    r->argCount = outerCount;
    if (res == 0) {
        mp_obj_t thrown = r->thrown;
        r->thrown = MP_OBJ_NULL;
        if (thrown == MP_OBJ_NULL) mp_raise_type(&mp_type_RuntimeError);
        nlr_raise(thrown);
    }
    if (res < 0) return mp_const_none;
    return at(r, res);
}

static MP_DEFINE_CONST_OBJ_TYPE(ampy_type_host, MP_QSTR_function, MP_TYPE_FLAG_NONE, call, host_call);

// --- Lifecycle ---------------------------------------------------------------

// Whether the program already has an open runtime.
bool ampyBusy(void) {
    pthread_mutex_lock(&currentMu);
    bool busy = current != NULL;
    pthread_mutex_unlock(&currentMu);
    return busy;
}

static void op_setup(Ampy* r, Op* op) {
    (void)r;
    (void)op;
    // Scripts catch what ADM functions fail with as AdmError.
    mp_store_attr(MP_OBJ_FROM_PTR(&mp_module_builtins), MP_QSTR_AdmError, MP_OBJ_FROM_PTR(&ampy_type_AdmError));
}

static void free_areas(Ampy* r) {
    for (int64_t i = 0; i < r->areaLen; i++) free(r->areas[i].start);
    free(r->areas);
    r->areas = NULL;
    r->areaLen = r->areaCap = 0;
    r->heapBytes = 0;
}

// A new runtime, NULL when the program has one open or memory ran out.
// memoryLimit and stackSize are bytes, 0 for no limit and 1 MiB of stack.
Ampy* ampyNew(int64_t memoryLimit, int64_t stackSize, AmpyHost host) {
    Ampy* r = calloc(1, sizeof(Ampy));
    if (!r) return NULL;
    pthread_mutex_lock(&currentMu);
    if (current) {
        pthread_mutex_unlock(&currentMu);
        free(r);
        return NULL;
    }
    current = r;
    pthread_mutex_unlock(&currentMu);

    r->host = host;
    r->stackSize = stackSize > 0 ? (size_t)stackSize : DEFAULT_STACK;
    if (r->stackSize < 64 * 1024) r->stackSize = 64 * 1024;
    pthread_mutex_init(&r->mu, NULL);
    pthread_cond_init(&r->cv, NULL);
    pthread_mutex_init(&r->queueMu, NULL);

    void* first = ampy_heap_alloc(FIRST_AREA);
    if (!first) {
        pthread_mutex_lock(&currentMu);
        current = NULL;
        pthread_mutex_unlock(&currentMu);
        free(r->areas);
        free(r);
        return NULL;
    }
    volatile int here;
    mp_cstack_init_with_top((void*)&here, r->stackSize);
    gc_init(first, (uint8_t*)first + FIRST_AREA);
    mp_init();
    r->open = true;
    Op op = {0};
    protect(r, op_setup, &op);
    r->hasException = false;
    r->exception = MP_OBJ_NULL;
    // The limit starts after the runtime is set up.
    r->memoryLimit = memoryLimit > 0 ? r->heapBytes + (size_t)memoryLimit : 0;
    return r;
}

// Frees the Python side; the handle stays valid until ampyFree. False when
// called from inside a host function.
bool ampyClose(Ampy* r) {
    bool ok = true;
    if (enter(r)) {
        if (r->depth > 1 || r->protectDepth > 0) {
            ok = false;
        } else {
            r->open = false;
            mp_deinit();
            free_areas(r);
            r->live = 0;
            r->exception = r->thrown = MP_OBJ_NULL;
            for (int64_t i = 0; i < r->sourceLen; i++) {
                free(r->sources[i].name);
                free(r->sources[i].source);
            }
            free(r->sources);
            r->sources = NULL;
            r->sourceLen = r->sourceCap = 0;
            pthread_mutex_lock(&currentMu);
            current = NULL;
            pthread_mutex_unlock(&currentMu);
        }
    }
    leave(r);
    return ok;
}

// Frees the handle.
void ampyFree(Ampy* r) {
    if (!r) return;
    ampyClose(r);
    free(r->slots);
    free(r->freeSlots);
    free(r->released);
    free(r->drained);
    free(r->dead);
    pthread_mutex_destroy(&r->mu);
    pthread_cond_destroy(&r->cv);
    pthread_mutex_destroy(&r->queueMu);
    free(r);
}

// Gives a slot back, from any task: at once when the caller holds the lock or
// nobody does, else at the next entry.
void ampyRelease(Ampy* r, int64_t h) {
    if (!r) return;
    void* me = self_id();
    void* expected = NULL;
    if (atomic_load(&r->owner) == me) {
        if (r->hostDepth == 0) {
            if (r->open) free_slot(r, h);
            return;
        }
    } else if (atomic_compare_exchange_strong(&r->owner, &expected, me)) {
        r->depth = 1;
        if (r->open) free_slot(r, h);
        leave(r);
        return;
    }
    pthread_mutex_lock(&r->queueMu);
    if (r->releasedLen == r->releasedCap) {
        r->releasedCap = r->releasedCap ? r->releasedCap * 2 : 64;
        r->released = realloc(r->released, (size_t)r->releasedCap * sizeof(int64_t));
    }
    r->released[r->releasedLen++] = h;
    atomic_store(&r->hasReleased, true);
    pthread_mutex_unlock(&r->queueMu);
}

// Slots in use.
int64_t ampyLive(Ampy* r) {
    int64_t n = 0;
    if (enter(r)) n = r->live;
    leave(r);
    return n;
}

// Stops the running script, or the next one, from any task.
void ampyInterrupt(Ampy* r) {
    if (r) atomic_store(&r->interrupt, 1);
}

// Longest a call may run, in nanoseconds; 0 for no limit.
void ampyTimeout(Ampy* r, int64_t ns) {
    if (!r) return;
    enter(r);
    r->timeoutNs = ns;
    leave(r);
}

// Why the last failed call was stopped: 0 not stopped, 1 interrupted, 2 timed
// out, 3 out of memory.
int64_t ampyStopReason(Ampy* r) {
    if (!r) return STOP_NONE;
    enter(r);
    int64_t s = r->failStop;
    r->failStop = STOP_NONE;
    leave(r);
    return s;
}

// Bytes of the heap in use.
int64_t ampyMemory(Ampy* r) {
    int64_t n = 0;
    if (enter(r)) {
        gc_info_t info;
        gc_info(&info);
        n = (int64_t)info.used;
    }
    leave(r);
    return n;
}

static void op_collect(Ampy* r, Op* op) {
    (void)r;
    (void)op;
    gc_collect();
}

// Runs a full collection.
void ampyCollect(Ampy* r) {
    Op op = {0};
    if (enter(r)) protect(r, op_collect, &op);
    leave(r);
}

static void op_exception(Ampy* r, Op* op) { op->out = put(r, r->exception); }

// The exception the last failed call put aside, in a slot; 0 when there is none.
int64_t ampyException(Ampy* r) {
    Op op = {0};
    if (enter(r) && r->hasException) {
        r->hasException = false;
        if (!protect(r, op_exception, &op)) op.out = 0;
        r->exception = MP_OBJ_NULL;
        r->hasException = false;
    }
    leave(r);
    return op.out;
}

static void op_throw(Ampy* r, Op* op) {
    if (op->flag) {
        r->thrown = at(r, op->a);
        return;
    }
    mp_obj_t args[2];
    args[0] = mp_obj_new_str(op->s, strlen(op->s));
    size_t n = 1;
    if (op->b) args[n++] = new_foreign(op->b);
    r->thrown = mp_obj_new_exception_args(&ampy_type_AdmError, n, args);
}

// Sets the exception the host function in progress raises when it returns 0:
// an AdmError with this message, with a non-zero foreign id riding on it.
void ampyThrow(Ampy* r, const char* message, int64_t foreignId) {
    Op op = {.s = message, .b = foreignId};
    if (enter(r) && !protect(r, op_throw, &op)) {
        r->thrown = MP_OBJ_NULL;
        r->hasException = false;
    }
    leave(r);
}

// The same with a value, which must be an exception.
void ampyThrowValue(Ampy* r, int64_t h) {
    Op op = {.a = h, .flag = true};
    if (enter(r)) protect(r, op_throw, &op);
    leave(r);
}

// --- Making values -----------------------------------------------------------

enum { MAKE_NONE, MAKE_BOOL, MAKE_FLOAT, MAKE_INT, MAKE_STRING, MAKE_BYTES, MAKE_LIST, MAKE_DICT, MAKE_TUPLE, MAKE_GLOBALS, MAKE_FUNCTION, MAKE_FOREIGN };

static void op_make(Ampy* r, Op* op) {
    mp_obj_t o = mp_const_none;
    switch (op->c) {
    case MAKE_NONE:
        break;
    case MAKE_BOOL:
        o = mp_obj_new_bool(op->flag);
        break;
    case MAKE_FLOAT:
        o = mp_obj_new_float(op->d);
        break;
    case MAKE_INT:
        o = mp_obj_new_int_from_ll(op->a);
        break;
    case MAKE_STRING:
        o = mp_obj_new_str(op->p, (size_t)op->a);
        break;
    case MAKE_BYTES:
        o = mp_obj_new_bytes(op->p, (size_t)op->a);
        break;
    case MAKE_LIST:
        o = mp_obj_new_list(0, NULL);
        break;
    case MAKE_DICT:
        o = mp_obj_new_dict(0);
        break;
    case MAKE_TUPLE: {
        mp_obj_tuple_t* t = MP_OBJ_TO_PTR(mp_obj_new_tuple((size_t)op->n, NULL));
        for (int64_t i = 0; i < op->n; i++) t->items[i] = at(r, op->list[i]);
        o = MP_OBJ_FROM_PTR(t);
        break;
    }
    case MAKE_GLOBALS:
        o = MP_OBJ_FROM_PTR(&mp_module___main__);
        break;
    case MAKE_FUNCTION: {
        AmpyHostFunction* f = mp_obj_malloc(AmpyHostFunction, &ampy_type_host);
        f->fn = op->a;
        o = MP_OBJ_FROM_PTR(f);
        break;
    }
    case MAKE_FOREIGN:
        o = new_foreign(op->a);
        break;
    }
    op->out = put(r, o);
}

static int64_t make(Ampy* r, Op* op) {
    if (!enter(r) || !protect(r, op_make, op)) op->out = 0;
    leave(r);
    return op->out;
}

int64_t ampyNone(Ampy* r) { return make(r, &(Op){.c = MAKE_NONE}); }
int64_t ampyBool(Ampy* r, bool v) { return make(r, &(Op){.c = MAKE_BOOL, .flag = v}); }
int64_t ampyFloat(Ampy* r, double v) { return make(r, &(Op){.c = MAKE_FLOAT, .d = v}); }
int64_t ampyInt(Ampy* r, int64_t v) { return make(r, &(Op){.c = MAKE_INT, .a = v}); }
int64_t ampyString(Ampy* r, const char* text) { return make(r, &(Op){.c = MAKE_STRING, .p = text, .a = (int64_t)strlen(text)}); }
// A bytes object holding these bytes.
int64_t ampyBytesOf(Ampy* r, const uint8_t* data, int64_t len) { return make(r, &(Op){.c = MAKE_BYTES, .p = data, .a = len}); }
int64_t ampyList(Ampy* r) { return make(r, &(Op){.c = MAKE_LIST}); }
int64_t ampyDict(Ampy* r) { return make(r, &(Op){.c = MAKE_DICT}); }
// A tuple of these values.
int64_t ampyTuple(Ampy* r, const int64_t* items, int64_t n) { return make(r, &(Op){.c = MAKE_TUPLE, .list = items, .n = n}); }
// The module scripts run in, __main__.
int64_t ampyGlobals(Ampy* r) { return make(r, &(Op){.c = MAKE_GLOBALS}); }
// A function that calls host function `fn`.
int64_t ampyFunction(Ampy* r, int64_t fn) { return make(r, &(Op){.c = MAKE_FUNCTION, .a = fn}); }
// An opaque object standing for the ADM object kept under `id`.
int64_t ampyForeign(Ampy* r, int64_t id) { return make(r, &(Op){.c = MAKE_FOREIGN, .a = id}); }

// The id behind a foreign object, or behind the one an AdmError carries; 0
// for any other value.
int64_t ampyForeignId(Ampy* r, int64_t h) {
    int64_t id = 0;
    if (enter(r)) {
        mp_obj_t o = at(r, h);
        if (mp_obj_is_obj(o) && mp_obj_get_type(o) == &ampy_type_AdmError) {
            mp_obj_exception_t* e = MP_OBJ_TO_PTR(o);
            o = e->args && e->args->len > 1 ? e->args->items[1] : mp_const_none;
        }
        if (mp_obj_is_obj(o) && mp_obj_get_type(o) == &ampy_type_foreign) id = ((AmpyForeign*)MP_OBJ_TO_PTR(o))->id;
    }
    leave(r);
    return id;
}

// Moves up to cap ids of collected foreign objects into out.
int64_t ampyDead(Ampy* r, int64_t* out, int64_t cap) {
    int64_t n = 0;
    if (enter(r)) {
        while (n < cap && r->deadLen > 0) out[n++] = r->dead[--r->deadLen];
    }
    leave(r);
    return n;
}

// The arguments beyond the fourth of the host call in progress.
int64_t ampyArgs(Ampy* r, int64_t* out, int64_t cap) {
    int64_t n = 0;
    if (enter(r)) {
        n = r->argCount < cap ? r->argCount : cap;
        for (int64_t i = 0; i < n; i++) out[i] = r->args[i];
    }
    leave(r);
    return n;
}

// --- Reading values ----------------------------------------------------------

static void op_truth(Ampy* r, Op* op) { op->flag = mp_obj_is_true(at(r, op->a)); }

// Python's truth, which may run the value's __bool__ or __len__; false when
// that raised.
bool ampyToBool(Ampy* r, int64_t h) {
    Op op = {.a = h};
    if (!enter(r)) {
        op.flag = false;
    } else if (!protect(r, op_truth, &op)) {
        op.flag = false;
        r->hasException = false;
        r->exception = MP_OBJ_NULL;
    }
    leave(r);
    return op.flag;
}

// An int or a float as a float; 0 for anything else.
double ampyToFloat(Ampy* r, int64_t h) {
    mp_float_t v = 0;
    if (enter(r)) {
        mp_obj_t o = at(r, h);
        if (!mp_obj_is_bool(o) && !mp_obj_get_float_maybe(o, &v)) v = 0;
    }
    leave(r);
    return (double)v;
}

// Whether the value is an int that fits 64 bits.
bool ampyIntFits(Ampy* r, int64_t h) {
    bool fits = false;
    if (enter(r)) {
        int64_t v;
        fits = int_value(at(r, h), &v);
    }
    leave(r);
    return fits;
}

// An int that fits 64 bits; 0 for anything else.
int64_t ampyToInt(Ampy* r, int64_t h) {
    int64_t v = 0;
    if (enter(r) && !int_value(at(r, h), &v)) v = 0;
    leave(r);
    return v;
}

enum { TEXT_STR, TEXT_REPR, TEXT_ERROR, TEXT_TRACE, TEXT_BYTES };

static void op_text(Ampy* r, Op* op) {
    mp_obj_t o = at(r, op->a);
    if (op->c == TEXT_BYTES) {
        mp_buffer_info_t info;
        mp_get_buffer_raise(o, &info, MP_BUFFER_READ);
        op->blob = blob_new(info.buf, info.len);
        if (!op->blob) mp_raise_type(&mp_type_MemoryError);
        return;
    }
    // Printing runs the value's __str__ or __repr__, which may raise: the
    // text goes to a block of the collector's, copied out at the end.
    vstr_t vstr;
    mp_print_t print;
    vstr_init_print(&vstr, 64, &print);
    switch (op->c) {
    case TEXT_STR:
        mp_obj_print_helper(&print, o, PRINT_STR);
        break;
    case TEXT_REPR:
        mp_obj_print_helper(&print, o, PRINT_REPR);
        break;
    case TEXT_ERROR:
        mp_obj_print_helper(&print, o, mp_obj_is_exception_instance(o) ? PRINT_EXC : PRINT_STR);
        break;
    case TEXT_TRACE:
        if (mp_obj_is_exception_instance(o)) mp_obj_print_exception(&print, o);
        break;
    }
    op->blob = blob_new(vstr.buf, vstr.len);
    if (!op->blob) mp_raise_type(&mp_type_MemoryError);
}

static AmpyBlob* text(Ampy* r, int64_t h, int mode) {
    Op op = {.a = h, .c = mode};
    if (!enter(r) || !protect(r, op_text, &op)) op.blob = NULL;
    leave(r);
    return op.blob;
}

// str(value) in a blob; NULL when that raised.
AmpyBlob* ampyText(Ampy* r, int64_t h) { return text(r, h, TEXT_STR); }
// repr(value).
AmpyBlob* ampyRepr(Ampy* r, int64_t h) { return text(r, h, TEXT_REPR); }
// An exception as its last traceback line gives it: "ValueError: bad input".
AmpyBlob* ampyErrorText(Ampy* r, int64_t h) { return text(r, h, TEXT_ERROR); }
// An exception's traceback, as Python prints it; empty for any other value.
AmpyBlob* ampyTrace(Ampy* r, int64_t h) { return text(r, h, TEXT_TRACE); }
// The bytes of a str (UTF-8), bytes or bytearray; NULL for any other value.
AmpyBlob* ampyBytes(Ampy* r, int64_t h) { return text(r, h, TEXT_BYTES); }

// --- Attributes and items ----------------------------------------------------

enum {
    ITEM_ATTR, ITEM_SET_ATTR, ITEM_HAS_ATTR, ITEM_DEL_ATTR, ITEM_GET, ITEM_SET, ITEM_DEL, ITEM_CONTAINS, ITEM_LENGTH, ITEM_KEYS, ITEM_APPEND,
};

// The key of an item operation: a slot (op->flag), a string, or an index.
static mp_obj_t key_of(Ampy* r, Op* op) {
    if (op->flag) return at(r, op->b);
    if (op->s) return mp_obj_new_str(op->s, strlen(op->s));
    return mp_obj_new_int_from_ll(op->b);
}

static void op_item(Ampy* r, Op* op) {
    mp_obj_t o = at(r, op->a);
    switch (op->c) {
    case ITEM_ATTR:
        op->out = put(r, mp_load_attr(o, qstr_from_str(op->s)));
        break;
    case ITEM_SET_ATTR:
        mp_store_attr(o, qstr_from_str(op->s), at(r, op->b));
        break;
    case ITEM_DEL_ATTR:
        mp_store_attr(o, qstr_from_str(op->s), MP_OBJ_NULL);
        break;
    case ITEM_HAS_ATTR: {
        mp_obj_t dest[2];
        mp_load_method_maybe(o, qstr_from_str(op->s), dest);
        op->out = dest[0] != MP_OBJ_NULL;
        break;
    }
    case ITEM_GET:
        op->out = put(r, mp_obj_subscr(o, key_of(r, op), MP_OBJ_SENTINEL));
        break;
    case ITEM_SET:
        mp_obj_subscr(o, key_of(r, op), at(r, op->n));
        break;
    case ITEM_DEL:
        mp_obj_subscr(o, key_of(r, op), MP_OBJ_NULL);
        break;
    case ITEM_CONTAINS:
        op->out = mp_obj_is_true(mp_binary_op(MP_BINARY_OP_IN, key_of(r, op), o));
        break;
    case ITEM_LENGTH:
        op->out = mp_obj_get_int(mp_obj_len(o));
        break;
    case ITEM_APPEND:
        mp_obj_list_append(o, at(r, op->b));
        break;
    case ITEM_KEYS: {
        // Each string key as a 4-byte little-endian length and its bytes.
        // Nothing in the loop raises, so the C buffer cannot leak.
        if (kind_of(o) != K_DICT) mp_raise_TypeError(MP_ERROR_TEXT("keys() needs a dict"));
        mp_map_t* map = mp_obj_dict_get_map(o);
        ByteBuf buf = {0};
        for (size_t i = 0; i < map->alloc; i++) {
            if (!mp_map_slot_is_filled(map, i) || !mp_obj_is_str(map->table[i].key)) continue;
            size_t len = 0;
            const char* key = mp_obj_str_get_data(map->table[i].key, &len);
            uint8_t head[4] = {(uint8_t)len, (uint8_t)(len >> 8), (uint8_t)(len >> 16), (uint8_t)(len >> 24)};
            buf_add(&buf, (const char*)head, 4);
            buf_add(&buf, key, len);
        }
        op->blob = buf.failed ? NULL : blob_new(buf.data, buf.len);
        free(buf.data);
        if (!op->blob) mp_raise_type(&mp_type_MemoryError);
        break;
    }
    }
}

static bool item_op(Ampy* r, Op* op) {
    bool ok = enter(r) && protect(r, op_item, op);
    leave(r);
    return ok;
}

// value.name; 0 when that raised.
int64_t ampyAttr(Ampy* r, int64_t h, const char* name) {
    Op op = {.c = ITEM_ATTR, .a = h, .s = name};
    return item_op(r, &op) ? op.out : 0;
}

// value.name = v. False when the assignment raised.
bool ampySetAttr(Ampy* r, int64_t h, const char* name, int64_t v) {
    Op op = {.c = ITEM_SET_ATTR, .a = h, .s = name, .b = v};
    return item_op(r, &op);
}

// del value.name.
bool ampyDelAttr(Ampy* r, int64_t h, const char* name) {
    Op op = {.c = ITEM_DEL_ATTR, .a = h, .s = name};
    return item_op(r, &op);
}

// hasattr(value, name): 1, 0, or -1 when looking it up raised.
int64_t ampyHasAttr(Ampy* r, int64_t h, const char* name) {
    Op op = {.c = ITEM_HAS_ATTR, .a = h, .s = name};
    return item_op(r, &op) ? op.out : -1;
}

// value[key] with the key in a slot; 0 when that raised.
int64_t ampyItem(Ampy* r, int64_t h, int64_t key) {
    Op op = {.c = ITEM_GET, .a = h, .b = key, .flag = true};
    return item_op(r, &op) ? op.out : 0;
}

// value[name].
int64_t ampyKey(Ampy* r, int64_t h, const char* name) {
    Op op = {.c = ITEM_GET, .a = h, .s = name};
    return item_op(r, &op) ? op.out : 0;
}

// value[index].
int64_t ampyIndex(Ampy* r, int64_t h, int64_t index) {
    Op op = {.c = ITEM_GET, .a = h, .b = index};
    return item_op(r, &op) ? op.out : 0;
}

// value[key] = v. False when the assignment raised.
bool ampySetItem(Ampy* r, int64_t h, int64_t key, int64_t v) {
    Op op = {.c = ITEM_SET, .a = h, .b = key, .flag = true, .n = v};
    return item_op(r, &op);
}

// value[name] = v.
bool ampySetKey(Ampy* r, int64_t h, const char* name, int64_t v) {
    Op op = {.c = ITEM_SET, .a = h, .s = name, .n = v};
    return item_op(r, &op);
}

// value[index] = v.
bool ampySetIndex(Ampy* r, int64_t h, int64_t index, int64_t v) {
    Op op = {.c = ITEM_SET, .a = h, .b = index, .n = v};
    return item_op(r, &op);
}

// del value[key].
bool ampyDelItem(Ampy* r, int64_t h, int64_t key) {
    Op op = {.c = ITEM_DEL, .a = h, .b = key, .flag = true};
    return item_op(r, &op);
}

// del value[name].
bool ampyDelKey(Ampy* r, int64_t h, const char* name) {
    Op op = {.c = ITEM_DEL, .a = h, .s = name};
    return item_op(r, &op);
}

// key in value: 1, 0, or -1 when the test raised.
int64_t ampyContains(Ampy* r, int64_t h, int64_t key) {
    Op op = {.c = ITEM_CONTAINS, .a = h, .b = key, .flag = true};
    return item_op(r, &op) ? op.out : -1;
}

// name in value.
int64_t ampyHasKey(Ampy* r, int64_t h, const char* name) {
    Op op = {.c = ITEM_CONTAINS, .a = h, .s = name};
    return item_op(r, &op) ? op.out : -1;
}

// list.append(v). False when the value is not a list.
bool ampyAppend(Ampy* r, int64_t h, int64_t v) {
    Op op = {.c = ITEM_APPEND, .a = h, .b = v};
    if ((h >> KIND_SHIFT) != K_LIST) return false;
    return item_op(r, &op);
}

// len(value); -1 when that raised.
int64_t ampyLength(Ampy* r, int64_t h) {
    Op op = {.c = ITEM_LENGTH, .a = h};
    return item_op(r, &op) ? op.out : -1;
}

// A dict's string keys in a blob, in the dict's order; NULL when the value is
// not a dict.
AmpyBlob* ampyKeys(Ampy* r, int64_t h) {
    Op op = {.c = ITEM_KEYS, .a = h};
    return item_op(r, &op) ? op.blob : NULL;
}

static void op_numbers(Ampy* r, Op* op) {
    mp_obj_iter_buf_t iterBuf;
    mp_obj_t it = mp_getiter(at(r, op->a), &iterBuf);
    int64_t n = 0;
    mp_obj_t item;
    while (n < op->b && (item = mp_iternext(it)) != MP_OBJ_STOP_ITERATION) {
        if (op->outList) {
            int64_t v;
            mp_float_t f;
            if (mp_obj_is_bool(item)) {
                mp_raise_msg_varg(&mp_type_TypeError, MP_ERROR_TEXT("element %d is not an integer"), (int)n);
            } else if (mp_obj_is_int(item)) {
                if (!int_value(item, &v))
                    mp_raise_msg_varg(&mp_type_OverflowError, MP_ERROR_TEXT("element %d does not fit 64 bits"), (int)n);
            } else if (mp_obj_is_float(item) && (f = mp_obj_float_get(item), f >= -9223372036854775808.0 && f < 9223372036854775808.0 && f == (mp_float_t)(mp_int_t)f)) {
                v = (int64_t)f;
            } else {
                mp_raise_msg_varg(&mp_type_TypeError, MP_ERROR_TEXT("element %d is not an integer"), (int)n);
            }
            op->outList[n] = v;
        } else {
            mp_float_t f;
            if (mp_obj_is_bool(item) || !mp_obj_get_float_maybe(item, &f))
                mp_raise_msg_varg(&mp_type_TypeError, MP_ERROR_TEXT("element %d is not a number"), (int)n);
            ((double*)op->p)[n] = f;
        }
        n++;
    }
    op->out = n;
}

// Copies the elements of a sequence into out as floats, up to cap of them,
// and returns how many it copied; -1 when iterating raised or an element is
// not a number.
int64_t ampyFloats(Ampy* r, int64_t h, double* out, int64_t cap) {
    Op op = {.a = h, .b = cap, .p = out};
    if (!enter(r) || !protect(r, op_numbers, &op)) op.out = -1;
    leave(r);
    return op.out;
}

// The same for integers: each element an int that fits 64 bits or a float
// with no fraction.
int64_t ampyInts(Ampy* r, int64_t h, int64_t* out, int64_t cap) {
    Op op = {.a = h, .b = cap, .outList = out};
    if (!enter(r) || !protect(r, op_numbers, &op)) op.out = -1;
    leave(r);
    return op.out;
}

// Whether both are the same object, as `is` compares.
bool ampySame(Ampy* r, int64_t a, int64_t b) {
    bool same = false;
    if (enter(r)) same = at(r, a) == at(r, b);
    leave(r);
    return same;
}

static void op_equal(Ampy* r, Op* op) { op->flag = mp_obj_equal(at(r, op->a), at(r, op->b)); }

// a == b; false when comparing raised.
bool ampyEqual(Ampy* r, int64_t a, int64_t b) {
    Op op = {.a = a, .b = b};
    if (!enter(r)) {
        op.flag = false;
    } else if (!protect(r, op_equal, &op)) {
        op.flag = false;
        r->hasException = false;
        r->exception = MP_OBJ_NULL;
    }
    leave(r);
    return op.flag;
}

// --- Calls and code ----------------------------------------------------------

enum { RUN_CALL, RUN_INVOKE, RUN_EVAL, RUN_EXEC, RUN_IMPORT, RUN_MODULE, RUN_BYTECODE, RUN_COMPILE };

static mp_lexer_t* lexer_of(Op* op) {
    return mp_lexer_new_from_str_len(qstr_from_str(op->s), op->p, (size_t)op->a, 0);
}

// Imports the module `name` and returns it: a module the program defined, or
// one of MicroPython's own. A module that fails while it runs is forgotten,
// so a later import runs it again.
static mp_obj_t import_module(const char* name) {
    qstr q = qstr_from_str(name);
    mp_map_t* loaded = &MP_STATE_VM(mp_loaded_modules_dict).map;
    bool known = mp_map_lookup(loaded, MP_OBJ_NEW_QSTR(q), MP_MAP_LOOKUP) != NULL;
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        // A non-empty "from" list makes the import return the module itself,
        // not the package at the head of a dotted name.
        mp_obj_t module = mp_import_name(q, mp_const_true, MP_OBJ_NEW_SMALL_INT(0));
        nlr_pop();
        return module;
    }
    if (!known) mp_map_lookup(loaded, MP_OBJ_NEW_QSTR(q), MP_MAP_LOOKUP_REMOVE_IF_FOUND);
    nlr_jump(nlr.ret_val);
}

// The module registered as `name`, made on first use by running `code` as
// the file `file`. Scripts cannot import it unless its name is one `import`
// can spell.
static mp_obj_t embedded_module(const char* name, const char* file, const char* code, size_t len) {
    qstr q = qstr_from_str(name);
    mp_map_t* loaded = &MP_STATE_VM(mp_loaded_modules_dict).map;
    mp_map_elem_t* known = mp_map_lookup(loaded, MP_OBJ_NEW_QSTR(q), MP_MAP_LOOKUP);
    if (known) return known->value;
    mp_obj_t module = mp_obj_new_module(q);
    mp_obj_dict_t* globals = mp_obj_module_get_globals(module);
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_parse_compile_execute(mp_lexer_new_from_str_len(qstr_from_str(file), code, len, 0), MP_PARSE_FILE_INPUT, globals, globals);
        nlr_pop();
        return module;
    }
    mp_map_lookup(loaded, MP_OBJ_NEW_QSTR(q), MP_MAP_LOOKUP_REMOVE_IF_FOUND);
    nlr_jump(nlr.ret_val);
}

// Runs a script function or code. The result goes to op->out.
static void op_run(Ampy* r, Op* op) {
    mp_obj_dict_t* main = &MP_STATE_VM(dict_main);
    switch (op->c) {
    case RUN_CALL:
    case RUN_INVOKE: {
        // [function, self] then the arguments, as mp_call_method_n_kw takes them.
        size_t total = (size_t)(op->n + 2 * op->kw);
        mp_obj_t* args = m_new(mp_obj_t, total + 2);
        if (op->c == RUN_INVOKE) {
            mp_load_method(at(r, op->a), qstr_from_str(op->s), args);
        } else {
            args[0] = at(r, op->a);
            args[1] = MP_OBJ_NULL;
        }
        for (size_t i = 0; i < total; i++) args[2 + i] = at(r, op->list[i]);
        op->out = put(r, mp_call_method_n_kw((size_t)op->n, (size_t)op->kw, args));
        break;
    }
    case RUN_EVAL:
        op->out = put(r, mp_parse_compile_execute(lexer_of(op), MP_PARSE_EVAL_INPUT, main, main));
        break;
    case RUN_EXEC:
        mp_parse_compile_execute(lexer_of(op), MP_PARSE_FILE_INPUT, main, main);
        break;
    case RUN_IMPORT:
        op->out = put(r, import_module(op->s));
        break;
    case RUN_MODULE:
        op->out = put(r, embedded_module(op->s, op->file, op->p, (size_t)op->a));
        break;
    case RUN_BYTECODE: {
        mp_module_context_t* context = m_new_obj(mp_module_context_t);
        context->module.globals = main;
        mp_compiled_module_t compiled = {0};
        compiled.context = context;
        mp_raw_code_load_mem(op->p, (size_t)op->a, &compiled);
        mp_obj_t f = mp_make_function_from_proto_fun(compiled.rc, context, MP_OBJ_NULL);
        mp_obj_dict_t* globals = mp_globals_get();
        mp_obj_dict_t* locals = mp_locals_get();
        mp_globals_set(main);
        mp_locals_set(main);
        // protect() restores the dicts when the call raises.
        mp_call_function_0(f);
        mp_globals_set(globals);
        mp_locals_set(locals);
        break;
    }
    case RUN_COMPILE: {
        mp_lexer_t* lex = lexer_of(op);
        qstr name = lex->source_name;
        mp_parse_tree_t tree = mp_parse(lex, MP_PARSE_FILE_INPUT);
        mp_compiled_module_t compiled = {0};
        compiled.context = m_new_obj(mp_module_context_t);
        compiled.context->module.globals = mp_globals_get();
        mp_compile_to_raw_code(&tree, name, false, &compiled);
        vstr_t vstr;
        mp_print_t print;
        vstr_init_print(&vstr, 256, &print);
        mp_raw_code_save(&compiled, &print);
        op->blob = blob_new(vstr.buf, vstr.len);
        if (!op->blob) mp_raise_type(&mp_type_MemoryError);
        break;
    }
    }
}

static int64_t run(Ampy* r, Op* op) {
    if (!enter(r) || !protect(r, op_run, op)) op->out = 0;
    leave(r);
    return op->out;
}

// Calls fn with argc positional arguments and kw keyword arguments: args
// holds the positional values, then a name (a str) and a value for each
// keyword. Returns the result, 0 when the call raised.
int64_t ampyCall(Ampy* r, int64_t fn, const int64_t* args, int64_t argc, int64_t kw) {
    return run(r, &(Op){.c = RUN_CALL, .a = fn, .list = args, .n = argc, .kw = kw});
}

// obj.name(args...).
int64_t ampyInvoke(Ampy* r, int64_t obj, const char* name, const int64_t* args, int64_t argc, int64_t kw) {
    return run(r, &(Op){.c = RUN_INVOKE, .a = obj, .s = name, .list = args, .n = argc, .kw = kw});
}

// Evaluates an expression in __main__ and returns its value.
int64_t ampyEval(Ampy* r, const char* code, const char* name) {
    return run(r, &(Op){.c = RUN_EVAL, .p = code, .a = (int64_t)strlen(code), .s = name});
}

// Runs statements in __main__. False when they raised.
bool ampyExec(Ampy* r, const char* code, const char* name) {
    Op op = {.c = RUN_EXEC, .p = code, .a = (int64_t)strlen(code), .s = name};
    bool ok = enter(r) && protect(r, op_run, &op);
    leave(r);
    return ok;
}

// Makes a module's source available to import under name, without running it.
void ampyDefine(Ampy* r, const char* name, const char* code) {
    size_t len = strlen(code);
    if (enter(r)) {
        AmpySource* s = find_source(r, name, strlen(name));
        if (!s) {
            if (r->sourceLen == r->sourceCap) {
                r->sourceCap = r->sourceCap ? r->sourceCap * 2 : 8;
                r->sources = realloc(r->sources, (size_t)r->sourceCap * sizeof(AmpySource));
            }
            s = &r->sources[r->sourceLen++];
            s->name = strdup(name);
            s->source = NULL;
        }
        free(s->source);
        s->source = strdup(code);
        s->len = len;
    }
    leave(r);
}

// The module `name`, run on first use.
int64_t ampyImport(Ampy* r, const char* name) { return run(r, &(Op){.c = RUN_IMPORT, .s = name}); }

// The module registered as `name`: on first use `code` runs as the file
// `file` and its globals become the module.
int64_t ampyModule(Ampy* r, const char* name, const char* file, const char* code) {
    return run(r, &(Op){.c = RUN_MODULE, .s = name, .file = file, .p = code, .a = (int64_t)strlen(code)});
}

// Compiles source text to .mpy bytecode in a blob; NULL when it does not compile.
AmpyBlob* ampyCompile(Ampy* r, const char* code, const char* name) {
    Op op = {.c = RUN_COMPILE, .p = code, .a = (int64_t)strlen(code), .s = name};
    if (!enter(r) || !protect(r, op_run, &op)) op.blob = NULL;
    leave(r);
    return op.blob;
}

// Runs bytecode from ampyCompile in __main__. MicroPython checks the header
// of the bytes, not the code: anything else can crash the program.
bool ampyRun(Ampy* r, const uint8_t* code, int64_t len) {
    Op op = {.c = RUN_BYTECODE, .p = code, .a = len};
    bool ok = enter(r) && protect(r, op_run, &op);
    leave(r);
    return ok;
}
