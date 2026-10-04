/*
 * corrado_ops.c
 *
 * OS-independent high-level chip operations for the Corrado EPROM suite:
 * backup (chip -> file), copy (chip -> chip), and delete (erase + verify).
 * These build on the per-OS low-level USB primitives (read/write/erase)
 * so the logic is written and tested once for all three platforms.
 *
 * SPDX-License-Identifier: MIT
 */

#include "corrado_usb.h"

#include <string.h>

corrado_status_t corrado_usb_backup(corrado_usb_dev_t *dev,
                                    corrado_eprom_type_t type,
                                    const char *path,
                                    corrado_progress_cb cb, void *user)
{
    corrado_image_t img;
    corrado_status_t st;

    if (!dev || !path) return CORRADO_ERR_ARG;

    st = corrado_image_alloc(&img, type);
    if (st != CORRADO_OK) return st;

    st = corrado_usb_read(dev, &img, cb, user);
    if (st == CORRADO_OK)
        st = corrado_image_save(&img, path);

    corrado_image_free(&img);
    return st;
}

corrado_status_t corrado_usb_copy(corrado_usb_dev_t *dev,
                                  corrado_eprom_type_t type,
                                  corrado_copy_phase_t phase,
                                  corrado_image_t *buf,
                                  corrado_progress_cb cb, void *user)
{
    if (!dev || !buf) return CORRADO_ERR_ARG;

    if (phase == CORRADO_COPY_READ) {
        /* Read the master chip into a freshly allocated buffer. */
        corrado_status_t st = corrado_image_alloc(buf, type);
        if (st != CORRADO_OK) return st;

        st = corrado_usb_read(dev, buf, cb, user);
        if (st != CORRADO_OK) {
            corrado_image_free(buf);
            return st;
        }
        return CORRADO_OK;
    }

    if (phase == CORRADO_COPY_WRITE) {
        /* Program the previously-read buffer into the target chip. */
        if (!buf->data) return CORRADO_ERR_ARG;
        if (buf->type != type || buf->size != corrado_eprom_size(type))
            return CORRADO_ERR_SIZE;
        /* corrado_usb_write() programs and then verifies by read-back. */
        return corrado_usb_write(dev, buf, cb, user);
    }

    return CORRADO_ERR_ARG;
}

corrado_status_t corrado_usb_delete(corrado_usb_dev_t *dev,
                                    corrado_eprom_type_t type)
{
    if (!dev) return CORRADO_ERR_ARG;
    /* corrado_usb_erase() already erases then blank-checks. */
    return corrado_usb_erase(dev, type);
}
