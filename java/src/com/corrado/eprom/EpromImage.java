/*
 * EpromImage.java
 *
 * Java equivalent of the corrado_image_t container and the image library
 * (alloc/load/save/checksum) from source/corrado_eprom.c. Ported from this
 * repo's own MIT-licensed C sources. In Java the owned byte buffer is a
 * byte[], so there is no explicit free().
 *
 * SPDX-License-Identifier: MIT
 */
package com.corrado.eprom;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

/** A ROM image: a device type and its owned byte buffer. */
public final class EpromImage {

    private final EpromType type;
    private final byte[] data;

    private EpromImage(EpromType type, byte[] data) {
        this.type = type;
        this.data = data;
    }

    public EpromType type() { return type; }
    public int size() { return data.length; }

    /** Direct access to the backing buffer (mirrors C img.data). */
    public byte[] data() { return data; }

    /**
     * Allocate a blank image for a device type. An erased EPROM cell reads as
     * 1, so a blank image is all 0xFF (mirrors corrado_image_alloc).
     */
    public static EpromImage blank(EpromType type) throws CorradoException {
        int sz = type.sizeBytes();
        if (sz == 0) throw new CorradoException(CorradoStatus.ERR_UNSUPPORTED);
        byte[] buf = new byte[sz];
        java.util.Arrays.fill(buf, (byte) 0xFF);
        return new EpromImage(type, buf);
    }

    /** Wrap an existing byte array as an image of the given type. */
    public static EpromImage of(EpromType type, byte[] bytes)
            throws CorradoException {
        if (type == EpromType.UNKNOWN) throw new CorradoException(CorradoStatus.ERR_UNSUPPORTED);
        if (bytes.length != type.sizeBytes())
            throw new CorradoException(CorradoStatus.ERR_SIZE);
        return new EpromImage(type, bytes.clone());
    }

    /**
     * Load a raw binary ROM dump from disk. When {@code type} is UNKNOWN the
     * device type is inferred from the file size (mirrors corrado_image_load).
     */
    public static EpromImage load(Path path, EpromType type)
            throws CorradoException {
        final byte[] bytes;
        try {
            bytes = Files.readAllBytes(path);
        } catch (IOException e) {
            throw new CorradoException(CorradoStatus.ERR_IO, e);
        }
        EpromType t = (type == EpromType.UNKNOWN)
                ? EpromType.fromSize(bytes.length) : type;
        if (t == EpromType.UNKNOWN || t.sizeBytes() != bytes.length)
            throw new CorradoException(CorradoStatus.ERR_SIZE);
        return new EpromImage(t, bytes);
    }

    /** Write the image out to a raw binary file (mirrors corrado_image_save). */
    public void save(Path path) throws CorradoException {
        try {
            Files.write(path, data);
        } catch (IOException e) {
            throw new CorradoException(CorradoStatus.ERR_IO, e);
        }
    }

    /**
     * 16-bit additive checksum over the whole image (mirrors corrado_checksum16).
     *
     * <p>Note: this sums every byte individually, including the two trailing
     * checksum bytes. {@link #checksumFix()} / {@link #checksumVerify()} treat
     * those two bytes as a single 16-bit word, so {@code checksum16()} is NOT
     * generally 0 after a fix. This matches the C library's convention exactly;
     * the two methods answer different questions by design.
     */
    public int checksum16() {
        int sum = 0;
        for (byte b : data) sum = (sum + (b & 0xFF)) & 0xFFFF;
        return sum;
    }

    /**
     * Verify the Digifant/Motronic-style trailing checksum word (last two
     * bytes, little-endian) so the 16-bit sum of all bytes is zero. Mirrors
     * corrado_checksum_fix(..., verify_only=1).
     *
     * @return OK if the stored word is correct, ERR_CHECKSUM otherwise.
     */
    public CorradoStatus checksumVerify() throws CorradoException {
        if (data.length < 2) throw new CorradoException(CorradoStatus.ERR_SIZE);
        int need = requiredChecksumWord();
        int lo = data[data.length - 2] & 0xFF;
        int hi = data[data.length - 1] & 0xFF;
        int have = lo | (hi << 8);
        return (have == need) ? CorradoStatus.OK : CorradoStatus.ERR_CHECKSUM;
    }

    /**
     * Recompute and store the trailing checksum word so the 16-bit sum of all
     * bytes equals 0 (mirrors corrado_checksum_fix(..., verify_only=0)).
     *
     * @return the stored checksum word.
     */
    public int checksumFix() throws CorradoException {
        if (data.length < 2) throw new CorradoException(CorradoStatus.ERR_SIZE);
        int need = requiredChecksumWord();
        data[data.length - 2] = (byte) (need & 0xFF);
        data[data.length - 1] = (byte) ((need >> 8) & 0xFF);
        return need;
    }

    /* Checksum word that makes (sum-of-all-except-word + word) == 0 mod 65536. */
    private int requiredChecksumWord() {
        int sumWithout = 0;
        for (int i = 0; i < data.length - 2; ++i)
            sumWithout = (sumWithout + (data[i] & 0xFF)) & 0xFFFF;
        return (0x10000 - sumWithout) & 0xFFFF;
    }
}
