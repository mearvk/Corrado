# Corrado EPROM Programmer Suite

Cross-platform tooling to read, write, verify and checksum the engine-ECU
**EPROM of a Volkswagen Corrado (model years 1990–1996)** — the G60
(Bosch Digifant) and VR6 (Bosch Motronic) cars — using a MiniPRO **TL866**
family USB chip programmer.

Supports **Windows 10+, Linux, and macOS**, with build systems, source,
and USB drivers (in both **C** and **C++**) for all three.

> ⚠️ Reprogramming an engine ECU can make a vehicle unsafe or non-compliant.
> Always back up the stock image first and verify every write. See
> [`docs/HARDWARE.md`](docs/HARDWARE.md).

**Background reading:** [`CORRADO.md`](CORRADO.md) is a full reference on the
car, its engines, the ECU/EPROM, and an **HP-evaluation chart** for EPROM
tuning (raising boost and fuel pressure for more power/torque).

**Tuning to 260+ WHP:** [`TUNERZ.md`](TUNERZ.md) is a build guide — upgrade
parts sorted by HP/torque gain with manufacturer, era and cost, plus factory
MPG by year and average US fuel prices.

**EPROM maps:** [`eproms/EPROM_MAPS.md`](eproms/EPROM_MAPS.md) documents the
G60 (Digifant) and VR6 (Motronic) image layouts with hex excerpts, ships
MIT-licensed reference `.bin` images the tool can read/write/checksum, and
includes `fetch-eproms.sh` to pull real community G60 dumps.

**Factory audio:** [`audio/`](audio/) documents the Corrado's 1990–1996
factory stereo systems (standard vs. premium head units and speakers) and
audio wiring, with SVG wiring/speaker-layout diagrams.

**Regional (EU vs. US):** [`regional/`](regional/) indexes the
European-vs-American differences, with region-specific docs sorted under
each area's `regional/europe/` and `regional/america/` folders (currently
EPROM/ECU and audio).

**Photos:** [`images/`](images/) curates 8 CC-licensed Corrado photos (4
G60, 4 VR6) from Wikimedia Commons — run `images/fetch-images.sh` to
download them (the binaries aren't committed; full attribution in
[`images/README.md`](images/README.md)).

## The target chip

The Corrado ECU calibration lives in a socketed 28-pin **27C256** EPROM
(32K × 8 = 32,768 bytes, +5 V, UV-erasable). A 27C512 (64 KB) twin-tune
adapter is also supported. Full details: [`docs/HARDWARE.md`](docs/HARDWARE.md).

## Repository layout

```
Corrado/
├── Makefile                     # top-level: detects OS, delegates
├── include/
│   ├── corrado_eprom.h          # EPROM/image/checksum API (OS-independent)
│   └── corrado_usb.h            # USB programmer API (OS-independent)
├── source/
│   ├── corrado_eprom.c          # shared image + checksum implementation
│   ├── corrado_ops.c            # shared backup/copy/delete operations
│   ├── corrado_profile.h        # per-year ECU profile interface
│   ├── main.c                   # shared CLI front end
│   ├── linux/
│   │   ├── Makefile
│   │   └── 1990/ … 1996/        # per-year profile.c
│   ├── windows/
│   │   ├── Makefile             # MinGW (gcc)
│   │   ├── Makefile.msvc        # MSVC (nmake)
│   │   └── 1990/ … 1996/
│   └── macos/
│       ├── Makefile
│       └── 1990/ … 1996/
├── usb-driver/
│   ├── corrado_usb.hpp          # portable C++ RAII wrapper (all 3 OSes)
│   ├── example_cpp.cpp          # C++ usage example
│   ├── linux/   corrado_usb_linux.c   + 60-corrado-tl866.rules (udev)
│   ├── windows/ corrado_usb_windows.c (libusb/WinUSB)
│   └── macos/   corrado_usb_macos.c   (libusb)
├── build/                       # build output: build/<os>/<year>/
└── docs/HARDWARE.md
```

Each `source/<os>/<year>/profile.c` encodes that model year's engine, ECU
system, VW part number, and stock EPROM device, so the resulting binary
knows what chip it is talking to.

## Dependencies

All three drivers use **libusb-1.0** as the USB transport.

| OS       | Install libusb                                   | Compiler            |
|----------|--------------------------------------------------|---------------------|
| Linux    | `apt install libusb-1.0-0-dev` / `dnf install libusb1-devel` | `gcc`/`clang` |
| macOS    | `brew install libusb pkg-config`                 | Xcode `clang`       |
| Windows  | libusb-1.0 (MinGW or MSVC build), bind device via Zadig to WinUSB | MinGW `gcc` or MSVC `cl` |

## Build

From the repo root, the top-level Makefile detects your OS:

```sh
make            # build every model year for this OS -> build/<os>/<year>/
make YEAR=1992  # build just one year
make info       # show detected configuration
make clean
```

Or invoke a platform Makefile directly:

```sh
# Linux / macOS
make -C source/linux
make -C source/macos YEAR=1995

# Windows, MinGW
mingw32-make -C source\windows

# Windows, MSVC (from a VS Native Tools prompt)
nmake /f source\windows\Makefile.msvc LIBUSB_DIR=C:\libusb
```

Binaries land in `build/<os>/<year>/corrado-eprom[.exe]`.

### Linux device permissions

Install the udev rule so a non-root user can access the programmer:

```sh
sudo cp usb-driver/linux/60-corrado-tl866.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
sudo usermod -aG plugdev "$USER"   # then log out/in
```

## Usage

```sh
corrado-eprom info                  # show the year/ECU/EPROM profile
corrado-eprom read   stock.bin      # dump the chip (back up first!)
corrado-eprom write  mytune.bin     # program the chip, then auto-verify
corrado-eprom verify mytune.bin     # compare chip against a file
corrado-eprom blankcheck            # confirm the chip is erased (all 0xFF)
corrado-eprom backup [out.bin]      # read chip -> file (auto-timestamped)
corrado-eprom copy                  # clone one chip to another (prompts a swap)
corrado-eprom delete                # electrically erase the chip (reusable parts)
corrado-eprom checksum mytune.bin --fix   # check/repair trailing checksum
```

### Backup, copy and delete

- **backup** — reads the whole chip to a raw binary file. With no filename
  it writes an auto-timestamped name like
  `corrado-1992-27C256-20260104-192154.bin`. This is the chip→file
  direction; always back up a stock chip before writing.
- **copy** — chip→chip clone. It reads the SOURCE (master) chip into memory,
  prompts you to swap in the TARGET chip, then programs and verifies it.
- **delete** — electrically erases the chip and blank-checks it.
  > A genuine 27C-series part is **UV-erasable / one-time-programmable** and
  > **cannot** be erased electrically — the tool reports this and you must
  > remove the chip and use a UV eraser. `delete` succeeds on pin-compatible
  > reusable replacements (SST27SF512, Winbond W27C512, 28C256 EEPROM, …)
  > that people fit to these sockets.

The `checksum` command works on a file alone (no hardware needed) and can
repair the Digifant/Motronic-style trailing checksum word so the 16-bit
sum of the image is zero.

## C vs C++ drivers

- **C** — one backend per OS under `usb-driver/<os>/`, all implementing the
  same `corrado_usb.h` interface. The CLI links against the matching one.
- **C++** — `usb-driver/corrado_usb.hpp` is a single portable RAII wrapper
  (exceptions, `std::vector<uint8_t>` images, `std::function` progress
  callbacks) over whichever C backend is compiled in. See
  `usb-driver/example_cpp.cpp`.

## Status / caveats

This is a complete, compiling scaffold with a verified image/checksum
library and CLI. The TL866 bulk-protocol framing in the drivers is a
readable reference of the read/write/verify path; before flashing a
specific TL866 firmware revision for real, cross-check the framing against
the upstream open-source `minipro` project for your exact unit, and
confirm the per-year VW part numbers against your own ECU.

## License

MIT — see [`LICENSE`](LICENSE).
