// The C side of adm.interop.lua: Lua 5.4 behind functions that take and return
// integers.
//
// A Lua value held by ADM is a slot in its runtime's table (a Lua table in the
// registry). A function that returns one returns the slot number with the
// value's kind in the top byte, or 0 after putting the error aside for
// aluaException.
//
// Lua reports errors with longjmp, which must never cross an ADM frame: every
// entry runs its Lua calls inside lua_pcall, and a host function's error is
// raised only after the ADM function has returned.
//
// A Lua state has one caller at a time: every entry takes the runtime's lock,
// which belongs to an ADM task and counts, because a host function called
// from Lua calls back in.

// clock_gettime and strdup.
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

// The running ADM task, NULL on a thread that runs none (ADM runtime).
extern void* adm_sched_current(void);

// Calls ADM host function `fn` with `argc` arguments, the first four in
// a0..a3 and the rest through aluaArgs. Returns a slot to hand to Lua, -1 for
// no result, or 0 after aluaThrow.
typedef int64_t (*AluaHost)(int64_t fn, int64_t argc, int64_t a0, int64_t a1, int64_t a2, int64_t a3);

enum {
    K_NIL, K_BOOL, K_INTEGER, K_FLOAT, K_STRING, K_TABLE, K_FUNCTION, K_USERDATA, K_THREAD, K_FOREIGN,
};

#define KIND_SHIFT 56
#define SLOT_MASK ((INT64_C(1) << KIND_SHIFT) - 1)

enum { STOP_NONE, STOP_INTERRUPTED, STOP_TIMED_OUT, STOP_OUT_OF_MEMORY };

// Instructions between two looks at the interrupt flag and the deadline.
#define HOOK_COUNT 1000

#define FOREIGN_TYPE "adm object"

typedef struct {
    char* name;
    char* source;
    size_t len;
} AluaSource;

typedef struct Alua {
    lua_State* L;
    AluaHost host;

    // Free slot numbers; the values live in the registry table at SLOTS.
    int64_t* freeSlots;
    int64_t slotCap, freeLen, live;

    bool hasException;
    int failStop;

    // Arguments of the host call in progress beyond the first four.
    int64_t* args;
    int64_t argCount;
    // Host calls in progress. While one runs, slots its task releases wait in
    // the queue: the slot it returns is released before C reads it.
    int64_t hostDepth;

    AluaSource* sources;
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

    // Ids of foreign objects whose userdata was collected.
    int64_t* dead;
    int64_t deadLen, deadCap;

    // Bytes the state holds, counted by the allocator, which refuses what
    // would pass memoryLimit (0 for none).
    size_t allocated, memoryLimit;
    bool outOfMemory;

    atomic_int interrupt;
    int stop;
    int64_t timeoutNs, deadlineNs;
} Alua;

// A length-prefixed block handed to ADM, which copies it out with aluaBlobTake.
typedef struct {
    int64_t len;
    uint8_t data[];
} AluaBlob;

// Registry keys (their addresses).
static const char SLOTS = 0, EXCEPTION = 0, TRACE = 0, THROWN = 0, LOADED = 0;

static AluaBlob* blob_new(const void* data, size_t len) {
    AluaBlob* b = malloc(sizeof(AluaBlob) + len + 1);
    if (!b) return NULL;
    b->len = (int64_t)len;
    if (len) memcpy(b->data, data, len);
    b->data[len] = 0;
    return b;
}

// Bytes collected outside Lua's heap.
typedef struct {
    uint8_t* data;
    size_t len, cap;
    bool failed;
} ByteBuf;

static void buf_add(ByteBuf* b, const void* p, size_t n) {
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

int64_t aluaBlobLen(AluaBlob* b) { return b ? b->len : 0; }

void aluaBlobTake(AluaBlob* b, uint8_t* out, int64_t cap) {
    if (!b) return;
    int64_t n = b->len < cap ? b->len : cap;
    if (n > 0) memcpy(out, b->data, (size_t)n);
    free(b);
}

static void* alloc_hook(void* ud, void* ptr, size_t osize, size_t nsize) {
    Alua* r = ud;
    if (!ptr) osize = 0;
    if (nsize == 0) {
        r->allocated -= osize;
        free(ptr);
        return NULL;
    }
    if (nsize > osize && r->memoryLimit && r->allocated + (nsize - osize) > r->memoryLimit) {
        r->outOfMemory = true;
        return NULL;
    }
    void* q = realloc(ptr, nsize);
    if (q) r->allocated += nsize - osize;
    return q;
}

static int on_panic(lua_State* L) {
    fprintf(stderr, "adm.interop.lua: unprotected Lua error: %s\n", lua_tostring(L, -1));
    abort();
    return 0;
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

static Alua* runtime_of(lua_State* L) { return *(Alua**)lua_getextraspace(L); }

// Stops the script when it was interrupted or ran past its deadline. The
// reason stays set until the outermost entry returns, and from then on the
// hook runs before every instruction of the thread it stopped: a script that
// catches the error with pcall is stopped again at its next instruction,
// wherever that is.
static void on_count(lua_State* L, lua_Debug* ar) {
    (void)ar;
    Alua* r = runtime_of(L);
    if (r->stop == STOP_NONE) {
        if (atomic_exchange(&r->interrupt, 0))
            r->stop = STOP_INTERRUPTED;
        else if (r->deadlineNs && now_ns() > r->deadlineNs)
            r->stop = STOP_TIMED_OUT;
    }
    if (r->stop != STOP_INTERRUPTED && r->stop != STOP_TIMED_OUT) {
        // A thread stopped by an earlier call goes back to the normal pace.
        if (lua_gethookcount(L) != HOOK_COUNT) lua_sethook(L, on_count, LUA_MASKCOUNT, HOOK_COUNT);
        return;
    }
    if (lua_gethookcount(L) != 1) lua_sethook(L, on_count, LUA_MASKCOUNT, 1);
    luaL_error(L, r->stop == STOP_INTERRUPTED ? "interrupted" : "timed out");
}

static void free_slot(Alua* r, int64_t h) {
    h &= SLOT_MASK;
    if (h <= 0 || h >= r->slotCap) return;
    lua_State* L = r->L;
    // Clearing a slot that holds a value never allocates.
    lua_rawgetp(L, LUA_REGISTRYINDEX, &SLOTS);
    lua_pushnil(L);
    lua_rawseti(L, -2, h);
    lua_pop(L, 1);
    r->freeSlots[r->freeLen++] = h;
    r->live--;
}

static void drain_released(Alua* r) {
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
    if (r->L)
        for (int64_t i = 0; i < n; i++) free_slot(r, list[i]);
}

// Takes the lock. False when the runtime is closed; the lock is held either way.
static bool enter(Alua* r) {
    void* me = self_id();
    if (atomic_load(&r->owner) == me) {
        r->depth++;
        return r->L != NULL;
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
    if (!r->L) return false;
    if (atomic_load(&r->hasReleased)) drain_released(r);
    r->stop = STOP_NONE;
    r->deadlineNs = r->timeoutNs > 0 ? now_ns() + r->timeoutNs : 0;
    return true;
}

static void leave(Alua* r) {
    if (--r->depth > 0) return;
    atomic_store(&r->owner, NULL);
    if (atomic_load(&r->waiters)) {
        pthread_mutex_lock(&r->mu);
        pthread_cond_broadcast(&r->cv);
        pthread_mutex_unlock(&r->mu);
    }
}

static int kind_at(lua_State* L, int idx) {
    switch (lua_type(L, idx)) {
    case LUA_TBOOLEAN:
        return K_BOOL;
    case LUA_TNUMBER:
        return lua_isinteger(L, idx) ? K_INTEGER : K_FLOAT;
    case LUA_TSTRING:
        return K_STRING;
    case LUA_TTABLE:
        return K_TABLE;
    case LUA_TFUNCTION:
        return K_FUNCTION;
    case LUA_TTHREAD:
        return K_THREAD;
    case LUA_TUSERDATA:
        return luaL_testudata(L, idx, FOREIGN_TYPE) ? K_FOREIGN : K_USERDATA;
    case LUA_TLIGHTUSERDATA:
        return K_USERDATA;
    default:
        return K_NIL;
    }
}

// Moves the value on top of the stack into a slot and returns it with its
// kind. Raises a Lua error when memory runs out: call it protected.
static int64_t put(lua_State* L) {
    Alua* r = runtime_of(L);
    if (r->freeLen == 0) {
        int64_t cap = r->slotCap ? r->slotCap * 2 : 256;
        int64_t* grown = realloc(r->freeSlots, (size_t)cap * sizeof(int64_t));
        if (!grown) luaL_error(L, "not enough memory");
        r->freeSlots = grown;
        // Slot 0 stays unused: 0 means failure.
        for (int64_t i = cap - 1; i >= (r->slotCap ? r->slotCap : 1); i--) r->freeSlots[r->freeLen++] = i;
        r->slotCap = cap;
    }
    int64_t h = r->freeSlots[r->freeLen - 1];
    int64_t kind = kind_at(L, -1);
    lua_rawgetp(L, LUA_REGISTRYINDEX, &SLOTS);
    lua_insert(L, -2);
    lua_rawseti(L, -2, h);
    lua_pop(L, 1);
    r->freeLen--;
    r->live++;
    return h | (kind << KIND_SHIFT);
}

// Pushes the value in slot h; nil for a slot that holds nothing.
static void push_slot(lua_State* L, int64_t h) {
    lua_rawgetp(L, LUA_REGISTRYINDEX, &SLOTS);
    lua_rawgeti(L, -1, h & SLOT_MASK);
    lua_remove(L, -2);
}

// Puts the error on top of the stack aside for aluaException and pops it.
static void set_exception(Alua* r, int status) {
    lua_State* L = r->L;
    if (status == LUA_ERRMEM || r->outOfMemory) {
        r->outOfMemory = false;
        if (r->stop == STOP_NONE) r->stop = STOP_OUT_OF_MEMORY;
    }
    r->failStop = r->stop;
    // The key exists since aluaNew, so this store does not allocate.
    lua_rawsetp(L, LUA_REGISTRYINDEX, &EXCEPTION);
    r->hasException = true;
}

// Runs f(ud) protected. False after putting its error aside.
static bool protect(Alua* r, lua_CFunction f, void* ud) {
    lua_State* L = r->L;
    if (!lua_checkstack(L, LUA_MINSTACK)) {
        r->stop = STOP_OUT_OF_MEMORY;
        r->failStop = r->stop;
        return false;
    }
    lua_pushcfunction(L, f);
    lua_pushlightuserdata(L, ud);
    int status = lua_pcall(L, 1, 0, 0);
    if (status == LUA_OK) return true;
    set_exception(r, status);
    return false;
}

// Keeps the traceback of an error raised by a script, for aluaTrace, and
// passes the error on unchanged.
static int on_error(lua_State* L) {
    luaL_traceback(L, L, NULL, 1);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &TRACE);
    return 1;
}

static int foreign_gc(lua_State* L) {
    Alua* r = runtime_of(L);
    int64_t* id = lua_touserdata(L, 1);
    if (!id || !*id) return 0;
    if (r->deadLen == r->deadCap) {
        int64_t cap = r->deadCap ? r->deadCap * 2 : 64;
        int64_t* grown = realloc(r->dead, (size_t)cap * sizeof(int64_t));
        if (!grown) return 0;
        r->dead = grown;
        r->deadCap = cap;
    }
    r->dead[r->deadLen++] = *id;
    return 0;
}

static int host_call(lua_State* L) {
    Alua* r = runtime_of(L);
    if (atomic_load(&r->hasReleased)) drain_released(r);
    int64_t fn = lua_tointeger(L, lua_upvalueindex(1));
    int argc = lua_gettop(L);

    int64_t first[4] = {0, 0, 0, 0};
    int64_t* rest = NULL;
    if (argc > 4) {
        // Lua owns the block, so an error below does not leak it.
        rest = lua_newuserdatauv(L, (size_t)(argc - 4) * sizeof(int64_t), 0);
        lua_insert(L, 1);
    }
    int base = rest ? 1 : 0;
    for (int i = 0; i < argc; i++) {
        lua_pushvalue(L, base + i + 1);
        int64_t h = put(L);
        if (i < 4)
            first[i] = h;
        else
            rest[i - 4] = h;
    }
    int64_t* outerArgs = r->args;
    int64_t outerCount = r->argCount;
    r->args = rest;
    r->argCount = argc > 4 ? argc - 4 : 0;

    r->hostDepth++;
    int64_t res = r->host(fn, argc, first[0], first[1], first[2], first[3]);
    r->hostDepth--;

    r->args = outerArgs;
    r->argCount = outerCount;
    if (res == 0) {
        lua_rawgetp(L, LUA_REGISTRYINDEX, &THROWN);
        return lua_error(L);
    }
    if (res < 0) return 0;
    push_slot(L, res);
    return 1;
}

static AluaSource* find_source(Alua* r, const char* name) {
    for (int64_t i = 0; i < r->sourceLen; i++)
        if (strcmp(r->sources[i].name, name) == 0) return &r->sources[i];
    return NULL;
}

// Loads text as a chunk named `name`; bytecode is refused.
static void load_text(lua_State* L, const char* code, size_t len, const char* name) {
    lua_pushfstring(L, "@%s", name);
    int status = luaL_loadbufferx(L, code, len, lua_tostring(L, -1), "t");
    lua_remove(L, -2);
    if (status != LUA_OK) lua_error(L);
}

// Pushes the value of the module defined under `name`, running it on first
// use: what it returns, or true when it returns nothing.
static void require_module(lua_State* L, const char* name) {
    Alua* r = runtime_of(L);
    lua_rawgetp(L, LUA_REGISTRYINDEX, &LOADED);
    lua_getfield(L, -1, name);
    if (!lua_isnil(L, -1)) {
        lua_remove(L, -2);
        return;
    }
    lua_pop(L, 1);
    AluaSource* s = find_source(r, name);
    if (!s) luaL_error(L, "module '%s' is not defined", name);
    load_text(L, s->source, s->len, name);
    lua_pushstring(L, name);
    lua_call(L, 1, 1);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_pushboolean(L, 1);
    }
    lua_pushvalue(L, -1);
    lua_setfield(L, -3, name);
    lua_remove(L, -2);
}

// require(name) for scripts: the modules the program defined.
static int script_require(lua_State* L) {
    require_module(L, luaL_checkstring(L, 1));
    return 1;
}

// load(chunk, chunkname, mode, env) for scripts, with the mode forced to
// text: bytecode is not verified by Lua.
static int script_load(lua_State* L) {
    if (lua_gettop(L) < 3) lua_settop(L, 3);
    lua_pushliteral(L, "t");
    lua_replace(L, 3);
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_insert(L, 1);
    lua_call(L, lua_gettop(L) - 1, LUA_MULTRET);
    return lua_gettop(L);
}

// Opens the libraries scripts get and sets up the registry. Runs protected.
static int setup(lua_State* L) {
    static const luaL_Reg libs[] = {
        {LUA_GNAME, luaopen_base},     {LUA_COLIBNAME, luaopen_coroutine}, {LUA_TABLIBNAME, luaopen_table},
        {LUA_STRLIBNAME, luaopen_string}, {LUA_MATHLIBNAME, luaopen_math},  {LUA_UTF8LIBNAME, luaopen_utf8},
        {NULL, NULL},
    };
    for (const luaL_Reg* lib = libs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);
    }
    // No files: dofile and loadfile go, load takes text only, require finds
    // what the program defined.
    lua_pushnil(L);
    lua_setglobal(L, "dofile");
    lua_pushnil(L);
    lua_setglobal(L, "loadfile");
    lua_getglobal(L, "load");
    lua_pushcclosure(L, script_load, 1);
    lua_setglobal(L, "load");
    lua_pushcfunction(L, script_require);
    lua_setglobal(L, "require");

    lua_newtable(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &SLOTS);
    lua_newtable(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &LOADED);
    lua_pushboolean(L, 0);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &EXCEPTION);
    lua_pushboolean(L, 0);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &TRACE);
    lua_pushboolean(L, 0);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &THROWN);

    luaL_newmetatable(L, FOREIGN_TYPE);
    lua_pushcfunction(L, foreign_gc);
    lua_setfield(L, -2, "__gc");
    lua_pushboolean(L, 0);
    lua_setfield(L, -2, "__metatable");
    lua_pop(L, 1);
    return 0;
}

// --- Lifecycle ---------------------------------------------------------------

// A new runtime. memoryLimit is bytes, 0 for none.
Alua* aluaNew(int64_t memoryLimit, AluaHost host) {
    Alua* r = calloc(1, sizeof(Alua));
    if (!r) return NULL;
    r->host = host;
    pthread_mutex_init(&r->mu, NULL);
    pthread_cond_init(&r->cv, NULL);
    pthread_mutex_init(&r->queueMu, NULL);
    r->L = lua_newstate(alloc_hook, r);
    if (!r->L) {
        free(r);
        return NULL;
    }
    *(Alua**)lua_getextraspace(r->L) = r;
    lua_atpanic(r->L, on_panic);
    lua_pushcfunction(r->L, setup);
    if (lua_pcall(r->L, 0, 0, 0) != LUA_OK) {
        lua_close(r->L);
        free(r);
        return NULL;
    }
    // The limit starts after the libraries are open.
    r->memoryLimit = memoryLimit > 0 ? r->allocated + (size_t)memoryLimit : 0;
    lua_sethook(r->L, on_count, LUA_MASKCOUNT, HOOK_COUNT);
    return r;
}

// Frees the Lua side; the handle stays valid until aluaFree. False when
// called from inside a host function.
bool aluaClose(Alua* r) {
    bool ok = true;
    if (enter(r)) {
        if (r->depth > 1) {
            ok = false;
        } else {
            lua_State* L = r->L;
            r->memoryLimit = 0;
            r->L = NULL;
            lua_close(L);
            r->live = 0;
            for (int64_t i = 0; i < r->sourceLen; i++) {
                free(r->sources[i].name);
                free(r->sources[i].source);
            }
            free(r->sources);
            r->sources = NULL;
            r->sourceLen = r->sourceCap = 0;
        }
    }
    leave(r);
    return ok;
}

// Frees the handle.
void aluaFree(Alua* r) {
    if (!r) return;
    aluaClose(r);
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
void aluaRelease(Alua* r, int64_t h) {
    void* me = self_id();
    void* expected = NULL;
    if (atomic_load(&r->owner) == me) {
        if (r->hostDepth == 0) {
            if (r->L) free_slot(r, h);
            return;
        }
    } else if (atomic_compare_exchange_strong(&r->owner, &expected, me)) {
        r->depth = 1;
        if (r->L) free_slot(r, h);
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
int64_t aluaLive(Alua* r) {
    int64_t n = 0;
    if (enter(r)) n = r->live;
    leave(r);
    return n;
}

// Stops the running script, or the next one, from any task.
void aluaInterrupt(Alua* r) { atomic_store(&r->interrupt, 1); }

// Longest a call may run, in nanoseconds; 0 for no limit.
void aluaTimeout(Alua* r, int64_t ns) {
    enter(r);
    r->timeoutNs = ns;
    leave(r);
}

// Why the last failed call was stopped: 0 not stopped, 1 interrupted, 2 timed
// out, 3 out of memory.
int64_t aluaStopReason(Alua* r) {
    enter(r);
    int64_t s = r->failStop;
    r->failStop = STOP_NONE;
    leave(r);
    return s;
}

// Bytes the runtime holds.
int64_t aluaMemory(Alua* r) {
    int64_t n = 0;
    if (enter(r)) n = (int64_t)r->allocated;
    leave(r);
    return n;
}

// Runs a full collection.
void aluaCollect(Alua* r) {
    if (enter(r)) lua_gc(r->L, LUA_GCCOLLECT);
    leave(r);
}

typedef struct {
    int64_t a, b, c, n, out;
    double d;
    const char* s;
    const void* p;
    const int64_t* list;
    int64_t* outList;
    AluaBlob* blob;
    bool flag;
} Op;

static int op_exception(lua_State* L) {
    Op* op = lua_touserdata(L, 1);
    lua_rawgetp(L, LUA_REGISTRYINDEX, &EXCEPTION);
    op->out = put(L);
    return 0;
}

// The error the last failed call put aside, in a slot; 0 when there is none.
int64_t aluaException(Alua* r) {
    Op op = {0};
    if (enter(r) && r->hasException) {
        r->hasException = false;
        if (!protect(r, op_exception, &op)) op.out = 0;
        lua_pushboolean(r->L, 0);
        lua_rawsetp(r->L, LUA_REGISTRYINDEX, &EXCEPTION);
    }
    leave(r);
    return op.out;
}

static int op_trace(lua_State* L) {
    Op* op = lua_touserdata(L, 1);
    lua_rawgetp(L, LUA_REGISTRYINDEX, &TRACE);
    size_t len = 0;
    const char* s = lua_tolstring(L, -1, &len);
    op->blob = blob_new(s ? s : "", s ? len : 0);
    lua_pushboolean(L, 0);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &TRACE);
    return 0;
}

// The traceback of the last error a script raised, in a blob; empty when
// there is none.
AluaBlob* aluaTrace(Alua* r) {
    Op op = {0};
    if (enter(r)) protect(r, op_trace, &op);
    leave(r);
    return op.blob;
}

// tostring() of an error that carries an ADM error: its message.
static int error_text(lua_State* L) {
    lua_getfield(L, 1, "message");
    return 1;
}

static int op_throw(lua_State* L) {
    Op* op = lua_touserdata(L, 1);
    if (op->flag) {
        push_slot(L, op->a);
    } else if (op->b) {
        // An error that carries an ADM error: a table that prints as its
        // message and holds the foreign id.
        lua_createtable(L, 0, 2);
        lua_pushstring(L, op->s);
        lua_setfield(L, -2, "message");
        int64_t* id = lua_newuserdatauv(L, sizeof(int64_t), 0);
        *id = op->b;
        luaL_setmetatable(L, FOREIGN_TYPE);
        lua_setfield(L, -2, "admError");
        lua_createtable(L, 0, 1);
        lua_pushcfunction(L, error_text);
        lua_setfield(L, -2, "__tostring");
        lua_setmetatable(L, -2);
    } else {
        lua_pushstring(L, op->s);
    }
    lua_rawsetp(L, LUA_REGISTRYINDEX, &THROWN);
    return 0;
}

// Sets the error the host function in progress raises when it returns 0: a
// message, with a non-zero foreign id riding on it.
void aluaThrow(Alua* r, const char* message, int64_t foreignId) {
    Op op = {.s = message, .b = foreignId};
    if (enter(r) && !protect(r, op_throw, &op)) {
        lua_pushliteral(r->L, "not enough memory");
        lua_rawsetp(r->L, LUA_REGISTRYINDEX, &THROWN);
    }
    leave(r);
}

// The same with a value.
void aluaThrowValue(Alua* r, int64_t h) {
    Op op = {.a = h, .flag = true};
    if (enter(r)) protect(r, op_throw, &op);
    leave(r);
}

// --- Making values -----------------------------------------------------------

enum { MAKE_NIL, MAKE_BOOL, MAKE_NUMBER, MAKE_INTEGER, MAKE_STRING, MAKE_TABLE, MAKE_GLOBALS, MAKE_FUNCTION, MAKE_FOREIGN };

static int op_make(lua_State* L) {
    Op* op = lua_touserdata(L, 1);
    switch (op->c) {
    case MAKE_NIL:
        lua_pushnil(L);
        break;
    case MAKE_BOOL:
        lua_pushboolean(L, op->flag);
        break;
    case MAKE_NUMBER:
        lua_pushnumber(L, op->d);
        break;
    case MAKE_INTEGER:
        lua_pushinteger(L, op->a);
        break;
    case MAKE_STRING:
        lua_pushlstring(L, op->p, (size_t)op->a);
        break;
    case MAKE_TABLE:
        lua_newtable(L);
        break;
    case MAKE_GLOBALS:
        lua_pushglobaltable(L);
        break;
    case MAKE_FUNCTION:
        lua_pushinteger(L, op->a);
        lua_pushcclosure(L, host_call, 1);
        break;
    case MAKE_FOREIGN: {
        int64_t* id = lua_newuserdatauv(L, sizeof(int64_t), 0);
        *id = op->a;
        luaL_setmetatable(L, FOREIGN_TYPE);
        break;
    }
    }
    op->out = put(L);
    return 0;
}

static int64_t make(Alua* r, Op* op) {
    if (!enter(r) || !protect(r, op_make, op)) op->out = 0;
    leave(r);
    return op->out;
}

int64_t aluaNil(Alua* r) { return make(r, &(Op){.c = MAKE_NIL}); }
int64_t aluaBool(Alua* r, bool v) { return make(r, &(Op){.c = MAKE_BOOL, .flag = v}); }
int64_t aluaNumber(Alua* r, double v) { return make(r, &(Op){.c = MAKE_NUMBER, .d = v}); }
int64_t aluaInteger(Alua* r, int64_t v) { return make(r, &(Op){.c = MAKE_INTEGER, .a = v}); }
int64_t aluaString(Alua* r, const char* text) { return make(r, &(Op){.c = MAKE_STRING, .p = text, .a = (int64_t)strlen(text)}); }
// A string holding these bytes.
int64_t aluaStringOf(Alua* r, const uint8_t* data, int64_t len) { return make(r, &(Op){.c = MAKE_STRING, .p = data, .a = len}); }
int64_t aluaTable(Alua* r) { return make(r, &(Op){.c = MAKE_TABLE}); }
int64_t aluaGlobals(Alua* r) { return make(r, &(Op){.c = MAKE_GLOBALS}); }
// A function that calls host function `fn`.
int64_t aluaFunction(Alua* r, int64_t fn) { return make(r, &(Op){.c = MAKE_FUNCTION, .a = fn}); }
// An opaque object standing for the ADM object kept under `id`.
int64_t aluaForeign(Alua* r, int64_t id) { return make(r, &(Op){.c = MAKE_FOREIGN, .a = id}); }

// The id behind a foreign object; 0 for any other value.
int64_t aluaForeignId(Alua* r, int64_t h) {
    int64_t id = 0;
    if (enter(r)) {
        lua_State* L = r->L;
        push_slot(L, h);
        int64_t* p = luaL_testudata(L, -1, FOREIGN_TYPE);
        if (p) id = *p;
        lua_pop(L, 1);
    }
    leave(r);
    return id;
}

// Moves up to cap ids of collected foreign objects into out.
int64_t aluaDead(Alua* r, int64_t* out, int64_t cap) {
    int64_t n = 0;
    if (enter(r)) {
        while (n < cap && r->deadLen > 0) out[n++] = r->dead[--r->deadLen];
    }
    leave(r);
    return n;
}

// The arguments beyond the fourth of the host call in progress.
int64_t aluaArgs(Alua* r, int64_t* out, int64_t cap) {
    int64_t n = 0;
    if (enter(r)) {
        n = r->argCount < cap ? r->argCount : cap;
        for (int64_t i = 0; i < n; i++) out[i] = r->args[i];
    }
    leave(r);
    return n;
}

// --- Reading values ----------------------------------------------------------

// Lua's truth: everything but nil and false.
bool aluaToBool(Alua* r, int64_t h) {
    bool v = false;
    if (enter(r)) {
        push_slot(r->L, h);
        v = lua_toboolean(r->L, -1);
        lua_pop(r->L, 1);
    }
    leave(r);
    return v;
}

// A number as a float; 0 for anything else.
double aluaToNumber(Alua* r, int64_t h) {
    double v = 0;
    if (enter(r)) {
        push_slot(r->L, h);
        if (lua_type(r->L, -1) == LUA_TNUMBER) v = lua_tonumber(r->L, -1);
        lua_pop(r->L, 1);
    }
    leave(r);
    return v;
}

// An integer; 0 for anything else, a float included.
int64_t aluaToInteger(Alua* r, int64_t h) {
    int64_t v = 0;
    if (enter(r)) {
        push_slot(r->L, h);
        if (lua_isinteger(r->L, -1)) v = lua_tointeger(r->L, -1);
        lua_pop(r->L, 1);
    }
    leave(r);
    return v;
}

static int op_text(lua_State* L) {
    Op* op = lua_touserdata(L, 1);
    push_slot(L, op->a);
    size_t len = 0;
    const char* s = op->flag ? lua_tolstring(L, -1, &len) : luaL_tolstring(L, -1, &len);
    op->blob = blob_new(s ? s : "", s ? len : 0);
    if (!op->blob) luaL_error(L, "not enough memory");
    return 0;
}

// The value as tostring() gives it, in a blob; NULL when that raised.
AluaBlob* aluaText(Alua* r, int64_t h) {
    Op op = {.a = h};
    if (!enter(r) || !protect(r, op_text, &op)) op.blob = NULL;
    leave(r);
    return op.blob;
}

// The bytes of a string, in a blob; empty for any other value.
AluaBlob* aluaBytes(Alua* r, int64_t h) {
    Op op = {.a = h, .flag = true};
    if ((h >> KIND_SHIFT) != K_STRING) return blob_new("", 0);
    if (!enter(r) || !protect(r, op_text, &op)) op.blob = NULL;
    leave(r);
    return op.blob;
}

// --- Tables ------------------------------------------------------------------

enum { TABLE_GET, TABLE_GET_INDEX, TABLE_SET, TABLE_SET_INDEX, TABLE_LENGTH, TABLE_KEYS };

static int op_table(lua_State* L) {
    Op* op = lua_touserdata(L, 1);
    push_slot(L, op->a);
    switch (op->c) {
    case TABLE_GET:
        lua_getfield(L, -1, op->s);
        op->out = put(L);
        break;
    case TABLE_GET_INDEX:
        lua_geti(L, -1, op->b);
        op->out = put(L);
        break;
    case TABLE_SET:
        push_slot(L, op->b);
        lua_setfield(L, -2, op->s);
        break;
    case TABLE_SET_INDEX:
        push_slot(L, op->out);
        lua_seti(L, -2, op->b);
        break;
    case TABLE_LENGTH:
        op->out = luaL_len(L, -1);
        break;
    case TABLE_KEYS: {
        // Each string key as a 4-byte little-endian length and its bytes.
        // Nothing below raises, so the C buffer cannot leak.
        luaL_checktype(L, -1, LUA_TTABLE);
        int t = lua_gettop(L);
        ByteBuf buf = {0};
        lua_pushnil(L);
        while (lua_next(L, t)) {
            lua_pop(L, 1);
            if (lua_type(L, -1) != LUA_TSTRING) continue;
            size_t len = 0;
            const char* key = lua_tolstring(L, -1, &len);
            uint8_t head[4] = {(uint8_t)len, (uint8_t)(len >> 8), (uint8_t)(len >> 16), (uint8_t)(len >> 24)};
            buf_add(&buf, head, 4);
            buf_add(&buf, key, len);
        }
        op->blob = buf.failed ? NULL : blob_new(buf.data, buf.len);
        free(buf.data);
        if (!op->blob) luaL_error(L, "not enough memory");
        break;
    }
    }
    return 0;
}

static bool table_op(Alua* r, Op* op) {
    bool ok = enter(r) && protect(r, op_table, op);
    leave(r);
    return ok;
}

// value[name]; 0 when indexing raised.
int64_t aluaGet(Alua* r, int64_t h, const char* name) {
    Op op = {.c = TABLE_GET, .a = h, .s = name};
    return table_op(r, &op) ? op.out : 0;
}

// value[index].
int64_t aluaGetIndex(Alua* r, int64_t h, int64_t index) {
    Op op = {.c = TABLE_GET_INDEX, .a = h, .b = index};
    return table_op(r, &op) ? op.out : 0;
}

// value[name] = v. False when the assignment raised.
bool aluaSet(Alua* r, int64_t h, const char* name, int64_t v) {
    Op op = {.c = TABLE_SET, .a = h, .s = name, .b = v};
    return table_op(r, &op);
}

// value[index] = v.
bool aluaSetIndex(Alua* r, int64_t h, int64_t index, int64_t v) {
    Op op = {.c = TABLE_SET_INDEX, .a = h, .b = index, .out = v};
    return table_op(r, &op);
}

// #value; -1 when taking the length raised.
int64_t aluaLength(Alua* r, int64_t h) {
    Op op = {.c = TABLE_LENGTH, .a = h};
    return table_op(r, &op) ? op.out : -1;
}

// The table's string keys in a blob; NULL when the value is not a table.
AluaBlob* aluaKeys(Alua* r, int64_t h) {
    Op op = {.c = TABLE_KEYS, .a = h};
    return table_op(r, &op) ? op.blob : NULL;
}

static int op_numbers(lua_State* L) {
    Op* op = lua_touserdata(L, 1);
    push_slot(L, op->a);
    lua_Integer len = luaL_len(L, -1);
    int64_t n = len < op->b ? len : op->b;
    for (int64_t i = 0; i < n; i++) {
        lua_geti(L, -1, i + 1);
        if (op->outList) {
            int ok = 0;
            op->outList[i] = lua_tointegerx(L, -1, &ok);
            if (!ok || lua_type(L, -1) != LUA_TNUMBER) luaL_error(L, "element %d is not an integer", (int)(i + 1));
        } else {
            if (lua_type(L, -1) != LUA_TNUMBER) luaL_error(L, "element %d is not a number", (int)(i + 1));
            ((double*)op->p)[i] = lua_tonumber(L, -1);
        }
        lua_pop(L, 1);
    }
    op->out = n;
    return 0;
}

// Copies t[1..#t] into out as floats, up to cap of them, and returns how
// many it copied; -1 when reading raised or an element is not a number.
int64_t aluaNumbers(Alua* r, int64_t h, double* out, int64_t cap) {
    Op op = {.a = h, .b = cap, .p = out};
    if (!enter(r) || !protect(r, op_numbers, &op)) op.out = -1;
    leave(r);
    return op.out;
}

// The same for integers: each element an integer or a float with no
// fraction.
int64_t aluaIntegers(Alua* r, int64_t h, int64_t* out, int64_t cap) {
    Op op = {.a = h, .b = cap, .outList = out};
    if (!enter(r) || !protect(r, op_numbers, &op)) op.out = -1;
    leave(r);
    return op.out;
}

// Whether both are the same value, as rawequal compares.
bool aluaSame(Alua* r, int64_t a, int64_t b) {
    bool same = false;
    if (enter(r)) {
        push_slot(r->L, a);
        push_slot(r->L, b);
        same = lua_rawequal(r->L, -1, -2);
        lua_pop(r->L, 2);
    }
    leave(r);
    return same;
}

// --- Calls and code ----------------------------------------------------------

enum { RUN_CALL, RUN_INVOKE, RUN_EVAL, RUN_REQUIRE, RUN_BYTECODE, RUN_COMPILE };

static int dump_writer(lua_State* L, const void* p, size_t sz, void* ud) {
    (void)L;
    buf_add(ud, p, sz);
    return 0;
}

// Runs a script function or chunk. Results go to op->outList (up to op->b of
// them, op->out the count) or, without a list, the first one to op->out.
static int op_run(lua_State* L) {
    Op* op = lua_touserdata(L, 1);
    lua_settop(L, 0);
    lua_pushcfunction(L, on_error);
    int argc = 0;
    switch (op->c) {
    case RUN_CALL:
        push_slot(L, op->a);
        break;
    case RUN_INVOKE:
        push_slot(L, op->a);
        lua_getfield(L, -1, op->s);
        lua_insert(L, -2);
        argc = 1;
        break;
    case RUN_EVAL:
        load_text(L, op->p, (size_t)op->a, op->s);
        break;
    case RUN_REQUIRE:
        require_module(L, op->s);
        op->out = put(L);
        return 0;
    case RUN_BYTECODE:
        if (luaL_loadbufferx(L, op->p, (size_t)op->a, "=bytecode", "b") != LUA_OK) lua_error(L);
        break;
    case RUN_COMPILE: {
        load_text(L, op->p, (size_t)op->a, op->s);
        ByteBuf buf = {0};
        lua_dump(L, dump_writer, &buf, op->flag);
        op->blob = buf.failed ? NULL : blob_new(buf.data, buf.len);
        free(buf.data);
        if (!op->blob) luaL_error(L, "not enough memory");
        return 0;
    }
    }
    if (op->list) {
        luaL_checkstack(L, (int)op->n + 4, "too many arguments");
        for (int64_t i = 0; i < op->n; i++) push_slot(L, op->list[i]);
        argc += (int)op->n;
    }
    int status = lua_pcall(L, argc, LUA_MULTRET, 1);
    if (status != LUA_OK) return lua_error(L);
    int results = lua_gettop(L) - 1;
    if (op->outList) {
        int64_t n = results < op->b ? results : op->b;
        for (int64_t i = 0; i < n; i++) {
            lua_pushvalue(L, 2 + (int)i);
            op->outList[i] = put(L);
        }
        op->out = results;
        return 0;
    }
    if (results == 0) lua_pushnil(L);
    lua_settop(L, 2);
    op->out = put(L);
    return 0;
}

static int64_t run(Alua* r, Op* op, int64_t failed) {
    if (!enter(r) || !protect(r, op_run, op)) op->out = failed;
    leave(r);
    return op->out;
}

// Calls fn with the values in args and returns its first result (nil when it
// returns nothing); 0 when it raised.
int64_t aluaCall(Alua* r, int64_t fn, const int64_t* args, int64_t argc) {
    return run(r, &(Op){.c = RUN_CALL, .a = fn, .list = args, .n = argc}, 0);
}

// The same, with up to cap results in out; returns how many the function
// returned, -1 when it raised.
int64_t aluaCallAll(Alua* r, int64_t fn, const int64_t* args, int64_t argc, int64_t* out, int64_t cap) {
    return run(r, &(Op){.c = RUN_CALL, .a = fn, .list = args, .n = argc, .outList = out, .b = cap}, -1);
}

// obj:name(args...), first result.
int64_t aluaInvoke(Alua* r, int64_t obj, const char* name, const int64_t* args, int64_t argc) {
    return run(r, &(Op){.c = RUN_INVOKE, .a = obj, .s = name, .list = args, .n = argc}, 0);
}

// Runs source text as a chunk and returns its first result.
int64_t aluaEval(Alua* r, const char* code, const char* name) {
    return run(r, &(Op){.c = RUN_EVAL, .p = code, .a = (int64_t)strlen(code), .s = name}, 0);
}

// Makes a module's source available to require under name, without running it.
void aluaDefine(Alua* r, const char* name, const char* code) {
    size_t len = strlen(code);
    if (enter(r)) {
        AluaSource* s = find_source(r, name);
        if (!s) {
            if (r->sourceLen == r->sourceCap) {
                r->sourceCap = r->sourceCap ? r->sourceCap * 2 : 8;
                r->sources = realloc(r->sources, (size_t)r->sourceCap * sizeof(AluaSource));
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

// The value of the module defined under name, run on first use.
int64_t aluaRequire(Alua* r, const char* name) {
    return run(r, &(Op){.c = RUN_REQUIRE, .s = name}, 0);
}

// Compiles source text to bytecode in a blob; NULL when it does not compile.
AluaBlob* aluaCompile(Alua* r, const char* code, const char* name, bool strip) {
    Op op = {.c = RUN_COMPILE, .p = code, .a = (int64_t)strlen(code), .s = name, .flag = strip};
    if (!enter(r) || !protect(r, op_run, &op)) op.blob = NULL;
    leave(r);
    return op.blob;
}

// Runs bytecode from aluaCompile and returns its first result. Lua does not
// verify bytecode: anything else can crash the program.
int64_t aluaRun(Alua* r, const uint8_t* code, int64_t len) {
    return run(r, &(Op){.c = RUN_BYTECODE, .p = code, .a = len}, 0);
}
