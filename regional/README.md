# Regional Differences — European vs. American Corrado

The Volkswagen Corrado was sold in both **Europe** and **North America**,
and several systems differ between the two markets. This directory is the
**index** for region-specific documentation; the actual region-sorted
documents live next to the subsystem they describe, under a consistent
`regional/europe/` and `regional/america/` split.

## Convention

Each subsystem area keeps its **region-neutral** docs at its top level and
sorts **region-specific** material into:

```
<area>/regional/europe/    # European-market specifics
<area>/regional/america/   # North American (US/Canada) specifics
```

So far the areas with real EU/US differences are:

| Area | Region-specific docs |
|------|----------------------|
| **EPROM / ECU** | [`eproms/regional/europe/`](../eproms/regional/europe/) · [`eproms/regional/america/`](../eproms/regional/america/) |
| **Audio / radio** | [`audio/regional/europe/`](../audio/regional/europe/) · [`audio/regional/america/`](../audio/regional/america/) |

> **"The next thing":** when we add the next region-sensitive subsystem
> (e.g. **lighting & bumpers** — US DOT sealed-beam/side-marker and
> impact-bumper rules vs. European headlights and trim), create
> `lighting/regional/europe/` and `lighting/regional/america/` the same way
> and add a row to the table above.

## Why these differ (summary)

- **EPROM / ECU** — the engines and their emissions calibrations differed by
  market. The clearest case: the **VR6** was **2.9 L (ABV, 190 PS)** in
  Europe but **2.8 L (AAA, 178 hp)** in North America, and **California**
  cars ran stricter emissions calibrations than 49-state US cars. The G60
  also carried market-specific emissions trim.
- **Audio / radio** — Europe shipped VW's **Alpha/Beta/Gamma** (coded)
  radios; North America commonly shipped **Blaupunkt** units. Tuner band
  plans/step and antenna arrangements differed between regions.

See each area's `regional/<region>/` folder for the detail.

*Content was rephrased for compliance with licensing restrictions; verify
specifics against your own vehicle's label and official VW documentation.*
