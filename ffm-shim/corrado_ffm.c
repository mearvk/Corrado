/*
 * corrado_ffm.c
 *
 * A thin, FFM-friendly C ABI over the Corrado USB driver + image library, so a
 * Java host can drive the real programmer through the JDK Foreign Function &
 * Memory API (loaded via SLeeLa's DynamiteConnector) without JNI and without
 * marshalling the opaque corrado_usb_dev_t / corrado_image_t structs.
 *
 * The shim keeps ONE process-global open device and hands Java only:
 *   - plain int status codes (mirroring corrado_status_t),
 *   - plain int sizes / models,
 *   - caller-provided byte buffers for read/write.
 *
 * This is the "direct in-JVM USB path": Java FFM -> this shim -> corrado_usb_* ->
 * libusb -> TL866 -> EPROM. The same safety posture applies as the rest of the
 * suite: back up stock first, verify every write.
 *
 * Build (Linux): cc -shared -fPIC -o libcorrado_ffm.so corrado_ffm.c \
 *                   ../source/corrado_eprom.c ../usb-driver/linux/corrado_usb_linux.c \
 *                   -lusb-1.0
 *
 * SPDX-License-Identifier: MIT
 */

#include "corrado_usb.h"   /* includes corrado_eprom.h */
#include <string.h>

#ifdef _WIN32
#  define CFFM_EXPORT __declspec(dllexport)
#else
#  define CFFM_EXPORT __attribute__((visibility("default")))
#endif

/* One process-global device handle, opened by cffm_open(). */
static corrado_usb_dev_t *g_dev = NULL;

/* Map a device-type ordinal from Java to the C enum.
 *   0 = 27C128, 1 = 27C256, 2 = 27C512  (matches CorradoEprom / EpromType order
 *   for the standard parts; see cffm_device_size()). */
static corrado_eprom_type_t type_of(int ordinal)
{
    switch (ordinal) {
        case 0:  return CORRADO_EPROM_27C128;
        case 1:  return CORRADO_EPROM_27C256;
        case 2:  return CORRADO_EPROM_27C512;
        default: return CORRADO_EPROM_UNKNOWN;
    }
}

/* ---- lifecycle -------------------------------------------------------- */

/* Initialise libusb and open the first TL866. Returns a corrado_status_t int. */
CFFM_EXPORT int cffm_open(void)
{
    corrado_status_t st = corrado_usb_init();
    if (st != CORRADO_OK) return (int)st;
    if (g_dev) return (int)CORRADO_OK;          /* already open */
    st = corrado_usb_open(&g_dev);
    if (st != CORRADO_OK) g_dev = NULL;
    return (int)st;
}

/* Close the device and tear down libusb. Always safe to call. */
CFFM_EXPORT void cffm_close(void)
{
    if (g_dev) { corrado_usb_close(g_dev); g_dev = NULL; }
    corrado_usb_exit();
}

/* Detected programmer model as an int (corrado_prog_model_t), or -1 if closed. */
CFFM_EXPORT int cffm_model(void)
{
    if (!g_dev) return -1;
    return (int)corrado_usb_model(g_dev);
}

/* Byte size for a device-type ordinal; 0 if unknown. */
CFFM_EXPORT int cffm_device_size(int type_ordinal)
{
    return (int)corrado_eprom_size(type_of(type_ordinal));
}

/* ---- operations ------------------------------------------------------- */

/* Read the whole device into caller buffer 'out' of 'out_len' bytes. The
 * buffer must be exactly the device size. Returns corrado_status_t int. */
CFFM_EXPORT int cffm_read(int type_ordinal, unsigned char *out, int out_len)
{
    corrado_eprom_type_t t = type_of(type_ordinal);
    corrado_image_t img;
    corrado_status_t st;

    if (!g_dev)   return (int)CORRADO_ERR_NO_DEVICE;
    if (!out)     return (int)CORRADO_ERR_ARG;
    if ((int)corrado_eprom_size(t) != out_len) return (int)CORRADO_ERR_SIZE;

    st = corrado_image_alloc(&img, t);
    if (st != CORRADO_OK) return (int)st;

    st = corrado_usb_read(g_dev, &img, NULL, NULL);
    if (st == CORRADO_OK) memcpy(out, img.data, img.size);

    corrado_image_free(&img);
    return (int)st;
}

/* Program the whole device from caller buffer 'in' of 'in_len' bytes (must be
 * exactly the device size), then verify by read-back. Returns status int. */
CFFM_EXPORT int cffm_write(int type_ordinal, const unsigned char *in, int in_len)
{
    corrado_eprom_type_t t = type_of(type_ordinal);
    corrado_image_t img;
    corrado_status_t st;

    if (!g_dev)  return (int)CORRADO_ERR_NO_DEVICE;
    if (!in)     return (int)CORRADO_ERR_ARG;
    if ((int)corrado_eprom_size(t) != in_len) return (int)CORRADO_ERR_SIZE;

    st = corrado_image_alloc(&img, t);
    if (st != CORRADO_OK) return (int)st;
    memcpy(img.data, in, img.size);

    st = corrado_usb_write(g_dev, &img, NULL, NULL);   /* programs then verifies */

    corrado_image_free(&img);
    return (int)st;
}

/* Blank-check: 0 => blank (CORRADO_OK), nonzero => a corrado_status_t int
 * (CORRADO_ERR_VERIFY if not blank). */
CFFM_EXPORT int cffm_blank_check(int type_ordinal)
{
    if (!g_dev) return (int)CORRADO_ERR_NO_DEVICE;
    return (int)corrado_usb_blank_check(g_dev, type_of(type_ordinal));
}

/* Electrically erase (reusable parts only) then blank-check. Returns status
 * int; CORRADO_ERR_UNSUPPORTED for a genuine UV/OTP 27C part. */
CFFM_EXPORT int cffm_erase(int type_ordinal)
{
    if (!g_dev) return (int)CORRADO_ERR_NO_DEVICE;
    return (int)corrado_usb_erase(g_dev, type_of(type_ordinal));
}
