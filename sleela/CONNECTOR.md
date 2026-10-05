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

## Transports (interchangeable implementations)

Every transport implements the one `EpromConnector` contract, so a host can
choose or swap transports without changing call sites — exactly the SLeeLa
property. Three ship here:

- **Direct (in-process)** — `DirectEpromConnector` bridges `EpromControl` to
  `EpromConnector` in-process, mapping each verb onto chip operations. Analogue
  of nothing in SLeeLa by itself — it is the "control layer in the same JVM"
  path, suitable with a real libusb-backed `EpromControl`.
- **Process (native CLI)** — `ProcessEpromConnector` launches the native
  Corrado `corrado-eprom` executable as a child process, turning
  `(operation, arguments)` into argv `[exe, operation, arguments]`, stdout into
  the result value and a non-zero exit into a failure. The EPROM analogue of
  SLeeLa's `SleelaProcessConnector`, with the same real-liveness `health()`
  (executable present + runnable, working directory present).
- **HTTP (gateway + client)** — `EpromHttpGateway` exposes a delegate connector
  over `GET /eprom/health` and `POST /eprom/invoke?operation=<name>` (body =
  arguments), and `EpromHttpConnector` is the JDK `HttpClient` that talks to it.
  Modelled on SLeeLa's `SleelaHttpGateway` / `SleelaHttpConnector`, including the
  1 MiB body/response caps and optional CORS origin. An operation-level fault
  comes back as HTTP 422 carrying the error text.

```text
Java host → EpromConnector → { Direct | Process | HTTP } → EpromControl / CLI → TL866 → EPROM
```

All three were verified against `eproms/G60_StockEprom_REFERENCE.bin`: Direct and
HTTP run the full verb flow over the fake backend, and Process drove the real
compiled `corrado-eprom` CLI (`info` returned the live 1992 profile).

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

The remaining pieces are a real **libusb-backed `EpromControl`** (via a usb4java
binding or JNI/JNA to the Corrado C `usb-driver/<os>/` backend) and, if wanted,
an **RMI** transport — both drop in behind the unchanged `EpromConnector`
contract, the same extension property SLeeLa's connector series has. The
Process transport already reaches the real hardware path today by driving the
native `corrado-eprom` CLI.
