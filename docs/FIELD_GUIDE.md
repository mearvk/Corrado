# Corrado ECU EPROM — Field Guide for Working Mechanics

A no-nonsense, bench-side guide for the person who actually pulls the ECU,
reads the chip, and flashes a tune. It assumes you know your way around a car
and a soldering iron, and focuses on *not bricking a customer's ECU*.

> Read [`HARDWARE.md`](HARDWARE.md) once for the chip/programmer facts. This
> guide is the day-to-day workflow built on top of it.

## 0. The one rule

**Always keep a verified copy of the stock chip before you write anything.**
The tool enforces this for you: `write` automatically reads and saves the
current chip to a timestamped `.bin` *before* programming, and aborts if that
backup fails. You should not need `--force`; if you reach for it, stop and ask
why the backup failed first.

## 1. Before you touch the car — check your bench

Run the built-in setup check. It tells you in plain language what is and isn't
ready, with the exact fix for each problem:

```sh
corrado-eprom doctor
```

Typical output when you're good to go:

```
  [ OK ] USB subsystem (libusb) initialised
  [ OK ] Programmer detected: TL866II+
         family: minipro
  Ready: the tool can talk to your programmer.
```

If it says **no programmer found** on Linux, you almost always need the udev
rule + `plugdev` group (doctor prints the commands). On Windows you need the
WinUSB binding via Zadig.

See every programmer the tool supports — no device required:

```sh
corrado-eprom list
```

## 2. Identify the car and chip

```sh
corrado-eprom info
```

This prints the model year, engine, ECU system and part number, and the EPROM
device the per-year binary is built for. Confirm it against the markings on
*your* ECU and chip — the per-year profile is a default, not gospel.

| Years     | Engine | ECU            | Stock EPROM |
|-----------|--------|----------------|-------------|
| 1990–1992 | G60    | Bosch Digifant | 27C256      |
| 1992–1995 | VR6    | Bosch Motronic | 27C256      |

A 27C512 twin-tune adapter (two switchable maps in one 64 KB part) is also
supported.

## 3. The safe flashing workflow

```sh
# 1. Back up the stock chip to a named file (do this even though write
#    also auto-backs-up -- a labelled stock file is worth keeping).
corrado-eprom backup stock-customer-car.bin

# 2. (Optional) sanity-check / repair the trailing checksum on your tune
#    file BEFORE writing. No hardware needed.
corrado-eprom checksum mytune.bin --fix

# 3. Write. The tool auto-backs-up the current chip, programs, then
#    read-back-verifies. A verify mismatch is reported as an error.
corrado-eprom write mytune.bin

# 4. Independently verify against your file afterwards if you want a second
#    confirmation.
corrado-eprom verify mytune.bin
```

If a write reports a verify mismatch: **do not ship the car.** Re-seat the
chip, check it isn't a tired/worn part, and write again. If it keeps failing,
the chip or socket is suspect.

## 4. Cloning a chip (chip → chip)

```sh
corrado-eprom copy
```

It reads the SOURCE (master) chip into memory, prompts you to swap in the
TARGET (blank) chip, then programs and verifies it. Good for duplicating a
known-good tune onto fresh parts.

## 5. Erasing — UV vs. electrical (the gotcha)

A genuine **27C256** is a UV-erasable (often one-time-programmable) part. It
**cannot** be erased electrically. `corrado-eprom delete` sends an electrical
erase and will report `unsupported` for a true UV/OTP chip — that is expected:

- **27C-series UV parts:** pull the chip, expose the quartz window to a UV
  eraser for ~15–30 minutes. OTP parts can't be reused at all.
- **Reusable drop-ins** people fit for repeated tuning (SST27SF512, Winbond
  W27C512, 28C256 EEPROM, …): `delete` erases and blank-checks them.

Check an erased part reads all `0xFF`:

```sh
corrado-eprom blankcheck
```

## 6. Chip handling (the boring stuff that saves ECUs)

- **ESD:** these are old MOS parts. Wrist strap, grounded mat. One careless
  touch on a dry day can wound a chip so it reads fine today and fails next
  month.
- **Orientation:** pin 1 toward the ZIF lever. A backwards insertion can cook
  the part.
- **Legs:** straighten bent legs before seating; a leg folded under the body
  gives intermittent reads that look like a "bad tune."
- **Socket wear:** if you flash a lot, the ECU's original solder-in socket
  gets tired. A fresh machined-pin socket on the bench adapter beats chasing
  phantom verify failures.

## 7. Picking a programmer for shop use

All supported units read/write the same images; the difference is throughput
and ergonomics (run `corrado-eprom list` for the live table):

- **TL866II+ / XGecu T48 / T56** (`minipro` family) — the workhorses. The T48
  and T56 are the current-production successors; get one of these for a new
  bench.
- **CH341A dongle** (`ch341a-spi`) — cheap, fine for SPI NOR parts, but slow
  and fiddly. Keep one as a backup.
- **CH347** (`ch347-hs-spi`) — USB 2.0 high-speed; the fastest option here and
  the one to reach for when you're flashing part after part all day.

## 8. When something looks wrong

| Symptom | Likely cause | Do this |
|---|---|---|
| `no programmer found` | permissions / not plugged / wrong driver | `corrado-eprom doctor` and follow its fix |
| read differs every time | bent leg, ESD-wounded chip, tired socket | re-seat; try a known-good chip/socket |
| write → verify mismatch | worn chip, bad seating, wrong part | re-seat; replace chip; never ship on a mismatch |
| `unsupported` on `delete` | genuine UV/OTP 27C part | use a UV eraser, or fit a reusable drop-in |
| checksum MISMATCH on a file | edited map without fixing the sum word | `checksum <file> --fix` before writing |

---

*This guide is practical workshop advice, not a warranty. Reprogramming an
engine ECU can make a vehicle unsafe or non-compliant — verify every write and
keep the stock image.*
