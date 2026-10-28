#!/usr/bin/env bash
# Build the native Linux x86_64 player and the Steam Deck AppImage.
# Run from WSL2 or a native Linux host:
#   ./scripts/build-linux.sh [ROM_DIR]
# ROM_DIR defaults to ./roms-src (unzipped matrim + neogeo files).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ROM="${1:-$ROOT/roms-src}"
exec "$ROOT/packaging/appimage/build-appimage.sh" "$ROM"
