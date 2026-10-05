package com.mearvk.sleela.eprom.control;

import java.util.Arrays;

/**
 * Hardware-free {@link EpromControl} backend: the "fake backend" every SLeeLa
 * driver family is expected to ship (see {@code DRIVERS.md} &rarr; Testing). The
 * simulated chip is a byte buffer, so the connector, control contract, and
 * operators can be exercised and unit-tested without a TL866 attached.
 *
 * <p>A real backend would implement {@link EpromControl} over a libusb binding
 * (e.g. usb4java) or JNI/JNA to the Corrado C {@code usb-driver/<os>/} backend.
 */
public final class FakeEpromControl implements EpromControl {

    private final DeviceType type;
    private final boolean erasable;
    private byte[] cell;
    private boolean open;

    /** @param erasable false models a genuine UV/OTP 27C part (erase unsupported). */
    public FakeEpromControl(DeviceType type, byte[] initial, boolean erasable) {
        this.type = type;
        this.erasable = erasable;
        this.cell = (initial != null) ? initial.clone() : blank(type.sizeBytes());
    }

    private static byte[] blank(int n) {
        byte[] b = new byte[n];
        Arrays.fill(b, (byte) 0xFF);
        return b;
    }

    @Override
    public Model open() throws EpromControlException {
        open = true;
        return Model.TL866II;
    }

    @Override
    public byte[] read(DeviceType t, Progress progress) throws EpromControlException {
        requireOpen();
        requireType(t);
        if (progress != null) progress.update(cell.length, cell.length);
        return cell.clone();
    }

    @Override
    public void write(DeviceType t, byte[] image, Progress progress) throws EpromControlException {
        requireOpen();
        requireType(t);
        if (image == null || image.length != t.sizeBytes())
            throw new EpromControlException(EpromControlException.Reason.SIZE);
        cell = image.clone();
        if (progress != null) progress.update(cell.length, cell.length);
        if (!Arrays.equals(cell, image))   // verify by read-back
            throw new EpromControlException(EpromControlException.Reason.VERIFY);
    }

    @Override
    public boolean blankCheck(DeviceType t) throws EpromControlException {
        requireOpen();
        requireType(t);
        for (byte b : cell) if ((b & 0xFF) != 0xFF) return false;
        return true;
    }

    @Override
    public void erase(DeviceType t) throws EpromControlException {
        requireOpen();
        requireType(t);
        if (!erasable)
            throw new EpromControlException(EpromControlException.Reason.UNSUPPORTED);
        cell = blank(t.sizeBytes());
        if (!blankCheck(t))
            throw new EpromControlException(EpromControlException.Reason.VERIFY);
    }

    @Override
    public void close() { open = false; }

    private void requireOpen() throws EpromControlException {
        if (!open) throw new EpromControlException(EpromControlException.Reason.NO_DEVICE);
    }

    private void requireType(DeviceType t) throws EpromControlException {
        if (t != type) throw new EpromControlException(EpromControlException.Reason.SIZE);
    }
}
