# Makefile -- VENDORED libusb build (opt-in).
#
# Builds a static libusb-1.0.a from the source archive shipped in
# include/libusb-1.0.30.zip, so the Corrado tool can be built on a machine
# that has no system libusb installed. This is OPT-IN: the normal build still
# uses a system libusb (see Dependencies in the top-level README). Enable the
# vendored path by passing LIBUSB_VENDOR=1 to the Linux source Makefile.
#
# Linux-only here (usbfs + netlink backend, no libudev needed -> pthreads only).
# The macOS/Windows backends have their own system frameworks; for those,
# install libusb via Homebrew / MSYS2 / the MSVC .7z as the README describes.
#
# Targets:
#   make            unzip + build  staged/lib/libusb-1.0.a and staged header
#   make clean      remove the staged build (leaves the .zip untouched)
#
# SPDX-License-Identifier: MIT  (this Makefile; libusb itself is LGPL-2.1)

ROOT    := ../..
ZIP     := $(ROOT)/include/libusb-1.0.30.zip
VERSION := 1.0.30
WORK    := staged
SRCDIR  := $(WORK)/libusb-$(VERSION)
LUDIR   := $(SRCDIR)/libusb
INCOUT  := $(WORK)/include/libusb-1.0
LIBOUT  := $(WORK)/lib
LIB     := $(LIBOUT)/libusb-1.0.a

CC      ?= cc
CFLAGS  ?= -O2 -std=gnu11 -fvisibility=hidden

# Core + Linux (usbfs/netlink/posix) sources. No udev backend on purpose.
LU_SRCS := core.c descriptor.c hotplug.c io.c strerror.c sync.c \
           os/linux_usbfs.c os/events_posix.c os/threads_posix.c \
           os/linux_netlink.c

OBJS := $(patsubst %.c,$(LUDIR)/%.o,$(LU_SRCS))

# Because the .c files do not exist until the archive is unzipped, the whole
# compile+archive is driven as one recipe guarded by a stamp file. This keeps
# make from trying to build objects before the sources exist.
.PHONY: all clean
all: $(LIB)

$(LIB): $(ZIP) config.linux.h
	@mkdir -p $(WORK)
	unzip -q -o $(ZIP) -d $(WORK)
	cp config.linux.h $(LUDIR)/config.h
	@mkdir -p $(INCOUT) $(LIBOUT)
	cp $(LUDIR)/libusb.h $(INCOUT)/libusb.h
	@set -e; for s in $(LU_SRCS); do \
	    echo "  CC libusb/$$s"; \
	    $(CC) $(CFLAGS) -I$(LUDIR) -c $(LUDIR)/$$s -o $(LUDIR)/$${s%.c}.o; \
	done
	$(AR) rcs $(LIB) $(OBJS)
	@echo "built vendored $(LIB) (+ header in $(INCOUT))"

clean:
	rm -rf $(WORK)
