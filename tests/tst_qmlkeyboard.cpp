// SPDX-License-Identifier: GPL-3.0-or-later
//
// Loads the real Main.qml against the real KeyboardUiBridge (with a fake
// input-method backend) in an offscreen QQuickView. Guards UI hot-path
// behaviour that unit tests of the bridge alone cannot observe.

#include <QtTest/QTest>

#include <QColor>
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
#include "app/voicecontroller.h"
#include "core/locallexicon.h"

#include <QPointingDevice>
#include <QStyleHints>
#include <QRegularExpression>
#include <QSettings>

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

QQuickItem *findText(QQuickItem *item, const QString &text)
{
    if (item->isVisible() && item->property("text").toString() == text) return item;
    const auto children = item->childItems();
    for (QQuickItem *child : children) {
        if (QQuickItem *found = findText(child, text)) return found;
    }
    return nullptr;
}

QQuickItem *findNamed(QQuickItem *item, const QString &name)
{
    if (item->objectName() == name && item->isVisible()) return item;
    const auto children = item->childItems();
    for (QQuickItem *child : children) {
        if (QQuickItem *found = findNamed(child, name)) return found;
    }
    return nullptr;
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
        QCoreApplication::setOrganizationName(QStringLiteral("V3KeyboardTests"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_qmlkeyboard"));
        V3Keyboard::LocalLexicon::setDictionarySearchPaths({QStringLiteral(V3KBD_TEST_DATA "/empty")});
        V3Keyboard::LocalLexicon::setFrequencySearchPaths({QStringLiteral(V3KBD_TEST_DATA "/empty")});
        V3Keyboard::LocalLexicon::setBlocklistSearchPaths({QStringLiteral(V3KBD_TEST_DATA "/empty")});
        V3Keyboard::LocalLexicon::setUserDictionaryFile(QStringLiteral(V3KBD_TEST_DATA "/empty/none.txt"));
    }

    void init()
    {
        QSettings().clear();
        // Script errors in handlers are silent on device; make them fatal here.
        QTest::failOnWarning(QRegularExpression(QStringLiteral("ReferenceError|TypeError|is not declared")));
    }

    void tappingSuggestionReplacesTheTypedWord()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);

        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(V3KBD_MAIN_QML)));
        QCOMPARE(view.status(), QQuickView::Ready);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));

        bridge.tapLetter(QStringLiteral("h"));
        bridge.tapLetter(QStringLiteral("e"));
        bridge.tapLetter(QStringLiteral("l"));
        // The strip follows the typed case ("Hel" at sentence start).
        QTRY_VERIFY(findText(view.rootObject(), QStringLiteral("Hello")));

        QQuickItem *label = findText(view.rootObject(), QStringLiteral("Hello"));
        const QPointF centre = label->mapToScene(QPointF(label->width() / 2, label->height() / 2));
        QTest::mouseClick(&view, Qt::LeftButton, {}, centre.toPoint());

        QTRY_COMPARE(backend.commits.size(), 5);
        QCOMPARE(backend.backspaces, 3);
        QCOMPARE(backend.commits.mid(3), QStringList({QStringLiteral("Hello"), QStringLiteral(" ")}));
    }

    void tappingARealKeyCommitsTheLetter()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);

        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(V3KBD_MAIN_QML)));
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));

        // Press/move/release exercise every Key handler (glide, drag, popup).
        QQuickItem *key = findText(view.rootObject(), QStringLiteral("Q"));
        QVERIFY(key);
        const QPoint centre = key->mapToScene(QPointF(key->width() / 2, key->height() / 2)).toPoint();
        QTest::mousePress(&view, Qt::LeftButton, {}, centre);
        QTest::mouseMove(&view, centre + QPoint(1, 0));
        QTest::mouseRelease(&view, Qt::LeftButton, {}, centre + QPoint(1, 0));
        QTRY_COMPARE(backend.commits, QStringList({QStringLiteral("Q")}));
    }

    void overlappingTwoThumbTapsAreBothCommitted()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);
        bridge.setAutoCapitalizationEnabled(false);

        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(V3KBD_MAIN_QML)));
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));

        auto centreOf = [&](const QString &label) {
            QQuickItem *key = findText(view.rootObject(), label);
            return key ? key->mapToScene(QPointF(key->width() / 2, key->height() / 2)).toPoint() : QPoint(-1, -1);
        };
        const QPoint left = centreOf(QStringLiteral("a"));
        const QPoint right = centreOf(QStringLiteral("l"));
        QVERIFY(left.x() >= 0 && right.x() >= 0);

        static QPointingDevice *touch = QTest::createTouchDevice();
        // Fast two-thumb typing: the right thumb lands before the left lifts.
        QTest::touchEvent(&view, touch).press(0, left);
        QTest::touchEvent(&view, touch).stationary(0).press(1, right);
        QTest::touchEvent(&view, touch).release(0, left).stationary(1);
        QTest::touchEvent(&view, touch).release(1, right);

        QTRY_COMPARE(backend.commits, QStringList({QStringLiteral("a"), QStringLiteral("l")}));
    }

    void singleFingerGlideAndLongPressStillWork()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);
        bridge.setAutoCapitalizationEnabled(false);

        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(V3KBD_MAIN_QML)));
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        auto centreOf = [&](const QString &label) {
            QQuickItem *key = findText(view.rootObject(), label);
            return key ? key->mapToScene(QPointF(key->width() / 2, key->height() / 2)).toPoint() : QPoint(-1, -1);
        };

        static QPointingDevice *touch = QTest::createTouchDevice();
        // Glide h-e-l-o with one finger.
        const QList<QPoint> path = {centreOf(QStringLiteral("h")), centreOf(QStringLiteral("e")),
                                    centreOf(QStringLiteral("l")), centreOf(QStringLiteral("o"))};
        QTest::touchEvent(&view, touch).press(0, path.first());
        for (int i = 1; i < path.size(); ++i) {
            const QPoint from = path.at(i - 1);
            const QPoint to = path.at(i);
            for (int step = 1; step <= 6; ++step) {
                QTest::touchEvent(&view, touch).move(0, from + (to - from) * step / 6);
            }
        }
        QQuickItem *trail = findNamed(view.rootObject(), QStringLiteral("glideTrail"));
        QVERIFY2(trail && trail->isVisible(), "gesture trail is drawn while gliding");
        QTest::touchEvent(&view, touch).release(0, path.last());
        QTRY_COMPARE(backend.commits.value(0), QStringLiteral("hello"));
        QTRY_VERIFY(!findNamed(view.rootObject(), QStringLiteral("glideTrail")));   // hidden after

        // Long press on a key with an alternate commits the alternate (DE s -> ß).
        bridge.setLanguage(QStringLiteral("de"));
        // Let the old EN key delegates finish deleting so lookups hit DE keys.
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();
        QTRY_VERIFY(findText(view.rootObject(), QStringLiteral("ä")));
        const QString alternate = bridge.alternateForKey(QStringLiteral("s"));
        QCOMPARE(alternate, QStringLiteral("ß"));
        backend.commits.clear();
        const QPoint q = centreOf(QStringLiteral("s"));
        QTest::touchEvent(&view, touch).press(0, q);
        QTest::qWait(qApp->styleHints()->mousePressAndHoldInterval() + 200);
        QTest::touchEvent(&view, touch).release(0, q);
        QTRY_COMPARE(backend.commits, QStringList({alternate}));
    }

    void compactModeDocksTheKeysLeftOrRight()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);
        bridge.setAutoCapitalizationEnabled(false);          // lowercase labels

        QQuickView view;
        view.setResizeMode(QQuickView::SizeRootObjectToView);
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(V3KBD_MAIN_QML)));
        view.resize(1600, 520);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        auto keyX = [&](const QString &label) {
            QQuickItem *key = findText(view.rootObject(), label);
            return key ? key->mapToScene(QPointF(key->width() / 2, 0)).x() : -1.0;
        };
        QTRY_VERIFY(keyX(QStringLiteral("q")) > 0 && keyX(QStringLiteral("p")) > 0);
        const qreal fullSpan = keyX(QStringLiteral("p")) - keyX(QStringLiteral("q"));
        QVERIFY(fullSpan > 300);

        QCOMPARE(bridge.layoutMode(), QStringLiteral("full"));
        bridge.setLayoutMode(QStringLiteral("left"));
        QTRY_VERIFY(keyX(QStringLiteral("p")) < view.width() / 2 + 200);
        QVERIFY(keyX(QStringLiteral("p")) - keyX(QStringLiteral("q")) < fullSpan * 0.8);
        bridge.setLayoutMode(QStringLiteral("right"));
        QTRY_VERIFY(keyX(QStringLiteral("q")) > view.width() / 2 - 200);
        bridge.setLayoutMode(QStringLiteral("bogus"));        // ignored
        QCOMPARE(bridge.layoutMode(), QStringLiteral("right"));

        // Persisted like the other preferences.
        V3Keyboard::KeyboardUiBridge reloaded(controller, model);
        QCOMPARE(reloaded.layoutMode(), QStringLiteral("right"));
    }

    void micButtonAppearsOnlyWithVoiceAndStartsRecording()
    {
        struct Recorder final : V3Keyboard::AudioRecorder {
            bool start(QString *) override { started = true; return true; }
            std::vector<float> stop() override { return {}; }
            bool started = false;
        } recorder;
        struct Recognizer final : V3Keyboard::SpeechRecognizer {
            QString unavailableReason() const override { return {}; }
            void recognize(std::vector<float>, const QString &, std::function<void(QString, QString)>) override {}
        } recognizer;

        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(V3KBD_MAIN_QML)));
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        QVERIFY(!findText(view.rootObject(), QStringLiteral("🎤")));      // not built in

        V3Keyboard::VoiceController voice(&recorder, &recognizer);
        bridge.setVoiceController(&voice);
        QTRY_VERIFY(findText(view.rootObject(), QStringLiteral("🎤")));
        QTest::qWait(100);   // let the toolbar Row re-position its children (polish)
        QQuickItem *mic = findText(view.rootObject(), QStringLiteral("🎤"));
        QTest::mouseClick(&view, Qt::LeftButton, {}, mic->mapToScene(QPointF(mic->width() / 2, mic->height() / 2)).toPoint());
        QTRY_VERIFY(recorder.started);
        QCOMPARE(bridge.voiceState(), QStringLiteral("recording"));
        QTRY_VERIFY(findText(view.rootObject(), QStringLiteral("Listening… tap 🎤 to finish")));
    }

    void gboardLongPressPickerNumberRowHintsAndForget()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);
        bridge.setAutoCapitalizationEnabled(false);
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(V3KBD_MAIN_QML)));
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        auto centre = [&](QQuickItem *item) { return item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint(); };
        static QPointingDevice *touch = QTest::createTouchDevice();

        // 1. Long-press "e", slide one cell right, release -> second choice.
        QQuickItem *eLabel = findText(view.rootObject(), QStringLiteral("e"));
        QVERIFY(eLabel);
        const QPoint e = centre(eLabel);
        QTest::touchEvent(&view, touch).press(0, e);
        QTest::qWait(qApp->styleHints()->mousePressAndHoldInterval() + 150);
        QTRY_VERIFY(findText(view.rootObject(), QStringLiteral("è")));        // picker open
        const int cell = int(eLabel->parentItem()->width() * 0.86) + 2;
        QTest::touchEvent(&view, touch).move(0, e + QPoint(cell, 0));
        QTest::touchEvent(&view, touch).release(0, e + QPoint(cell, 0));
        QTRY_COMPARE(backend.commits, QStringList({QStringLiteral("è")}));

        // 2. Symbol hints in key corners can be switched off.
        QVERIFY(findText(view.rootObject(), QStringLiteral("1")));            // hint on "q"
        bridge.setSymbolHints(false);
        QTRY_VERIFY(!findText(view.rootObject(), QStringLiteral("1")));

        // 3. Number row grows the panel and types digits.
        const qreal before = view.rootObject()->height();
        bridge.setNumberRow(true);
        QTRY_VERIFY(findNamed(view.rootObject(), QStringLiteral("numberKey_7")));
        QVERIFY(view.rootObject()->height() > before);
        QTest::qWait(100);                                                    // Row polish
        QTest::mouseClick(&view, Qt::LeftButton, {}, centre(findNamed(view.rootObject(), QStringLiteral("numberKey_7"))));
        QTRY_COMPARE(backend.commits.last(), QStringLiteral("7"));

        // 4. Long-press a suggestion removes it (and does not insert it).
        bridge.setNumberRow(false);
        bridge.space();
        for (const QChar ch : QStringLiteral("hel")) bridge.tapLetter(QString(ch));
        QTRY_VERIFY(findText(view.rootObject(), QStringLiteral("hello")));
        QTest::qWait(100);
        const int commitsBefore = backend.commits.size();
        const QPoint h = centre(findText(view.rootObject(), QStringLiteral("hello")));
        QTest::mousePress(&view, Qt::LeftButton, {}, h);
        QTest::qWait(qApp->styleHints()->mousePressAndHoldInterval() + 200);
        QTest::mouseRelease(&view, Qt::LeftButton, {}, h);
        QTRY_VERIFY(!bridge.suggestions().contains(QStringLiteral("hello")));
        QCOMPARE(backend.commits.size(), commitsBefore);
    }

    void stripQuotesTypedWordAndBoldsTheCorrection()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);
        bridge.setAutoCapitalizationEnabled(false);
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(V3KBD_MAIN_QML)));
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        for (const QChar ch : QStringLiteral("teh")) bridge.tapLetter(QString(ch));
        QTRY_VERIFY(findText(view.rootObject(), QStringLiteral("\u201Cteh\u201D")));
        QQuickItem *correction = findText(view.rootObject(), QStringLiteral("the"));
        QVERIFY(correction);
        QCOMPARE(correction->property("font").value<QFont>().weight(), QFont::Bold);
    }

    void numberFieldsShowANumpadEmailFieldsAnAt()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(V3KBD_MAIN_QML)));
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        auto centre = [&](QQuickItem *item) { return item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint(); };

        bridge.setContentType(0, 4);                                  // phone
        QTRY_VERIFY(findNamed(view.rootObject(), QStringLiteral("numpadKey_5")));
        QVERIFY(findNamed(view.rootObject(), QStringLiteral("numpadKey_+")));
        QVERIFY(!findText(view.rootObject(), QStringLiteral("Q")));    // letters hidden
        QTest::qWait(100);
        QTest::mouseClick(&view, Qt::LeftButton, {}, centre(findNamed(view.rootObject(), QStringLiteral("numpadKey_5"))));
        QTRY_COMPARE(backend.commits, QStringList({QStringLiteral("5")}));

        bridge.setContentType(0, 6);                                  // e-mail
        QTRY_VERIFY(findText(view.rootObject(), QStringLiteral("@")));
        QVERIFY(!findNamed(view.rootObject(), QStringLiteral("numpadKey_5")));
        QVERIFY(findText(view.rootObject(), QStringLiteral("q")));     // letters back (no auto-cap)
    }

    void gboardSlideGesturesAndEmojiRow()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);
        bridge.setAutoCapitalizationEnabled(false);
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(V3KBD_MAIN_QML)));
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        auto centre = [&](QQuickItem *item) { return item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint(); };
        static QPointingDevice *touch = QTest::createTouchDevice();
        auto slide = [&](const QPoint &from, const QPoint &to) {
            QTest::touchEvent(&view, touch).press(0, from);
            for (int i = 1; i <= 8; ++i) {
                QTest::touchEvent(&view, touch).move(0, from + (to - from) * i / 8);
                QTest::qWait(10);
            }
            QTest::touchEvent(&view, touch).release(0, to);
        };

        // 1. Shift slide onto "d" -> one capital, then lowercase again.
        const QPoint d = centre(findText(view.rootObject(), QStringLiteral("d")));
        slide(centre(findNamed(view.rootObject(), QStringLiteral("shiftKey"))), d);
        QTRY_COMPARE(backend.commits, QStringList({QStringLiteral("D")}));
        QTRY_VERIFY(!bridge.uppercase());

        // 2. ?123 slide onto the key where "5" appears (same place as "t").
        const QPoint t = centre(findText(view.rootObject(), QStringLiteral("t")));
        slide(centre(findNamed(view.rootObject(), QStringLiteral("symbolsKey"))), t);
        QTRY_COMPARE(backend.commits.last(), QStringLiteral("5"));
        QTRY_VERIFY(!bridge.symbolsActive());

        // 3. Emoji fast-access row.
        bridge.insertEmoji(QStringLiteral("😀"));
        bridge.setEmojiRow(true);
        QTRY_VERIFY(findNamed(view.rootObject(), QStringLiteral("emojiRowKey_😀")));
        QTest::qWait(100);
        backend.commits.clear();                 // insertEmoji above committed one already
        QTest::mouseClick(&view, Qt::LeftButton, {}, centre(findNamed(view.rootObject(), QStringLiteral("emojiRowKey_😀"))));
        QTRY_COMPARE(backend.commits, QStringList({QStringLiteral("😀")}));
    }

    void lightDarkAndAmoledPalettes()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(V3KBD_MAIN_QML)));
        QCOMPARE(view.status(), QQuickView::Ready);
        auto colour = [&](const char *name) { return view.rootObject()->property(name).value<QColor>(); };
        bridge.setTheme(QStringLiteral("light"));
        QCOMPARE(colour("backgroundColor"), QColor(QStringLiteral("#e8eaed")));
        QCOMPARE(colour("keyColor"), QColor(QStringLiteral("#ffffff")));
        QCOMPARE(colour("textColor"), QColor(QStringLiteral("#202124")));
        bridge.setTheme(QStringLiteral("dark"));
        QCOMPARE(colour("backgroundColor"), QColor(QStringLiteral("#202124")));
        bridge.setTheme(QStringLiteral("amoled"));
        QCOMPARE(colour("backgroundColor"), QColor(QStringLiteral("#000000")));
        QCOMPARE(colour("textColor"), QColor(QStringLiteral("#f1f3f4")));

        // Settings show version and data attributions.
        bridge.activateToolbarAction(QStringLiteral("settings"));
        QTRY_VERIFY(findNamed(view.rootObject(), QStringLiteral("aboutText")));
        const QString about = findNamed(view.rootObject(), QStringLiteral("aboutText"))->property("text").toString();
        QVERIFY(about.contains(bridge.version()));
        QVERIFY(about.contains(QStringLiteral("CC BY-SA 4.0")));
    }

    void savingAndRemovingPersonalWordsThroughTheUi()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);
        bridge.setAutoCapitalizationEnabled(false);
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(V3KBD_MAIN_QML)));
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        auto centre = [&](QQuickItem *item) { return item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint(); };

        for (const QChar ch : QStringLiteral("zorgle")) bridge.tapLetter(QString(ch));
        QTRY_VERIFY(findText(view.rootObject(), QStringLiteral("\u201Czorgle\u201D")));
        QTest::qWait(100);
        QTest::mouseClick(&view, Qt::LeftButton, {}, centre(findText(view.rootObject(), QStringLiteral("\u201Czorgle\u201D"))));
        QTRY_VERIFY(findNamed(view.rootObject(), QStringLiteral("saveWordChip")));
        QTest::qWait(50);
        QTest::mouseClick(&view, Qt::LeftButton, {}, centre(findNamed(view.rootObject(), QStringLiteral("saveWordChip"))));
        QTRY_COMPARE(bridge.userWords(), QStringList({QStringLiteral("zorgle")}));

        bridge.activateToolbarAction(QStringLiteral("settings"));
        QTRY_VERIFY(findNamed(view.rootObject(), QStringLiteral("userWord_zorgle")));
        QTest::qWait(100);
        QQuickItem *chip = findNamed(view.rootObject(), QStringLiteral("userWord_zorgle"));
        // The section sits low in the settings: scroll there like a user would.
        QQuickItem *flick = chip->parentItem();
        while (flick && !flick->property("contentY").isValid()) flick = flick->parentItem();
        QVERIFY(flick);
        const QPointF inContent = chip->mapToItem(flick->property("contentItem").value<QQuickItem *>(), QPointF(0, 0));
        flick->setProperty("contentY", qMax(0.0, inContent.y() - 20));
        QTest::qWait(100);
        const QPoint at = centre(chip);
        QVERIFY2(at.y() > 0 && at.y() < view.height(), "chip scrolled into view");
        QTest::mouseClick(&view, Qt::LeftButton, {}, at);
        QTRY_VERIFY(bridge.userWords().isEmpty());
    }

    void topRowKeyPreviewStaysInsideThePanel()
    {
        // Live report: the preview of top-row keys was cut off — Wayland cannot
        // draw above the panel surface, so the whole bubble (incl. its rounded
        // top) must stay inside, for any panel size.
        for (const QSize size : {QSize(1600, 560), QSize(1700, 420), QSize(1280, 360)}) {
            FakeBackend backend;
            V3Keyboard::KeyboardController controller(backend);
            V3Keyboard::KeyboardModel model;
            V3Keyboard::KeyboardUiBridge bridge(controller, model);
            bridge.setAutoCapitalizationEnabled(false);
            QQuickView view;
            view.setResizeMode(QQuickView::SizeRootObjectToView);
            view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
            view.setSource(QUrl::fromLocalFile(QStringLiteral(V3KBD_MAIN_QML)));
            view.resize(size);
            view.show();
            QVERIFY(QTest::qWaitForWindowExposed(&view));
            QTest::qWait(50);
            QQuickItem *label = findText(view.rootObject(), QStringLiteral("q"));
            QVERIFY(label);
            const QPoint at = label->mapToScene(QPointF(label->width() / 2, label->height() / 2)).toPoint();
            static QPointingDevice *touch = QTest::createTouchDevice();
            QTest::touchEvent(&view, touch).press(0, at);
            QTest::qWait(80);
            QQuickItem *preview = findNamed(view.rootObject(), QStringLiteral("keyPreview"));
            QVERIFY2(preview, "a key preview is shown while pressing");
            const QRectF box = preview->mapRectToScene(QRectF(0, 0, preview->width(), preview->height()));
            QTest::touchEvent(&view, touch).release(0, at);
            QVERIFY2(box.top() >= 2.0, qPrintable(QStringLiteral("%1x%2: preview top %3 touches/leaves the panel edge")
                                                    .arg(size.width()).arg(size.height()).arg(box.top())));
        }
    }

    void holdingBackspaceKeepsDeletingUntilRelease()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(V3KBD_MAIN_QML)));
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        QQuickItem *key = findNamed(view.rootObject(), QStringLiteral("backspaceKey"));
        QVERIFY(key);
        const QPoint at = key->mapToScene(QPointF(key->width() / 2, key->height() / 2)).toPoint();
        static QPointingDevice *touch = QTest::createTouchDevice();

        QTest::touchEvent(&view, touch).press(0, at);
        QTest::qWait(qApp->styleHints()->mousePressAndHoldInterval() + 800);
        QTest::touchEvent(&view, touch).release(0, at);
        const int whileHeld = backend.backspaces;
        QVERIFY2(whileHeld >= 6, qPrintable(QStringLiteral("only %1 deletions while held").arg(whileHeld)));
        QTest::qWait(300);
        QCOMPARE(backend.backspaces, whileHeld);                 // stops on release
    }

    void touchPointInsideTheKeyReachesCorrection()
    {
        FakeBackend backend;
        V3Keyboard::KeyboardController controller(backend);
        V3Keyboard::KeyboardModel model;
        V3Keyboard::KeyboardUiBridge bridge(controller, model);
        bridge.setAutoCapitalizationEnabled(false);
        bridge.addWordToDictionary(QStringLiteral("bat"));
        bridge.addWordToDictionary(QStringLiteral("bet"));
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(V3KBD_MAIN_QML)));
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        QTest::qWait(100);
        auto keyOf = [&](const QString &label) { return findText(view.rootObject(), label)->parentItem(); };
        auto tapAt = [&](QQuickItem *key, qreal fx, qreal fy) {
            QTest::mouseClick(&view, Qt::LeftButton, {}, key->mapToScene(QPointF(key->width() * fx, key->height() * fy)).toPoint());
            QTest::qWait(30);
        };
        auto typeBst = [&](qreal sx, qreal sy) {
            tapAt(keyOf(QStringLiteral("b")), 0.5, 0.5);
            tapAt(keyOf(QStringLiteral("s")), sx, sy);
            tapAt(keyOf(QStringLiteral("t")), 0.5, 0.5);
        };
        // The literal "bst" stays first (kept/saveable); corrections follow.
        auto before = [&](const QString &a, const QString &b) {
            const QStringList s = bridge.suggestions();
            return s.value(0) == QStringLiteral("bst") && s.contains(a) && (!s.contains(b) || s.indexOf(a) < s.indexOf(b));
        };
        typeBst(0.92, 0.10);                               // top-right edge of s -> e side
        QTRY_VERIFY2(before(QStringLiteral("bet"), QStringLiteral("bat")), qPrintable(bridge.suggestions().join(',')));
        for (int i = 0; i < 3; ++i) bridge.backspace();
        typeBst(0.08, 0.55);                               // left edge of s -> a side
        QTRY_VERIFY2(before(QStringLiteral("bat"), QStringLiteral("bet")), qPrintable(bridge.suggestions().join(',')));
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
