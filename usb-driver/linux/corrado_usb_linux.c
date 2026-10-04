/*
 * corrado_usb_linux.c
 *
 * Linux USB backend for the Corrado EPROM programmer, built on libusb-1.0.
 *
 * Hardware: MiniPRO TL866 family (TL866A/CS, TL866II+).
 * Build:    cc ... -lusb-1.0
 * Access:   install the udev rule in this directory so a non-root user in
 *           the 'plugdev' group can open the device.
 *
 * SPDX-License-Identifier: MIT
 */

#include "corrado_usb.h"

#include <libusb-1.0/libusb.h>
#include <stdlib.h>
#include <string.h>

struct corrado_usb_dev {
    libusb_device_handle *h;
    corrado_prog_model_t  model;
    int                   claimed_iface;
};

static libusb_context *g_ctx = NULL;

corrado_status_t corrado_usb_init(void)
{
    if (g_ctx) return CORRADO_OK;
    if (libusb_init(&g_ctx) != 0) {
        g_ctx = NULL;
        return CORRADO_ERR_USB;
    }
    return CORRADO_OK;
}

void corrado_usb_exit(void)
{
    if (g_ctx) {
        libusb_exit(g_ctx);
        g_ctx = NULL;
    }
}

/* Try to open one of the known TL866 VID/PID pairs. */
static libusb_device_handle *open_known(corrado_prog_model_t *model)
{
    libusb_device_handle *h;

    h = libusb_open_device_with_vid_pid(g_ctx, TL866II_VID, TL866II_PID);
    if (h) { *model = CORRADO_PROG_TL866II; return h; }

    h = libusb_open_device_with_vid_pid(g_ctx, TL866A_VID, TL866A_PID);
    if (h) { *model = CORRADO_PROG_TL866A; return h; }

    *model = CORRADO_PROG_UNKNOWN;
    return NULL;
}

corrado_status_t corrado_usb_open(corrado_usb_dev_t **out)
{
    corrado_usb_dev_t   *d;
    corrado_prog_model_t model;
    libusb_device_handle *h;

    if (!out) return CORRADO_ERR_ARG;
    if (!g_ctx) {
        corrado_status_t st = corrado_usb_init();
        if (st != CORRADO_OK) return st;
    }

    h = open_known(&model);
    if (!h) return CORRADO_ERR_NO_DEVICE;

    /* Detach a kernel driver if one grabbed the interface. */
    libusb_set_auto_detach_kernel_driver(h, 1);

    if (libusb_claim_interface(h, 0) != 0) {
        libusb_close(h);
        return CORRADO_ERR_USB;
    }

    d = (corrado_usb_dev_t *)calloc(1, sizeof(*d));
    if (!d) {
        libusb_release_interface(h, 0);
        libusb_close(h);
        return CORRADO_ERR_IO;
    }
    d->h = h;
    d->model = model;
    d->claimed_iface = 1;
    *out = d;
    return CORRADO_OK;
}

void corrado_usb_close(corrado_usb_dev_t *dev)
{
    if (!dev) return;
    if (dev->h) {
        if (dev->claimed_iface)
            libusb_release_interface(dev->h, 0);
        libusb_close(dev->h);
    }
    free(dev);
}

corrado_prog_model_t corrado_usb_model(const corrado_usb_dev_t *dev)
{
    return dev ? dev->model : CORRADO_PROG_UNKNOWN;
}

/* ---- TL866 bulk framing (simplified) ---------------------------------- */
/*
 * The TL866 protocol exchanges fixed-size command blocks over bulk
 * endpoints 0x01 (OUT) and 0x81 (IN). The command bytes below follow the
 * minipro open-source project's conventions. Addresses and lengths are
 * little-endian. This is a readable reference implementation of the EPROM
 * read / write path for the 27C-series parts used in Corrado ECUs.
 */
#define TL866_CMD_READ_CODE  0x05
#define TL866_CMD_WRITE_CODE 0x06
#define TL866_CMD_ERASE      0x19  /* chip erase (reusable/flash parts)   */
#define TL866_BLOCK          0x80  /* 128-byte transfer granularity */

static corrado_status_t bulk_out(corrado_usb_dev_t *d,
                                 const uint8_t *buf, int len)
{
    int moved = 0;
    int r = libusb_bulk_transfer(d->h, TL866_EP_OUT, (unsigned char *)buf,
                                 len, &moved, CORRADO_USB_TIMEOUT_MS);
    if (r == LIBUSB_ERROR_TIMEOUT) return CORRADO_ERR_TIMEOUT;
    if (r != 0 || moved != len)    return CORRADO_ERR_USB;
    return CORRADO_OK;
}

static corrado_status_t bulk_in(corrado_usb_dev_t *d, uint8_t *buf, int len)
{
    int moved = 0;
    int r = libusb_bulk_transfer(d->h, TL866_EP_IN, buf, len, &moved,
                                 CORRADO_USB_TIMEOUT_MS);
    if (r == LIBUSB_ERROR_TIMEOUT) return CORRADO_ERR_TIMEOUT;
    if (r != 0 || moved != len)    return CORRADO_ERR_USB;
    return CORRADO_OK;
}

static void put_le32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}

corrado_status_t corrado_usb_read(corrado_usb_dev_t *dev,
                                  corrado_image_t *img,
                                  corrado_progress_cb cb, void *user)
{
    size_t addr;
    uint8_t cmd[8];
    corrado_status_t st;

    if (!dev || !img || !img->data) return CORRADO_ERR_ARG;

    for (addr = 0; addr < img->size; addr += TL866_BLOCK) {
        size_t blk = img->size - addr;
        if (blk > TL866_BLOCK) blk = TL866_BLOCK;

        memset(cmd, 0, sizeof(cmd));
        cmd[0] = TL866_CMD_READ_CODE;
        cmd[1] = (uint8_t)blk;
        put_le32(&cmd[2], (uint32_t)addr);

        st = bulk_out(dev, cmd, (int)sizeof(cmd));
        if (st != CORRADO_OK) return st;

        st = bulk_in(dev, img->data + addr, (int)blk);
        if (st != CORRADO_OK) return st;

        if (cb) cb(addr + blk, img->size, user);
    }
    return CORRADO_OK;
}

corrado_status_t corrado_usb_write(corrado_usb_dev_t *dev,
                                   const corrado_image_t *img,
                                   corrado_progress_cb cb, void *user)
{
    size_t addr;
    uint8_t frame[8 + TL866_BLOCK];
    corrado_status_t st;
    corrado_image_t readback;

    if (!dev || !img || !img->data) return CORRADO_ERR_ARG;

    for (addr = 0; addr < img->size; addr += TL866_BLOCK) {
        size_t blk = img->size - addr;
        if (blk > TL866_BLOCK) blk = TL866_BLOCK;

        memset(frame, 0, sizeof(frame));
        frame[0] = TL866_CMD_WRITE_CODE;
        frame[1] = (uint8_t)blk;
        put_le32(&frame[2], (uint32_t)addr);
        memcpy(&frame[8], img->data + addr, blk);

        st = bulk_out(dev, frame, (int)(8 + blk));
        if (st != CORRADO_OK) return st;

        if (cb) cb(addr + blk, img->size, user);
    }

    /* Verify by reading the device back and comparing. */
    st = corrado_image_alloc(&readback, img->type);
    if (st != CORRADO_OK) return st;

    st = corrado_usb_read(dev, &readback, NULL, NULL);
    if (st == CORRADO_OK &&
        memcmp(readback.data, img->data, img->size) != 0)
        st = CORRADO_ERR_VERIFY;

    corrado_image_free(&readback);
    return st;
}

corrado_status_t corrado_usb_blank_check(corrado_usb_dev_t *dev,
                                         corrado_eprom_type_t type)
{
    corrado_image_t img;
    corrado_status_t st;
    size_t i;

    st = corrado_image_alloc(&img, type);
    if (st != CORRADO_OK) return st;

    st = corrado_usb_read(dev, &img, NULL, NULL);
    if (st == CORRADO_OK) {
        for (i = 0; i < img.size; ++i) {
            if (img.data[i] != 0xFF) { st = CORRADO_ERR_VERIFY; break; }
        }
    }
    corrado_image_free(&img);
    return st;
}

corrado_status_t corrado_usb_erase(corrado_usb_dev_t *dev,
                                   corrado_eprom_type_t type)
{
    uint8_t cmd[8];
    corrado_status_t st;

    if (!dev) return CORRADO_ERR_ARG;
    if (corrado_eprom_size(type) == 0) return CORRADO_ERR_UNSUPPORTED;

    /*
     * Issue the TL866 chip-erase command. A genuine UV-erasable / OTP
     * 27C-series part cannot be erased electrically: the programmer
     * rejects the command, which surfaces here as a USB/transport error
     * that we translate to CORRADO_ERR_UNSUPPORTED so the caller can tell
     * the user to use a UV eraser. Pin-compatible reusable replacements
     * (SST27SF512, W27C512, 28C256, ...) accept the command.
     */
    memset(cmd, 0, sizeof(cmd));
    cmd[0] = TL866_CMD_ERASE;
    cmd[1] = (uint8_t)type;

    st = bulk_out(dev, cmd, (int)sizeof(cmd));
    if (st == CORRADO_ERR_USB) return CORRADO_ERR_UNSUPPORTED;
    if (st != CORRADO_OK)      return st;

    /* Confirm the erase actually took: the whole device must read 0xFF. */
    return corrado_usb_blank_check(dev, type);
}
