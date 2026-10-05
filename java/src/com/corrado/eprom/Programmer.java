/*
 * Programmer.java
 *
 * Java equivalent of the OS-independent USB programmer interface
 * (include/corrado_usb.h): the low-level read/write/erase/blank-check
 * primitives a backend must implement. Ported from this repo's own
 * MIT-licensed C sources.
 *
 * Java has no built-in libusb; a concrete backend (e.g. via the usb4java
 * libusb binding, or JNI/JNA to the C backend) implements this interface.
 * The operator logic in CorradoOps is written once against it, exactly as
 * the C corrado_ops.c is written once against the C primitives.
 *
 * SPDX-License-Identifier: MIT
 */
package com.corrado.eprom;

/** Low-level programmer transport, mirroring the C USB primitive functions. */
public interface Programmer extends AutoCloseable {

    /** Known programmer models (mirrors corrado_prog_model_t). */
    enum Model {
        TL866A("TL866A/CS"),
        TL866II("TL866II+"),
        UNKNOWN("unknown");

        private final String display;
        Model(String display) { this.display = display; }
        public String display() { return display; }
    }

    /** Progress callback: done/total bytes (mirrors corrado_progress_cb). */
    @FunctionalInterface
    interface Progress { void update(long done, long total); }

    /** Detected programmer model. */
    Model model();

    /** Read the entire device into a freshly allocated image of {@code type}. */
    EpromImage read(EpromType type, Progress progress) throws CorradoException;

    /** Program the whole image to the device, then verify by read-back. */
    void write(EpromImage image, Progress progress) throws CorradoException;

    /** Confirm the whole device reads as 0xFF (erased). */
    boolean blankCheck(EpromType type) throws CorradoException;

    /**
     * Electrically erase the device, then confirm blank. A genuine UV/OTP
     * 27C part throws {@link CorradoException} with
     * {@link CorradoStatus#ERR_UNSUPPORTED}; reusable replacements succeed.
     */
    void erase(EpromType type) throws CorradoException;

    @Override
    void close();
}
