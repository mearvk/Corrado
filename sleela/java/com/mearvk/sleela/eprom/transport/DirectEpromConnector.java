package com.mearvk.sleela.eprom.transport;

import com.mearvk.sleela.eprom.connector.EpromConnector;
import com.mearvk.sleela.eprom.connector.EpromInvocation;
import com.mearvk.sleela.eprom.connector.EpromResult;
import com.mearvk.sleela.eprom.control.EpromControl;
import com.mearvk.sleela.eprom.control.EpromControlException;

import java.nio.file.Files;
import java.nio.file.Path;

/**
 * Direct, in-process transport: adapts an {@link EpromControl} backend to the
 * {@link EpromConnector} contract. This is the EPROM analogue of SLeeLa's
 * {@code SleelaProcessConnector} — the "same machine" adapter — but talking to
 * the control layer directly rather than launching a separate executable.
 *
 * <p>It maps the textual connector operations onto chip verbs:
 * <ul>
 *   <li>{@code backup  <path>} — read the chip, write bytes to the file.</li>
 *   <li>{@code write   <path>} — program the file's bytes, then verify.</li>
 *   <li>{@code verify  <path>} — read the chip and compare to the file.</li>
 *   <li>{@code blankcheck}     — confirm the chip reads all 0xFF.</li>
 *   <li>{@code erase}          — erase (reusable parts) and blank-check.</li>
 *   <li>{@code model}          — report the detected programmer model.</li>
 * </ul>
 * Faults are folded into {@link EpromResult#failure(String)}, per the SLeeLa
 * connector error model.
 */
public final class DirectEpromConnector implements EpromConnector {

    private final EpromControl control;
    private final EpromControl.DeviceType type;
    private EpromControl.Model model = EpromControl.Model.UNKNOWN;
    private boolean opened;

    public DirectEpromConnector(EpromControl control, EpromControl.DeviceType type) {
        this.control = control;
        this.type = type;
    }

    private void ensureOpen() throws EpromControlException {
        if (!opened) { model = control.open(); opened = true; }
    }

    @Override
    public EpromResult invoke(EpromInvocation invocation) {
        try {
            ensureOpen();
            final String op = invocation.operation();
            final String arg = invocation.arguments();
            switch (op) {
                case "model":
                    return EpromResult.success(model.name());

                case "blankcheck":
                    return EpromResult.success(
                        control.blankCheck(type) ? "blank" : "not-blank");

                case "erase":
                    control.erase(type);
                    return EpromResult.success("erased");

                case "backup": {
                    requireArg(arg, "backup <path>");
                    byte[] img = control.read(type, null);
                    Files.write(Path.of(arg), img);
                    return EpromResult.success("backup " + arg + " " + img.length + " bytes");
                }

                case "write": {
                    requireArg(arg, "write <path>");
                    byte[] img = Files.readAllBytes(Path.of(arg));
                    control.write(type, img, null);   // write() verifies by read-back
                    return EpromResult.success("wrote+verified " + img.length + " bytes");
                }

                case "verify": {
                    requireArg(arg, "verify <path>");
                    byte[] onChip = control.read(type, null);
                    byte[] onDisk = Files.readAllBytes(Path.of(arg));
                    boolean same = java.util.Arrays.equals(onChip, onDisk);
                    return same ? EpromResult.success("verify match")
                                : EpromResult.failure("verify mismatch");
                }

                default:
                    return EpromResult.failure("unknown operation: " + op);
            }
        } catch (EpromControlException e) {
            return EpromResult.failure(e.reason().message());
        } catch (Exception e) {
            return EpromResult.failure("file I/O error: " + e.getMessage());
        }
    }

    private static void requireArg(String arg, String usage) {
        if (arg == null || arg.isBlank())
            throw new IllegalArgumentException("usage: " + usage);
    }

    @Override
    public String health() throws Exception {
        ensureOpen();
        return "programmer=" + model.name();
    }

    @Override
    public void close() {
        control.close();
    }
}
