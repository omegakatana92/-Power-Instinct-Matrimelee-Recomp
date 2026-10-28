Matrimelee - Windows x64
========================

This is a native static recompilation of Matrimelee (Neo Geo MVS, MAME set
"matrim"). The 68000 program in your own legally obtained dump is translated
to C at build time and runs natively; the Z80 sound driver and YM2610 are
emulated. No emulator frontend and no ROM data are included.

Run
---
  1. Put your unzipped ROM files in the roms\ folder (or use
     Prepare-ROMs.ps1 to extract matrim.zip and neogeo.zip and decrypt them).
  2. Start Matrimelee.exe.

Required files (the prepared roms\ folder after Prepare-ROMs.ps1):

  266-p1.p1   266-p2.sp2   266-c1.c1 .. 266-c8.c8
  266-s1.s1   266-m1.m1    266-v1.v1    266-v2.v2
  sp-s2.sp1   sfix.sfix    sm1.sm1      000-lo.lo

The retail files are encrypted. matrimelee_decrypt.exe converts the
original archive contents into the runtime-ready files above; it never
modifies your archive.

Controls
--------
  Keyboard: arrows, Z/X/C/V = A/B/C/D, 5 = coin, 1 = start,
            3 = select, 9 = service, F2 = test, F11 = fullscreen, Esc = quit.
  Gamepad:  D-pad or left stick; A/B/X/Y = Neo Geo A/B/C/D;
            Start = start; Back = coin; left shoulder = select.

Options
-------
  --rom-path DIR   game and BIOS files (default: roms)
  --bios-path DIR  BIOS files if separate
  --bios FILE      MVS system ROM (default sp-s2.sp1)
  --scale N        window scale (default 3)
  --fullscreen     borderless fullscreen, 4:3 pillarboxed
  --headless       no window
  --wav FILE       write the audio stream to a WAV file
  --verify         replay every native block in Musashi and compare
  --interp         run everything in the interpreter
  --frames N       stop after N frames

Licence
-------
The original project code is PolyForm Noncommercial 1.0.0; NeoGeoRecomp,
Musashi, z80, ymfm and SDL2 keep their own licences. See LICENSES\ and
THIRD_PARTY_NOTICES.md. No SNK ROM or BIOS data is distributed.
