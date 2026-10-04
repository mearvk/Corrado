# Corrado EPROM Maps — G60 (Digifant) and VR6 (Motronic)

This directory documents the layout of the Corrado engine-ECU calibration
EPROM and ships **reference images** the `corrado-eprom` tool can read,
write, verify and checksum end-to-end.

## What's here

| File | What it is |
|------|-----------|
| `G60_StockEprom_REFERENCE.bin` | Synthetic, MIT-licensed reference image for the G60 (Bosch Digifant). 32,768 bytes, valid checksum. |
| `VR6_StockEprom_REFERENCE.bin` | Synthetic, MIT-licensed reference image for the VR6 (Bosch Motronic). 32,768 bytes, valid checksum. |
| `make_reference_eproms.py` | Generator for the two reference images above. |
| `fetch-eproms.sh` | Downloads **real community** G60 dumps for actual tuning (see licensing note). |

### Why reference (synthetic) images, not real dumps?

A real stock calibration dump is someone's copyrighted data. The freely
available community G60 dumps are published **without an open licence**
(all rights reserved), and freely redistributable VR6 Corrado Motronic
dumps are not available at all — they sit behind paywalled ECU-file
archives. Rather than redistribute files we have no licence to, this repo
ships **structurally faithful synthetic images** (our own work, MIT) plus a
`fetch-eproms.sh` helper so you can pull the real community dumps yourself.
The reference images reproduce the real layout — fill bytes, data regions,
part-number strings, representative tables and a valid checksum — so the
tool, the XDF/offset docs and tests all work without any third-party data.

> The region layout documented below was derived by **analysing the
> structure** (not copying the contents) of a community stock G60 Digifant
> dump (`"Stock G60 Single Ignition Map.BIN"`, 32,768 bytes). Only the
> layout — which byte ranges hold data — is reproduced here; the actual
> calibration values are our own synthetic ramps.

---

## Per-year folders

The stock calibration differs by **engine/ECU**, which maps to model-year
ranges — not every single year has a unique image. Each `eproms/<year>/`
folder holds the reference image(s) appropriate for that year:

| Year(s)            | ECU                | VW part    | Reference bin(s) in `eproms/<year>/` |
|--------------------|--------------------|------------|--------------------------------------|
| 1990, 1991         | Digifant (G60)     | 037906022B | `Corrado_<yr>_G60_Digifant_REFERENCE.bin` |
| **1992**           | **both** (G60 + VR6 introduced) | — | G60 **and** VR6 reference bins |
| 1993–1996          | Motronic (VR6)     | 021906259  | `Corrado_<yr>_VR6_Motronic_REFERENCE.bin` |

So 1990 and 1991 share the G60 calibration, 1993–1996 share the VR6
calibration, and 1992 (the transition year, when the VR6 arrived alongside
the G60) carries both. The files are copies of the two canonical reference
images in this directory, named per year for convenience.

> These remain **synthetic MIT reference** images (see below). Real stock
> calibrations also varied by market (Euro/US/California), transmission and
> ECU revision within a year — always confirm against your own ECU label
> and a matching TunerPro/WinOLS XDF. Use `fetch-eproms.sh` for real
> community G60 dumps.

## 1. Device

Both ECUs store the calibration in a **27C256**-class 28-pin DIP EPROM:

- **32,768 bytes** (32K × 8, 256 Kbit), single +5 V.
- Erased state reads **0xFF**; this project's images use a fill byte to mark
  unused space (the observed G60 dump uses `0x41` = ASCII `'A'`).
- A **trailing little-endian checksum word** occupies the last two bytes
  (`0x7FFE..0x7FFF`). `corrado-eprom checksum <file> --fix` recomputes it so
  that `storedWord == (0 - sum_of_all_other_bytes) mod 65536`.

---

## 2. G60 (Digifant) image map

Observed region layout of a stock 32 KB G60 Digifant image:

```
0x0000 ┬─────────────────────────────────────────────┐
       │  program / fill area                         │  mostly 0x41 fill
0x4000 ┼─────────────────────────────────────────────┤
       │  PRIMARY MAP BLOCK                           │
       │   0x4000  16x16 fuel map (pulse width)       │
       │   0x4100  16x16 ignition advance map         │
       │   0x4200  boost / load limit curve           │
0x5500 ┼─────────────────────────────────────────────┤  fill
0x6000 ┼─────────────────────────────────────────────┤
       │  SECONDARY MAP BLOCK                          │
       │   0x6000  warm-up / enrichment map           │
       │   0x6100  rev-limit ramp                      │
0x6F00 ┼─────────────────────────────────────────────┤  fill
0x7F00 ┼─────────────────────────────────────────────┤
       │  IDENTITY / VECTORS                           │
       │   0x7F00  ASCII part + engine code            │
       │   0x7FEE  idle rpm scale (ref)                │
       │   0x7FEF  base fuel-pressure index (ref)      │
       │   0x7FFE  checksum word (LE)                  │
0x8000 └─────────────────────────────────────────────┘
```

> Real-dump note: in the analysed community G60 image the content regions
> fell at **0x4000–0x55FF**, **0x6000–0x6FFF** and **0x7F00–0x7FFF**, with
> the rest `0x41` fill. The exact in-block table offsets vary by ECU
> revision — always confirm against a TunerPro XDF for your specific bin
> (e.g. the G60 1-ignition vs 3-ignition map definitions).

### G60 reference hex excerpts

Identity block at `0x7F00`:

```
007f00  56 57 20 30 33 37 39 30 36 30 32 32 42 20 44 49  VW 037906022B DI
007f10  47 49 46 41 4e 54 20 47 36 30 20 50 47 20 52 45  GIFANT G60 PG RE
007f20  46 41 41 41 41 41 41 41 41 41 41 41 41 41 41 41  FAAAAAAAAAAAAAAA
007f40  43 4f 52 52 41 44 4f 20 31 2e 38 20 47 36 30 20  CORRADO 1.8 G60 
007f50  31 36 30 50 53 20 32 37 43 32 35 36 20 52 45 46  160PS 27C256 REF
```

Fuel map head at `0x4000` (rows = load, columns = rpm; values ramp up):

```
004000  30 35 3b 41 47 4d 53 59 5e 64 6a 70 76 7c 82 88
004010  3b 40 46 4c 52 58 5e 64 69 6f 75 7b 81 87 8d 93
004020  47 4c 52 58 5e 64 6a 70 75 7b 81 87 8d 93 99 9f
004030  53 58 5e 64 6a 70 76 7c 81 87 8d 93 99 9f a5 ab
```

Checksum tail at `0x7FE0` (idle/FP refs at `0x7FEE/EF`, checksum at `0x7FFE`):

```
007fe0  41 41 41 41 41 41 41 41 41 41 41 41 41 41 1e 1c
007ff0  41 41 41 41 41 41 41 41 41 41 41 41 41 41 20 a0
                                                    └──┴─ checksum word (LE)
```

---

## 3. VR6 (Motronic) image map

The VR6 OBD1 Motronic calibration also fits a 27C256-class part. Typical
layout of the reference image:

```
0x0000 ┬─────────────────────────────────────────────┐
       │  0x0000  ASCII part + engine code             │
       │  0x0040  model/engine descriptor              │
0x2000 ┼─────────────────────────────────────────────┤
       │  0x2000  16x16 fuel map                       │
       │  0x2400  16x16 ignition advance map           │
       │  0x2800  16x16 lambda / enrichment map        │
       │  0x2C00  rev-limit / fuel-cut ramp (~6500)    │
0x3000 ┼─────────────────────────────────────────────┤  0xFF (erased) fill
0x7FFE ┼─────────────────────────────────────────────┤
       │  checksum word (LE)                           │
0x8000 └─────────────────────────────────────────────┘
```

### VR6 reference hex excerpts

Identity block at `0x0000`:

```
000000  56 57 20 30 32 31 39 30 36 32 35 39 20 4d 4f 54  VW 021906259 MOT
000010  52 4f 4e 49 43 20 56 52 36 20 41 42 56 20 52 45  RONIC VR6 ABV RE
000040  43 4f 52 52 41 44 4f 20 32 2e 38 2f 32 2e 39 20  CORRADO 2.8/2.9 
000050  56 52 36 20 31 37 38 2d 31 39 30 50 53 20 32 37  VR6 178-190PS 27
```

Fuel map head at `0x2000`:

```
002000  28 2e 34 3b 41 48 4e 54 5b 61 68 6e 74 7b 81 88
002010  34 3a 40 47 4d 54 5a 60 67 6d 74 7a 80 87 8d 94
002020  41 47 4d 54 5a 61 67 6d 74 7a 81 87 8d 94 9a a1
002030  4e 54 5a 61 67 6e 74 7a 81 87 8e 94 9a a1 a7 ae
```

> VR6 Motronic map offsets vary between the 2.8 (AAA) and 2.9 (ABV) and
> across software revisions. The offsets above are the reference image's;
> for a real dump, define maps with the appropriate TunerPro/WinOLS XDF.

---

## 4. Using these with the tool

```sh
# Inspect / verify the reference images (no hardware needed):
corrado-eprom checksum eproms/G60_StockEprom_REFERENCE.bin
corrado-eprom checksum eproms/VR6_StockEprom_REFERENCE.bin

# Program a (reference or real) image to a chip and verify:
corrado-eprom write eproms/G60_StockEprom_REFERENCE.bin

# Regenerate the reference images:
python3 eproms/make_reference_eproms.py

# Pull REAL community G60 dumps for actual tuning (see licensing note):
./eproms/fetch-eproms.sh
```

---

## 5. Licensing / attribution

- The **reference `.bin` images and `make_reference_eproms.py` are MIT**,
  part of this repository.
- The region-layout analysis references the structure of a community G60
  Digifant dump hosted in the public GitHub repo
  [`YOU54F/PoloG40Digifant`](https://github.com/YOU54F/PoloG40Digifant).
  That repo publishes no licence, so **its binaries are not included here**;
  `fetch-eproms.sh` downloads them to your machine on demand. Respect the
  upstream authors' rights and use real calibration data at your own risk.
- No freely/openly-licensed stock **VR6 Corrado Motronic** dump was found;
  such files are typically sold through ECU-file archives. The VR6 reference
  here is synthetic.

*Content was rephrased for compliance with licensing restrictions.*
