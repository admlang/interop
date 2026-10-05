// The Java half of adm.interop.java. The native side defines this class in
// the VM it starts and hands every request to request(): bytes in, bytes
// out. Compile it with a JDK and keep the class file beside this one:
//
//   javac --release 8 -d . bridge/Bridge.java && mv adm/interop/Bridge.class bridge/
//
// It must stay one class file (no nested or anonymous classes, no lambdas
// that need a second file): the native side defines exactly one class.
package adm.interop;

import java.io.ByteArrayOutputStream;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.lang.ref.PhantomReference;
import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.reflect.Array;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.lang.reflect.Proxy;
import java.lang.reflect.UndeclaredThrowableException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.WeakHashMap;

// An instance stands for something of the ADM program: a host function
// (the handler behind every proxy made for it) or an object Java holds
// without seeing inside it.
public final class Bridge implements InvocationHandler {
    // Requests.
    static final int BIND = 1, CALL = 2, LOAD = 3, GET = 4, SET = 5, GET_INDEX = 6, SET_INDEX = 7, LENGTH = 8,
        INVOKE = 9, CONSTRUCT = 10, TEXT = 11, SAME = 12, EQUALS = 13, VALUE = 15, PROXY = 16,
        FOREIGN = 17, COLLECT = 18, STATS = 19, DEAD = 20, ELEMENTS = 21;
    // The first byte of an answer.
    static final int OK = 0, THROWN = 4, BINDING = 5, CONVERSION = 6;
    // Values on the wire.
    static final int T_NULL = 0, T_BOOL = 1, T_INT = 2, T_FLOAT = 3, T_STRING = 4, T_HANDLE = 5, T_FUNCTION = 6, T_ARRAY = 7;
    // What a handle is, as the ADM side names it.
    static final int K_OBJECT = 0, K_CLASS = 1, K_ARRAY = 2, K_FOREIGN = 3;
    static final int ABSENT = 0xffffffff;
    // A parameter no argument fits.
    static final int NO = -1;

    static final int FUNCTION = 1, OPAQUE = 2;

    // Objects the ADM program holds, by handle minus one.
    static final ArrayList<Object> held = new ArrayList<Object>();
    static final ArrayList<Integer> spare = new ArrayList<Integer>();
    static int live;
    // Methods bound by declarations.
    static final ArrayList<Method> bound = new ArrayList<Method>();
    static final ArrayList<String> boundKinds = new ArrayList<String>();
    // What the ADM program keeps for Java (functions, objects, errors), by
    // the reference that says Java dropped it.
    static final ReferenceQueue<Object> dropped = new ReferenceQueue<Object>();
    static final HashMap<Reference<?>, Long> watched = new HashMap<Reference<?>, Long>();
    // Ids to hand back that never got an object of their own.
    static final ArrayList<Long> credits = new ArrayList<Long>();
    static final HashMap<Long, Reference<Bridge>> handlers = new HashMap<Long, Reference<Bridge>>();
    // ADM errors travelling through Java, by the exception that carries them.
    static final WeakHashMap<Throwable, Long> carried = new WeakHashMap<Throwable, Long>();
    // The exceptions refuse() made, with the answer's first byte.
    static final WeakHashMap<Throwable, Integer> refusals = new WeakHashMap<Throwable, Integer>();
    // The public methods of a class by name, as invoke() found them. A class
    // looked into stays loaded.
    static final HashMap<Class<?>, HashMap<String, Method[]>> methods = new HashMap<Class<?>, HashMap<String, Method[]>>();
    static final Object lock = new Object();

    final int role;
    final long id;

    private Bridge(int role, long id) {
        this.role = role;
        this.id = id;
    }

    // Runs host function fn of the ADM program with packed arguments and
    // returns its packed answer.
    private static native byte[] upcall(long fn, byte[] args);

    // What stops a request: the answer's first byte and its text.
    private static RuntimeException refuse(int status, String message) {
        RuntimeException refusal = new IllegalStateException(message);
        synchronized (lock) {
            refusals.put(refusal, status);
        }
        return refusal;
    }

    public static byte[] request(byte[] in) {
        ByteArrayOutputStream out = new ByteArrayOutputStream(64);
        try {
            ByteBuffer b = ByteBuffer.wrap(in).order(ByteOrder.LITTLE_ENDIAN);
            // In front of every request: the handles the program dropped.
            int gone = b.getInt();
            synchronized (lock) {
                for (int i = 0; i < gone; i++) {
                    int slot = (int) b.getLong() - 1;
                    if (slot < 0 || slot >= held.size() || held.get(slot) == null) continue;
                    held.set(slot, null);
                    spare.add(slot);
                    live--;
                }
            }
            out.write(OK);
            serve(b.get() & 0xff, b, out);
        } catch (Throwable failure) {
            out.reset();
            fail(failure, out);
        }
        return out.toByteArray();
    }

    private static void fail(Throwable failure, ByteArrayOutputStream out) {
        Throwable t = failure;
        while ((t instanceof InvocationTargetException || t instanceof UndeclaredThrowableException) && t.getCause() != null) t = t.getCause();
        long error = 0;
        Integer refused;
        synchronized (lock) {
            refused = refusals.get(t);
        }
        if (refused != null) {
            out.write(refused);
            putString(out, t.getMessage());
            return;
        }
        synchronized (lock) {
            for (Throwable c = t; c != null && error == 0; c = c.getCause() == c ? null : c.getCause()) {
                Long id = carried.get(c);
                if (id != null) error = id;
            }
        }
        StringWriter text = new StringWriter();
        try {
            PrintWriter printer = new PrintWriter(text);
            t.printStackTrace(printer);
            printer.flush();
        } catch (Throwable ignored) {
        }
        String head;
        try {
            head = t.toString();
        } catch (Throwable ignored) {
            head = t.getClass().getName();
        }
        out.write(THROWN);
        putLong(out, error);
        putString(out, head);
        putString(out, text.toString());
        put(out, t);
    }

    private static void serve(int op, ByteBuffer b, ByteArrayOutputStream out) throws Throwable {
        switch (op) {
            case BIND: {
                putLong(out, bind(string(b), string(b), string(b)));
                return;
            }
            case CALL: {
                call(b, out);
                return;
            }
            case LOAD: {
                put(out, find(string(b)));
                return;
            }
            case GET: {
                Object target = value(b);
                String name = string(b);
                Field f = field(target, name);
                if (f == null && target != null && target.getClass().isArray() && name.equals("length")) {
                    put(out, Array.getLength(target));
                    return;
                }
                if (f == null) throw refuse(BINDING, describe(target) + " has no field " + name);
                put(out, f.get(Modifier.isStatic(f.getModifiers()) ? null : target));
                return;
            }
            case SET: {
                Object target = value(b);
                String name = string(b);
                Object v = value(b);
                Field f = field(target, name);
                if (f == null) throw refuse(BINDING, describe(target) + " has no field " + name);
                if (fit(v, f.getType()) == NO) throw refuse(CONVERSION, "field " + name + " of " + describe(target) + " is a " + f.getType().getTypeName() + ", which cannot take " + describe(v));
                f.set(Modifier.isStatic(f.getModifiers()) ? null : target, coerce(v, f.getType()));
                return;
            }
            case GET_INDEX: {
                Object target = value(b);
                long at = b.getLong();
                if (target != null && target.getClass().isArray()) put(out, Array.get(target, index(at, Array.getLength(target))));
                else if (target instanceof List) put(out, ((List<?>) target).get(index(at, ((List<?>) target).size())));
                else throw refuse(CONVERSION, describe(target) + " has no elements by position");
                return;
            }
            case SET_INDEX: {
                Object target = value(b);
                long at = b.getLong();
                Object v = value(b);
                if (target != null && target.getClass().isArray()) {
                    Class<?> element = target.getClass().getComponentType();
                    if (fit(v, element) == NO) throw refuse(CONVERSION, "an element of " + describe(target) + " cannot take " + describe(v));
                    Array.set(target, index(at, Array.getLength(target)), coerce(v, element));
                } else if (target instanceof List) {
                    @SuppressWarnings("unchecked")
                    List<Object> list = (List<Object>) target;
                    list.set(index(at, list.size()), coerce(v, Object.class));
                } else {
                    throw refuse(CONVERSION, describe(target) + " has no elements by position");
                }
                return;
            }
            case LENGTH: {
                Object target = value(b);
                if (target != null && target.getClass().isArray()) put(out, Array.getLength(target));
                else if (target instanceof CharSequence) put(out, ((CharSequence) target).length());
                else if (target instanceof Collection) put(out, ((Collection<?>) target).size());
                else if (target instanceof Map) put(out, ((Map<?, ?>) target).size());
                else throw refuse(CONVERSION, describe(target) + " has no length");
                return;
            }
            case INVOKE: {
                Object target = value(b);
                String name = string(b);
                Object[] args = values(b);
                if (target == null) throw refuse(CONVERSION, "cannot call " + name + " on null");
                put(out, invoke(target, name, args));
                return;
            }
            case CONSTRUCT: {
                Object target = value(b);
                Object[] args = values(b);
                if (!(target instanceof Class)) throw refuse(CONVERSION, describe(target) + " is not a class");
                put(out, construct((Class<?>) target, args));
                return;
            }
            case TEXT: {
                put(out, String.valueOf(value(b)));
                return;
            }
            case SAME: {
                Object a = value(b);
                put(out, a == value(b));
                return;
            }
            case EQUALS: {
                Object a = value(b);
                Object other = value(b);
                put(out, a == null ? other == null : a.equals(other));
                return;
            }
            case VALUE: {
                put(out, value(b));
                return;
            }
            case PROXY: {
                Bridge handler = handler(b.getLong());
                Class<?> type = find(string(b));
                if (!type.isInterface()) throw refuse(BINDING, type.getName() + " is not an interface: an ADM function stands in for an interface");
                put(out, proxy(handler, type));
                return;
            }
            case FOREIGN: {
                Bridge opaque = new Bridge(OPAQUE, b.getLong());
                watch(opaque, opaque.id);
                put(out, opaque);
                return;
            }
            case COLLECT: {
                System.gc();
                return;
            }
            case STATS: {
                Runtime rt = Runtime.getRuntime();
                putLong(out, rt.totalMemory() - rt.freeMemory());
                synchronized (lock) {
                    putLong(out, live);
                }
                return;
            }
            case DEAD: {
                ArrayList<Long> ids = new ArrayList<Long>();
                synchronized (lock) {
                    ids.addAll(credits);
                    credits.clear();
                    for (Reference<?> r = dropped.poll(); r != null; r = dropped.poll()) {
                        Long id = watched.remove(r);
                        if (id == null) continue;
                        ids.add(id);
                        Reference<Bridge> handler = handlers.get(id);
                        if (handler != null && handler.get() == null) handlers.remove(id);
                    }
                }
                putInt(out, ids.size());
                for (Long id : ids) putLong(out, id);
                return;
            }
            case ELEMENTS: {
                Object target = value(b);
                if (target == null || !target.getClass().isArray()) throw refuse(CONVERSION, describe(target) + " is not an array");
                putArray(out, target);
                return;
            }
            default:
                throw refuse(BINDING, "unknown request " + op);
        }
    }

    // ---- reading and writing the wire ----

    private static void putInt(ByteArrayOutputStream out, int v) {
        out.write(v);
        out.write(v >> 8);
        out.write(v >> 16);
        out.write(v >> 24);
    }

    private static void putLong(ByteArrayOutputStream out, long v) {
        putInt(out, (int) v);
        putInt(out, (int) (v >> 32));
    }

    private static void putString(ByteArrayOutputStream out, String s) {
        if (s == null) {
            putInt(out, ABSENT);
            return;
        }
        byte[] text = s.getBytes(StandardCharsets.UTF_8);
        putInt(out, text.length);
        out.write(text, 0, text.length);
    }

    private static String string(ByteBuffer b) {
        int n = b.getInt();
        if (n == ABSENT) return null;
        String s = new String(b.array(), b.position(), n, StandardCharsets.UTF_8);
        b.position(b.position() + n);
        return s;
    }

    // Writes a value: numbers, booleans and strings as they are, anything
    // else as a new handle.
    private static void put(ByteArrayOutputStream out, Object v) {
        if (v == null) {
            out.write(T_NULL);
        } else if (v instanceof Boolean) {
            out.write(T_BOOL);
            out.write((Boolean) v ? 1 : 0);
        } else if (v instanceof Long || v instanceof Integer || v instanceof Short || v instanceof Byte) {
            out.write(T_INT);
            putLong(out, ((Number) v).longValue());
        } else if (v instanceof Double || v instanceof Float) {
            out.write(T_FLOAT);
            putLong(out, Double.doubleToRawLongBits(((Number) v).doubleValue()));
        } else if (v instanceof String || v instanceof Character) {
            out.write(T_STRING);
            putString(out, v.toString());
        } else {
            out.write(T_HANDLE);
            putLong(out, hold(v));
            if (v instanceof Bridge && ((Bridge) v).role == OPAQUE) {
                out.write(K_FOREIGN);
                putLong(out, ((Bridge) v).id);
            } else {
                out.write(v instanceof Class ? K_CLASS : v.getClass().isArray() ? K_ARRAY : K_OBJECT);
            }
        }
    }

    private static long hold(Object v) {
        synchronized (lock) {
            live++;
            if (!spare.isEmpty()) {
                int slot = spare.remove(spare.size() - 1);
                held.set(slot, v);
                return slot + 1;
            }
            held.add(v);
            return held.size();
        }
    }

    // Reads a value: a Long, a Double, a Boolean, a String, the object
    // behind a handle, a new array, or the handler of a host function.
    private static Object value(ByteBuffer b) {
        int tag = b.get() & 0xff;
        switch (tag) {
            case T_NULL:
                return null;
            case T_BOOL:
                return b.get() != 0;
            case T_INT:
                return b.getLong();
            case T_FLOAT:
                return Double.longBitsToDouble(b.getLong());
            case T_STRING:
                return string(b);
            case T_HANDLE: {
                long handle = b.getLong();
                synchronized (lock) {
                    Object v = handle > 0 && handle <= held.size() ? held.get((int) handle - 1) : null;
                    if (v == null) throw refuse(CONVERSION, "the value was released");
                    return v;
                }
            }
            case T_FUNCTION:
                return handler(b.getLong());
            case T_ARRAY: {
                char element = (char) (b.get() & 0xff);
                if (element != 'O') return array(b, element);
                int n = b.getInt();
                Object[] items = new Object[n];
                for (int i = 0; i < n; i++) items[i] = coerce(value(b), Object.class);
                return items;
            }
            default:
                throw refuse(CONVERSION, "unknown value tag " + tag);
        }
    }

    private static Object[] values(ByteBuffer b) {
        Object[] args = new Object[b.getInt()];
        for (int i = 0; i < args.length; i++) args[i] = value(b);
        return args;
    }

    // An array of n packed elements of this kind; null for an absent one.
    private static Object array(ByteBuffer b, char element) {
        int n = b.getInt();
        if (n == ABSENT) return null;
        switch (element) {
            case 'Z': {
                boolean[] a = new boolean[n];
                for (int i = 0; i < n; i++) a[i] = b.get() != 0;
                return a;
            }
            case 'B': {
                byte[] a = new byte[n];
                b.get(a);
                return a;
            }
            case 'S': {
                short[] a = new short[n];
                b.asShortBuffer().get(a);
                b.position(b.position() + 2 * n);
                return a;
            }
            case 'C': {
                char[] a = new char[n];
                b.asCharBuffer().get(a);
                b.position(b.position() + 2 * n);
                return a;
            }
            case 'I': {
                int[] a = new int[n];
                b.asIntBuffer().get(a);
                b.position(b.position() + 4 * n);
                return a;
            }
            case 'J': {
                long[] a = new long[n];
                b.asLongBuffer().get(a);
                b.position(b.position() + 8 * n);
                return a;
            }
            case 'F': {
                float[] a = new float[n];
                b.asFloatBuffer().get(a);
                b.position(b.position() + 4 * n);
                return a;
            }
            case 'D': {
                double[] a = new double[n];
                b.asDoubleBuffer().get(a);
                b.position(b.position() + 8 * n);
                return a;
            }
            case 'T': {
                String[] a = new String[n];
                for (int i = 0; i < n; i++) a[i] = string(b);
                return a;
            }
            default:
                throw refuse(CONVERSION, "unknown element kind " + element);
        }
    }

    // Writes an array with its elements: T_ARRAY, the element kind, the
    // count, the elements.
    private static void putArray(ByteArrayOutputStream out, Object a) {
        int n = Array.getLength(a);
        Class<?> element = a.getClass().getComponentType();
        out.write(T_ARRAY);
        if (!element.isPrimitive()) {
            if (element == String.class) {
                out.write('T');
                putInt(out, n);
                for (int i = 0; i < n; i++) putString(out, ((String[]) a)[i]);
            } else {
                out.write('O');
                putInt(out, n);
                for (int i = 0; i < n; i++) put(out, Array.get(a, i));
            }
            return;
        }
        int width = element == boolean.class || element == byte.class ? 1 : element == short.class || element == char.class ? 2
            : element == int.class || element == float.class ? 4 : 8;
        ByteBuffer packed = ByteBuffer.allocate(n * width).order(ByteOrder.LITTLE_ENDIAN);
        if (element == boolean.class) {
            out.write('Z');
            for (int i = 0; i < n; i++) packed.put((byte) (((boolean[]) a)[i] ? 1 : 0));
        } else if (element == byte.class) {
            out.write('B');
            packed.put((byte[]) a);
        } else if (element == short.class) {
            out.write('S');
            packed.asShortBuffer().put((short[]) a);
        } else if (element == char.class) {
            out.write('C');
            packed.asCharBuffer().put((char[]) a);
        } else if (element == int.class) {
            out.write('I');
            packed.asIntBuffer().put((int[]) a);
        } else if (element == long.class) {
            out.write('J');
            packed.asLongBuffer().put((long[]) a);
        } else if (element == float.class) {
            out.write('F');
            packed.asFloatBuffer().put((float[]) a);
        } else {
            out.write('D');
            packed.asDoubleBuffer().put((double[]) a);
        }
        putInt(out, n);
        out.write(packed.array(), 0, n * width);
    }

    private static int index(long at, int length) {
        if (at < 0 || at >= length) throw refuse(CONVERSION, "index " + at + " is outside 0.." + (length - 1));
        return (int) at;
    }

    // How messages name a value.
    private static String describe(Object v) {
        if (v == null) return "null";
        if (v instanceof Class) return "class " + ((Class<?>) v).getName();
        if (v instanceof Bridge) return ((Bridge) v).role == FUNCTION ? "an ADM function" : "an ADM object";
        if (v instanceof Long) return "an integer";
        if (v instanceof Double) return "a float";
        if (v instanceof Boolean) return "a boolean";
        if (v instanceof String) return "a string";
        return "a " + v.getClass().getTypeName();
    }

    // ---- classes and members ----

    // The class by its name; "java.util.Map.Entry" also names the nested
    // class java.util.Map$Entry, and "int[]" an array class.
    private static Class<?> find(String name) {
        if (name.endsWith("[]")) return Array.newInstance(find(name.substring(0, name.length() - 2)), 0).getClass();
        switch (name) {
            case "boolean": return boolean.class;
            case "byte": return byte.class;
            case "short": return short.class;
            case "char": return char.class;
            case "int": return int.class;
            case "long": return long.class;
            case "float": return float.class;
            case "double": return double.class;
            default:
        }
        ClassLoader loader = ClassLoader.getSystemClassLoader();
        String tried = name;
        for (;;) {
            try {
                return Class.forName(tried, true, loader);
            } catch (ClassNotFoundException | NoClassDefFoundError missing) {
                int dot = tried.lastIndexOf('.');
                if (dot < 0) throw refuse(BINDING, "class " + name + " was not found on the class path");
                tried = tried.substring(0, dot) + '$' + tried.substring(dot + 1);
            }
        }
    }

    // The class whose members a value gives: the class itself for a class
    // value, else the value's class.
    private static Field field(Object target, String name) {
        if (target == null) throw refuse(CONVERSION, "null has no field " + name);
        Class<?> type = target instanceof Class ? (Class<?>) target : target.getClass();
        try {
            Field f = type.getField(name);
            if (target instanceof Class && !Modifier.isStatic(f.getModifiers())) return null;
            return f;
        } catch (NoSuchFieldException missing) {
            return null;
        }
    }

    // The same method where it can be called from here: a public method of
    // a class that is not public is reached through the public class or
    // interface that declares it.
    private static Method reachable(Method m, Class<?> from) {
        if (Modifier.isPublic(m.getDeclaringClass().getModifiers())) return m;
        for (Class<?> c = from; c != null; c = c.getSuperclass()) {
            Method found = declared(c, m);
            if (found != null) return found;
            for (Class<?> i : c.getInterfaces()) {
                found = reachable(m, i);
                if (found != null && Modifier.isPublic(found.getDeclaringClass().getModifiers())) return found;
            }
        }
        return from.isInterface() ? declared(from, m) : m;
    }

    private static Method declared(Class<?> c, Method like) {
        if (!Modifier.isPublic(c.getModifiers())) return null;
        try {
            return c.getMethod(like.getName(), like.getParameterTypes());
        } catch (NoSuchMethodException missing) {
            return null;
        }
    }

    // How well an argument fits a parameter: 0 is the parameter's own
    // type, larger is a wider conversion, NO is no conversion that keeps
    // the value.
    private static int fit(Object a, Class<?> p) {
        if (a == null) return p.isPrimitive() ? NO : 1;
        if (a instanceof Long) {
            long v = (Long) a;
            if (p == long.class) return 0;
            if (p == int.class) return v == (int) v ? 1 : NO;
            if (p == short.class) return v == (short) v ? 2 : NO;
            if (p == byte.class) return v == (byte) v ? 2 : NO;
            if (p == double.class) return Math.abs(v) <= 1L << 53 ? 3 : NO;
            if (p == float.class) return Math.abs(v) <= 1L << 24 ? 4 : NO;
            if (p == char.class || p == Character.class) return v >= 0 && v <= 0xffff ? 8 : NO;
            if (p == Long.class) return 5;
            if (p == Integer.class) return v == (int) v ? 6 : NO;
            if (p == Short.class) return v == (short) v ? 7 : NO;
            if (p == Byte.class) return v == (byte) v ? 7 : NO;
            if (p == Double.class) return Math.abs(v) <= 1L << 53 ? 8 : NO;
            if (p == Float.class) return Math.abs(v) <= 1L << 24 ? 8 : NO;
            return p.isAssignableFrom(Long.class) ? 9 : NO;
        }
        if (a instanceof Double) {
            double v = (Double) a;
            boolean single = Double.isNaN(v) || (double) (float) v == v;
            if (p == double.class) return 0;
            if (p == float.class) return single ? 1 : NO;
            if (p == Double.class) return 5;
            if (p == Float.class) return single ? 6 : NO;
            return p.isAssignableFrom(Double.class) ? 9 : NO;
        }
        if (a instanceof Boolean) return p == boolean.class ? 0 : p == Boolean.class ? 5 : p.isAssignableFrom(Boolean.class) ? 9 : NO;
        if (a instanceof String) {
            if (p == String.class) return 0;
            if (p == Object.class) return 9;
            if (p.isAssignableFrom(String.class)) return 1;
            return (p == char.class || p == Character.class) && ((String) a).length() == 1 ? 5 : NO;
        }
        if (a instanceof Bridge && ((Bridge) a).role == FUNCTION) return p.isInterface() ? 2 : NO;
        if (p.isPrimitive()) return NO;
        return p == a.getClass() ? 0 : p == Object.class ? 9 : p.isInstance(a) ? 1 : NO;
    }

    // The argument as the parameter takes it; fit() said it can.
    private static Object coerce(Object a, Class<?> p) {
        if (a instanceof Long) {
            long v = (Long) a;
            if (p == long.class || p == Long.class) return a;
            if (p == int.class || p == Integer.class) return (int) v;
            if (p == short.class || p == Short.class) return (short) v;
            if (p == byte.class || p == Byte.class) return (byte) v;
            if (p == double.class || p == Double.class) return (double) v;
            if (p == float.class || p == Float.class) return (float) v;
            if (p == char.class || p == Character.class) return (char) v;
            // Where any number or any object is taken, an int when it fits.
            return v == (int) v ? (Object) (int) v : a;
        }
        if (a instanceof Double) return p == float.class || p == Float.class ? (Object) (float) (double) (Double) a : a;
        if (a instanceof String) return p == char.class || p == Character.class ? (Object) ((String) a).charAt(0) : a;
        if (a instanceof Bridge && ((Bridge) a).role == FUNCTION && p.isInterface()) return proxy((Bridge) a, p);
        return a;
    }

    // Fits arguments to parameters, the last ones into a variable-arity
    // parameter when `spread`. Returns the cost, NO when they do not fit.
    private static int cost(Class<?>[] params, Object[] args, boolean spread) {
        int fixed = spread ? params.length - 1 : params.length;
        if (spread ? args.length < fixed : args.length != fixed) return NO;
        int total = spread ? 20 : 0;
        for (int i = 0; i < args.length; i++) {
            int c = fit(args[i], i < fixed ? params[i] : params[fixed].getComponentType());
            if (c == NO) return NO;
            total += c;
        }
        return total;
    }

    private static Object[] arrange(Class<?>[] params, Object[] args, boolean spread) {
        Object[] out = new Object[params.length];
        int fixed = spread ? params.length - 1 : params.length;
        for (int i = 0; i < fixed; i++) out[i] = coerce(args[i], params[i]);
        if (spread) {
            Class<?> element = params[fixed].getComponentType();
            Object rest = Array.newInstance(element, args.length - fixed);
            for (int i = fixed; i < args.length; i++) Array.set(rest, i - fixed, coerce(args[i], element));
            out[fixed] = rest;
        }
        return out;
    }

    // Whether every parameter of a is at least as narrow as b's: Java's
    // rule for the more specific of two methods that both apply.
    private static boolean narrower(Class<?>[] a, Class<?>[] b) {
        if (a.length != b.length) return false;
        for (int i = 0; i < a.length; i++) {
            if (a[i] != b[i] && !b[i].isAssignableFrom(a[i]) && !(a[i].isPrimitive() && b[i].isPrimitive() && rank(a[i]) < rank(b[i]))) return false;
        }
        return true;
    }

    private static int rank(Class<?> primitive) {
        return primitive == byte.class ? 0 : primitive == short.class || primitive == char.class ? 1 : primitive == int.class ? 2
            : primitive == long.class ? 3 : primitive == float.class ? 4 : 5;
    }

    private static String spell(Object[] args) {
        StringBuilder s = new StringBuilder();
        for (int i = 0; i < args.length; i++) s.append(i == 0 ? "" : ", ").append(describe(args[i]));
        return s.toString();
    }

    // Calls the method `name` that takes these arguments: of the class for
    // a class value (a static method, else a method of java.lang.Class),
    // else of the object.
    private static Object invoke(Object target, String name, Object[] args) throws Throwable {
        boolean statics = target instanceof Class;
        Class<?> type = statics ? (Class<?>) target : target.getClass();
        for (int round = 0; round < 2; round++) {
            Method best = null;
            boolean bestSpread = false;
            int bestCost = Integer.MAX_VALUE;
            boolean named = false;
            for (Method m : methodsNamed(type, name)) {
                if (statics && !Modifier.isStatic(m.getModifiers())) continue;
                named = true;
                for (int form = 0; form < (m.isVarArgs() ? 2 : 1); form++) {
                    int c = cost(m.getParameterTypes(), args, form == 1);
                    if (c == NO) continue;
                    if (c < bestCost || c == bestCost && best != null && narrower(m.getParameterTypes(), best.getParameterTypes())) {
                        best = m;
                        bestSpread = form == 1;
                        bestCost = c;
                    }
                }
            }
            if (best != null) {
                Method m = reachable(best, type);
                Object[] arranged = arrange(best.getParameterTypes(), args, bestSpread);
                Object self = Modifier.isStatic(m.getModifiers()) ? null : target;
                try {
                    return m.invoke(self, arranged);
                } catch (IllegalAccessException closed) {
                    m.setAccessible(true);
                    return m.invoke(self, arranged);
                }
            }
            if (round == 1) break;
            if (!statics) {
                if (named) throw refuse(BINDING, type.getTypeName() + " has no method " + name + " that takes (" + spell(args) + ")");
                throw refuse(BINDING, type.getTypeName() + " has no method " + name);
            }
            if (named) throw refuse(BINDING, "class " + type.getName() + " has no static method " + name + " that takes (" + spell(args) + ")");
            // A class value is also an object: getName(), isInterface().
            statics = false;
            type = Class.class;
        }
        throw refuse(BINDING, "class " + ((Class<?>) target).getName() + " has no static method " + name);
    }

    // The public methods `name` of a class. Class.getMethods() copies every
    // method of the class on each call, so the answer is kept.
    private static Method[] methodsNamed(Class<?> type, String name) {
        synchronized (lock) {
            HashMap<String, Method[]> known = methods.get(type);
            if (known == null) {
                known = new HashMap<String, Method[]>();
                methods.put(type, known);
            }
            Method[] found = known.get(name);
            if (found == null) {
                ArrayList<Method> all = new ArrayList<Method>();
                for (Method m : type.getMethods()) {
                    if (m.getName().equals(name)) all.add(m);
                }
                found = all.toArray(new Method[0]);
                known.put(name, found);
            }
            return found;
        }
    }

    private static Object construct(Class<?> type, Object[] args) throws Throwable {
        if (type.isArray()) {
            // new int[n].
            if (args.length != 1 || !(args[0] instanceof Long) || (Long) args[0] < 0 || (Long) args[0] > Integer.MAX_VALUE) {
                throw refuse(BINDING, "an array class is constructed with one argument, its length: " + type.getTypeName() + " got (" + spell(args) + ")");
            }
            return Array.newInstance(type.getComponentType(), (int) (long) (Long) args[0]);
        }
        Constructor<?> best = null;
        boolean bestSpread = false;
        int bestCost = Integer.MAX_VALUE;
        for (Constructor<?> c : type.getConstructors()) {
            for (int form = 0; form < (c.isVarArgs() ? 2 : 1); form++) {
                int n = cost(c.getParameterTypes(), args, form == 1);
                if (n == NO) continue;
                if (n < bestCost || n == bestCost && best != null && narrower(c.getParameterTypes(), best.getParameterTypes())) {
                    best = c;
                    bestSpread = form == 1;
                    bestCost = n;
                }
            }
        }
        if (best == null) throw refuse(BINDING, "class " + type.getName() + " has no public constructor that takes (" + spell(args) + ")");
        return best.newInstance(arrange(best.getParameterTypes(), args, bestSpread));
    }

    // ---- declarations: a static method by its exact signature ----

    private static Class<?> kind(String signature, int[] at) {
        char c = signature.charAt(at[0]++);
        switch (c) {
            case 'Z': return boolean.class;
            case 'B': return byte.class;
            case 'S': return short.class;
            case 'I': return int.class;
            case 'J': return long.class;
            case 'F': return float.class;
            case 'D': return double.class;
            case 'V': return void.class;
            case '[': return Array.newInstance(kind(signature, at), 0).getClass();
            default: {
                int end = signature.indexOf(';', at[0]);
                String name = signature.substring(at[0], end).replace('/', '.');
                at[0] = end + 1;
                return name.equals("java.lang.String") ? String.class : find(name);
            }
        }
    }

    // Binds the static method of a class with this JNI signature and
    // returns its number.
    private static long bind(String className, String name, String signature) {
        Class<?> type = find(className);
        ArrayList<Class<?>> params = new ArrayList<Class<?>>();
        StringBuilder kinds = new StringBuilder();
        int[] at = {1};
        while (signature.charAt(at[0]) != ')') {
            int from = at[0];
            params.add(kind(signature, at));
            kinds.append(signature.charAt(from) == '[' ? Character.toLowerCase(signature.charAt(from + 1)) : signature.charAt(from));
        }
        at[0]++;
        int from = at[0];
        Class<?> result = kind(signature, at);
        kinds.append(signature.charAt(from) == '[' ? Character.toLowerCase(signature.charAt(from + 1)) : signature.charAt(from));
        Class<?>[] types = params.toArray(new Class<?>[0]);
        for (Class<?> c = type; c != null; c = c.getSuperclass()) {
            Method m;
            try {
                m = c.getDeclaredMethod(name, types);
            } catch (NoSuchMethodException missing) {
                continue;
            }
            if (!Modifier.isStatic(m.getModifiers()) || m.getReturnType() != result) continue;
            try {
                m.setAccessible(true);
            } catch (RuntimeException closed) {
                // Stays as reachable as it is.
            }
            synchronized (lock) {
                bound.add(m);
                boundKinds.add(kinds.toString());
                return bound.size() - 1;
            }
        }
        StringBuilder spelled = new StringBuilder();
        for (Class<?> p : types) spelled.append(spelled.length() == 0 ? "" : ", ").append(p.getSimpleName());
        throw refuse(BINDING, "class " + className + " has no method `static " + result.getSimpleName() + " " + name + "(" + spelled + ")`");
    }

    // Calls a bound method with arguments packed by its signature: a
    // number as 8 bytes, a string as its length and bytes, an array as its
    // count and elements. `kinds` holds one letter per parameter and one
    // for the result, lower case for an array of that kind.
    private static void call(ByteBuffer b, ByteArrayOutputStream out) throws Throwable {
        Method m;
        String kinds;
        synchronized (lock) {
            int number = b.getInt();
            m = bound.get(number);
            kinds = boundKinds.get(number);
        }
        Object[] args = new Object[kinds.length() - 1];
        for (int i = 0; i < args.length; i++) {
            char k = kinds.charAt(i);
            if (k == 'L') {
                args[i] = string(b);
            } else if (Character.isLowerCase(k)) {
                args[i] = array(b, k == 'l' ? 'T' : Character.toUpperCase(k));
            } else {
                long bits = b.getLong();
                switch (k) {
                    case 'Z': args[i] = bits != 0; break;
                    case 'B': args[i] = (byte) bits; break;
                    case 'S': args[i] = (short) bits; break;
                    case 'I': args[i] = (int) bits; break;
                    case 'F': args[i] = Float.intBitsToFloat((int) bits); break;
                    case 'D': args[i] = Double.longBitsToDouble(bits); break;
                    default: args[i] = bits;
                }
            }
        }
        Object result = m.invoke(null, args);
        char k = kinds.charAt(kinds.length() - 1);
        if (result == null) {
            out.write(T_NULL);
        } else if (k == 'L') {
            out.write(T_STRING);
            putString(out, (String) result);
        } else if (Character.isLowerCase(k)) {
            putArray(out, result);
        } else {
            long bits;
            switch (k) {
                case 'Z': bits = (Boolean) result ? 1 : 0; break;
                case 'F': bits = Float.floatToRawIntBits((Float) result) & 0xffffffffL; break;
                case 'D': bits = Double.doubleToRawLongBits((Double) result); break;
                default: bits = ((Number) result).longValue();
            }
            out.write(T_INT);
            putLong(out, bits);
        }
    }

    // ---- what the ADM program keeps for Java ----

    // Tells the ADM side, at its next DEAD request, when v is collected.
    private static void watch(Object v, long id) {
        synchronized (lock) {
            watched.put(new PhantomReference<Object>(v, dropped), id);
        }
    }

    // The handler of a host function: one while any proxy uses it. Every
    // call hands one count of the function back to the ADM side in time:
    // when the handler made here is collected, or at once when there
    // already is one.
    private static Bridge handler(long fn) {
        synchronized (lock) {
            Reference<Bridge> known = handlers.get(fn);
            Bridge h = known == null ? null : known.get();
            if (h != null) {
                credits.add(fn);
                return h;
            }
            h = new Bridge(FUNCTION, fn);
            handlers.put(fn, new java.lang.ref.WeakReference<Bridge>(h));
            watched.put(new PhantomReference<Object>(h, dropped), fn);
            return h;
        }
    }

    private static Object proxy(Bridge handler, Class<?> type) {
        ClassLoader loader = type.getClassLoader();
        return Proxy.newProxyInstance(loader == null ? ClassLoader.getSystemClassLoader() : loader, new Class<?>[] {type}, handler);
    }

    // A call of an interface method on a proxy: runs the host function
    // with the call's arguments.
    @Override
    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
        if (method.getDeclaringClass() == Object.class) {
            if (method.getName().equals("equals")) return proxy == args[0];
            if (method.getName().equals("hashCode")) return System.identityHashCode(proxy);
            return "ADM function " + id;
        }
        if (method.isDefault()) {
            // InvocationHandler.invokeDefault, Java 16 and later.
            Method invokeDefault;
            try {
                invokeDefault = InvocationHandler.class.getMethod("invokeDefault", Object.class, Method.class, Object[].class);
            } catch (NoSuchMethodException old) {
                throw new UnsupportedOperationException("an ADM function cannot run the default method " + method.getName() + " before Java 16");
            }
            try {
                return invokeDefault.invoke(null, proxy, method, args);
            } catch (InvocationTargetException thrown) {
                throw thrown.getCause();
            }
        }
        ByteArrayOutputStream packed = new ByteArrayOutputStream(64);
        int n = args == null ? 0 : args.length;
        putInt(packed, n);
        for (int i = 0; i < n; i++) put(packed, args[i]);
        ByteBuffer answer = ByteBuffer.wrap(upcall(id, packed.toByteArray())).order(ByteOrder.LITTLE_ENDIAN);
        int status = answer.get();
        if (status == 2) {
            // A Java exception the function let through.
            Object thrown = value(answer);
            throw thrown instanceof Throwable ? (Throwable) thrown : new RuntimeException(String.valueOf(thrown));
        }
        if (status != 0) {
            long error = answer.getLong();
            RuntimeException failure = new RuntimeException(string(answer));
            if (error != 0) {
                synchronized (lock) {
                    carried.put(failure, error);
                    watched.put(new PhantomReference<Object>(failure, dropped), error);
                }
            }
            throw failure;
        }
        Object result = value(answer);
        Class<?> wanted = method.getReturnType();
        if (wanted == void.class) return null;
        if (fit(result, wanted) == NO) throw new ClassCastException("the ADM function returned " + describe(result) + " where " + method.getName() + " returns " + wanted.getTypeName());
        return coerce(result, wanted);
    }
}
