# Matrimelee â€” native Neo Geo recompilation

A native **Windows x64** and **Linux x86_64 / Steam Deck** recompilation of
*Power Instinct Matrimelee* - also titled *Matrimelee / Shin Gouketsuji
Ichizoku Toukon* (Neo Geo MVS, 2003, NGM-2660, MAME set `matrim`).

The 68000 program from your own legally obtained dump is translated ahead of
time into C by [NeoGeoRecomp](https://github.com/sp00nznet/neogeorecomp) and
runs natively against a model of the MVS board. The Z80 sound driver and
YM2610 are emulated. There is no emulator frontend and no BIOS HLE: the real
system ROM boots the cartridge. No ROM or BIOS data is included.

| Deliverable | Path |
|---|---|
| Windows x64 | `dist/windows/Matrimelee-Windows-x64/Matrimelee.exe` |
| Steam Deck / Linux x86_64 | `dist/steamdeck/Matrimelee-SteamDeck-x86_64.AppImage` |
| Source | this directory |
| Licence | PolyForm Noncommercial 1.0.0 for original code (see below) |

Release artifacts (SHA-256 in `dist/SHA256SUMS.txt`):

```
8b1e6c0bfbd55e3d114d0e028464f936a7d7b69b811b92f211977bc9db285e1a  Matrimelee-Windows-x64.zip
54fff4f1614d5b1b66fd3c4f42a2db7c1ffc7c34bdc055d7c08b62660e884545  Matrimelee-SteamDeck-x86_64.AppImage
```

The Windows zip is 7.3 MB and the AppImage is 8.0 MB. Neither contains ROM,
BIOS or decrypted game data.

## Status

Measured on 2026-10-27 with toolkit v0.2.0, upstream commit
`af96b2b4b39b42b392b17bcaa6a74a5f27088202` (see `toolkit.ref`).

| Area | Result |
|---|---|
| Boot | BIOS eyecatcher, Playmore logo, title, HOW TO PLAY, player select and a match all render |
| Gameplay | Coin/start, character select, first match, HUD, sprites, palette, scrolling and hit effects exercised; not a full clear of every mode |
| Native coverage | **100.00%** of a 19,000-frame scripted session: 365,069,515 native instructions, 114 interpreted (one short runtime-only entry stub), **0 uncovered** |
| `--verify` | **0 mismatches**; 3,114,807 blocks on a 5,600-frame probe and 1,418,883 blocks on the 3,000-frame AppImage check |
| Native vs interpreter | Screenshots at frames 4,700 and 5,100 are byte-identical (SHA-256) |
| ROM validation | Input CRCs match MAME; decrypted P/M1/S/V outputs match the independent Darksoft reference hashes (see `docs/rom-layout.md`) |
| Audio | Z80 driver, YM2610 FM/SSG/ADPCM-A, stereo output and `--wav` capture work; 321 s capture has no driver stall. Boot and the scripted GAME OVER â†’ attract â†’ HOW TO PLAY gap (244â€“281 s) are game content and identical in `--interp` |
| Windows | Built with MSVC 2022/2026 toolset, x64 Release; launched in a window and headless |
| Steam Deck | AppImage built and run headless, `--verify`, and fullscreen 1280Ã—800 under WSL2/Xvfb; **not tested on physical Deck hardware** |
| Controllers | Keyboard and SDL2 GameController mapping implemented; **not tested on physical gamepads** |
| Audio listening | Waveform/continuity measured; **not subjectively auditioned** |

## Quick start

### Windows

1. Extract `Matrimelee-Windows-x64.zip` (or use the folder).
2. Run `Prepare-ROMs.ps1` and point it at your `matrim.zip` and
   `neogeo.zip`. It extracts the files and runs the bundled decryptor into
   `roms\`.
3. Run `Matrimelee.exe`.

The retail set is encrypted. `matrimelee_decrypt.exe` converts the original
archive contents into the runtime-ready files; it never modifies the archive.

### Steam Deck / Linux

```bash
chmod +x Matrimelee-SteamDeck-x86_64.AppImage
./Matrimelee-SteamDeck-x86_64.AppImage
```

Put the unzipped original `matrim` and `neogeo` files in `roms/` next to the
AppImage, or in `~/.local/share/matrim/roms`. On first launch the AppImage
decrypts them once into `~/.local/share/matrim/roms` and then starts the game.
You can also do it explicitly:

```bash
./Matrimelee-SteamDeck-x86_64.AppImage --prepare-roms \
    --rom-path /path/to/unzipped/matrim+neogeo \
    --out ~/.local/share/matrim/roms
```

Add the AppImage to Steam as a non-Steam game for Gaming Mode. It starts
fullscreen and pillarboxes the Neo Geo 4:3 picture on the 1280Ã—800 panel.

## ROM set

`docs/rom-layout.md` has the full table, CRCs and layout. The short version:

```
matrim.zip : 266-p1.p1  266-p2.sp2  266-c1.c1 .. 266-c8.c8
             266-m1.m1  266-v1.v1   266-v2.v2
neogeo.zip : sp-s2.sp1  sfix.sfix   sm1.sm1  000-lo.lo
```

The cartridge has no S ROM: the 512 KB fixed layer is embedded at the end of
the C data and is extracted by the decryptor as `266-s1.s1`.

## Controls

| Action | Keyboard | Gamepad |
|---|---|---|
| Move | Arrows | D-pad or left stick |
| A / B / C / D | Z / X / C / V | A / B / X / Y (Xbox layout positions) |
| Start | 1 | Start |
| Coin | 5 | Back |
| Select | 3 | Left shoulder |
| Service / Test | 9 / F2 | â€” |
| Fullscreen | F11 | â€” |
| Quit | Esc | Steam overlay |

On a Steam Deck, use Steam Input to map Back (or another button) to Coin if
you want a pad-only coin button. The bundled mapping is unchanged from the
toolkit.

## Build from source

The toolkit is vendored in `neogeorecomp/`; its MIT licence and notices are
preserved. The vendored copy is v0.2.0 plus the generic runtime changes in
`docs/runtime-changes.md`.

### Windows x64 (MSVC + CMake)

```powershell
cmake -S . -B build-windows -G "Visual Studio 18 2026" -A x64 `
  -DSDL2_DIR=deps\sdl2\SDL2-2.32.10\cmake `
  -DMATRIMLEE_ROM_DIR=roms-src
cmake --build build-windows --config Release --parallel 8
```

The build runs `matrimelee_decrypt`, then `matrimelee_recomp`, then compiles
the generated C. `scripts\build-windows.ps1` wraps the same steps and stages
`dist\windows\`.

### Linux / Steam Deck (gcc + CMake)

```bash
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release \
      -DMATRIMLEE_ROM_DIR="$PWD/roms-src"
cmake --build build-linux --parallel "$(nproc)"
./packaging/appimage/build-appimage.sh "$PWD/roms-src"
```

`scripts/build-linux.sh` does both steps. Prerequisites: `gcc`, `g++`,
`cmake`, `make`, `libsdl2-dev`, `wget`.

### Verification and coverage

```bash
# CPU comparison against Musashi
./Matrimelee --headless --verify --rom-path build-linux/roms \
    --frames 5600 --input tests/probe.txt

# Audio continuity (allows the boot and GAME OVER/attract windows)
./Matrimelee --headless --rom-path build-linux/roms --frames 19000 \
    --input tests/play_long.txt --wav audio.wav
python tests/check_audio.py audio.wav --ignore 0-70 --ignore 235-295

# Runtime-only entry points, then rebuild
cmake --build build-linux --target profile
```

## Project layout

```
CMakeLists.txt              build: decrypt -> recompile -> player
src/game.c, game.h          the verified matrim ROM layout
src/main.c                  ng_main() player
src/recomp_main.c           ng_recomp_main() generator
tools/decrypt.c             NEO-PCM2/K2K2 + CMC50 + PCM2 preparation
tools/tables.h              CMC/PCM tables ported from MAME (BSD-3)
neogeorecomp/               vendored MIT toolkit
tests/                      input scripts, audio regression, AppImage test
packaging/                  Windows and AppImage packaging
docs/                       ROM layout, compatibility, testing, runtime notes
```

## Licensing

- Original project code (game host, ROM layout, decryptor, packaging,
  documentation) is licensed under the **PolyForm Noncommercial License
  1.0.0** (`LICENSE`), confirmed by the project owner on 2026-10-27.
- `neogeorecomp/` keeps its upstream **MIT** licence and `NOTICE`.
- Musashi (MIT), superzazu/z80 (MIT), ymfm (BSD-3-Clause) and SDL2 (zlib)
  keep their own licences; see `THIRD_PARTY_NOTICES.md` and `LICENSES/`.
- No SNK ROM, BIOS, recompiled game code, entry list or gameplay screenshot is
  distributed. Users supply their own dumps.

PolyForm Noncommercial is not an open-source licence and does not permit
commercial redistribution. Matrimelee, Shin Gouketsuji Ichizoku, Neo Geo and
SNK are trademarks of their respective owners. This project is not affiliated
with or endorsed by SNK, Noise Factory or Atlus.

## Known limitations

- Physical Steam Deck, SteamOS audio output, Gaming Mode and Steam Input were
  not tested; validation is WSL2/Xvfb plus `ldd` and waveform analysis.
- Physical gamepads were not tested.
- The sound mix was measured, not auditioned by ear.
- Not every character, stage, ending, continue path or two-player mode was
  played through. The 19,000-frame soak covers boot, attract, select and two
  matches.
- The Windows package needs the Microsoft VC++ runtime DLLs shipped beside
  the executable (included).
- See `docs/compatibility.md` and `docs/testing.md` for the exact scope.

