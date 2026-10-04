# Makefile -- Corrado EPROM tool, macOS build.
#
# Builds one binary per model year into ../../build/macos/<year>/.
# Uses libusb-1.0 via Homebrew (install: brew install libusb pkg-config).
#
#   make              build every year
#   make YEAR=1994    build a single year
#   make clean
#
# SPDX-License-Identifier: MIT

ROOT      := ../..
INCLUDE   := $(ROOT)/include
COMMON    := $(ROOT)/source
USBDIR    := $(ROOT)/usb-driver/macos
BUILD     := $(ROOT)/build/macos

YEARS     := 1990 1991 1992 1993 1994 1995 1996
TARGET    := corrado-eprom

CC        ?= cc
CFLAGS    ?= -O2 -Wall -Wextra -std=c11
# Resolve libusb via pkg-config; falls back to Homebrew default prefix.
PKG_CFLAGS := $(shell pkg-config --cflags libusb-1.0 2>/dev/null)
PKG_LIBS   := $(shell pkg-config --libs   libusb-1.0 2>/dev/null)
ifeq ($(strip $(PKG_LIBS)),)
PKG_CFLAGS := -I/opt/homebrew/include -I/usr/local/include
PKG_LIBS   := -L/opt/homebrew/lib -L/usr/local/lib -lusb-1.0
endif

CPPFLAGS  += -I$(INCLUDE) -I$(COMMON) -I$(USBDIR) $(PKG_CFLAGS)
LDLIBS    += $(PKG_LIBS)

COMMON_SRC := $(COMMON)/corrado_eprom.c $(COMMON)/corrado_ops.c $(COMMON)/main.c
USB_SRC    := $(USBDIR)/corrado_usb_macos.c

ifdef YEAR
YEARS := $(YEAR)
endif

.PHONY: all clean
all: $(foreach y,$(YEARS),$(BUILD)/$(y)/$(TARGET))

$(BUILD)/%/$(TARGET): $(COMMON_SRC) $(USB_SRC) $(COMMON)/%/profile.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ \
	    $(COMMON_SRC) $(USB_SRC) $(COMMON)/$*/profile.c $(LDLIBS)
	@echo "built $@"

clean:
	rm -rf $(BUILD)
