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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CORRADO_USB_H */
