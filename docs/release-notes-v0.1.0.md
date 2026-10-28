# Matrimelee v0.1.0 — native recompilation

First build of the Matrimelee static recompilation on NeoGeoRecomp v0.2.0.

## Downloads

* `Matrimelee-Windows-x64.zip` — Windows 10/11 x64, SDL2 and VC runtime
  included.
* `Matrimelee-SteamDeck-x86_64.AppImage` — Linux x86_64 / Steam Deck.
* `SHA256SUMS.txt` — hashes for both packages.

No game ROMs or BIOS files are included. Supply your own legally obtained
MAME `matrim` and `neogeo` sets. Windows: run `Prepare-ROMs.ps1`. Linux: put
the unzipped files in `roms/` next to the AppImage, or use
`--prepare-roms`.

## Verified

* Input CRCs match MAME `ROM_START( matrim )`.
* Prepared P, M1, S and V data match the independent Darksoft reference
  hashes.
* 322,979 recompiled instructions across 7,632 routines; 13,250 dispatch
  entries after one profile pass.
* 19,000-frame soak: 365,069,515 native instructions, 114 interpreted,
  0 uncovered (100.00% native).
* `--verify`: 3,114,807 blocks / 0 mismatches (5,600-frame probe);
  1,418,885 blocks / 0 mismatches (AppImage, 3,000 frames).
* Native and interpreter frames byte-identical at frames 4,700 and 5,100.
* Audio: 321 s capture, peak 30,231; no driver stall. The only long quiet
  window is the scripted GAME OVER → attract → HOW TO PLAY sequence, also
  present in `--interp`.
* Windows windowed and headless launches passed; audio device opened.
* AppImage headless, `--verify`, first-run decryption and fullscreen launches
  passed under Xvfb 1280×800; 4:3 pillarbox measured at 1066×800 (x=107).

## Notes

* Steam Deck hardware, Steam Input and physical gamepads were not available;
  those paths remain untested.
* Audio was measured, not auditioned.
* 100% coverage covers the scripted sessions. Code outside them can still run
  in Musashi.
* Original project code: PolyForm Noncommercial 1.0.0 (confirmed by the project owner,
  2026-10-27). Vendored toolkit and dependencies keep their MIT/BSD/zlib
  licences.

