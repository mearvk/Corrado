/*
 * CorradoException.java
 *
 * Checked exception carrying a CorradoStatus, the Java idiom for the C
 * library's integer return codes. Mirrors the C++ wrapper's UsbError.
 *
 * SPDX-License-Identifier: MIT
 */
package com.corrado.eprom;

/** Exception carrying a {@link CorradoStatus}. */
public class CorradoException extends Exception {
    private static final long serialVersionUID = 1L;
    private final CorradoStatus status;

    public CorradoException(CorradoStatus status) {
        super(status.message());
        this.status = status;
    }

    public CorradoException(CorradoStatus status, Throwable cause) {
        super(status.message(), cause);
        this.status = status;
    }

    public CorradoStatus status() { return status; }
}
