# Matrimelee ROM layout and validation

MAME set: **`matrim`** — *Matrimelee / Shin Gouketsuji Ichizoku Toukon*
(NGM-2660, 2003, Noise Factory / Atlus).
The retail cartridge is an encrypted MVS/AES release using **NEO-PCM2**,
**NEO-CMC 7050** and the **K2K2** program block mapper. It has **no S ROM**:
the 512 KB fixed layer is embedded at the end of the C data.

## Original files (from your own `matrim.zip`)

Sizes and CRC32 are MAME's. All four files in the supplied archive match
these values exactly.

| File | Size | CRC32 | Role |
|---|---:|---|---|
| `266-p1.p1` | 1,048,576 | `5d4c2dc7` | 68000 fixed bank (word-swapped) |
| `266-p2.sp2` | 4,194,304 | `a14b1906` | 68000 banked image (word-swapped, K2K2-permuted) |
| `266-c1.c1` | 8,388,608 | `505f4e30` | sprite plane 0/1, pair 1 |
| `266-c2.c2` | 8,388,608 | `3cb57482` | sprite plane 2/3, pair 1 |
| `266-c3.c3` | 8,388,608 | `f1cc6ad0` | sprite plane 0/1, pair 2 |
| `266-c4.c4` | 8,388,608 | `45b806b7` | sprite plane 2/3, pair 2 |
| `266-c5.c5` | 8,388,608 | `9a15dd6b` | sprite plane 0/1, pair 3 |
| `266-c6.c6` | 8,388,608 | `281cb939` | sprite plane 2/3, pair 3 |
| `266-c7.c7` | 8,388,608 | `4b71f780` | sprite plane 0/1, pair 4 |
| `266-c8.c8` | 8,388,608 | `29873d33` | sprite plane 2/3, pair 4 |
| `266-m1.m1` | 131,072 | `456c3e6c` | CMC50-encrypted Z80 program |
| `266-v1.v1` | 8,388,608 | `a4f83690` | NEO-PCM2-scrambled ADPCM-A |
| `266-v2.v2` | 8,388,608 | `d0f69eda` | NEO-PCM2-scrambled ADPCM-A |

BIOS files (from `neogeo.zip`): `sp-s2.sp1`, `sfix.sfix`, `sm1.sm1`,
`000-lo.lo`.

## Prepared runtime files

`tools/decrypt.c` writes these into the build directory. The original archive
is never modified.

| File | Size | CRC32 of the prepared output | Transformation |
|---|---:|---|---|
| `266-p1.p1` | 1,048,576 | `5d4c2dc7` | copied |
| `266-p2.sp2` | 4,194,304 | `b0132cef` | K2K2 512 KB block permutation undone |
| `266-s1.s1` | 524,288 | `10b1ce57` | last 512 KB of decrypted C data, de-interleaved |
| `266-c1.c1` | 8,388,608 | `0ab54c54` | CMC50 graphics decrypt, key `0x6a` |
| `266-c2.c2` | 8,388,608 | `b834dd89` | CMC50 graphics decrypt |
| `266-c3.c3` | 8,388,608 | `f31d68d4` | CMC50 graphics decrypt |
| `266-c4.c4` | 8,388,608 | `58cfb6e9` | CMC50 graphics decrypt |
| `266-c5.c5` | 8,388,608 | `f3c0e3fd` | CMC50 graphics decrypt |
| `266-c6.c6` | 8,388,608 | `b53c8dcf` | CMC50 graphics decrypt |
| `266-c7.c7` | 8,388,608 | `3c1fb02a` | CMC50 graphics decrypt |
| `266-c8.c8` | 8,388,608 | `5ee31f80` | CMC50 graphics decrypt |
| `266-m1.m1` | 524,288 | `b348b06d` | CMC50 M1 decrypt (512 KB region image) |
| `266-v1.v1` | 8,388,608 | `328b2194` | NEO-PCM2 mode-1 V scramble undone |
| `266-v2.v2` | 8,388,608 | `2a13562d` | NEO-PCM2 mode-1 V scramble undone |

## Independent validation

The prepared outputs were checked against the Darksoft Neo Geo reference
database (`Darksoft Neo Geo SMDB.txt`), which is independent of this decoder:

| Prepared data | SHA-1 | Matches reference |
|---|---|---|
| concatenated P ROM (`p1` + `p2`) | `fc614694460db03453c3c616dac7a2a42e70b460` | yes (`prom`) |
| first 128 KB of M1 | `dd42f8a301f07e079fbc2bfd23e788baa03ca72d` | yes (`m1rom`) |
| S ROM | `de40a742508eb44995c1fb467aef5c3aed403497` | yes (`srom`) |
| concatenated V ROM (`v1` + `v2`) | `e445a42ab722ae44cabe0d7403b3380caccaa868` | yes (`vroma0`) |

Because the S ROM is extracted *after* the C graphics decryption, its exact
match also confirms the CMC50 key and address permutation. The C-plane hashes
above are recorded for regression; the sprite renderer was additionally
checked against native/interpreter screenshots.

## Memory map used by the runtime

```
$000000-$0FFFFF   fixed P bank           266-p1.p1
$100000-$1FFFFF   work RAM (mirrored)
$200000-$2FFFFF   banked P window        266-p2.sp2, standard 1 MB banks
$2FFFF0-$2FFFFF   bank select
$300000-$3FFFFF   I/O and LSPC
$400000-$7FFFFF   palette
$C00000-$C1FFFF   MVS system ROM
```

`game.c` sets `prom_size = 0x500000` (1 MB fixed + 4 MB banked),
`bank_map = NULL` (standard 1 MB banking after the K2K2 permutation), and
`fix_bank_type = NG_FIX_BANK_KOF2000` because the embedded S ROM is 512 KB and
the fixed layer takes its two extra code bits from VRAM, exactly like KOF
2000 / Metal Slug 3.

## References

- MAME `src/mame/snk/neogeo.cpp`, `ROM_START( matrim )` and
  `mvs_led_state::matrim`.
- MAME `src/devices/bus/neogeo/kof2k2.cpp` (`neogeo_matrim_cart_device`),
  `prot_kof2k2.cpp`, `prot_cmc.cpp` and `prot_pcm2.cpp` — algorithms and
  tables ported under BSD-3-Clause.
- NeoGeo Development Wiki: PROGBK2 / CHAFIO, NEO-PCM2, NEO-CMC 7050,
  K2K2 mapper.
