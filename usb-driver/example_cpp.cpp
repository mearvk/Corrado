/*
 * example_cpp.cpp
 *
 * Minimal example of the portable C++ RAII wrapper (corrado_usb.hpp).
 * Reads the full chip and prints a short hex preview of the first bytes.
 *
 * Build (Linux):
 *   g++ -std=c++17 -I../include -I. example_cpp.cpp \
 *       linux/corrado_usb_linux.c ../source/corrado_eprom.c -lusb-1.0
 *
 * Build (macOS):
 *   g++ -std=c++17 -I../include -I. example_cpp.cpp \
 *       macos/corrado_usb_macos.c ../source/corrado_eprom.c \
 *       $(pkg-config --cflags --libs libusb-1.0)
 *
 * SPDX-License-Identifier: MIT
 */

#include "corrado_usb.hpp"

#include <cstdio>
#include <iostream>

int main()
{
    try {
        corrado::UsbContext ctx;      // init / exit the USB subsystem
        corrado::Programmer  prog;    // open the first TL866-family device

        std::cout << "Programmer: " << prog.modelName() << "\n";

        auto image = prog.read(
            CORRADO_EPROM_27C256,
            [](std::size_t done, std::size_t total) {
                std::printf("\rreading %3zu%%",
                            total ? done * 100 / total : 100);
                std::fflush(stdout);
            });
        std::printf("\nread %zu bytes\n", image.size());

        std::printf("first 16 bytes:");
        for (int i = 0; i < 16 && i < (int)image.size(); ++i)
            std::printf(" %02X", image[i]);
        std::printf("\n");
        return 0;
    }
    catch (const corrado::UsbError &e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
