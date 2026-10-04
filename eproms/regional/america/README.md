# EPROM / ECU — North American (US / Canada) Corrado

Region-specific notes for **North American** Corrado engine calibrations.
For the region-neutral image layout, device details and reference binaries,
see the parent [`eproms/EPROM_MAPS.md`](../../EPROM_MAPS.md).

US sales ran **1990–1994** (Canada from 1989), so the American ECU story is
G60 early on, then the 2.8 VR6.

## Engines & ECUs (North America)

| Engine | Code | Displacement | Power | ECU |
|--------|------|--------------|-------|-----|
| G60 1.8 supercharged | **PG** | 1,781 cc | ~158 hp | Bosch **Digifant** (US emissions trim) |
| VR6 (SLC) | **AAA** | **2,792 cc (2.8 L)** | **178 hp / 177 lb-ft** | Bosch **Motronic** |

Key North American points:

- The US/Canada VR6 is the **2.8-litre AAA** making **178 hp** — smaller and
  less powerful than Europe's 2.9 ABV. US VR6 cars were marketed as the
  **Corrado SLC**.
- **California vs. 49-state:** California-emissions cars ran a **stricter
  emissions calibration** (and sometimes different emissions hardware) than
  federal 49-state cars. Expect a **different EPROM calibration** — and
  sometimes a different ECU part — between California and non-California
  cars of the same year.
- Calibrations are tuned for **US pump octane (AKI/(R+M)/2)**, which is
  numerically lower than European RON for the same fuel.

## Calibration implications

- A US **2.8 AAA** EPROM is **not** interchangeable with a Euro **2.9 ABV**
  calibration — different displacement, power and emissions targets.
- Within the US, confirm whether your car is **California** or **federal**
  before swapping an ECU/EPROM; mixing them can set emissions faults and
  run incorrectly.
- The G60's US calibration assumes US-market emissions plumbing.

> No openly-licensed stock US VR6 (AAA) Motronic dump is publicly available;
> the repo's VR6 reference image is synthetic. Community **G60** dumps pulled
> via `../../fetch-eproms.sh` are mostly European-spec — treat them as a
> structural reference, not a US-emissions-correct calibration.

*Sources: Hagerty / autoevolution (US VR6 2.8 AAA, 178 hp), VW market data,
Bosch Digifant/Motronic references, US EPA/CARB emissions-tier context.
Content was rephrased for compliance with licensing restrictions.*
