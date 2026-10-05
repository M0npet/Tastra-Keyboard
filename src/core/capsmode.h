// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>

namespace V3Keyboard::CapsMode
{

// Sentence auto-capitalisation, ported from AOSP LatinIME
// CapsModeUtils.getCapsMode (Apache-2.0): whether the next word should start
// with a capital, given the text before the cursor. Handles abbreviations
// ("e.g. ", "U.S. "), closing quotes in American typography, opening
// punctuation, paragraph starts and German rules (dates "3. ", letters after
// "Liebe Sara,\n").
bool sentenceCaps(const QString &textBeforeCursor, bool hasSpaceBefore,
                  bool americanTypography, bool germanRules);

}
