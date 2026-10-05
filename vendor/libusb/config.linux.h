/*
 * config.linux.h — hand-written libusb build config for the Corrado
 * VENDORED build (opt-in). Used when compiling the libusb-1.0.30 source that
 * ships in include/libusb-1.0.30.zip, instead of a system-installed libusb.
 *
 * This selects the Linux usbfs backend with NETLINK hotplug, so the build has
 * NO external dependency on libudev — it needs only pthreads. Modelled on
 * libusb's own hand-written android/config.h (LGPL-2.1).
 *
 * libusb itself is Copyright © its authors, licensed LGPL-2.1-or-later; see
 * the COPYING/AUTHORS files inside include/libusb-1.0.30.zip. This small
 * config header is part of the Corrado repo (MIT) and only selects build
 * options; it contains no libusb source.
 */
#ifndef CORRADO_VENDOR_LIBUSB_CONFIG_LINUX_H
#define CORRADO_VENDOR_LIBUSB_CONFIG_LINUX_H

#define DEFAULT_VISIBILITY __attribute__ ((visibility ("default")))
#define ENABLE_LOGGING 1
#define HAVE_ASM_TYPES_H 1
#define HAVE_CLOCK_GETTIME 1
#define HAVE_EVENTFD 1
#define HAVE_TIMERFD 1
#define HAVE_NFDS_T 1
#define HAVE_PIPE2 1
#define HAVE_PTHREAD_CONDATTR_SETCLOCK 1
#define HAVE_PTHREAD_SETNAME_NP 1
#define HAVE_STRUCT_TIMESPEC 1
#define HAVE_SYS_TIME_H 1
#define PLATFORM_POSIX 1
#define PRINTF_FORMAT(a, b) __attribute__ ((__format__ (__printf__, a, b)))
#define _GNU_SOURCE 1

#endif /* CORRADO_VENDOR_LIBUSB_CONFIG_LINUX_H */
