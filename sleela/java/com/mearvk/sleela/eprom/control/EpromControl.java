package com.mearvk.sleela.eprom.control;

/**
 * Device-facing control contract for the USB&rarr;EPROM programmer — the
 * "Control" half of the SLeeLa Connector/Control series for this feature.
 *
 * <p>Modelled on the SLeeLa driver model (see {@code DRIVERS.md}):
 * <pre>
 *   application &rarr; EPROM API &rarr; EpromControl (contract) &rarr; OS adapter &rarr; hardware
 * </pre>
 * where the connector supplies the integration surface and the control contract
 * supplies the device verbs. An implementation (direct libusb, a CLI shell-out,
 * a remote gateway, or a fake) carries the SLeeLa-listed driver
 * responsibilities: initialization/shutdown, capability discovery, I/O
 * submission, completion/error reporting, and resource cleanup.
 *
 * <p>The verbs mirror the Corrado C/Java core (read/write/verify/erase/
 * blank-check + the backup/copy operators). Sizes are expressed as a
 * {@link DeviceType} rather than raw ints so callers cannot mis-size a chip.
 */
public interface EpromControl extends AutoCloseable {

    /** Supported chip sizes, mirroring the Corrado {@code EpromType}. */
    enum DeviceType {
        _27C128(16384), _27C256(32768), _27C512(65536);
        private final int bytes;
        DeviceType(int bytes) { this.bytes = bytes; }
        public int sizeBytes() { return bytes; }
    }

    /** Programmer model, mirroring the Corrado {@code Programmer.Model}. */
    enum Model { TL866A, TL866II, UNKNOWN }

    /** Progress callback for long transfers. */
    @FunctionalInterface
    interface Progress { void update(long done, long total); }

    /** Driver init / capability discovery: open and identify the programmer. */
    Model open() throws EpromControlException;

    /** Read the whole device into a returned byte image. */
    byte[] read(DeviceType type, Progress progress) throws EpromControlException;

    /** Program the whole image, then verify by read-back. */
    void write(DeviceType type, byte[] image, Progress progress) throws EpromControlException;

    /** Confirm the whole device reads as 0xFF (erased). */
    boolean blankCheck(DeviceType type) throws EpromControlException;

    /**
     * Electrically erase, then confirm blank. Genuine UV/OTP 27C parts throw
     * with {@link EpromControlException.Reason#UNSUPPORTED}; reusable
     * replacements succeed.
     */
    void erase(DeviceType type) throws EpromControlException;

    @Override
    void close();
}
