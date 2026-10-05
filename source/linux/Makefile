# Makefile -- Corrado EPROM tool, Linux build.
#
# Builds one binary per model year into ../../build/linux/<year>/.
#
# USB transport (two ways to supply libusb-1.0):
#   (default) system libusb -- install:
#               apt install libusb-1.0-0-dev / dnf install libusb1-devel
#   (opt-in)  vendored libusb built from include/libusb-1.0.30.zip:
#               make LIBUSB_VENDOR=1          (needs no system libusb/libudev)
#
#   make                      build every year (system libusb)
#   make YEAR=1992            build a single year
#   make LIBUSB_VENDOR=1      build using the bundled libusb source
#   make clean
#
# SPDX-License-Identifier: MIT

ROOT      := ../..
INCLUDE   := $(ROOT)/include
COMMON    := $(ROOT)/source
PROFILES  := $(COMMON)/linux
USBDIR    := $(ROOT)/usb-driver/linux
BUILD     := $(ROOT)/build/linux
VENDOR    := $(ROOT)/vendor/libusb

YEARS     := 1990 1991 1992 1993 1994 1995 1996
TARGET    := corrado-eprom

CC        ?= cc
CFLAGS    ?= -O2 -Wall -Wextra -std=c11
CPPFLAGS  += -I$(INCLUDE) -I$(COMMON) -I$(USBDIR)

COMMON_SRC := $(COMMON)/corrado_eprom.c $(COMMON)/corrado_ops.c $(COMMON)/main.c
USB_SRC    := $(USBDIR)/corrado_usb_linux.c

# --- USB transport selection -------------------------------------------------
# LIBUSB_VENDOR=1 builds the bundled libusb (vendor/libusb) and links it
# statically; otherwise link the system -lusb-1.0.
ifeq ($(LIBUSB_VENDOR),1)
VENDOR_LIB  := $(VENDOR)/staged/lib/libusb-1.0.a
VENDOR_INC  := $(VENDOR)/staged/include
CPPFLAGS    += -I$(VENDOR_INC)
LDLIBS      += $(VENDOR_LIB) -lpthread
LIBUSB_DEP  := $(VENDOR_LIB)
else
LDLIBS      += -lusb-1.0
LIBUSB_DEP  :=
endif

ifdef YEAR
YEARS := $(YEAR)
endif

.PHONY: all clean vendor-libusb
all: $(foreach y,$(YEARS),$(BUILD)/$(y)/$(TARGET))

# Build the vendored static lib on demand when LIBUSB_VENDOR=1.
$(VENDOR_LIB):
	$(MAKE) -C $(VENDOR)

vendor-libusb: $(VENDOR_LIB)

$(BUILD)/%/$(TARGET): $(COMMON_SRC) $(USB_SRC) $(PROFILES)/%/profile.c $(LIBUSB_DEP)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ \
	    $(COMMON_SRC) $(USB_SRC) $(PROFILES)/$*/profile.c $(LDLIBS)
	@echo "built $@"

clean:
	rm -rf $(BUILD)
	$(MAKE) -C $(VENDOR) clean
