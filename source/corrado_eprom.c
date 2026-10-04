/*
 * corrado_eprom.c
 *
 * OS-independent implementation of the Corrado EPROM image library.
 * Shared by every per-OS / per-year application build.
 *
 * SPDX-License-Identifier: MIT
 */

#include "corrado_eprom.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *corrado_strerror(corrado_status_t s)
{
    switch (s) {
        case CORRADO_OK:             return "success";
        case CORRADO_ERR_ARG:        return "invalid argument";
        case CORRADO_ERR_IO:         return "file I/O error";
        case CORRADO_ERR_SIZE:       return "image size mismatch";
        case CORRADO_ERR_CHECKSUM:   return "checksum verification failed";
        case CORRADO_ERR_NO_DEVICE:  return "no programmer found";
        case CORRADO_ERR_USB:        return "USB transport error";
        case CORRADO_ERR_VERIFY:     return "verify mismatch";
        case CORRADO_ERR_UNSUPPORTED:return "unsupported operation";
        case CORRADO_ERR_TIMEOUT:    return "operation timed out";
        default:                     return "unknown error";
    }
}

corrado_status_t corrado_image_alloc(corrado_image_t *img,
                                     corrado_eprom_type_t type)
{
    size_t sz;

    if (!img) return CORRADO_ERR_ARG;
    sz = corrado_eprom_size(type);
    if (sz == 0) return CORRADO_ERR_UNSUPPORTED;

    img->data = (uint8_t *)malloc(sz);
    if (!img->data) return CORRADO_ERR_IO;

    /* An erased EPROM cell reads as 1, so a blank image is all 0xFF. */
    memset(img->data, 0xFF, sz);
    img->type = type;
    img->size = sz;
    return CORRADO_OK;
}

void corrado_image_free(corrado_image_t *img)
{
    if (!img) return;
    free(img->data);
    img->data = NULL;
    img->size = 0;
    img->type = CORRADO_EPROM_UNKNOWN;
}

/* Infer a device type from a raw file byte count. */
static corrado_eprom_type_t type_from_size(long n)
{
    switch (n) {
        case 16384: return CORRADO_EPROM_27C128;
        case 32768: return CORRADO_EPROM_27C256;
        case 65536: return CORRADO_EPROM_27C512;
        default:    return CORRADO_EPROM_UNKNOWN;
    }
}

corrado_status_t corrado_image_load(corrado_image_t *img,
                                    const char *path,
                                    corrado_eprom_type_t type)
{
    FILE *f;
    long  n;
    size_t rd;
    corrado_status_t st;

    if (!img || !path) return CORRADO_ERR_ARG;

    f = fopen(path, "rb");
    if (!f) return CORRADO_ERR_IO;

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return CORRADO_ERR_IO; }
    n = ftell(f);
    if (n < 0) { fclose(f); return CORRADO_ERR_IO; }
    rewind(f);

    if (type == CORRADO_EPROM_UNKNOWN)
        type = type_from_size(n);

    if (type == CORRADO_EPROM_UNKNOWN ||
        (long)corrado_eprom_size(type) != n) {
        fclose(f);
        return CORRADO_ERR_SIZE;
    }

    st = corrado_image_alloc(img, type);
    if (st != CORRADO_OK) { fclose(f); return st; }

    rd = fread(img->data, 1, img->size, f);
    fclose(f);
    if (rd != img->size) {
        corrado_image_free(img);
        return CORRADO_ERR_IO;
    }
    return CORRADO_OK;
}

corrado_status_t corrado_image_save(const corrado_image_t *img,
                                    const char *path)
{
    FILE *f;
    size_t wr;

    if (!img || !img->data || !path) return CORRADO_ERR_ARG;

    f = fopen(path, "wb");
    if (!f) return CORRADO_ERR_IO;

    wr = fwrite(img->data, 1, img->size, f);
    if (fclose(f) != 0 || wr != img->size)
        return CORRADO_ERR_IO;

    return CORRADO_OK;
}

uint16_t corrado_checksum16(const corrado_image_t *img)
{
    uint16_t sum = 0;
    size_t i;

    if (!img || !img->data) return 0;
    for (i = 0; i < img->size; ++i)
        sum = (uint16_t)(sum + img->data[i]);
    return sum;
}

corrado_status_t corrado_checksum_fix(corrado_image_t *img, int verify_only)
{
    uint16_t sum_wo;  /* sum of all bytes except the final checksum word */
    uint16_t need;
    size_t i, cslo, cshi;

    if (!img || !img->data) return CORRADO_ERR_ARG;
    if (img->size < 2)       return CORRADO_ERR_SIZE;

    cslo = img->size - 2;    /* checksum stored little-endian at the tail */
    cshi = img->size - 1;

    sum_wo = 0;
    for (i = 0; i < cslo; ++i)
        sum_wo = (uint16_t)(sum_wo + img->data[i]);

    /* We want (sum_wo + checksum_word) mod 65536 == 0. */
    need = (uint16_t)(0u - sum_wo);

    if (verify_only) {
        uint16_t have = (uint16_t)(img->data[cslo] |
                                   ((uint16_t)img->data[cshi] << 8));
        return (have == need) ? CORRADO_OK : CORRADO_ERR_CHECKSUM;
    }

    img->data[cslo] = (uint8_t)(need & 0xFF);
    img->data[cshi] = (uint8_t)((need >> 8) & 0xFF);
    return CORRADO_OK;
}
