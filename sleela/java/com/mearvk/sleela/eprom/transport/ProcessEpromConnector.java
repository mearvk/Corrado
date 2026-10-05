package com.mearvk.sleela.eprom.transport;

import com.mearvk.sleela.eprom.connector.EpromConnector;
import com.mearvk.sleela.eprom.connector.EpromInvocation;
import com.mearvk.sleela.eprom.connector.EpromResult;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.TimeUnit;

/**
 * Local-process transport: drives the native Corrado CLI
 * ({@code build/<os>/<year>/corrado-eprom}) as a child process, implementing
 * the {@link EpromConnector} contract. This is the EPROM analogue of SLeeLa's
 * {@code SleelaProcessConnector} — appropriate when the Java host and the real
 * programmer tooling live on the same machine.
 *
 * <p>An invocation {@code (operation, arguments)} becomes the argv
 * {@code [exe, operation, arguments]} (the argument is omitted when blank),
 * matching the CLI verbs: {@code info|read|write|verify|blankcheck|backup|
 * copy|delete|checksum}. The child's stdout is the result value; a non-zero
 * exit folds stderr into {@link EpromResult#failure(String)}.
 */
public final class ProcessEpromConnector implements EpromConnector {

    private final Path executable;
    private final Path workingDirectory;
    private final Duration timeout;

    /** Simple duration holder to avoid importing java.time for one field. */
    public record Duration(long seconds) {
        public static Duration ofSeconds(long s) { return new Duration(s); }
    }

    public ProcessEpromConnector(Path executable, Path workingDirectory) {
        this(executable, workingDirectory, Duration.ofSeconds(120));
    }

    public ProcessEpromConnector(Path executable, Path workingDirectory, Duration timeout) {
        this.executable = Objects.requireNonNull(executable, "executable");
        this.workingDirectory = Objects.requireNonNull(workingDirectory, "workingDirectory");
        this.timeout = Objects.requireNonNull(timeout, "timeout");
    }

    @Override
    public EpromResult invoke(EpromInvocation invocation) {
        List<String> argv = new ArrayList<>();
        argv.add(executable.toString());
        argv.add(invocation.operation());
        if (!invocation.arguments().isBlank()) {
            argv.add(invocation.arguments());
        }
        try {
            ProcessBuilder pb = new ProcessBuilder(argv)
                    .directory(workingDirectory.toFile());
            Process p = pb.start();

            byte[] out = p.getInputStream().readAllBytes();
            byte[] err = p.getErrorStream().readAllBytes();

            boolean finished = p.waitFor(timeout.seconds(), TimeUnit.SECONDS);
            if (!finished) {
                p.destroyForcibly();
                return EpromResult.failure("operation timed out after "
                        + timeout.seconds() + "s");
            }
            String stdout = new String(out, StandardCharsets.UTF_8).strip();
            String stderr = new String(err, StandardCharsets.UTF_8).strip();
            if (p.exitValue() == 0) {
                return EpromResult.success(stdout);
            }
            String msg = !stderr.isEmpty() ? stderr
                       : (!stdout.isEmpty() ? stdout
                       : "corrado-eprom exited " + p.exitValue());
            return EpromResult.failure(msg);
        } catch (IOException e) {
            return EpromResult.failure("cannot launch " + executable + ": " + e.getMessage());
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            return EpromResult.failure("interrupted");
        }
    }

    /**
     * Real liveness check, like SLeeLa's: confirm the CLI exists, is a regular
     * executable file, and the working directory is present.
     */
    @Override
    public String health() {
        if (!Files.isRegularFile(executable)) {
            throw new IllegalStateException("corrado-eprom not found: " + executable);
        }
        if (!Files.isExecutable(executable)) {
            throw new IllegalStateException("corrado-eprom is not executable: " + executable);
        }
        if (!Files.isDirectory(workingDirectory)) {
            throw new IllegalStateException("working directory not found: " + workingDirectory);
        }
        return "process connector = ready (" + executable + ")";
    }

    @Override
    public void close() {
        // Each invoke() runs a short-lived child; nothing persistent to release.
    }
}
