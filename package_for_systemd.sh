#!/bin/bash
set -euo pipefail

trap cleanup EXIT
cleanup() {
    rm -rf "$tmpdir"
}

tmpdir=$(mktemp -d --tmpdir package_for_systemd.XXXXXX)
mkdir "$tmpdir/brubeck"

for f in brubeck install_for_systemd.sh brubeck.service; do
    cp "$f" "$tmpdir/brubeck"
done

tar -C "$tmpdir" -czf brubeck.tgz brubeck
