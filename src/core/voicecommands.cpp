// SPDX-License-Identifier: GPL-3.0-or-later

#include "voicecommands.h"

#include <QHash>
#include <QList>
#include <QPair>
#include <QRegularExpression>
#include <QStringList>

namespace Tastra
{

namespace
{

// Lower case, ё as е, punctuation dropped, single spaces: what Whisper
// writes for "Delete last word." and "delete last word" alike.
QString normalized(const QString &text)
{
    QString out;
    out.reserve(text.size());
    for (QChar ch : text.toLower()) {
        if (ch == QChar(0x0451)) ch = QChar(0x0435);       // ё -> е
        if (ch.isLetterOrNumber() || ch == QLatin1Char('\'')) out += ch;
        else out += QLatin1Char(' ');
    }
    return out.simplified();
}

const QHash<QString, VoiceCommand::Kind> &phrases()
{
    static const QHash<QString, VoiceCommand::Kind> table = [] {
        QHash<QString, VoiceCommand::Kind> t;
        auto add = [&t](VoiceCommand::Kind kind, std::initializer_list<const char *> list) {
            for (const char *phrase : list) t.insert(QString::fromUtf8(phrase), kind);
        };
        add(VoiceCommand::DeleteLastWord,
            {"delete last word", "delete the last word", "letztes wort löschen", "das letzte wort löschen",
             "lösche das letzte wort", "lösch das letzte wort", "удали последнее слово", "удалить последнее слово",
             "сотри последнее слово", "стереть последнее слово", "видали останнє слово", "видалити останнє слово",
             "зітри останнє слово", "стерти останнє слово"});
        add(VoiceCommand::ClearSentence,
            {"clear", "delete last sentence", "delete the last sentence", "letzten satz löschen",
             "den letzten satz löschen", "lösche den letzten satz", "удали последнее предложение",
             "удалить последнее предложение", "видали останнє речення", "видалити останнє речення"});
        add(VoiceCommand::ClearAll,
            {"clear all", "delete all", "delete everything", "alles löschen", "lösche alles", "удали все",
             "удалить все", "очистить все", "стереть все", "видали все", "видалити все", "очистити все",
             "стерти все"});
        add(VoiceCommand::Send,
            {"send", "send it", "send message", "send the message", "senden", "absenden", "abschicken",
             "nachricht senden", "отправить", "отправь", "отправить сообщение", "надіслати", "надішли",
             "відправити", "відправ", "надіслати повідомлення"});
        add(VoiceCommand::NewLine,
            {"new line", "next line", "neue zeile", "nächste zeile", "новая строка", "с новой строки",
             "новий рядок", "з нового рядка"});
        add(VoiceCommand::Undo,
            {"undo", "rückgängig", "rückgängig machen", "отменить", "отмени", "скасувати", "скасуй"});
        return t;
    }();
    return table;
}

const QStringList &paragraphPhrases()
{
    static const QStringList list = {QStringLiteral("new paragraph"), QStringLiteral("neuer absatz"),
                                     QString::fromUtf8("новый абзац"), QString::fromUtf8("новий абзац")};
    return list;
}

}

VoiceCommand parseVoiceCommand(const QString &utterance)
{
    VoiceCommand command;
    const QString said = normalized(utterance);
    if (said.isEmpty()) return command;
    const auto found = phrases().constFind(said);
    if (found != phrases().constEnd()) {
        command.kind = found.value();
        if (command.kind == VoiceCommand::NewLine) command.argument = QStringLiteral("\n");
        return command;
    }
    if (paragraphPhrases().contains(said)) {
        command.kind = VoiceCommand::NewLine;
        command.argument = QStringLiteral("\n\n");
        return command;
    }
    // "<name> emoji" / "эмодзи <name>": at most four words for the name.
    static const QStringList emojiWords = {QStringLiteral("emoji"), QStringLiteral("emojis"),
                                           QString::fromUtf8("эмодзи"), QString::fromUtf8("емодзі"),
                                           QString::fromUtf8("смайлик")};
    QStringList words = said.split(QLatin1Char(' '));
    if (words.size() >= 2 && words.size() <= 5) {
        if (emojiWords.contains(words.last())) words.removeLast();
        else if (emojiWords.contains(words.first())) words.removeFirst();
        else return command;
        command.kind = VoiceCommand::Emoji;
        command.argument = words.join(QLatin1Char(' '));
    }
    return command;
}

QString spokenEmojiAlias(const QString &name)
{
    static const QHash<QString, QString> aliases = [] {
        QHash<QString, QString> a;
        auto add = [&a](const char *emoji, std::initializer_list<const char *> names) {
            for (const char *n : names) a.insert(QString::fromUtf8(n), QString::fromUtf8(emoji));
        };
        add("\u2764\uFE0F", {"heart", "red heart", "love heart", "herz", "сердце", "сердечко", "серце", "серденько"});
        add("\U0001F603", {"smiley", "smiley face", "smiling face", "смайлик", "смайл"});
        add("\U0001F602", {"laughing", "laughing face", "lol", "lachen", "смех", "смеюсь", "сміх"});
        add("\U0001F622", {"crying", "crying face", "sad", "sad face", "weinen", "traurig", "плач", "грусть", "сум"});
        add("\U0001F618", {"kiss", "kissing face", "kuss", "поцелуй", "поцілунок"});
        add("\U0001F609", {"wink", "winking face", "zwinkern", "подмигивание", "підморгування"});
        add("\U0001F44D", {"thumbs up", "like", "daumen hoch", "лайк", "класс", "клас"});
        add("\U0001F64F", {"thank you", "thanks", "danke", "спасибо", "дякую"});
        return a;
    }();
    return aliases.value(normalized(name));
}

QString applySpokenPunctuation(const QString &dictation)
{
    QString text = dictation.trimmed();
    if (text.isEmpty()) return text;
    const auto options = QRegularExpression::CaseInsensitiveOption | QRegularExpression::UseUnicodePropertiesOption;
    // Anywhere: names that are never ordinary words. A mark Whisper put
    // right before or after the name goes with it.
    static const QList<QPair<QRegularExpression, QString>> anywhere = {
        {QRegularExpression(QString::fromUtf8(
             R"(\s*(?:[,.;:]\s*)?(?<!\p{L})(?:question mark|fragezeichen|вопросительный знак|знак питання)(?!\p{L})[.,!?]?)"), options),
         QStringLiteral("?")},
        {QRegularExpression(QString::fromUtf8(
             R"(\s*(?:[,.;:]\s*)?(?<!\p{L})(?:exclamation mark|exclamation point|ausrufezeichen|восклицательный знак|знак оклику)(?!\p{L})[.,!?]?)"),
             options),
         QStringLiteral("!")},
    };
    for (const auto &[pattern, mark] : anywhere) text.replace(pattern, mark);
    // Only as the last words: "comma" and "full stop" could not be meant as
    // words there. ("period", "Punkt", "точка", "крапка" can: "during that
    // period", "ein wichtiger Punkt"; Whisper writes the full stop itself.)
    static const QRegularExpression atEnd(QString::fromUtf8(
        R"(\s*(?:[,.;:]\s*)?(?<!\p{L})(comma|komma|запятая|кома|full stop)(?!\p{L})[.,!?]?\s*$)"), options);
    const QRegularExpressionMatch end = atEnd.match(text);
    if (end.hasMatch()) {
        const QString word = end.captured(1).toLower();
        text.replace(end.capturedStart(0), end.capturedLength(0),
                     word == QLatin1String("full stop") ? QStringLiteral(".") : QStringLiteral(","));
    }
    // A space after a mark that ends a sentence, and a capital after it.
    static const QRegularExpression afterMark(QStringLiteral(R"(([?!])\s*(\p{Ll}))"), QRegularExpression::UseUnicodePropertiesOption);
    QRegularExpressionMatch m;
    qsizetype from = 0;
    while ((m = afterMark.match(text, from)).hasMatch()) {
        const QString replacement = m.captured(1) + QLatin1Char(' ') + m.captured(2).toUpper();
        text.replace(m.capturedStart(0), m.capturedLength(0), replacement);
        from = m.capturedStart(0) + replacement.size();
    }
    return text;
}

}
