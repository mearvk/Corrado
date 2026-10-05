# ffm-shim — direct in-JVM USB path (Java FFM → C → libusb → TL866)

This is the real, direct hardware path for the `sleela/eprom` Connector/Control
series: a Java host drives the physical programmer **in-process**, with no JNI
and no child process, by binding a thin native shim through the JDK Foreign
Function & Memory (FFM) API. The shim is loaded via SLeeLa's
`DynamiteConnector`.

```
Java (FfmEpromControl)
   │  JDK FFM downcall handles
   ▼
libcorrado_ffm.so / .dylib / .dll        <- this shim (flat C ABI)
   │  corrado_usb_* / corrado_image_*
   ▼
usb-driver/<os> + source/corrado_eprom   <- the existing Corrado C driver
   │  libusb-1.0
   ▼
MiniPRO TL866  →  EPROM
```

## Why a shim

The Corrado USB functions take opaque `corrado_usb_dev_t*` and
`corrado_image_t*` pointers, which are awkward and fragile to marshal across
FFM. `corrado_ffm.c` exposes a **flat ABI** — plain `int` status/sizes and
caller byte buffers — over one process-global open device:

```c
int  cffm_open(void);
void cffm_close(void);
int  cffm_model(void);
int  cffm_device_size(int typeOrdinal);
int  cffm_read (int typeOrdinal, unsigned char *out, int outLen);
int  cffm_write(int typeOrdinal, const unsigned char *in, int inLen);
int  cffm_blank_check(int typeOrdinal);
int  cffm_erase(int typeOrdinal);
```

Status ints are the Corrado `corrado_status_t` values (0 == OK); device-type
ordinals are 0=27C128, 1=27C256, 2=27C512.

## Build

```sh
cd ffm-shim
make                    # link against system libusb-1.0
make LIBUSB_VENDOR=1    # or the bundled libusb (needs it built with -fPIC)
```

Produces `libcorrado_ffm.so` (Linux). The macOS/Windows equivalents
(`libcorrado_ffm.dylib` / `corrado_ffm.dll`) build the same way with the
respective OS USB backend.

> Vendored-libusb note: to link the static vendored libusb into a shared
> object, build it with PIC:
> `make -C ../vendor/libusb CFLAGS="-O2 -std=gnu11 -fPIC -fvisibility=hidden"`.

## Use from Java

```java
import com.mearvk.sleela.eprom.control.FfmEpromControl;
import java.nio.file.Path;

try (var control = new FfmEpromControl(Path.of("ffm-shim"))) {   // dir with the .so
    control.open();                                   // opens the real TL866
    byte[] image = control.read(FfmEpromControl.DeviceType._27C256, null);
    // ... back up, verify, etc.
}
```

Running requires `--enable-native-access` for the owning module/jar
(e.g. `--enable-native-access=ALL-UNNAMED` on the classpath), a JDK rule for
FFM, not a limitation of the shim.

## Verified

Built on Linux (vendored libusb, `-fPIC`); exports all eight `cffm_*` symbols.
`DynamiteConnector` resolves the `.so`, `FfmEpromControl` binds every downcall
handle, and `open()` reaches the libusb path — returning `NO_DEVICE` when no
TL866 is attached, which proves the full Java→C→libusb chain executes. Live
read/write/erase require a physical programmer and a UV eraser (or a reusable
pin-compatible part) and are gated by the usual safety posture: **back up stock
first, verify every write.**
