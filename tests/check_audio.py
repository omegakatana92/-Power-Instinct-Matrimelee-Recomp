#!/usr/bin/env python3
"""Audio continuity regression for Matrimelee.

Usage: tests/check_audio.py <capture.wav> [--max-silence SEC] [--ignore A-B]...

Fails on music-sized silent runs after the BIOS boot.  This catches a stalled
Z80 music stream while one-shot effects still play, which a whole-file RMS
check would miss.  ``--ignore A-B`` excludes a known game-content gap (for
example GAME OVER -> attract -> HOW TO PLAY) from the verdict, in seconds.
"""
import math
import struct
import sys
import wave

MIN_PEAK = 1000
MAX_SILENCE = 3.0
BOOT_SECONDS = 10


def fail(msg):
    print(f"FAIL: {msg}")
    return 1


def parse_args(argv):
    path = None
    max_silence = MAX_SILENCE
    ignore = []
    i = 0
    while i < len(argv):
        a = argv[i]
        if a == "--max-silence" and i + 1 < len(argv):
            max_silence = float(argv[i + 1]); i += 2
        elif a == "--ignore" and i + 1 < len(argv):
            lo, hi = argv[i + 1].split("-", 1)
            ignore.append((float(lo), float(hi))); i += 2
        elif path is None:
            path = a; i += 1
        else:
            raise SystemExit(f"unexpected argument: {a}")
    if path is None:
        raise SystemExit("usage: tests/check_audio.py <capture.wav> "
                         "[--max-silence SEC] [--ignore A-B]...")
    return path, max_silence, ignore


def in_ignored(lo, hi, runs):
    return any(lo >= a and hi <= b for a, b in runs)


def main(path, max_silence, ignore):
    try:
        w = wave.open(path, "rb")
    except OSError as e:
        return fail(f"cannot open {path}: {e}")
    rate, channels, width = w.getframerate(), w.getnchannels(), w.getsampwidth()
    n = w.getnframes()
    raw = w.readframes(n)
    w.close()
    if rate < 8000 or channels not in (1, 2) or width != 2:
        return fail(f"unexpected WAV format: {rate} Hz, {channels} ch, {width * 8} bit")

    samples = struct.unpack("<%dh" % (len(raw) // 2), raw)
    if channels == 2:
        mono = [(samples[i] + samples[i + 1]) / 2.0 for i in range(0, len(samples), 2)]
    else:
        mono = list(samples)

    peak = max((abs(x) for x in mono), default=0)
    if peak < MIN_PEAK:
        return fail(f"capture is silent (peak {peak:.0f})")

    secs = []
    for start in range(0, len(mono) - rate + 1, rate):
        block = mono[start:start + rate]
        secs.append(math.sqrt(sum(x * x for x in block) / len(block)))

    silent = [i for i, r in enumerate(secs) if i >= BOOT_SECONDS and r < 50.0]
    runs = []
    for i in silent:
        if runs and i == runs[-1][1] + 1:
            runs[-1] = (runs[-1][0], i)
        else:
            runs.append((i, i))
    longest = max((b - a + 1 for a, b in runs), default=0)
    print(f"duration {len(mono) / rate:.1f} s, peak {peak:.0f}, "
          f"longest silent run {longest} s")
    if runs:
        print("silent runs:", [(a, b, b - a + 1) for a, b in runs])
    if ignore:
        print("ignored content windows:", [f"{a:.0f}-{b:.0f}s" for a, b in ignore])
    culprits = [(a, b) for a, b in runs
                if b - a + 1 > max_silence and not in_ignored(a, b, ignore)]
    if culprits:
        return fail(f"silent run longer than {max_silence:.0f} s")
    print("PASS: audio is continuous after boot")
    return 0


if __name__ == "__main__":
    p, ms, ig = parse_args(sys.argv[1:])
    sys.exit(main(p, ms, ig))
