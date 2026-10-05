# vendor/ — bundled third-party dependencies (opt-in)

This directory builds a **vendored** copy of libusb from the source archive at
[`../include/libusb-1.0.30.zip`](../include/libusb-1.0.30.zip), so the Corrado
tool can be built on a machine with **no system libusb installed**. It is
entirely **opt-in** — the normal build still uses a system libusb.

## Licensing — important

> **libusb is third-party software, licensed LGPL-2.1-or-later**, © its
> authors. It is **not** under this repository's MIT licence. The full licence
> and author list travel with the code inside `include/libusb-1.0.30.zip`
> (`COPYING`, `AUTHORS`). The MIT licence at the repo root covers Corrado's own
> code, **not** libusb.
>
> The small files in `vendor/libusb/` that this repo authored —
> `config.linux.h` and the `Makefile` — are MIT and contain no libusb source;
> they only *select build options* for, and compile, the upstream source.

Under LGPL-2.1, linking is permitted; if you distribute a binary built against
libusb you must preserve libusb's licence/attribution and allow the libusb
portion to be replaced (static linking is allowed provided you also make the
object/relink means available — see the LGPL text in the archive).

## What the vendored build produces

`vendor/libusb/` compiles libusb's core plus the **Linux usbfs + netlink**
backend (POSIX threads), deliberately with **no libudev dependency**, into:

```
vendor/libusb/staged/
├── libusb-1.0.30/…            # unzipped upstream source (git-ignored)
├── include/libusb-1.0/libusb.h # staged public header
└── lib/libusb-1.0.a            # static library
```

`staged/` is a build artifact and is git-ignored; only the source `.zip`
(tracked in `include/`) and the two MIT helper files here are committed.

## Usage

Build Corrado against the vendored libusb (Linux):

```sh
# all years, bundled libusb (no system libusb/libudev needed):
make -C source/linux LIBUSB_VENDOR=1

# a single year:
make -C source/linux YEAR=1992 LIBUSB_VENDOR=1
```

Or build just the vendored library on its own:

```sh
make -C vendor/libusb          # -> staged/lib/libusb-1.0.a
make -C vendor/libusb clean
```

Without `LIBUSB_VENDOR=1`, the build links the **system** `-lusb-1.0` exactly
as before (see the Dependencies table in the top-level README).

## Scope

- **Linux only** here. The vendored path targets the Linux usbfs/netlink
  backend. For macOS and Windows, install libusb the normal way (Homebrew /
  MSYS2 / the MSVC `.7z`), as the top-level README describes — those backends
  rely on system frameworks that are simpler to obtain than to vendor.
- This is a convenience/offline path. For most users a packaged system libusb
  is still the recommended route.
