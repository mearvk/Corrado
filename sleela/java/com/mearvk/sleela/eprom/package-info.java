/**
 * SLeeLa-style <b>Connector/Control</b> series for the USB&rarr;EPROM feature.
 *
 * <p>This package applies the SLeeLa connector + driver design (see the SLeeLa
 * repository's {@code CONNECTOR.md} and {@code DRIVERS.md}) to the Corrado
 * EPROM programmer, giving the feature a transport-neutral integration surface.
 *
 * <h2>Two contracts</h2>
 * <ul>
 *   <li><b>Connector</b> &mdash;
 *       {@link com.mearvk.sleela.eprom.connector.EpromConnector}: the Java-side
 *       integration contract a host programs against ({@code invoke},
 *       {@code health}, {@code isHealthy}, {@code close}), with the immutable
 *       {@link com.mearvk.sleela.eprom.connector.EpromInvocation} /
 *       {@link com.mearvk.sleela.eprom.connector.EpromResult} value records.</li>
 *   <li><b>Control</b> &mdash;
 *       {@link com.mearvk.sleela.eprom.control.EpromControl}: the device-facing
 *       driver contract (open/read/write/verify/blank-check/erase) that a
 *       backend implements. {@link com.mearvk.sleela.eprom.control.FakeEpromControl}
 *       is the required fake backend for tests.</li>
 * </ul>
 *
 * <h2>Transport</h2>
 * {@link com.mearvk.sleela.eprom.transport.DirectEpromConnector} bridges Control
 * to Connector in-process — the EPROM analogue of SLeeLa's process connector.
 * RMI/HTTP transports could be added later without changing call sites, exactly
 * as in the SLeeLa series.
 *
 * <pre>
 *   Java host &rarr; EpromConnector &rarr; transport adapter &rarr; EpromControl &rarr; TL866 &rarr; EPROM
 * </pre>
 *
 * <h2>Error model</h2>
 * {@code invoke(...)} folds faults into {@code EpromResult.failure(...)} rather
 * than throwing, mirroring the SLeeLa connector. The same safety posture as the
 * rest of the Corrado suite applies: back up stock images first and verify
 * every write.
 */
package com.mearvk.sleela.eprom;
