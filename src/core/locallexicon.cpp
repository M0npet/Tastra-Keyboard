// SPDX-License-Identifier: GPL-3.0-or-later

#include "locallexicon.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QRegularExpression>
#include <QVector>
#include <algorithm>

namespace V3Keyboard
{
namespace
{

QStringList fallbackWords(const QString &code)
{
    if (code == QStringLiteral("de")) {
        return {
            QStringLiteral("aber"), QStringLiteral("alle"), QStringLiteral("auch"), QStringLiteral("auf"),
            QStringLiteral("aus"), QStringLiteral("bei"), QStringLiteral("bin"), QStringLiteral("bitte"),
            QStringLiteral("danke"), QStringLiteral("das"), QStringLiteral("dein"), QStringLiteral("der"),
            QStringLiteral("die"), QStringLiteral("du"), QStringLiteral("ein"), QStringLiteral("eine"),
            QStringLiteral("für"), QStringLiteral("gut"), QStringLiteral("hallo"), QStringLiteral("haben"),
            QStringLiteral("heute"), QStringLiteral("hier"), QStringLiteral("ich"), QStringLiteral("ist"),
            QStringLiteral("ja"), QStringLiteral("kann"), QStringLiteral("machen"), QStringLiteral("mein"),
            QStringLiteral("mit"), QStringLiteral("morgen"), QStringLiteral("nicht"), QStringLiteral("noch"),
            QStringLiteral("oder"), QStringLiteral("schon"), QStringLiteral("sehr"), QStringLiteral("sein"),
            QStringLiteral("sie"), QStringLiteral("so"), QStringLiteral("und"), QStringLiteral("uns"),
            QStringLiteral("was"), QStringLiteral("wenn"), QStringLiteral("wie"), QStringLiteral("wir"),
            QStringLiteral("wo"), QStringLiteral("wollen"), QStringLiteral("zeit"), QStringLiteral("zu")
        };
    }
    if (code == QStringLiteral("uk")) {
        return {
            QStringLiteral("але"), QStringLiteral("без"), QStringLiteral("буде"), QStringLiteral("бути"),
            QStringLiteral("вам"), QStringLiteral("вас"), QStringLiteral("вже"), QStringLiteral("він"),
            QStringLiteral("вона"), QStringLiteral("вони"), QStringLiteral("все"), QStringLiteral("гарно"),
            QStringLiteral("де"), QStringLiteral("для"), QStringLiteral("добре"), QStringLiteral("дякую"),
            QStringLiteral("є"), QStringLiteral("зараз"), QStringLiteral("і"), QStringLiteral("його"),
            QStringLiteral("коли"), QStringLiteral("може"), QStringLiteral("можна"), QStringLiteral("ми"),
            QStringLiteral("мене"), QStringLiteral("на"), QStringLiteral("нам"), QStringLiteral("не"),
            QStringLiteral("ні"), QStringLiteral("потрібно"), QStringLiteral("привіт"), QStringLiteral("про"),
            QStringLiteral("так"), QStringLiteral("тебе"), QStringLiteral("тепер"), QStringLiteral("ти"),
            QStringLiteral("тут"), QStringLiteral("у"), QStringLiteral("хочу"), QStringLiteral("це"),
            QStringLiteral("цей"), QStringLiteral("час"), QStringLiteral("що"), QStringLiteral("як"),
            QStringLiteral("я"), QStringLiteral("який"), QStringLiteral("якщо")
        };
    }
    if (code == QStringLiteral("ru")) {
        return {
            QStringLiteral("а"), QStringLiteral("без"), QStringLiteral("будет"), QStringLiteral("быть"),
            QStringLiteral("вам"), QStringLiteral("вас"), QStringLiteral("всё"), QStringLiteral("вы"),
            QStringLiteral("где"), QStringLiteral("да"), QStringLiteral("для"), QStringLiteral("добро"),
            QStringLiteral("есть"), QStringLiteral("ещё"), QStringLiteral("здесь"), QStringLiteral("и"),
            QStringLiteral("как"), QStringLiteral("когда"), QStringLiteral("может"), QStringLiteral("можно"),
            QStringLiteral("мы"), QStringLiteral("мне"), QStringLiteral("на"), QStringLiteral("нам"),
            QStringLiteral("не"), QStringLiteral("нет"), QStringLiteral("но"), QStringLiteral("он"),
            QStringLiteral("она"), QStringLiteral("они"), QStringLiteral("очень"), QStringLiteral("по"),
            QStringLiteral("пока"), QStringLiteral("потом"), QStringLiteral("привет"), QStringLiteral("про"),
            QStringLiteral("сейчас"), QStringLiteral("спасибо"), QStringLiteral("так"), QStringLiteral("тебя"),
            QStringLiteral("ты"), QStringLiteral("у"), QStringLiteral("хочу"), QStringLiteral("что"),
            QStringLiteral("это"), QStringLiteral("я"), QStringLiteral("если")
        };
    }

    return {
        QStringLiteral("a"), QStringLiteral("about"), QStringLiteral("after"), QStringLiteral("all"),
        QStringLiteral("also"), QStringLiteral("and"), QStringLiteral("are"), QStringLiteral("as"),
        QStringLiteral("at"), QStringLiteral("be"), QStringLiteral("because"), QStringLiteral("but"),
        QStringLiteral("can"), QStringLiteral("do"), QStringLiteral("for"), QStringLiteral("from"),
        QStringLiteral("good"), QStringLiteral("have"), QStringLiteral("hello"), QStringLiteral("here"),
        QStringLiteral("how"), QStringLiteral("i"), QStringLiteral("if"), QStringLiteral("in"),
        QStringLiteral("is"), QStringLiteral("it"), QStringLiteral("just"), QStringLiteral("know"),
        QStringLiteral("like"), QStringLiteral("me"), QStringLiteral("more"), QStringLiteral("my"),
        QStringLiteral("need"), QStringLiteral("no"), QStringLiteral("not"), QStringLiteral("now"),
        QStringLiteral("of"), QStringLiteral("on"), QStringLiteral("one"), QStringLiteral("or"),
        QStringLiteral("please"), QStringLiteral("so"), QStringLiteral("thank"), QStringLiteral("that"),
        QStringLiteral("the"), QStringLiteral("there"), QStringLiteral("this"), QStringLiteral("time"),
        QStringLiteral("to"), QStringLiteral("today"), QStringLiteral("want"), QStringLiteral("was"),
        QStringLiteral("we"), QStringLiteral("what"), QStringLiteral("when"), QStringLiteral("where"),
        QStringLiteral("will"), QStringLiteral("with"), QStringLiteral("yes"), QStringLiteral("you"),
        QStringLiteral("your")
    };
}

QStringList dictionaryCandidates(const QString &code)
{
    if (code == QStringLiteral("de")) {
        return {QStringLiteral("de_DE.dic"), QStringLiteral("de_DE_frami.dic"), QStringLiteral("de_AT.dic")};
    }
    if (code == QStringLiteral("uk")) {
        return {QStringLiteral("uk_UA.dic"), QStringLiteral("uk_UA-large.dic")};
    }
    if (code == QStringLiteral("ru")) {
        return {QStringLiteral("ru_RU.dic")};
    }
    return {QStringLiteral("en_US.dic"), QStringLiteral("en_GB.dic")};
}

QString prefixKey(const QString &word)
{
    return word.left(qMin(3, word.size()));
}

QString collapsed(const QString &word)
{
    QString result;
    for (const QChar ch : word) {
        if (result.isEmpty() || result.back() != ch) {
            result.append(ch);
        }
    }
    return result;
}

int levenshtein(const QString &a, const QString &b)
{
    QVector<int> prev(b.size() + 1);
    QVector<int> cur(b.size() + 1);
    for (int j = 0; j <= b.size(); ++j) prev[j] = j;
    for (int i = 1; i <= a.size(); ++i) {
        cur[0] = i;
        for (int j = 1; j <= b.size(); ++j) {
            const int cost = a.at(i - 1) == b.at(j - 1) ? 0 : 1;
            cur[j] = qMin(qMin(cur[j - 1] + 1, prev[j] + 1), prev[j - 1] + cost);
        }
        prev.swap(cur);
    }
    return prev[b.size()];
}

}

LocalLexicon::LocalLexicon()
{
    reload();
}

void LocalLexicon::setLanguage(const QString &code)
{
    if (code == m_language) return;
    m_language = code;
    reload();
}

QString LocalLexicon::language() const { return m_language; }

QString LocalLexicon::normalize(const QString &word) const
{
    QString w = word.trimmed().toLower();
    while (!w.isEmpty() && !w.front().isLetter()) w.remove(0, 1);
    while (!w.isEmpty() && !w.back().isLetter()) w.chop(1);
    return w;
}

QString LocalLexicon::alphabet() const
{
    if (m_language == QStringLiteral("de")) return QStringLiteral("abcdefghijklmnopqrstuvwxyzäöüß");
    if (m_language == QStringLiteral("uk")) return QStringLiteral("абвгґдеєжзиіїйклмнопрстуфхцчшщьюя");
    if (m_language == QStringLiteral("ru")) return QStringLiteral("абвгдеёжзийклмнопрстуфхцчшщъыьэюя");
    return QStringLiteral("abcdefghijklmnopqrstuvwxyz");
}

void LocalLexicon::reload()
{
    m_words.clear();
    m_prefixIndex.clear();
    m_personalFrequency.clear();
    m_bigramFrequency.clear();
    loadFallbackWords();
    loadSystemDictionary();
    loadLearning();
}

void LocalLexicon::addWord(const QString &word)
{
    const QString w = normalize(word);
    if (w.size() < 1 || w.size() > 48 || m_words.contains(w)) return;
    m_words.insert(w);
    for (int n = 1; n <= qMin(3, static_cast<int>(w.size())); ++n) {
        m_prefixIndex[w.left(n)].append(w);
    }
}

void LocalLexicon::loadFallbackWords()
{
    for (const QString &word : fallbackWords(m_language)) addWord(word);
}

void LocalLexicon::loadSystemDictionary()
{
    const QString base = QStringLiteral("/usr/share/hunspell");
    QString path;
    for (const QString &name : dictionaryCandidates(m_language)) {
        const QString candidate = base + QLatin1Char('/') + name;
        if (QFileInfo::exists(candidate)) {
            path = candidate;
            break;
        }
    }
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;
    const QByteArray raw = file.readAll();
    QString text = QString::fromUtf8(raw);
    if (text.count(QChar::ReplacementCharacter) > 8) {
        text = QString::fromLatin1(raw);
    }

    const QStringList lines = text.split(QLatin1Char('\n'));
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines.at(i).trimmed();
        if (i == 0) {
            bool ok = false;
            line.toInt(&ok);
            if (ok) continue;
        }
        if (line.isEmpty()) continue;
        line = line.section(QRegularExpression(QStringLiteral("[\\s/]")), 0, 0);
        addWord(line);
    }
}

void LocalLexicon::loadLearning()
{
    QSettings settings;
    const QVariantMap words = settings.value(QStringLiteral("learning/%1/words").arg(m_language)).toMap();
    for (auto it = words.constBegin(); it != words.constEnd(); ++it) {
        const QString word = normalize(it.key());
        const int count = it.value().toInt();
        if (!word.isEmpty() && count > 0) {
            m_personalFrequency.insert(word, count);
            addWord(word);
        }
    }

    const QVariantMap bigrams = settings.value(QStringLiteral("learning/%1/bigrams").arg(m_language)).toMap();
    for (auto it = bigrams.constBegin(); it != bigrams.constEnd(); ++it) {
        if (it.value().toInt() > 0) m_bigramFrequency.insert(it.key(), it.value().toInt());
    }
}

void LocalLexicon::persistLearning() const
{
    QVariantMap words;
    for (auto it = m_personalFrequency.constBegin(); it != m_personalFrequency.constEnd(); ++it) {
        words.insert(it.key(), it.value());
    }
    QVariantMap bigrams;
    for (auto it = m_bigramFrequency.constBegin(); it != m_bigramFrequency.constEnd(); ++it) {
        bigrams.insert(it.key(), it.value());
    }
    QSettings settings;
    settings.setValue(QStringLiteral("learning/%1/words").arg(m_language), words);
    settings.setValue(QStringLiteral("learning/%1/bigrams").arg(m_language), bigrams);
}

QString LocalLexicon::bigramKey(const QString &previousWord, const QString &word) const
{
    return normalize(previousWord) + QChar(0x001f) + normalize(word);
}

int LocalLexicon::scoreCandidate(const QString &candidate, const QString &typed, const QString &previousWord, bool prefix) const
{
    int score = prefix ? 1200 : 800;
    if (candidate == typed) score += 500;
    if (candidate.startsWith(typed)) score += 220;
    score -= qAbs(candidate.size() - typed.size()) * 8;
    score += qMin(300, m_personalFrequency.value(candidate) * 20);
    if (!previousWord.isEmpty()) {
        score += qMin(600, m_bigramFrequency.value(bigramKey(previousWord, candidate)) * 60);
    }
    return score;
}

QStringList LocalLexicon::ranked(const QSet<QString> &candidates, const QString &typed, const QString &previousWord, bool prefix, int limit) const
{
    struct Scored { QString word; int score; };
    QVector<Scored> scored;
    scored.reserve(candidates.size());
    for (const QString &candidate : candidates) {
        scored.push_back({candidate, scoreCandidate(candidate, typed, previousWord, prefix)});
    }
    std::sort(scored.begin(), scored.end(), [](const Scored &a, const Scored &b) {
        if (a.score != b.score) return a.score > b.score;
        if (a.word.size() != b.word.size()) return a.word.size() < b.word.size();
        return a.word < b.word;
    });
    QStringList result;
    for (const auto &item : scored) {
        if (result.size() >= limit) break;
        result.append(item.word);
    }
    return result;
}

QSet<QString> LocalLexicon::oneEditCandidates(const QString &word) const
{
    QSet<QString> result;
    const QString chars = alphabet();

    for (int i = 0; i < word.size(); ++i) {
        QString deleted = word;
        deleted.remove(i, 1);
        if (m_words.contains(deleted)) result.insert(deleted);
    }

    for (int i = 0; i + 1 < word.size(); ++i) {
        QString transposed = word;
        const QChar tmp = transposed[i];
        transposed[i] = transposed[i + 1];
        transposed[i + 1] = tmp;
        if (m_words.contains(transposed)) result.insert(transposed);
    }

    for (int i = 0; i < word.size(); ++i) {
        for (const QChar ch : chars) {
            if (word.at(i) == ch) continue;
            QString substituted = word;
            substituted[i] = ch;
            if (m_words.contains(substituted)) result.insert(substituted);
        }
    }

    for (int i = 0; i <= word.size(); ++i) {
        for (const QChar ch : chars) {
            QString inserted = word;
            inserted.insert(i, ch);
            if (m_words.contains(inserted)) result.insert(inserted);
        }
    }

    return result;
}

QStringList LocalLexicon::suggestions(const QString &word, const QString &previousWord, int limit) const
{
    const QString typed = normalize(word);
    if (typed.isEmpty() || limit <= 0) return {};

    QSet<QString> prefixCandidates;
    const QString key = prefixKey(typed);
    const auto bucket = m_prefixIndex.value(key);
    for (const QString &candidate : bucket) {
        if (candidate.startsWith(typed)) prefixCandidates.insert(candidate);
    }
    if (m_words.contains(typed)) prefixCandidates.insert(typed);

    QStringList result = ranked(prefixCandidates, typed, previousWord, true, limit);
    if (result.size() >= limit) return result;

    QSet<QString> corrections = oneEditCandidates(typed);
    for (const QString &existing : result) corrections.remove(existing);
    const QStringList corrected = ranked(corrections, typed, previousWord, false, limit - result.size());
    result.append(corrected);
    return result;
}

QString LocalLexicon::bestCorrection(const QString &word, const QString &previousWord) const
{
    const QString typed = normalize(word);
    // Two-character tokens are too ambiguous for safe automatic replacement.
    // Keeping them verbatim also makes double-space punctuation independent of
    // the host system's Hunspell dictionary contents.
    if (typed.size() < 3 || m_words.contains(typed)) return {};
    const QStringList result = ranked(oneEditCandidates(typed), typed, previousWord, false, 1);
    return result.isEmpty() ? QString() : result.first();
}

QStringList LocalLexicon::nextWords(const QString &previousWord, int limit) const
{
    const QString prev = normalize(previousWord);
    if (prev.isEmpty() || limit <= 0) return {};
    struct Scored { QString word; int score; };
    QVector<Scored> scored;
    const QString prefix = prev + QChar(0x001f);
    for (auto it = m_bigramFrequency.constBegin(); it != m_bigramFrequency.constEnd(); ++it) {
        if (!it.key().startsWith(prefix)) continue;
        scored.push_back({it.key().mid(prefix.size()), it.value()});
    }
    std::sort(scored.begin(), scored.end(), [](const Scored &a, const Scored &b) {
        if (a.score != b.score) return a.score > b.score;
        return a.word < b.word;
    });
    QStringList result;
    for (const auto &item : scored) {
        if (result.size() >= limit) break;
        result.append(item.word);
    }
    return result;
}

QString LocalLexicon::decodeGlide(const QStringList &trace, const QString &previousWord) const
{
    if (trace.isEmpty()) return {};
    QString sequence;
    for (const QString &part : trace) {
        const QString normalized = normalize(part);
        if (!normalized.isEmpty()) sequence += normalized.front();
    }
    sequence = collapsed(sequence);
    if (sequence.size() < 2) return {};

    QSet<QString> candidates;
    const QString first = sequence.left(1);
    const auto bucket = m_prefixIndex.value(first);
    for (const QString &word : bucket) {
        if (word.isEmpty() || word.front() != sequence.front()) continue;
        if (word.back() != sequence.back()) continue;
        if (qAbs(word.size() - sequence.size()) > qMax(3, static_cast<int>(sequence.size() / 2))) continue;
        candidates.insert(word);
    }
    if (candidates.isEmpty()) {
        for (const QString &word : m_words) {
            if (!word.isEmpty() && word.front() == sequence.front() && word.back() == sequence.back()) candidates.insert(word);
        }
    }

    QString best;
    int bestScore = -100000;
    for (const QString &word : candidates) {
        const int distance = levenshtein(collapsed(word), sequence);
        int score = 1000 - distance * 120 - qAbs(word.size() - sequence.size()) * 12;
        score += qMin(240, m_personalFrequency.value(word) * 20);
        score += qMin(480, m_bigramFrequency.value(bigramKey(previousWord, word)) * 60);
        if (score > bestScore) {
            bestScore = score;
            best = word;
        }
    }
    return bestScore >= 520 ? best : QString();
}

void LocalLexicon::learnWord(const QString &word)
{
    const QString w = normalize(word);
    if (w.size() < 2 || w.size() > 48) return;
    addWord(w);
    m_personalFrequency[w] = qMin(1000, m_personalFrequency.value(w) + 1);
    persistLearning();
}

void LocalLexicon::learnBigram(const QString &previousWord, const QString &word)
{
    const QString prev = normalize(previousWord);
    const QString next = normalize(word);
    if (prev.isEmpty() || next.isEmpty()) return;
    const QString key = bigramKey(prev, next);
    m_bigramFrequency[key] = qMin(1000, m_bigramFrequency.value(key) + 1);
    persistLearning();
}

void LocalLexicon::learnWordWithContext(const QString &word, const QString &previousWord)
{
    const QString w = normalize(word);
    if (w.size() < 2 || w.size() > 48) return;

    addWord(w);
    m_personalFrequency[w] = qMin(1000, m_personalFrequency.value(w) + 1);

    const QString prev = normalize(previousWord);
    if (!prev.isEmpty()) {
        const QString key = bigramKey(prev, w);
        m_bigramFrequency[key] = qMin(1000, m_bigramFrequency.value(key) + 1);
    }

    persistLearning();
}

void LocalLexicon::clearLearning()
{
    m_personalFrequency.clear();
    m_bigramFrequency.clear();
    QSettings settings;
    settings.remove(QStringLiteral("learning/%1").arg(m_language));
    reload();
}

bool LocalLexicon::hasWord(const QString &word) const { return m_words.contains(normalize(word)); }
int LocalLexicon::dictionarySize() const { return m_words.size(); }

}
