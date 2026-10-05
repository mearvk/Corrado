/*
 * Demo.java
 *
 * Exercises the Java port end to end against the repo's reference images and
 * the in-memory programmer backend: load image + checksum, then backup / copy
 * / delete via CorradoOps. The Java analogue of running the C library/CLI
 * against eproms/*_REFERENCE.bin.
 *
 * Usage: java -cp out com.corrado.eprom.Demo <reference.bin> [work-dir]
 *
 * SPDX-License-Identifier: MIT
 */
package com.corrado.eprom;

import java.nio.file.Files;
import java.nio.file.Path;

public final class Demo {

    public static void main(String[] args) throws Exception {
        if (args.length < 1) {
            System.err.println("usage: Demo <reference.bin> [work-dir]");
            System.exit(2);
            return;
        }
        Path ref = Path.of(args[0]);
        Path work = Path.of(args.length >= 2 ? args[1] : ".");

        // 1. image library + checksum
        EpromImage img = EpromImage.load(ref, EpromType.UNKNOWN);
        System.out.printf("loaded %s: type=%s size=%d checksum16=0x%04X%n",
                ref.getFileName(), img.type(), img.size(), img.checksum16());
        System.out.println("  checksumVerify: " + img.checksumVerify());

        // 2. simulate a chip holding this image, then BACKUP it to a file
        try (Programmer dev = new MemoryProgrammer(img.type(), img.data(), true)) {
            System.out.println("programmer model: " + dev.model().display());

            Path backup = work.resolve("backup.bin");
            CorradoOps.backup(dev, img.type(), backup,
                    (done, total) -> { });
            System.out.printf("backup -> %s (%d bytes), identical=%b%n",
                    backup.getFileName(), Files.size(backup),
                    java.util.Arrays.equals(Files.readAllBytes(ref),
                                            Files.readAllBytes(backup)));

            // 3. COPY: read master, then write into the (same) target + verify
            EpromImage master = CorradoOps.copy(dev, img.type(),
                    CorradoOps.CopyPhase.READ, null, null);
            CorradoOps.copy(dev, img.type(),
                    CorradoOps.CopyPhase.WRITE, master, null);
            System.out.println("copy (read master -> write+verify target): OK");

            // 4. DELETE: erase a reusable part and confirm blank
            CorradoOps.delete(dev, img.type());
            System.out.println("delete (erase + blank-check): OK, blank="
                    + dev.blankCheck(img.type()));
        }

        // 5. checksum fix round-trip on a fresh copy. Behaviour matches the C
        //    library exactly: checksumFix() stores the trailing word so that
        //    checksumVerify() passes. (Note: the suite's checksum16() sums the
        //    two trailing bytes individually while fix/verify treat them as one
        //    16-bit word, so checksum16() is not driven to 0 by fix() — this is
        //    the C library's own convention, reproduced here for parity.)
        EpromImage img2 = EpromImage.load(ref, EpromType.UNKNOWN);
        int word = img2.checksumFix();
        System.out.printf("checksumFix stored word=0x%04X, verify now=%s%n",
                word, img2.checksumVerify());
    }
}
