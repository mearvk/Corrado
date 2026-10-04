/*
 * corrado_usb.hpp
 *
 * C++ RAII wrapper over the Corrado USB programmer C backend. This single
 * header is portable across Linux, macOS and Windows: it links against
 * whichever corrado_usb_<os>.c backend is compiled into the target.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef CORRADO_USB_HPP
#define CORRADO_USB_HPP

#include "corrado_usb.h"

#include <functional>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdint>

namespace corrado {

/* Exception carrying a corrado_status_t. */
class UsbError : public std::runtime_error {
public:
    explicit UsbError(corrado_status_t s)
        : std::runtime_error(corrado_strerror(s)), status_(s) {}
    corrado_status_t status() const noexcept { return status_; }
private:
    corrado_status_t status_;
};

/* RAII owner of the global USB subsystem. Create one at program start. */
class UsbContext {
public:
    UsbContext() {
        corrado_status_t st = corrado_usb_init();
        if (st != CORRADO_OK) throw UsbError(st);
    }
    ~UsbContext() { corrado_usb_exit(); }

    UsbContext(const UsbContext &) = delete;
    UsbContext &operator=(const UsbContext &) = delete;
};

using ProgressFn = std::function<void(std::size_t done, std::size_t total)>;

/* RAII handle to one attached programmer. */
class Programmer {
public:
    Programmer() {
        corrado_status_t st = corrado_usb_open(&dev_);
        if (st != CORRADO_OK) throw UsbError(st);
    }
    ~Programmer() { if (dev_) corrado_usb_close(dev_); }

    Programmer(const Programmer &) = delete;
    Programmer &operator=(const Programmer &) = delete;
    Programmer(Programmer &&o) noexcept : dev_(o.dev_) { o.dev_ = nullptr; }

    corrado_prog_model_t model() const { return corrado_usb_model(dev_); }

    std::string modelName() const {
        switch (model()) {
            case CORRADO_PROG_TL866A:  return "TL866A/CS";
            case CORRADO_PROG_TL866II: return "TL866II+";
            default:                   return "unknown";
        }
    }

    /* Read the whole device into a byte vector. */
    std::vector<std::uint8_t> read(corrado_eprom_type_t type,
                                   ProgressFn progress = {}) {
        corrado_image_t img{};
        corrado_status_t st = corrado_image_alloc(&img, type);
        if (st != CORRADO_OK) throw UsbError(st);

        st = corrado_usb_read(dev_, &img, trampoline,
                              progress ? &progress : nullptr);
        if (st != CORRADO_OK) {
            corrado_image_free(&img);
            throw UsbError(st);
        }
        std::vector<std::uint8_t> out(img.data, img.data + img.size);
        corrado_image_free(&img);
        return out;
    }

    /* Program the device from a byte buffer, then verify. */
    void write(corrado_eprom_type_t type,
               const std::vector<std::uint8_t> &bytes,
               ProgressFn progress = {}) {
        corrado_image_t img{};
        corrado_status_t st = corrado_image_alloc(&img, type);
        if (st != CORRADO_OK) throw UsbError(st);
        if (bytes.size() != img.size) {
            corrado_image_free(&img);
            throw UsbError(CORRADO_ERR_SIZE);
        }
        std::copy(bytes.begin(), bytes.end(), img.data);

        st = corrado_usb_write(dev_, &img, trampoline,
                               progress ? &progress : nullptr);
        corrado_image_free(&img);
        if (st != CORRADO_OK) throw UsbError(st);
    }

    bool blankCheck(corrado_eprom_type_t type) {
        corrado_status_t st = corrado_usb_blank_check(dev_, type);
        if (st == CORRADO_OK)         return true;
        if (st == CORRADO_ERR_VERIFY) return false;
        throw UsbError(st);
    }

    /* BACKUP: read the whole chip and save it to a raw binary file. */
    void backup(corrado_eprom_type_t type, const std::string &path,
                ProgressFn progress = {}) {
        corrado_status_t st = corrado_usb_backup(
            dev_, type, path.c_str(), trampoline,
            progress ? &progress : nullptr);
        if (st != CORRADO_OK) throw UsbError(st);
    }

    /* COPY, phase 1: read the master chip and return its contents. Swap
     * in the target chip, then call writeCopy() with the same bytes. */
    std::vector<std::uint8_t> readCopy(corrado_eprom_type_t type,
                                       ProgressFn progress = {}) {
        return read(type, std::move(progress));
    }

    /* COPY, phase 2: program bytes captured by readCopy() into the chip
     * now in the socket, then verify. */
    void writeCopy(corrado_eprom_type_t type,
                   const std::vector<std::uint8_t> &bytes,
                   ProgressFn progress = {}) {
        write(type, bytes, std::move(progress));
    }

    /* DELETE: electrically erase the chip (reusable parts only) and
     * verify it is blank. Throws UsbError(CORRADO_ERR_UNSUPPORTED) for a
     * true UV/OTP 27C part - use a UV eraser for those. */
    void erase(corrado_eprom_type_t type) {
        corrado_status_t st = corrado_usb_delete(dev_, type);
        if (st != CORRADO_OK) throw UsbError(st);
    }

private:
    static void trampoline(std::size_t done, std::size_t total, void *user) {
        if (user) (*static_cast<ProgressFn *>(user))(done, total);
    }
    corrado_usb_dev_t *dev_ = nullptr;
};

} // namespace corrado

#endif // CORRADO_USB_HPP
