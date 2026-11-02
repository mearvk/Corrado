# Corrado ECU EPROM — Hardware Notes

This document summarises the chip and programmer facts the software is
built around. Always confirm against the markings on *your* ECU and chip
before erasing or writing anything.

## The ECU and its EPROM

The Volkswagen Corrado (Typ 53I, model years 1990–1996) used two Bosch
engine-management systems depending on the engine:

| Years      | Engine                       | ECU system      | Typical EPROM |
|------------|------------------------------|-----------------|---------------|
| 1990–1995  | G60 1.8 supercharged (PG)    | Bosch Digifant  | 27C256 (32 KB)|
| 1992–1996  | VR6 2.8 (ABV)                | Bosch Motronic  | 27C256 (32 KB)|

The calibration ("tune") lives in a **socketed 28-pin DIP EPROM** of the
**27C256** family:

- Organisation: **32K × 8 = 32,768 bytes** (256 Kbit).
- Single **+5 V** supply, TTL-level programming signals.
- UV-erasable (quartz window). A blank/erased cell reads as `1`, so a
  blank image is all `0xFF`.
- A **27C512** (64 KB) is used by twin-tune switch adapters that drop into
  the same 28-pin socket; this suite supports it as an alternate device.

> The exact VW part numbers in the per-year `profile.c` files are starting
> points — e.g. Digifant ECU `0261200280` / `037906022B`. Read the label on
> your own ECU and edit the profile if it differs.

## Removing and handling the chip

1. The EPROM is socketed — lever it out gently and evenly with a PLCC/DIP
   puller to avoid bending pins.
2. Observe ESD precautions; the parts are decades old.
3. To re-use an existing (non-OTP) EPROM you must erase it first under a
   UV eraser (typically 15–30 minutes). OTP parts cannot be reused.

## USB programmer

The reference programmers are the **MiniPRO / XGecu minipro family**, the
common open-hardware EPROM programmers, driven over `libusb` the same way the
open-source `minipro` project drives them. A second family, the WinChipHead
**CH341A** USB-to-SPI dongle, is also supported for SPI NOR parts. Run
`corrado-eprom list` to print every supported programmer and its family.

USB identifiers used by the drivers (minipro IDs match the upstream `minipro`
udev rules; the CH341A id is the standard WinChipHead SPI bridge):

| Model            | Family       | VID      | PID      |
|------------------|--------------|----------|----------|
| TL866A / TL866CS | `minipro`    | `0x04D8` | `0xE11C` |
| TL866II+         | `minipro`    | `0xA466` | `0x0A53` |
| XGecu T48        | `minipro`    | `0xA466` | `0x0001` |
| XGecu T56        | `minipro`    | `0xA466` | `0x0053` |
| XGecu T76        | `minipro`    | `0xA466` | *(tbc)*  |
| CH341A (SPI)     | `ch341a-spi` | `0x1A86` | `0x5512` |

The XGecu **T76** is recognised by name but its USB product id is not yet
confirmed from a citable source, so its registry entry is marked `probe=0`
and it is not auto-detected until the real PID is filled in.

Two **protocol families** are implemented and the backend dispatches on the
family field: `minipro` (8-byte LE command header over bulk endpoints) and
`ch341a-spi` (the CH341 `0xA8` SPI-stream command carrying JEDEC SPI-flash
opcodes — `0x03` read, `0x02` page-program, `0xC7` chip-erase — in
`source/corrado_ch341.c`).

The set of supported programmers is **data, not code**: it lives in the
`corrado_prog_desc_t` registry (`usb-driver/corrado_prog.h`,
`source/corrado_prog.c`), a table pairing each VID/PID with its model
identity, protocol family and transport parameters (endpoints, block size,
read/write/erase opcodes). Every per-OS backend walks that table to find and
drive an attached device. Adding a programmer in an existing family is a
one-line entry; adding a new family is one protocol module plus a dispatch
branch.

Insert the 27C256 into the ZIF socket at the position the programmer marks
for a 28-pin device (pin 1 toward the lever), then use the CLI:

```
corrado-eprom read   stock.bin        # back up the original first!
corrado-eprom write  mytune.bin       # program + auto verify
corrado-eprom verify mytune.bin       # compare chip to a file
corrado-eprom blankcheck              # confirm an erased chip
corrado-eprom backup                  # read chip -> timestamped file
corrado-eprom copy                    # clone one chip to another
corrado-eprom delete                  # electrically erase (reusable parts)
corrado-eprom checksum mytune.bin --fix   # repair trailing checksum
```

### Erasing: UV vs. electrical

The `delete` command sends an electrical chip-erase. A real **27C256** is a
UV-erasable (often OTP) EPROM: it **cannot** be erased this way. For those
parts you must pull the chip and expose the quartz window to a UV eraser
(typically 15–30 minutes); the tool reports `unsupported` and tells you so.
The electrical erase succeeds only on reusable, pin-compatible replacements
(e.g. SST27SF512, Winbond W27C512, or a 28C256 EEPROM) that are commonly
fitted to these sockets for repeated tuning.

**Always keep a verified backup of the stock image before writing.**

## Safety / scope

The protocol framing in the drivers is a readable reference of the
read/write/verify path for 27C-series parts. For production flashing of a
specific TL866 firmware revision, cross-check the command framing against
the upstream `minipro` sources for your exact unit.
