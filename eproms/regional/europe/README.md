# EPROM / ECU — European-market Corrado

Region-specific notes for **European** Corrado engine calibrations. For the
region-neutral image layout, device details and reference binaries, see the
parent [`eproms/EPROM_MAPS.md`](../../EPROM_MAPS.md).

## Engines & ECUs (Europe)

| Engine | Code | Displacement | Power | ECU |
|--------|------|--------------|-------|-----|
| G60 1.8 supercharged | **PG** | 1,781 cc | **118 kW / 160 PS** | Bosch **Digifant** |
| VR6 | **ABV** | **2,861 cc (2.9 L)** | **140 kW / 190 PS** | Bosch **Motronic** |

Key European points:

- The European VR6 is the **2.9-litre ABV** making **190 PS** — larger and
  more powerful than the North American 2.8.
- Emissions calibration follows **European** standards of the period (and
  evolved with EU emissions rules through the car's life); fuel was tuned
  for European pump octane (RON).
- Radios/ECUs are generally **coded** (anti-theft) in Europe — unrelated to
  the ECU, but typical of the market.

## Calibration implications

- An ECU/EPROM pulled from a **Euro VR6** is calibrated for the **2.9 ABV**
  and its emissions equipment — it is **not** a drop-in match for a US 2.8
  AAA car, and vice versa.
- Tuning headroom and stock boost/fuel maps for the **Euro G60** assume
  European-spec exhaust/emissions hardware.

> Use `../../fetch-eproms.sh` for community G60 (Digifant) dumps — those are
> predominantly European-market images. No openly-licensed Euro VR6 Motronic
> dump is publicly available; the repo's VR6 reference image is synthetic.

*Sources: VW engine-version data, encycarpedia/ultimatespecs (Euro VR6
2.9/190 PS), Bosch Digifant/Motronic references. Content was rephrased for
compliance with licensing restrictions.*
