package com.mearvk.sleela.eprom;

import com.mearvk.sleela.eprom.connector.EpromConnector;
import com.mearvk.sleela.eprom.connector.EpromInvocation;
import com.mearvk.sleela.eprom.connector.EpromResult;
import com.mearvk.sleela.eprom.control.EpromControl;
import com.mearvk.sleela.eprom.control.FakeEpromControl;
import com.mearvk.sleela.eprom.transport.DirectEpromConnector;

import java.nio.file.Files;
import java.nio.file.Path;

/**
 * Exercises the SLeeLa-style Connector/Control series for USB&rarr;EPROM end to
 * end over the fake backend: health, model, backup, verify, write, blank-check,
 * erase — all through the transport-neutral {@link EpromConnector} contract.
 *
 * Usage: java ... EpromConnectorDemo <reference.bin> [work-dir]
 */
public final class EpromConnectorDemo {

    public static void main(String[] args) throws Exception {
        if (args.length < 1) {
            System.err.println("usage: EpromConnectorDemo <reference.bin> [work-dir]");
            System.exit(2);
            return;
        }
        Path ref = Path.of(args[0]);
        Path work = Path.of(args.length >= 2 ? args[1] : ".");
        byte[] stock = Files.readAllBytes(ref);

        // A fake 27C256 chip pre-loaded with the reference image, reusable part.
        EpromControl chip =
            new FakeEpromControl(EpromControl.DeviceType._27C256, stock, true);

        try (EpromConnector c = new DirectEpromConnector(chip, EpromControl.DeviceType._27C256)) {
            System.out.println("health      : " + c.health());
            System.out.println("isHealthy   : " + c.isHealthy());
            print("model", c.invoke(EpromInvocation.of("model")));

            Path backup = work.resolve("sleela-backup.bin");
            print("backup", c.invoke("backup", backup.toString()));
            System.out.println("  identical : " +
                java.util.Arrays.equals(stock, Files.readAllBytes(backup)));

            print("verify", c.invoke("verify", backup.toString()));
            print("write",  c.invoke("write",  backup.toString()));
            print("blankcheck(before erase)", c.invoke(EpromInvocation.of("blankcheck")));
            print("erase",  c.invoke(EpromInvocation.of("erase")));
            print("blankcheck(after erase)",  c.invoke(EpromInvocation.of("blankcheck")));
            print("unknown-op", c.invoke("frobnicate", ""));
        }

        // --- HTTP transport: same contract, over the loopback gateway --------
        System.out.println();
        System.out.println("=== HTTP transport (gateway + client, same contract) ===");
        EpromControl chip2 =
            new FakeEpromControl(EpromControl.DeviceType._27C256, stock, true);
        try (EpromConnector backing =
                 new DirectEpromConnector(chip2, EpromControl.DeviceType._27C256);
             com.mearvk.sleela.eprom.transport.EpromHttpGateway gw =
                 new com.mearvk.sleela.eprom.transport.EpromHttpGateway("127.0.0.1", 0, backing)) {
            gw.start();
            try (EpromConnector http =
                     new com.mearvk.sleela.eprom.transport.EpromHttpConnector(gw.baseUri())) {
                System.out.println("http.health : " + http.health());
                print("http model",  http.invoke(EpromInvocation.of("model")));
                Path httpBackup = work.resolve("sleela-http-backup.bin");
                print("http backup", http.invoke("backup", httpBackup.toString()));
                print("http verify", http.invoke("verify", httpBackup.toString()));
                print("http unknown", http.invoke("frobnicate", ""));
            }
        }
    }

    private static void print(String label, EpromResult r) {
        System.out.printf("%-26s: %s%n", label,
            r.success() ? "OK  " + r.value() : "ERR " + r.error());
    }
}
