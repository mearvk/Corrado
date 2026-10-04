# The Volkswagen Corrado — Complete Reference

A comprehensive reference for the Volkswagen Corrado: the car, its engines
and engine-management systems, the calibration **EPROM** this project reads
and writes, and a horsepower-evaluation chart for EPROM tuning (where fuel
pressure and boost are raised for more power and torque).

> This document is background and engineering reference for the
> `corrado-eprom` tool in this repository. See also
> [`docs/HARDWARE.md`](docs/HARDWARE.md) for the chip/programmer wiring
> details and [`README.md`](README.md) for the software.
>
> ⚠️ **Safety:** every tuning figure below assumes supporting hardware,
> fresh fuel of the stated octane, a healthy engine, and dyno verification.
> Raising boost and fuel pressure without matching fuelling, cooling and
> knock margin will destroy an engine. Always keep a verified **backup of
> the stock EPROM image** before writing (`corrado-eprom backup`).

---

## 1. The car at a glance

| Attribute        | Detail |
|------------------|--------|
| Manufacturer     | Volkswagen (built by **Karmann**, Osnabrück, Germany) |
| Type             | 3-door sport compact coupé, 2+2, liftback |
| Layout           | Front engine, front-wheel drive (transverse) |
| Platform         | VW Group **A2** (shared with Golf Mk2); later VR6 widened track |
| Designer         | Herbert Schaefer |
| Production       | **1 September 1988 – 31 July 1995** |
| Model years      | 1988–1995 (Europe); **US sales from 1990**, Canada from 1989 |
| Predecessor      | Scirocco II (project codename "Taifun"/Typhoon) |
| Signature detail | Speed-active **rear spoiler** (deploys ~120 km/h in Germany) |
| Boot capacity    | ~300 litres |
| Length           | ~4,048 mm |

The Corrado sat above the Scirocco in VW's range and was marketed as a
genuine sports coupé. The automatically extending rear spoiler was a party
piece often compared to the Porsche 964's.

*Sources: Volkswagen Newsroom (Corrado 1988–1995), conceptcarz, Hemmings,
Jalopnik. Content was rephrased for compliance with licensing restrictions.*

---

## 2. Engine lineup

The Corrado was offered with four main engines over its life. Figures are
manufacturer ratings (PS = metric horsepower; 1 PS ≈ 0.9863 hp).

| Engine                | Code | Disp.   | Valves | Induction        | Power (kW / PS / hp) | Torque        | ECU / fuelling     |
|-----------------------|------|---------|--------|------------------|----------------------|---------------|--------------------|
| 1.8 16V               | PL/… | 1,781cc | 16     | Naturally asp.   | 100 / 136 / 134      | ~158 Nm       | Digifant / KE-Motronic |
| 2.0 16V               | 9A   | 1,984cc | 16     | Naturally asp.   | 100 / 136 / 134      | ~180 Nm       | Digifant           |
| **1.8 G60**           | **PG** | 1,781cc | 8    | **G-Lader supercharger** | **118 / 160 / 158** | **225 Nm @ 3,600** | **Bosch Digifant** |
| **2.9 VR6** (Europe)  | **ABV** | 2,861cc | 12   | Naturally asp.   | **140 / 190 / 187**  | **245 Nm @ 4,200** | **Bosch Motronic** |
| 2.8 VR6 (US / early)  | AAA  | 2,792cc | 12     | Naturally asp.   | 128 / 178 / 178      | 177 lb-ft (~240 Nm) | Bosch Motronic |

Notes:
- The **G60** is the model this project is primarily aimed at: a 1.8-litre
  8-valve four force-fed by VW's **G-Lader** scroll supercharger, engine
  code **PG**, running **Bosch Digifant** with a socketed tuning EPROM.
- The **VR6** (narrow-angle 15° "V" sharing one head) arrived for 1992; it
  runs **Bosch Motronic**. Europe got the 2.9 (ABV, 190 PS); North America
  got the 2.8 (AAA, 178 hp).
- Early non-supercharged 16V cars used Digifant / KE-Motronic depending on
  market and year.

*Sources: VW Newsroom engine-versions table, encycarpedia, ultimatespecs,
Hagerty, Wikipedia (Corrado / G60 / VR6). Content was rephrased for
compliance with licensing restrictions.*

### 2.1 The G-Lader supercharger (G60)

The **G-Lader** is a scroll-type ("spiral") supercharger. A displacer
orbits within nested spiral chambers, pumping air toward the centre outlet
without reciprocating valves. Characteristics relevant to tuning:

- Delivers useful boost from low rpm — strong low/mid torque.
- Stock boost on the Corrado G60 is roughly **0.6–0.7 bar** (≈9–10 psi).
- The drive **pulley diameter sets boost**: a smaller pulley spins the
  charger faster for more boost (e.g. the common swap from the stock
  **70 mm to a 68 mm or 65 mm pulley**).
- The G-Lader is a **wear item**: the apex seals/strips need periodic
  rebuilding, and over-driving it (too-small pulley, high rpm) shortens its
  life. "Without the right chip, a smaller pulley is fatal" is the common
  tuner warning — boost and fuelling must be matched in the EPROM.

*Sources: Wikipedia (G-Lader), bar-tek G-Lader tips. Content was rephrased
for compliance with licensing restrictions.*

---

## 3. The engine-management EPROM

The whole point of the `corrado-eprom` tool: the engine calibration (the
fuel, ignition and boost-related maps) lives in a **socketed, removable
EPROM** on the ECU board.

| Property        | Value |
|-----------------|-------|
| ECU (G60)       | Bosch **Digifant** (e.g. 0261200280 / VW 037906022B) |
| ECU (VR6)       | Bosch **Motronic** |
| Memory device   | **27C256** family — 28-pin DIP EPROM |
| Capacity        | **32,768 bytes** (32K × 8, 256 Kbit) |
| Supply          | single **+5 V**, TTL-level programming signals |
| Erase           | **UV light** (quartz window); genuine parts are UV/OTP |
| Erased state    | all bytes read **0xFF** |
| Twin-tune       | 27C512 (64 KB) adapters supported for A/B tunes |

### What the maps contain (conceptually)

A Digifant/Motronic calibration image holds, among other tables:

- **Fuel / injection** maps — injector pulse width vs. load and rpm, and
  enrichment under boost.
- **Ignition timing** maps — advance vs. load and rpm; this is pulled back
  under boost to control knock.
- **Boost / load limits** and **rev limit** — the ceilings the ECU enforces.
- **Fuel-pressure expectations** — the calibration assumes a given base
  fuel pressure; raising it (e.g. an **adjustable fuel pressure regulator**)
  shifts the whole fuel delivery curve and must be reflected in the map.
- A **checksum** word so the ECU can validate the image. The tool's
  `checksum --fix` recomputes it after edits.

### Why "tuning" means editing this chip

Because the calibration is a swappable 32 KB EPROM, you can read the stock
image, modify the maps (richer fuelling and adjusted timing to support more
boost), fix the checksum, and burn it back — exactly the read / edit /
write / verify cycle this tool implements:

```
corrado-eprom backup stock.bin      # 1. ALWAYS save the original
# ... edit maps in a tuning editor (e.g. TunerPro) ...
corrado-eprom checksum tune.bin --fix
corrado-eprom write tune.bin        # 2. burn + auto-verify
```

**Image maps & reference binaries:** the actual byte layout of the G60
(Digifant) and VR6 (Motronic) images — data regions, part-number strings,
map offsets and the checksum tail — is documented in
[`eproms/EPROM_MAPS.md`](eproms/EPROM_MAPS.md), which also ships
MIT-licensed reference `.bin` images and a `fetch-eproms.sh` helper for real
community G60 dumps.

*Sources: alldatasheet/Microchip 27C256 datasheet, chiptuning ECU lists,
TunerPro Digifant community, structural analysis of a community G60 dump.
Content was rephrased for compliance with licensing restrictions.*

---

## 4. HP evaluation chart (EPROM tuning)

The tool writes calibrations that can be tuned for more power. The chart
below estimates crank output as **boost and fuel pressure are raised** and
the EPROM is re-mapped to suit. Figures are **typical, dyno-dependent
estimates**, consistent with established G60 tuning stages — treat them as
planning targets, not guarantees.

### 4.1 G60 (1.8 supercharged, code PG) — stock 160 PS

| Stage | EPROM change | Boost (approx) | Fuel pressure | Pulley | Supporting hardware | Est. power | Est. torque | Fuel |
|-------|-------------|----------------|---------------|--------|---------------------|-----------|-------------|------|
| **Stock** | factory map | ~0.65 bar / 9.4 psi | ~3.0 bar base | 70 mm | — | **160 PS** | 225 Nm | 95 RON |
| **Stage 1** | chip only | ~0.75 bar / 11 psi | ~3.2 bar | 70 mm | free-flow air + exhaust | **175–180 PS** | ~250 Nm | 98 RON |
| **Stage 2** | chip + boost | ~0.9 bar / 13 psi | ~3.5 bar | **68 mm** | **front-mount intercooler**, bigger injectors | **185–195 PS** | ~270 Nm | 98–102 RON |
| **Stage 3** | chip + high boost | ~1.0–1.1 bar / 15–16 psi | ~4.0 bar | **65 mm** | large intercooler, ported head, rebuilt G-Lader, uprated fuel pump | **~200 PS** | ~290 Nm | **102 RON** |
| **Stage 3+** | aggressive map | >1.1 bar / >16 psi | 4.0 bar+ | 65 mm + ported charger | forged internals, cams, standalone often needed | **210–230 PS** | ~300 Nm+ | race fuel |

**Reading the trend:** roughly every **+0.1 bar (~1.5 psi)** of boost, with
fuelling and timing corrected in the EPROM, is worth **~8–12 PS** on the
G60 up to the point the stock G-Lader and head flow run out of breath
(~200 PS). Beyond that, hardware — not the chip — is the limit.

### 4.2 VR6 (naturally aspirated) — stock 178–190 PS

The VR6 is naturally aspirated, so an EPROM-only tune yields smaller gains
(timing/fuel optimisation, raised limiter); big numbers need forced
induction, which is a hardware project, not just a reflash.

| Stage | EPROM change | Supporting hardware | Est. power | Est. torque |
|-------|-------------|---------------------|-----------|-------------|
| Stock (2.9 ABV) | factory | — | 190 PS | 245 Nm |
| Chip only | optimised timing/fuel, raised limiter | intake + exhaust | +8–12 PS | +10–15 Nm |
| N/A bolt-ons | matched map | headers, cams, head work | +20–30 PS | modest |
| Forced induction | full remap | supercharger/turbo kit, injectors, fuel system | 250–350+ PS | large |

### 4.3 Power/boost curve (G60, visual)

```
Est. crank PS
 230 |                                             ● 3+ (race fuel)
 220 |                                        
 210 |                                   ● (edge of stock charger)
 200 |                              ● Stage 3  (1.0-1.1 bar, 102 RON)
 190 |                        ● Stage 2  (0.9 bar, intercooler)
 180 |                 ● Stage 1  (0.75 bar, chip)
 170 |            
 160 | ● Stock  (0.65 bar)
 150 |____________________________________________________________
       0.6     0.7      0.8      0.9      1.0      1.1     >1.1  bar
                         boost pressure (approx.)
```

### 4.4 Assumptions behind the chart

- **Boost up + fuel pressure up + EPROM re-map together.** Each stage
  assumes the EPROM is re-calibrated to match the new airflow and fuel —
  this is the whole purpose of writing the chip.
- **Fuel pressure:** raising base pressure (adjustable FPR) increases
  injector flow for a given pulse width; the chart pairs modest FPR rises
  with boost so the mixture stays safe under load.
- **Octane:** higher boost needs higher octane to resist knock. 102 RON
  (or race fuel) is assumed at the top stages, mirroring real G60 guidance.
- **G-Lader health:** aggressive stages assume a freshly serviced/rebuilt
  charger and a correctly sized pulley; over-driving a tired charger fails.
- **Diminishing returns:** the stock G-Lader and 8-valve head cap out near
  **~200 PS**; past that you are rebuilding the engine, not just the chip.

*Stage targets are consistent with published G60 tuning programmes (e.g.
bar-tek: chip + 68 mm pulley + large intercooler ≈ up to ~190–200 PS on
high-octane fuel). Content was rephrased for compliance with licensing
restrictions; exact results vary by car and dyno.*

---

## 5. Tuning workflow with this tool

1. **Identify** your car/year: `corrado-eprom info`.
2. **Back up** the stock chip — twice, keep one safe:
   `corrado-eprom backup stock.bin`.
3. **Blank-check** a fresh/erased chip before writing:
   `corrado-eprom blankcheck`.
4. **Edit** the maps in a calibration editor (fuel, timing, boost/limits)
   to suit the planned boost and fuel pressure.
5. **Fix the checksum:** `corrado-eprom checksum tune.bin --fix`.
6. **Write + verify:** `corrado-eprom write tune.bin` (auto-verifies).
7. **Clone** proven chips to spares: `corrado-eprom copy`.
8. **Dyno and data-log.** Re-check knock, AFR, and EGT; iterate.

> For a true UV/OTP 27C256 you must UV-erase before re-writing; the
> `delete` (erase) verb only works on reusable, pin-compatible flash
> replacements. See [`docs/HARDWARE.md`](docs/HARDWARE.md).

---

## 6. Quick facts summary

- **Corrado**: VW's Karmann-built sports coupé, 1988–1995, Golf Mk2 (A2)
  underpinnings, pop-up aero spoiler.
- **Headline engine**: the **G60** — 1.8 8V + G-Lader scroll supercharger,
  160 PS, Bosch Digifant, tuned via a socketed **27C256** EPROM.
- **The chip**: 32 KB UV-erasable EPROM holding fuel/ignition/boost maps
  and a checksum.
- **Tuning**: raise boost (smaller pulley/intercooler) and fuel pressure,
  re-map the EPROM to match → ~175 PS (chip only) up to ~200 PS (full
  Stage 3) on the stock charger; beyond that is a hardware build.

---

## 7. Sources

All figures were cross-referenced and paraphrased from the following
public sources (content rephrased for compliance with licensing
restrictions):

- Volkswagen Newsroom — *Corrado (1988–1995)* and *Engine versions, Corrado*.
- Wikipedia — *Volkswagen Corrado*, *Volkswagen G60 engine*, *G-Lader*,
  *VR6 engine*.
- encycarpedia, ultimatespecs, fastestlaps — Corrado G60 / VR6 specs.
- Hagerty, Hemmings, Jalopnik, conceptcarz — model history and context.
- bar-tek Motorsport — G60 tuning guide and performance kits (stage/boost
  guidance).
- Microchip / alldatasheet — **27C256** EPROM datasheet.
- TunerPro / Digifant tuning community — calibration/EPROM background.

*Specifications varied by market and model year; always verify against your
own vehicle's data plate and ECU label.*
