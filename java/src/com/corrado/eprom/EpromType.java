/*
 * EpromType.java
 *
 * Java equivalent of corrado_eprom_type_t plus the size/name helpers from
 * the C library. Ported from this repo's own MIT-licensed C sources.
 *
 * SPDX-License-Identifier: MIT
 */
package com.corrado.eprom;

/** Supported EPROM device types, mirroring the C {@code corrado_eprom_type_t}. */
public enum EpromType {
    _27C128("27C128", 16384),
    _27C256("27C256", 32768),   /* standard Corrado ECU chip */
    _27C512("27C512", 65536),   /* twin-tune / 512 adapters  */
    UNKNOWN("UNKNOWN", 0);

    private final String deviceName;
    private final int sizeBytes;

    EpromType(String deviceName, int sizeBytes) {
        this.deviceName = deviceName;
        this.sizeBytes = sizeBytes;
    }

    /** Byte size of the device; 0 for UNKNOWN. */
    public int sizeBytes() { return sizeBytes; }

    /** Human-readable device name. */
    public String deviceName() { return deviceName; }

    /** Infer a type from a raw file/image byte count (UNKNOWN if no match). */
    public static EpromType fromSize(long n) {
        for (EpromType t : values())
            if (t != UNKNOWN && t.sizeBytes == n) return t;
        return UNKNOWN;
    }

    @Override public String toString() { return deviceName; }
}
