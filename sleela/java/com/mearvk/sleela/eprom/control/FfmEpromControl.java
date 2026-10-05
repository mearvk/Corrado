package com.mearvk.sleela.eprom.control;

import com.mearvk.sleela.connector.DynamiteConnector;

import java.lang.foreign.Arena;
import java.lang.foreign.FunctionDescriptor;
import java.lang.foreign.Linker;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.SymbolLookup;
import java.lang.foreign.ValueLayout;
import java.lang.invoke.MethodHandle;
import java.nio.file.Path;
import java.util.Objects;

/**
 * Real, direct in-JVM {@link EpromControl} backend: Java &rarr; FFM &rarr; the
 * native {@code corrado_ffm} shim &rarr; the Corrado USB driver &rarr; libusb
 * &rarr; TL866 &rarr; EPROM. No JNI and no child process — it binds the shim's
 * flat C ABI through the JDK Foreign Function &amp; Memory API, loading the
 * shared library via SLeeLa's {@link DynamiteConnector}.
 *
 * <p>Bound shim symbols (see {@code ffm-shim/corrado_ffm.c}):
 * <pre>
 *   int  cffm_open(void)
 *   void cffm_close(void)
 *   int  cffm_model(void)
 *   int  cffm_device_size(int typeOrdinal)
 *   int  cffm_read(int typeOrdinal, unsigned char *out, int outLen)
 *   int  cffm_write(int typeOrdinal, const unsigned char *in, int inLen)
 *   int  cffm_blank_check(int typeOrdinal)
 *   int  cffm_erase(int typeOrdinal)
 * </pre>
 *
 * <p>Status ints are the Corrado {@code corrado_status_t} values; 0 == OK.
 * Running requires {@code --enable-native-access} for the owning module/jar.
 *
 * <p>Safety unchanged: back up stock first, verify every write; destructive
 * verbs reach real hardware here.
 */
public final class FfmEpromControl implements EpromControl {

    /* Corrado corrado_status_t -> this layer's Reason (0 == OK). */
    private static EpromControlException.Reason reasonOf(int status) {
        return switch (status) {
            case  0 -> null;                                         // OK
            case -1 -> EpromControlException.Reason.ARG;
            case -2 -> EpromControlException.Reason.IO;
            case -3 -> EpromControlException.Reason.SIZE;
            case -4 -> EpromControlException.Reason.CHECKSUM;
            case -5 -> EpromControlException.Reason.NO_DEVICE;
            case -6 -> EpromControlException.Reason.USB;
            case -7 -> EpromControlException.Reason.VERIFY;
            case -8 -> EpromControlException.Reason.UNSUPPORTED;
            case -9 -> EpromControlException.Reason.TIMEOUT;
            default -> EpromControlException.Reason.USB;
        };
    }

    /* Map the Java DeviceType to the shim's type ordinal (0/1/2). */
    private static int ordinalOf(DeviceType type) {
        return switch (type) {
            case _27C128 -> 0;
            case _27C256 -> 1;
            case _27C512 -> 2;
        };
    }

    private final DynamiteConnector dynamite;
    private final boolean ownsDynamite;
    private final MethodHandle hOpen, hClose, hModel, hRead, hWrite, hBlank, hErase;
    private boolean open;

    /**
     * @param libraryDir directory holding the native shim
     *                   (libcorrado_ffm.so / .dll / .dylib)
     */
    public FfmEpromControl(Path libraryDir) {
        this(newConnector(libraryDir), true);
    }

    /** Use a caller-managed DynamiteConnector (which must allow "corrado_ffm"). */
    public FfmEpromControl(DynamiteConnector dynamite, boolean ownsDynamite) {
        this.dynamite = Objects.requireNonNull(dynamite, "dynamite");
        this.ownsDynamite = ownsDynamite;

        SymbolLookup lk = dynamite.lookup("corrado_ffm");
        Linker linker = Linker.nativeLinker();

        FunctionDescriptor iV   = FunctionDescriptor.of(ValueLayout.JAVA_INT);
        FunctionDescriptor vV   = FunctionDescriptor.ofVoid();
        FunctionDescriptor iI   = FunctionDescriptor.of(ValueLayout.JAVA_INT, ValueLayout.JAVA_INT);
        FunctionDescriptor iIPI = FunctionDescriptor.of(ValueLayout.JAVA_INT,
                ValueLayout.JAVA_INT, ValueLayout.ADDRESS, ValueLayout.JAVA_INT);

        this.hOpen  = bind(linker, lk, "cffm_open", iV);
        this.hClose = bind(linker, lk, "cffm_close", vV);
        this.hModel = bind(linker, lk, "cffm_model", iV);
        this.hRead  = bind(linker, lk, "cffm_read", iIPI);
        this.hWrite = bind(linker, lk, "cffm_write", iIPI);
        this.hBlank = bind(linker, lk, "cffm_blank_check", iI);
        this.hErase = bind(linker, lk, "cffm_erase", iI);
    }

    private static DynamiteConnector newConnector(Path libraryDir) {
        DynamiteConnector dc = new DynamiteConnector();
        dc.addRoot(libraryDir).allowLibrary("corrado_ffm");
        return dc;
    }

    private static MethodHandle bind(Linker linker, SymbolLookup lk,
                                     String symbol, FunctionDescriptor fd) {
        MemorySegment addr = lk.find(symbol).orElseThrow(() ->
                new IllegalStateException("shim symbol not found: " + symbol));
        return linker.downcallHandle(addr, fd);
    }

    @Override
    public Model open() throws EpromControlException {
        int st;
        try { st = (int) hOpen.invokeExact(); }
        catch (Throwable t) { throw ffmFault(t); }
        check(st);
        open = true;
        int m;
        try { m = (int) hModel.invokeExact(); }
        catch (Throwable t) { throw ffmFault(t); }
        return switch (m) {
            case 0 -> Model.TL866A;
            case 1 -> Model.TL866II;
            default -> Model.UNKNOWN;
        };
    }

    @Override
    public byte[] read(DeviceType type, Progress progress) throws EpromControlException {
        requireOpen();
        int n = type.sizeBytes();
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment buf = arena.allocate(n);
            int st;
            try { st = (int) hRead.invokeExact(ordinalOf(type), buf, n); }
            catch (Throwable t) { throw ffmFault(t); }
            check(st);
            byte[] out = buf.toArray(ValueLayout.JAVA_BYTE);
            if (progress != null) progress.update(n, n);
            return out;
        }
    }

    @Override
    public void write(DeviceType type, byte[] image, Progress progress) throws EpromControlException {
        requireOpen();
        if (image == null || image.length != type.sizeBytes())
            throw new EpromControlException(EpromControlException.Reason.SIZE);
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment buf = arena.allocate(image.length);
            MemorySegment.copy(image, 0, buf, ValueLayout.JAVA_BYTE, 0, image.length);
            int st;
            try { st = (int) hWrite.invokeExact(ordinalOf(type), buf, image.length); }
            catch (Throwable t) { throw ffmFault(t); }
            check(st);                       // shim write() already verifies
            if (progress != null) progress.update(image.length, image.length);
        }
    }

    @Override
    public boolean blankCheck(DeviceType type) throws EpromControlException {
        requireOpen();
        int st;
        try { st = (int) hBlank.invokeExact(ordinalOf(type)); }
        catch (Throwable t) { throw ffmFault(t); }
        if (st == 0) return true;
        if (st == -7) return false;          // CORRADO_ERR_VERIFY => not blank
        check(st);                           // any other code is a real fault
        return false;                        // unreachable
    }

    @Override
    public void erase(DeviceType type) throws EpromControlException {
        requireOpen();
        int st;
        try { st = (int) hErase.invokeExact(ordinalOf(type)); }
        catch (Throwable t) { throw ffmFault(t); }
        check(st);
    }

    @Override
    public void close() {
        try { if (open) hClose.invokeExact(); }
        catch (Throwable ignored) { /* best-effort teardown */ }
        finally {
            open = false;
            if (ownsDynamite) dynamite.close();
        }
    }

    private void requireOpen() throws EpromControlException {
        if (!open) throw new EpromControlException(EpromControlException.Reason.NO_DEVICE);
    }

    private static void check(int status) throws EpromControlException {
        EpromControlException.Reason r = reasonOf(status);
        if (r != null) throw new EpromControlException(r);
    }

    private static EpromControlException ffmFault(Throwable t) {
        return new EpromControlException(EpromControlException.Reason.USB, t);
    }
}
