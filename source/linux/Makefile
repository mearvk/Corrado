# Makefile -- Corrado EPROM tool, Linux build.
#
# Builds one binary per model year into ../../build/linux/<year>/.
# Uses libusb-1.0 (install: apt install libusb-1.0-0-dev / dnf install libusb1-devel).
#
#   make              build every year
#   make YEAR=1992    build a single year
#   make clean
#
# SPDX-License-Identifier: MIT

ROOT      := ../..
INCLUDE   := $(ROOT)/include
COMMON    := $(ROOT)/source
USBDIR    := $(ROOT)/usb-driver/linux
BUILD     := $(ROOT)/build/linux

YEARS     := 1990 1991 1992 1993 1994 1995 1996
TARGET    := corrado-eprom

CC        ?= cc
CFLAGS    ?= -O2 -Wall -Wextra -std=c11
CPPFLAGS  += -I$(INCLUDE) -I$(COMMON) -I$(USBDIR)
LDLIBS    += -lusb-1.0

COMMON_SRC := $(COMMON)/corrado_eprom.c $(COMMON)/main.c
USB_SRC    := $(USBDIR)/corrado_usb_linux.c

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
