package com.mearvk.sleela.eprom.control;

/**
 * Failure from the EPROM control layer, carrying an explicit reason so the
 * connector can map it to an {@code EpromResult.failure(...)} message. Mirrors
 * the Corrado {@code CorradoStatus} categories and the SLeeLa driver model's
 * "explicit ownership and failure semantics" requirement.
 */
public class EpromControlException extends Exception {

    private static final long serialVersionUID = 1L;

    /** Failure categories, aligned with the Corrado status codes. */
    public enum Reason {
        ARG("invalid argument"),
        IO("file I/O error"),
        SIZE("image size mismatch"),
        CHECKSUM("checksum verification failed"),
        NO_DEVICE("no programmer found"),
        USB("USB transport error"),
        VERIFY("verify mismatch"),
        UNSUPPORTED("unsupported operation"),
        TIMEOUT("operation timed out");

        private final String message;
        Reason(String message) { this.message = message; }
        public String message() { return message; }
    }

    private final Reason reason;

    public EpromControlException(Reason reason) {
        super(reason.message());
        this.reason = reason;
    }

    public EpromControlException(Reason reason, Throwable cause) {
        super(reason.message(), cause);
        this.reason = reason;
    }

    public Reason reason() { return reason; }
}
