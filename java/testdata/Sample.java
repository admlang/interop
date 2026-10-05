// The class java_test.adm calls. Compile it with a JDK and keep the class
// file beside it:
//
//   javac --release 8 testdata/Sample.java
public class Sample {
    public static int add(int a, int b) { return a + b; }
    public static long wide(long a, byte b, short c) { return a + b + c; }
    public static boolean both(boolean a, boolean b) { return a && b; }
    public static double half(double v, float f) { return v / 2 + f; }
    public static float single(float v) { return v * 1.5f; }
    public static byte lowest(byte[] values) { byte m = Byte.MAX_VALUE; for (byte v : values) if (v < m) m = v; return m; }
    public static String greet(String name) { return "hello, " + name; }
    public static int length(String text) { return text.codePointCount(0, text.length()); }
    public static void nothing() { }
    public static String missing(boolean give) { return give ? "here" : null; }
    public static String missingText(boolean give) { return missing(give); }
    public static double sum(double[] values) { double s = 0; for (double v : values) s += v; return s; }
    public static int[] squares(int n) { int[] out = new int[n]; for (int i = 0; i < n; i++) out[i] = i * i; return out; }
    public static long[] doubled(long[] values) { long[] out = new long[values.length]; for (int i = 0; i < values.length; i++) out[i] = values[i] * 2; return out; }
    public static byte[] reversed(byte[] data) { byte[] out = new byte[data.length]; for (int i = 0; i < data.length; i++) out[i] = data[data.length - 1 - i]; return out; }
    public static boolean[] flipped(boolean[] flags) { boolean[] out = new boolean[flags.length]; for (int i = 0; i < flags.length; i++) out[i] = !flags[i]; return out; }
    public static short[] shorts(short[] v) { short[] out = v.clone(); for (int i = 0; i < out.length; i++) out[i] = (short) -out[i]; return out; }
    public static float[] scaled(float[] v, float by) { float[] out = new float[v.length]; for (int i = 0; i < v.length; i++) out[i] = v[i] * by; return out; }
    public static String[] upper(String[] words) { String[] out = new String[words.length]; for (int i = 0; i < words.length; i++) out[i] = words[i].toUpperCase(); return out; }
    public static String joined(String[] words, String with) { return String.join(with, words); }
    public static int refuse(String message) { throw new IllegalStateException(message); }
    public static long fib(int n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }
    public static String property(String name) { return System.getProperty(name); }

    // For the tests of values: fields, constructors, overloads, callbacks.
    public static int total = 7;
    public int count;
    public String label;
    public Sample() { this.label = "plain"; }
    public Sample(String label, int count) { this.label = label; this.count = count; }
    public int bump(int by) { count += by; return count; }
    public String describe(long v) { return "long " + v; }
    public String describe(double v) { return "double " + v; }
    public String describe(String v) { return "string " + v; }
    public String describe(Object v) { return "object " + v; }
    public static String joinAll(String separator, Object... parts) {
        StringBuilder out = new StringBuilder();
        for (Object part : parts) out.append(out.length() == 0 ? "" : separator).append(part);
        return out.toString();
    }
    public static Object echo(Object v) { return v; }
    public static String typeOf(Object v) { return v == null ? "null" : v.getClass().getName(); }
    public static float narrow(float v) { return v; }
    public static Object apply(java.util.function.Function<Object, Object> f, Object v) { return f.apply(v); }
    public static int twice(java.util.function.IntUnaryOperator f, int v) { return f.applyAsInt(f.applyAsInt(v)); }
    // Calls c on a thread of its own and waits for it.
    public static Object later(java.util.concurrent.Callable<Object> c) throws Exception {
        java.util.concurrent.FutureTask<Object> task = new java.util.concurrent.FutureTask<Object>(c);
        new Thread(task).start();
        try {
            return task.get();
        } catch (java.util.concurrent.ExecutionException failed) {
            throw (Exception) failed.getCause();
        }
    }
}
