/*
 * CorradoOps.java
 *
 * Java equivalent of the OS-independent high-level chip operations in
 * source/corrado_ops.c: backup (chip -> file), copy (chip -> chip), and
 * delete (erase + verify). Written once against the Programmer interface,
 * exactly as the C version is written once against the C USB primitives.
 * Ported from this repo's own MIT-licensed C sources.
 *
 * SPDX-License-Identifier: MIT
 */
package com.corrado.eprom;

import java.nio.file.Path;

/** High-level chip operations, mirroring the C corrado_usb_* ops verbs. */
public final class CorradoOps {

    private CorradoOps() { }

    /** Copy phase, mirroring the C {@code corrado_copy_phase_t}. */
    public enum CopyPhase { READ, WRITE }

    /**
     * BACKUP: read the whole chip and save it to a raw binary file (the
     * chip -> file direction). Mirrors corrado_usb_backup().
     */
    public static void backup(Programmer dev, EpromType type, Path path,
                              Programmer.Progress progress)
            throws CorradoException {
        if (dev == null || path == null)
            throw new CorradoException(CorradoStatus.ERR_ARG);
        EpromImage img = dev.read(type, progress);
        img.save(path);
    }

    /**
     * COPY (chip -> chip) in two phases that share one in-memory image.
     *
     *   READ  : read the SOURCE/master chip and return its image.
     *   WRITE : program the given image into the TARGET chip and verify.
     *
     * The caller swaps the physical chip between phases. Mirrors
     * corrado_usb_copy(). On READ, {@code buf} is ignored and the master
     * image is returned; on WRITE, {@code buf} must be the image from READ.
     *
     * @return the master image on READ; {@code null} on WRITE.
     */
    public static EpromImage copy(Programmer dev, EpromType type,
                                  CopyPhase phase, EpromImage buf,
                                  Programmer.Progress progress)
            throws CorradoException {
        if (dev == null) throw new CorradoException(CorradoStatus.ERR_ARG);

        switch (phase) {
            case READ:
                return dev.read(type, progress);
            case WRITE:
                if (buf == null) throw new CorradoException(CorradoStatus.ERR_ARG);
                if (buf.type() != type || buf.size() != type.sizeBytes())
                    throw new CorradoException(CorradoStatus.ERR_SIZE);
                dev.write(buf, progress);   /* write() programs then verifies */
                return null;
            default:
                throw new CorradoException(CorradoStatus.ERR_ARG);
        }
    }

    /**
     * DELETE: electrically erase the chip (reusable parts only) and verify it
     * is blank. Thin wrapper over erase(), mirroring corrado_usb_delete().
     */
    public static void delete(Programmer dev, EpromType type)
            throws CorradoException {
        if (dev == null) throw new CorradoException(CorradoStatus.ERR_ARG);
        dev.erase(type);   /* erase() already erases then blank-checks */
    }
}
