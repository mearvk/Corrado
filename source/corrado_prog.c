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

/*
 * All currently supported units are the MiniPRO/XGecu minipro-family
 * programmers, which share the same bulk framing (8-byte LE command header,
 * 128-byte transfer block, endpoints 0x01/0x81, opcodes 0x05/0x06/0x19).
 * They are therefore described with the same protocol parameters and differ
 * only by USB id and label.
 *
 * VID/PIDs match the upstream open-source `minipro` project's udev rules and
 * the XGecu device enumeration:
 *   TL866A / TL866CS : 0x04D8 / 0xE11C   (Microchip PIC18F VID)
 *   TL866II+         : 0xA466 / 0x0A53
 *   XGecu T48        : 0xA466 / 0x0001   (a.k.a. TL866-3G)
 *   XGecu T56        : 0xA466 / 0x0053
 *   XGecu T76        : 0xA466 / (PID not yet publicly confirmed)
 *
 * The `probe` column says whether a backend should try to open that VID/PID.
 * The T76 is a real, shipping XGecu programmer in the same family, but its USB
 * product id is not publicly documented in a source we can cite; rather than
 * invent one (which could make the tool open the wrong device), it is listed
 * with probe=0 so it is recognised and named but never auto-opened until the
 * PID is confirmed. Fill in its real PID and flip probe to 1 to enable it.
 *
 * Probe order is newest-first so that, on the rare bus with more than one
 * programmer attached, the more capable successor is preferred.
 */
static const corrado_prog_desc_t k_table[] = {
    /* model                 name          vid      pid     epO   epI   blk   rd    wr    er    probe */
    { CORRADO_PROG_T76,     "XGecu T76",   0xA466u, 0x0000u, 0x01, 0x81, 0x80, 0x05, 0x06, 0x19, 0 },
    { CORRADO_PROG_T56,     "XGecu T56",   0xA466u, 0x0053u, 0x01, 0x81, 0x80, 0x05, 0x06, 0x19, 1 },
    { CORRADO_PROG_T48,     "XGecu T48",   0xA466u, 0x0001u, 0x01, 0x81, 0x80, 0x05, 0x06, 0x19, 1 },
    { CORRADO_PROG_TL866II, "TL866II+",    0xA466u, 0x0A53u, 0x01, 0x81, 0x80, 0x05, 0x06, 0x19, 1 },
    { CORRADO_PROG_TL866A,  "TL866A/CS",   0x04D8u, 0xE11Cu, 0x01, 0x81, 0x80, 0x05, 0x06, 0x19, 1 },

    /* sentinel */
    { CORRADO_PROG_UNKNOWN, NULL,          0,       0,       0,    0,    0,    0,    0,    0,    0 }
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
