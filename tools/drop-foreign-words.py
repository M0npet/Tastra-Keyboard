#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Drop words of a neighbouring language from a bundled frequency list.

FrequencyWords counts every token of a language's subtitles, and Ukrainian
subtitles are full of Russian: "что" was rank 8 and "как" rank 34 of the
Ukrainian list, so they were offered and corrected to while typing
Ukrainian. A word of the list is dropped when the other language's Hunspell
dictionary accepts it exactly as written (so its names, "Джек", stay) and

  1. the list language's dictionary rejects it, also capitalised ("что"), or
  2. wordfreq (Robyn Speer, data CC BY-SA 4.0) finds it at least ten times
     more common in the other language ("конечно", "мне": Hunspell's
     Ukrainian dictionary accepts them), or

when it has a letter of the other alphabet only (ы, э, ъ, ё in Ukrainian).
Hunspell still accepts what it accepts, so a dropped word is never corrected
away; it only loses its rank. Run once after updating the lists, with the
dictionaries installed:

    pip install wordfreq
    python3 tools/drop-foreign-words.py data/frequency uk:ru

Dictionaries are looked up in /usr/share/hunspell (or $HUNSPELL_DIR). Only the
Ukrainian list needs it: the Russian list has 12 such words, and German words
in the English list ("facto", "cetera") are mostly parts of English phrases.
"""
import ctypes
import ctypes.util
import os
import sys
from pathlib import Path

DICTIONARIES = {'en': 'en_US', 'de': 'de_DE', 'ru': 'ru_RU', 'uk': 'uk_UA'}
ALPHABETS = {'ru': set('абвгдеёжзийклмнопрстуфхцчшщъыьэюя'), 'uk': set('абвгґдеєжзиіїйклмнопрстуфхцчшщьюя')}


class Speller:
    _lib = None

    def __init__(self, language: str):
        if Speller._lib is None:
            name = ctypes.util.find_library('hunspell') or ctypes.util.find_library('hunspell-1.7')
            Speller._lib = ctypes.CDLL(name or 'libhunspell-1.7.so.0')
            Speller._lib.Hunspell_create.restype = ctypes.c_void_p
            Speller._lib.Hunspell_create.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
            Speller._lib.Hunspell_spell.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        base = Path(os.environ.get('HUNSPELL_DIR', '/usr/share/hunspell')) / DICTIONARIES[language]
        aff, dic = base.with_suffix('.aff'), base.with_suffix('.dic')
        if not aff.exists() or not dic.exists():
            raise SystemExit(f'missing Hunspell dictionary {base}.aff/.dic')
        self.encoding = 'utf-8'
        for line in aff.read_bytes().splitlines():
            if line.startswith(b'SET '):
                self.encoding = line[4:].strip().decode('ascii').lower()
                break
        self.handle = Speller._lib.Hunspell_create(str(aff).encode(), str(dic).encode())

    def spell(self, word: str) -> bool:
        try:
            data = word.encode(self.encoding)
        except UnicodeEncodeError:
            return False
        return Speller._lib.Hunspell_spell(self.handle, data) != 0


def drop(path: Path, language: str, other: str) -> tuple[int, list[str]]:
    import wordfreq
    own, foreign = Speller(language), Speller(other)
    other_only = ALPHABETS.get(other, set()) - ALPHABETS.get(language, set())
    words = [w.strip() for w in path.read_text(encoding='utf-8').splitlines() if w.strip()]
    kept, dropped = [], []
    for w in words:
        foreign_letters = bool(other_only) and bool(set(w) & other_only)
        foreign_word = foreign.spell(w) and (
            (not own.spell(w) and not own.spell(w[:1].upper() + w[1:]))
            or wordfreq.zipf_frequency(w, other) - wordfreq.zipf_frequency(w, language) >= 1.0)
        (dropped if foreign_letters or foreign_word else kept).append(w)
    path.write_text('\n'.join(kept) + '\n', encoding='utf-8')
    return len(words), dropped


def main() -> int:
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    folder = Path(sys.argv[1])
    for pair in sys.argv[2:]:
        language, other = pair.split(':')
        total, dropped = drop(folder / f'{language}.txt', language, other)
        print(f'{language}: dropped {len(dropped)} of {total} words that are {other} '
              f'(top: {" ".join(dropped[:12])})')
    return 0


if __name__ == '__main__':
    sys.exit(main())
