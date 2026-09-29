// SPDX-License-Identifier: GPL-3.0-or-later
//
// Loads the real Main.qml against the real KeyboardUiBridge (with a fake
// input-method backend) in an offscreen QQuickView. Guards UI hot-path
// behaviour that unit tests of the bridge alone cannot observe.

#include <QtTest/QTest>

#include <QGuiApplication>
#include <QPointer>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickView>
#include <QStandardPaths>

#include "app/keyboarduibridge.h"
#include "core/inputmethodbackend.h"
#include "core/keyboardcontroller.h"
#include "core/keyboardmodel.h"

namespace
{

class FakeBackend final : public V3Keyboard::InputMethodBackend
{
public:
    void commitText(const QString &text) override { commits.append(text); }
    void backspace() override { ++backspaces; }
    void deleteForward() override {}
    void moveLeft() override {}
    void moveRight() override {}
    void moveHome() override {}
    void moveEnd() override {}
    void enter() override {}

    QStringList commits;
    int backspaces = 0;
};

void collectLetterKeys(QQuickItem *item, QList<QPointer<QQuickItem>> &result)
{
    // Letter keys are the only items exposing a non-empty glideValue.
    const QVariant glide = item->property("glideValue");
    if (glide.isValid() && !glide.toString().isEmpty()) result.append(item);
    const auto children = item->childItems();
    for (QQuickItem *child : children) collectLetterKeys(child, result);
}

QList<QPointer<QQuickItem>> letterKeys(QQuickItem *root)
{
    QList<QPointer<QQuickItem>> result;
    collectLetterKeys(root, result);
    return result;
}

}

class QmlKeyboardTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
    }

    void caseChangesDoNotRecreateLetterKeys()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);

        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(V3KBD_MAIN_QML)));
        QCOMPARE(view.status(), QQuickView::Ready);

        const auto before = letterKeys(view.rootObject());
        QVERIFY2(before.size() >= 26, "expected the EN letter rows to be instantiated");

        // Sentence start -> first letter consumes auto-uppercase (case flip),
        // Shift toggles case again. None of this changes the layout.
        bridge.tapLetter(QStringLiteral("h"));
        bridge.shift();
        bridge.tapLetter(QStringLiteral("i"));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();

        for (const auto &key : before) {
            QVERIFY2(!key.isNull(), "a letter key was destroyed by a case-only change");
        }
        QCOMPARE(letterKeys(view.rootObject()).size(), before.size());
        QCOMPARE(backend.commits, QStringList({QStringLiteral("H"), QStringLiteral("I")}));

        // Sanity check of the probe itself: a real layout change must rebuild.
        bridge.setLanguage(QStringLiteral("de"));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();
        bool anyDestroyed = false;
        for (const auto &key : before) anyDestroyed = anyDestroyed || key.isNull();
        QVERIFY2(anyDestroyed, "probe could not observe delegate recreation");
    }
};

QTEST_MAIN(QmlKeyboardTest)
#include "tst_qmlkeyboard.moc"
