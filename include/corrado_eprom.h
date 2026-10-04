/*
 * corrado_eprom.h
 *
 * Shared definitions for the Corrado EPROM programmer suite.
 *
 * Target vehicle:  Volkswagen Corrado (Typ 53I), model years 1990-1996.
 *   - G60  (1.8 supercharged)  : Bosch Digifant engine control unit.
 *                                e.g. ECU 0261200280, VW part 037906022B.
 *   - VR6  (2.8/2.9, 1992+)    : Bosch Motronic engine control unit.
 *
 * Target memory device:  27C256-family UV-erasable EPROM.
 *   - Organisation : 32K x 8  (32768 bytes, 256 Kbit).
 *   - Package      : 28-pin DIP, socketed on the ECU main board.
 *   - Supply       : single +5V, TTL-level programming signals.
 * A 27C512 (64 KB) variant is supported for twin-tune switch adapters.
 *
 * This header is OS-independent and is shared by the per-OS, per-year
 * application sources and by the USB programmer drivers.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef CORRADO_EPROM_H
#define CORRADO_EPROM_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Library version -------------------------------------------------- */
#define CORRADO_VERSION_MAJOR 1
#define CORRADO_VERSION_MINOR 0
#define CORRADO_VERSION_PATCH 0
#define CORRADO_VERSION_STRING "1.0.0"

/* ---- Supported EPROM device types ------------------------------------- */
typedef enum {
    CORRADO_EPROM_27C256 = 0, /* 32768 bytes - standard Corrado ECU chip   */
    CORRADO_EPROM_27C512,     /* 65536 bytes - twin-tune / 512 adapters    */
    CORRADO_EPROM_27C128,     /* 16384 bytes - early/alternate ROMs        */
    CORRADO_EPROM_UNKNOWN
} corrado_eprom_type_t;

/* Byte size of a given EPROM device type. Returns 0 for UNKNOWN. */
static inline size_t corrado_eprom_size(corrado_eprom_type_t t)
{
    switch (t) {
        case CORRADO_EPROM_27C128: return 16384u;
        case CORRADO_EPROM_27C256: return 32768u;
        case CORRADO_EPROM_27C512: return 65536u;
        default:                   return 0u;
    }
}

/* Human-readable device name. */
static inline const char *corrado_eprom_name(corrado_eprom_type_t t)
{
    switch (t) {
        case CORRADO_EPROM_27C128: return "27C128";
        case CORRADO_EPROM_27C256: return "27C256";
        case CORRADO_EPROM_27C512: return "27C512";
        default:                   return "UNKNOWN";
    }
}

/* ---- Engine management system identification -------------------------- */
typedef enum {
    CORRADO_ECU_DIGIFANT = 0, /* G60 supercharged 1.8                      */
    CORRADO_ECU_MOTRONIC,     /* VR6 2.8 / 2.9                             */
    CORRADO_ECU_UNKNOWN
} corrado_ecu_t;

/* ---- Return / status codes -------------------------------------------- */
typedef enum {
    CORRADO_OK = 0,
    CORRADO_ERR_ARG         = -1,  /* bad argument / null pointer          */
    CORRADO_ERR_IO          = -2,  /* file or stream I/O failure           */
    CORRADO_ERR_SIZE        = -3,  /* image size mismatch for device       */
    CORRADO_ERR_CHECKSUM    = -4,  /* checksum verification failed         */
    CORRADO_ERR_NO_DEVICE   = -5,  /* no USB programmer found              */
    CORRADO_ERR_USB         = -6,  /* USB transport error                  */
    CORRADO_ERR_VERIFY      = -7,  /* read-back verify mismatch            */
    CORRADO_ERR_UNSUPPORTED = -8,  /* operation not supported              */
    CORRADO_ERR_TIMEOUT     = -9   /* operation timed out                  */
} corrado_status_t;

/* Textual description of a status code. */
const char *corrado_strerror(corrado_status_t s);

/* ---- ROM image container ---------------------------------------------- */
typedef struct {
    corrado_eprom_type_t type;
    uint8_t             *data;   /* owned buffer, corrado_eprom_size(type) */
    size_t               size;   /* == corrado_eprom_size(type)            */
} corrado_image_t;

/* Allocate a blank (0xFF filled, i.e. erased) image for a device type. */
corrado_status_t corrado_image_alloc(corrado_image_t *img,
                                     corrado_eprom_type_t type);

/* Release an image allocated by corrado_image_alloc/corrado_image_load. */
void corrado_image_free(corrado_image_t *img);

/* Load a raw binary ROM dump from disk into an image. The device type
 * is inferred from the file size when 'type' is CORRADO_EPROM_UNKNOWN. */
corrado_status_t corrado_image_load(corrado_image_t *img,
                                    const char *path,
                                    corrado_eprom_type_t type);

/* Write an image out to a raw binary file. */
corrado_status_t corrado_image_save(const corrado_image_t *img,
                                    const char *path);

/* ---- Checksums -------------------------------------------------------- */
/* Simple 16-bit additive checksum over the whole image (sum of bytes). */
uint16_t corrado_checksum16(const corrado_image_t *img);

/* Recompute and store the Digifant/Motronic style trailing checksum so
 * that the 16-bit sum of all bytes equals 0. The checksum word occupies
 * the last two bytes of the image (little-endian). Returns the stored
 * word. Safe no-op friendly: pass verify_only=1 to only check. */
corrado_status_t corrado_checksum_fix(corrado_image_t *img, int verify_only);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CORRADO_EPROM_H */
