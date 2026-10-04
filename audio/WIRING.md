# Corrado Audio Wiring (1990–1996)

Radio power, speaker and antenna wiring for the factory head unit, plus
notes for the amplified premium system.

> ⚠️ **Wire colours vary by market and build date.** The colours below are
> the commonly reported Corrado/VW values for this era; **always meter your
> own wires** (constant 12 V, switched 12 V, ground) before connecting a
> head unit. Reverse-polarity or a mis-identified constant/switched feed can
> damage a radio or drain the battery.

## 1. Connector family

The Corrado uses the **old-style VW/ISO radio connectors** of the late
1980s / early 1990s — chamfered multi-pin **power** and **speaker** blocks,
commonly adapted to the standard **ISO** radio connector with a plug-and-play
harness (the same adapters cover VW Golf/Jetta/Passat/Corrado ~1987–2002).

It is **NOT** the later 40-pin **Quadlock** connector (that arrived on VW
later in the 1990s). Aftermarket adapters for the Corrado are the VW/ISO
type (e.g. Metra 70-1787 for amp/Bose integration 1993-on, Absolute
H1004/9002, RED WOLF VW 1990–1994).

## 2. Power / ignition wiring (head unit)

| Function | Typical colour | Notes |
|----------|----------------|-------|
| Constant **+12 V** (battery / memory) | **Red/White** | Always-on; keeps clock/presets & radio code |
| Switched **+12 V** (accessory/ignition) | (varies) | Run from fuse box / ignition if not present at plug |
| **Ground** | **Brown** | VW uses brown for ground throughout the car |
| **Illumination** (dash dimmer feed) | **Gray/Blue** | Dims display with headlights |
| **Power-antenna / amp trigger** | **White/Blue** | Switched output to raise antenna / wake amp |

> On many VW cars of this era the plug supplies **constant 12 V** but **not**
> a separate **switched/accessory** 12 V — installers often jumper constant
> to switched, or tap the fuse box for a true ignition-switched feed. Check
> yours; getting this wrong leaves the radio either always-on or dead.

## 3. Speaker wiring (standard 4-speaker)

Four channels, each a **+ / −** pair. VW typically uses a solid colour for
**+** and the same colour **with a black stripe** for **−** (verify).

| Channel | Location | Typical +/− colours |
|---------|----------|---------------------|
| Front Left  | LH dash | solid / striped pair |
| Front Right | RH dash | solid / striped pair |
| Rear Left   | LH rear panel | solid / striped pair |
| Rear Right  | RH rear panel | solid / striped pair |

- Keep **polarity consistent** across all four speakers or you lose bass to
  phase cancellation.
- **Do not ground** either speaker lead. These are **floating bridged (BTL)**
  outputs from the head unit; grounding a − lead can blow the output stage.

## 4. Premium / amplified "Sound System"

Cars with the factory amp differ from the simple 4-speaker setup:

- The head unit feeds the **external amplifier** (line-level pre-outs on
  premium units, or high-level speaker signal on others).
- The amp has its **own switched power + constant power + ground**, and a
  **remote turn-on** (often the head unit's power-antenna/amp-trigger lead,
  i.e. the **White/Blue** wire).
- The amp then drives the speakers; the speaker wires at the radio may be
  **signal to the amp**, not direct to the speakers.
- When replacing a head unit on an amplified car, use an **amp-integration
  harness** so the factory amp still gets signal and a remote-on.

```
 Head unit ──(pre-out / hi-level)──▶ Factory AMP ──▶ Front L/R + tweeters
     │                                   ▲
     └──(White/Blue remote turn-on)──────┘
   +12V const (Red/White), +12V sw, GND (Brown) to head unit
   AMP has its own +12V const, +12V sw, GND, and remote-in
```

## 5. Antenna

- Standard: a roof/fender mast antenna with a **DIN antenna plug** at the
  radio; power antennas use the **White/Blue** trigger to raise/lower.
- Some cars use an **in-glass** antenna with an **antenna amplifier/booster**
  that needs a **12 V phantom feed** on the antenna lead (the radio's antenna
  power output). If reception is weak after a head-unit swap, confirm the new
  unit supplies **antenna phantom power**.

## 6. ASCII pinout — generic VW/ISO radio block

Standard ISO radio connector (what adapters present), two 8-pin blocks:

```
  ISO "A" — Power block                 ISO "B" — Speaker block
  ┌───────────────────────────┐         ┌───────────────────────────┐
  │ A1  (spare)               │         │ B1  Rear Right  +          │
  │ A2  (spare)               │         │ B2  Rear Right  −          │
  │ A3  (spare)               │         │ B3  Front Right +          │
  │ A4  +12V constant (Red/Wh)│         │ B4  Front Right −          │
  │ A5  Power-ant / amp (Wh/Bl)│         │ B5  Front Left  +          │
  │ A6  Illumination (Gy/Bl)  │         │ B6  Front Left  −          │
  │ A7  +12V switched         │         │ B7  Rear Left   +          │
  │ A8  Ground (Brown)        │         │ B8  Rear Left   −          │
  └───────────────────────────┘         └───────────────────────────┘
```

> This is the **generic VW/ISO** mapping that Corrado adapter harnesses
> conform to. The car-side factory block is the chamfered VW variant of the
> same signals; an ISO adapter remaps it to the above. Confirm against your
> own loom — spare pins and switched-12 V presence vary.

*Sources: Modified Life 1992 Corrado radio wiring guide (power/illumination/
antenna colours), VW/ISO radio connector standard, aftermarket harness
fitment data (Metra/Absolute/RED WOLF), Crutchfield. Content was rephrased
for compliance with licensing restrictions. Verify every wire on your own
vehicle before connecting.*
