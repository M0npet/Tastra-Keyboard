// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QHash>
#include <QLocale>
#include <QString>
#include <QStringList>
#include <QTranslator>

namespace Tastra
{

// The keyboard's own interface texts in German, Russian or Ukrainian, read
// from ":/tastra/i18n/<code>.tsv" ("English text<TAB>translation" per line).
// A plain table instead of Qt Linguist's .qm files keeps the build free of
// Qt's translation tools; qsTr() and QCoreApplication::translate() find the
// texts through the installed translator as usual.
class UiTranslator : public QTranslator
{
public:
    using QTranslator::QTranslator;
    // "en" (or an unknown code) empties the table: the English texts show.
    bool loadLanguage(const QString &code);
    QString language() const { return m_language; }

    QString translate(const char *context, const char *sourceText, const char *disambiguation = nullptr,
                      int n = -1) const override;
    bool isEmpty() const override { return m_texts.isEmpty(); }

    // Interface languages besides English.
    static QStringList translatedLanguages();
    // "system" -> the first translated language among the system's UI
    // languages, else "en"; a language code -> itself if known.
    static QString resolve(const QString &setting, const QLocale &system = QLocale::system());

private:
    QHash<QString, QString> m_texts;
    QString m_language = QStringLiteral("en");
};

}
