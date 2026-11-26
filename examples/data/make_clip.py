#!/usr/bin/env python3
# ------------------------------------------------------------------------------
# \author (c) Marco Paland (info@paland.design)
#             2026, PALANDesign, LLC
#
# \license The MIT License (MIT)
#          This file is part of the sid library (https://github.com/mpaland/sid), see LICENSE.
#
# \brief Makes the recorded clip of the examples: clip.h, through a WAV file and tools/wav2h.py
#        The sound is computed here, so the clip has no other source: plucked strings (Karplus-Strong: a burst
#        of noise in a delay line with a low-pass in its feedback) play an arpeggio and a chord.
#        11025 Hz, 16 bit, mono, 1.5 s. The noise is seeded: every run gives the same file.
#
#          python make_clip.py
# ------------------------------------------------------------------------------

import os
import random
import struct
import subprocess
import sys
import tempfile

RATE = 11025
LENGTH = int(RATE * 1.5)


def pluck(out, start, freq, level):
    """add one plucked string to out[] from frame 'start' to the end"""
    period = int(RATE / freq + 0.5)
    line = [random.uniform(-1.0, 1.0) for _ in range(period)]
    # the burst without its mean, so the string does not carry an offset
    mean = sum(line) / period
    line = [v - mean for v in line]
    for i in range(start, len(out)):
        k = (i - start) % period
        value = line[k]
        out[i] += value * level
        # feedback: the mean of two neighbours (low-pass), a little damping
        line[k] = 0.996 * 0.5 * (value + line[(k + 1) % period])


def main():
    random.seed(1982)
    out = [0.0] * LENGTH
    c4, e4, g4, c5 = 261.63, 329.63, 392.00, 523.25
    step = int(RATE * 0.14)
    for n, freq in enumerate((c4, e4, g4, c5)):
        pluck(out, n * step, freq, 0.5)
    chord = int(RATE * 0.70)
    for n, freq in enumerate((c4 / 2.0, c4, e4, g4, c5)):
        pluck(out, chord + n * int(RATE * 0.012), freq, 0.4)
    # fade out over the last 150 ms, then normalise to 90 % of full scale
    fade = int(RATE * 0.15)
    for i in range(fade):
        out[LENGTH - 1 - i] *= i / fade
    peak = max(abs(v) for v in out)
    samples = b''.join(struct.pack('<h', int(v / peak * 0.9 * 32767.0)) for v in out)

    here = os.path.dirname(os.path.abspath(__file__))
    wav = os.path.join(tempfile.gettempdir(), 'clip.wav')   # the header is the result, the WAV file is not kept
    fmt = struct.pack('<HHIIHH', 1, 1, RATE, RATE * 2, 2, 16)
    body = b'WAVE' + b'fmt ' + struct.pack('<I', len(fmt)) + fmt + b'data' + struct.pack('<I', len(samples)) + samples
    with open(wav, 'wb') as f:
        f.write(b'RIFF' + struct.pack('<I', len(body)) + body)

    tool = os.path.join(here, '..', '..', 'tools', 'wav2h.py')
    subprocess.check_call([sys.executable, tool, wav, os.path.join(here, 'clip.h'), '--name', 'clip'])
    os.remove(wav)


if __name__ == '__main__':
    main()
