# Makefile -- FFM shim shared library (direct in-JVM USB path).
#
# Builds libcorrado_ffm.so (Linux) / .dylib (macOS) / corrado_ffm.dll (Windows)
# exposing the flat FFM ABI over the Corrado USB driver + image library. A Java
# host loads it via SLeeLa's DynamiteConnector and binds symbols through the
# JDK Foreign Function & Memory API.
#
#   make                     system libusb
#   make LIBUSB_VENDOR=1      bundled libusb from ../include/libusb-1.0.30.zip
#   make clean
#
# SPDX-License-Identifier: MIT

ROOT      := ..
INCLUDE   := $(ROOT)/include
COMMON    := $(ROOT)/source
VENDOR    := $(ROOT)/vendor/libusb

CC        ?= cc
CFLAGS    ?= -O2 -Wall -Wextra -std=c11 -fPIC -fvisibility=hidden
CPPFLAGS  += -I$(INCLUDE) -I$(COMMON)

# Shared sources: the shim + the image library + the Linux USB backend.
SRC := corrado_ffm.c $(COMMON)/corrado_eprom.c $(ROOT)/usb-driver/linux/corrado_usb_linux.c

LIB := libcorrado_ffm.so

ifeq ($(LIBUSB_VENDOR),1)
VENDOR_LIB := $(VENDOR)/staged/lib/libusb-1.0.a
VENDOR_INC := $(VENDOR)/staged/include
CPPFLAGS   += -I$(VENDOR_INC)
LDLIBS     := $(VENDOR_LIB) -lpthread
LIBUSB_DEP := $(VENDOR_LIB)
else
LDLIBS     := -lusb-1.0
LIBUSB_DEP :=
endif

.PHONY: all clean
all: $(LIB)

# The vendored libusb must be built with -fPIC so its static archive can be
# linked into THIS shared object. (A plain `make -C vendor/libusb` produces a
# non-PIC archive, which the linker rejects for a .so.)
$(VENDOR_LIB):
	$(MAKE) -C $(VENDOR) CFLAGS="-O2 -std=gnu11 -fPIC -fvisibility=hidden"

$(LIB): $(SRC) $(LIBUSB_DEP)
	$(CC) $(CFLAGS) $(CPPFLAGS) -shared -o $@ $(SRC) $(LDLIBS)
	@echo "built $@"

clean:
	rm -f $(LIB)
