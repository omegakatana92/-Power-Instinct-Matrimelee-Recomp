# Matrimelee compatibility report

Scope of the measurements in this release. A successful build is not the same
as a tested game; this file separates the two.

## Verified working

| Area | Evidence |
|---|---|
| MVS BIOS boot | Eyecatcher, memory test and Playmore logo at frames 600â€“1200 |
| Cartridge boot | Title screen ~frame 2,400, HOW TO PLAY and player select after coin/start |
| Game entry | Coin + Start reaches character select; a full match starts, including HUD, health bars, timer, combo counter and round intros |
| Sprites | Character, crowd and stage sprites render with correct palettes and animation |
| Fixed layer | Title, menus, INSERT COIN, character names and HOW TO PLAY text |
| Raster / palette | Stage background, palette fades and per-scanline effects visible in captures |
| 68000 static recompilation | 322,979 instructions across 7,632 routines; 13,250 dispatch entries |
| Native coverage | 100.00% of a 19,000-frame scripted session (365,069,515 native instructions, 114 interpreted, 0 uncovered) |
| `--verify` | 0 mismatches on 3,114,807 blocks (Windows probe) and 1,418,883 blocks (AppImage) |
| Native vs interpreter | Byte-identical PNGs at frames 4,700 and 5,100 |
| Z80 sound CPU | Real M1 driver runs, banks switch, NMI/reply latches respond |
| YM2610 | FM, SSG and ADPCM-A produce a stereo stream; 321 s capture, peak 30,231 |
| Audio continuity | No driver stall in gameplay; boot and GAME OVER â†’ attract â†’ HOW TO PLAY gaps are content and match `--interp` |
| Windows | x64 Release `.exe` launches in a window and headless; audio device opens without error |
| Linux / AppImage | AppImage starts, `--verify` passes, 19,000-frame headless soak 100% native |
| Fullscreen | 1280Ã—800 output with 4:3 pillarbox: destination `107,0 1066Ã—800` under Xvfb |
| First-run ROM prep | AppImage detects the encrypted set, decrypts once to `~/.local/share/matrim/roms`, then runs |
| Keyboard | Arrows, Z/X/C/V, 5, 1, 3, 9, F2, F11, Esc are wired |
| SDL gamepads | Standard GameController mapping, hot-plug, two players (code path only) |

## Not verified

| Area | Status |
|---|---|
| Physical Steam Deck (LCD/OLED) | Not available. No claim beyond WSL2/Xvfb + `ldd` + waveform tests. |
| SteamOS audio device | Not available; the SDL stream path is designed to accept any device rate, but no physical Deck output was heard. |
| Steam Input | Not tested; the bundled mapping is documented. |
| Physical Xbox/gamepad | Not tested. |
| Subjective audio quality | Not auditioned; measurements are waveform/RMS/peak and timeline. |
| Every character/stage/ending | Not played. The soak covers two matches and the standard attract/select flow. |
| Two-player match | Inputs are implemented but not exercised in a full session. |
| Service menu / DIP switches | Not exercised. |
| Memory card / link-up | Not exercised; the memory card is not present in the board model. |
| PAL / AES variants | Not tested; the runtime is the MVS NTSC model. |
| Physical Windows audio playback | Device opens and the WAV path is verified; no listening test. |

## Audio notes

The earlier family of "music stops while effects continue" bugs came from the
Z80 M1 bank window and a pull-based sample pump. This build contains both
generic corrections (mask from the M1 file size, `audio_sync()` at the frame
boundary) and a real-time output path that:

- opens the device at whatever rate/format/channel count it supports and
  converts the YM2610's 55,555 Hz through `SDL_AudioStream`;
- starts with a 60 ms cushion;
- recovers an underrun by inserting silence only up to a 90 ms target, rather
  than padding every frame;
- discards queued audio only after a genuine >250 ms stall, then re-cushions.

The YM2610 mix uses MAME's Neo Geo stereo route (FM 0.98, SSG 0.84) with a
final clamp, avoiding the full-scale clipping seen in earlier ports.

The 38-second quiet window (244â€“281 s) in `tests/play_long.txt` is the
scripted **GAME OVER â†’ attract â†’ HOW TO PLAY** sequence. The interpreter run
has the same window, so it is not a native or driver stall. The audio check
allows that window and the boot period explicitly.

## Hardware coverage

| Item | State |
|---|---|
| 68000 CPU / interrupts / flags | Recompiled, `--verify` clean |
| MVS BIOS vectors and boot | Real system ROM, no HLE |
| P-ROM banking | Standard 1 MB window after K2K2 descramble |
| 512 KB fixed layer (embedded S) | `NG_FIX_BANK_KOF2000` VRAM bank bits |
| Sprite shrink / 96-per-line | Toolkit model; exercised by the stage and UI |
| Palette banks | Exercised by fades and title |
| Raster timer IRQ | Exercised during gameplay |
| Z80 / YM2610 / ADPCM-A | Real driver + ymfm synthesis |
| uPD4990A RTC | BIOS time path works; game does not depend on a specific date |
| Watchdog | Generic model present; no reset observed during the soaks |
| SDL2 input/audio | Keyboard + gamepad mapping; audio stream output |

