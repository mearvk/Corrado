/*
 * corrado_usb_macos.c
 *
 * macOS USB backend for the Corrado EPROM programmer, built on libusb-1.0
 * (which sits atop IOKit / IOUSBHost on modern macOS).
 *
 * Hardware: MiniPRO / XGecu minipro family (TL866A/CS, TL866II+, T48, T56).
 *           The supported-programmer set is the corrado_prog_desc_t registry
 *           (usb-driver/corrado_prog.c); this backend is data-driven off it.
 * Build:    cc ... $(pkg-config --cflags --libs libusb-1.0)
 *           Install libusb with Homebrew:  brew install libusb
 * Notes:    No kernel extension or driver signing is required because these
 *           are vendor-specific devices with no competing kext; macOS lets a
 *           user-space process claim them directly through libusb.
 *
 * SPDX-License-Identifier: MIT
 */

#include "corrado_usb.h"
#include "corrado_prog.h"

#include <libusb-1.0/libusb.h>
#include <stdlib.h>
#include <string.h>

struct corrado_usb_dev {
    libusb_device_handle      *h;
    const corrado_prog_desc_t *desc;  /* descriptor for the opened model */
    int                        claimed_iface;
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

/* Walk the programmer registry and open the first attached known device. */
static libusb_device_handle *open_known(const corrado_prog_desc_t **desc)
{
    const corrado_prog_desc_t *d;

    for (d = corrado_prog_table(); d->name; ++d) {
        libusb_device_handle *h;
        if (!d->probe) continue;   /* skip models whose USB id is unconfirmed */
        h = libusb_open_device_with_vid_pid(g_ctx, d->vid, d->pid);
        if (h) { *desc = d; return h; }
    }
    *desc = NULL;
    return NULL;
}

corrado_status_t corrado_usb_open(corrado_usb_dev_t **out)
{
    corrado_usb_dev_t         *d;
    const corrado_prog_desc_t *desc;
    libusb_device_handle      *h;

    if (!out) return CORRADO_ERR_ARG;
    if (!g_ctx) {
        corrado_status_t st = corrado_usb_init();
        if (st != CORRADO_OK) return st;
    }

    h = open_known(&desc);
    if (!h) return CORRADO_ERR_NO_DEVICE;

    /* macOS: no kernel driver to detach for this vendor device, but the
     * call is harmless and keeps the backend uniform with Linux. */
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
    d->desc = desc;
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
    return (dev && dev->desc) ? dev->desc->model : CORRADO_PROG_UNKNOWN;
}

/* ---- minipro-family bulk framing (see Linux backend for notes) -------- */
/* Opcodes, endpoints and block size come from the opened device's
 * descriptor (corrado_prog.c), so this code carries no per-model constants. */

static corrado_status_t bulk_out(corrado_usb_dev_t *d,
                                 const uint8_t *buf, int len)
{
    int moved = 0;
    int r = libusb_bulk_transfer(d->h, d->desc->ep_out, (unsigned char *)buf,
                                 len, &moved, CORRADO_USB_TIMEOUT_MS);
    if (r == LIBUSB_ERROR_TIMEOUT) return CORRADO_ERR_TIMEOUT;
    if (r != 0 || moved != len)    return CORRADO_ERR_USB;
    return CORRADO_OK;
}

static corrado_status_t bulk_in(corrado_usb_dev_t *d, uint8_t *buf, int len)
{
    int moved = 0;
    int r = libusb_bulk_transfer(d->h, d->desc->ep_in, buf, len, &moved,
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

/* Largest transfer block any registered model uses; sizes the write frame. */
#define CORRADO_MAX_BLOCK 0x80

corrado_status_t corrado_usb_read(corrado_usb_dev_t *dev,
                                  corrado_image_t *img,
                                  corrado_progress_cb cb, void *user)
{
    size_t addr, block;
    uint8_t cmd[8];
    corrado_status_t st;

    if (!dev || !img || !img->data) return CORRADO_ERR_ARG;
    block = dev->desc->block;

    for (addr = 0; addr < img->size; addr += block) {
        size_t blk = img->size - addr;
        if (blk > block) blk = block;

        memset(cmd, 0, sizeof(cmd));
        cmd[0] = dev->desc->cmd_read;
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
    size_t addr, block;
    uint8_t frame[8 + CORRADO_MAX_BLOCK];
    corrado_status_t st;
    corrado_image_t readback;

    if (!dev || !img || !img->data) return CORRADO_ERR_ARG;
    block = dev->desc->block;
    if (block == 0 || block > CORRADO_MAX_BLOCK) return CORRADO_ERR_UNSUPPORTED;

    for (addr = 0; addr < img->size; addr += block) {
        size_t blk = img->size - addr;
        if (blk > block) blk = block;

        memset(frame, 0, sizeof(frame));
        frame[0] = dev->desc->cmd_write;
        frame[1] = (uint8_t)blk;
        put_le32(&frame[2], (uint32_t)addr);
        memcpy(&frame[8], img->data + addr, blk);

        st = bulk_out(dev, frame, (int)(8 + blk));
        if (st != CORRADO_OK) return st;

        if (cb) cb(addr + blk, img->size, user);
    }

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
    cmd[0] = dev->desc->cmd_erase;
    cmd[1] = (uint8_t)type;

    st = bulk_out(dev, cmd, (int)sizeof(cmd));
    if (st == CORRADO_ERR_USB) return CORRADO_ERR_UNSUPPORTED;
    if (st != CORRADO_OK)      return st;

    /* Confirm the erase actually took: the whole device must read 0xFF. */
    return corrado_usb_blank_check(dev, type);
}
