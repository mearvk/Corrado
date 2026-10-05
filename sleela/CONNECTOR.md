# SLeeLa-style Connector/Control series — USB → EPROM

This directory applies the **SLeeLa connector + driver design** (referenced
from the [SLeeLa](https://github.com/mearvk/SLeeLa) repository's `CONNECTOR.md`
and `DRIVERS.md`) to the Corrado USB→EPROM feature, giving it a
transport-neutral integration surface. It is an original representation of that
pattern for this domain — the Corrado EPROM code remains authoritative for the
actual chip operation.

## Design

```text
                 Java application
                        │
              EpromConnector (contract)
                        │
          ┌─────────────┼─────────────┐
          │             │             │
       Direct          RMI*          HTTP*
          │             │             │
     EpromControl   (future)      (future)
          │
      OS adapter  (libusb / CLI / fake)
          │
        TL866  →  EPROM

   * RMI/HTTP transports are future adapters; the Direct in-process
     transport ships here, mirroring SLeeLa's process connector.
```

The connector presents the integration surface; the **control** contract
presents the device verbs. This is the two-contract split SLeeLa uses: a
connector (`CONNECTOR.md`) for the application boundary, a driver model
(`DRIVERS.md`) for the device boundary.

## Connector contract

`EpromConnector` is the transport-neutral interface. A Java program uses:

```java
EpromResult r = connector.invoke("backup", "/path/stock.bin");
if (r.success()) System.out.println(r.value());
else             System.err.println(r.error());
```

Core types (mirroring SLeeLa's `SleelaJavaConnector` / `SleelaInvocation` /
`SleelaResult`):

- `EpromConnector` — common Java-side contract (`invoke`, `health`,
  `isHealthy`, `close`).
- `EpromInvocation` — immutable `(operation, arguments)` request record.
- `EpromResult` — immutable `(success, value, error)` result record.

Operations are the EPROM chip verbs: `model`, `backup <path>`, `write <path>`,
`verify <path>`, `blankcheck`, `erase`.

## Control contract (the driver model)

`EpromControl` is the device-facing contract — the SLeeLa driver model applied
to the programmer:

```text
application → EPROM API → EpromControl (contract) → OS adapter → hardware
```

It carries the SLeeLa-listed driver responsibilities (init/shutdown, capability
discovery, I/O submission, completion/error reporting, cleanup) and exposes
`open / read / write / blankCheck / erase`. `EpromControlException.Reason`
mirrors the Corrado status categories for explicit failure semantics.

Backends:

- `FakeEpromControl` — the required **fake backend** (SLeeLa `DRIVERS.md` →
  Testing): an in-memory chip for unit tests and the demo.
- *(real)* — would implement `EpromControl` over a libusb binding (usb4java) or
  JNI/JNA to the Corrado C `usb-driver/<os>/` backend speaking the TL866 bulk
  protocol.

## Transport

`DirectEpromConnector` bridges `EpromControl` to `EpromConnector` in-process —
the EPROM analogue of SLeeLa's `SleelaProcessConnector`. It maps each textual
operation onto chip verbs and folds faults into `EpromResult.failure(...)`,
per the SLeeLa connector error model.

```text
Java host → EpromConnector → DirectEpromConnector → EpromControl → TL866 → EPROM
```

## Error model

`invoke(...)` returns an `EpromResult` rather than throwing: any control or I/O
fault becomes a `failure(...)`, so callers branch on `success()`. `health()`
may throw; `isHealthy()` wraps it to a boolean. Identical to the SLeeLa
connector contract.

## Build & run

```sh
cd sleela/java
javac -d out $(find com -name '*.java')
java -cp out com.mearvk.sleela.eprom.EpromConnectorDemo \
    ../../eproms/G60_StockEprom_REFERENCE.bin /tmp
```

The demo runs health → model → backup → verify → write → blank-check → erase
over the fake backend against a reference image (verified: backup is
byte-identical, all verbs succeed, unknown ops fail cleanly).

## Security / safety boundary

As in SLeeLa, the connector is an integration boundary, not an authorization
system, and any future RMI/HTTP transport must add authentication, TLS, and
request limits before exposure. The Corrado hardware safety posture also
applies: **back up the stock image first and verify every write.**

## Future extension

Only new transport adapters (RMI, HTTP gateway) and a real libusb-backed
`EpromControl` need to be added; the `EpromConnector` contract and call sites
stay unchanged — the same extension property SLeeLa's connector series has.
