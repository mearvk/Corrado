/*
 * main.c
 *
 * Corrado EPROM programmer command-line front end. Shared across all three
 * operating systems and every model year; linked against the OS-specific
 * USB backend and the per-year profile.c.
 *
 * Usage:
 *   corrado-eprom info
 *   corrado-eprom read  <out.bin>
 *   corrado-eprom write <in.bin> [--no-verify]
 *   corrado-eprom verify <ref.bin>
 *   corrado-eprom blankcheck
 *   corrado-eprom checksum <file.bin> [--fix]
 *
 * SPDX-License-Identifier: MIT
 */

/* Expose localtime_r from <time.h> under -std=c11 on POSIX systems. */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#  define _POSIX_C_SOURCE 200809L
#endif

#include "corrado_eprom.h"
#include "corrado_usb.h"
#include "corrado_profile.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

static const char *ecu_name(corrado_ecu_t e)
{
    switch (e) {
        case CORRADO_ECU_DIGIFANT: return "Bosch Digifant";
        case CORRADO_ECU_MOTRONIC: return "Bosch Motronic";
        default:                   return "unknown";
    }
}

static void print_banner(void)
{
    const corrado_year_profile_t *p = corrado_year_profile();
    printf("Corrado EPROM Tool v%s\n", CORRADO_VERSION_STRING);
    printf("  Model year : %d\n", p->year);
    printf("  Engine     : %s\n", p->engine);
    printf("  ECU        : %s", ecu_name(p->ecu));
    if (p->ecu_part && p->ecu_part[0]) printf("  (part %s)", p->ecu_part);
    printf("\n");
    printf("  EPROM      : %s (%zu bytes)\n",
           corrado_eprom_name(p->eprom), corrado_eprom_size(p->eprom));
    if (p->notes && p->notes[0]) printf("  Notes      : %s\n", p->notes);
}

static void progress(size_t done, size_t total, void *user)
{
    const char *verb = (const char *)user;
    int pct = total ? (int)((done * 100) / total) : 100;
    printf("\r  %s %3d%% (%zu/%zu bytes)", verb, pct, done, total);
    fflush(stdout);
    if (done >= total) printf("\n");
}

static int usage(const char *argv0)
{
    fprintf(stderr,
        "Usage: %s <command> [args]\n"
        "  info                     show year/ECU/EPROM profile\n"
        "  read   <out.bin>         dump the chip to a file\n"
        "  write  <in.bin> [--no-verify]  program the chip from a file\n"
        "  verify <ref.bin>         compare chip against a file\n"
        "  blankcheck               confirm the chip is erased (all 0xFF)\n"
        "  backup [out.bin]         read the chip to a (timestamped) file\n"
        "  copy                     clone one chip to another (prompts a swap)\n"
        "  delete                   electrically erase the chip (reusable parts)\n"
        "  checksum <file.bin> [--fix]  check/repair trailing checksum\n",
        argv0);
    return 2;
}

/* Build a default timestamped backup filename for the current profile. */
static void default_backup_name(char *out, size_t n)
{
    const corrado_year_profile_t *p = corrado_year_profile();
    time_t now = time(NULL);
    struct tm tmv;
#if defined(_WIN32)
    localtime_s(&tmv, &now);
#else
    localtime_r(&now, &tmv);
#endif
    snprintf(out, n, "corrado-%d-%s-%04d%02d%02d-%02d%02d%02d.bin",
             p->year, corrado_eprom_name(p->eprom),
             tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
             tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
}

/* Wait for the operator to swap the physical chip between copy phases. */
static void prompt_swap(const char *msg)
{
    printf("\n>>> %s\n>>> Insert the chip and press Enter to continue...", msg);
    fflush(stdout);
    { int c; while ((c = getchar()) != '\n' && c != EOF) { } }
}

/* checksum sub-command works on a file only; no hardware needed. */
static int cmd_checksum(const char *path, int fix)
{
    corrado_image_t img;
    corrado_status_t st = corrado_image_load(&img, path, CORRADO_EPROM_UNKNOWN);
    if (st != CORRADO_OK) {
        fprintf(stderr, "load '%s': %s\n", path, corrado_strerror(st));
        return 1;
    }
    printf("  additive sum16 : 0x%04X\n", corrado_checksum16(&img));
    st = corrado_checksum_fix(&img, /*verify_only=*/1);
    printf("  trailing cksum : %s\n",
           st == CORRADO_OK ? "OK" : "MISMATCH");
    if (fix && st != CORRADO_OK) {
        corrado_checksum_fix(&img, 0);
        st = corrado_image_save(&img, path);
        printf("  fix written    : %s\n",
               st == CORRADO_OK ? "yes" : corrado_strerror(st));
    }
    corrado_image_free(&img);
    return 0;
}

int main(int argc, char **argv)
{
    const corrado_year_profile_t *p = corrado_year_profile();
    corrado_eprom_type_t type = p->eprom;
    corrado_usb_dev_t *dev = NULL;
    corrado_status_t st;
    int rc = 0;

    if (argc < 2) return usage(argv[0]);

    if (strcmp(argv[1], "info") == 0) {
        print_banner();
        return 0;
    }
    if (strcmp(argv[1], "checksum") == 0) {
        if (argc < 3) return usage(argv[0]);
        return cmd_checksum(argv[2], argc > 3 && !strcmp(argv[3], "--fix"));
    }

    /* Remaining commands need the programmer. */
    print_banner();

    st = corrado_usb_init();
    if (st != CORRADO_OK) {
        fprintf(stderr, "usb init: %s\n", corrado_strerror(st));
        return 1;
    }
    st = corrado_usb_open(&dev);
    if (st != CORRADO_OK) {
        fprintf(stderr, "open programmer: %s\n", corrado_strerror(st));
        corrado_usb_exit();
        return 1;
    }
    printf("  Programmer : %s\n",
           corrado_usb_model(dev) == CORRADO_PROG_TL866II ? "TL866II+" :
           corrado_usb_model(dev) == CORRADO_PROG_TL866A  ? "TL866A/CS" :
                                                            "unknown");

    if (strcmp(argv[1], "read") == 0 && argc >= 3) {
        corrado_image_t img;
        st = corrado_image_alloc(&img, type);
        if (st == CORRADO_OK) {
            st = corrado_usb_read(dev, &img, progress, (void *)"reading");
            if (st == CORRADO_OK) st = corrado_image_save(&img, argv[2]);
            corrado_image_free(&img);
        }
    } else if (strcmp(argv[1], "write") == 0 && argc >= 3) {
        corrado_image_t img;
        st = corrado_image_load(&img, argv[2], type);
        if (st == CORRADO_OK) {
            st = corrado_usb_write(dev, &img, progress, (void *)"writing");
            corrado_image_free(&img);
        }
    } else if (strcmp(argv[1], "verify") == 0 && argc >= 3) {
        corrado_image_t ref, chip;
        st = corrado_image_load(&ref, argv[2], type);
        if (st == CORRADO_OK) {
            st = corrado_image_alloc(&chip, type);
            if (st == CORRADO_OK) {
                st = corrado_usb_read(dev, &chip, progress, (void *)"reading");
                if (st == CORRADO_OK)
                    st = memcmp(ref.data, chip.data, ref.size) == 0
                             ? CORRADO_OK : CORRADO_ERR_VERIFY;
                corrado_image_free(&chip);
            }
            corrado_image_free(&ref);
        }
    } else if (strcmp(argv[1], "blankcheck") == 0) {
        st = corrado_usb_blank_check(dev, type);
    } else if (strcmp(argv[1], "backup") == 0) {
        char auto_name[128];
        const char *out = argv[2];
        if (!out) { default_backup_name(auto_name, sizeof auto_name);
                    out = auto_name; }
        st = corrado_usb_backup(dev, type, out, progress, (void *)"backup");
        if (st == CORRADO_OK) printf("  Backup     : %s\n", out);
    } else if (strcmp(argv[1], "copy") == 0) {
        corrado_image_t buf;
        memset(&buf, 0, sizeof buf);
        prompt_swap("Insert the SOURCE (master) chip to copy FROM.");
        st = corrado_usb_copy(dev, type, CORRADO_COPY_READ, &buf,
                              progress, (void *)"reading");
        if (st == CORRADO_OK) {
            prompt_swap("Now insert the TARGET (blank) chip to copy TO.");
            st = corrado_usb_copy(dev, type, CORRADO_COPY_WRITE, &buf,
                                  progress, (void *)"writing");
            corrado_image_free(&buf);
        }
    } else if (strcmp(argv[1], "delete") == 0) {
        st = corrado_usb_delete(dev, type);
        if (st == CORRADO_ERR_UNSUPPORTED)
            fprintf(stderr,
                "  Note       : a true UV/OTP 27C part cannot be erased\n"
                "               electrically - remove it and use a UV eraser.\n");
    } else {
        corrado_usb_close(dev);
        corrado_usb_exit();
        return usage(argv[0]);
    }

    if (st == CORRADO_OK) {
        printf("  Result     : OK\n");
    } else {
        fprintf(stderr, "  Result     : %s\n", corrado_strerror(st));
        rc = 1;
    }

    corrado_usb_close(dev);
    corrado_usb_exit();
    return rc;
}
