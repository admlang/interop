// The type dotnet_test.adm calls.
using System;
using System.Linq;

namespace Acme
{
    public static class Sample
    {
        public static int Add(int a, int b) => unchecked(a + b);
        public static long Wide(long a, sbyte b, short c) => a + b + c;
        public static ulong Unsigned(ulong a, uint b, ushort c, byte d) => a + b + c + d;
        public static bool Both(bool a, bool b) => a && b;
        public static double Half(double v, float f) => v / 2 + f;
        public static float Single(float v) => v * 1.5f;
        public static string Greet(string name) => "hello, " + name;
        public static int Length(string text) => new System.Globalization.StringInfo(text).LengthInTextElements;
        public static void Nothing() { }
        public static string? Missing(bool give) => give ? "here" : null;
        public static string? MissingText(bool give) => Missing(give);
        public static double Sum(double[] values) => values.Sum();
        public static int[] Squares(int n) => Enumerable.Range(0, n).Select(i => i * i).ToArray();
        public static long[] Doubled(long[] values) => values.Select(v => v * 2).ToArray();
        public static byte[] Reversed(byte[] data) => data.Reverse().ToArray();
        public static bool[] Flipped(bool[] flags) => flags.Select(f => !f).ToArray();
        public static ushort[] Words(ushort[] v) => v.Select(x => (ushort)(x + 1)).ToArray();
        public static float[] Scaled(float[] v, float by) => v.Select(x => x * by).ToArray();
        public static string[] Upper(string[] words) => words.Select(w => w.ToUpperInvariant()).ToArray();
        public static string Joined(string[] words, string with) => string.Join(with, words);
        public static int Refuse(string message) => throw new InvalidOperationException(message);
        public static long Fib(int n) => n < 2 ? n : Fib(n - 1) + Fib(n - 2);
        // A lower-case name, as a declaration spells it.
        public static int exact(int v) => v + 1;

        // For the tests of values.
        public static string JoinAll(string separator, params object[] parts) => string.Join(separator, parts);
        public static object? Echo(object? v) => v;
        public static string TypeOf(object? v) => v == null ? "null" : v.GetType().FullName!;
        public static float Narrow(float v) => v;
        public static object? Apply(Func<object?, object?> f, object? v) => f(v);
        public static int Twice(Func<int, int> f, int v) => f(f(v));
        // Calls c on a thread of the pool and waits for it.
        public static object? Later(Func<object?> c) => System.Threading.Tasks.Task.Run(c).GetAwaiter().GetResult();
        public static int Order(System.Collections.Generic.IComparer<string> c, string a, string b) => c.Compare(a, b);
        public static string Name(Shade shade) => shade.ToString();
    }

    public enum Shade { Light, Dark }

    public class Counter
    {
        public static int Total = 7;
        public int Count;
        public string? Label { get; set; }
        public Counter() { Label = "plain"; }
        public Counter(string label, int count) { Label = label; Count = count; }
        public int Bump(int by = 1) { Count += by; return Count; }
        public string Describe(long v) => "long " + v;
        public string Describe(double v) => "double " + v.ToString(System.Globalization.CultureInfo.InvariantCulture);
        public string Describe(string? v) => "string " + v;
        public string Describe(object? v) => "object " + v;
    }
}
