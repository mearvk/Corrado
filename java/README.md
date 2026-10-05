# Java — Corrado EPROM library (port of the C/C++ operator files)

A Java port of the Corrado suite's **OS-independent** core: the image library
(`source/corrado_eprom.c`), the high-level chip operators
(`source/corrado_ops.c` — backup / copy / delete), and the programmer
interface (`include/corrado_usb.h`). It mirrors the C++ RAII wrapper's
object-oriented shape (`usb-driver/corrado_usb.hpp`).

This is the repo's own MIT-licensed code, re-expressed in Java.

## Layout

```
java/src/com/corrado/eprom/
├── CorradoStatus.java    # status codes + messages  (corrado_status_t / strerror)
├── CorradoException.java # checked exception carrying a status (≈ C++ UsbError)
├── EpromType.java        # device types + size/name  (corrado_eprom_type_t)
├── EpromImage.java       # image container + load/save/checksum (corrado_eprom.c)
├── Programmer.java       # low-level USB primitives interface (corrado_usb.h)
├── CorradoOps.java       # backup / copy / delete operators    (corrado_ops.c)
├── MemoryProgrammer.java # hardware-free in-memory backend (for tests)
└── Demo.java             # end-to-end demo against the reference images
```

## Build & run

```sh
cd java
javac -d out src/com/corrado/eprom/*.java

# exercise the whole library against a reference image (no hardware needed):
java -cp out com.corrado.eprom.Demo ../eproms/G60_StockEprom_REFERENCE.bin /tmp
java -cp out com.corrado.eprom.Demo ../eproms/VR6_StockEprom_REFERENCE.bin /tmp
```

## Parity with the C library (verified)

The Java image/checksum code produces **identical** results to the C library
on the committed reference images:

| image | C `checksum16` | Java `checksum16` |
|-------|----------------|-------------------|
| `G60_StockEprom_REFERENCE.bin` | `0x60A0` | `0x60A0` |
| `VR6_StockEprom_REFERENCE.bin` | `0xE41C` | `0xE41C` |

### A faithfully-reproduced quirk

`checksum16()` sums every byte (including the two trailing checksum bytes),
while `checksumFix()` / `checksumVerify()` treat those two bytes as one 16-bit
word. So a `checksumFix()` makes `checksumVerify()` pass but does **not** drive
`checksum16()` to 0. This is the **C library's own convention** (confirmed
against `source/corrado_eprom.c`); the Java port reproduces it rather than
silently diverging. The methods answer different questions by design.

## Hardware (USB) backend

`Programmer` is the Java analogue of the C `corrado_usb.h` primitives. A real
backend would implement it over a libusb binding (e.g. **usb4java**) or via
JNI/JNA to the existing C `usb-driver/<os>/` backend, speaking the TL866 bulk
protocol. `MemoryProgrammer` is a hardware-free stand-in used for the demo and
tests — the Java analogue of exercising the C library against the `eproms/`
reference images instead of a live chip.

> The same safety posture applies as the rest of the suite: back up the stock
> image first and verify every write. See the top-level `README.md`.
