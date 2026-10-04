/*
 * corrado_usb.h
 *
 * OS-independent USB programmer interface for the Corrado EPROM suite.
 *
 * The reference programmer hardware is the MiniPRO TL866 family
 * (TL866A / TL866CS / TL866II+ / T48), the de-facto open-hardware
 * EPROM programmer driven by libusb on all three platforms.
 *
 *   TL866A / TL866CS : USB VID 0x04D8 (Microchip) PID 0xE11C
 *   TL866II+         : USB VID 0xA466            PID 0x0A53
 *
 * Each OS provides a backend implementing the functions declared here:
 *   - usb-driver/linux   : libusb-1.0 backend
 *   - usb-driver/macos   : libusb-1.0 backend (IOKit/IOUSBHost under it)
 *   - usb-driver/windows : WinUSB / libusb-1.0 backend
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef CORRADO_USB_H
#define CORRADO_USB_H

#include "corrado_eprom.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Known TL866-family USB identifiers. */
#define TL866A_VID    0x04D8u
#define TL866A_PID    0xE11Cu
#define TL866II_VID   0xA466u
#define TL866II_PID   0x0A53u

/* Bulk endpoints used by the TL866 protocol. */
#define TL866_EP_OUT  0x01u
#define TL866_EP_IN   0x81u

/* Default USB transfer timeout, milliseconds. */
#define CORRADO_USB_TIMEOUT_MS 5000

typedef enum {
    CORRADO_PROG_TL866A = 0,
    CORRADO_PROG_TL866II,
    CORRADO_PROG_UNKNOWN
} corrado_prog_model_t;

/* Opaque handle to an open programmer. Backend-defined contents. */
typedef struct corrado_usb_dev corrado_usb_dev_t;

/* Progress callback: 'done'/'total' bytes, 'user' is the opaque pointer
 * passed to the read/write call. May be NULL. */
typedef void (*corrado_progress_cb)(size_t done, size_t total, void *user);

/* Initialise the USB subsystem for this process. Call once at start. */
corrado_status_t corrado_usb_init(void);

/* Tear down the USB subsystem. Call once at exit. */
void corrado_usb_exit(void);

/* Open the first attached TL866-family programmer. On success *out holds
 * an allocated handle to be released with corrado_usb_close(). */
corrado_status_t corrado_usb_open(corrado_usb_dev_t **out);

/* Close and free a handle from corrado_usb_open(). */
void corrado_usb_close(corrado_usb_dev_t *dev);

/* Query the detected programmer model. */
corrado_prog_model_t corrado_usb_model(const corrado_usb_dev_t *dev);

/* Read the entire device into 'img' (already allocated for the target
 * EPROM type). Invokes 'cb' with progress if non-NULL. */
corrado_status_t corrado_usb_read(corrado_usb_dev_t *dev,
                                  corrado_image_t *img,
                                  corrado_progress_cb cb, void *user);

/* Program (write) the entire image to the device, then verify. */
corrado_status_t corrado_usb_write(corrado_usb_dev_t *dev,
                                   const corrado_image_t *img,
                                   corrado_progress_cb cb, void *user);

/* Blank-check: confirm the whole device reads as 0xFF (erased). */
corrado_status_t corrado_usb_blank_check(corrado_usb_dev_t *dev,
                                         corrado_eprom_type_t type);

/* Electrically erase the device, then confirm it reads blank (0xFF).
 *
 * NOTE: a genuine 27C-series part is a UV-erasable (and often one-time
 * programmable) EPROM that CANNOT be erased electrically - it must be
 * removed and placed under a UV eraser. This call therefore returns
 * CORRADO_ERR_UNSUPPORTED for pure UV/OTP devices. It succeeds for the
 * pin-compatible, in-circuit-erasable replacements commonly dropped into
 * these sockets (e.g. SST27SF512, Winbond W27C512, 28C256 EEPROM). */
corrado_status_t corrado_usb_erase(corrado_usb_dev_t *dev,
                                   corrado_eprom_type_t type);

/* ---- High-level chip operations (built on read/write/erase) ----------- */

/* BACKUP: read the whole chip and save it to a raw binary file. This is
 * the chip -> file direction; always back up a stock chip before writing. */
corrado_status_t corrado_usb_backup(corrado_usb_dev_t *dev,
                                    corrado_eprom_type_t type,
                                    const char *path,
                                    corrado_progress_cb cb, void *user);

/* COPY (chip -> chip) in two phases that share one in-memory buffer:
 *
 *   phase CORRADO_COPY_READ  : read the SOURCE/master chip into *buf
 *                              (allocates *buf; caller frees with
 *                               corrado_image_free()).
 *   phase CORRADO_COPY_WRITE : program *buf into the TARGET chip and
 *                              verify (does not free *buf).
 *
 * The caller swaps the physical chip in the programmer between the two
 * phases. See corrado_usb_copy_oneshot() for the file-backed convenience. */
typedef enum {
    CORRADO_COPY_READ = 0,
    CORRADO_COPY_WRITE
} corrado_copy_phase_t;

corrado_status_t corrado_usb_copy(corrado_usb_dev_t *dev,
                                  corrado_eprom_type_t type,
                                  corrado_copy_phase_t phase,
                                  corrado_image_t *buf,
                                  corrado_progress_cb cb, void *user);

/* DELETE: electrically erase the chip (reusable parts only) and verify it
 * is blank. Thin wrapper over corrado_usb_erase() for symmetry with the
 * backup/copy verbs. */
corrado_status_t corrado_usb_delete(corrado_usb_dev_t *dev,
                                    corrado_eprom_type_t type);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CORRADO_USB_H */
