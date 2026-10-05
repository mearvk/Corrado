# Makefile -- FFM shim shared library (direct in-JVM USB path).
#
# Builds the flat-ABI shim over the Corrado USB driver + image library into a
# shared library a Java host loads via SLeeLa's DynamiteConnector and binds
# through the JDK Foreign Function & Memory API:
#
#   Linux   : libcorrado_ffm.so     (usb-driver/linux,   libusb-1.0)
#   macOS   : libcorrado_ffm.dylib   (usb-driver/macos,   libusb-1.0 via Homebrew)
#   Windows : corrado_ffm.dll        (usb-driver/windows, libusb-1.0 over WinUSB)
#
#   make                     system libusb (all OSes)
#   make LIBUSB_VENDOR=1      bundled libusb from ../include/libusb-1.0.30.zip
#                            (Linux only; macOS/Windows use system libusb)
#   make clean
#
# OS is auto-detected from `uname`; override with OS=linux|macos|windows.
#
# SPDX-License-Identifier: MIT

ROOT      := ..
INCLUDE   := $(ROOT)/include
COMMON    := $(ROOT)/source
VENDOR    := $(ROOT)/vendor/libusb

# ---- OS detection ----------------------------------------------------------
UNAME_S := $(shell uname -s 2>/dev/null)
ifeq ($(UNAME_S),Linux)
  OS ?= linux
else ifeq ($(UNAME_S),Darwin)
  OS ?= macos
else
  OS ?= windows
endif

CC        ?= cc
CFLAGS    ?= -O2 -Wall -Wextra -std=c11 -fvisibility=hidden
CPPFLAGS  += -I$(INCLUDE) -I$(COMMON)

# ---- per-OS: USB backend source, output name, link/compile flags -----------
ifeq ($(OS),linux)
  USB_SRC  := $(ROOT)/usb-driver/linux/corrado_usb_linux.c
  LIB      := libcorrado_ffm.so
  CFLAGS   += -fPIC
  SOFLAGS  := -shared
  SYS_LIBS := -lusb-1.0
else ifeq ($(OS),macos)
  USB_SRC  := $(ROOT)/usb-driver/macos/corrado_usb_macos.c
  LIB      := libcorrado_ffm.dylib
  CFLAGS   += -fPIC
  SOFLAGS  := -dynamiclib
  # Resolve libusb via pkg-config; fall back to Homebrew prefixes.
  PKG_CFLAGS := $(shell pkg-config --cflags libusb-1.0 2>/dev/null)
  PKG_LIBS   := $(shell pkg-config --libs   libusb-1.0 2>/dev/null)
  ifeq ($(strip $(PKG_LIBS)),)
    PKG_CFLAGS := -I/opt/homebrew/include -I/usr/local/include
    PKG_LIBS   := -L/opt/homebrew/lib -L/usr/local/lib -lusb-1.0
  endif
  CPPFLAGS += $(PKG_CFLAGS)
  SYS_LIBS := $(PKG_LIBS)
else
  # Windows (MinGW). For MSVC, build the .dll with cl /LD against libusb-1.0.lib.
  USB_SRC  := $(ROOT)/usb-driver/windows/corrado_usb_windows.c
  LIB      := corrado_ffm.dll
  SOFLAGS  := -shared
  SYS_LIBS := -lusb-1.0
endif

SRC := corrado_ffm.c $(COMMON)/corrado_eprom.c $(USB_SRC)

# ---- libusb strategy: system (default) or bundled (Linux only) -------------
ifeq ($(LIBUSB_VENDOR),1)
  ifneq ($(OS),linux)
    $(error LIBUSB_VENDOR=1 is Linux-only; macOS/Windows use a system libusb)
  endif
  VENDOR_LIB := $(VENDOR)/staged/lib/libusb-1.0.a
  CPPFLAGS   += -I$(VENDOR)/staged/include
  LDLIBS     := $(VENDOR_LIB) -lpthread
else
  LDLIBS     := $(SYS_LIBS)
endif

.PHONY: all clean vendor-pic
all: $(LIB)

# Force a clean -fPIC rebuild of the vendored libusb before linking (Linux),
# as an order-only prerequisite: a prior non-PIC build (e.g. from the C CLI)
# would otherwise be reused and the shared-object link would fail.
ifeq ($(LIBUSB_VENDOR),1)
$(LIB): | vendor-pic
endif

vendor-pic:
	$(MAKE) -C $(VENDOR) clean
	$(MAKE) -C $(VENDOR) CFLAGS="-O2 -std=gnu11 -fPIC -fvisibility=hidden"

$(LIB): $(SRC)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SOFLAGS) -o $@ $(SRC) $(LDLIBS)
	@echo "built $@ (OS=$(OS))"

clean:
	rm -f libcorrado_ffm.so libcorrado_ffm.dylib corrado_ffm.dll
