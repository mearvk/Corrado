#!/usr/bin/env bash
#
# fetch-images.sh
#
# Download the 8 CC-licensed Corrado photos (4 G60, 4 VR6) listed in
# images.manifest from Wikimedia Commons into images/g60/ and images/vr6/.
#
# The image files themselves are NOT committed to this repository (they are
# third-party CC-licensed works). This script fetches them to YOUR machine,
# where normal internet access is available. Each image's licence and
# Commons source page are recorded in images.manifest and README.md.
#
# Requires: curl. Wikimedia requires a descriptive User-Agent.
#
# SPDX-License-Identifier: MIT   (this script only; not the downloaded images)

set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
MANIFEST="$HERE/images.manifest"
UA="CorradoRepo-image-fetcher/1.0 (https://github.com/mearvk/Corrado)"

[ -f "$MANIFEST" ] || { echo "manifest not found: $MANIFEST" >&2; exit 1; }

ok=0; fail=0
while IFS=$'\t' read -r rel url lic page; do
  # skip comments / blank lines
  case "$rel" in ''|'#'*) continue ;; esac
  out="$HERE/$rel"
  mkdir -p "$(dirname "$out")"
  echo "-> $rel"
  echo "   $lic  ($page)"
  if curl -fsSL -A "$UA" "$url" -o "$out"; then
    sz=$(wc -c < "$out")
    echo "   saved $sz bytes"
    ok=$((ok+1))
  else
    echo "   FAILED: $url" >&2
    rm -f "$out"
    fail=$((fail+1))
  fi
done < "$MANIFEST"

echo
echo "Done: $ok downloaded, $fail failed."
echo "Attribution for each image is in images/README.md and images.manifest."
