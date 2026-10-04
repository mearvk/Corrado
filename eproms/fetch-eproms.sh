#!/usr/bin/env bash
#
# fetch-eproms.sh
#
# Download REAL community Corrado G60 (Digifant) EPROM dumps for actual
# tuning work. These are fetched from the public GitHub repository
# YOU54F/PoloG40Digifant into ./real/ next to this script.
#
# LICENSING: the upstream repository publishes NO licence, so these files
# are "all rights reserved" by their authors. This script downloads them to
# YOUR machine on demand; it does NOT redistribute them in this repo. Use
# them at your own risk and respect the upstream authors' rights. No freely
# licensed stock VR6 Corrado Motronic dump is publicly available, so none is
# fetched here.
#
# Requires: curl (and optionally the 'gh' CLI). No authentication needed for
# public raw files.
#
# SPDX-License-Identifier: MIT   (this script only; not the downloaded data)

set -euo pipefail

REPO="YOU54F/PoloG40Digifant"
BRANCH="master"
DEST="$(cd "$(dirname "$0")" && pwd)/real"

# Known 32,768-byte (27C256) stock G60 Digifant dumps in the upstream repo.
FILES=(
  "G60_and_other_eeproms/Stock G60 Single Ignition Map.BIN"
  "G60_and_other_eeproms/Stock G60 Three ignition maps.bin"
  "G60_IDAProFiles/vw6636.bin"
)

mkdir -p "$DEST"
echo "Downloading real community G60 dumps into: $DEST"
echo "(Upstream: https://github.com/$REPO - no licence; use at your own risk)"
echo

fetch_one() {
  # $1 = repo path, $2 = output file. Prefer the gh CLI (works behind
  # corporate/sandbox gateways); fall back to curl against raw.github.
  local path="$1" out="$2"
  if command -v gh >/dev/null 2>&1; then
    if gh api "repos/$REPO/contents/$path?ref=$BRANCH" --jq '.content' 2>/dev/null \
         | base64 -d > "$out" 2>/dev/null && [ -s "$out" ]; then
      return 0
    fi
  fi
  local enc="${path// /%20}"
  curl -fsSL "https://raw.githubusercontent.com/$REPO/$BRANCH/$enc" -o "$out"
}

for f in "${FILES[@]}"; do
  out="$DEST/$(basename "$f")"
  echo "  -> $(basename "$f")"
  if ! fetch_one "$f" "$out"; then
    echo "     FAILED to download $f" >&2
    rm -f "$out"
    continue
  fi
  sz=$(wc -c < "$out")
  echo "     saved $sz bytes"
  if [ "$sz" -ne 32768 ]; then
    echo "     WARNING: expected 32768 bytes (27C256)" >&2
  fi
done

echo
echo "Done. Verify a dump's checksum with:"
echo "  corrado-eprom checksum \"$DEST/Stock G60 Single Ignition Map.BIN\""
