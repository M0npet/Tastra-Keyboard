#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Put words with an apostrophe back into the bundled frequency lists.

FrequencyWords splits tokens at apostrophes, so the lists lack "don't",
"I'm", "п'ять" and "м'ясо", and instead rank fragments such as "don", "isn"
or "ясо" far too high. wordfreq (Robyn Speer; data CC BY-SA 4.0) keeps them
whole. For each language this script

  1. takes every word with an apostrophe from wordfreq's top 100 000,
  2. drops fragments of those words that wordfreq does not know as words
     ("isn", "didn", "ясо") and moves fragments whose apostrophe forms are
     used more than the word itself ("don", "let") to wordfreq's rank,
  3. inserts the apostrophe words at the position of wordfreq's rank,

where "position of wordfreq rank r" is the number of list words that wordfreq
ranks before r. The rest of the list keeps its order. Run once after
updating the lists:

    pip install wordfreq
    python3 tools/merge-apostrophe-words.py data/frequency en uk
"""
import bisect
import re
import sys
from pathlib import Path

import wordfreq

APOSTROPHES = re.compile(r"[ʼ’`]")
WORD = re.compile(r"^[^\W\d_]+(?:'[^\W\d_]+)+$")


def merge(path: Path, lang: str, top_n: int = 100_000) -> tuple[int, int, int]:
    words = [w.strip() for w in path.read_text(encoding='utf-8').splitlines() if w.strip()]
    top = wordfreq.top_n_list(lang, top_n)
    wf_rank = {w: i for i, w in enumerate(top)}

    apostrophe_words = []
    for w in top:
        w = APOSTROPHES.sub("'", w)
        if WORD.match(w) and w not in apostrophe_words:
            apostrophe_words.append(w)
    fragments = {part for w in apostrophe_words for part in w.split("'")}
    forms_mass = {}
    for w in apostrophe_words:
        for part in set(w.split("'")):
            forms_mass[part] = forms_mass.get(part, 0.0) + wordfreq.word_frequency(w, lang)

    existing = set(words)
    kept, moved, dropped = [], [], 0
    for w in words:
        if w in fragments:
            if w not in wf_rank:
                dropped += 1          # only ever a piece of a word ("isn", "ясо")
                continue
            # Inflated by the split when its apostrophe forms are used more
            # than the word itself ("don" < "don't", "let" < "let's"); then it
            # goes to wordfreq's rank. Ordinary words ("people" from
            # "people's", "father" from "father's") stay where they are.
            if forms_mass.get(w, 0.0) > wordfreq.word_frequency(w, lang):
                moved.append(w)
                continue
        kept.append(w)

    # Position for wordfreq rank r = how many kept words wordfreq ranks
    # higher (a count, so one odd word cannot drag everything to the front).
    known = sorted(wf_rank[w] for w in kept if w in wf_rank)
    inserts = [(wf_rank[w], w) for w in moved]
    inserts += [(wf_rank[w], w) for w in apostrophe_words if w not in existing and w in wf_rank]
    inserts.sort()
    result, k = [], 0
    for position, w in enumerate(kept):
        while k < len(inserts) and bisect.bisect_left(known, inserts[k][0]) <= position:
            result.append(inserts[k][1])
            k += 1
        result.append(w)
    result.extend(w for _, w in inserts[k:])

    seen, unique = set(), []
    for w in result:
        if w not in seen:
            seen.add(w)
            unique.append(w)
    path.write_text('\n'.join(unique) + '\n', encoding='utf-8')
    added = sum(1 for _, w in inserts if w not in existing)
    return added, len(moved), dropped


def main() -> int:
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    folder = Path(sys.argv[1])
    for lang in sys.argv[2:]:
        added, moved, dropped = merge(folder / f'{lang}.txt', lang)
        print(f'{lang}: +{added} apostrophe words, {moved} fragments re-ranked, {dropped} fragments dropped')
    return 0


if __name__ == '__main__':
    sys.exit(main())
