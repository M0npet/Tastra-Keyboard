// SPDX-License-Identifier: GPL-3.0-or-later

#include "uitranslator.h"

#include <QFile>

namespace Tastra
{

QStringList UiTranslator::translatedLanguages()
{
    return {QStringLiteral("de"), QStringLiteral("ru"), QStringLiteral("uk")};
}

QString UiTranslator::resolve(const QString &setting, const QLocale &system)
{
    if (setting == QStringLiteral("en") || translatedLanguages().contains(setting)) return setting;
    for (const QString &name : system.uiLanguages()) {
        const QString code = name.left(2).toLower();
        if (code == QStringLiteral("en")) return code;
        if (translatedLanguages().contains(code)) return code;
    }
    return QStringLiteral("en");
}

bool UiTranslator::loadLanguage(const QString &code)
{
    m_texts.clear();
    m_language = QStringLiteral("en");
    if (!translatedLanguages().contains(code)) return code == QStringLiteral("en");
    QFile file(QStringLiteral(":/tastra/i18n/%1.tsv").arg(code));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    const QString text = QString::fromUtf8(file.readAll());
    for (const QStringView line : QStringView(text).split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        if (line.startsWith(QLatin1Char('#'))) continue;
        const qsizetype tab = line.indexOf(QLatin1Char('\t'));
        if (tab <= 0) continue;
        m_texts.insert(line.left(tab).toString(), line.mid(tab + 1).toString());
    }
    m_language = code;
    return !m_texts.isEmpty();
}

QString UiTranslator::translate(const char *, const char *sourceText, const char *, int) const
{
    if (!sourceText || m_texts.isEmpty()) return {};
    return m_texts.value(QString::fromUtf8(sourceText));
}

}
