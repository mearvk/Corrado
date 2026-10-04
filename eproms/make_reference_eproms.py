#!/usr/bin/env python3
"""
make_reference_eproms.py

Generate STRUCTURALLY-FAITHFUL, FREELY-LICENSED reference EPROM images for
the Corrado G60 (Bosch Digifant) and VR6 (Bosch Motronic) ECUs.

These are NOT dumps of a real vehicle calibration. They are synthetic 32 KB
(27C256) images that reproduce the *layout* of a stock image - fill bytes,
the data regions where fuel/ignition/boost tables live, a VW part-number
string, representative monotonic map tables, and a valid trailing checksum
word (16-bit sum of the image == 0, matching corrado_checksum_fix()).

Why synthetic? Freely redistributable real Corrado calibration dumps are
not available under an open licence. These reference images let the
`corrado-eprom` tool, the XDF/hex docs and the test suite work end-to-end
without redistributing anyone's copyrighted calibration. Use
./fetch-eproms.sh to pull real community dumps for actual tuning.

Region layout mirrors an observed stock G60 Digifant image:
  0x0000-0x3FFF : program/fill area (0x41 'A' fill in the reference)
  0x4000-0x55FF : primary map block (fuel + ignition tables)
  0x6000-0x6FFF : secondary map block (enrichment / boost / aux)
  0x7F00-0x7FFF : vectors / identity / checksum tail

SPDX-License-Identifier: MIT
"""

import struct
import sys

SIZE = 0x8000  # 32768, a 27C256
FILL = 0x41    # 'A' - matches the fill seen in the reference G60 dump


def _put_str(buf, off, s):
    b = s.encode("ascii")
    buf[off:off + len(b)] = b


def _ramp_table(buf, off, rows, cols, base, span):
    """Write a rows x cols byte table that ramps with load and rpm, like a
    plausible fuel or timing map (monotonic, bounded 0..255)."""
    for r in range(rows):
        for c in range(cols):
            v = base + (r * span) // max(rows - 1, 1) + (c * span) // (2 * max(cols - 1, 1))
            buf[off + r * cols + c] = max(0, min(255, v))


def _apply_checksum(buf):
    """Store the trailing little-endian 16-bit checksum word, identical to
    corrado_checksum_fix(): the stored word value equals (0 - sum_wo) mod
    65536, where sum_wo is the 16-bit additive sum of every byte EXCEPT the
    final two (the checksum word itself)."""
    sum_wo = sum(buf[:SIZE - 2]) & 0xFFFF
    word = (0 - sum_wo) & 0xFFFF
    buf[SIZE - 2] = word & 0xFF
    buf[SIZE - 1] = (word >> 8) & 0xFF
    return word


def _checksum_ok(buf):
    """Verify like corrado_checksum_fix(verify_only=1): stored word == need."""
    sum_wo = sum(buf[:SIZE - 2]) & 0xFFFF
    need = (0 - sum_wo) & 0xFFFF
    have = buf[SIZE - 2] | (buf[SIZE - 1] << 8)
    return have == need


def build_g60():
    """Reference stock G60 (Digifant) image."""
    buf = bytearray([FILL]) * SIZE

    # --- Identity block (0x7F00): part + engine code, ASCII -------------
    _put_str(buf, 0x7F00, "VW 037906022B DIGIFANT G60 PG REF")
    _put_str(buf, 0x7F40, "CORRADO 1.8 G60 160PS 27C256 REFIMG")

    # --- Primary map block 0x4000.. (fuel then ignition) ----------------
    # 16 x 16 fuel map: injector pulse (richer at high load/rpm)
    _ramp_table(buf, 0x4000, 16, 16, base=0x30, span=0xB0)
    # 16 x 16 ignition map: advance (degrees*scale), pulled back up top
    _ramp_table(buf, 0x4100, 16, 16, base=0x50, span=0x40)
    # boost/load limit curve, 32 points
    for i in range(32):
        buf[0x4200 + i] = min(255, 0x80 + i * 3)   # rising boost target

    # --- Secondary map block 0x6000.. (enrichment / aux) ----------------
    _ramp_table(buf, 0x6000, 16, 16, base=0x20, span=0x60)   # warmup enrich
    for i in range(16):
        buf[0x6100 + i] = 0x40 + i                            # rev-limit ramp

    # Idle / base-pressure reference bytes (documented in the hex map)
    buf[0x7FEE] = 0x1E   # idle rpm scale (reference)
    buf[0x7FEF] = 0x1C   # base fuel-pressure index (reference)

    _apply_checksum(buf)
    return bytes(buf)


def build_vr6():
    """Reference stock VR6 (Motronic) image.

    The VR6 OBD1 Motronic calibration also fits a 27C256-class part; this
    reference uses a different fill and part string, with map blocks placed
    where a Motronic image typically carries its fuel/ignition tables."""
    buf = bytearray([0xFF]) * SIZE   # Motronic reference uses erased fill

    _put_str(buf, 0x0000, "VW 021906259 MOTRONIC VR6 ABV REF")
    _put_str(buf, 0x0040, "CORRADO 2.8/2.9 VR6 178-190PS 27C256")

    # Fuel map 0x2000, ignition 0x2400, lambda/enrich 0x2800
    _ramp_table(buf, 0x2000, 16, 16, base=0x28, span=0xC0)
    _ramp_table(buf, 0x2400, 16, 16, base=0x48, span=0x50)
    _ramp_table(buf, 0x2800, 16, 16, base=0x30, span=0x50)
    for i in range(16):
        buf[0x2C00 + i] = 0x50 + i   # rev-limit / cut ramp (stock ~6500)

    _apply_checksum(buf)
    return bytes(buf)


def main():
    out = {
        "G60_StockEprom_REFERENCE.bin": build_g60(),
        "VR6_StockEprom_REFERENCE.bin": build_vr6(),
    }
    for name, data in out.items():
        assert len(data) == SIZE, name
        assert _checksum_ok(bytearray(data)), f"{name} checksum invalid"
        with open(name, "wb") as f:
            f.write(data)
        print(f"wrote {name}: {len(data)} bytes, checksum OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
