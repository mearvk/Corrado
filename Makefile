# Top-level Makefile for the Corrado EPROM programmer suite.
#
# Detects the host OS and delegates to the matching per-OS Makefile under
# source/<os>/. Override with:  make OS=linux|macos|windows
#
#   make            build every model year for the detected OS
#   make YEAR=1992  build a single year
#   make clean
#   make info       print the detected configuration
#
# SPDX-License-Identifier: MIT

UNAME_S := $(shell uname -s 2>/dev/null)
ifeq ($(UNAME_S),Linux)
  OS ?= linux
else ifeq ($(UNAME_S),Darwin)
  OS ?= macos
else
  OS ?= windows
endif

SUBDIR := source/$(OS)

.PHONY: all clean info
all:
	$(MAKE) -C $(SUBDIR) $(if $(YEAR),YEAR=$(YEAR),)

clean:
	$(MAKE) -C $(SUBDIR) clean

info:
	@echo "Detected OS : $(UNAME_S)"
	@echo "Build target: $(OS)"
	@echo "Subdir      : $(SUBDIR)"
	@echo "Years       : 1990..1996 (override with YEAR=<year>)"
