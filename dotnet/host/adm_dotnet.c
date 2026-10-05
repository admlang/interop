// The C side of adm.interop.dotnet: the .NET runtime loaded at run time and
// kept on a thread of its own.
//
// The runtime reads the stack bounds of the threads it runs on, so it is
// started on a thread this file creates (with an ordinary large stack) and
// every request is handed to it: the caller waits until that thread has
// answered. Nothing here needs the .NET SDK to build: libhostfxr is opened
// with dlopen and asked for the runtime's function that loads an assembly
// and returns a pointer to one of its methods. That method is the entry of
// the bridge assembly (bridge/Bridge.cs), which does the work in .NET: a
// request is bytes for it, the answer the bytes it returns.
//
// When .NET calls a function of the ADM program, the bridge calls
// native_upcall here, and the request that is waiting returns to its ADM
// task with that call to run (adnRequest returns 1); the task answers with
// adnResume and goes on waiting.

#include <dirent.h>
#include <dlfcn.h>
#include <limits.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// A block of bytes handed to the ADM side: an answer, or the arguments of a
// call into the program.
typedef struct { size_t size; char data[]; } blob;

static blob* blob_of(const void* data, size_t size) {
    blob* b = malloc(sizeof(blob) + size + 1);
    if (!b) abort();
    b->size = size;
    if (size) memcpy(b->data, data, size);
    b->data[size] = 0;
    return b;
}

int64_t adnBlobSize(uint64_t handle) { return handle ? (int64_t)((blob*)(uintptr_t)handle)->size : 0; }

void adnBlobRead(uint64_t handle, uint8_t* out, int64_t size) {
    blob* b = (blob*)(uintptr_t)handle;
    if (b && size > 0) memcpy(out, b->data, (size_t)size < b->size ? (size_t)size : b->size);
}

void adnBlobFree(uint64_t handle) { free((void*)(uintptr_t)handle); }

typedef void (*upcall_fn)(int64_t fn, const uint8_t* args, int64_t size, uint8_t** reply, int64_t* replySize);

// The request the bridge's entry reads (Bridge.Request).
typedef struct {
    const uint8_t* data;
    int64_t size;
    uint8_t** out;
    int64_t* outSize;
    upcall_fn upcall;
} bridge_request;

typedef int (*entry_fn)(void* args, int32_t size);

struct request;

// A call from .NET into the ADM program, waiting for its answer.
typedef struct upcall {
    int64_t fn;
    blob* args;
    blob* reply;
    int taken;  // an ADM task is running it
    int done;
    struct request* owner;  // the request it came out of; 0 from a thread .NET started
    struct upcall* next;
} upcall;

typedef struct request {
    blob* data;
    blob* answer;
    int done;
    upcall* pending;  // the call into the program this request waits on
    struct request* next;
} request;

static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t wake = PTHREAD_COND_INITIALIZER;      // the runtime thread: a request or an answer arrived
static pthread_cond_t answered = PTHREAD_COND_INITIALIZER;  // ADM tasks and threads .NET started
static request* queue_head;
static request* queue_tail;
// Calls into the program from threads .NET started: the next ADM task that
// waits here runs them.
static upcall* stray_head;
static upcall* stray_tail;
static int started;  // 0 not yet, 1 starting, 2 running, 3 failed
static char start_error[PATH_MAX + 256];
static char* fxr_path;
static char* config_path;
static char* bridge_path;
static entry_fn entry;
static pthread_t runtime_thread_id;
static request* current;  // the request the runtime thread is in, innermost

// A hand-off between two threads through a condition variable costs several
// microseconds each way, most of a short call. So a thread that is about to
// wait first watches `changes`, which counts the signals, for SPIN_NS without
// sleeping: the answer to a short call arrives within that time. Signals
// are sent, and `changes` counted, with the lock held.
#define SPIN_NS 50000
static _Atomic uint64_t changes;
static int spinning = -1;      // whether there is a second processor to spin on

static int64_t now_ns(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (int64_t)t.tv_sec * 1000000000 + t.tv_nsec;
}

// When a wait that starts now stops watching and sleeps; 0 for at once.
static int64_t spin_until(void) {
    if (spinning < 0) spinning = sysconf(_SC_NPROCESSORS_ONLN) > 1;
    return spinning ? now_ns() + SPIN_NS : 0;
}

// Signals the threads waiting on `c`. The lock is held.
static void signal_all(pthread_cond_t* c) {
    atomic_fetch_add_explicit(&changes, 1, memory_order_release);
    pthread_cond_broadcast(c);
}

// Waits, with the lock held, for a signal on `c`: watching until `deadline`
// (from spin_until), asleep after it. Returns with the lock held, possibly
// without a signal for this thread: the caller checks what it waits for.
static void wait_on(pthread_cond_t* c, int64_t deadline) {
    uint64_t seen = atomic_load_explicit(&changes, memory_order_relaxed);
    if (deadline) {
        pthread_mutex_unlock(&mu);
        for (;;) {
            if (atomic_load_explicit(&changes, memory_order_acquire) != seen) {
                pthread_mutex_lock(&mu);
                return;
            }
            if (now_ns() >= deadline) break;
#if defined(__x86_64__) || defined(__i386__)
            __builtin_ia32_pause();
#elif defined(__aarch64__)
            __asm__ volatile("yield");
#endif
        }
        pthread_mutex_lock(&mu);
    }
    if (atomic_load_explicit(&changes, memory_order_relaxed) == seen) pthread_cond_wait(c, &mu);
}

static void native_upcall(int64_t fn, const uint8_t* args, int64_t size, uint8_t** reply, int64_t* replySize);

// Runs a request in .NET, on the runtime thread.
static void run(request* r) {
    request* outer = current;
    current = r;
    uint8_t* out = 0;
    int64_t outSize = 0;
    bridge_request call = {(const uint8_t*)r->data->data, (int64_t)r->data->size, &out, &outSize, native_upcall};
    entry(&call, (int32_t)sizeof call);
    // The bridge allocates with NativeMemory.Alloc, which is malloc.
    r->answer = blob_of(out, out ? (size_t)outSize : 0);
    free(out);
    current = outer;
}

// .NET calls host function `fn` of the ADM program. On the runtime thread
// the call goes to the ADM task whose request .NET is running, and the
// thread runs further requests while it waits, so the function can call
// back into .NET. From a thread .NET started it goes to whichever ADM task
// waits here next. The bridge frees *reply (NativeMemory.Free, which is
// free).
static void native_upcall(int64_t fn, const uint8_t* args, int64_t size, uint8_t** reply, int64_t* replySize) {
    upcall u = {.fn = fn, .args = blob_of(args, (size_t)size)};
    pthread_mutex_lock(&mu);
    int inside = pthread_equal(pthread_self(), runtime_thread_id) && current;
    if (inside) {
        u.owner = current;
        current->pending = &u;
    } else {
        if (stray_tail) stray_tail->next = &u;
        else stray_head = &u;
        stray_tail = &u;
    }
    signal_all(&answered);
    int64_t deadline = spin_until();
    while (!u.done) {
        if (inside && queue_head) {
            request* r = queue_head;
            queue_head = r->next;
            if (!queue_head) queue_tail = 0;
            pthread_mutex_unlock(&mu);
            run(r);
            pthread_mutex_lock(&mu);
            r->done = 1;
            signal_all(&answered);
            deadline = spin_until();
        } else {
            wait_on(inside ? &wake : &answered, deadline);
        }
    }
    pthread_mutex_unlock(&mu);
    uint8_t* out = malloc(u.reply->size ? u.reply->size : 1);
    if (!out) abort();
    memcpy(out, u.reply->data, u.reply->size);
    *reply = out;
    *replySize = (int64_t)u.reply->size;
    free(u.reply);
}

static void* runtime_thread(void* unused) {
    (void)unused;
    char why[sizeof start_error];
    why[0] = 0;
    void* lib = dlopen(fxr_path, RTLD_NOW | RTLD_GLOBAL);
    if (!lib) {
        snprintf(why, sizeof why, "cannot load the .NET host from %s: %s", fxr_path, dlerror());
    } else {
        int (*init)(const char*, const void*, void**) = (int (*)(const char*, const void*, void**))dlsym(lib, "hostfxr_initialize_for_runtime_config");
        int (*delegate)(void*, int, void**) = (int (*)(void*, int, void**))dlsym(lib, "hostfxr_get_runtime_delegate");
        int (*close_handle)(void*) = (int (*)(void*))dlsym(lib, "hostfxr_close");
        void* handle = 0;
        void* loader = 0;
        int rc = init && delegate ? init(config_path, 0, &handle) : -1;
        // 5 is hdt_load_assembly_and_get_function_pointer.
        if (rc < 0 || !handle) {
            snprintf(why, sizeof why, "the .NET runtime did not start (error 0x%x): is a .NET runtime of version 8 or later installed?", (unsigned)rc);
        } else if ((rc = delegate(handle, 5, &loader)) != 0 || !loader) {
            snprintf(why, sizeof why, "the .NET runtime gave no assembly loader (error 0x%x)", (unsigned)rc);
        } else {
            int (*load)(const char*, const char*, const char*, const char*, void*, void**) =
                (int (*)(const char*, const char*, const char*, const char*, void*, void**))loader;
            rc = load(bridge_path, "Adm.Bridge, AdmBridge", "Entry", 0, 0, (void**)&entry);
            if (rc != 0 || !entry) snprintf(why, sizeof why, "the .NET runtime could not load %s (error 0x%x)", bridge_path, (unsigned)rc);
        }
        if (handle && close_handle) close_handle(handle);
    }
    pthread_mutex_lock(&mu);
    if (why[0]) {
        memcpy(start_error, why, sizeof why);
        started = 3;
        pthread_cond_broadcast(&answered);
        pthread_mutex_unlock(&mu);
        return 0;
    }
    started = 2;
    pthread_cond_broadcast(&answered);
    int64_t deadline = 0;
    for (;;) {
        while (!queue_head) wait_on(&wake, deadline);
        request* r = queue_head;
        queue_head = r->next;
        if (!queue_head) queue_tail = 0;
        pthread_mutex_unlock(&mu);
        run(r);
        pthread_mutex_lock(&mu);
        r->done = 1;
        signal_all(&answered);
        // The next request of a program that calls in a loop follows at once.
        deadline = spin_until();
    }
    return 0;
}

// Starts the .NET runtime of this process: `fxr` is libhostfxr, `config`
// the runtime configuration file and `bridge` the bridge assembly. Writes
// why it did not start to `why` and returns the length of that text, 0
// when it runs. A second call returns what the first did.
int64_t adnStart(const char* fxr, const char* config, const char* bridge, uint8_t* why, int64_t cap) {
    pthread_mutex_lock(&mu);
    if (started == 0) {
        started = 1;
        fxr_path = strdup(fxr);
        config_path = strdup(config);
        bridge_path = strdup(bridge);
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 16u << 20);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (pthread_create(&runtime_thread_id, &attr, runtime_thread, 0) != 0) {
            snprintf(start_error, sizeof start_error, "cannot start a thread for the .NET runtime");
            started = 3;
        }
        pthread_attr_destroy(&attr);
    }
    while (started == 1) pthread_cond_wait(&answered, &mu);
    int64_t n = 0;
    if (started == 3) {
        n = (int64_t)strlen(start_error);
        if (n > cap) n = cap;
        memcpy(why, start_error, (size_t)n);
    }
    pthread_mutex_unlock(&mu);
    return n;
}

// Waits, with the lock held, until the request is answered or a call into
// the program wants running. Returns 0 with the answer as a blob in out[0],
// or 1 with the call: its arguments as a blob in out[0], the call in
// out[1], the function in out[2] and the request in out[3].
static int64_t await(request* r, int64_t* out) {
    int64_t deadline = spin_until();
    for (;;) {
        upcall* u = 0;
        if (r->pending && !r->pending->taken) {
            u = r->pending;
        } else if (stray_head) {
            u = stray_head;
            stray_head = u->next;
            if (!stray_head) stray_tail = 0;
        }
        if (u) {
            u->taken = 1;
            out[0] = (int64_t)(uintptr_t)u->args;
            out[1] = (int64_t)(uintptr_t)u;
            out[2] = u->fn;
            out[3] = (int64_t)(uintptr_t)r;
            u->args = 0;
            pthread_mutex_unlock(&mu);
            return 1;
        }
        if (r->done) {
            out[0] = (int64_t)(uintptr_t)r->answer;
            pthread_mutex_unlock(&mu);
            free(r->data);
            free(r);
            return 0;
        }
        wait_on(&answered, deadline);
    }
}

// Hands a request (`head` then `data`: bytes for the bridge) to the runtime thread and waits;
// see await for what comes back. The runtime must run.
int64_t adnRequest(const uint8_t* head, int64_t headSize, const uint8_t* data, int64_t size, int64_t* out) {
    request* r = calloc(1, sizeof(request));
    blob* whole = malloc(sizeof(blob) + (size_t)headSize + (size_t)size + 1);
    if (!r || !whole) abort();
    whole->size = (size_t)headSize + (size_t)size;
    if (headSize > 0) memcpy(whole->data, head, (size_t)headSize);
    if (size > 0) memcpy(whole->data + headSize, data, (size_t)size);
    whole->data[whole->size] = 0;
    r->data = whole;
    pthread_mutex_lock(&mu);
    if (queue_tail) queue_tail->next = r;
    else queue_head = r;
    queue_tail = r;
    signal_all(&wake);
    return await(r, out);
}

// The requests are little-endian, as the processors ADM runs on are, so the
// elements of a numeric array cross as they lie in memory.
_Static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "requests are packed for a little-endian processor");

// Copies `bytes` bytes of an array or a string into a request, at `at`.
void adnPut(uint8_t* dst, int64_t at, const void* src, int64_t bytes) {
    if (bytes > 0) memcpy(dst + at, src, (size_t)bytes);
}

// Copies `bytes` bytes of an answer, from `at`, into an array.
void adnTake(void* dst, const uint8_t* src, int64_t at, int64_t bytes) {
    if (bytes > 0) memcpy(dst, src + at, (size_t)bytes);
}

// The length of a string in bytes.
int64_t adnTextSize(const char* text) { return text ? (int64_t)strlen(text) : 0; }

// Answers the call into the program that adnRequest or adnResume returned,
// and goes on waiting for the request.
int64_t adnResume(uint64_t req, uint64_t call, const uint8_t* reply, int64_t size, int64_t* out) {
    request* r = (request*)(uintptr_t)req;
    upcall* u = (upcall*)(uintptr_t)call;
    blob* answer = blob_of(reply, (size_t)size);
    pthread_mutex_lock(&mu);
    // Once done is set the thread that made the call may return, and the
    // call is gone.
    if (u->owner) u->owner->pending = 0;
    u->reply = answer;
    u->done = 1;
    signal_all(&wake);
    signal_all(&answered);
    return await(r, out);
}

// Compares two version folder names ("10.0.12", "8.0.30") by their numbers.
static int version_compare(const char* a, const char* b) {
    while (*a || *b) {
        long x = strtol(a, (char**)&a, 10);
        long y = strtol(b, (char**)&b, 10);
        if (x != y) return x < y ? -1 : 1;
        if (*a == '.') a++;
        if (*b == '.') b++;
        if ((*a && (*a < '0' || *a > '9')) || (*b && (*b < '0' || *b > '9'))) return strcmp(a, b);
    }
    return 0;
}

// The newest libhostfxr under a .NET installation; 1 when there is one.
static int fxr_under(const char* root, char* out, size_t cap) {
    char dir[PATH_MAX];
    snprintf(dir, sizeof dir, "%s/host/fxr", root);
    DIR* d = opendir(dir);
    if (!d) return 0;
    char best[256];
    best[0] = 0;
    struct dirent* e;
    while ((e = readdir(d))) {
        if (e->d_name[0] < '0' || e->d_name[0] > '9' || strlen(e->d_name) >= sizeof best) continue;
        if (!best[0] || version_compare(e->d_name, best) > 0) strcpy(best, e->d_name);
    }
    closedir(d);
    if (!best[0]) return 0;
    static const char* names[] = {"libhostfxr.so", "libhostfxr.dylib"};
    for (size_t i = 0; i < 2; i++) {
        snprintf(out, cap, "%s/%s/%s", dir, best, names[i]);
        if (access(out, R_OK) == 0) return 1;
    }
    return 0;
}

// Finds libhostfxr: under `root` (DOTNET_ROOT) when it is set, else beside
// the `dotnet` found on `path` (PATH), following links to the installation,
// else in the usual places. Writes its absolute path to `out` and returns
// its length, 0 when there is none.
int64_t adnLocate(const char* root, const char* path, uint8_t* out, int64_t cap) {
    char found[PATH_MAX];
    found[0] = 0;
    if (root && root[0]) {
        fxr_under(root, found, sizeof found) || (found[0] = 0);
    } else {
        const char* at = path ? path : "";
        while (*at && !found[0]) {
            const char* end = strchr(at, ':');
            size_t n = end ? (size_t)(end - at) : strlen(at);
            char launcher[PATH_MAX], real[PATH_MAX];
            if (n && n + 8 < sizeof launcher) {
                memcpy(launcher, at, n);
                memcpy(launcher + n, "/dotnet", 8);
                if (realpath(launcher, real)) {
                    char* slash = strrchr(real, '/');
                    if (slash) *slash = 0;
                    fxr_under(real, found, sizeof found) || (found[0] = 0);
                }
            }
            at += n + (end ? 1 : 0);
        }
        static const char* usual[] = {"/usr/lib64/dotnet", "/usr/lib/dotnet", "/usr/share/dotnet", "/usr/local/share/dotnet"};
        for (size_t i = 0; i < 4 && !found[0]; i++) fxr_under(usual[i], found, sizeof found) || (found[0] = 0);
    }
    size_t n = strlen(found);
    if (n == 0 || (int64_t)n > cap) return 0;
    memcpy(out, found, n);
    return (int64_t)n;
}
