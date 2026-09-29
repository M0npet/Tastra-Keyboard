// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest/QTest>

#include "core/inputmethodbackend.h"
#include "core/keyboardcontroller.h"

class FakeBackend final : public V3Keyboard::InputMethodBackend
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

class SpecialKeysTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void controllerForwardsBackspace()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);

        controller.backspace();

        QCOMPARE(backend.backspaceCount, 1);
        QCOMPARE(backend.enterCount, 0);
    }

    void controllerForwardsEditingKeys()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);

        controller.moveLeft();
        controller.moveRight();
        controller.moveHome();
        controller.moveEnd();
        controller.deleteForward();
        controller.backspace();

        QCOMPARE(backend.moveLeftCount, 1);
        QCOMPARE(backend.moveRightCount, 1);
        QCOMPARE(backend.moveHomeCount, 1);
        QCOMPARE(backend.moveEndCount, 1);
        QCOMPARE(backend.deleteForwardCount, 1);
        QCOMPARE(backend.backspaceCount, 1);
    }

    void controllerForwardsEnter()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);

        controller.enter();

        QCOMPARE(backend.enterCount, 1);
        QCOMPARE(backend.backspaceCount, 0);
    }
};

QTEST_APPLESS_MAIN(SpecialKeysTest)

#include "tst_specialkeys.moc"
