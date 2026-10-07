#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build the bundled word pairs (bigrams) for next-word suggestions.

Gboard and LatinIME ship a language model with the keyboard, so "I" is
followed by "am", "have", "don't" before the user has typed anything. Tastra
learns pairs from the user's own typing; these lists give it a start. Only
corpora whose text is under a licence compatible with the GPL-3.0 are used
(CC BY-SA 4.0 and CC BY 4.0; CC licences below 4.0 and NC ones are not):

  en  UD English EWT (web text: reviews, e-mail, blogs, answers)   CC BY-SA 4.0
      UD English ESLSpok (spoken English of learners)              CC BY-SA 4.0
  de  UD German GSD (news, reviews, wiki)                          CC BY-SA 4.0
  ru  UD Russian Taiga (blogs, social media, fiction, news)        CC BY-SA 4.0
      UD Russian GSD (wiki)                                        CC BY-SA 4.0
  uk  UA-GEC, corrected texts (essays, posts, translations)       CC BY 4.0
      UD Ukrainian ParlaMint (Verkhovna Rada transcripts)          CC BY-SA 4.0

The test parts of the corpora are left out (they are for measuring). A pair
counts when both words are in the bundled frequency list of the language,
within a sentence; commas, colons and dashes keep the pair (the keyboard
keeps the previous word across them), anything else breaks it. Pairs seen
fewer than twice are dropped.

    git clone --depth 1 https://github.com/UniversalDependencies/UD_English-EWT  (and the others)
    git clone --depth 1 https://github.com/grammarly/ua-gec
    python3 tools/build-bigrams.py CORPORA_DIR data
"""
import collections
import glob
import os
import re
import sys

SOURCES = {
    'en': [('ud', 'UD_English-EWT'), ('ud', 'UD_English-ESLSpok')],
    'de': [('ud', 'UD_German-GSD')],
    'ru': [('ud', 'UD_Russian-Taiga'), ('ud', 'UD_Russian-GSD')],
    'uk': [('uagec', 'ua-gec'), ('ud', 'UD_Ukrainian-ParlaMint')],
}
CREDITS = {
    'en': 'UD English EWT and ESLSpok (CC BY-SA 4.0)',
    'de': 'UD German GSD (CC BY-SA 4.0)',
    'ru': 'UD Russian Taiga and GSD (CC BY-SA 4.0)',
    'uk': 'UA-GEC by Grammarly (CC BY 4.0) and UD Ukrainian ParlaMint (CC BY-SA 4.0)',
}
MIN_COUNT = 2

APOSTROPHES = str.maketrans({'’': "'", 'ʼ': "'", '‘': "'", '`': "'"})
# Letters with inner apostrophes or hyphens ("don't", "кто-то", "м'ясо").
TOKEN = re.compile(r"[^\W\d_]+(?:['-][^\W\d_]+)*|[,;:–—-]|\S", re.UNICODE)
KEEPS_PAIR = set(',;:-–—')


def ud_texts(directory):
    for path in sorted(glob.glob(os.path.join(directory, '*.conllu'))):
        if '-test.' in os.path.basename(path):
            continue
        with open(path, encoding='utf-8') as f:
            for line in f:
                if line.startswith('# text = '):
                    yield line[len('# text = '):].strip()


def uagec_texts(directory):
    # One corrected version per document (several annotators corrected some).
    seen = set()
    for path in sorted(glob.glob(os.path.join(directory, 'data/gec-fluency/train/target/*.txt'))):
        doc = os.path.basename(path).split('.')[0]
        if doc in seen:
            continue
        seen.add(doc)
        with open(path, encoding='utf-8') as f:
            yield from f.read().splitlines()


def sentences(text):
    # Sentence ends inside a line break the pairs too.
    yield from re.split(r'[.!?…]+', text.translate(APOSTROPHES))


def count_pairs(texts, known):
    pairs = collections.Counter()
    for text in texts:
        for sentence in sentences(text):
            previous = None
            for token in TOKEN.findall(sentence):
                if token in KEEPS_PAIR:
                    continue
                word = token.lower()
                if word not in known:
                    previous = None
                    continue
                if previous is not None:
                    pairs[(previous, word)] += 1
                previous = word
    return pairs


def main():
    corpora, data = sys.argv[1], sys.argv[2]
    for language, sources in SOURCES.items():
        with open(os.path.join(data, 'frequency', language + '.txt'), encoding='utf-8') as f:
            known = {line.strip() for line in f if line.strip()}
        pairs = collections.Counter()
        for kind, name in sources:
            directory = os.path.join(corpora, name)
            texts = ud_texts(directory) if kind == 'ud' else uagec_texts(directory)
            pairs.update(count_pairs(texts, known))
        kept = [(p, n) for p, n in pairs.items() if n >= MIN_COUNT]
        kept.sort(key=lambda item: (item[0][0], -item[1], item[0][1]))
        os.makedirs(os.path.join(data, 'bigrams'), exist_ok=True)
        out = os.path.join(data, 'bigrams', language + '.txt')
        with open(out, 'w', encoding='utf-8') as f:
            f.write(f'# Word pairs for next-word suggestions: previous word, next word, count.\n')
            f.write(f'# Built by tools/build-bigrams.py from {CREDITS[language]}.\n')
            for (previous, word), n in kept:
                f.write(f'{previous} {word} {n}\n')
        print(f'{language}: {len(kept)} pairs (of {len(pairs)}), {sum(pairs.values())} counted', file=sys.stderr)


if __name__ == '__main__':
    main()
