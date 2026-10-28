#!/usr/bin/env bash
# List the AppImage payload to check the binary, decryptor, desktop file,
# icon and licence files are present.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
APP="$ROOT/dist/steamdeck/Matrimelee-SteamDeck-x86_64.AppImage"
rm -rf /tmp/matrim-extract
mkdir -p /tmp/matrim-extract
cd /tmp/matrim-extract
"$APP" --appimage-extract >/dev/null
echo "--- executable bits ---"
ls -l squashfs-root/AppRun squashfs-root/usr/bin/matrimelee squashfs-root/usr/bin/matrimelee_decrypt
echo "--- payload ---"
find squashfs-root -maxdepth 5 \( -type f -o -type l \) | sort
echo "--- ldd (AppDir binary) ---"
ldd squashfs-root/usr/bin/matrimelee | grep -E 'SDL|not found' || true
