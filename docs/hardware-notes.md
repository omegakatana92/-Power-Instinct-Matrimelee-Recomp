# Hardware notes — Matrimelee

Matrimelee is a late Neo Geo cart with three protection blocks. The generic
runtime needed no title-specific hardware patches; the cartridge-specific work
is confined to `tools/decrypt.c` and `src/game.c`.

## Program

```
$000000-$0FFFFF   fixed bank       266-p1.p1
$200000-$2FFFFF   banked window    266-p2.sp2 (4 MB, 1 MB banks)
$C00000-$C1FFFF   MVS system ROM
```

The retail P ROMs are word-swapped and the 4 MB banked image is permuted by
the **K2K2** mapper (the same family as KOF 2002). `tools/decrypt.c` undoes
the 512 KB block permutation; the result is a standard 1 MB banked image, so
`game.c` leaves `bank_map = NULL`. A wrong permutation would still boot the
fixed bank but jump into garbage as soon as the BIOS or game selected another
bank.

## Graphics / fixed layer

The C ROMs are **NEO-CMC 7050** encrypted, key `0x6a`. There is no separate S
ROM on the encrypted board: the 512 KB fixed-layer data is appended to the C
data and is extracted after the CMC50 address permutation. `game.c` therefore
sets `fix_bank_type = NG_FIX_BANK_KOF2000`, because the 512 KB fixed layer
takes two extra tile-code bits from VRAM. With the wrong selector the title
and HUD glyphs use the wrong tiles; with the right one they match the
interpreter frame for frame.

## Sound

The M1 is **CMC50-encrypted**; the key is the 16-bit sum of the first 64 KB of
the encrypted region. The decrypted region is 512 KB, matching MAME's audio
region layout. The two 8 MB V ROMs are **NEO-PCM2 mode 1** scrambled.

The runtime models the four Z80 bank windows at `$8000/$C000/$E000/$F000`,
the NMI/reply latches and the YM2610's FM, SSG and ADPCM-A/B. The generic M1
mask fix (`(size - 1) & 0x3FFFF`, no second `0x10000` base) and the
frame-boundary `audio_sync()` are present. `ym2610.cpp` uses MAME's stereo
mix scaling.

## Timing

The player paces to the MVS refresh rate, 59.1856 Hz. The 19,000-frame soak
finishes with 0 uncovered entry points and 114 interpreted instructions after
one profile pass.

## Cartridge identification

MAME's `ROM_START( matrim )` comments it as **Encrypted Set**, MVS and AES
version, and the machine config selects `cartslot_fixed(config,
"k2k2_matrim")`. The cartridge has no `s1` ROM. The supplied archive matches
MAME's CRCs exactly; see `docs/rom-layout.md`.

## Untested hardware

* Physical MVS/AES board, memory card and link-up.
* AES BIOS variants and Korean MVS sets.
* PAL timing (the toolkit models NTSC/MVS only).
* Physical Steam Deck and SteamOS audio hardware.
