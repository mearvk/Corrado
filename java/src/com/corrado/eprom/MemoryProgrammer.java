/*
 * MemoryProgrammer.java
 *
 * A hardware-free, in-memory Programmer backend. It simulates a chip with a
 * byte buffer so the operator logic (CorradoOps), the image library, and the
 * checksum code can be exercised and unit-tested without a TL866 attached.
 *
 * A real backend would implement Programmer over a libusb binding (e.g.
 * usb4java) or JNI/JNA to the C usb-driver, speaking the TL866 bulk protocol.
 * This stub is the Java analogue of running the C library against the
 * eproms/*_REFERENCE.bin images rather than live hardware.
 *
 * SPDX-License-Identifier: MIT
 */
package com.corrado.eprom;

import java.util.Arrays;

/** In-memory programmer for testing: the "chip" is a byte[] you control. */
public final class MemoryProgrammer implements Programmer {

    private final EpromType type;
    private byte[] cell;             /* current chip contents            */
    private final boolean erasable;  /* false => UV/OTP (erase ERR_UNSUPPORTED) */
    private boolean open = true;

    /** @param erasable true for a reusable part; false for genuine UV/OTP 27C. */
    public MemoryProgrammer(EpromType type, byte[] initial, boolean erasable) {
        this.type = type;
        this.erasable = erasable;
        this.cell = (initial != null)
                ? initial.clone()
                : blankCells(type.sizeBytes());
    }

    private static byte[] blankCells(int n) {
        byte[] b = new byte[n];
        Arrays.fill(b, (byte) 0xFF);
        return b;
    }

    @Override public Model model() { return Model.TL866II; }

    @Override
    public EpromImage read(EpromType t, Progress progress) throws CorradoException {
        requireOpen();
        if (t != type) throw new CorradoException(CorradoStatus.ERR_SIZE);
        if (progress != null) progress.update(cell.length, cell.length);
        return EpromImage.of(t, cell);
    }

    @Override
    public void write(EpromImage image, Progress progress) throws CorradoException {
        requireOpen();
        if (image.type() != type || image.size() != type.sizeBytes())
            throw new CorradoException(CorradoStatus.ERR_SIZE);
        cell = image.data().clone();
        if (progress != null) progress.update(cell.length, cell.length);
        /* verify by read-back, as the real write() does */
        if (!Arrays.equals(cell, image.data()))
            throw new CorradoException(CorradoStatus.ERR_VERIFY);
    }

    @Override
    public boolean blankCheck(EpromType t) throws CorradoException {
        requireOpen();
        if (t != type) throw new CorradoException(CorradoStatus.ERR_SIZE);
        for (byte b : cell) if ((b & 0xFF) != 0xFF) return false;
        return true;
    }

    @Override
    public void erase(EpromType t) throws CorradoException {
        requireOpen();
        if (t != type) throw new CorradoException(CorradoStatus.ERR_SIZE);
        if (!erasable) throw new CorradoException(CorradoStatus.ERR_UNSUPPORTED);
        cell = blankCells(type.sizeBytes());
        if (!blankCheck(t)) throw new CorradoException(CorradoStatus.ERR_VERIFY);
    }

    @Override public void close() { open = false; }

    private void requireOpen() throws CorradoException {
        if (!open) throw new CorradoException(CorradoStatus.ERR_USB);
    }
}
