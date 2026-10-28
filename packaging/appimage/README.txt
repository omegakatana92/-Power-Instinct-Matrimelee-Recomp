Matrimelee - Steam Deck / Linux x86_64
======================================

This AppImage is a native static recompilation of Matrimelee (Neo Geo MVS,
MAME set "matrim"). The 68000 program in your own legally obtained ROM dump
is translated to C at build time and runs natively; the Z80 sound driver and
YM2610 are emulated. No emulator frontend and no ROM data are included.

Run
---
  chmod +x Matrimelee-SteamDeck-x86_64.AppImage
  ./Matrimelee-SteamDeck-x86_64.AppImage

The AppImage looks for the unzipped ROM files in these places, in order:

  1. --rom-path DIR on the command line
  2. $MATRIMLEE_ROM_DIR
  3. roms/ or roms/matrim/ next to the AppImage
  4. ~/.local/share/matrim/roms
  5. ~/Emulation/roms/matrim, ~/roms/matrim, ~/roms

The retail Matrimelee set is encrypted. Put the unzipped original files in
one of those folders and the AppImage will decrypt once into
~/.local/share/matrim/roms on first launch. You can also do it explicitly:

  ./Matrimelee-SteamDeck-x86_64.AppImage --prepare-roms \
      --rom-path /path/to/unzipped/matrim+neogeo --out ~/.local/share/matrim/roms

Required original files (from your own matrim.zip and neogeo.zip):

  266-p1.p1   266-p2.sp2   266-c1.c1 .. 266-c8.c8
  266-m1.m1   266-v1.v1    266-v2.v2
  sp-s2.sp1   sfix.sfix    sm1.sm1      000-lo.lo

The prepared runtime set adds 266-s1.s1 (the fixed layer extracted from the
C ROMs) and decrypted M1/V/C data. The AppImage itself contains no game or
BIOS data.

Controls
--------
  Keyboard: arrows, Z/X/C/V = A/B/C/D, 5 = coin, 1 = start,
            3 = select, 9 = service, F2 = test, F11 = fullscreen, Esc = quit.
  Gamepad:  D-pad or left stick; A/B/X/Y = Neo Geo A/B/C/D;
            Start = start; Back = coin; left shoulder = select.

On a Steam Deck, add the AppImage to Steam as a non-Steam game. Steam Input's
default gamepad template maps the face buttons and Start; map Back to the
"Coin" action if you want an insert-coin button on the pad. The AppImage
starts fullscreen and keeps the Neo Geo 4:3 picture pillarboxed on the
1280x800 panel (no stretching). F11 toggles windowed mode.

Troubleshooting
---------------
  --scale N        window scale in windowed mode
  --headless       no window (for logs/tests)
  --wav FILE       write the audio stream to a WAV file
  --verify         replay every native block in Musashi and compare
  --interp         run everything in the interpreter
  --bios FILE      select a different MVS system ROM (default sp-s2.sp1)

Licence
-------
The original project code is PolyForm Noncommercial 1.0.0; NeoGeoRecomp,
Musashi, z80, ymfm and SDL2 keep their own licences. See LICENSES/ and
THIRD_PARTY_NOTICES.txt. No SNK ROM or BIOS data is distributed.
