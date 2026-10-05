// The .NET half of adm.interop.dotnet. The native side loads this assembly
// and hands every request to Bridge.Entry: bytes in, bytes out. The wire
// format is the one of adm.interop.java's bridge.
using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Linq.Expressions;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.ExceptionServices;
using System.Runtime.InteropServices;
using System.Text;

namespace Adm
{
    // A host function of the ADM program: what every delegate and proxy made
    // for it holds. The ADM side hears when the last one is collected.
    sealed class Host
    {
        public readonly long Id;
        public Host(long id) { Id = id; }
        ~Host() { Bridge.Dropped.Enqueue(Id); }
    }

    // An ADM object .NET holds without seeing inside it.
    sealed class Opaque
    {
        public readonly long Id;
        public Opaque(long id) { Id = id; }
        ~Opaque() { Bridge.Dropped.Enqueue(Id); }
        public override string ToString() => "ADM object " + Id;
    }

    // The ADM error a host function failed with, travelling through .NET.
    sealed class HostException : Exception
    {
        public readonly long Error;
        public HostException(string message, long error) : base(message) { Error = error; }
        ~HostException() { if (Error != 0) Bridge.Dropped.Enqueue(Error); }
    }

    // What stops a request: the answer's first byte and its text.
    sealed class Refusal : Exception
    {
        public readonly int Status;
        public Refusal(int status, string message) : base(message) { Status = status; }
    }

    // A host function behind an interface: every method runs it.
    public class HostProxy : DispatchProxy
    {
        internal Host? Host;

        protected override object? Invoke(MethodInfo? method, object?[]? args) =>
            Bridge.Upcall(Host!, args ?? Array.Empty<object?>(), method!.ReturnType, method.Name);
    }

    sealed class Reader
    {
        readonly byte[] data;
        int at;
        public Reader(byte[] data) { this.data = data; }
        public int Byte() => data[at++];
        public int Int() { at += 4; return BitConverter.ToInt32(data, at - 4); }
        public long Long() { at += 8; return BitConverter.ToInt64(data, at - 8); }
        public string? String()
        {
            uint n = (uint)Int();
            if (n == Bridge.Absent) return null;
            at += (int)n;
            return Encoding.UTF8.GetString(data, at - (int)n, (int)n);
        }
        // Copies n bytes into a primitive array.
        public void Fill(Array into, int n) { Buffer.BlockCopy(data, at, into, 0, n); at += n; }
    }

    sealed class Writer
    {
        readonly MemoryStream stream = new MemoryStream(64);
        public void Byte(int v) => stream.WriteByte((byte)v);
        public void Int(int v) => stream.Write(BitConverter.GetBytes(v));
        public void Long(long v) => stream.Write(BitConverter.GetBytes(v));
        public void Bytes(byte[] v) => stream.Write(v);
        public void String(string? s)
        {
            if (s == null) { Int(unchecked((int)Bridge.Absent)); return; }
            byte[] text = Encoding.UTF8.GetBytes(s);
            Int(text.Length);
            stream.Write(text);
        }
        public void Reset() => stream.SetLength(0);
        public byte[] ToArray() => stream.ToArray();
    }

    public static unsafe class Bridge
    {
        // What the native side hands over (bridge_request in adm_dotnet.c).
        [StructLayout(LayoutKind.Sequential)]
        struct Request
        {
            public byte* Data;
            public long Size;
            public byte** Out;
            public long* OutSize;
            public delegate* unmanaged[Cdecl]<long, byte*, long, byte**, long*, void> Upcall;
        }

        // Requests.
        const int FOLDERS = 0, BIND = 1, CALL = 2, LOAD = 3, GET = 4, SET = 5, GET_INDEX = 6, SET_INDEX = 7, LENGTH = 8,
            INVOKE = 9, CONSTRUCT = 10, TEXT = 11, SAME = 12, EQUALS = 13, VALUE = 15, PROXY = 16,
            FOREIGN = 17, COLLECT = 18, STATS = 19, DEAD = 20, ELEMENTS = 21;
        // The first byte of an answer.
        const int OK = 0, THROWN = 4, BINDING = 5, CONVERSION = 6;
        // Values on the wire.
        const int T_NULL = 0, T_BOOL = 1, T_INT = 2, T_FLOAT = 3, T_STRING = 4, T_HANDLE = 5, T_FUNCTION = 6, T_ARRAY = 7;
        // What a handle is, as the ADM side names it.
        const int K_OBJECT = 0, K_TYPE = 1, K_ARRAY = 2, K_FOREIGN = 3;
        internal const uint Absent = 0xffffffff;
        // A parameter no argument fits.
        const int NO = -1;
        const BindingFlags Members = BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static | BindingFlags.FlattenHierarchy;

        sealed class Bound
        {
            public MethodInfo Method = null!;
            public string[] Kinds = Array.Empty<string>();
            public string Result = "V";
        }

        // Objects the ADM program holds, by handle minus one.
        static readonly List<object?> held = new();
        static readonly Stack<int> spare = new();
        static int live;
        static readonly List<Bound> bound = new();
        static readonly List<string> folders = new();
        static bool resolving;
        // Ids of what the ADM program keeps for .NET and .NET dropped.
        internal static readonly ConcurrentQueue<long> Dropped = new();
        static readonly Dictionary<long, WeakReference<Host>> handlers = new();
        // The public methods of a type by name, as Invoke found them.
        static readonly ConditionalWeakTable<Type, Dictionary<string, MethodInfo[]>> methods = new();
        // The parameters of a method or constructor an overload was chosen
        // among, and whether the last one is a `params` array.
        static readonly ConditionalWeakTable<MethodBase, Shape> shapes = new();
        static readonly object gate = new();
        static delegate* unmanaged[Cdecl]<long, byte*, long, byte**, long*, void> upcall;

        // The one entry point (the hosting API's default signature).
        public static int Entry(IntPtr args, int size)
        {
            Request* r = (Request*)args;
            upcall = r->Upcall;
            Writer w = new();
            try
            {
                byte[] data = new byte[r->Size];
                Marshal.Copy((IntPtr)r->Data, data, 0, data.Length);
                Reader b = new(data);
                // In front of every request: the handles the program dropped.
                int gone = b.Int();
                lock (gate)
                {
                    for (int i = 0; i < gone; i++)
                    {
                        int slot = (int)b.Long() - 1;
                        if (slot < 0 || slot >= held.Count || held[slot] == null) continue;
                        held[slot] = null;
                        spare.Push(slot);
                        live--;
                    }
                }
                w.Byte(OK);
                Serve(b.Byte(), b, w);
            }
            catch (Exception failure)
            {
                w.Reset();
                Fail(failure, w);
            }
            byte[] answer = w.ToArray();
            byte* p = (byte*)NativeMemory.Alloc((nuint)Math.Max(answer.Length, 1));
            answer.CopyTo(new Span<byte>(p, answer.Length));
            *r->Out = p;
            *r->OutSize = answer.Length;
            return 0;
        }

        static void Fail(Exception failure, Writer w)
        {
            Exception t = failure;
            while (t is TargetInvocationException && t.InnerException != null) t = t.InnerException;
            if (t is Refusal refused)
            {
                w.Byte(refused.Status);
                w.String(refused.Message);
                return;
            }
            long error = 0;
            for (Exception? c = t; c != null && error == 0; c = c.InnerException)
            {
                if (c is HostException carried) error = carried.Error;
            }
            w.Byte(THROWN);
            w.Long(error);
            w.String(t.GetType().FullName + ": " + t.Message);
            w.String(t.ToString());
            Put(w, t);
        }

        static void Serve(int op, Reader b, Writer w)
        {
            switch (op)
            {
                case FOLDERS:
                    Folders(b.String()!);
                    return;
                case BIND:
                    w.Long(Bind(b.String()!));
                    return;
                case CALL:
                    Call(b, w);
                    return;
                case LOAD:
                    Put(w, Find(b.String()!));
                    return;
                case GET:
                {
                    object? target = Value(b);
                    string name = b.String()!;
                    if (target == null) throw new Refusal(CONVERSION, "null has no member " + name);
                    foreach (Type type in Faces(target))
                    {
                        bool statics = target is Type && type == (Type)target;
                        FieldInfo? f = type.GetField(name, Members);
                        if (f != null && (f.IsStatic || !statics)) { Put(w, f.GetValue(f.IsStatic ? null : target)); return; }
                        PropertyInfo? p = Property(type, name);
                        MethodInfo? getter = p?.GetGetMethod();
                        if (getter != null && (getter.IsStatic || !statics)) { Put(w, getter.Invoke(getter.IsStatic ? null : target, null)); return; }
                    }
                    throw new Refusal(BINDING, Describe(target) + " has no field or property " + name);
                }
                case SET:
                {
                    object? target = Value(b);
                    string name = b.String()!;
                    object? v = Value(b);
                    if (target == null) throw new Refusal(CONVERSION, "null has no member " + name);
                    foreach (Type type in Faces(target))
                    {
                        bool statics = target is Type && type == (Type)target;
                        FieldInfo? f = type.GetField(name, Members);
                        if (f != null && (f.IsStatic || !statics))
                        {
                            if (Fit(v, f.FieldType) == NO) throw new Refusal(CONVERSION, "field " + name + " of " + Describe(target) + " is a " + Spell(f.FieldType) + ", which cannot take " + Describe(v));
                            f.SetValue(f.IsStatic ? null : target, Coerce(v, f.FieldType));
                            return;
                        }
                        PropertyInfo? p = Property(type, name);
                        MethodInfo? setter = p?.GetSetMethod();
                        if (setter != null && (setter.IsStatic || !statics))
                        {
                            if (Fit(v, p!.PropertyType) == NO) throw new Refusal(CONVERSION, "property " + name + " of " + Describe(target) + " is a " + Spell(p.PropertyType) + ", which cannot take " + Describe(v));
                            setter.Invoke(setter.IsStatic ? null : target, new[] { Coerce(v, p.PropertyType) });
                            return;
                        }
                    }
                    throw new Refusal(BINDING, Describe(target) + " has no field or property " + name + " that can be set");
                }
                case GET_INDEX:
                {
                    object? target = Value(b);
                    long at = b.Long();
                    if (target is Array array && array.Rank == 1) { Put(w, array.GetValue(Index(at, array.Length))); return; }
                    PropertyInfo? indexer = Indexer(target);
                    if (indexer?.GetGetMethod() == null) throw new Refusal(CONVERSION, Describe(target) + " has no elements by position");
                    Put(w, indexer.GetValue(target, new object[] { checked((int)at) }));
                    return;
                }
                case SET_INDEX:
                {
                    object? target = Value(b);
                    long at = b.Long();
                    object? v = Value(b);
                    if (target is Array array && array.Rank == 1)
                    {
                        Type element = array.GetType().GetElementType()!;
                        if (Fit(v, element) == NO) throw new Refusal(CONVERSION, "an element of " + Describe(target) + " cannot take " + Describe(v));
                        array.SetValue(Coerce(v, element), Index(at, array.Length));
                        return;
                    }
                    PropertyInfo? indexer = Indexer(target);
                    if (indexer?.GetSetMethod() == null) throw new Refusal(CONVERSION, Describe(target) + " has no elements to set by position");
                    if (Fit(v, indexer.PropertyType) == NO) throw new Refusal(CONVERSION, "an element of " + Describe(target) + " cannot take " + Describe(v));
                    indexer.SetValue(target, Coerce(v, indexer.PropertyType), new object[] { checked((int)at) });
                    return;
                }
                case LENGTH:
                {
                    object? target = Value(b);
                    if (target is Array array) Put(w, array.Length);
                    else if (target is string text) Put(w, text.Length);
                    else if (target is ICollection collection) Put(w, collection.Count);
                    else
                    {
                        PropertyInfo? count = target == null ? null : target.GetType().GetProperty("Count", typeof(int)) ?? target.GetType().GetProperty("Length", typeof(int));
                        if (count?.GetGetMethod() == null || count.GetIndexParameters().Length != 0) throw new Refusal(CONVERSION, Describe(target) + " has no length");
                        Put(w, count.GetValue(target));
                    }
                    return;
                }
                case INVOKE:
                {
                    object? target = Value(b);
                    string name = b.String()!;
                    object?[] args = Values(b);
                    if (target == null) throw new Refusal(CONVERSION, "cannot call " + name + " on null");
                    Put(w, Invoke(target, name, args));
                    return;
                }
                case CONSTRUCT:
                {
                    object? target = Value(b);
                    object?[] args = Values(b);
                    if (target is not Type type) throw new Refusal(CONVERSION, Describe(target) + " is not a type");
                    Put(w, Construct(type, args));
                    return;
                }
                case TEXT:
                {
                    object? v = Value(b);
                    Put(w, v == null ? "null" : v is IFormattable f ? f.ToString(null, CultureInfo.InvariantCulture) : v is bool flag ? (flag ? "true" : "false") : v.ToString() ?? "");
                    return;
                }
                case SAME:
                {
                    object? a = Value(b);
                    Put(w, ReferenceEquals(a, Value(b)));
                    return;
                }
                case EQUALS:
                {
                    object? a = Value(b);
                    Put(w, Equals(a, Value(b)));
                    return;
                }
                case VALUE:
                    Put(w, Value(b));
                    return;
                case PROXY:
                {
                    Host host = Handler(b.Long());
                    Type type = Find(b.String()!);
                    if (!Callable(type)) throw new Refusal(BINDING, Spell(type) + " is not a delegate type or an interface: an ADM function stands in for one of those");
                    Put(w, Stand(host, type));
                    return;
                }
                case FOREIGN:
                    Put(w, new Opaque(b.Long()));
                    return;
                case COLLECT:
                    GC.Collect();
                    GC.WaitForPendingFinalizers();
                    return;
                case STATS:
                    w.Long(GC.GetTotalMemory(false));
                    lock (gate) w.Long(live);
                    return;
                case DEAD:
                {
                    List<long> ids = new();
                    while (Dropped.TryDequeue(out long id)) ids.Add(id);
                    lock (gate)
                    {
                        foreach (long id in ids)
                        {
                            if (handlers.TryGetValue(id, out WeakReference<Host>? known) && !known.TryGetTarget(out _)) handlers.Remove(id);
                        }
                    }
                    w.Int(ids.Count);
                    foreach (long id in ids) w.Long(id);
                    return;
                }
                case ELEMENTS:
                {
                    object? target = Value(b);
                    if (target is not Array array || array.Rank != 1) throw new Refusal(CONVERSION, Describe(target) + " is not an array");
                    PutArray(w, array);
                    return;
                }
                default:
                    throw new Refusal(BINDING, "unknown request " + op);
            }
        }

        // ---- reading and writing the wire ----

        // Writes a value: numbers, booleans and strings as they are, anything
        // else as a new handle.
        static void Put(Writer w, object? v)
        {
            switch (v)
            {
                case null: w.Byte(T_NULL); return;
                case bool flag: w.Byte(T_BOOL); w.Byte(flag ? 1 : 0); return;
                case long or int or short or sbyte or byte or ushort or uint: w.Byte(T_INT); w.Long(Convert.ToInt64(v)); return;
                case ulong wide: w.Byte(T_INT); w.Long(unchecked((long)wide)); return;
                case double or float: w.Byte(T_FLOAT); w.Long(BitConverter.DoubleToInt64Bits(Convert.ToDouble(v))); return;
                case string or char: w.Byte(T_STRING); w.String(v.ToString()); return;
            }
            w.Byte(T_HANDLE);
            w.Long(Hold(v));
            if (v is Opaque opaque)
            {
                w.Byte(K_FOREIGN);
                w.Long(opaque.Id);
            }
            else
            {
                w.Byte(v is Type ? K_TYPE : v is Array ? K_ARRAY : K_OBJECT);
            }
        }

        static long Hold(object v)
        {
            lock (gate)
            {
                live++;
                if (spare.Count > 0)
                {
                    int slot = spare.Pop();
                    held[slot] = v;
                    return slot + 1;
                }
                held.Add(v);
                return held.Count;
            }
        }

        // Reads a value: a long, a double, a bool, a string, the object behind
        // a handle, a new array, or the Host of a host function.
        static object? Value(Reader b)
        {
            int tag = b.Byte();
            switch (tag)
            {
                case T_NULL: return null;
                case T_BOOL: return b.Byte() != 0;
                case T_INT: return b.Long();
                case T_FLOAT: return BitConverter.Int64BitsToDouble(b.Long());
                case T_STRING: return b.String();
                case T_HANDLE:
                {
                    long handle = b.Long();
                    lock (gate)
                    {
                        return (handle > 0 && handle <= held.Count ? held[(int)handle - 1] : null) ?? throw new Refusal(CONVERSION, "the value was released");
                    }
                }
                case T_FUNCTION: return Handler(b.Long());
                case T_ARRAY:
                {
                    char element = (char)b.Byte();
                    if (element != 'O') return ReadArray(b, element);
                    object?[] items = new object?[b.Int()];
                    for (int i = 0; i < items.Length; i++) items[i] = Coerce(Value(b), typeof(object));
                    return items;
                }
                default: throw new Refusal(CONVERSION, "unknown value tag " + tag);
            }
        }

        static object?[] Values(Reader b)
        {
            object?[] args = new object?[b.Int()];
            for (int i = 0; i < args.Length; i++) args[i] = Value(b);
            return args;
        }

        static Type Element(char kind) => kind switch
        {
            'Z' => typeof(bool), 'b' => typeof(sbyte), 'B' => typeof(byte), 's' => typeof(short), 'S' => typeof(ushort),
            'i' => typeof(int), 'I' => typeof(uint), 'l' => typeof(long), 'L' => typeof(ulong),
            'f' => typeof(float), 'd' => typeof(double), 'T' => typeof(string), 'V' => typeof(void),
            _ => throw new Refusal(CONVERSION, "unknown element kind " + kind),
        };

        static int Width(char kind) => kind switch
        {
            'Z' or 'b' or 'B' => 1, 's' or 'S' => 2, 'i' or 'I' or 'f' => 4, _ => 8,
        };

        // An array of packed elements of this kind; null for an absent one.
        static object? ReadArray(Reader b, char element)
        {
            uint n = (uint)b.Int();
            if (n == Absent) return null;
            if (element == 'T')
            {
                string?[] strings = new string?[n];
                for (int i = 0; i < n; i++) strings[i] = b.String();
                return strings;
            }
            Array array = Array.CreateInstance(Element(element), (int)n);
            b.Fill(array, (int)n * Width(element));
            return array;
        }

        // Writes an array with its elements: T_ARRAY, the element kind, the
        // count, the elements.
        static void PutArray(Writer w, Array a)
        {
            Type element = a.GetType().GetElementType()!;
            w.Byte(T_ARRAY);
            char kind = element == typeof(bool) ? 'Z' : element == typeof(sbyte) ? 'b' : element == typeof(byte) ? 'B'
                : element == typeof(short) ? 's' : element == typeof(ushort) || element == typeof(char) ? 'S' : element == typeof(int) ? 'i'
                : element == typeof(uint) ? 'I' : element == typeof(long) ? 'l' : element == typeof(ulong) ? 'L'
                : element == typeof(float) ? 'f' : element == typeof(double) ? 'd' : element == typeof(string) ? 'T' : 'O';
            w.Byte(kind);
            w.Int(a.Length);
            if (kind == 'T')
            {
                foreach (string? s in (string?[])a) w.String(s);
            }
            else if (kind == 'O')
            {
                foreach (object? item in a) Put(w, item);
            }
            else
            {
                byte[] packed = new byte[a.Length * Width(kind)];
                Buffer.BlockCopy(a, 0, packed, 0, packed.Length);
                w.Bytes(packed);
            }
        }

        static int Index(long at, int length)
        {
            if (at < 0 || at >= length) throw new Refusal(CONVERSION, "index " + at + " is outside 0.." + (length - 1));
            return (int)at;
        }

        // A type as C# spells it, for messages.
        static string Spell(Type t)
        {
            if (t.IsArray) return Spell(t.GetElementType()!) + "[]";
            return Type.GetTypeCode(t) switch
            {
                TypeCode.Boolean => "bool", TypeCode.SByte => "sbyte", TypeCode.Byte => "byte", TypeCode.Int16 => "short",
                TypeCode.UInt16 => "ushort", TypeCode.Int32 => "int", TypeCode.UInt32 => "uint", TypeCode.Int64 => "long",
                TypeCode.UInt64 => "ulong", TypeCode.Single => "float", TypeCode.Double => "double", TypeCode.String => "string",
                TypeCode.Char => "char", TypeCode.Decimal => "decimal",
                _ => t == typeof(void) ? "void" : t == typeof(object) ? "object" : t.FullName ?? t.Name,
            };
        }

        // How messages name a value.
        static string Describe(object? v) => v switch
        {
            null => "null",
            Type t => "type " + Spell(t),
            Host => "an ADM function",
            Opaque => "an ADM object",
            long => "an integer",
            double => "a float",
            bool => "a boolean",
            string => "a string",
            _ => "a " + Spell(v.GetType()),
        };

        static string SpellAll(object?[] args) => string.Join(", ", args.Select(Describe));

        // ---- types and members ----

        // The folders assemblies are loaded from, one per line.
        static void Folders(string list)
        {
            foreach (string folder in list.Split('\n', StringSplitOptions.RemoveEmptyEntries)) folders.Add(Path.GetFullPath(folder));
            if (resolving) return;
            resolving = true;
            AppDomain.CurrentDomain.AssemblyResolve += (_, wanted) =>
            {
                string name = new AssemblyName(wanted.Name).Name + ".dll";
                foreach (string folder in folders)
                {
                    string path = Path.Combine(folder, name);
                    if (File.Exists(path)) return Assembly.LoadFrom(path);
                }
                return null;
            };
        }

        static Type? Keyword(string name) => name switch
        {
            "bool" => typeof(bool), "sbyte" => typeof(sbyte), "byte" => typeof(byte), "short" => typeof(short),
            "ushort" => typeof(ushort), "int" => typeof(int), "uint" => typeof(uint), "long" => typeof(long),
            "ulong" => typeof(ulong), "float" => typeof(float), "double" => typeof(double), "decimal" => typeof(decimal),
            "char" => typeof(char), "string" => typeof(string), "object" => typeof(object),
            _ => null,
        };

        // The type by its name: "Acme.Billing, Billing" names its assembly,
        // "Acme.Billing" is looked for in the runtime's own library, the
        // loaded assemblies and every assembly of the folders; "int[]" and
        // the other C# keywords name their types.
        static Type Find(string typeName)
        {
            if (typeName.EndsWith("[]")) return Find(typeName.Substring(0, typeName.Length - 2)).MakeArrayType();
            Type? found = Keyword(typeName) ?? Type.GetType(typeName, false);
            if (found != null) return found;
            if (!typeName.Contains(','))
            {
                foreach (Assembly loaded in AppDomain.CurrentDomain.GetAssemblies())
                {
                    found = loaded.GetType(typeName, false);
                    if (found != null) return found;
                }
                foreach (string folder in folders)
                {
                    if (!Directory.Exists(folder)) continue;
                    foreach (string path in Directory.GetFiles(folder, "*.dll"))
                    {
                        try
                        {
                            found = Assembly.LoadFrom(path).GetType(typeName, false);
                            if (found != null) return found;
                        }
                        catch (BadImageFormatException) { }
                        catch (FileLoadException) { }
                    }
                }
            }
            throw new Refusal(BINDING, "type " + typeName + " was not found in the assembly folders");
        }

        // The types whose members a value gives: for a type value the type
        // itself (its static members), then System.Type's own; else the
        // value's type.
        static Type[] Faces(object target) => target is Type type ? new[] { type, target.GetType() } : new[] { target.GetType() };

        static PropertyInfo? Property(Type type, string name)
        {
            foreach (PropertyInfo p in type.GetProperties(Members))
            {
                if (p.Name == name && p.GetIndexParameters().Length == 0) return p;
            }
            return null;
        }

        // The indexer of a value that takes one int: list[i], text[i].
        static PropertyInfo? Indexer(object? target)
        {
            if (target == null) return null;
            foreach (MemberInfo m in target.GetType().GetDefaultMembers())
            {
                if (m is PropertyInfo p && p.GetIndexParameters().Length == 1 && p.GetIndexParameters()[0].ParameterType == typeof(int)) return p;
            }
            return null;
        }

        static bool Callable(Type p) => p.IsInterface || (typeof(Delegate).IsAssignableFrom(p) && !p.IsAbstract);

        // How well an argument fits a parameter: 0 is the parameter's own
        // type, larger is a wider conversion, NO is no conversion that keeps
        // the value.
        static int Fit(object? a, Type p)
        {
            if (p.IsByRef || p.IsPointer) return NO;
            Type? inner = Nullable.GetUnderlyingType(p);
            if (inner != null)
            {
                if (a == null) return 1;
                int c = Fit(a, inner);
                return c == NO ? NO : c + 1;
            }
            if (a == null) return p.IsValueType ? NO : 1;
            switch (a)
            {
                case long v:
                    if (p.IsEnum) return 8;
                    switch (Type.GetTypeCode(p))
                    {
                        case TypeCode.Int64: return 0;
                        case TypeCode.Int32: return v == (int)v ? 1 : NO;
                        case TypeCode.Int16: return v == (short)v ? 2 : NO;
                        case TypeCode.SByte: return v == (sbyte)v ? 2 : NO;
                        case TypeCode.Byte: return v == (byte)v ? 2 : NO;
                        case TypeCode.UInt16: return v == (ushort)v ? 2 : NO;
                        case TypeCode.UInt32: return v == (uint)v ? 2 : NO;
                        case TypeCode.UInt64: return v >= 0 ? 2 : NO;
                        case TypeCode.Double: return Math.Abs(v) <= 1L << 53 ? 3 : NO;
                        case TypeCode.Single: return Math.Abs(v) <= 1L << 24 ? 4 : NO;
                        case TypeCode.Decimal: return 4;
                        case TypeCode.Char: return v >= 0 && v <= 0xffff ? 8 : NO;
                    }
                    return p.IsAssignableFrom(typeof(long)) ? 9 : NO;
                case double d:
                    switch (Type.GetTypeCode(p))
                    {
                        case TypeCode.Double: return 0;
                        case TypeCode.Single: return double.IsNaN(d) || (double)(float)d == d ? 1 : NO;
                    }
                    return p.IsAssignableFrom(typeof(double)) ? 9 : NO;
                case bool:
                    return p == typeof(bool) ? 0 : p.IsAssignableFrom(typeof(bool)) ? 9 : NO;
                case string s:
                    if (p == typeof(string)) return 0;
                    if (p == typeof(object)) return 9;
                    if (p.IsAssignableFrom(typeof(string))) return 1;
                    return p == typeof(char) && s.Length == 1 ? 5 : NO;
                case Host:
                    return Callable(p) ? 2 : NO;
            }
            return p == a.GetType() ? 0 : p == typeof(object) ? 9 : p.IsInstanceOfType(a) ? 1 : NO;
        }

        // The argument as the parameter takes it; Fit said it can.
        static object? Coerce(object? a, Type p)
        {
            p = Nullable.GetUnderlyingType(p) ?? p;
            switch (a)
            {
                case long v:
                    if (p.IsEnum) return Enum.ToObject(p, v);
                    switch (Type.GetTypeCode(p))
                    {
                        case TypeCode.Int64: return v;
                        case TypeCode.Int32: return (int)v;
                        case TypeCode.Int16: return (short)v;
                        case TypeCode.SByte: return (sbyte)v;
                        case TypeCode.Byte: return (byte)v;
                        case TypeCode.UInt16: return (ushort)v;
                        case TypeCode.UInt32: return (uint)v;
                        case TypeCode.UInt64: return (ulong)v;
                        case TypeCode.Double: return (double)v;
                        case TypeCode.Single: return (float)v;
                        case TypeCode.Decimal: return (decimal)v;
                        case TypeCode.Char: return (char)v;
                    }
                    // Where any object is taken, an int when it fits.
                    return v == (int)v ? (object)(int)v : v;
                case double d:
                    return p == typeof(float) ? (object)(float)d : d;
                case string s:
                    return p == typeof(char) ? s[0] : s;
                case Host host when Callable(p):
                    return Stand(host, p);
            }
            return a;
        }

        // Fits arguments to parameters: the last ones into a `params` array
        // when `spread`, and parameters with a default may be left out.
        // Returns the cost, NO when they do not fit.
        static int Cost(ParameterInfo[] ps, object?[] args, bool spread)
        {
            int n = spread ? ps.Length - 1 : ps.Length;
            if (args.Length > n && !spread) return NO;
            int total = spread ? 20 : 0;
            for (int i = 0; i < Math.Max(args.Length, n); i++)
            {
                if (i >= args.Length)
                {
                    if (!ps[i].HasDefaultValue) return NO;
                    total++;
                    continue;
                }
                int c = Fit(args[i], i < n ? ps[i].ParameterType : ps[n].ParameterType.GetElementType()!);
                if (c == NO) return NO;
                total += c;
            }
            return total;
        }

        static object?[] Arrange(ParameterInfo[] ps, object?[] args, bool spread)
        {
            object?[] arranged = new object?[ps.Length];
            int n = spread ? ps.Length - 1 : ps.Length;
            for (int i = 0; i < n; i++) arranged[i] = i < args.Length ? Coerce(args[i], ps[i].ParameterType) : ps[i].DefaultValue;
            if (spread)
            {
                Type element = ps[n].ParameterType.GetElementType()!;
                Array rest = Array.CreateInstance(element, Math.Max(args.Length - n, 0));
                for (int i = n; i < args.Length; i++) rest.SetValue(Coerce(args[i], element), i - n);
                arranged[n] = rest;
            }
            return arranged;
        }

        // Whether every parameter of a is at least as narrow as b's.
        static bool Narrower(ParameterInfo[] a, ParameterInfo[] b)
        {
            if (a.Length != b.Length) return false;
            for (int i = 0; i < a.Length; i++)
            {
                if (a[i].ParameterType != b[i].ParameterType && !b[i].ParameterType.IsAssignableFrom(a[i].ParameterType)) return false;
            }
            return true;
        }

        // The method or constructor, of these, that takes the arguments best.
        sealed class Shape
        {
            internal readonly ParameterInfo[] Parameters;
            internal readonly bool Variable;

            internal Shape(MethodBase m)
            {
                Parameters = m.GetParameters();
                Variable = Parameters.Length > 0 && Parameters[^1].IsDefined(typeof(ParamArrayAttribute), false);
            }
        }

        // GetParameters copies its array and IsDefined reads metadata on
        // each call, so both are kept per method.
        static Shape ShapeOf(MethodBase m) => shapes.GetValue(m, static made => new Shape(made));

        static T? Pick<T>(IEnumerable<T> candidates, object?[] args, out bool spread) where T : MethodBase
        {
            T? best = null;
            int bestCost = int.MaxValue;
            spread = false;
            foreach (T m in candidates)
            {
                Shape shape = ShapeOf(m);
                ParameterInfo[] ps = shape.Parameters;
                for (int form = 0; form < (shape.Variable ? 2 : 1); form++)
                {
                    int c = Cost(ps, args, form == 1);
                    if (c == NO) continue;
                    if (c < bestCost || (c == bestCost && best != null && Narrower(ps, ShapeOf(best).Parameters)))
                    {
                        best = m;
                        bestCost = c;
                        spread = form == 1;
                    }
                }
            }
            return best;
        }

        // Calls the method `name` that takes these arguments: of the type
        // for a type value (a static method, else a method of System.Type),
        // else of the object.
        static object? Invoke(object target, string name, object?[] args)
        {
            bool named = false;
            foreach (Type type in Faces(target))
            {
                bool statics = target is Type && type == (Type)target;
                List<MethodInfo> candidates = new();
                foreach (MethodInfo m in MethodsNamed(type, name))
                {
                    if (statics && !m.IsStatic) continue;
                    named = true;
                    if (!m.IsGenericMethodDefinition) candidates.Add(m);
                }
                MethodInfo? best = Pick(candidates, args, out bool spread);
                if (best != null) return best.Invoke(best.IsStatic ? null : target, Arrange(ShapeOf(best).Parameters, args, spread));
                if (named) break;
            }
            string owner = target is Type t ? "type " + Spell(t) : Spell(target.GetType());
            string what = target is Type ? "static method " : "method ";
            throw new Refusal(BINDING, owner + " has no " + what + name + (named ? " that takes (" + SpellAll(args) + ")" : ""));
        }

        // The public methods `name` of a type. Type.GetMethods copies every
        // method of the type on each call, so the answer is kept.
        static MethodInfo[] MethodsNamed(Type type, string name)
        {
            lock (gate)
            {
                Dictionary<string, MethodInfo[]> known = methods.GetOrCreateValue(type);
                if (!known.TryGetValue(name, out MethodInfo[]? found))
                {
                    List<MethodInfo> all = new();
                    foreach (MethodInfo m in type.GetMethods(Members))
                    {
                        if (m.Name == name) all.Add(m);
                    }
                    found = all.ToArray();
                    known[name] = found;
                }
                return found;
            }
        }

        static object? Construct(Type type, object?[] args)
        {
            if (type.IsArray)
            {
                if (args.Length != 1 || args[0] is not long n || n < 0 || n > int.MaxValue)
                {
                    throw new Refusal(BINDING, "an array type is constructed with one argument, its length: " + Spell(type) + " got (" + SpellAll(args) + ")");
                }
                return Array.CreateInstance(type.GetElementType()!, (int)n);
            }
            if (type.IsValueType && args.Length == 0) return Activator.CreateInstance(type);
            ConstructorInfo? best = Pick(type.GetConstructors(), args, out bool spread);
            if (best == null) throw new Refusal(BINDING, "type " + Spell(type) + " has no public constructor that takes (" + SpellAll(args) + ")");
            return best.Invoke(Arrange(best.GetParameters(), args, spread));
        }

        // ---- declarations: a static method by its exact signature ----

        static Type Kind(string kind) => kind.StartsWith('[') ? Element(kind[1]).MakeArrayType() : Element(kind[0]);

        // Binds "type\nmethod\nresult\nparameter kinds separated by spaces"
        // and returns the method's number.
        static int Bind(string spec)
        {
            string[] parts = spec.Split('\n');
            Type type = Find(parts[0]);
            string[] kinds = parts[3].Split(' ', StringSplitOptions.RemoveEmptyEntries);
            Type[] types = Array.ConvertAll(kinds, Kind);
            const BindingFlags statics = BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static;
            // A declaration `total` also names the method `Total`.
            MethodInfo? method = type.GetMethod(parts[1], statics, null, types, null)
                ?? type.GetMethod(char.ToUpperInvariant(parts[1][0]) + parts[1].Substring(1), statics, null, types, null);
            if (method == null || method.ReturnType != Kind(parts[2]))
            {
                throw new Refusal(BINDING, "type " + parts[0] + " has no method `static " + Spell(Kind(parts[2])) + " " + parts[1] + "(" + string.Join(", ", types.Select(Spell)) + ")`");
            }
            lock (gate)
            {
                bound.Add(new Bound { Method = method, Kinds = kinds, Result = parts[2] });
                return bound.Count - 1;
            }
        }

        // Calls a bound method with arguments packed by its signature: a
        // number as 8 bytes, a string as its length and bytes, an array as
        // its count and elements.
        static void Call(Reader b, Writer w)
        {
            Bound target;
            int number = b.Int();
            lock (gate) target = bound[number];
            object?[] args = new object?[target.Kinds.Length];
            for (int i = 0; i < args.Length; i++)
            {
                string kind = target.Kinds[i];
                if (kind[0] == '[') { args[i] = ReadArray(b, kind[1]); continue; }
                if (kind == "T") { args[i] = b.String(); continue; }
                ulong bits = (ulong)b.Long();
                args[i] = kind switch
                {
                    "Z" => bits != 0, "b" => (sbyte)bits, "B" => (byte)bits, "s" => (short)bits, "S" => (ushort)bits,
                    "i" => (int)bits, "I" => (uint)bits, "l" => (long)bits, "L" => bits,
                    "f" => BitConverter.Int32BitsToSingle((int)bits), _ => BitConverter.Int64BitsToDouble((long)bits),
                };
            }
            object? result = target.Method.Invoke(null, args);
            if (result == null)
            {
                w.Byte(T_NULL);
            }
            else if (result is string text)
            {
                w.Byte(T_STRING);
                w.String(text);
            }
            else if (result is Array array)
            {
                PutArray(w, array);
            }
            else
            {
                w.Byte(T_INT);
                w.Long(result switch
                {
                    bool flag => flag ? 1 : 0,
                    float single => (uint)BitConverter.SingleToInt32Bits(single),
                    double wide => BitConverter.DoubleToInt64Bits(wide),
                    ulong unsigned => unchecked((long)unsigned),
                    _ => Convert.ToInt64(result),
                });
            }
        }

        // ---- what the ADM program keeps for .NET ----

        // The Host of a host function: one while anything uses it. Every
        // call hands one count of the function back to the ADM side in time:
        // when the Host made here is collected, or at once when there
        // already is one.
        static Host Handler(long fn)
        {
            lock (gate)
            {
                if (handlers.TryGetValue(fn, out WeakReference<Host>? known) && known.TryGetTarget(out Host? host))
                {
                    Dropped.Enqueue(fn);
                    return host;
                }
                host = new Host(fn);
                handlers[fn] = new WeakReference<Host>(host);
                return host;
            }
        }

        // An object of a delegate type or an interface that runs the host
        // function.
        static object Stand(Host host, Type type)
        {
            if (type.IsInterface)
            {
                object proxy = DispatchProxy.Create(type, typeof(HostProxy));
                ((HostProxy)proxy).Host = host;
                return proxy;
            }
            MethodInfo invoke = type.GetMethod("Invoke")!;
            ParameterInfo[] ps = invoke.GetParameters();
            if (ps.Any(p => p.ParameterType.IsByRef)) throw new Refusal(BINDING, "an ADM function cannot stand in for " + Spell(type) + ", which has ref or out parameters");
            ParameterExpression[] inputs = ps.Select(p => Expression.Parameter(p.ParameterType, p.Name)).ToArray();
            Expression call = Expression.Call(
                typeof(Bridge).GetMethod(nameof(Upcall), BindingFlags.NonPublic | BindingFlags.Static)!,
                Expression.Constant(host),
                Expression.NewArrayInit(typeof(object), inputs.Select(p => Expression.Convert(p, typeof(object)))),
                Expression.Constant(invoke.ReturnType, typeof(Type)),
                Expression.Constant(type.Name));
            Expression body = invoke.ReturnType == typeof(void) ? call : Expression.Convert(call, invoke.ReturnType);
            return Expression.Lambda(type, body, inputs).Compile();
        }

        // Runs a host function of the ADM program with these arguments and
        // returns its result as `wanted`.
        internal static object? Upcall(Host host, object?[] args, Type wanted, string name)
        {
            Writer packed = new();
            packed.Int(args.Length);
            foreach (object? a in args) Put(packed, a);
            byte[] sent = packed.ToArray();
            byte[] answer;
            fixed (byte* p = sent)
            {
                byte* reply = null;
                long size = 0;
                upcall(host.Id, p, sent.Length, &reply, &size);
                answer = new byte[size];
                Marshal.Copy((IntPtr)reply, answer, 0, (int)size);
                NativeMemory.Free(reply);
            }
            GC.KeepAlive(host);
            Reader b = new(answer);
            int status = b.Byte();
            if (status == 2)
            {
                // A .NET exception the function let through.
                object? thrown = Value(b);
                if (thrown is Exception original) ExceptionDispatchInfo.Capture(original).Throw();
                throw new InvalidOperationException(Convert.ToString(thrown));
            }
            if (status != 0)
            {
                long error = b.Long();
                throw new HostException(b.String() ?? "", error);
            }
            object? result = Value(b);
            if (wanted == typeof(void)) return null;
            if (Fit(result, wanted) == NO) throw new InvalidCastException("the ADM function returned " + Describe(result) + " where " + name + " returns " + Spell(wanted));
            return Coerce(result, wanted);
        }
    }
}
