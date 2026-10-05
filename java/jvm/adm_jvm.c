// The C side of adm.interop.java: a Java virtual machine loaded at run time
// and kept on a thread of its own.
//
// The JVM reads the stack bounds of the thread it runs on, and every JNI
// environment belongs to one thread, so the VM is created on a thread this
// file starts (with an ordinary large stack) and every request is handed to
// it: the caller waits until the JVM thread has answered. Nothing here needs
// the JDK to build: libjvm is opened with dlopen, and JNI functions are
// called through their positions in the JNI function table, which the JNI
// specification fixes.
//
// The work is done in Java, by the bridge class (bridge/Bridge.java) this
// file defines in the VM: a request is bytes for Bridge.request, the answer
// the bytes it returns. When Java calls a function of the ADM program, the
// bridge calls back into native_upcall here, and the request that is waiting
// returns to its ADM task with that call to run (ajvmRequest returns 1); the
// task answers with ajvmResume and goes on waiting.

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

// Positions in the JNI function table (JNI specification, "Interface
// Function Table").
enum {
    J_DefineClass = 5, J_FindClass = 6, J_ExceptionDescribe = 16, J_ExceptionClear = 17, J_NewGlobalRef = 21,
    J_DeleteLocalRef = 23, J_GetStaticMethodID = 113, J_CallStaticObjectMethodA = 116, J_GetArrayLength = 171,
    J_NewByteArray = 176, J_GetByteArrayRegion = 200, J_SetByteArrayRegion = 208, J_RegisterNatives = 215,
    J_ExceptionCheck = 228,
};

typedef void** JNIEnv;  // a pointer to the function table
typedef void* jobject;
typedef union { uint8_t z; int8_t b; uint16_t c; int16_t s; int32_t i; int64_t j; float f; double d; jobject l; } jvalue;
typedef struct { char* optionString; void* extraInfo; } JavaVMOption;
typedef struct { int32_t version; int32_t nOptions; JavaVMOption* options; uint8_t ignoreUnrecognized; } JavaVMInitArgs;
typedef struct { const char* name; const char* signature; void* fnPtr; } JNINativeMethod;

#define JNI(env, index, type) ((type)((*(env))[index]))

// A block of bytes handed to the ADM side: an answer, the arguments of a
// call into the program, or text.
typedef struct { size_t size; char data[]; } blob;

static blob* blob_new(size_t size) {
    blob* b = malloc(sizeof(blob) + size + 1);
    if (!b) abort();
    b->size = size;
    b->data[size] = 0;
    return b;
}

static uint64_t blob_of(const char* data, size_t size) {
    blob* b = blob_new(size);
    if (size) memcpy(b->data, data, size);
    return (uint64_t)(uintptr_t)b;
}

static uint64_t blob_text(const char* text) { return blob_of(text, strlen(text)); }

int64_t ajvmBlobSize(uint64_t handle) { return handle ? (int64_t)((blob*)(uintptr_t)handle)->size : 0; }

void ajvmBlobRead(uint64_t handle, uint8_t* out, int64_t size) {
    blob* b = (blob*)(uintptr_t)handle;
    if (b && size > 0) memcpy(out, b->data, (size_t)size < b->size ? (size_t)size : b->size);
}

void ajvmBlobFree(uint64_t handle) { free((void*)(uintptr_t)handle); }

struct request;

// A call from Java into the ADM program, waiting for its answer.
typedef struct upcall {
    int64_t fn;
    blob* args;
    blob* reply;
    int taken;  // an ADM task is running it
    int done;
    struct request* owner;  // the request it came out of; 0 from a thread Java started
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
static pthread_cond_t wake = PTHREAD_COND_INITIALIZER;      // the JVM thread: a request or an answer arrived
static pthread_cond_t answered = PTHREAD_COND_INITIALIZER;  // ADM tasks and threads Java started
static request* queue_head;
static request* queue_tail;
// Calls into the program from threads Java started: the next ADM task that
// waits here runs them.
static upcall* stray_head;
static upcall* stray_tail;
static int started;            // 0 not yet, 1 starting, 2 running, 3 failed
static uint64_t start_error;
static char* library_path;
static JavaVMOption* vm_options;
static int vm_option_count;
static blob* bridge_bytes;
static JNIEnv* env;            // the JVM thread's
static pthread_t jvm_thread_id;
static jobject bridge_class;
static void* bridge_request;
static request* current;       // the request the JVM thread is in, innermost

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

static int pending(JNIEnv* e) { return JNI(e, J_ExceptionCheck, uint8_t (*)(JNIEnv*))(e); }

static jobject bytes_new(JNIEnv* e, const blob* b) {
    jobject array = JNI(e, J_NewByteArray, jobject (*)(JNIEnv*, int32_t))(e, (int32_t)b->size);
    if (array && b->size) JNI(e, J_SetByteArrayRegion, void (*)(JNIEnv*, jobject, int32_t, int32_t, const void*))(e, array, 0, (int32_t)b->size, b->data);
    return array;
}

static blob* bytes_of(JNIEnv* e, jobject array) {
    int32_t n = JNI(e, J_GetArrayLength, int32_t (*)(JNIEnv*, jobject))(e, array);
    blob* b = blob_new((size_t)n);
    if (n) JNI(e, J_GetByteArrayRegion, void (*)(JNIEnv*, jobject, int32_t, int32_t, void*))(e, array, 0, n, b->data);
    return b;
}

// An answer made here, for a request the bridge could not answer: the
// "binding" status of Bridge.java and a text.
static blob* refusal(const char* text) {
    size_t n = strlen(text);
    blob* b = blob_new(5 + n);
    uint32_t size = (uint32_t)n;
    b->data[0] = 5;
    memcpy(b->data + 1, &size, 4);
    memcpy(b->data + 5, text, n);
    return b;
}

// Runs a request in Java, on the JVM thread.
static void run(request* r) {
    request* outer = current;
    current = r;
    jobject in = bytes_new(env, r->data);
    jvalue arg = {.l = in};
    jobject out = in ? JNI(env, J_CallStaticObjectMethodA, jobject (*)(JNIEnv*, jobject, void*, const jvalue*))(env, bridge_class, bridge_request, &arg) : 0;
    if (pending(env) || !out) {
        JNI(env, J_ExceptionClear, void (*)(JNIEnv*))(env);
        r->answer = refusal("the Java VM could not run the request (out of memory?)");
    } else {
        r->answer = bytes_of(env, out);
    }
    if (out) JNI(env, J_DeleteLocalRef, void (*)(JNIEnv*, jobject))(env, out);
    if (in) JNI(env, J_DeleteLocalRef, void (*)(JNIEnv*, jobject))(env, in);
    current = outer;
}

// Bridge.upcall: Java calls host function `fn` of the ADM program. On the
// JVM thread the call goes to the ADM task whose request Java is running,
// and the thread runs further requests while it waits, so the function can
// call back into Java. From a thread Java started it goes to whichever ADM
// task waits here next.
static jobject native_upcall(JNIEnv* e, jobject cls, int64_t fn, jobject args) {
    (void)cls;
    upcall u = {.fn = fn, .args = bytes_of(e, args)};
    pthread_mutex_lock(&mu);
    int inside = pthread_equal(pthread_self(), jvm_thread_id) && current;
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
    jobject out = bytes_new(e, u.reply);
    free(u.reply);
    return out;
}

// Creates the VM and defines the bridge class in it. Returns why it could
// not, as text in `why`.
static void bring_up(char* why, size_t cap) {
    void* lib = dlopen(library_path, RTLD_NOW | RTLD_GLOBAL);
    int32_t (*create)(void**, void**, void*) = lib ? (int32_t (*)(void**, void**, void*))dlsym(lib, "JNI_CreateJavaVM") : 0;
    if (!create) {
        snprintf(why, cap, "cannot load the Java VM from %s: %s", library_path, dlerror());
        return;
    }
    void* vm = 0;
    JavaVMInitArgs args = {0x00010008 /* JNI 1.8 */, vm_option_count, vm_options, 0};
    int32_t rc = create(&vm, (void**)&env, &args);
    if (rc != 0) {
        snprintf(why, cap, "the Java VM in %s did not start (JNI error %d): check the options given to it", library_path, rc);
        return;
    }
    // ClassLoader.getSystemClassLoader(): the bridge finds classes on the
    // class path through the loader that defines it.
    jobject loaders = JNI(env, J_FindClass, jobject (*)(JNIEnv*, const char*))(env, "java/lang/ClassLoader");
    void* system = loaders ? JNI(env, J_GetStaticMethodID, void* (*)(JNIEnv*, jobject, const char*, const char*))(env, loaders, "getSystemClassLoader", "()Ljava/lang/ClassLoader;") : 0;
    jobject loader = system ? JNI(env, J_CallStaticObjectMethodA, jobject (*)(JNIEnv*, jobject, void*, const jvalue*))(env, loaders, system, 0) : 0;
    jobject cls = loader && !pending(env)
        ? JNI(env, J_DefineClass, jobject (*)(JNIEnv*, const char*, jobject, const void*, int32_t))(env, "adm/interop/Bridge", loader, bridge_bytes->data, (int32_t)bridge_bytes->size)
        : 0;
    JNINativeMethod native = {"upcall", "(J[B)[B", (void*)native_upcall};
    if (!cls || pending(env)
        || JNI(env, J_RegisterNatives, int32_t (*)(JNIEnv*, jobject, const JNINativeMethod*, int32_t))(env, cls, &native, 1) != 0
        || !(bridge_request = JNI(env, J_GetStaticMethodID, void* (*)(JNIEnv*, jobject, const char*, const char*))(env, cls, "request", "([B)[B"))) {
        // The VM prints what it refused to standard error.
        if (pending(env)) JNI(env, J_ExceptionDescribe, void (*)(JNIEnv*))(env);
        JNI(env, J_ExceptionClear, void (*)(JNIEnv*))(env);
        snprintf(why, cap, "the Java VM in %s could not take the bridge class of adm.interop.java: it needs Java 8 or later", library_path);
        return;
    }
    bridge_class = JNI(env, J_NewGlobalRef, jobject (*)(JNIEnv*, jobject))(env, cls);
}

static void* jvm_thread(void* unused) {
    (void)unused;
    char why[PATH_MAX + 256];
    why[0] = 0;
    bring_up(why, sizeof why);
    pthread_mutex_lock(&mu);
    if (why[0]) {
        start_error = blob_text(why);
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

// Starts the Java VM of this process from the library at `library`, with
// `count` options packed one after the other, each ending in a zero byte,
// and defines the bridge class from `bridge`. Returns 0, or a blob saying
// why it did not start. A second call returns what the first did: a process
// has one Java VM.
uint64_t ajvmStart(const char* library, const uint8_t* options, int64_t count, const uint8_t* bridge, int64_t bridgeSize) {
    pthread_mutex_lock(&mu);
    if (started == 0) {
        started = 1;
        library_path = strdup(library);
        bridge_bytes = (blob*)(uintptr_t)blob_of((const char*)bridge, (size_t)bridgeSize);
        vm_option_count = (int)count;
        vm_options = calloc((size_t)count + 1, sizeof(JavaVMOption));
        const char* at = (const char*)options;
        for (int64_t i = 0; i < count; i++) {
            vm_options[i].optionString = strdup(at);
            at += strlen(at) + 1;
        }
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 16u << 20);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (pthread_create(&jvm_thread_id, &attr, jvm_thread, 0) != 0) {
            start_error = blob_text("cannot start a thread for the Java VM");
            started = 3;
        }
        pthread_attr_destroy(&attr);
    }
    while (started == 1) pthread_cond_wait(&answered, &mu);
    uint64_t out = 0;
    if (started == 3) out = blob_of(((blob*)(uintptr_t)start_error)->data, ((blob*)(uintptr_t)start_error)->size);
    pthread_mutex_unlock(&mu);
    return out;
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

// Hands a request (`head` then `data`: bytes for Bridge.request) to the JVM thread and waits;
// see await for what comes back. The VM must run.
int64_t ajvmRequest(const uint8_t* head, int64_t headSize, const uint8_t* data, int64_t size, int64_t* out) {
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
void ajvmPut(uint8_t* dst, int64_t at, const void* src, int64_t bytes) {
    if (bytes > 0) memcpy(dst + at, src, (size_t)bytes);
}

// Copies `bytes` bytes of an answer, from `at`, into an array.
void ajvmTake(void* dst, const uint8_t* src, int64_t at, int64_t bytes) {
    if (bytes > 0) memcpy(dst, src + at, (size_t)bytes);
}

// The length of a string in bytes.
int64_t ajvmTextSize(const char* text) { return text ? (int64_t)strlen(text) : 0; }

// Answers the call into the program that ajvmRequest or ajvmResume
// returned, and goes on waiting for the request.
int64_t ajvmResume(uint64_t req, uint64_t call, const uint8_t* reply, int64_t size, int64_t* out) {
    request* r = (request*)(uintptr_t)req;
    upcall* u = (upcall*)(uintptr_t)call;
    blob* answer = (blob*)(uintptr_t)blob_of((const char*)reply, (size_t)size);
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

static int jvm_under(const char* home, char* out, size_t cap) {
    static const char* places[] = {
        "lib/server/libjvm.so", "lib/server/libjvm.dylib", "jre/lib/server/libjvm.dylib",
        "jre/lib/amd64/server/libjvm.so", "jre/lib/aarch64/server/libjvm.so", "lib/client/libjvm.so",
    };
    for (size_t i = 0; i < sizeof places / sizeof places[0]; i++) {
        snprintf(out, cap, "%s/%s", home, places[i]);
        if (access(out, R_OK) == 0) return 1;
    }
    return 0;
}

// Finds the Java VM library: under `javaHome` (JAVA_HOME) when it is set,
// else beside the `java` found on `path` (PATH), following links to the
// installation it belongs to. Writes its absolute path to `out` and returns
// its length, 0 when there is none.
int64_t ajvmLocate(const char* javaHome, const char* path, uint8_t* out, int64_t cap) {
    char found[PATH_MAX];
    found[0] = 0;
    if (javaHome && javaHome[0]) {
        jvm_under(javaHome, found, sizeof found) || (found[0] = 0);
    } else if (path) {
        const char* at = path;
        while (*at && !found[0]) {
            const char* end = strchr(at, ':');
            size_t n = end ? (size_t)(end - at) : strlen(at);
            char launcher[PATH_MAX], real[PATH_MAX];
            if (n && n + 6 < sizeof launcher) {
                memcpy(launcher, at, n);
                memcpy(launcher + n, "/java", 6);
                if (realpath(launcher, real)) {
                    // <home>/bin/java
                    char* slash = strrchr(real, '/');
                    if (slash) *slash = 0;
                    slash = strrchr(real, '/');
                    if (slash) *slash = 0;
                    jvm_under(real, found, sizeof found) || (found[0] = 0);
                }
            }
            at += n + (end ? 1 : 0);
        }
    }
    size_t n = strlen(found);
    if (n == 0 || (int64_t)n > cap) return 0;
    memcpy(out, found, n);
    return (int64_t)n;
}
