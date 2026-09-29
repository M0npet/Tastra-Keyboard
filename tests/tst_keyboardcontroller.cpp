// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest/QTest>

#include "core/inputmethodbackend.h"
#include "core/keyboardcontroller.h"

class FakeInputMethodBackend final : public V3Keyboard::InputMethodBackend
{
public:
    void commitText(const QString &text) override
    {
        commits.append(text);
    }

    void backspace() override
    {
        ++backspaceCount;
    }

    void deleteForward() override
    {
        ++deleteForwardCount;
    }

    void moveLeft() override
    {
        ++moveLeftCount;
    }

    void moveRight() override
    {
        ++moveRightCount;
    }

    void moveHome() override
    {
        ++moveHomeCount;
    }

    void moveEnd() override
    {
        ++moveEndCount;
    }

    void enter() override
    {
        ++enterCount;
    }

    QStringList commits;
    int backspaceCount = 0;
    int deleteForwardCount = 0;
    int moveLeftCount = 0;
    int moveRightCount = 0;
    int moveHomeCount = 0;
    int moveEndCount = 0;
    int enterCount = 0;
};

class KeyboardControllerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void tappingTextKeyCommitsExactlyOnce()
    {
        FakeInputMethodBackend backend;
        V3Keyboard::KeyboardController controller(backend);

        controller.tapText(QStringLiteral("a"));

        QCOMPARE(backend.commits, QStringList{QStringLiteral("a")});
    }
};

QTEST_APPLESS_MAIN(KeyboardControllerTest)

#include "tst_keyboardcontroller.moc"
