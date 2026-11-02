/*
 * corrado_ch341.c
 *
 * CH341A SPI-flash protocol implementation (see corrado_ch341.h). OS-
 * independent: it frames CH341 + SPI bytes on top of the transport supplied by
 * the backend. Compiled into every per-year binary alongside the other shared
 * source.
 *
 * Protocol notes (from the public CH341 datasheet and open-source tools such
 * as ch341prog / flashrom):
 *   - CH341A_CMD_SPI_STREAM (0xA8) prefixes a full-duplex SPI byte stream on
 *     bulk EP 0x02; the device returns the MISO bytes on EP 0x82. CS is
 *     asserted for the duration of one stream packet.
 *   - The CH341 shifts SPI bytes LSB-first on the wire, so every data byte is
 *     bit-reversed on the way out and on the way back (swap_bits()).
 *   - SPI NOR flash is driven with standard JEDEC opcodes: 0x03 READ (24-bit
 *     address), 0x06 WREN, 0x02 PAGE PROGRAM (<=256 B, must not cross a page),
 *     0x05 RDSR (bit0 = WIP), 0xC7 CHIP ERASE.
 *
 * SPDX-License-Identifier: MIT
 */

#include "corrado_ch341.h"

#include <string.h>

/* ---- Descriptive metadata (version 1.11 nested-friendly accessors) ----
 * Worker-class artifact (single-purpose SPI protocol wrapper); baseline id.
 *   metaVersion = "1.0.0"         metaId = 3
 *   metaAuthor  = "mearvk"        metaDate = "2026-11-02"
 *   metaOrigin  = "International"  metaBirthDate = "2026-11-02"
 *   metaScienceRating = 5         metaNationalSchedule = "JEDEC JESD216 (SFDP)"
 * (JESD216 is the real serial-flash discoverable-parameter standard the SPI
 *  NOR opcodes used here conform to.) Full suite record: corrado_metadata.c. */

#define CH341_CMD_SPI_STREAM 0xA8u
#define CH341_PACKET_LEN     0x20u   /* 32-byte USB packet granularity */

#define SPI_PAGE             256u    /* SPI NOR page-program window */
#define SPI_CMD_WREN         0x06u
#define SPI_CMD_RDSR         0x05u
#define SPI_SR_WIP           0x01u   /* status register: write-in-progress */

/* How many RDSR polls to wait for a program/erase to finish. */
#define CH341_POLL_MAX       100000u

static uint8_t swap_bits(uint8_t b)
{
    b = (uint8_t)(((b & 0xF0u) >> 4) | ((b & 0x0Fu) << 4));
    b = (uint8_t)(((b & 0xCCu) >> 2) | ((b & 0x33u) << 2));
    b = (uint8_t)(((b & 0xAAu) >> 1) | ((b & 0x55u) << 1));
    return b;
}

/*
 * One full-duplex SPI transaction: 'len' MOSI bytes in 'tx' are clocked out
 * (CS low for the whole packet) and the simultaneous MISO bytes land in 'rx'
 * (either may be NULL / caller ignores). len must be <= CH341_PACKET_LEN - 1
 * because the 0xA8 opcode occupies one byte of the 32-byte packet.
 */
static corrado_status_t spi_xfer(const corrado_ch341_io_t *io,
                                 const uint8_t *tx, uint8_t *rx, size_t len)
{
    uint8_t pkt[CH341_PACKET_LEN];
    uint8_t in[CH341_PACKET_LEN];
    corrado_status_t st;
    size_t i;

    if (len == 0 || len > CH341_PACKET_LEN - 1) return CORRADO_ERR_ARG;

    pkt[0] = CH341_CMD_SPI_STREAM;
    for (i = 0; i < len; ++i)
        pkt[1 + i] = swap_bits(tx ? tx[i] : 0x00u);

    st = io->bulk_out(io->ctx, io->ep_out, pkt, (int)(len + 1));
    if (st != CORRADO_OK) return st;

    st = io->bulk_in(io->ctx, io->ep_in, in, (int)len);
    if (st != CORRADO_OK) return st;

    if (rx)
        for (i = 0; i < len; ++i)
            rx[i] = swap_bits(in[i]);
    return CORRADO_OK;
}

/* Send a single one-byte SPI command (e.g. WREN). */
static corrado_status_t spi_cmd1(const corrado_ch341_io_t *io, uint8_t cmd)
{
    return spi_xfer(io, &cmd, NULL, 1);
}

/* Poll RDSR until WIP clears (program/erase complete) or we give up. */
static corrado_status_t spi_wait_ready(const corrado_ch341_io_t *io)
{
    uint8_t tx[2], rx[2];
    unsigned tries;

    for (tries = 0; tries < CH341_POLL_MAX; ++tries) {
        corrado_status_t st;
        tx[0] = SPI_CMD_RDSR;
        tx[1] = 0x00u;
        st = spi_xfer(io, tx, rx, 2);
        if (st != CORRADO_OK) return st;
        if ((rx[1] & SPI_SR_WIP) == 0) return CORRADO_OK;
    }
    return CORRADO_ERR_TIMEOUT;
}

static void put_addr24(uint8_t *p, uint32_t addr)
{
    p[0] = (uint8_t)((addr >> 16) & 0xFFu);
    p[1] = (uint8_t)((addr >> 8) & 0xFFu);
    p[2] = (uint8_t)(addr & 0xFFu);
}

corrado_status_t corrado_ch341_read(const corrado_ch341_io_t *io,
                                    const corrado_prog_desc_t *desc,
                                    corrado_image_t *img,
                                    corrado_progress_cb cb, void *user)
{
    size_t addr;
    /* 1 opcode + 3 address bytes leaves this many data bytes per 32B packet. */
    const size_t chunk = CH341_PACKET_LEN - 1 - 4; /* = 27 */
    uint8_t tx[CH341_PACKET_LEN - 1];
    uint8_t rx[CH341_PACKET_LEN - 1];
    corrado_status_t st;

    if (!io || !desc || !img || !img->data) return CORRADO_ERR_ARG;

    for (addr = 0; addr < img->size; addr += chunk) {
        size_t n = img->size - addr;
        if (n > chunk) n = chunk;

        tx[0] = desc->cmd_read;            /* 0x03 READ */
        put_addr24(&tx[1], (uint32_t)addr);
        memset(&tx[4], 0, n);              /* dummy clocks for the read data */

        st = spi_xfer(io, tx, rx, n + 4);
        if (st != CORRADO_OK) return st;

        memcpy(img->data + addr, &rx[4], n);
        if (cb) cb(addr + n, img->size, user);
    }
    return CORRADO_OK;
}

corrado_status_t corrado_ch341_write(const corrado_ch341_io_t *io,
                                     const corrado_prog_desc_t *desc,
                                     const corrado_image_t *img,
                                     corrado_progress_cb cb, void *user)
{
    size_t addr;
    /* 1 opcode + 3 address bytes of overhead per page-program packet. */
    const size_t chunk = CH341_PACKET_LEN - 1 - 4; /* = 27 */
    uint8_t tx[CH341_PACKET_LEN - 1];
    corrado_status_t st;
    corrado_image_t readback;

    if (!io || !desc || !img || !img->data) return CORRADO_ERR_ARG;

    /* Advance by the ACTUAL bytes programmed each pass (n), which may be
     * clamped below 'chunk' at a page boundary -- never by the fixed chunk,
     * or the clamped tail bytes would be skipped. */
    for (addr = 0; addr < img->size; ) {
        size_t n = img->size - addr;
        if (n > chunk) n = chunk;
        /* Never let a page-program cross a 256-byte page boundary. */
        if ((addr % SPI_PAGE) + n > SPI_PAGE)
            n = SPI_PAGE - (addr % SPI_PAGE);

        st = spi_cmd1(io, SPI_CMD_WREN);
        if (st != CORRADO_OK) return st;

        tx[0] = desc->cmd_write;           /* 0x02 PAGE PROGRAM */
        put_addr24(&tx[1], (uint32_t)addr);
        memcpy(&tx[4], img->data + addr, n);

        st = spi_xfer(io, tx, NULL, n + 4);
        if (st != CORRADO_OK) return st;

        st = spi_wait_ready(io);
        if (st != CORRADO_OK) return st;

        addr += n;
        if (cb) cb(addr, img->size, user);
    }

    /* Verify by reading the device back and comparing. */
    st = corrado_image_alloc(&readback, img->type);
    if (st != CORRADO_OK) return st;

    st = corrado_ch341_read(io, desc, &readback, NULL, NULL);
    if (st == CORRADO_OK &&
        memcmp(readback.data, img->data, img->size) != 0)
        st = CORRADO_ERR_VERIFY;

    corrado_image_free(&readback);
    return st;
}

corrado_status_t corrado_ch341_blank_check(const corrado_ch341_io_t *io,
                                           const corrado_prog_desc_t *desc,
                                           corrado_eprom_type_t type)
{
    corrado_image_t img;
    corrado_status_t st;
    size_t i;

    st = corrado_image_alloc(&img, type);
    if (st != CORRADO_OK) return st;

    st = corrado_ch341_read(io, desc, &img, NULL, NULL);
    if (st == CORRADO_OK) {
        for (i = 0; i < img.size; ++i) {
            if (img.data[i] != 0xFFu) { st = CORRADO_ERR_VERIFY; break; }
        }
    }
    corrado_image_free(&img);
    return st;
}

corrado_status_t corrado_ch341_erase(const corrado_ch341_io_t *io,
                                     const corrado_prog_desc_t *desc,
                                     corrado_eprom_type_t type)
{
    corrado_status_t st;

    if (!io || !desc) return CORRADO_ERR_ARG;
    if (corrado_eprom_size(type) == 0) return CORRADO_ERR_UNSUPPORTED;

    st = spi_cmd1(io, SPI_CMD_WREN);
    if (st != CORRADO_OK) return st;

    st = spi_cmd1(io, desc->cmd_erase);    /* 0xC7 CHIP ERASE */
    if (st != CORRADO_OK) return st;

    st = spi_wait_ready(io);
    if (st != CORRADO_OK) return st;

    return corrado_ch341_blank_check(io, desc, type);
}
