/*
 * corrado_ch347.c
 *
 * CH347 high-speed SPI-flash protocol (see corrado_ch347.h). OS-independent;
 * frames the CH347 command protocol + SPI-flash JEDEC opcodes on top of the
 * transport the backend supplies.
 *
 * Protocol notes (CH347 SPI command set, reverse-engineered by the community
 * from the WCH library and documented in the CH347 application manual):
 *   - Every USB packet is [cmd, len_lo, len_hi, payload...] (little-endian
 *     16-bit length of the payload that follows).
 *   - CH347_CMD_SPI_INIT (0xC0)  : configure the SPI engine (mode/speed).
 *   - CH347_CMD_SPI_CONTROL (0xC1): assert/deassert chip-select.
 *   - CH347_CMD_SPI_RD_WR (0xC2) : full-duplex SPI byte exchange; the device
 *     returns the same number of MISO bytes (also as a 0xC2 packet).
 *   Unlike the CH341 the CH347 clocks SPI MSB-first, so no bit reversal.
 *
 * SPDX-License-Identifier: MIT
 */

#include "corrado_ch347.h"

#include <string.h>

/* ---- Descriptive metadata (version 1.11 nested-friendly accessors) ----
 * Worker-class artifact (single-purpose high-speed SPI protocol wrapper).
 *   metaVersion = "1.0.0"  metaId = 4  metaAuthor = "mearvk"
 *   metaDate = "2026-11-02"  metaOrigin = "International"
 *   metaBirthDate = "2026-11-02"  metaScienceRating = 4
 *   metaNationalSchedule = "JEDEC JESD216 (SFDP)"
 * Full suite record: corrado_suite_metadata() in corrado_metadata.c. */

#define CH347_CMD_SPI_INIT    0xC0u
#define CH347_CMD_SPI_CONTROL 0xC1u
#define CH347_CMD_SPI_RD_WR   0xC2u

#define CH347_CS_ASSERT       0x00u
#define CH347_CS_DEASSERT     0x01u

#define CH347_MAX_PAYLOAD     0x200u   /* comfortably within HS bulk packets */

#define SPI_PAGE              256u
#define SPI_CMD_WREN          0x06u
#define SPI_CMD_RDSR          0x05u
#define SPI_SR_WIP            0x01u
#define CH347_POLL_MAX        100000u

/* Send one framed CH347 command packet: [cmd, len_lo, len_hi, payload]. */
static corrado_status_t pkt_out(const corrado_ch341_io_t *io, uint8_t cmd,
                                const uint8_t *payload, uint16_t len)
{
    uint8_t hdr[3];
    corrado_status_t st;

    hdr[0] = cmd;
    hdr[1] = (uint8_t)(len & 0xFFu);
    hdr[2] = (uint8_t)((len >> 8) & 0xFFu);

    st = io->bulk_out(io->ctx, io->ep_out, hdr, (int)sizeof(hdr));
    if (st != CORRADO_OK) return st;
    if (len == 0) return CORRADO_OK;
    return io->bulk_out(io->ctx, io->ep_out, payload, (int)len);
}

/* Read one framed CH347 response packet into 'buf' (expects <= cap bytes of
 * payload); returns the payload length in *out_len. */
static corrado_status_t pkt_in(const corrado_ch341_io_t *io,
                               uint8_t *buf, uint16_t cap, uint16_t *out_len)
{
    uint8_t hdr[3];
    uint16_t len;
    corrado_status_t st = io->bulk_in(io->ctx, io->ep_in, hdr, (int)sizeof(hdr));
    if (st != CORRADO_OK) return st;

    len = (uint16_t)(hdr[1] | ((uint16_t)hdr[2] << 8));
    if (len > cap) return CORRADO_ERR_USB;
    if (out_len) *out_len = len;
    if (len == 0) return CORRADO_OK;
    return io->bulk_in(io->ctx, io->ep_in, buf, (int)len);
}

static corrado_status_t cs(const corrado_ch341_io_t *io, uint8_t state)
{
    uint8_t p = state;
    return pkt_out(io, CH347_CMD_SPI_CONTROL, &p, 1);
}

/*
 * One SPI-flash transaction: assert CS, exchange 'len' full-duplex bytes
 * (MOSI from 'tx', MISO into 'rx', either may be NULL), deassert CS.
 */
static corrado_status_t spi_xfer(const corrado_ch341_io_t *io,
                                 const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    uint8_t out[CH347_MAX_PAYLOAD];
    uint8_t in[CH347_MAX_PAYLOAD];
    uint16_t got = 0;
    corrado_status_t st;

    if (len == 0 || len > CH347_MAX_PAYLOAD) return CORRADO_ERR_ARG;

    st = cs(io, CH347_CS_ASSERT);
    if (st != CORRADO_OK) return st;

    if (tx) memcpy(out, tx, len);
    else    memset(out, 0, len);

    st = pkt_out(io, CH347_CMD_SPI_RD_WR, out, len);
    if (st == CORRADO_OK)
        st = pkt_in(io, in, (uint16_t)sizeof(in), &got);

    /* Always try to release CS, even on error, to leave the bus sane. */
    (void)cs(io, CH347_CS_DEASSERT);

    if (st != CORRADO_OK) return st;
    if (got != len) return CORRADO_ERR_USB;
    if (rx) memcpy(rx, in, len);
    return CORRADO_OK;
}

static corrado_status_t spi_cmd1(const corrado_ch341_io_t *io, uint8_t cmd)
{
    return spi_xfer(io, &cmd, NULL, 1);
}

static corrado_status_t spi_wait_ready(const corrado_ch341_io_t *io)
{
    uint8_t tx[2], rx[2];
    unsigned tries;
    for (tries = 0; tries < CH347_POLL_MAX; ++tries) {
        corrado_status_t st;
        tx[0] = SPI_CMD_RDSR; tx[1] = 0x00u;
        st = spi_xfer(io, tx, rx, 2);
        if (st != CORRADO_OK) return st;
        if ((rx[1] & SPI_SR_WIP) == 0) return CORRADO_OK;
    }
    return CORRADO_ERR_TIMEOUT;
}

static void put_addr24(uint8_t *p, uint32_t a)
{
    p[0] = (uint8_t)((a >> 16) & 0xFFu);
    p[1] = (uint8_t)((a >> 8) & 0xFFu);
    p[2] = (uint8_t)(a & 0xFFu);
}

corrado_status_t corrado_ch347_begin(const corrado_ch341_io_t *io)
{
    /*
     * SPI engine init: mode 0, master, 8-bit, MSB-first. The exact config
     * blob is device-firmware specific; a zeroed config selects the CH347's
     * sane defaults (mode 0, high clock) which suit SPI NOR flash. Reference
     * implementation -- validate against the WCH manual for a specific unit.
     */
    uint8_t cfg[26];
    if (!io) return CORRADO_ERR_ARG;
    memset(cfg, 0, sizeof(cfg));
    return pkt_out(io, CH347_CMD_SPI_INIT, cfg, (uint16_t)sizeof(cfg));
}

corrado_status_t corrado_ch347_read(const corrado_ch341_io_t *io,
                                    const corrado_prog_desc_t *desc,
                                    corrado_image_t *img,
                                    corrado_progress_cb cb, void *user)
{
    size_t addr;
    const size_t chunk = 256;          /* data bytes per read transaction */
    uint8_t tx[4 + 256];
    uint8_t rx[4 + 256];
    corrado_status_t st;

    if (!io || !desc || !img || !img->data) return CORRADO_ERR_ARG;

    for (addr = 0; addr < img->size; addr += chunk) {
        size_t n = img->size - addr;
        if (n > chunk) n = chunk;

        tx[0] = desc->cmd_read;        /* 0x03 READ */
        put_addr24(&tx[1], (uint32_t)addr);
        memset(&tx[4], 0, n);

        st = spi_xfer(io, tx, rx, (uint16_t)(n + 4));
        if (st != CORRADO_OK) return st;

        memcpy(img->data + addr, &rx[4], n);
        if (cb) cb(addr + n, img->size, user);
    }
    return CORRADO_OK;
}

corrado_status_t corrado_ch347_write(const corrado_ch341_io_t *io,
                                     const corrado_prog_desc_t *desc,
                                     const corrado_image_t *img,
                                     corrado_progress_cb cb, void *user)
{
    size_t addr;
    uint8_t tx[4 + SPI_PAGE];
    corrado_status_t st;
    corrado_image_t readback;

    if (!io || !desc || !img || !img->data) return CORRADO_ERR_ARG;

    /* Program a full SPI page at a time; advance by the bytes actually sent. */
    for (addr = 0; addr < img->size; ) {
        size_t n = SPI_PAGE - (addr % SPI_PAGE);
        if (n > img->size - addr) n = img->size - addr;

        st = spi_cmd1(io, SPI_CMD_WREN);
        if (st != CORRADO_OK) return st;

        tx[0] = desc->cmd_write;       /* 0x02 PAGE PROGRAM */
        put_addr24(&tx[1], (uint32_t)addr);
        memcpy(&tx[4], img->data + addr, n);

        st = spi_xfer(io, tx, NULL, (uint16_t)(n + 4));
        if (st != CORRADO_OK) return st;

        st = spi_wait_ready(io);
        if (st != CORRADO_OK) return st;

        addr += n;
        if (cb) cb(addr, img->size, user);
    }

    st = corrado_image_alloc(&readback, img->type);
    if (st != CORRADO_OK) return st;
    st = corrado_ch347_read(io, desc, &readback, NULL, NULL);
    if (st == CORRADO_OK &&
        memcmp(readback.data, img->data, img->size) != 0)
        st = CORRADO_ERR_VERIFY;
    corrado_image_free(&readback);
    return st;
}

corrado_status_t corrado_ch347_blank_check(const corrado_ch341_io_t *io,
                                           const corrado_prog_desc_t *desc,
                                           corrado_eprom_type_t type)
{
    corrado_image_t img;
    corrado_status_t st;
    size_t i;

    st = corrado_image_alloc(&img, type);
    if (st != CORRADO_OK) return st;
    st = corrado_ch347_read(io, desc, &img, NULL, NULL);
    if (st == CORRADO_OK)
        for (i = 0; i < img.size; ++i)
            if (img.data[i] != 0xFFu) { st = CORRADO_ERR_VERIFY; break; }
    corrado_image_free(&img);
    return st;
}

corrado_status_t corrado_ch347_erase(const corrado_ch341_io_t *io,
                                     const corrado_prog_desc_t *desc,
                                     corrado_eprom_type_t type)
{
    corrado_status_t st;
    if (!io || !desc) return CORRADO_ERR_ARG;
    if (corrado_eprom_size(type) == 0) return CORRADO_ERR_UNSUPPORTED;

    st = spi_cmd1(io, SPI_CMD_WREN);
    if (st != CORRADO_OK) return st;
    st = spi_cmd1(io, desc->cmd_erase);   /* 0xC7 CHIP ERASE */
    if (st != CORRADO_OK) return st;
    st = spi_wait_ready(io);
    if (st != CORRADO_OK) return st;

    return corrado_ch347_blank_check(io, desc, type);
}
