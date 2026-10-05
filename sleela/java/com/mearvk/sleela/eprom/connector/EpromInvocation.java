package com.mearvk.sleela.eprom.connector;

import java.util.Objects;

/**
 * Immutable request sent from a Java host into the USB&rarr;EPROM connector.
 *
 * <p>Modelled on the SLeeLa connector contract (see {@code CONNECTOR.md} in the
 * SLeeLa repository): a transport-neutral {@code (operation, arguments)} record.
 * Here the operation is an EPROM chip verb — {@code read}, {@code write},
 * {@code verify}, {@code blankcheck}, {@code backup}, {@code copy},
 * {@code erase}, {@code checksum} — and {@code arguments} is the operation's
 * single argument string (e.g. a file path), kept textual so no transport needs
 * Java serialization.
 */
public record EpromInvocation(String operation, String arguments) {

    public EpromInvocation {
        Objects.requireNonNull(operation, "operation");
        if (operation.isBlank()) {
            throw new IllegalArgumentException("operation must not be blank");
        }
        arguments = arguments == null ? "" : arguments;
    }

    /** An invocation of {@code operation} with no arguments. */
    public static EpromInvocation of(String operation) {
        return new EpromInvocation(operation, "");
    }

    /** An invocation of {@code operation} with a single argument string. */
    public static EpromInvocation of(String operation, String arguments) {
        return new EpromInvocation(operation, arguments);
    }
}
