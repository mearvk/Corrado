/*
 * corrado_ch341.h
 *
 * OS-independent CH341A SPI-flash protocol for the Corrado EPROM suite.
 *
 * The WinChipHead CH341A ("CH341A mini programmer" dongle, USB 0x1A86/0x5512)
 * is a USB-to-SPI bridge. To read/write a Corrado-compatible image we drive a
 * standard SPI NOR flash sitting in its socket with JEDEC opcodes, each
 * wrapped in the CH341 0xA8 SPI-stream command over bulk endpoints 0x02 (OUT)
 * and 0x82 (IN). This is a different wire protocol from the minipro family, so
 * it lives in its own module and the per-OS backend dispatches to it when the
 * opened device's descriptor has family == CORRADO_FAMILY_CH341A.
 *
 * To stay OS-independent and unit-testable, this module does NOT call libusb
 * directly: the backend supplies a tiny transport (bulk out/in function
 * pointers + an opaque handle) via corrado_ch341_io_t, and this code frames
 * the CH341/SPI bytes on top of it.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef CORRADO_CH341_H
#define CORRADO_CH341_H

#include "corrado_eprom.h"   /* corrado_status_t, corrado_image_t */
#include "corrado_usb.h"     /* corrado_progress_cb */
#include "corrado_prog.h"    /* corrado_prog_desc_t */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Transport vtable the backend supplies. bulk_out/bulk_in move raw bytes over
 * the already-claimed CH341 bulk endpoints; they return CORRADO_OK on a full
 * transfer. 'ctx' is passed back untouched (typically the backend's device
 * handle). ep_out/ep_in come from the descriptor.
 */
typedef struct {
    void *ctx;
    uint8_t ep_out;
    uint8_t ep_in;
    corrado_status_t (*bulk_out)(void *ctx, uint8_t ep,
                                 const uint8_t *buf, int len);
    corrado_status_t (*bulk_in)(void *ctx, uint8_t ep,
                                uint8_t *buf, int len);
} corrado_ch341_io_t;

/* Read the whole SPI flash into 'img' (already allocated for the target). */
corrado_status_t corrado_ch341_read(const corrado_ch341_io_t *io,
                                    const corrado_prog_desc_t *desc,
                                    corrado_image_t *img,
                                    corrado_progress_cb cb, void *user);

/* Program 'img' into the SPI flash (page program), then verify by read-back. */
corrado_status_t corrado_ch341_write(const corrado_ch341_io_t *io,
                                     const corrado_prog_desc_t *desc,
                                     const corrado_image_t *img,
                                     corrado_progress_cb cb, void *user);

/* Chip-erase the SPI flash, then confirm it reads blank (0xFF). */
corrado_status_t corrado_ch341_erase(const corrado_ch341_io_t *io,
                                     const corrado_prog_desc_t *desc,
                                     corrado_eprom_type_t type);

/* Confirm the whole SPI flash reads as 0xFF. */
corrado_status_t corrado_ch341_blank_check(const corrado_ch341_io_t *io,
                                           const corrado_prog_desc_t *desc,
                                           corrado_eprom_type_t type);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CORRADO_CH341_H */
