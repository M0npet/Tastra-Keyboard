#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Generates the key sounds in data/sounds, 16-bit mono 44.1 kHz.
# Own work, no samples used. Like LatinIME (AudioManager FX_KEYPRESS_STANDARD, _SPACEBAR,
# _DELETE, _RETURN), Space, Backspace and Return sound different from letters:
#   click.wav   a soft 22 ms tick (noise burst with a fast exponential decay)
#   space.wav   lower and duller, a little longer
#   delete.wav  a short, slightly higher tick
#   return.wav  the deepest, with a short second knock
import math, random, struct, wave
from pathlib import Path

RATE = 44100


def tick(length, decay, pitch, tone_share, gain, seed, second=None):
    random.seed(seed)
    frames = []
    for i in range(int(RATE * length)):
        t = i / RATE
        envelope = math.exp(-t * decay)
        if second is not None and t >= second:
            envelope += 0.6 * math.exp(-(t - second) * decay)
        tone = math.sin(2 * math.pi * pitch * t) * tone_share
        noise = (random.random() * 2 - 1) * (1.0 - tone_share)
        frames.append(int(max(-1.0, min(1.0, (tone + noise) * envelope * gain)) * 32767))
    return frames


SOUNDS = {
    'click.wav': tick(0.022, 260.0, 1900, 0.35, 0.45, 7),
    'space.wav': tick(0.030, 200.0, 900, 0.55, 0.42, 11),
    'delete.wav': tick(0.018, 300.0, 2400, 0.40, 0.40, 13),
    'return.wav': tick(0.045, 210.0, 700, 0.60, 0.45, 17, second=0.018),
}

out_dir = Path(__file__).resolve().parent.parent / 'data/sounds'
for name, frames in SOUNDS.items():
    out = out_dir / name
    with wave.open(str(out), 'wb') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(RATE)
        w.writeframes(b''.join(struct.pack('<h', f) for f in frames))
    print(out, out.stat().st_size, 'bytes')
