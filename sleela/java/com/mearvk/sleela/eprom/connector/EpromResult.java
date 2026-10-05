package com.mearvk.sleela.eprom.connector;

import java.util.Objects;

/**
 * Transport-neutral result returned to a Java host from a USB&rarr;EPROM
 * operation.
 *
 * <p>Modelled on the SLeeLa connector's result record: a validated
 * {@code (success, value, error)} triple. A successful read/backup might carry
 * a checksum or a file path in {@code value}; a failure carries a human-readable
 * {@code error} (typically a {@code CorradoStatus} message).
 */
public record EpromResult(boolean success, String value, String error) {

    public EpromResult {
        value = value == null ? "" : value;
        error = error == null ? "" : error;
        if (success && !error.isEmpty()) {
            throw new IllegalArgumentException("successful results cannot contain an error");
        }
        if (!success && error.isEmpty()) {
            throw new IllegalArgumentException("failed results must contain an error");
        }
    }

    public static EpromResult success(String value) {
        return new EpromResult(true, Objects.requireNonNullElse(value, ""), "");
    }

    public static EpromResult failure(String error) {
        return new EpromResult(false, "", Objects.requireNonNull(error, "error"));
    }
}
