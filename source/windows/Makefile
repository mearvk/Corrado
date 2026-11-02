# Makefile -- Corrado EPROM tool, Windows / MinGW (gcc) build.
#
# Builds one binary per model year into ..\..\build\windows\<year>\.
# Needs a MinGW-w64 toolchain and libusb-1.0 for MinGW.
# For an MSVC build, use the sibling Makefile.msvc with nmake instead.
#
#   mingw32-make              build every year
#   mingw32-make YEAR=1996    build a single year
#   mingw32-make clean
#
# SPDX-License-Identifier: MIT

ROOT      := ../..
INCLUDE   := $(ROOT)/include
COMMON    := $(ROOT)/source
PROFILES  := $(COMMON)/windows
USBDIR    := $(ROOT)/usb-driver/windows
USBCOMMON := $(ROOT)/usb-driver
BUILD     := $(ROOT)/build/windows

YEARS     := 1990 1991 1992 1993 1994 1995 1996
TARGET    := corrado-eprom.exe

CC        ?= gcc
CFLAGS    ?= -O2 -Wall -Wextra -std=c11
CPPFLAGS  += -I$(INCLUDE) -I$(COMMON) -I$(USBDIR) -I$(USBCOMMON)
LDLIBS    += -lusb-1.0

COMMON_SRC := $(COMMON)/corrado_eprom.c $(COMMON)/corrado_ops.c \
              $(COMMON)/corrado_prog.c $(COMMON)/corrado_ch341.c \
              $(COMMON)/corrado_ch347.c $(COMMON)/corrado_metadata.c \
              $(COMMON)/main.c
USB_SRC    := $(USBDIR)/corrado_usb_windows.c

ifdef YEAR
YEARS := $(YEAR)
endif

.PHONY: all clean
all: $(foreach y,$(YEARS),$(BUILD)/$(y)/$(TARGET))

$(BUILD)/%/$(TARGET): $(COMMON_SRC) $(USB_SRC) $(PROFILES)/%/profile.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ \
	    $(COMMON_SRC) $(USB_SRC) $(PROFILES)/$*/profile.c $(LDLIBS)
	@echo built $@

clean:
	rm -rf $(BUILD)
