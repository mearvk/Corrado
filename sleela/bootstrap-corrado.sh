#!/usr/bin/env bash
#
# bootstrap-corrado.sh
#
# SLeeLa-side bootstrap for the Corrado USB->EPROM runtime: download (clone) the
# Corrado repository from GitHub, build every layer, and report any errors so a
# host can decide whether it can run the Corrado program.
#
# What it builds (each independently; a failure in one does not abort the rest):
#   1. C CLI            (make -C source/linux [LIBUSB_VENDOR=1])  -> corrado-eprom
#   2. Java core port   (javac java/src/com/corrado/eprom/*.java)
#   3. sleela series    (javac sleela/java/.../**.java)           connector+control+FFM
#   4. FFM shim .so     (make -C ffm-shim [LIBUSB_VENDOR=1])      direct in-JVM USB path
#
# It prefers a system libusb; if absent it falls back to the repo's bundled
# libusb (LIBUSB_VENDOR=1) so the native pieces still build offline.
#
# Usage:
#   sleela/bootstrap-corrado.sh [--dir DEST] [--ref BRANCH_OR_TAG]
#                               [--repo URL] [--no-clone] [--vendor-libusb]
#
#   --dir DEST        where to clone/build (default: ./corrado-runtime)
#   --ref REF         branch, tag or commit to check out (default: main)
#   --repo URL        source repo (default: https://github.com/mearvk/Corrado.git)
#   --no-clone        build an existing checkout in DEST (skip the download)
#   --vendor-libusb   force the bundled libusb even if a system one exists
#
# Exit code: number of build steps that FAILED (0 = all good).
#
# SPDX-License-Identifier: MIT

set -u

# ---- configuration ---------------------------------------------------------
REPO_URL="https://github.com/mearvk/Corrado.git"
REF="main"
DEST="corrado-runtime"
DO_CLONE=1
FORCE_VENDOR=0

while [ $# -gt 0 ]; do
  case "$1" in
    --dir)            DEST="${2:?--dir needs a path}"; shift 2 ;;
    --ref)            REF="${2:?--ref needs a value}"; shift 2 ;;
    --repo)           REPO_URL="${2:?--repo needs a URL}"; shift 2 ;;
    --no-clone)       DO_CLONE=0; shift ;;
    --vendor-libusb)  FORCE_VENDOR=1; shift ;;
    -h|--help)        sed -n '2,40p' "$0"; exit 0 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

# ---- small helpers ---------------------------------------------------------
STEP_FAILS=0
log()   { printf '%s\n' "== $* =="; }
info()  { printf '   %s\n' "$*"; }
ok()    { printf '   [ OK ]  %s\n' "$*"; }
fail()  { printf '   [FAIL]  %s\n' "$*"; STEP_FAILS=$((STEP_FAILS+1)); }
have()  { command -v "$1" >/dev/null 2>&1; }

# Run a build step, capturing output; print a trimmed error tail on failure.
run_step() {
  # run_step "<label>" <command...>
  local label="$1"; shift
  local out rc
  out="$("$@" 2>&1)"; rc=$?
  if [ $rc -eq 0 ]; then
    ok "$label"
  else
    fail "$label (exit $rc)"
    printf '%s\n' "$out" | tail -n 15 | sed 's/^/        | /'
  fi
  return $rc
}

# ---- 0. toolchain discovery ------------------------------------------------
log "toolchain"
MISSING_CORE=0
for t in git cc make; do
  if have "$t"; then info "found $t: $(command -v "$t")"; else
    fail "missing required tool: $t"; MISSING_CORE=1; fi
done
HAVE_JAVAC=0
if have javac; then HAVE_JAVAC=1; info "found javac: $(javac -version 2>&1)"; else
  info "javac not found - Java layers will be skipped"; fi

if [ $MISSING_CORE -ne 0 ]; then
  echo
  echo "Cannot continue without git, a C compiler, and make. Install them and retry."
  exit $STEP_FAILS
fi

# ---- 1. download (clone) ---------------------------------------------------
if [ $DO_CLONE -eq 1 ]; then
  log "download: $REPO_URL ($REF) -> $DEST"
  if [ -e "$DEST/.git" ]; then
    info "existing checkout found; fetching + checking out $REF"
    run_step "git fetch"    git -C "$DEST" fetch --depth 1 origin "$REF"
    run_step "git checkout" git -C "$DEST" checkout -q FETCH_HEAD
  else
    run_step "git clone" git clone --depth 1 --branch "$REF" "$REPO_URL" "$DEST" \
      || run_step "git clone (default branch)" git clone --depth 1 "$REPO_URL" "$DEST"
  fi
else
  log "using existing checkout in $DEST (--no-clone)"
fi

if [ ! -d "$DEST" ]; then
  echo; echo "Destination $DEST does not exist and clone did not run/succeed."
  exit $STEP_FAILS
fi
cd "$DEST" || { echo "cannot enter $DEST"; exit $STEP_FAILS; }
info "building in: $(pwd)"
if have git && [ -e .git ]; then info "at commit: $(git rev-parse --short HEAD 2>/dev/null)"; fi

# ---- 2. choose libusb strategy --------------------------------------------
log "USB transport (libusb) strategy"
VENDOR_FLAG=""
if [ $FORCE_VENDOR -eq 1 ]; then
  VENDOR_FLAG="LIBUSB_VENDOR=1"; info "forced: bundled libusb (--vendor-libusb)"
elif have pkg-config && pkg-config --exists libusb-1.0 2>/dev/null; then
  info "system libusb-1.0 present ($(pkg-config --modversion libusb-1.0))"
elif [ -f /usr/include/libusb-1.0/libusb.h ]; then
  info "system libusb-1.0 header present"
elif [ -f include/libusb-1.0.30.zip ]; then
  VENDOR_FLAG="LIBUSB_VENDOR=1"; info "no system libusb; using bundled libusb-1.0.30.zip"
else
  info "no system libusb and no bundled archive; native USB builds may fail"
fi

# ---- 3. build the C CLI ----------------------------------------------------
log "build: C EPROM CLI"
if [ -d source/linux ]; then
  # shellcheck disable=SC2086
  run_step "C CLI (all model years)" make -C source/linux $VENDOR_FLAG
  if ls build/linux/*/corrado-eprom >/dev/null 2>&1; then
    info "binaries: $(ls build/linux/*/corrado-eprom | wc -l) year(s)"
  fi
else
  info "source/linux not present on this platform checkout; skipping C CLI"
fi

# ---- 4. build the Java core port ------------------------------------------
if [ $HAVE_JAVAC -eq 1 ] && [ -d java/src ]; then
  log "build: Java core port (com.corrado.eprom)"
  rm -rf java/out 2>/dev/null; mkdir -p java/out
  # shellcheck disable=SC2046
  run_step "javac java core" javac -d java/out $(find java/src -name '*.java')
fi

# ---- 5. build the sleela connector/control series (incl. FFM) --------------
if [ $HAVE_JAVAC -eq 1 ] && [ -d sleela/java ]; then
  log "build: sleela connector/control series"
  rm -rf sleela/java/out 2>/dev/null; mkdir -p sleela/java/out
  # shellcheck disable=SC2046
  run_step "javac sleela series" \
    javac -d sleela/java/out $(find sleela/java/com -name '*.java')
fi

# ---- 6. build the FFM shim shared library ---------------------------------
if [ -d ffm-shim ]; then
  log "build: FFM shim (direct in-JVM USB path)"
  # shellcheck disable=SC2086
  run_step "FFM shim shared library" make -C ffm-shim $VENDOR_FLAG
  if ls ffm-shim/libcorrado_ffm.* >/dev/null 2>&1; then
    info "shim: $(ls ffm-shim/libcorrado_ffm.* 2>/dev/null)"
  fi
fi

# ---- 7. smoke check: can we run the program? -------------------------------
log "smoke check"
CLI="$(ls build/linux/*/corrado-eprom 2>/dev/null | head -1)"
if [ -n "$CLI" ] && [ -x "$CLI" ]; then
  if "$CLI" info >/dev/null 2>&1; then
    ok "CLI runs: $CLI info"
  else
    info "CLI built but 'info' returned nonzero (often fine without hardware)"
  fi
else
  info "no runnable CLI produced on this platform"
fi

# ---- summary ---------------------------------------------------------------
echo
log "summary"
if [ $STEP_FAILS -eq 0 ]; then
  echo "   All build steps succeeded. The Corrado runtime is ready in: $(pwd)"
  echo "   Try:  $CLI info      (and see README.md for read/backup/write/verify)"
else
  echo "   $STEP_FAILS build step(s) FAILED - see the [FAIL] blocks above."
  echo "   Common fixes: install a C toolchain + libusb-1.0, or re-run with"
  echo "   --vendor-libusb to use the bundled libusb; install a JDK for the"
  echo "   Java layers."
fi
exit $STEP_FAILS
