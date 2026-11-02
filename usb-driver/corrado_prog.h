/*
 * corrado_prog.h
 *
 * OS-independent registry of supported chip-programmer hardware for the
 * Corrado EPROM suite.
 *
 * Historically the USB backends hard-coded a single programmer (the MiniPRO
 * TL866). This header turns "which programmers do we support" into DATA: a
 * table of corrado_prog_desc_t entries, each pairing a USB VID/PID with the
 * model identity and the bulk-protocol parameters that model uses
 * (endpoints, transfer block size, command opcodes). Each OS backend simply
 * walks the table to open the first attached known device and then drives it
 * with the parameters from its descriptor, instead of baking TL866 constants
 * into the transfer code.
 *
 * Adding a new programmer that speaks the same minipro-style bulk framing is
 * therefore a one-line table entry in corrado_prog.c -- no backend edits.
 * A programmer that needs genuinely different framing adds its own opcodes to
 * the descriptor (and, if its transfer shape differs, a branch keyed on
 * corrado_prog_model_t in the backend transfer helpers).
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef CORRADO_PROG_H
#define CORRADO_PROG_H

#include "corrado_usb.h"   /* corrado_prog_model_t */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Protocol parameters + identity for one supported programmer model.
 *
 * The TL866 family (TL866A/CS, TL866II+) and the XGecu successors (T48, T56)
 * all share the same minipro-style bulk framing: an 8-byte little-endian
 * command header [cmd, len, le32 addr, pad, pad] followed, for writes, by the
 * data block; reads return one data block per command. The fields below
 * capture everything the backend needs so the transfer code carries no
 * model-specific constants of its own.
 */
typedef struct {
    corrado_prog_model_t model;      /* enum identity                        */
    const char          *name;       /* human-readable label, e.g. "TL866II+" */
    uint16_t             vid;         /* USB vendor id                        */
    uint16_t             pid;         /* USB product id                       */
    uint8_t              ep_out;      /* bulk OUT endpoint                    */
    uint8_t              ep_in;       /* bulk IN endpoint                     */
    uint16_t             block;       /* transfer granularity, bytes          */
    uint8_t              cmd_read;    /* READ_CODE opcode                     */
    uint8_t              cmd_write;   /* WRITE_CODE opcode                    */
    uint8_t              cmd_erase;   /* chip ERASE opcode                    */
    uint8_t              probe;       /* 1 = auto-probe this VID/PID; 0 = skip
                                       *     (model recognised/named but its
                                       *      USB id is not yet confirmed)    */
} corrado_prog_desc_t;

/*
 * The registry of supported programmers, in the order the backends should
 * probe them (newest/most-specific first). Terminated by a sentinel whose
 * .name is NULL. Walk it like:
 *
 *   for (const corrado_prog_desc_t *d = corrado_prog_table(); d->name; ++d)
 *       ... try_open(d->vid, d->pid) ...
 */
const corrado_prog_desc_t *corrado_prog_table(void);

/* Number of real entries in the table (excluding the sentinel). */
size_t corrado_prog_count(void);

/* Look up a descriptor by model enum, or NULL if unknown. */
const corrado_prog_desc_t *corrado_prog_find(corrado_prog_model_t model);

/* Human-readable name for a model (never NULL; "unknown" if unmatched). */
const char *corrado_prog_name(corrado_prog_model_t model);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CORRADO_PROG_H */
