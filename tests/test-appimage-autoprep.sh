#!/usr/bin/env bash
# Verify the AppImage first-run decryption path with the encrypted source set.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
rm -rf /tmp/matrimhome
mkdir -p /tmp/matrimhome
HOME=/tmp/matrimhome \
MATRIMLEE_ROM_DIR="$ROOT/roms-src" \
APPIMAGE_EXTRACT_AND_RUN=1 \
    ./dist/steamdeck/Matrimelee-SteamDeck-x86_64.AppImage --headless --frames 120
echo "--- prepared files ---"
ls -l /tmp/matrimhome/.local/share/matrim/roms | head -25
