/*
 * CorradoStatus.java
 *
 * Java equivalent of the corrado_status_t enum + corrado_strerror() from
 * the C library (include/corrado_eprom.h, source/corrado_eprom.c). Ported
 * from this repo's own MIT-licensed C sources.
 *
 * SPDX-License-Identifier: MIT
 */
package com.corrado.eprom;

/** Result/status codes, mirroring the C {@code corrado_status_t}. */
public enum CorradoStatus {
    OK("success"),
    ERR_ARG("invalid argument"),
    ERR_IO("file I/O error"),
    ERR_SIZE("image size mismatch"),
    ERR_CHECKSUM("checksum verification failed"),
    ERR_NO_DEVICE("no programmer found"),
    ERR_USB("USB transport error"),
    ERR_VERIFY("verify mismatch"),
    ERR_UNSUPPORTED("unsupported operation"),
    ERR_TIMEOUT("operation timed out");

    private final String message;

    CorradoStatus(String message) { this.message = message; }

    /** Human-readable description, mirroring corrado_strerror(). */
    public String message() { return message; }

    @Override public String toString() { return message; }
}
