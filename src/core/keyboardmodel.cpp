// SPDX-License-Identifier: GPL-3.0-or-later

#include "keyboardmodel.h"

#include <array>

namespace V3Keyboard
{
namespace
{

struct LayoutDefinition
{
    const char *code;
    const char *label;
    const char *row1;
    const char *row2;
    const char *row3;
};

constexpr std::array<LayoutDefinition, 4> layouts = {{
    {"en", "English", "qwertyuiop", "asdfghjkl", "zxcvbnm"},
    {"de", "Deutsch", "qwertzuiopü", "asdfghjklöä", "yxcvbnm"},
    {"uk", "Українська", "йцукенгшщзхї", "фівапролджє", "ячсмитьбю"},
    {"ru", "Русский", "йцукенгшщзхъ", "фывапролджэ", "ячсмитьбю"},
}};

QStringList splitCharacters(const QString &text)
{
    QStringList result;
    result.reserve(text.size());

    for (const QChar ch : text) {
        result.append(QString(ch));
    }

    return result;
}

}


void KeyboardModel::pressShift()
{
    if (m_layer != Layer::Alphabet) {
        return;
    }

    switch (m_shiftState) {
    case ShiftState::Lowercase:
        m_shiftState = ShiftState::OneShot;
        break;
    case ShiftState::OneShot:
        m_shiftState = ShiftState::CapsLock;
        break;
    case ShiftState::CapsLock:
        m_shiftState = ShiftState::Lowercase;
        break;
    }
}

void KeyboardModel::consumeShiftAfterLetter()
{
    if (m_shiftState == ShiftState::OneShot) {
        m_shiftState = ShiftState::Lowercase;
    }
}

void KeyboardModel::toggleSymbols()
{
    m_layer = (m_layer == Layer::Alphabet)
        ? Layer::Symbols
        : Layer::Alphabet;

    resetShift();
}

bool KeyboardModel::uppercase() const
{
    return m_layer == Layer::Alphabet
        && m_shiftState != ShiftState::Lowercase;
}

bool KeyboardModel::capsLock() const
{
    return m_layer == Layer::Alphabet
        && m_shiftState == ShiftState::CapsLock;
}

bool KeyboardModel::symbolsActive() const
{
    return m_layer == Layer::Symbols;
}

QString KeyboardModel::textForLetter(const QString &text) const
{
    return uppercase() ? text.toUpper() : text.toLower();
}

QString KeyboardModel::alternateForKey(const QString &text) const
{
    if (m_layer != Layer::Alphabet) {
        return {};
    }

    const QString lower = text.toLower();
    const QString code = languageCode();

    if (code == QStringLiteral("de") && lower == QStringLiteral("s")) {
        return uppercase() ? QStringLiteral("ẞ") : QStringLiteral("ß");
    }
    if (code == QStringLiteral("uk") && lower == QStringLiteral("г")) {
        return uppercase() ? QStringLiteral("Ґ") : QStringLiteral("ґ");
    }
    if (code == QStringLiteral("ru") && lower == QStringLiteral("е")) {
        return uppercase() ? QStringLiteral("Ё") : QStringLiteral("ё");
    }

    return {};
}

QString KeyboardModel::languageCode() const
{
    return QString::fromUtf8(layouts.at(m_languageIndex).code);
}

QString KeyboardModel::languageLabel() const
{
    return QString::fromUtf8(layouts.at(m_languageIndex).label);
}

QStringList KeyboardModel::languageCodes() const
{
    QStringList result;
    result.reserve(static_cast<int>(layouts.size()));
    for (const auto &layout : layouts) {
        result.append(QString::fromUtf8(layout.code));
    }
    return result;
}

QStringList KeyboardModel::languageLabels() const
{
    QStringList result;
    result.reserve(static_cast<int>(layouts.size()));
    for (const auto &layout : layouts) {
        result.append(QString::fromUtf8(layout.label));
    }
    return result;
}

QStringList KeyboardModel::row1() const
{
    return splitCharacters(QString::fromUtf8(layouts.at(m_languageIndex).row1));
}

QStringList KeyboardModel::row2() const
{
    return splitCharacters(QString::fromUtf8(layouts.at(m_languageIndex).row2));
}

QStringList KeyboardModel::row3() const
{
    return splitCharacters(QString::fromUtf8(layouts.at(m_languageIndex).row3));
}

void KeyboardModel::setLanguage(const QString &code)
{
    for (int i = 0; i < static_cast<int>(layouts.size()); ++i) {
        if (code == QString::fromUtf8(layouts.at(i).code)) {
            if (m_languageIndex != i) {
                m_languageIndex = i;
                resetShift();
            }
            return;
        }
    }
}

void KeyboardModel::nextLanguage()
{
    m_languageIndex = (m_languageIndex + 1) % static_cast<int>(layouts.size());
    resetShift();
}

void KeyboardModel::resetShift()
{
    m_shiftState = ShiftState::Lowercase;
}


}
