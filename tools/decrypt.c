/*
 * decrypt.c - Matrimelee / Shin Gouketsuji Ichizoku Toukon (MAME set
 * "matrim", NGM-2660) ROM preparation.
 *
 * The retail MVS/AES cart is protected by three chips:
 *
 *   NEO-PCM2 (Playmore 2002) - P-ROM block swap, V-ROM address scramble
 *   NEO-CMC 7050             - C-ROM encryption and the embedded S-ROM
 *   CMC50 M1 path            - encrypted Z80 program
 *
 * This tool reads the user's original archive contents (never modified) and
 * writes a runtime-ready set into the build directory:
 *
 *   266-p1.p1   1 MB fixed 68k bank, copied
 *   266-p2.sp2  4 MB banked 68k image, K2K2 block order restored
 *   266-c1..c8  8 MB each, CMC50-decrypted sprite planes (key 0x6a)
 *   266-s1.s1   512 KB fixed layer, extracted from the end of the C data
 *   266-m1.m1   CMC50-decrypted Z80 program (512 KB region image)
 *   266-v1/v2   8 MB each, NEO-PCM2 (mode 1) address scramble restored
 *
 * Algorithms and constant tables are ported from MAME's Neo Geo cart
 * devices (BSD-3-Clause; S. Smith, D. Haywood, F. Priuli, and the NeoGeo
 * driver authors):
 *
 *   src/devices/bus/neogeo/prot_kof2k2.cpp  matrim_decrypt_68k()
 *   src/devices/bus/neogeo/kof2k2.cpp       neogeo_matrim_cart_device
 *   src/devices/bus/neogeo/prot_cmc.cpp     cmc50_gfx_decrypt(), sfix_decrypt(),
 *                                           cmc50_m1_decrypt()
 *   src/devices/bus/neogeo/prot_pcm2.cpp    PCM2 swap(ym, size, 1)
 *
 * No ROM data is contained in this file or committed to the repository.
 *
 * Usage: matrimelee_decrypt [--rom-path DIR] [--out DIR]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifdef _WIN32
#include <direct.h>
#define MATRIM_MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define MATRIM_MKDIR(p) mkdir((p), 0777)
#endif
#include "tables.h"

#define MATRIM_GFX_KEY 0x6a
#define PCM2_MODE      1        /* matrim/samsho5 family */

static uint32_t crc_table[256];
static int crc_ready;

static void crc32_init(void) {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int k = 0; k < 8; k++)
            c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        crc_table[i] = c;
    }
    crc_ready = 1;
}

static uint32_t crc32_buf(const uint8_t *p, size_t n) {
    uint32_t c = 0xFFFFFFFFu;
    if (!crc_ready) crc32_init();
    for (size_t i = 0; i < n; i++) c = crc_table[(c ^ p[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

static uint8_t *read_file(const char *dir, const char *name, size_t *size, int required) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "rb");
    if (!f) {
        if (required) fprintf(stderr, "decrypt: cannot open %s\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0) { fclose(f); return NULL; }
    uint8_t *buf = (uint8_t *)malloc((size_t)n);
    if (!buf || fread(buf, 1, (size_t)n, f) != (size_t)n) {
        fclose(f); free(buf); return NULL;
    }
    fclose(f);
    *size = (size_t)n;
    return buf;
}

static int write_file(const char *dir, const char *name, const uint8_t *data, size_t size) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "decrypt: cannot write %s\n", path); return -1; }
    if (fwrite(data, 1, size, f) != size) { fclose(f); return -1; }
    fclose(f);
    printf("  %-14s %9zu bytes  crc32 %08x\n", name, size, crc32_buf(data, size));
    return 0;
}

static int read_checked(const char *dir, const char *name, size_t expect_size,
                        uint32_t expect_crc, uint8_t **out, size_t *out_size) {
    size_t size = 0;
    uint8_t *d = read_file(dir, name, &size, 1);
    if (!d) return -1;
    if (size != expect_size) {
        fprintf(stderr, "decrypt: %s is %zu bytes, expected %zu\n", name, size, expect_size);
        free(d);
        return -1;
    }
    uint32_t crc = crc32_buf(d, size);
    if (crc != expect_crc) {
        fprintf(stderr, "decrypt: %s has crc32 %08x, expected %08x (wrong dump?)\n",
                name, crc, expect_crc);
        free(d);
        return -1;
    }
    *out = d;
    *out_size = size;
    return 0;
}

static int copy_file(const char *dir, const char *outdir, const char *name) {
    size_t size = 0;
    uint8_t *d = read_file(dir, name, &size, 1);
    if (!d) return -1;
    int rc = write_file(outdir, name, d, size);
    free(d);
    return rc;
}

/* Sanity checks on the prepared files.  Matrimelee has no published
 * "decrypted set" CRCs, so this checks structure rather than guessing:
 * input CRCs were validated above, the fixed bank must carry the NEO-GEO
 * signature, the extracted S data and M1 must not be empty, and sizes must
 * be exactly what the runtime layout expects. */
static int verify_outputs(const char *outdir) {
    int bad = 0;
    size_t size = 0;
    uint8_t *d;

    d = read_file(outdir, "266-p1.p1", &size, 0);
    if (!d || size != 0x100000) {
        fprintf(stderr, "decrypt: verification: 266-p1.p1 wrong size\n");
        bad = 1;
    } else {
        if (crc32_buf(d, size) != 0x5d4c2dc7u) {
            fprintf(stderr, "decrypt: verification: 266-p1.p1 CRC mismatch\n");
            bad = 1;
        }
        /* P ROMs are word-swapped on disk; the signature is ASCII in the
         * 68000's big-endian view, so swap each pair back before comparing.
         * On disk the first four bytes are "EN-O" for "NEO-". */
        if (!(d[0x101] == 'N' && d[0x100] == 'E' &&
              d[0x103] == 'O' && d[0x102] == '-')) {
            fprintf(stderr, "decrypt: verification: NEO-GEO signature missing at $100\n");
            bad = 1;
        }
    }
    free(d);

    d = read_file(outdir, "266-p2.sp2", &size, 0);
    if (!d || size != 0x400000) {
        fprintf(stderr, "decrypt: verification: 266-p2.sp2 wrong size\n");
        bad = 1;
    }
    free(d);

    d = read_file(outdir, "266-s1.s1", &size, 0);
    if (!d || size != 0x80000) {
        fprintf(stderr, "decrypt: verification: 266-s1.s1 wrong size\n");
        bad = 1;
    } else {
        size_t nz = 0;
        for (size_t i = 0; i < size; i++) if (d[i]) nz++;
        if (nz < size / 8) {
            fprintf(stderr, "decrypt: verification: 266-s1.s1 looks empty\n");
            bad = 1;
        }
    }
    free(d);

    d = read_file(outdir, "266-m1.m1", &size, 0);
    if (!d || size != 0x80000) {
        fprintf(stderr, "decrypt: verification: 266-m1.m1 wrong size\n");
        bad = 1;
    } else {
        size_t nz = 0;
        for (size_t i = 0; i < 0x20000; i++) if (d[i]) nz++;
        if (nz < 0x1000) {
            fprintf(stderr, "decrypt: verification: 266-m1.m1 looks empty\n");
            bad = 1;
        }
    }
    free(d);

    for (int i = 1; i <= 2; i++) {
        char name[16];
        snprintf(name, sizeof(name), "266-v%d.v%d", i, i);
        d = read_file(outdir, name, &size, 0);
        if (!d || size != 0x800000) {
            fprintf(stderr, "decrypt: verification: %s wrong size\n", name);
            bad = 1;
        }
        free(d);
    }

    if (!bad) printf("Output verification: structure and P1 CRC match.\n");
    return bad ? -1 : 0;
}

/* ---- NEO-PCM2 / K2K2 program block swap
 * (MAME kof2002_prot_device::matrim_decrypt_68k) ---- */

static void p2_block_swap(uint8_t *rom) {
    static const uint32_t sec[8] = {
        0x100000, 0x280000, 0x300000, 0x180000,
        0x000000, 0x380000, 0x200000, 0x080000,
    };
    uint8_t *tmp = (uint8_t *)malloc(0x400000);
    if (!tmp) return;
    memcpy(tmp, rom, 0x400000);
    for (int i = 0; i < 8; i++)
        memcpy(rom + (size_t)i * 0x80000, tmp + sec[i], 0x80000);
    free(tmp);
}

/* ---- NEO-CMC 7050 graphics decryption ---- */

static void gfx_byte(uint8_t *r0, uint8_t *r1, uint8_t c0, uint8_t c1,
                     const uint8_t *t0hi, const uint8_t *t0lo, const uint8_t *t1,
                     uint32_t base, int invert) {
    uint8_t tmp = t1[(base & 0xff) ^ kof2000_address_0_7_xor[(base >> 8) & 0xff]];
    uint8_t xor0 = (uint8_t)((t0hi[(base >> 8) & 0xff] & 0xfe) | (tmp & 0x01));
    uint8_t xor1 = (uint8_t)((tmp & 0xfe) | (t0lo[(base >> 8) & 0xff] & 0x01));
    if (invert) { *r0 = c1 ^ xor0; *r1 = c0 ^ xor1; }
    else        { *r0 = c0 ^ xor0; *r1 = c1 ^ xor1; }
}

static int gfx_decrypt(uint8_t *rom, uint32_t rom_size) {
    uint8_t *buf = (uint8_t *)malloc(rom_size);
    if (!buf) return -1;
    const uint8_t *a07  = kof2000_address_0_7_xor;
    const uint8_t *a811 = kof2000_address_8_15_xor1;
    const uint8_t *a812 = kof2000_address_8_15_xor2;
    const uint8_t *a161 = kof2000_address_16_23_xor1;
    const uint8_t *a162 = kof2000_address_16_23_xor2;

    for (uint32_t rpos = 0; rpos < rom_size / 4; rpos++) {
        gfx_byte(&buf[4 * rpos], &buf[4 * rpos + 3], rom[4 * rpos], rom[4 * rpos + 3],
                 kof2000_type0_t03, kof2000_type0_t12, kof2000_type1_t03, rpos,
                 (int)((rpos >> 8) & 1));
        gfx_byte(&buf[4 * rpos + 1], &buf[4 * rpos + 2], rom[4 * rpos + 1], rom[4 * rpos + 2],
                 kof2000_type0_t12, kof2000_type0_t03, kof2000_type1_t12, rpos,
                 (int)(((rpos >> 16) ^ a162[(rpos >> 8) & 0xff]) & 1));
    }
    for (uint32_t rpos = 0; rpos < rom_size / 4; rpos++) {
        uint32_t baser = rpos;
        baser ^= MATRIM_GFX_KEY;
        baser ^= (uint32_t)a811[(baser >> 16) & 0xff] << 8;
        baser ^= (uint32_t)a812[baser & 0xff] << 8;
        baser ^= (uint32_t)a161[baser & 0xff] << 16;
        baser ^= (uint32_t)a162[(baser >> 8) & 0xff] << 16;
        baser ^= a07[(baser >> 8) & 0xff];
        baser &= (rom_size / 4) - 1;
        rom[4 * rpos]     = buf[4 * baser];
        rom[4 * rpos + 1] = buf[4 * baser + 1];
        rom[4 * rpos + 2] = buf[4 * baser + 2];
        rom[4 * rpos + 3] = buf[4 * baser + 3];
    }
    free(buf);
    return 0;
}

/* The S data is the last 512 KB of the decrypted C data on this cart. */
static void sfix_extract(const uint8_t *rom, uint32_t rom_size,
                         uint8_t *fixed, uint32_t fixed_size) {
    const uint8_t *src = rom + rom_size - fixed_size;
    for (uint32_t i = 0; i < fixed_size; i++)
        fixed[i] = src[(i & ~0x1fu) + ((i & 7) << 2) + ((~i & 8) >> 2) + ((i & 0x10) >> 4)];
}

/* ---- CMC50 M1 decryption (same path as KOF2002) ---- */

static uint16_t sum16(const uint8_t *data, uint32_t length) {
    uint16_t sum = 0;
    for (uint32_t i = 0; i < length; i++) sum = (uint16_t)(sum + data[i]);
    return sum;
}

static uint16_t bitswap16(uint16_t val, const uint8_t *bits) {
    uint16_t r = 0;
    for (int i = 0; i < 16; i++)
        r |= (uint16_t)(((val >> bits[i]) & 1) << (15 - i));
    return r;
}

static uint32_t m1_scramble(uint32_t address, uint16_t key) {
    static const uint8_t k1[16] = {12, 0, 2, 4, 8, 15, 7, 13, 10, 1, 3, 6, 11, 9, 14, 5};
    static const uint8_t k3[16] = {7, 15, 14, 6, 5, 13, 12, 4, 11, 3, 10, 2, 9, 1, 8, 0};
    uint32_t block = (address >> 16) & 7;
    uint32_t aux = address & 0xffff;
    uint8_t p[16];
    for (int i = 0; i < 16; i++) p[i] = (uint8_t)p1[block][15 - i];
    aux ^= bitswap16(key, k1);
    aux = bitswap16((uint16_t)aux, p);
    aux ^= m1_address_0_7_xor[(aux >> 8) & 0xff];
    aux ^= (uint32_t)m1_address_8_15_xor[aux & 0xff] << 8;
    aux = bitswap16((uint16_t)aux, k3);
    return (block << 16) | aux;
}

static int decrypt_m1(const uint8_t *raw, size_t raw_size, uint8_t **out, size_t *out_size) {
    if (raw_size != 0x20000) {
        fprintf(stderr, "decrypt: 266-m1.m1 is %zu bytes, expected 0x20000\n", raw_size);
        return -1;
    }
    /* MAME's CMC50 region is 512 KB; only the first 128 KB came from the
     * chip.  Zero fill matches the hardware region exactly. */
    uint8_t *crypt = (uint8_t *)calloc(1, 0x80000);
    uint8_t *dec = (uint8_t *)malloc(0x80000);
    if (!crypt || !dec) { free(crypt); free(dec); return -1; }
    memcpy(crypt, raw, 0x20000);
    uint16_t key = sum16(crypt, 0x10000);
    for (uint32_t i = 0; i < 0x80000; i++) dec[i] = crypt[m1_scramble(i, key)];
    printf("  M1 CMC50 key %04x\n", key);
    free(crypt);
    *out = dec;
    *out_size = 0x80000;
    return 0;
}

/* ---- NEO-PCM2 V-ROM scramble, mode 1 (MAME pcm2_prot_device::swap) ---- */

static uint32_t bitswap24(uint32_t v, const int *bits) {
    uint32_t r = 0;
    for (int i = 0; i < 24; i++) r |= ((v >> bits[i]) & 1u) << (23 - i);
    return r;
}

static int pcm2_swap(uint8_t *rom, size_t size) {
    static const int perm[24] = {
        23, 22, 21, 20, 19, 18, 17, 0,
        15, 14, 13, 12, 11, 10, 9, 8,
        7, 6, 5, 4, 3, 2, 1, 16
    };
    static const uint32_t addrs[7][2] = {
        {0x000000, 0xa5000}, {0xffce20, 0x01000}, {0xfe2cf6, 0x4e001},
        {0xffac28, 0xc2000}, {0xfeb2c0, 0x0a000}, {0xff14ea, 0xa7001},
        {0xffb440, 0x02000},
    };
    static const uint8_t xordata[7][8] = {
        {0xf9, 0xe0, 0x5d, 0xf3, 0xea, 0x92, 0xbe, 0xef},
        {0xc4, 0x83, 0xa8, 0x5f, 0x21, 0x27, 0x64, 0xaf},
        {0xc3, 0xfd, 0x81, 0xac, 0x6d, 0xe7, 0xbf, 0x9e},
        {0xc3, 0xfd, 0x81, 0xac, 0x6d, 0xe7, 0xbf, 0x9e},
        {0xcb, 0x29, 0x7d, 0x43, 0xd2, 0x3a, 0xc2, 0xb4},
        {0x4b, 0xa4, 0x63, 0x46, 0xf0, 0x91, 0xea, 0x62},
        {0x4b, 0xa4, 0x63, 0x46, 0xf0, 0x91, 0xea, 0x62},
    };
    if (size != 0x1000000) {
        fprintf(stderr, "decrypt: V ROMs total %zu bytes, expected 0x1000000\n", size);
        return -1;
    }
    uint8_t *buf = (uint8_t *)malloc(size);
    if (!buf) return -1;
    memcpy(buf, rom, size);
    for (uint32_t i = 0; i < 0x1000000; i++) {
        uint32_t j = bitswap24(i, perm) ^ addrs[PCM2_MODE][1];
        uint32_t d = (i + addrs[PCM2_MODE][0]) & 0xffffff;
        rom[j] = buf[d] ^ xordata[PCM2_MODE][j & 7];
    }
    free(buf);
    return 0;
}

/* ---- C ROM assembly ---- */

static int decrypt_croms(const char *dir, const char *outdir) {
    static const char *pairs[4][2] = {
        {"266-c1.c1", "266-c2.c2"}, {"266-c3.c3", "266-c4.c4"},
        {"266-c5.c5", "266-c6.c6"}, {"266-c7.c7", "266-c8.c8"},
    };
    static const uint32_t crcs[4][2] = {
        {0x505f4e30, 0x3cb57482}, {0xf1cc6ad0, 0x45b806b7},
        {0x9a15dd6b, 0x281cb939}, {0x4b71f780, 0x29873d33},
    };
    uint8_t *even[4] = {0}, *odd[4] = {0};
    size_t csize = 0, csize2 = 0;
    for (int i = 0; i < 4; i++) {
        if (read_checked(dir, pairs[i][0], 0x800000, crcs[i][0], &even[i], &csize) ||
            read_checked(dir, pairs[i][1], 0x800000, crcs[i][1], &odd[i], &csize2)) {
            for (int j = 0; j < 4; j++) { free(even[j]); free(odd[j]); }
            return -1;
        }
    }
    size_t total = csize * 8;   /* 64 MB interleaved */
    uint8_t *crom = (uint8_t *)malloc(total);
    uint8_t *srom = (uint8_t *)malloc(0x80000);
    if (!crom || !srom) { free(crom); free(srom); return -1; }
    for (int i = 0; i < 4; i++) {
        size_t base = csize * 2 * (size_t)i;
        for (size_t k = 0; k < csize; k++) {
            crom[base + 2 * k]     = even[i][k];
            crom[base + 2 * k + 1] = odd[i][k];
        }
    }
    if (gfx_decrypt(crom, (uint32_t)total)) return -1;
    sfix_extract(crom, (uint32_t)total, srom, 0x80000);
    if (write_file(outdir, "266-s1.s1", srom, 0x80000)) return -1;
    free(srom);

    for (int i = 0; i < 4; i++) {
        size_t base = csize * 2 * (size_t)i;
        for (size_t k = 0; k < csize; k++) {
            even[i][k] = crom[base + 2 * k];
            odd[i][k]  = crom[base + 2 * k + 1];
        }
        if (write_file(outdir, pairs[i][0], even[i], csize) ||
            write_file(outdir, pairs[i][1], odd[i], csize)) return -1;
        free(even[i]); free(odd[i]);
    }
    free(crom);
    return 0;
}

int main(int argc, char **argv) {
    const char *romdir = "roms-src", *outdir = "roms";
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--rom-path") && i + 1 < argc) romdir = argv[++i];
        else if (!strcmp(argv[i], "--out") && i + 1 < argc) outdir = argv[++i];
        else if (!strcmp(argv[i], "--help")) {
            printf("usage: matrimelee_decrypt [--rom-path DIR] [--out DIR]\n");
            return 0;
        }
    }
    crc32_init();
    printf("Matrimelee ROM preparation\n  input:  %s\n  output: %s\n", romdir, outdir);
    MATRIM_MKDIR(outdir);

    /* Program: fixed bank copied, 4 MB banked image block-unswapped. */
    size_t p1sz = 0, p2sz = 0;
    uint8_t *p1 = NULL, *p2 = NULL;
    if (read_checked(romdir, "266-p1.p1", 0x100000, 0x5d4c2dc7, &p1, &p1sz) ||
        read_checked(romdir, "266-p2.sp2", 0x400000, 0xa14b1906, &p2, &p2sz)) return 1;
    p2_block_swap(p2);
    if (write_file(outdir, "266-p1.p1", p1, p1sz) ||
        write_file(outdir, "266-p2.sp2", p2, p2sz)) return 1;
    free(p1); free(p2);

    /* Audio: CMC50 M1, then NEO-PCM2 V-ROM unscramble (mode 1). */
    size_t m1sz = 0, v1sz = 0, v2sz = 0;
    uint8_t *m1 = NULL, *v1 = NULL, *v2 = NULL, *m1dec = NULL;
    size_t m1decsz = 0;
    if (read_checked(romdir, "266-m1.m1", 0x20000, 0x456c3e6c, &m1, &m1sz)) return 1;
    if (decrypt_m1(m1, m1sz, &m1dec, &m1decsz) ||
        write_file(outdir, "266-m1.m1", m1dec, m1decsz)) return 1;
    free(m1); free(m1dec);

    if (read_checked(romdir, "266-v1.v1", 0x800000, 0xa4f83690, &v1, &v1sz) ||
        read_checked(romdir, "266-v2.v2", 0x800000, 0xd0f69eda, &v2, &v2sz)) return 1;
    uint8_t *vrom = (uint8_t *)malloc(0x1000000);
    if (!vrom) return 1;
    memcpy(vrom, v1, 0x800000);
    memcpy(vrom + 0x800000, v2, 0x800000);
    if (pcm2_swap(vrom, 0x1000000) ||
        write_file(outdir, "266-v1.v1", vrom, 0x800000) ||
        write_file(outdir, "266-v2.v2", vrom + 0x800000, 0x800000)) return 1;
    free(v1); free(v2); free(vrom);

    /* Graphics + embedded fixed layer. */
    if (decrypt_croms(romdir, outdir)) return 1;

    /* System ROMs the runtime loads from the same directory. */
    const char *bios[] = {"sp-s2.sp1", "sfix.sfix", "sm1.sm1", "000-lo.lo", NULL};
    for (int i = 0; bios[i]; i++)
        if (copy_file(romdir, outdir, bios[i])) return 1;

    if (verify_outputs(outdir)) return 1;
    printf("ROM preparation complete; the original archive was not modified.\n");
    return 0;
}
