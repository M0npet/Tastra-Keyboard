#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Generates data/sounds/click.wav: a soft 22 ms key tick (noise burst with a
# fast exponential decay), 16-bit mono 44.1 kHz. Own work, no samples used.
import math, random, struct, wave
from pathlib import Path

rate, length = 44100, 0.022
random.seed(7)
frames = []
for i in range(int(rate * length)):
    t = i / rate
    envelope = math.exp(-t * 260.0)
    tone = math.sin(2 * math.pi * 1900 * t) * 0.35
    noise = (random.random() * 2 - 1) * 0.65
    frames.append(int(max(-1.0, min(1.0, (tone + noise) * envelope * 0.45)) * 32767))
out = Path(__file__).resolve().parent.parent / "data/sounds/click.wav"
with wave.open(str(out), "wb") as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(rate)
    w.writeframes(b"".join(struct.pack("<h", f) for f in frames))
print(out, out.stat().st_size, "bytes")
