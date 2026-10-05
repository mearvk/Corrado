package com.mearvk.sleela.eprom.connector;

/**
 * Common Java-side integration contract for the USB&rarr;EPROM feature.
 *
 * <p>Modelled on {@code SleelaJavaConnector} from the SLeeLa connector series:
 * a Java application programs against this interface and does not depend on how
 * the programmer hardware is reached. The same {@code invoke(...)} call can be
 * backed by a direct in-process libusb driver, a launched native CLI, or a
 * remote gateway — selected at deployment, not at the call site.
 *
 * <pre>
 *   Java application &rarr; EpromConnector &rarr; transport adapter &rarr; TL866 &rarr; EPROM
 * </pre>
 *
 * <p>Like the SLeeLa contract, {@code invoke(...)} folds faults into a
 * {@link EpromResult#failure(String)} rather than throwing, so callers branch
 * on {@link EpromResult#success()}. {@link #health()} may throw and
 * {@link #isHealthy()} wraps it to a boolean.
 */
public interface EpromConnector extends AutoCloseable {

    /** Execute one EPROM operation, returning a transport-neutral result. */
    EpromResult invoke(EpromInvocation invocation);

    /** Convenience: build and invoke in one call. */
    default EpromResult invoke(String operation, String arguments) {
        return invoke(new EpromInvocation(operation, arguments));
    }

    /** Lightweight transport-level liveness check (e.g. programmer present). */
    String health() throws Exception;

    /** Boolean wrapper over {@link #health()}. */
    default boolean isHealthy() {
        try {
            health();
            return true;
        } catch (Exception failure) {
            return false;
        }
    }

    @Override
    default void close() throws Exception {
        // Stateless connectors have nothing to release.
    }
}
