/*
 * game.c - Matrimelee / Shin Gouketsuji Ichizoku Toukon (MAME set
 * "matrim", NGM-2660), the verified ROM layout shared by the player and the
 * recompiler.
 *
 * Layout and hashes were checked against MAME's src/mame/snk/neogeo.cpp,
 * ROM_START( matrim ):
 *
 *   266-p1.p1   1 MB  TC538200  fixed program bank, 16-bit word-swapped
 *   266-p2.sp2  4 MB  TC5332205 banked program image, word-swapped
 *   266-c1..c8  8 MB each, four odd/even bitplane pairs (64 MB paired)
 *   266-m1.m1 128 KB  CMC50-encrypted Z80 sound program
 *   266-v1.v1   8 MB  NEO-PCM2-scrambled ADPCM-A
 *   266-v2.v2   8 MB  NEO-PCM2-scrambled ADPCM-A
 *
 * The retail cart is protected by NEO-PCM2, NEO-CMC 7050 and the KOF2002
 * (K2K2) program scrambler.  It has no S ROM: the 512 KB fixed layer is
 * embedded at the end of the C ROMs.  tools/decrypt.c turns the user's
 * original files into the runtime-ready names below at build time, in the
 * build directory; the archive is never modified.
 *
 * The program uses the standard 1 MB banking window after the K2K2 block
 * permutation, and the fixed layer uses the KOF2000-style 512 KB bank map
 * (MAME's neogeo_matrim_cart_device::get_fixed_bank_type() returns 2).
 */
#include "game.h"

const ng_game_t k_matrimelee = {
    .name = "matrim",
    .title = "Matrimelee / Shin Gouketsuji Ichizoku Toukon",
    .prom_size = 0x500000,          /* 1 MB fixed + 4 MB banked */
    .prom = {
        /* file          file offset  dest       length    swap */
        {"266-p1.p1",   0x000000,    0x000000,  0x100000, 1},
        {"266-p2.sp2",  0x000000,    0x100000,  0x400000, 1},
    },
    .crom_size = 0x4000000,         /* 64 MB after pairing */
    .crom_pairs = {
        {"266-c1.c1", "266-c2.c2"},
        {"266-c3.c3", "266-c4.c4"},
        {"266-c5.c5", "266-c6.c6"},
        {"266-c7.c7", "266-c8.c8"},
    },
    .srom = "266-s1.s1",            /* 512 KB, extracted from the C data */
    .m1 = "266-m1.m1",              /* CMC50-decrypted Z80 program */
    .vrom = {"266-v1.v1", "266-v2.v2"},
    .fix_bank_type = NG_FIX_BANK_KOF2000,  /* 512 KB S ROM with VRAM bank bits */
    .bank_map = NULL,                      /* standard 1 MB P-ROM banking */
};
