/*
 * corrado_prog.c
 *
 * The supported-programmer registry for the Corrado EPROM suite. See
 * corrado_prog.h for the design. This translation unit is OS-independent and
 * is compiled into every per-year binary on all three platforms alongside
 * corrado_eprom.c / corrado_ops.c / main.c.
 *
 * SPDX-License-Identifier: MIT
 */

#include "corrado_prog.h"

#include <stddef.h>

/* ---- Descriptive metadata (version 1.11 nested-friendly accessors) ----
 * Worker-class artifact (single-purpose registry); baseline identity only.
 *   metaVersion = "1.0.0"         metaId = 2
 *   metaAuthor  = "mearvk"        metaDate = "2026-11-02"
 *   metaOrigin  = "International"  metaBirthDate = "2026-11-02"
 *   metaScienceRating = 6         metaNationalSchedule = "none"
 * Full record for the suite: corrado_suite_metadata() in corrado_metadata.c. */

/*
 * Two protocol families are supported (see corrado_prog_family_t):
 *
 *  - MINIPRO: the MiniPRO/XGecu programmers, which share one bulk framing
 *    (8-byte LE command header, 128-byte transfer block, endpoints
 *    0x01/0x81, opcodes 0x05/0x06/0x19) and differ only by USB id and label.
 *    VID/PIDs match the upstream open-source `minipro` project and the XGecu
 *    device enumeration:
 *      TL866A / TL866CS : 0x04D8 / 0xE11C   (Microchip PIC18F VID)
 *      TL866II+         : 0xA466 / 0x0A53
 *      XGecu T48        : 0xA466 / 0x0001   (a.k.a. TL866-3G)
 *      XGecu T56        : 0xA466 / 0x0053
 *      XGecu T76        : 0xA466 / (PID not yet publicly confirmed)
 *
 *  - CH341A: the WinChipHead CH341A USB-to-SPI dongle (0x1A86/0x5512), bulk
 *    endpoints 0x02/0x82, driving an SPI NOR flash with JEDEC opcodes
 *    (0x03 read / 0x02 page-program / 0xC7 chip-erase) over the CH341 0xA8
 *    SPI-stream command. This is a genuinely different wire protocol; the
 *    backend dispatches on the `family` field.
 *
 *  - CH347: the WinChipHead CH347 (0x1A86/0x55DB), a USB 2.0 high-speed
 *    USB-to-SPI bridge -- the bench-grade / "strong worker" programmer for
 *    flashing many chips quickly. It drives the same SPI NOR JEDEC opcodes
 *    but over the CH347 command protocol (0xC0 init / 0xC1 CS / 0xC2 rd-wr)
 *    on interface 2, bulk endpoints 0x06/0x86. The `block` field is 0 (the
 *    CH347 module chooses its own transaction size); the `iface` field is 2.
 *
 * The `probe` column says whether a backend should auto-open that VID/PID.
 * The T76 is a real, shipping XGecu programmer, but its USB product id is not
 * publicly documented in a source we can cite; rather than invent one (which
 * could make the tool open the wrong device), it is listed with probe=0 so it
 * is recognised and named but never auto-opened until the PID is confirmed.
 * Fill in its real PID and flip probe to 1 to enable it.
 *
 * Probe order is newest-first within the minipro family; the SPI CH341A is
 * last so a minipro programmer is preferred when both happen to be attached.
 */
static const corrado_prog_desc_t k_table[] = {
    /* model                 name            family                   vid      pid     epO   epI   if   blk   rd    wr    er    probe */
    { CORRADO_PROG_T76,     "XGecu T76",     CORRADO_FAMILY_MINIPRO,  0xA466u, 0x0000u, 0x01, 0x81, 0,  0x80, 0x05, 0x06, 0x19, 0 },
    { CORRADO_PROG_T56,     "XGecu T56",     CORRADO_FAMILY_MINIPRO,  0xA466u, 0x0053u, 0x01, 0x81, 0,  0x80, 0x05, 0x06, 0x19, 1 },
    { CORRADO_PROG_T48,     "XGecu T48",     CORRADO_FAMILY_MINIPRO,  0xA466u, 0x0001u, 0x01, 0x81, 0,  0x80, 0x05, 0x06, 0x19, 1 },
    { CORRADO_PROG_TL866II, "TL866II+",      CORRADO_FAMILY_MINIPRO,  0xA466u, 0x0A53u, 0x01, 0x81, 0,  0x80, 0x05, 0x06, 0x19, 1 },
    { CORRADO_PROG_TL866A,  "TL866A/CS",     CORRADO_FAMILY_MINIPRO,  0x04D8u, 0xE11Cu, 0x01, 0x81, 0,  0x80, 0x05, 0x06, 0x19, 1 },
    { CORRADO_PROG_CH341A,  "CH341A (SPI)",  CORRADO_FAMILY_CH341A,   0x1A86u, 0x5512u, 0x02, 0x82, 0,  0x20, 0x03, 0x02, 0xC7, 1 },
    { CORRADO_PROG_CH347,   "CH347 (HS-SPI)",CORRADO_FAMILY_CH347,    0x1A86u, 0x55DBu, 0x06, 0x86, 2,  0x00, 0x03, 0x02, 0xC7, 1 },

    /* sentinel */
    { CORRADO_PROG_UNKNOWN, NULL,            CORRADO_FAMILY_MINIPRO,  0,       0,       0,    0,    0,  0,    0,    0,    0,    0 }
};

const corrado_prog_desc_t *corrado_prog_table(void)
{
    return k_table;
}

size_t corrado_prog_count(void)
{
    size_t n = 0;
    const corrado_prog_desc_t *d = k_table;
    while (d->name) { ++n; ++d; }
    return n;
}

const corrado_prog_desc_t *corrado_prog_find(corrado_prog_model_t model)
{
    const corrado_prog_desc_t *d;
    for (d = k_table; d->name; ++d) {
        if (d->model == model) return d;
    }
    return NULL;
}

const char *corrado_prog_name(corrado_prog_model_t model)
{
    const corrado_prog_desc_t *d = corrado_prog_find(model);
    return d ? d->name : "unknown";
}

const char *corrado_prog_family_name(corrado_prog_family_t family)
{
    switch (family) {
        case CORRADO_FAMILY_MINIPRO: return "minipro";
        case CORRADO_FAMILY_CH341A:  return "ch341a-spi";
        case CORRADO_FAMILY_CH347:   return "ch347-hs-spi";
        default:                     return "unknown";
    }
}
