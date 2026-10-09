// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

namespace Tastra
{

// Gboard voice typing commands (support.google.com/gboard/answer/11197787):
// said on their own, after a pause, they act instead of being typed.
// English, German, Russian and Ukrainian phrases are understood whatever the
// keyboard language is.
struct VoiceCommand {
    enum Kind {
        None,            // ordinary dictation
        DeleteLastWord,  // "delete last word"
        ClearSentence,   // "clear": the last sentence
        ClearAll,        // "clear all"
        Send,            // "send": the Return key
        NewLine,         // "new line" / "new paragraph": `argument` is the break
        Undo,            // "undo"
        Emoji,           // "smiley emoji": `argument` is what to look up
    };
    Kind kind = None;
    QString argument;
};

VoiceCommand parseVoiceCommand(const QString &utterance);

// The emoji people usually mean by a short spoken name ("heart" is ❤️, not
// the first emoji with the keyword "heart"); empty for other names, which
// are looked up in the emoji keywords.
QString spokenEmojiAlias(const QString &name);

// Spoken punctuation: "question mark", "exclamation mark" (and their German,
// Russian, Ukrainian names) anywhere; "comma" ("Komma", "запятая", "кома")
// and "full stop" only at the very end, where they cannot be meant as words
// ("drei Komma fünf"). "Period", "Punkt", "точка", "крапка" stay words
// ("точка зрения"); Whisper writes the full stop itself.
QString applySpokenPunctuation(const QString &dictation);

}
