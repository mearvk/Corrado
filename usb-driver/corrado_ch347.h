/*
 * corrado_ch347.h
 *
 * OS-independent CH347 high-speed SPI-flash protocol for the Corrado EPROM
 * suite -- the bench-grade / "strong worker" programmer.
 *
 * The WinChipHead CH347 (USB 0x1A86/0x55DB) is a USB 2.0 high-speed USB-to-SPI
 * bridge, markedly faster than the CH341A, and is the unit a working mechanic
 * reaches for when flashing many chips. Like the CH341A module it drives a
 * standard SPI NOR flash with JEDEC opcodes, but the CH347 wraps them in its
 * own command protocol (0xC0 INIT / 0xC1 CS-CONTROL / 0xC2 read-write) over
 * bulk endpoints 0x06 (OUT) / 0x86 (IN) on interface 2, with a 3-byte packet
 * header [cmd, len_lo, len_hi]. This is a third protocol family; the backend
 * dispatches to it when the descriptor's family == CORRADO_FAMILY_CH347.
 *
 * It reuses the corrado_ch341_io_t transport (generic bulk out/in supplied by
 * the backend) so the module stays libusb-free and unit-testable.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef CORRADO_CH347_H
#define CORRADO_CH347_H

#include "corrado_eprom.h"
#include "corrado_usb.h"
#include "corrado_prog.h"
#include "corrado_ch341.h"   /* reuse corrado_ch341_io_t transport */

#ifdef __cplusplus
extern "C" {
#endif

/* One-time SPI engine init (mode 0, 8-bit, MSB-first). Call after open. */
corrado_status_t corrado_ch347_begin(const corrado_ch341_io_t *io);

corrado_status_t corrado_ch347_read(const corrado_ch341_io_t *io,
                                    const corrado_prog_desc_t *desc,
                                    corrado_image_t *img,
                                    corrado_progress_cb cb, void *user);

corrado_status_t corrado_ch347_write(const corrado_ch341_io_t *io,
                                     const corrado_prog_desc_t *desc,
                                     const corrado_image_t *img,
                                     corrado_progress_cb cb, void *user);

corrado_status_t corrado_ch347_erase(const corrado_ch341_io_t *io,
                                     const corrado_prog_desc_t *desc,
                                     corrado_eprom_type_t type);

corrado_status_t corrado_ch347_blank_check(const corrado_ch341_io_t *io,
                                           const corrado_prog_desc_t *desc,
                                           corrado_eprom_type_t type);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CORRADO_CH347_H */
