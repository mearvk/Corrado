# TUNERZ.md — Corrado Tuning to 260+ WHP, Costs, MPG & Fuel Prices

A build reference for taking a Volkswagen Corrado from stock up past
**260 wheel horsepower (WHP)**: the upgrades, who makes them, roughly when
they appeared, what they cost, and the power/torque each adds. Plus the
factory **MPG** for each model year and the **average US pump price** of
that year.

> ⚠️ **Reality check.** "260+ WHP" in a Corrado almost always means
> **forced induction** — a VR6 turbo/supercharger kit, or a heavily built
> G60. Bolt-ons alone will not get a stock N/A VR6 (≈150–160 WHP) to 260.
> Every figure below is a **typical, dyno-dependent estimate** and assumes
> matching fuel, cooling and tuning. Prices are **approximate street prices
> in current USD** unless noted; brands/dates mark when a part/approach
> became common, not an exact release.
>
> **WHP vs crank:** WHP is measured at the wheels and is ~15% lower than the
> crank (flywheel) figure VW quotes. A 190-PS crank VR6 is ≈150–160 WHP stock.

---

## 1. The path to 260+ WHP (two routes)

- **VR6 route (recommended):** stock 2.8/2.9 VR6 ≈ 150–160 WHP. Bolt-ons add
  a little; a **turbo or supercharger kit** is what crosses 260 WHP. A
  common target is **250–300 WHP** on a stock-internals VR6 at modest boost.
- **G60 route:** stock ≈ 135–140 WHP. The stock G-Lader caps around
  ~200 crank PS (~170 WHP). Reaching 260 WHP needs a big charger rebuild or
  a **turbo conversion / VR6 swap** — i.e. you end up on the VR6 route.

---

## 2. Upgrade & tuner-parts table

Sorted by **increasing HP/torque gain**. Gains are **crank** unless the row
says WHP; "cumulative target" shows where a sensible build sits after adding
that item on a VR6.

| # | Part / Upgrade | Typical Mfr(s) | Appeared (approx) | Cost (USD) | HP gain | Torque gain | Notes / cumulative target |
|---|----------------|----------------|-------------------|-----------:|--------:|------------:|---------------------------|
| 1 | ECU/EPROM chip tune (N/A) | TunerPro/community, GIAC, UM | 1990s–now | $100–400 | +8–12 | +10–15 Nm | Timing/fuel/limiter; smallest N/A gain |
| 2 | Cold-air / open intake | K&N, aFe, Forge | 1990s | $60–250 | +3–7 | +5–10 Nm | Pairs with chip; marginal alone |
| 3 | Cat-back exhaust | Magnaflow, Supersprint, Techtonics | 1990s | $400–1,000 | +5–10 | +8–15 Nm | Sound + small top-end gain |
| 4 | Pulley + belt (G60) | BAR-TEK, Vom Werk | 1990s | $150–400 | +8–15 | +15–25 Nm | **G60 only** — 68→65 mm raises boost |
| 5 | Header / tubular manifold | Techtonics, Supersprint | 1990s | $500–1,200 | +8–15 | +10–20 Nm | Best with cams + tune |
| 6 | Ported head + cams (N/A) | Autotech, Schrick, TT | 1990s | $1,500–3,500 | +20–35 | +20–35 Nm | Diminishing returns N/A; ~190–200 crank max |
| 7 | G60 Stage "big-charger" kit | BAR-TEK, G-Werks | 1990s–now | $1,500–3,000 | +30–50 | +40–70 Nm | Rebuilt/ported G-Lader + chip + intercooler → ~200 crank |
| 8 | VR6 **supercharger** kit (Stage 1) | VF-Engineering (Vortech V3) | ~2000s | ~$4,000–5,000 | **+80–90** | +60–80 Nm | 170→**~250 hp** crank; biggest reliable bolt-on |
| 9 | VR6 **turbo** kit (budget) | Nashville Perf. / generic GTX30xx | 2010s–now | ~$2,500–3,500 | **+100–150** | +120–180 Nm | 2.8/2.9 to **~250–300 WHP** on low boost |
| 10 | VR6 supercharger Stage 2 | VF-Engineering | ~2000s | +$500–1,500 (over St.1) | **+110–120** | +100–110 Nm | ~**280 hp** crank / ~250 WHP |
| 11 | VR6 **turbo** kit (full, boost-up) | BAR-TEK Turbo-Total, HPA | 2000s–now | $4,000–10,000 | **+150–350+** | +200–400 Nm | 300–500+ hp; needs fuel + clutch + cooling |

### Supporting hardware required at 260+ WHP

These don't make big power alone but are **mandatory** to run it safely:

| Part | Typical Mfr(s) | Cost (USD) | Why |
|------|----------------|-----------:|-----|
| Front-mount intercooler (FI) | Forge, BAR-TEK, generic | $300–800 | Cools charge; prevents knock |
| Bigger injectors + fuel pump | Bosch, Walbro, DeatschWerks | $300–700 | Feeds the extra air |
| Standalone / full ECU tune | United Motorsport, Lugtronic, MS | $500–2,000 | Maps boost/fuel/ignition safely |
| Reinforced clutch | Sachs, Clutchmasters, Spec | $400–1,000 | Stock clutch slips past ~250 lb-ft |
| Oil cooler + gauges | Mocal, BAR-TEK | $200–500 | Protects engine under boost |

**Representative 260+ WHP VR6 build cost:** a budget turbo kit (~$3,000) +
intercooler/injectors/pump (~$1,000) + tune (~$1,000) + clutch (~$700)
≈ **$5,500–7,000** in parts (DIY labor), landing around **~280 WHP**. A
VF-Engineering supercharger Stage 2 route is a cleaner but pricier
**~$6,000–8,000** for a similar ~250–280 WHP.

*Sources: VF-Engineering / Vortech, BAR-TEK Motorsport, Nashville
Performance, HPA Motorsports, Studio RSR, Techtonics/Autotech/Schrick,
TunerPro/community. Content was rephrased for compliance with licensing
restrictions; verify current pricing with the vendor.*

---

## 3. Factory MPG by model year (US EPA)

The Corrado was sold in the US 1990–1994 (G60 then VR6). Figures are the
**EPA ratings of the period** (original window-sticker method; modern
"adjusted" numbers are lower). Non-US years shown for completeness.

| Model year | Engine (US) | EPA city/hwy | EPA combined |
|------------|-------------|-------------:|-------------:|
| 1990 | G60 1.8 s/c | 18 / 26 | ~21 |
| 1991 | G60 1.8 s/c | 18 / 26 | ~21 |
| 1992 | G60 / VR6 2.8 | 16 / 23 (VR6) | ~19 |
| 1993 | VR6 2.8 | 18 / 24 | ~20 |
| 1994 | VR6 2.8 | 18 / 24 | ~20 |
| 1995 | (EU final year) | — | — |
| 1996 | (run-out/other mkts) | — | — |

> Tuning for 260+ WHP **lowers** real-world MPG: added boost and enrichment
> plus a heavier right foot typically drop combined economy well below the
> stock figures above.

*Sources: EPA / fueleconomy.gov via Edmunds Corrado specs (1990 18/26,
1992 VR6 16/23). Content was rephrased for compliance with licensing
restrictions.*

---

## 4. Average US fuel price per gallon, by year

National **average retail price of regular gasoline (all formulations),
US, $/gal**, from the U.S. Energy Information Administration (EIA). These
are **annual averages** computed from EIA monthly data.

| Year | Avg US regular ($/gal) | Period context (US national) |
|------|-----------------------:|------------------------------|
| 1990 | ~**$1.16** | Gulf War spike Aug–Oct 1990 pushed pump prices toward ~$1.35 |
| 1991 | ~**$1.10** | Prices eased after the war |
| 1992 | ~**$1.09** | Stable, low-price era |
| 1993 | ~**$1.07** | Lowest of the span (nominal) |
| 1994 | ~**$1.07** | Flat; late-year uptick |
| 1995 | ~**$1.11** | Mild spring rise |
| 1996 | ~**$1.21** | Spring price run-up |

> **On the "national advertisement / party pledge toward unity" item:** I
> could not find a verifiable dataset tying a specific printed national ad
> or a "party pledge toward unity" to these Corrado years, so I have **not**
> invented one. The sourceable, factual substitute is the **EIA national
> fuel price** above plus the brief period context. If you meant a specific
> real ad campaign or program, name it and it can be researched and added.

*Sources: U.S. Energy Information Administration (EIA), "U.S. Regular All
Formulations Retail Gasoline Prices," monthly series (public domain, US
Government). Annual averages computed from the monthly values.*

---

## 5. Build summary — getting to 260+ WHP sensibly

1. **Start with the engine choice:** a **VR6** is the realistic base for
   260+ WHP; a G60 gets you to ~170 WHP before you're rebuilding the charger
   or swapping engines.
2. **Add forced induction:** a **VR6 turbo kit** (~$3k budget, 250–300 WHP)
   or **VF-Engineering supercharger** (Stage 2, ~280 hp crank / ~250 WHP).
3. **Support it:** intercooler, injectors + fuel pump, standalone/flashed
   tune, reinforced clutch, oil cooler.
4. **Fuel it:** run **high-octane** (91–93 AKI US, 98–102 RON) at these
   power levels to control knock.
5. **Tune and data-log** on a dyno; watch AFR, knock and EGT. Expect **~$5.5k–8k**
   in parts for a reliable ~260–280 WHP street car (DIY labor).

*All performance and price figures are estimates and vary by car, boost,
fuel and dyno. Verify specifics with the vendor and a reputable tuner.*
