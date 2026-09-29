// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

#include "app/keyboarduibridge.h"
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

class KeyboardUiBridgeTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void tapTextReachesController()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);

        bridge.tapText(QStringLiteral("a"));

        QCOMPARE(backend.commits, QStringList{QStringLiteral("a")});
    }

    void editingCommandsReachController()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);

        bridge.moveLeft();
        bridge.moveRight();
        bridge.moveHome();
        bridge.moveEnd();
        bridge.deleteForward();

        QCOMPARE(backend.moveLeftCount, 1);
        QCOMPARE(backend.moveRightCount, 1);
        QCOMPARE(backend.moveHomeCount, 1);
        QCOMPARE(backend.moveEndCount, 1);
        QCOMPARE(backend.deleteForwardCount, 1);
    }


    void ordinaryTypingDoesNotInvalidateWholeKeyboard()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);

        // Consume the initial sentence-start capitalization with the first key.
        bridge.tapLetter(QStringLiteral("h"));

        QSignalSpy keyboardSpy(&bridge, &V3Keyboard::KeyboardUiBridge::keyboardStateChanged);
        QSignalSpy suggestionSpy(&bridge, &V3Keyboard::KeyboardUiBridge::suggestionsChanged);

        bridge.tapLetter(QStringLiteral("e"));

        QCOMPARE(keyboardSpy.count(), 0);
        QCOMPARE(suggestionSpy.count(), 1);
    }
};

QTEST_APPLESS_MAIN(KeyboardUiBridgeTest)

#include "tst_keyboarduibridge.moc"
