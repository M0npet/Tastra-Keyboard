# Word frequency lists

Derived from FrequencyWords by Hermit Dave (content/2018, 50k lists for en, de,
ru, uk), generated from OpenSubtitles 2018 (http://opus.nlpl.eu/).
https://github.com/hermitdave/FrequencyWords — content licensed CC BY-SA 4.0.

Adaptation: lowercased, counts removed, non-word tokens dropped; one word per
line in descending frequency. CC BY-SA 4.0 is one-way compatible with GPLv3
(Creative Commons, 2015); this project is GPL-3.0-or-later.

Words with an apostrophe (en: "don't", "I'm", "it's"…; uk: "п'ять", "м'ясо",
"зв'язок"…) were missing because FrequencyWords splits tokens at apostrophes.
They were added from wordfreq by Robyn Speer (https://github.com/rspeer/wordfreq,
v3; data CC BY-SA 4.0) with `tools/merge-apostrophe-words.py`, which also moves
fragments such as "don" or "ім" to wordfreq's rank and drops pure fragments
such as "isn". Only list positions were taken (no counts). wordfreq's data
includes SUBTLEX word lists by Marc Brysbaert et al.
(http://crr.ugent.be/programs-data/subtitle-frequencies; SUBTLEX is freely
available data), Google Books Ngrams (http://books.google.com/ngrams),
OpenSubtitles 2018 via OPUS, Wikipedia, ParaCrawl and the Leeds Internet
Corpus.
