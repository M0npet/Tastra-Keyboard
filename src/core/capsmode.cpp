// SPDX-License-Identifier: GPL-3.0-or-later
// Port of AOSP LatinIME CapsModeUtils.getCapsMode (sentence mode),
// Copyright (C) The Android Open Source Project, Apache License 2.0.
#include "capsmode.h"

namespace Tastra::CapsMode
{
namespace
{
bool isStartPunctuation(QChar c)
{
    return c == QLatin1Char('"') || c == QLatin1Char('\'') || c == QChar(0x00BF) || c == QChar(0x00A1)
        || c.category() == QChar::Punctuation_Open || c.category() == QChar::Punctuation_InitialQuote;
}
bool isSpaceChar(QChar c) { return c.category() == QChar::Separator_Space; }
bool isSentenceTerminator(QChar c) { return c == QLatin1Char('.') || c == QLatin1Char('?') || c == QLatin1Char('!'); }
bool isSentenceSeparator(QChar c) { return c == QLatin1Char('.'); }
bool isAbbreviationMarker(QChar c) { return c == QLatin1Char('.'); }
}

bool sentenceCaps(const QString &cs, bool hasSpaceBefore, bool americanTypography, bool germanRules)
{
    // Step 2: skip opening punctuation at the end of the input.
    qsizetype i;
    if (hasSpaceBefore) {
        i = cs.size() + 1;
    } else {
        for (i = cs.size(); i > 0; i--) {
            if (!isStartPunctuation(cs.at(i - 1))) break;
        }
    }
    // Step 3: start of a paragraph?
    qsizetype j = i;
    QChar prevChar = QLatin1Char(' ');
    if (hasSpaceBefore) --j;
    while (j > 0) {
        prevChar = cs.at(j - 1);
        if (!isSpaceChar(prevChar) && prevChar != QLatin1Char('\t')) break;
        j--;
    }
    if (j <= 0 || prevChar.isSpace()) {
        if (germanRules) {
            // No capital at the start of a line when the previous line ends in a comma.
            bool hasNewLine = false;
            while (--j >= 0 && prevChar.isSpace()) {
                if (prevChar == QLatin1Char('\n')) hasNewLine = true;
                prevChar = cs.at(j);
            }
            if (prevChar == QLatin1Char(',') && hasNewLine) return false;
        }
        return true;
    }
    if (i == j) return false;                       // no whitespace before the cursor
    // Step 4: end of a sentence?
    if (americanTypography) {
        for (; j > 0; j--) {
            const QChar c = cs.at(j - 1);
            if (c != QLatin1Char('"') && c != QLatin1Char('\'') && c.category() != QChar::Punctuation_Close
                && c.category() != QChar::Punctuation_FinalQuote) {
                break;
            }
        }
    }
    if (j <= 0) return false;
    QChar c = cs.at(--j);
    if (isSentenceTerminator(c) && !isAbbreviationMarker(c)) return true;
    if (!isSentenceSeparator(c) || j <= 0) return false;
    // A period: full stop or abbreviation like "e.g."?
    enum { START, WORD, PERIOD, LETTER, NUMBER } state = START;
    while (j > 0) {
        c = cs.at(--j);
        switch (state) {
        case START:
            if (c.isLetter()) state = WORD;
            else if (c.isSpace()) return false;
            else if (c.isDigit() && germanRules) state = NUMBER;
            else return true;
            break;
        case WORD:
            if (c.isLetter()) state = WORD;
            else if (isSentenceSeparator(c)) state = PERIOD;
            else return true;
            break;
        case PERIOD:
            if (c.isLetter()) state = LETTER;
            else return true;
            break;
        case LETTER:
            if (c.isLetter()) state = LETTER;
            else if (isSentenceSeparator(c)) state = PERIOD;
            else return false;
            break;
        case NUMBER:
            if (c.isLetter()) state = WORD;
            else if (c.isDigit()) state = NUMBER;
            else return false;
            break;
        }
    }
    return !(state == START || state == LETTER);
}

}
