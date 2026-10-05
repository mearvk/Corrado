/*
 * NOTE: Vendored copy. The authoritative source of DynamiteConnector lives in
 * the mearvk/SLeeLa repository (connector/java/.../DynamiteConnector.java). It
 * is copied here only so the Corrado FFM EPROM control can load the native
 * shim without a build-time dependency on SLeeLa. Keep in sync with upstream.
 */
package com.mearvk.sleela.connector;

import java.lang.foreign.Arena;
import java.lang.foreign.SymbolLookup;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;

/**
 * DynamiteConnector — intelligent loader of native shared libraries for SLeeLa.
 *
 * <p>The native-library analogue of {@link SleelaClassLoader}: where that one
 * resolves raw compiled <em>Java</em> classes under an allow-list, this one
 * resolves and loads raw <em>native</em> libraries ({@code .so} / {@code .dll}
 * / {@code .dylib}) across Linux, Windows and macOS, under the same hardened
 * posture — explicit roots, allow-listed logical names, cached, fail-closed.
 *
 * <p>It solves the problem that {@link System#loadLibrary(String)} only searches
 * {@code java.library.path} and gives no control over <em>which</em> library a
 * logical name maps to. A SLeeLa driver family (e.g. {@code eprom-corrado},
 * {@code sleela_java28}) registers a logical name, points at one or more search
 * roots, and loads by name without caring about the per-OS filename or
 * location.
 *
 * <p>Two load modes:
 * <ul>
 *   <li>{@link #load(String)} — resolve the file and
 *       {@link System#load(String)} it (JNI-style: the library's own
 *       {@code JNI_OnLoad} / registered natives become callable).</li>
 *   <li>{@link #lookup(String)} — resolve the file and open a
 *       {@link SymbolLookup} over it (FFM / Panama: look up and bind individual
 *       symbols without JNI). Returned handles are backed by an {@link Arena}
 *       released on {@link #close()}.</li>
 * </ul>
 *
 * <p>Thread-safe; {@link AutoCloseable} (closing releases FFM arenas opened for
 * symbol lookups). Loading is fail-closed: a disallowed logical name, or a name
 * that resolves to no file on any root, throws rather than guessing.
 */
public final class DynamiteConnector implements AutoCloseable {

    /** Host operating-system family, for per-OS library filename mapping. */
    public enum Os { LINUX, WINDOWS, MACOS, UNKNOWN }

    /** Thrown when a native library cannot be resolved or loaded. */
    public static final class LibraryResolutionException extends RuntimeException {
        public LibraryResolutionException(String message) { super(message); }
        public LibraryResolutionException(String message, Throwable cause) { super(message, cause); }
    }

    private final Os os;
    private final List<Path> roots = new ArrayList<>();
    private final Set<String> allowedNames = new LinkedHashSet<>();
    private boolean allowAny = false;

    /* logical name -> resolved absolute path (memoised) */
    private final Map<String, Path> resolved = new ConcurrentHashMap<>();
    /* logical name -> SymbolLookup (memoised); arenas closed on close() */
    private final Map<String, SymbolLookup> lookups = new ConcurrentHashMap<>();
    private final List<Arena> arenas = new ArrayList<>();
    private volatile boolean closed = false;

    public DynamiteConnector() { this(detectOs()); }

    public DynamiteConnector(Os os) {
        this.os = Objects.requireNonNull(os, "os");
    }

    // ---- configuration ----------------------------------------------------

    /** Add a directory to search for native libraries. */
    public synchronized DynamiteConnector addRoot(Path dir) {
        Objects.requireNonNull(dir, "dir");
        if (!Files.isDirectory(dir)) {
            throw new IllegalArgumentException("root is not a directory: " + dir);
        }
        if (!roots.contains(dir)) roots.add(dir);
        return this;
    }

    /** Allow a logical library name (e.g. {@code "sleela_java28"}). */
    public synchronized DynamiteConnector allowLibrary(String logicalName) {
        allowedNames.add(validateName(logicalName));
        return this;
    }

    /**
     * Remove the allow-list, permitting any validated logical name. Use only in
     * trusted contexts; the default (empty allow-list) is fail-closed.
     */
    public synchronized DynamiteConnector allowAny() {
        this.allowAny = true;
        return this;
    }

    public synchronized boolean isAllowed(String logicalName) {
        String n;
        try { n = validateName(logicalName); } catch (RuntimeException e) { return false; }
        return allowAny || allowedNames.contains(n);
    }

    public Os os() { return os; }

    // ---- resolution --------------------------------------------------------

    /**
     * Map a logical name to the per-OS filename and locate it on the roots,
     * without loading it. Memoised.
     */
    public Path resolve(String logicalName) {
        ensureOpen();
        String name = validateName(logicalName);
        if (!isAllowed(name)) {
            throw new LibraryResolutionException("library not allow-listed: " + name);
        }
        Path cached = resolved.get(name);
        if (cached != null) return cached;

        List<String> candidates = fileNamesFor(name);
        synchronized (this) {
            for (Path root : roots) {
                for (String file : candidates) {
                    Path p = root.resolve(file);
                    if (Files.isRegularFile(p)) {
                        Path abs = p.toAbsolutePath();
                        resolved.put(name, abs);
                        return abs;
                    }
                }
            }
        }
        throw new LibraryResolutionException(
            "no native library for '" + name + "' on roots " + roots
            + " (looked for " + candidates + ")");
    }

    // ---- loading -----------------------------------------------------------

    /**
     * Resolve and {@link System#load(String)} the library (JNI-style). Safe to
     * call repeatedly for the same name; the JVM de-duplicates by path.
     *
     * @return the absolute path that was loaded.
     */
    public Path load(String logicalName) {
        Path path = resolve(logicalName);
        try {
            System.load(path.toString());
        } catch (UnsatisfiedLinkError e) {
            throw new LibraryResolutionException("failed to load " + path, e);
        }
        return path;
    }

    /**
     * Resolve the library and open a {@link SymbolLookup} over it (FFM / no
     * JNI). The lookup is backed by an {@link Arena} that is released on
     * {@link #close()}. Memoised per logical name.
     */
    public SymbolLookup lookup(String logicalName) {
        ensureOpen();
        String name = validateName(logicalName);
        SymbolLookup cached = lookups.get(name);
        if (cached != null) return cached;

        Path path = resolve(name);
        synchronized (this) {
            SymbolLookup again = lookups.get(name);
            if (again != null) return again;
            Arena arena = Arena.ofShared();
            try {
                SymbolLookup lk = SymbolLookup.libraryLookup(path, arena);
                arenas.add(arena);
                lookups.put(name, lk);
                return lk;
            } catch (RuntimeException e) {
                arena.close();
                throw new LibraryResolutionException("failed to open symbol lookup for " + path, e);
            }
        }
    }

    /** Convenience: does the given exported symbol exist in the library? */
    public boolean hasSymbol(String logicalName, String symbol) {
        return lookup(logicalName).find(symbol).isPresent();
    }

    /** Find a symbol's address, if present (FFM). */
    public Optional<java.lang.foreign.MemorySegment> find(String logicalName, String symbol) {
        return lookup(logicalName).find(symbol);
    }

    // ---- per-OS filename mapping ------------------------------------------

    /**
     * The candidate filenames a logical name maps to on this OS. We try the
     * JVM's own {@link System#mapLibraryName(String)} first, then the common
     * conventions, so both {@code foo} and an already-decorated {@code libfoo.so}
     * resolve.
     */
    public List<String> fileNamesFor(String logicalName) {
        String name = validateName(logicalName);
        LinkedHashSet<String> out = new LinkedHashSet<>();
        // If the caller passed a decorated filename, honour it verbatim first.
        if (name.contains(".")) out.add(name);
        out.add(System.mapLibraryName(name)); // JVM's own mapping for this OS
        switch (os) {
            case LINUX   -> { out.add("lib" + name + ".so"); out.add(name + ".so"); }
            case MACOS   -> { out.add("lib" + name + ".dylib"); out.add(name + ".dylib"); }
            case WINDOWS -> { out.add(name + ".dll"); out.add("lib" + name + ".dll"); }
            default      -> { /* only the mapLibraryName result */ }
        }
        return new ArrayList<>(out);
    }

    public static Os detectOs() {
        String s = System.getProperty("os.name", "").toLowerCase(Locale.ROOT);
        if (s.contains("win")) return Os.WINDOWS;
        if (s.contains("mac") || s.contains("darwin")) return Os.MACOS;
        if (s.contains("nux") || s.contains("nix") || s.contains("aix")) return Os.LINUX;
        return Os.UNKNOWN;
    }

    // ---- lifecycle ---------------------------------------------------------

    @Override
    public synchronized void close() {
        if (closed) return;
        closed = true;
        lookups.clear();
        for (Arena a : arenas) {
            try { a.close(); } catch (RuntimeException ignored) { /* best-effort */ }
        }
        arenas.clear();
    }

    private void ensureOpen() {
        if (closed) throw new IllegalStateException("DynamiteConnector is closed");
    }

    /** A valid, non-blank library name without path separators or traversal. */
    private static String validateName(String name) {
        Objects.requireNonNull(name, "logicalName");
        String n = name.strip();
        if (n.isEmpty()) throw new IllegalArgumentException("library name is blank");
        if (n.contains("/") || n.contains("\\")) {
            throw new IllegalArgumentException("library name must not contain path separators: " + name);
        }
        if (n.contains("..")) {
            throw new IllegalArgumentException("library name must not contain '..': " + name);
        }
        return n;
    }
}
