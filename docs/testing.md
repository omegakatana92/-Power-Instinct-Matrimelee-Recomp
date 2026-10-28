# Testing Matrimelee

All commands below were run on 2026-10-27. Windows tests used the Release
MSVC build; Linux tests used Ubuntu 22.04 under WSL2 (gcc 11.4, CMake 4.4,
SDL2 2.0.20) and Xvfb 1280Ã—800.

## 1. ROM validation

Input files were checked against MAME's `matrim` definition. All sizes and
CRC32 values match, including the 64 MB of C data and the encrypted M1/V
files. The prepared outputs were compared with the independent Darksoft
reference hashes (P, M1, S, V); see `docs/rom-layout.md`.

## 2. Interpreter bring-up

```bash
Matrimelee --headless --interp --rom-path build-windows/roms \
    --frames 5600 --input tests/probe.txt \
    --screenshot 4700:i4700.png --screenshot 5100:i5100.png
```

The BIOS, title, HOW TO PLAY, player select and a match rendered. Native
frames at the same points were byte-identical:

```
p4700 vs i4700: IDENTICAL (SHA-256)
p5100 vs i5100: IDENTICAL (SHA-256)
```

## 3. Windows build

```powershell
scripts\build-windows.ps1 -RomDir .\roms-src -Test
```

Result: `Matrimelee.exe` (x64 Release), SDL2 and VC runtime DLLs staged.
The generator reported:

```
[m68krecomp] matrim: 322979 instructions emitted across routines
[m68krecomp] 7632 routines, 13250 dispatch entries
```

A windowed smoke test (`--scale 1 --frames 600`) exited 0 and printed no
`warning: no sound`.

## 4. CPU verification

```powershell
Matrimelee.exe --headless --verify --rom-path build-windows\roms `
    --frames 5600 --input tests\probe.txt
```

```
[verify] 3114807 blocks checked, 0 mismatches
[exec] native blocks 3114807, native instructions 108960474,
       interpreted instructions 1264732 (98.85% native), 66 uncovered
```

That run was before the profile pass. After one profile pass the long soak
reaches 100.00%.

## 5. Coverage / profile

```powershell
cmake --build build-windows --config Release --target profile
cmake --build build-windows --config Release --target matrimelee
Matrimelee.exe --headless --rom-path build-windows\roms --frames 19000 `
    --input tests\play_long.txt
```

```
[exec] native blocks 12234690, native instructions 365069515,
       interpreted instructions 114 (100.00% native), 0 uncovered entry points
```

The 114 interpreted instructions are a short runtime-only stub; the entry
list is merged into `build-windows/entries.txt` and is **not** committed.

## 6. Audio

```bash
Matrimelee --headless --rom-path build-windows/roms --frames 19000 \
    --input tests/play_long.txt --wav audio-play.wav
python tests/check_audio.py audio-play.wav --ignore 0-70 --ignore 235-295
```

```
duration 321.0 s, peak 30925
silent runs: 10, 41, 50-51, 55-62 (boot/title),
             244-281 (GAME OVER -> attract -> HOW TO PLAY), 285
PASS: audio is continuous after boot
```

The same silent windows appear in `--interp`, so they are content, not a
recompilation or Z80 stall. The Z80 trace shows the driver running and
answering commands throughout (banks `0F000/0E000/0C000/08000`, reply values
changing).

Real-time output path: `SDL_OpenAudioDevice` is opened with frequency,
format and channel changes allowed; `SDL_AudioStream` converts 55,555 Hz to
the device rate; a 60 ms cushion starts playback; underruns are recovered by
silence up to 90 ms; a >250 ms stall clears and re-cushions.

## 7. AppImage

```bash
chmod +x dist/steamdeck/Matrimelee-SteamDeck-x86_64.AppImage
APPIMAGE_EXTRACT_AND_RUN=1 ./dist/steamdeck/Matrimelee-SteamDeck-x86_64.AppImage \
    --headless --rom-path build-linux/roms --frames 3000 --verify
```

```
[verify] 1418885 blocks checked, 0 mismatches
[exec] native blocks 1418885, native instructions 60566044,
       interpreted instructions 38 (100.00% native), 0 uncovered
```

19,000-frame soak through the AppImage:

```
[exec] native blocks 12234627, native instructions 365069513,
       interpreted instructions 114 (100.00% native), 0 uncovered
```

First-run preparation was tested with the encrypted `roms-src` and a clean
`HOME`; it wrote the prepared set to `~/.local/share/matrim/roms` and reached
100.00% native.

## 8. Fullscreen / Steam Deck presentation

```bash
APPIMAGE_EXTRACT_AND_RUN=1 NG_WINDOW_TRACE=1 \
xvfb-run -a -s '-screen 0 1280x800x24' \
    ./dist/steamdeck/Matrimelee-SteamDeck-x86_64.AppImage \
    --fullscreen --rom-path build-linux/roms --frames 120
```

```
[window] output 1280x800 dst 107,0 1066x800 fullscreen=1
[exec] native blocks 31894, ... (100.00% native), 0 uncovered
```

The 320Ã—224 image is scaled to 1066Ã—800 and pillarboxed (107 px bars each
side); it is not stretched to 16:10.

`ldd build-linux/Matrimelee` shows `libSDL2-2.0.so.0` (and only standard
system libraries); the AppImage bundles that SDL2 copy and sets
`LD_LIBRARY_PATH` in `AppRun`.

## 9. Test assets

- `tests/coin_start.txt` â€” coin, start, HOW TO PLAY.
- `tests/probe.txt` â€” select a character and start a match.
- `tests/play_long.txt` â€” 19,000-frame soak with two matches and movement.
- `tests/check_audio.py` â€” continuity regression with explicit content
  windows.
- `tests/test-appimage-autoprep.sh` â€” AppImage first-run decryption test.

## 10. Not run

Physical Steam Deck, Gaming Mode, Steam Input, physical gamepads, subjective
listening, service menu, memory card and link-up.

