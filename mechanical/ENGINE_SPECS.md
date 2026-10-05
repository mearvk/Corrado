# Corrado Engine Mechanical Specifications

Engineering reference for the two primary Volkswagen Corrado (Typ 53I) engines:
the supercharged **G60** (1.8 8-valve, engine code **PG**, Bosch Digifant) and
the **VR6** (narrow-angle V6, Bosch Motronic). The tables below collect the
mechanical specs most often needed for service and tuning — timing, valve
clearances, timing-drive type, firing order, compression ratio, and related
dimensions.

> **Scope & accuracy.** These are factual engineering specifications compiled
> for reference. Values marked **~** are approximate or market-dependent; the
> VR6 shipped as the **2.8 (AAA, North America)** and the **2.9 (ABV, Europe)**,
> which differ, so VR6 rows note the variant where it matters. **Always confirm
> against the official factory manual for your exact engine code, model year,
> and market before performing engine work** — torque, clearance, and timing
> figures must come from the authoritative service data for the specific unit.
> This file is a convenience reference, not a substitute for that manual.

---

## G60 — 1.8 L supercharged 8-valve (engine code PG)

| Specification | Value |
|---|---|
| Engine code | **PG** |
| Layout | Inline-4, SOHC, 8 valves (2 per cylinder) |
| Displacement | 1,781 cc |
| Bore × stroke | 81.0 mm × 86.4 mm |
| Compression ratio | **8.0 : 1** (low, for forced induction) |
| Induction | **G-Lader** scroll-type supercharger |
| Stock boost | ~0.6–0.7 bar (≈9–10 psi) |
| Engine management | Bosch **Digifant** (socketed tuning EPROM) |
| Power (DIN) | ~118 kW / 160 PS / 158 hp |
| Torque | ~225 N·m @ ~3,600 rpm |
| Firing order | **1–3–4–2** (cylinder 1 at the timing-belt end) |
| Timing drive | **Toothed rubber timing belt** (camshaft); interference engine |
| Timing belt replacement | ~ every 60,000 mi / 90,000 km (confirm per manual) |
| Supercharger drive | Separate **poly-V (serpentine) belt** to the G-Lader |
| Valve clearances | **Hydraulic lifters — no manual adjustment** |
| Spark plug gap | ~0.7–0.8 mm (confirm per manual) |
| Spark plug tightening | ~25 N·m |
| Idle speed | ~800–900 rpm (ECU-controlled) |
| Coolant | ~5–6 L (system), ethylene-glycol (G11/G12 per year) |
| Engine oil | ~4.0 L with filter; grade per manual (commonly 5W-40 / 10W-40) |
| Ignition | Distributor-based, Digifant-controlled |

---

## VR6 — 2.8 / 2.9 L narrow-angle V6 (engine codes AAA / ABV)

| Specification | Value |
|---|---|
| Engine code | **AAA** (2.8, North America) / **ABV** (2.9, Europe) |
| Layout | **15° narrow-angle "VR" V6**, SOHC, 12 valves, **single cylinder head** |
| Displacement | 2,792 cc (AAA) / 2,861 cc (ABV) |
| Bore × stroke | 81.0 mm × 90.3 mm (2.8) / 82.0 mm × 90.3 mm (2.9) |
| Compression ratio | **10.0 : 1** (2.8) / ~**10.0–10.1 : 1** (2.9) |
| Induction | Naturally aspirated |
| Engine management | Bosch **Motronic** |
| Power (DIN) | ~174 hp (2.8, AAA) ; ~140 kW / 190 PS / 187 hp (2.9, ABV) |
| Torque | ~235 N·m (2.8, AAA) ; ~245 N·m @ ~4,200 rpm (2.9, ABV) |
| Firing order | **1–5–3–6–2–4** |
| Cylinder numbering | 1–3–5 on one bank, 2–4–6 on the other (per the single VR head) |
| Timing drive | **Timing chain(s)** (camshaft), at the transmission/flywheel end |
| Timing drive service | Chain — not a scheduled belt interval; inspect guides/tensioner |
| Accessory drive | **Poly-V (serpentine) belt** |
| Valve clearances | **Hydraulic lifters — no manual adjustment** |
| Spark plug gap | ~0.7–0.9 mm (confirm per manual) |
| Spark plug tightening | ~25–30 N·m |
| Idle speed | ~700–800 rpm (ECU-controlled) |
| Coolant | ~7 L (system) |
| Engine oil | ~5.0–5.5 L with filter; grade per manual |
| Ignition | Motronic-managed; distributor on early VR6 |

---

## Notes on the two designs

- **G60 timing vs. VR6 timing.** The G60 uses a conventional **rubber timing
  belt** that must be replaced on an interval (it is an interference engine, so
  a failed belt can bend valves). The VR6 uses **timing chains**, which are not
  on a fixed replacement interval but whose guides and tensioners wear and
  should be inspected — a well-known VR6 maintenance point.
- **Valve clearances.** Both engines use **hydraulic lifters (hydraulic lash
  adjusters)**, so there is no manual valve-clearance (shim/feeler-gauge)
  adjustment on either — clearance is maintained automatically. A "valve
  clearance" figure in the classic sense does not apply.
- **Firing order.** G60 (inline-4): **1–3–4–2**. VR6: **1–5–3–6–2–4**.
- **Compression ratio.** The G60's **8.0:1** is deliberately low to tolerate
  supercharger boost; the naturally-aspirated VR6 runs ~**10:1**.
- **The two induction belts on a G60.** Don't confuse them: the **timing belt**
  drives the camshaft (interval item), while a separate **poly-V belt** drives
  the **G-Lader supercharger**. Both matter, for different reasons.

## Relationship to the rest of this repo

The engine-management calibration lives in the ECU EPROM this project reads and
writes (`eproms/`, `source/`, `README.md`). Raising boost/fuel on a G60 (see
`CORRADO.md` and `TUNERZ.md`) interacts directly with the compression ratio and
timing figures above — the mechanical limits here are the backdrop against which
any EPROM tune must stay safe.

> **Safety.** Changing calibration, boost, or ignition timing can exceed these
> mechanical limits and damage the engine or make the car unsafe/non-compliant.
> Back up the stock EPROM first, verify every write, and keep within the
> manufacturer's limits for your exact engine.

---

### Sources & verification

Specifications compiled from general factory/enthusiast reference data for the
VW Corrado G60 (PG) and VR6 (AAA/ABV). Figures that vary by market or model year
are marked approximate (**~**). Content was rephrased and tabulated for this
reference; verify every value against the official VW service manual for your
specific engine code and year before use.
