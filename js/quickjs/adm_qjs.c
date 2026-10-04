// The C side of adm.interop.js: QuickJS-ng behind functions that take and
// return integers, since a JSValue is a struct passed by value.
//
// A JavaScript value held by ADM is a slot in its runtime's table. A function
// that returns one returns the slot number with the value's kind in the top
// byte, or 0 after putting the exception aside for aqjsException.
//
// QuickJS keeps no thread state, so a runtime needs only one caller at a
// time: every entry takes the runtime's lock, which belongs to an ADM task
// and counts, because a host function called from JavaScript calls back in.

// clock_gettime and strdup.
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "quickjs.h"

#if defined(__APPLE__)
#include <malloc/malloc.h>
#define aqjs_usable(p) malloc_size(p)
#elif defined(_WIN32)
#include <malloc.h>
#define aqjs_usable(p) _msize((void*)(p))
#else
#include <malloc.h>
#define aqjs_usable(p) malloc_usable_size((void*)(p))
#endif

// The running ADM task, NULL on a thread that runs none (ADM runtime).
extern void* adm_sched_current(void);

// Calls ADM host function `fn` with `argc` arguments, the first four in
// a0..a3 and the rest through aqjsArgs. Returns a slot to hand to JavaScript,
// -1 for undefined, or 0 after aqjsThrow.
typedef int64_t (*AqjsHost)(int64_t fn, int64_t argc, int64_t a0, int64_t a1, int64_t a2, int64_t a3);

enum {
    K_UNDEFINED, K_NULL, K_BOOL, K_NUMBER, K_BIGINT, K_STRING, K_SYMBOL, K_OBJECT,
    K_ARRAY, K_FUNCTION, K_ERROR, K_PROMISE, K_BYTES, K_DATE, K_FOREIGN,
};

#define KIND_SHIFT 56
#define SLOT_MASK ((INT64_C(1) << KIND_SHIFT) - 1)

enum { STOP_NONE, STOP_INTERRUPTED, STOP_TIMED_OUT, STOP_OUT_OF_MEMORY };

typedef struct {
    char* name;
    char* source;
    size_t len;
} AqjsSource;

typedef struct Aqjs {
    JSRuntime* rt;
    JSContext* ctx;
    AqjsHost host;
    JSClassID foreignClass;

    JSValue* slots;
    int64_t* freeSlots;
    int64_t slotCap, freeLen, live;

    JSValue exception;
    bool hasException;

    // Arguments of the host call in progress beyond the first four.
    int64_t* args;
    int64_t argCount;

    AqjsSource* sources;
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
    // Set when the queue holds something, so an entry looks without the mutex.
    atomic_bool hasReleased;
    // The list the last drain emptied, swapped back in by the next.
    int64_t* drained;
    int64_t drainedCap;

    // Ids of foreign objects whose JavaScript wrapper was collected.
    int64_t* dead;
    int64_t deadLen, deadCap;

    // Bytes the engine holds, counted by the allocator hooks below, which
    // refuse what would pass memoryLimit (0 for none).
    size_t allocated, memoryLimit;
    bool outOfMemory;

    atomic_int interrupt;
    int stop;
    int64_t timeoutNs, deadlineNs;
} Aqjs;

// A length-prefixed block handed to ADM, which copies it out with aqjsBlobTake.
typedef struct {
    int64_t len;
    uint8_t data[];
} AqjsBlob;

static bool charge(Aqjs* r, size_t more) {
    if (r->memoryLimit && r->allocated + more > r->memoryLimit) {
        r->outOfMemory = true;
        return false;
    }
    return true;
}

static void* hook_malloc(void* opaque, size_t size) {
    Aqjs* r = opaque;
    if (!charge(r, size)) return NULL;
    void* p = malloc(size);
    if (p) r->allocated += aqjs_usable(p);
    return p;
}

static void* hook_calloc(void* opaque, size_t count, size_t size) {
    Aqjs* r = opaque;
    if (size && count > SIZE_MAX / size) return NULL;
    if (!charge(r, count * size)) return NULL;
    void* p = calloc(count, size);
    if (p) r->allocated += aqjs_usable(p);
    return p;
}

static void hook_free(void* opaque, void* p) {
    Aqjs* r = opaque;
    if (!p) return;
    r->allocated -= aqjs_usable(p);
    free(p);
}

static void* hook_realloc(void* opaque, void* p, size_t size) {
    Aqjs* r = opaque;
    size_t old = p ? aqjs_usable(p) : 0;
    if (size > old && !charge(r, size - old)) return NULL;
    void* q = realloc(p, size);
    if (q) r->allocated += aqjs_usable(q) - old;
    return q;
}

static size_t hook_usable(const void* p) { return aqjs_usable(p); }

static const JSMallocFunctions hooks = {hook_calloc, hook_malloc, hook_free, hook_realloc, hook_usable};

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

static void free_slot_value(Aqjs* r, int64_t h) {
    if (h <= 0 || h >= r->slotCap) return;
    JS_FreeValue(r->ctx, r->slots[h]);
    r->slots[h] = JS_UNINITIALIZED;
    r->freeSlots[r->freeLen++] = h;
    r->live--;
}

static void drain_released(Aqjs* r) {
    // Swap the queue with the drained one, so neither side allocates again.
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
    if (r->ctx)
        for (int64_t i = 0; i < n; i++) free_slot_value(r, list[i]);
}

// Takes the lock. False when the runtime is closed; the lock is held either way.
static bool enter(Aqjs* r) {
    void* me = self_id();
    if (atomic_load(&r->owner) == me) {
        r->depth++;
        return r->ctx != NULL;
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
    if (!r->ctx) return false;
    // The stack limit is measured from here: tasks run on their own stacks.
    JS_UpdateStackTop(r->rt);
    if (atomic_load(&r->hasReleased)) drain_released(r);
    r->deadlineNs = r->timeoutNs > 0 ? now_ns() + r->timeoutNs : 0;
    return true;
}

static void leave(Aqjs* r) {
    if (--r->depth > 0) return;
    atomic_store(&r->owner, NULL);
    if (atomic_load(&r->waiters)) {
        pthread_mutex_lock(&r->mu);
        pthread_cond_broadcast(&r->cv);
        pthread_mutex_unlock(&r->mu);
    }
}

static int kind_of(Aqjs* r, JSValueConst v) {
    switch (JS_VALUE_GET_NORM_TAG(v)) {
    case JS_TAG_UNDEFINED:
    case JS_TAG_UNINITIALIZED:
        return K_UNDEFINED;
    case JS_TAG_NULL:
        return K_NULL;
    case JS_TAG_BOOL:
        return K_BOOL;
    case JS_TAG_INT:
    case JS_TAG_FLOAT64:
        return K_NUMBER;
    case JS_TAG_BIG_INT:
    case JS_TAG_SHORT_BIG_INT:
        return K_BIGINT;
    case JS_TAG_STRING:
    case JS_TAG_STRING_ROPE:
        return K_STRING;
    case JS_TAG_SYMBOL:
        return K_SYMBOL;
    case JS_TAG_OBJECT:
        break;
    default:
        return K_UNDEFINED;
    }
    if (JS_IsFunction(r->ctx, v)) return K_FUNCTION;
    if (JS_IsArray(v)) return K_ARRAY;
    if (JS_IsError(v)) return K_ERROR;
    if (JS_IsPromise(v)) return K_PROMISE;
    if (JS_IsArrayBuffer(v) || JS_GetTypedArrayType(v) >= 0 || JS_IsDataView(v)) return K_BYTES;
    if (JS_IsDate(v)) return K_DATE;
    if (JS_GetClassID(v) == r->foreignClass) return K_FOREIGN;
    return K_OBJECT;
}

static void set_exception(Aqjs* r) {
    if (r->outOfMemory) {
        r->outOfMemory = false;
        r->stop = STOP_OUT_OF_MEMORY;
    }
    if (r->hasException) JS_FreeValue(r->ctx, r->exception);
    r->exception = JS_GetException(r->ctx);
    r->hasException = true;
}

// Stores v (owned) in a slot and returns it with its kind; 0 for an exception.
static int64_t put(Aqjs* r, JSValue v) {
    if (JS_IsException(v)) {
        set_exception(r);
        return 0;
    }
    if (r->freeLen == 0) {
        int64_t cap = r->slotCap ? r->slotCap * 2 : 256;
        r->slots = realloc(r->slots, (size_t)cap * sizeof(JSValue));
        r->freeSlots = realloc(r->freeSlots, (size_t)cap * sizeof(int64_t));
        for (int64_t i = r->slotCap; i < cap; i++) r->slots[i] = JS_UNINITIALIZED;
        // Slot 0 stays unused: 0 means failure.
        for (int64_t i = cap - 1; i >= (r->slotCap ? r->slotCap : 1); i--) r->freeSlots[r->freeLen++] = i;
        r->slotCap = cap;
    }
    int64_t h = r->freeSlots[--r->freeLen];
    r->slots[h] = v;
    r->live++;
    return h | ((int64_t)kind_of(r, v) << KIND_SHIFT);
}

// The value in slot h, borrowed; undefined for a slot that holds nothing.
static JSValueConst at(Aqjs* r, int64_t h) {
    h &= SLOT_MASK;
    if (h <= 0 || h >= r->slotCap) return JS_UNDEFINED;
    JSValue v = r->slots[h];
    if (JS_VALUE_GET_TAG(v) == JS_TAG_UNINITIALIZED) return JS_UNDEFINED;
    return v;
}

static AqjsBlob* blob_new(const void* data, size_t len) {
    AqjsBlob* b = malloc(sizeof(AqjsBlob) + len);
    b->len = (int64_t)len;
    if (len) memcpy(b->data, data, len);
    return b;
}

// Bytes of a blob. ADM reads the length first, then copies and frees.
int64_t aqjsBlobLen(AqjsBlob* b) { return b ? b->len : 0; }

void aqjsBlobTake(AqjsBlob* b, uint8_t* out, int64_t cap) {
    if (!b) return;
    if (cap > b->len) cap = b->len;
    if (cap > 0) memcpy(out, b->data, (size_t)cap);
    free(b);
}

static int on_interrupt(JSRuntime* rt, void* opaque) {
    (void)rt;
    Aqjs* r = opaque;
    if (atomic_exchange(&r->interrupt, 0)) {
        r->stop = STOP_INTERRUPTED;
        return 1;
    }
    if (r->deadlineNs && now_ns() > r->deadlineNs) {
        r->stop = STOP_TIMED_OUT;
        return 1;
    }
    return 0;
}

static void foreign_finalizer(JSRuntime* rt, JSValueConst v) {
    Aqjs* r = JS_GetRuntimeOpaque(rt);
    int64_t id = (int64_t)(intptr_t)JS_GetOpaque(v, r->foreignClass);
    if (!id) return;
    if (r->deadLen == r->deadCap) {
        r->deadCap = r->deadCap ? r->deadCap * 2 : 64;
        r->dead = realloc(r->dead, (size_t)r->deadCap * sizeof(int64_t));
    }
    r->dead[r->deadLen++] = id;
}

static JSValue host_call(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int magic, JSValueConst* data) {
    (void)this_val;
    (void)magic;
    Aqjs* r = JS_GetContextOpaque(ctx);
    // Slots ADM dropped during this script are not waiting for it to end.
    if (atomic_load(&r->hasReleased)) drain_released(r);
    int64_t fn = 0;
    JS_ToInt64(ctx, &fn, data[0]);

    int64_t first[4] = {0, 0, 0, 0};
    int64_t* rest = NULL;
    for (int i = 0; i < argc && i < 4; i++) first[i] = put(r, JS_DupValue(ctx, argv[i]));
    if (argc > 4) {
        rest = malloc((size_t)(argc - 4) * sizeof(int64_t));
        for (int i = 4; i < argc; i++) rest[i - 4] = put(r, JS_DupValue(ctx, argv[i]));
    }
    int64_t* outerArgs = r->args;
    int64_t outerCount = r->argCount;
    r->args = rest;
    r->argCount = argc > 4 ? argc - 4 : 0;

    int64_t res = r->host(fn, argc, first[0], first[1], first[2], first[3]);

    r->args = outerArgs;
    r->argCount = outerCount;
    free(rest);
    if (res == 0) return JS_EXCEPTION;
    if (res < 0) return JS_UNDEFINED;
    return JS_DupValue(ctx, at(r, res));
}

static AqjsSource* find_source(Aqjs* r, const char* name) {
    for (int64_t i = 0; i < r->sourceLen; i++)
        if (strcmp(r->sources[i].name, name) == 0) return &r->sources[i];
    return NULL;
}

static JSModuleDef* load_module(JSContext* ctx, const char* name, void* opaque) {
    Aqjs* r = opaque;
    AqjsSource* s = find_source(r, name);
    if (!s) {
        JS_ThrowReferenceError(ctx, "module \"%s\" is not defined", name);
        return NULL;
    }
    JSValue fn = JS_Eval(ctx, s->source, s->len, name, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    if (JS_IsException(fn)) return NULL;
    JSModuleDef* m = JS_VALUE_GET_PTR(fn);
    JS_FreeValue(ctx, fn);
    return m;
}

// Runs promise jobs until none is left. -1 when a job threw.
static int64_t run_jobs(Aqjs* r) {
    int64_t n = 0;
    for (;;) {
        JSContext* ctx = NULL;
        int rc = JS_ExecutePendingJob(r->rt, &ctx);
        if (rc == 0) return n;
        if (rc < 0) {
            set_exception(r);
            return -1;
        }
        n++;
    }
}

// Evaluates a compiled module and returns its namespace in a slot.
static int64_t finish_module(Aqjs* r, JSValue fn) {
    JSModuleDef* m = JS_VALUE_GET_PTR(fn);
    JSValue done = JS_EvalFunction(r->ctx, fn);
    if (JS_IsException(done)) {
        set_exception(r);
        return 0;
    }
    if (JS_IsPromise(done)) {
        while (JS_PromiseState(r->ctx, done) == JS_PROMISE_PENDING) {
            int64_t ran = run_jobs(r);
            if (ran < 0) {
                JS_FreeValue(r->ctx, done);
                return 0;
            }
            if (ran == 0) break;
        }
        JSPromiseStateEnum state = JS_PromiseState(r->ctx, done);
        if (state == JS_PROMISE_REJECTED) {
            JS_Throw(r->ctx, JS_PromiseResult(r->ctx, done));
            JS_FreeValue(r->ctx, done);
            set_exception(r);
            return 0;
        }
        if (state == JS_PROMISE_PENDING) {
            JS_FreeValue(r->ctx, done);
            JS_ThrowInternalError(r->ctx, "the module waits on a promise nothing settles");
            set_exception(r);
            return 0;
        }
    }
    JS_FreeValue(r->ctx, done);
    return put(r, JS_GetModuleNamespace(r->ctx, m));
}

// --- Lifecycle ---------------------------------------------------------------

// A new runtime with one context. memoryLimit and stackSize are bytes, 0 for
// no memory limit and QuickJS's 1 MiB of stack.
Aqjs* aqjsNew(int64_t memoryLimit, int64_t stackSize, AqjsHost host) {
    Aqjs* r = calloc(1, sizeof(Aqjs));
    pthread_mutex_init(&r->mu, NULL);
    pthread_cond_init(&r->cv, NULL);
    pthread_mutex_init(&r->queueMu, NULL);
    r->host = host;
    r->memoryLimit = memoryLimit > 0 ? (size_t)memoryLimit : 0;
    r->rt = JS_NewRuntime2(&hooks, r);
    if (!r->rt) {
        free(r);
        return NULL;
    }
    JS_SetRuntimeOpaque(r->rt, r);
    if (stackSize > 0) JS_SetMaxStackSize(r->rt, (size_t)stackSize);
    JS_SetInterruptHandler(r->rt, on_interrupt, r);
    JS_SetModuleLoaderFunc(r->rt, NULL, load_module, r);

    r->foreignClass = 0;
    JS_NewClassID(r->rt, &r->foreignClass);
    JSClassDef def = {.class_name = "AdmObject", .finalizer = foreign_finalizer};
    JS_NewClass(r->rt, r->foreignClass, &def);

    r->ctx = JS_NewContext(r->rt);
    if (!r->ctx) {
        JS_FreeRuntime(r->rt);
        free(r);
        return NULL;
    }
    JS_SetContextOpaque(r->ctx, r);
    return r;
}

// Frees the JavaScript side. The handle stays valid (every call fails) until
// aqjsFree, so values released later have somewhere to go. False, and nothing
// freed, when called from inside a host function.
bool aqjsClose(Aqjs* r) {
    bool open = enter(r);
    if (r->depth > 1) {
        leave(r);
        return false;
    }
    if (open) {
        drain_released(r);
        if (r->hasException) JS_FreeValue(r->ctx, r->exception);
        r->hasException = false;
        for (int64_t h = 1; h < r->slotCap; h++)
            if (JS_VALUE_GET_TAG(r->slots[h]) != JS_TAG_UNINITIALIZED) JS_FreeValue(r->ctx, r->slots[h]);
        JS_FreeContext(r->ctx);
        JS_FreeRuntime(r->rt);
        r->ctx = NULL;
        r->rt = NULL;
        for (int64_t i = 0; i < r->sourceLen; i++) {
            free(r->sources[i].name);
            free(r->sources[i].source);
        }
        free(r->sources);
        free(r->slots);
        free(r->freeSlots);
        r->sources = NULL;
        r->slots = NULL;
        r->freeSlots = NULL;
        r->sourceLen = r->slotCap = r->freeLen = r->live = 0;
    }
    leave(r);
    return true;
}

// Frees the handle itself. Nothing may use it afterwards.
void aqjsFree(Aqjs* r) {
    aqjsClose(r);
    free(r->released);
    free(r->drained);
    free(r->dead);
    pthread_mutex_destroy(&r->mu);
    pthread_cond_destroy(&r->cv);
    pthread_mutex_destroy(&r->queueMu);
    free(r);
}

// Gives a slot back. Callable from any task: the slot is freed at the next entry.
void aqjsRelease(Aqjs* r, int64_t h) {
    h &= SLOT_MASK;
    if (h <= 0) return;
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
int64_t aqjsLive(Aqjs* r) {
    int64_t n = 0;
    if (enter(r)) n = r->live;
    leave(r);
    return n;
}

// Stops the script that is running, or the next one, from any task.
void aqjsInterrupt(Aqjs* r) { atomic_store(&r->interrupt, 1); }

// Longest a call from ADM may run, in nanoseconds; 0 for no limit.
void aqjsTimeout(Aqjs* r, int64_t ns) {
    enter(r);
    r->timeoutNs = ns;
    leave(r);
}

// Why the last script was stopped (STOP_*), cleared by the read.
int64_t aqjsStopReason(Aqjs* r) {
    enter(r);
    int64_t why = r->stop;
    r->stop = STOP_NONE;
    leave(r);
    return why;
}

// Bytes the runtime holds.
int64_t aqjsMemory(Aqjs* r) {
    int64_t n = 0;
    if (enter(r)) n = (int64_t)r->allocated;
    leave(r);
    return n;
}

// Runs the cycle collector.
void aqjsCollect(Aqjs* r) {
    if (enter(r)) JS_RunGC(r->rt);
    leave(r);
}

// --- Exceptions --------------------------------------------------------------

// The exception the last failed call put aside, in a slot; 0 when there is none.
int64_t aqjsException(Aqjs* r) {
    int64_t h = 0;
    if (enter(r) && r->hasException) {
        r->hasException = false;
        JSValue e = r->exception;
        if (JS_IsUncatchableError(e)) JS_ResetUncatchableError(r->ctx);
        h = put(r, e);
    }
    leave(r);
    return h;
}

// Throws an Error with this message from the host function in progress. A
// non-zero foreign id rides on it as the hidden property `admError`.
void aqjsThrow(Aqjs* r, const char* msg, int64_t foreignId) {
    if (enter(r)) {
        JSValue e = JS_NewError(r->ctx);
        JS_DefinePropertyValueStr(r->ctx, e, "message", JS_NewString(r->ctx, msg),
                                  JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE);
        if (foreignId) {
            JSValue box = JS_NewObjectClass(r->ctx, r->foreignClass);
            JS_SetOpaque(box, (void*)(intptr_t)foreignId);
            JS_DefinePropertyValueStr(r->ctx, e, "admError", box, 0);
        }
        JS_Throw(r->ctx, e);
    }
    leave(r);
}

// Throws the value in slot h from the host function in progress.
void aqjsThrowValue(Aqjs* r, int64_t h) {
    if (enter(r)) JS_Throw(r->ctx, JS_DupValue(r->ctx, at(r, h)));
    leave(r);
}

// --- Making values -----------------------------------------------------------

int64_t aqjsUndefined(Aqjs* r) {
    int64_t h = 0;
    if (enter(r)) h = put(r, JS_UNDEFINED);
    leave(r);
    return h;
}

int64_t aqjsNull(Aqjs* r) {
    int64_t h = 0;
    if (enter(r)) h = put(r, JS_NULL);
    leave(r);
    return h;
}

int64_t aqjsBool(Aqjs* r, bool v) {
    int64_t h = 0;
    if (enter(r)) h = put(r, JS_NewBool(r->ctx, v));
    leave(r);
    return h;
}

int64_t aqjsNumber(Aqjs* r, double v) {
    int64_t h = 0;
    if (enter(r)) h = put(r, JS_NewFloat64(r->ctx, v));
    leave(r);
    return h;
}

// An integer: a number when it is exact as one, else a BigInt.
int64_t aqjsInteger(Aqjs* r, int64_t v) {
    int64_t h = 0;
    if (enter(r)) {
        if (v >= -(INT64_C(1) << 53) && v <= (INT64_C(1) << 53))
            h = put(r, JS_NewInt64(r->ctx, v));
        else
            h = put(r, JS_NewBigInt64(r->ctx, v));
    }
    leave(r);
    return h;
}

int64_t aqjsBigInt(Aqjs* r, int64_t v) {
    int64_t h = 0;
    if (enter(r)) h = put(r, JS_NewBigInt64(r->ctx, v));
    leave(r);
    return h;
}

int64_t aqjsString(Aqjs* r, const char* text) {
    int64_t h = 0;
    if (enter(r)) h = put(r, JS_NewString(r->ctx, text));
    leave(r);
    return h;
}

int64_t aqjsObject(Aqjs* r) {
    int64_t h = 0;
    if (enter(r)) h = put(r, JS_NewObject(r->ctx));
    leave(r);
    return h;
}

int64_t aqjsArray(Aqjs* r) {
    int64_t h = 0;
    if (enter(r)) h = put(r, JS_NewArray(r->ctx));
    leave(r);
    return h;
}

int64_t aqjsGlobal(Aqjs* r) {
    int64_t h = 0;
    if (enter(r)) h = put(r, JS_GetGlobalObject(r->ctx));
    leave(r);
    return h;
}

int64_t aqjsDate(Aqjs* r, double epochMs) {
    int64_t h = 0;
    if (enter(r)) h = put(r, JS_NewDate(r->ctx, epochMs));
    leave(r);
    return h;
}

// A Uint8Array holding a copy of the bytes.
int64_t aqjsBytes(Aqjs* r, const uint8_t* data, int64_t len) {
    int64_t h = 0;
    if (enter(r)) {
        JSValue buf = JS_NewArrayBufferCopy(r->ctx, data, (size_t)len);
        if (JS_IsException(buf)) {
            set_exception(r);
        } else {
            h = put(r, JS_NewTypedArray(r->ctx, 1, &buf, JS_TYPED_ARRAY_UINT8));
            JS_FreeValue(r->ctx, buf);
        }
    }
    leave(r);
    return h;
}

// A Uint8Array over the caller's own bytes, with no copy. The caller keeps
// the bytes alive and calls aqjsDetach before they go.
int64_t aqjsView(Aqjs* r, uint8_t* data, int64_t len) {
    int64_t h = 0;
    if (enter(r)) {
        JSValue buf = JS_NewArrayBuffer(r->ctx, data, (size_t)len, 0, NULL, NULL, false);
        if (JS_IsException(buf)) {
            set_exception(r);
        } else {
            h = put(r, JS_NewTypedArray(r->ctx, 1, &buf, JS_TYPED_ARRAY_UINT8));
            JS_FreeValue(r->ctx, buf);
        }
    }
    leave(r);
    return h;
}

// Detaches the buffer behind a view: the script sees it empty from here on.
void aqjsDetach(Aqjs* r, int64_t h) {
    if (enter(r)) {
        JSValueConst v = at(r, h);
        if (JS_IsArrayBuffer(v)) {
            JS_DetachArrayBuffer(r->ctx, v);
        } else {
            JSValue buf = JS_GetTypedArrayBuffer(r->ctx, v, NULL, NULL, NULL);
            if (JS_IsException(buf)) {
                JS_FreeValue(r->ctx, JS_GetException(r->ctx));
            } else {
                JS_DetachArrayBuffer(r->ctx, buf);
                JS_FreeValue(r->ctx, buf);
            }
        }
    }
    leave(r);
}

// A function that calls host function `fn`.
int64_t aqjsFunction(Aqjs* r, int64_t fn, const char* name, int64_t arity) {
    int64_t h = 0;
    if (enter(r)) {
        JSValue data = JS_NewInt64(r->ctx, fn);
        JSValue f = JS_NewCFunctionData(r->ctx, host_call, (int)arity, 0, 1, &data);
        if (!JS_IsException(f) && name && name[0])
            JS_DefinePropertyValueStr(r->ctx, f, "name", JS_NewString(r->ctx, name), JS_PROP_CONFIGURABLE);
        h = put(r, f);
    }
    leave(r);
    return h;
}

// An opaque object standing for ADM object `id`. When the script drops it the
// id shows up in aqjsDead.
int64_t aqjsForeign(Aqjs* r, int64_t id) {
    int64_t h = 0;
    if (enter(r)) {
        JSValue box = JS_NewObjectClass(r->ctx, r->foreignClass);
        if (!JS_IsException(box)) JS_SetOpaque(box, (void*)(intptr_t)id);
        h = put(r, box);
    }
    leave(r);
    return h;
}

// The id behind a foreign object, 0 for anything else.
int64_t aqjsForeignId(Aqjs* r, int64_t h) {
    int64_t id = 0;
    if (enter(r)) id = (int64_t)(intptr_t)JS_GetOpaque(at(r, h), r->foreignClass);
    leave(r);
    return id;
}

// Moves up to cap ids of collected foreign objects into out.
int64_t aqjsDead(Aqjs* r, int64_t* out, int64_t cap) {
    int64_t n = 0;
    if (enter(r)) {
        n = r->deadLen < cap ? r->deadLen : cap;
        memcpy(out, r->dead + (r->deadLen - n), (size_t)n * sizeof(int64_t));
        r->deadLen -= n;
    }
    leave(r);
    return n;
}

// Arguments of the host call in progress from the fifth on.
int64_t aqjsArgs(Aqjs* r, int64_t* out, int64_t cap) {
    int64_t n = 0;
    if (enter(r)) {
        n = r->argCount < cap ? r->argCount : cap;
        if (n > 0) memcpy(out, r->args, (size_t)n * sizeof(int64_t));
    }
    leave(r);
    return n;
}

// --- Reading values ----------------------------------------------------------

bool aqjsToBool(Aqjs* r, int64_t h) {
    bool v = false;
    if (enter(r)) v = JS_ToBool(r->ctx, at(r, h)) > 0;
    leave(r);
    return v;
}

// The value as a number, NaN when it has none. Never runs script.
double aqjsToNumber(Aqjs* r, int64_t h) {
    double d = 0.0 / 0.0;
    if (enter(r)) {
        JSValueConst v = at(r, h);
        int tag = JS_VALUE_GET_NORM_TAG(v);
        if (tag == JS_TAG_INT)
            d = JS_VALUE_GET_INT(v);
        else if (tag == JS_TAG_FLOAT64)
            d = JS_VALUE_GET_FLOAT64(v);
        else if (tag == JS_TAG_BOOL)
            d = JS_VALUE_GET_BOOL(v);
        else if (JS_IsDate(v))
            JS_ToFloat64(r->ctx, &d, v);
    }
    leave(r);
    return d;
}

// A BigInt as 64 bits; aqjsBigIntFits says whether that lost anything.
int64_t aqjsToBigInt(Aqjs* r, int64_t h) {
    int64_t out = 0;
    if (enter(r) && JS_ToBigInt64(r->ctx, &out, at(r, h)) < 0) set_exception(r);
    leave(r);
    return out;
}

bool aqjsBigIntFits(Aqjs* r, int64_t h) {
    bool fits = false;
    if (enter(r)) {
        int64_t out = 0;
        if (JS_ToBigInt64(r->ctx, &out, at(r, h)) < 0) {
            set_exception(r);
        } else {
            JSValue back = JS_NewBigInt64(r->ctx, out);
            fits = JS_IsStrictEqual(r->ctx, back, at(r, h));
            JS_FreeValue(r->ctx, back);
        }
    }
    leave(r);
    return fits;
}

// The value as text (String(v)) in a blob; NULL when the conversion threw.
AqjsBlob* aqjsText(Aqjs* r, int64_t h) {
    AqjsBlob* b = NULL;
    if (enter(r)) {
        size_t len = 0;
        const char* s = JS_ToCStringLen(r->ctx, &len, at(r, h));
        if (!s) {
            set_exception(r);
        } else {
            b = blob_new(s, len);
            JS_FreeCString(r->ctx, s);
        }
    }
    leave(r);
    return b;
}

// JSON.stringify(v) in a blob; NULL when it threw or the value has no JSON form.
AqjsBlob* aqjsJson(Aqjs* r, int64_t h) {
    AqjsBlob* b = NULL;
    if (enter(r)) {
        JSValue text = JS_JSONStringify(r->ctx, at(r, h), JS_UNDEFINED, JS_UNDEFINED);
        if (JS_IsException(text)) {
            set_exception(r);
        } else if (JS_IsUndefined(text)) {
            JS_ThrowTypeError(r->ctx, "the value has no JSON form");
            set_exception(r);
        } else {
            size_t len = 0;
            const char* s = JS_ToCStringLen(r->ctx, &len, text);
            if (!s) {
                set_exception(r);
            } else {
                b = blob_new(s, len);
                JS_FreeCString(r->ctx, s);
            }
            JS_FreeValue(r->ctx, text);
        }
    }
    leave(r);
    return b;
}

// JSON.parse(text).
int64_t aqjsParseJson(Aqjs* r, const char* text) {
    int64_t h = 0;
    if (enter(r)) h = put(r, JS_ParseJSON(r->ctx, text, strlen(text), "<json>"));
    leave(r);
    return h;
}

static uint8_t* bytes_of(Aqjs* r, JSValueConst v, size_t* len) {
    if (JS_IsArrayBuffer(v)) return JS_GetArrayBuffer(r->ctx, len, v);
    size_t off = 0, n = 0, per = 0;
    JSValue buf = JS_GetTypedArrayBuffer(r->ctx, v, &off, &n, &per);
    if (JS_IsException(buf)) return NULL;
    size_t total = 0;
    uint8_t* base = JS_GetArrayBuffer(r->ctx, &total, buf);
    JS_FreeValue(r->ctx, buf);
    if (!base) return NULL;
    *len = n;
    return base + off;
}

// Byte length of an ArrayBuffer, typed array or DataView; -1 for anything else.
int64_t aqjsBytesLen(Aqjs* r, int64_t h) {
    int64_t n = -1;
    if (enter(r)) {
        size_t len = 0;
        if (bytes_of(r, at(r, h), &len))
            n = (int64_t)len;
        else if (JS_HasException(r->ctx))
            set_exception(r);
        else
            n = 0;
    }
    leave(r);
    return n;
}

// Copies up to cap of those bytes into out.
int64_t aqjsBytesCopy(Aqjs* r, int64_t h, uint8_t* out, int64_t cap) {
    int64_t n = -1;
    if (enter(r)) {
        size_t len = 0;
        uint8_t* p = bytes_of(r, at(r, h), &len);
        if (p) {
            n = (int64_t)len < cap ? (int64_t)len : cap;
            if (n > 0) memcpy(out, p, (size_t)n);
        } else if (JS_HasException(r->ctx)) {
            set_exception(r);
        } else {
            n = 0;
        }
    }
    leave(r);
    return n;
}

// --- Properties --------------------------------------------------------------

int64_t aqjsGet(Aqjs* r, int64_t h, const char* name) {
    int64_t out = 0;
    if (enter(r)) {
        JSAtom atom = JS_NewAtom(r->ctx, name);
        out = put(r, JS_GetProperty(r->ctx, at(r, h), atom));
        JS_FreeAtom(r->ctx, atom);
    }
    leave(r);
    return out;
}

int64_t aqjsGetIndex(Aqjs* r, int64_t h, int64_t index) {
    int64_t out = 0;
    if (enter(r)) out = put(r, JS_GetPropertyInt64(r->ctx, at(r, h), index));
    leave(r);
    return out;
}

bool aqjsSet(Aqjs* r, int64_t h, const char* name, int64_t value) {
    bool ok = false;
    if (enter(r)) {
        JSAtom atom = JS_NewAtom(r->ctx, name);
        ok = JS_SetProperty(r->ctx, at(r, h), atom, JS_DupValue(r->ctx, at(r, value))) >= 0;
        JS_FreeAtom(r->ctx, atom);
        if (!ok) set_exception(r);
    }
    leave(r);
    return ok;
}

bool aqjsSetIndex(Aqjs* r, int64_t h, int64_t index, int64_t value) {
    bool ok = false;
    if (enter(r)) {
        ok = JS_SetPropertyInt64(r->ctx, at(r, h), index, JS_DupValue(r->ctx, at(r, value))) >= 0;
        if (!ok) set_exception(r);
    }
    leave(r);
    return ok;
}

// 1 when the object has the property, 0 when not, -1 when the lookup threw.
int64_t aqjsHas(Aqjs* r, int64_t h, const char* name) {
    int64_t out = -1;
    if (enter(r)) {
        JSAtom atom = JS_NewAtom(r->ctx, name);
        out = JS_HasProperty(r->ctx, at(r, h), atom);
        JS_FreeAtom(r->ctx, atom);
        if (out < 0) set_exception(r);
    }
    leave(r);
    return out;
}

// 1 when the property is gone, 0 when it cannot be removed, -1 when it threw.
int64_t aqjsRemove(Aqjs* r, int64_t h, const char* name) {
    int64_t out = -1;
    if (enter(r)) {
        JSAtom atom = JS_NewAtom(r->ctx, name);
        out = JS_DeleteProperty(r->ctx, at(r, h), atom, 0);
        JS_FreeAtom(r->ctx, atom);
        if (out < 0) set_exception(r);
    }
    leave(r);
    return out;
}

// The `length` of an array or array-like; -1 when reading it threw.
int64_t aqjsLength(Aqjs* r, int64_t h) {
    int64_t n = -1;
    if (enter(r) && JS_GetLength(r->ctx, at(r, h), &n) < 0) {
        set_exception(r);
        n = -1;
    }
    leave(r);
    return n;
}

// The object's own enumerable string keys in a blob: each key as a 4-byte
// little-endian length and its bytes. NULL when listing them threw.
AqjsBlob* aqjsKeys(Aqjs* r, int64_t h) {
    AqjsBlob* b = NULL;
    if (enter(r)) {
        JSPropertyEnum* tab = NULL;
        uint32_t count = 0;
        if (JS_GetOwnPropertyNames(r->ctx, &tab, &count, at(r, h), JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) < 0) {
            set_exception(r);
        } else {
            size_t cap = 64, len = 0;
            uint8_t* buf = malloc(cap);
            for (uint32_t i = 0; i < count; i++) {
                size_t n = 0;
                const char* s = JS_AtomToCStringLen(r->ctx, &n, tab[i].atom);
                if (!s) continue;
                while (len + 4 + n > cap) {
                    cap *= 2;
                    buf = realloc(buf, cap);
                }
                buf[len] = (uint8_t)n;
                buf[len + 1] = (uint8_t)(n >> 8);
                buf[len + 2] = (uint8_t)(n >> 16);
                buf[len + 3] = (uint8_t)(n >> 24);
                memcpy(buf + len + 4, s, n);
                len += 4 + n;
                JS_FreeCString(r->ctx, s);
            }
            JS_FreePropertyEnum(r->ctx, tab, count);
            b = blob_new(buf, len);
            free(buf);
        }
    }
    leave(r);
    return b;
}

// --- Calls -------------------------------------------------------------------

static JSValue* arg_values(Aqjs* r, const int64_t* args, int64_t argc) {
    if (argc <= 0) return NULL;
    JSValue* out = malloc((size_t)argc * sizeof(JSValue));
    for (int64_t i = 0; i < argc; i++) out[i] = at(r, args[i]);
    return out;
}

// fn.call(self, ...args); self 0 for undefined.
int64_t aqjsCall(Aqjs* r, int64_t fn, int64_t self, const int64_t* args, int64_t argc) {
    int64_t out = 0;
    if (enter(r)) {
        JSValue* argv = arg_values(r, args, argc);
        out = put(r, JS_Call(r->ctx, at(r, fn), self ? at(r, self) : JS_UNDEFINED, (int)argc, argv));
        free(argv);
    }
    leave(r);
    return out;
}

// obj[name](...args).
int64_t aqjsInvoke(Aqjs* r, int64_t obj, const char* name, const int64_t* args, int64_t argc) {
    int64_t out = 0;
    if (enter(r)) {
        JSValue* argv = arg_values(r, args, argc);
        JSAtom atom = JS_NewAtom(r->ctx, name);
        out = put(r, JS_Invoke(r->ctx, at(r, obj), atom, (int)argc, argv));
        JS_FreeAtom(r->ctx, atom);
        free(argv);
    }
    leave(r);
    return out;
}

// new fn(...args).
int64_t aqjsConstruct(Aqjs* r, int64_t fn, const int64_t* args, int64_t argc) {
    int64_t out = 0;
    if (enter(r)) {
        JSValue* argv = arg_values(r, args, argc);
        out = put(r, JS_CallConstructor(r->ctx, at(r, fn), (int)argc, argv));
        free(argv);
    }
    leave(r);
    return out;
}

// --- Scripts, modules, jobs --------------------------------------------------

// Runs a script in the global scope and returns its completion value.
int64_t aqjsEval(Aqjs* r, const char* code, const char* name) {
    int64_t out = 0;
    if (enter(r)) out = put(r, JS_Eval(r->ctx, code, strlen(code), name, JS_EVAL_TYPE_GLOBAL));
    leave(r);
    return out;
}

// Makes a module's source available to `import` under this name.
void aqjsDefine(Aqjs* r, const char* name, const char* code) {
    if (enter(r)) {
        AqjsSource* s = find_source(r, name);
        if (!s) {
            if (r->sourceLen == r->sourceCap) {
                r->sourceCap = r->sourceCap ? r->sourceCap * 2 : 8;
                r->sources = realloc(r->sources, (size_t)r->sourceCap * sizeof(AqjsSource));
            }
            s = &r->sources[r->sourceLen++];
            s->name = strdup(name);
            s->source = NULL;
        }
        free(s->source);
        s->source = strdup(code);
        s->len = strlen(code);
    }
    leave(r);
}

// Evaluates the module defined under this name and returns its namespace.
int64_t aqjsImport(Aqjs* r, const char* name) {
    int64_t out = 0;
    if (enter(r)) {
        AqjsSource* s = find_source(r, name);
        if (!s) {
            JS_ThrowReferenceError(r->ctx, "module \"%s\" is not defined", name);
            set_exception(r);
        } else {
            JSValue fn = JS_Eval(r->ctx, s->source, s->len, name, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
            if (JS_IsException(fn))
                set_exception(r);
            else
                out = finish_module(r, fn);
        }
    }
    leave(r);
    return out;
}

// Compiles a script or module to bytecode in a blob; NULL when it does not compile.
AqjsBlob* aqjsCompile(Aqjs* r, const char* code, const char* name, bool module, bool strip) {
    AqjsBlob* b = NULL;
    if (enter(r)) {
        int flags = (module ? JS_EVAL_TYPE_MODULE : JS_EVAL_TYPE_GLOBAL) | JS_EVAL_FLAG_COMPILE_ONLY;
        JSValue fn = JS_Eval(r->ctx, code, strlen(code), name, flags);
        if (JS_IsException(fn)) {
            set_exception(r);
        } else {
            size_t size = 0;
            int wflags = JS_WRITE_OBJ_BYTECODE | (strip ? JS_WRITE_OBJ_STRIP_SOURCE | JS_WRITE_OBJ_STRIP_DEBUG : 0);
            uint8_t* buf = JS_WriteObject(r->ctx, &size, fn, wflags);
            JS_FreeValue(r->ctx, fn);
            if (!buf) {
                set_exception(r);
            } else {
                b = blob_new(buf, size);
                js_free(r->ctx, buf);
            }
        }
    }
    leave(r);
    return b;
}

// Runs bytecode aqjsCompile made: a script's completion value, or a module's
// namespace. The bytes are trusted: QuickJS does not verify bytecode.
int64_t aqjsRun(Aqjs* r, const uint8_t* code, int64_t len) {
    int64_t out = 0;
    if (enter(r)) {
        JSValue fn = JS_ReadObject(r->ctx, code, (size_t)len, JS_READ_OBJ_BYTECODE);
        if (JS_IsException(fn)) {
            set_exception(r);
        } else if (JS_VALUE_GET_TAG(fn) == JS_TAG_MODULE) {
            if (JS_ResolveModule(r->ctx, fn) < 0) {
                JS_FreeValue(r->ctx, fn);
                set_exception(r);
            } else {
                out = finish_module(r, fn);
            }
        } else {
            out = put(r, JS_EvalFunction(r->ctx, fn));
        }
    }
    leave(r);
    return out;
}

// Runs pending promise jobs; the number run, or -1 when one threw.
int64_t aqjsRunJobs(Aqjs* r) {
    int64_t n = 0;
    if (enter(r)) n = run_jobs(r);
    leave(r);
    return n;
}

// 0 pending, 1 fulfilled, 2 rejected, -1 not a promise.
int64_t aqjsPromiseState(Aqjs* r, int64_t h) {
    int64_t s = -1;
    if (enter(r)) s = JS_PromiseState(r->ctx, at(r, h));
    leave(r);
    return s;
}

// A settled promise's value or reason.
int64_t aqjsPromiseResult(Aqjs* r, int64_t h) {
    int64_t out = 0;
    if (enter(r)) out = put(r, JS_PromiseResult(r->ctx, at(r, h)));
    leave(r);
    return out;
}

// Puts a rejected promise's reason aside as the exception of this call.
void aqjsPromiseFail(Aqjs* r, int64_t h) {
    if (enter(r)) {
        if (r->hasException) JS_FreeValue(r->ctx, r->exception);
        r->exception = JS_PromiseResult(r->ctx, at(r, h));
        r->hasException = true;
    }
    leave(r);
}

// a === b.
bool aqjsSame(Aqjs* r, int64_t a, int64_t b) {
    bool same = false;
    if (enter(r)) same = JS_IsStrictEqual(r->ctx, at(r, a), at(r, b));
    leave(r);
    return same;
}
