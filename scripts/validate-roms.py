#!/usr/bin/env python3
"""Validate Matrimelee (MAME set `matrim`) ROM files.

Checks every file the build and runtime use against MAME's neogeo.cpp and the
recorded prepared-output hashes.  The game set and the `neogeo` system ROMs
may live in one directory or be passed as two.  Both the original encrypted
set and the prepared runtime set are accepted.

Usage:
    python scripts/validate-roms.py ROM_DIR [BIOS_DIR]

Exit code 0 means every file matched; 1 means a problem was found.
No ROM data leaves the paths given.
"""
import hashlib
import os
import sys
import zlib

# name: (orig_size, crc32 original, sha1 original,
#        prepared_size, sha1 prepared)
GAME = {
    "266-p1.p1":  (0x100000, "5d4c2dc7",
                   "8d723b0d28ec344eef26009b361a2b97d300dd51",
                   0x100000, "8d723b0d28ec344eef26009b361a2b97d300dd51"),
    "266-p2.sp2": (0x400000, "a14b1906",
                   "1daa14d73512f760ef569b06f9facb279437d1db",
                   0x400000, "3780db1b8947a8371a46b67ca1a14a12bf97986b"),
    "266-s1.s1":  (0x080000, None, None,
                   0x080000, "de40a742508eb44995c1fb467aef5c3aed403497"),
    "266-m1.m1":  (0x020000, "456c3e6c",
                   "5a07d0186198a18d2dda1331093cf29b0b9b2984",
                   0x080000, "3919a9133bffbdce013b23794773b7aa35ea91b8"),
    "266-v1.v1":  (0x800000, "a4f83690",
                   "200d683d8c30ebc6d0ed331aa5bbba39b4e07160",
                   0x800000, "6734da4f1e882a8524c04ff375567ed96a2dae5e"),
    "266-v2.v2":  (0x800000, "d0f69eda",
                   "9d7e98976ad433ed8a35d7afffa38130444ba7db",
                   0x800000, "dc13f1af6afaf43f2ac20b7cf1c751a047450312"),
    "266-c1.c1":  (0x800000, "505f4e30",
                   "f22b6f76fc0cad963555dc89d072967c8dc8b79a",
                   0x800000, "31bca460462b8545b3e5264804987ab951e9b202"),
    "266-c2.c2":  (0x800000, "3cb57482",
                   "dab15bc24391f9a5173de76af48b612fb9636ccf",
                   0x800000, "579470839a8d23b2d22cf727d2b43c6c3c113e1f"),
    "266-c3.c3":  (0x800000, "f1cc6ad0",
                   "66c1cccc0332ffd2d3064f06330c41f95ca09ced",
                   0x800000, "a6b8aaf11d25f871b52e9626fe0e4ae0683c3c52"),
    "266-c4.c4":  (0x800000, "45b806b7",
                   "c2bb866fded53d62fad0fc88d89d5e7d4cb1894f",
                   0x800000, "106875db996ec29b0254b34f9d935c761cde20c4"),
    "266-c5.c5":  (0x800000, "9a15dd6b",
                   "194a6973a7a9e3847efe1bdbaeaeb16e74aff2dd",
                   0x800000, "3a2f14517617fb98a202ceec88d52891e1438c8e"),
    "266-c6.c6":  (0x800000, "281cb939",
                   "bdb7766cfde581ccfaee2be7fe48445f360a2301",
                   0x800000, "9f4b0bac92262e8c1c1055a70f13c7c326367f73"),
    "266-c7.c7":  (0x800000, "4b71f780",
                   "d5611a6f6b730db58613b48f2b0174661ccfb7bb",
                   0x800000, "43e170e6d48c01d8c323f9309f97e6038bab20d7"),
    "266-c8.c8":  (0x800000, "29873d33",
                   "dc77f129ed49b8d40d0d4241feef3f6c2f19a987",
                   0x800000, "3cdd8037f5004df92f4c19ba0746c8bccb4e2473"),
}

BIOS = {
    "sp-s2.sp1": (0x020000, "9036d879", "4f5ed7105b7128794654ce82b51723e16e389543"),
    "sfix.sfix": (0x020000, "c2ea0cfd", "fd4a618cdcdbf849374f0a50dd8efe9dbab706c3"),
    "sm1.sm1":   (0x020000, "94416d67", "42f9d7ddd6c0931fd64226a60dc73602b2819dcf"),
    "000-lo.lo": (0x020000, "5a86cff2", "5992277debadeb64d1c1c64b0a92d9293eaf7e4a"),
}


def sha1(data):
    return hashlib.sha1(data).hexdigest()


def check_game(name, expected, directory):
    orig_size, crc, orig_sha, prep_size, prep_sha = expected
    path = os.path.join(directory, name)
    if not os.path.isfile(path):
        print("MISSING   %-12s" % name)
        return False
    data = open(path, "rb").read()
    got_sha = sha1(data)
    got_crc = "%08x" % (zlib.crc32(data) & 0xFFFFFFFF)
    if len(data) == orig_size and orig_sha and got_sha == orig_sha:
        print("ORIGINAL  %-12s %9d bytes  crc32 %s" % (name, len(data), got_crc))
        return True
    if len(data) == prep_size and prep_sha and got_sha == prep_sha:
        print("PREPARED  %-12s %9d bytes  crc32 %s" % (name, len(data), got_crc))
        return True
    print("MISMATCH  %-12s size %d (want %d original or %d prepared) crc32 %s sha1 %s"
          % (name, len(data), orig_size, prep_size, got_crc, got_sha))
    return False


def check_bios(name, expected, search):
    size, crc, want_sha = expected
    for d in search:
        p = os.path.join(d, name)
        if os.path.isfile(p):
            break
    else:
        print("MISSING   %-12s (looked in %s)" % (name, ", ".join(search)))
        return False
    data = open(p, "rb").read()
    got_crc = "%08x" % (zlib.crc32(data) & 0xFFFFFFFF)
    got_sha = sha1(data)
    ok = len(data) == size and got_crc == crc and got_sha == want_sha
    if ok:
        print("OK        %-12s %9d bytes  crc32 %s" % (name, len(data), got_crc))
    else:
        print("MISMATCH  %-12s size %d (want %d) crc32 %s (want %s)"
              % (name, len(data), size, got_crc, crc))
    return ok


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    game_dir = sys.argv[1]
    bios_dir = sys.argv[2] if len(sys.argv) > 2 else game_dir
    ok = True
    print("== game set matrim ==")
    for n, e in GAME.items():
        if n == "266-s1.s1" and not os.path.isfile(os.path.join(game_dir, n)):
            # The retail encrypted set has no S ROM; it is extracted from the
            # C data during preparation.  Its presence is required only for a
            # prepared runtime folder, which is checked when the file exists.
            print("OPTIONAL  %-12s (not in the original set; prepared from C data)" % n)
            continue
        ok &= check_game(n, e, game_dir)
    print("== system set neogeo ==")
    for n, e in BIOS.items():
        ok &= check_bios(n, e, [bios_dir, game_dir])
    print("result:", "all files verified" if ok else "PROBLEMS FOUND")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
