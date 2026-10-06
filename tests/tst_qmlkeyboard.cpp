// SPDX-License-Identifier: GPL-3.0-or-later
//
// Loads the real Main.qml against the real KeyboardUiBridge (with a fake
// input-method backend) in an offscreen QQuickView. Guards UI hot-path
// behaviour that unit tests of the bridge alone cannot observe.

#include <QtTest/QTest>

#include <QColor>
#include <QClipboard>
#include <QSignalSpy>
#include <QGuiApplication>
#include <QPointer>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickView>
#include <QStandardPaths>

#include "app/keyboarduibridge.h"
#include "core/emojicatalog.h"
#include "app/keyboardhider.h"
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

class FakeBackend final : public Tastra::InputMethodBackend
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
        QCoreApplication::setOrganizationName(QStringLiteral("TastraTests"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_qmlkeyboard"));
        Tastra::LocalLexicon::setDictionarySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
        Tastra::LocalLexicon::setFrequencySearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
        Tastra::LocalLexicon::setBlocklistSearchPaths({QStringLiteral(TASTRA_TEST_DATA "/empty")});
        Tastra::LocalLexicon::setUserDictionaryFile(QStringLiteral(TASTRA_TEST_DATA "/empty/none.txt"));
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
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);

        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
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
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);

        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
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
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.setAutoCapitalizationEnabled(false);

        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
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
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.setAutoCapitalizationEnabled(false);

        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
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

        // Settings: Gesture trail off -> the same glide works, without a trail.
        bridge.setGlideTrail(false);
        const int commitsBefore = backend.commits.size();
        QTest::touchEvent(&view, touch).press(0, path.first());
        for (int i = 1; i < path.size(); ++i) {
            const QPoint from = path.at(i - 1);
            const QPoint to = path.at(i);
            for (int step = 1; step <= 6; ++step) {
                QTest::touchEvent(&view, touch).move(0, from + (to - from) * step / 6);
            }
        }
        QVERIFY(!findNamed(view.rootObject(), QStringLiteral("glideTrail")));
        QTest::touchEvent(&view, touch).release(0, path.last());
        QTRY_VERIFY2(backend.commits.size() > commitsBefore, "the second glide really happened");
        bridge.setGlideTrail(true);

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
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.setAutoCapitalizationEnabled(false);          // lowercase labels

        QQuickView view;
        view.setResizeMode(QQuickView::SizeRootObjectToView);
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
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
        Tastra::KeyboardUiBridge reloaded(controller, model);
        QCOMPARE(reloaded.layoutMode(), QStringLiteral("right"));
    }

    void micButtonAppearsOnlyWithVoiceAndStartsRecording()
    {
        struct Recorder final : Tastra::AudioRecorder {
            bool start(QString *) override { started = true; return true; }
            std::vector<float> stop() override { return {}; }
            bool started = false;
        } recorder;
        struct Recognizer final : Tastra::SpeechRecognizer {
            QString unavailableReason() const override { return {}; }
            void recognize(std::vector<float>, const QString &, std::function<void(QString, QString)>) override {}
        } recognizer;

        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        QVERIFY(!findText(view.rootObject(), QStringLiteral("🎤")));      // not built in

        Tastra::VoiceController voice(&recorder, &recognizer);
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
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.setAutoCapitalizationEnabled(false);
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
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
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.setAutoCapitalizationEnabled(false);
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
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
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
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
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.setAutoCapitalizationEnabled(false);
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
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
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
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
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.setAutoCapitalizationEnabled(false);
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
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
            Tastra::KeyboardController controller(backend);
            Tastra::KeyboardModel model;
            Tastra::KeyboardUiBridge bridge(controller, model);
            bridge.setAutoCapitalizationEnabled(false);
            QQuickView view;
            view.setResizeMode(QQuickView::SizeRootObjectToView);
            view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
            view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
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
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
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
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.setAutoCapitalizationEnabled(false);
        bridge.addWordToDictionary(QStringLiteral("bat"));
        bridge.addWordToDictionary(QStringLiteral("bet"));
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
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

    void hideButtonHidesTheKeyboardLikeGboard()
    {
        struct FakeHider final : Tastra::KeyboardHider {
            int calls = 0;
            void hideKeyboard() override { ++calls; }
        } hider;
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.setKeyboardHider(&hider);
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        QQuickItem *button = findNamed(view.rootObject(), QStringLiteral("hideKeyboardButton"));
        QVERIFY(button);
        QTest::qWait(50);
        QTest::mouseClick(&view, Qt::LeftButton, {}, button->mapToScene(QPointF(button->width() / 2, button->height() / 2)).toPoint());
        QTRY_COMPARE(hider.calls, 1);

        // Without a hider (not on Plasma) there is no button.
        Tastra::KeyboardUiBridge plain(controller, model);
        QQuickView other;
        other.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &plain);
        other.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
        QVERIFY(!findNamed(other.rootObject(), QStringLiteral("hideKeyboardButton")));
    }

    void clipboardItemsArePinnedWithALongPress()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.clearClipboardHistory();
        QGuiApplication::clipboard()->setText(QStringLiteral("keep me"));   // bridge listens to dataChanged
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        bridge.activateToolbarAction(QStringLiteral("clipboard"));
        QTRY_VERIFY(findNamed(view.rootObject(), QStringLiteral("clipboardItem")));
        QTest::qWait(100);
        QQuickItem *item = findNamed(view.rootObject(), QStringLiteral("clipboardItem"));   // the list entry
        const QPoint at = item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint();
        QSignalSpy changed(&bridge, &Tastra::KeyboardUiBridge::clipboardChanged);
        QTest::mousePress(&view, Qt::LeftButton, {}, at);
        QTest::qWait(qApp->styleHints()->mousePressAndHoldInterval() + 200);
        QTest::mouseRelease(&view, Qt::LeftButton, {}, at);
        QTRY_VERIFY(bridge.isClipboardPinned(QStringLiteral("keep me")));
        QTRY_VERIFY(findText(view.rootObject(), QStringLiteral("\U0001F4CC keep me")));
        QVERIFY(backend.commits.isEmpty());                    // a long press does not paste

        // A tap on a non-interactive part of the open panel (the header that
        // shows the current clipboard; a key lies underneath it) must not
        // reach that key — first live symptom: "&" typed through the panel.
        QQuickItem *header = findText(view.rootObject(), QStringLiteral("keep me"));   // list shows "📌 keep me" now
        QVERIFY(header);
        QTest::mouseClick(&view, Qt::LeftButton, {}, header->mapToScene(QPointF(header->width() / 2, header->height() / 2)).toPoint());
        QTest::qWait(50);
        QVERIFY2(backend.commits.isEmpty(), qPrintable(backend.commits.join(',')));
    }

    void settingsAreGroupedLikeGboard()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        bridge.activateToolbarAction(QStringLiteral("settings"));
        const QStringList sections = {QStringLiteral("Languages"), QStringLiteral("Preferences"), QStringLiteral("Theme"), QStringLiteral("Text correction"),
                                      QStringLiteral("Glide typing"), QStringLiteral("Clipboard"), QStringLiteral("Dictionary"),
                                      QStringLiteral("Advanced")};
        QTRY_VERIFY(findNamed(view.rootObject(), QStringLiteral("settingsSection_Languages")));
        auto yOf = [&](QQuickItem *item) { return item->mapToScene(QPointF(0, 0)).y(); };
        QList<qreal> sectionY;
        for (const QString &name : sections) {
            QQuickItem *header = findNamed(view.rootObject(), QStringLiteral("settingsSection_") + name);
            QVERIFY2(header, qPrintable(name));
            sectionY.append(yOf(header));
            if (sectionY.size() > 1) QVERIFY2(sectionY.last() > sectionY.at(sectionY.size() - 2), qPrintable(name));
        }
        auto under = [&](const QString &label, const QString &section) {
            QQuickItem *item = findText(view.rootObject(), label);
            if (!item) return false;
            const qreal y = yOf(item);
            const int s = sections.indexOf(section);
            return y > sectionY.at(s) && (s + 1 >= sectionY.size() || y < sectionY.at(s + 1));
        };
        QVERIFY(under(QStringLiteral("Deutsch"), QStringLiteral("Languages")));
        QVERIFY(under(QStringLiteral("Number row"), QStringLiteral("Preferences")));
        // Toggling a language in settings takes it out of the globe rotation.
        QQuickItem *toggle = findNamed(view.rootObject(), QStringLiteral("languageToggle_de"));
        QVERIFY(toggle);
        QTest::qWait(100);
        QTest::mouseClick(&view, Qt::LeftButton, {}, toggle->mapToScene(QPointF(toggle->width() / 2, toggle->height() / 2)).toPoint());
        QTRY_VERIFY(!bridge.isLanguageEnabled(QStringLiteral("de")));
        QVERIFY(!bridge.languageCodes().contains(QStringLiteral("de")));
        bridge.setLanguageEnabled(QStringLiteral("de"), true);
        QVERIFY(under(QStringLiteral("Key borders"), QStringLiteral("Theme")));
        QVERIFY(under(QStringLiteral("Autocorrect"), QStringLiteral("Text correction")));
        QVERIFY(under(QStringLiteral("Double-space period"), QStringLiteral("Text correction")));
        QVERIFY(under(QStringLiteral("Gesture trail"), QStringLiteral("Glide typing")));
        QVERIFY(under(QStringLiteral("Clipboard history"), QStringLiteral("Clipboard")));
        QVERIFY(under(QStringLiteral("Local learning"), QStringLiteral("Dictionary")));
        QVERIFY(under(QStringLiteral("Underline word while typing"), QStringLiteral("Advanced")));
    }

    void splitLayoutForThumbTypingLikeGboardOnTablets()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        bridge.setAutoCapitalizationEnabled(false);
        QQuickView view;
        view.setResizeMode(QQuickView::SizeRootObjectToView);
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
        view.resize(1600, 560);                                   // landscape tablet
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        bridge.setLayoutMode(QStringLiteral("split"));
        QTest::qWait(150);
        auto box = [&](const QString &label) {
            QQuickItem *key = findText(view.rootObject(), label)->parentItem();
            return key->mapRectToScene(QRectF(0, 0, key->width(), key->height()));
        };
        const qreal mid = view.width() / 2.0;
        const QRectF t = box(QStringLiteral("t"));
        const QRectF y = box(QStringLiteral("y"));
        QVERIFY2(t.right() < mid && y.left() > mid, "t on the left half, y on the right half");
        QVERIFY2(y.left() - t.right() > view.width() * 0.15, qPrintable(QStringLiteral("gap %1").arg(y.left() - t.right())));
        const QRectF g = box(QStringLiteral("g"));
        const QRectF h = box(QStringLiteral("h"));
        QVERIFY(g.right() < mid && h.left() > mid);                // asdfg | hjkl
        QQuickItem *space = findText(view.rootObject(), QStringLiteral("English"));
        QVERIFY(space);
        const QRectF s = space->parentItem()->mapRectToScene(QRectF(0, 0, space->parentItem()->width(), space->parentItem()->height()));
        QVERIFY2(s.left() < mid && s.right() > mid, "the space bar stays reachable from both halves");
        // ...and spans the gap: its ends meet the inner edges of both halves.
        const qreal slack = view.width() * 0.03;
        QVERIFY2(s.left() <= t.right() + slack && s.right() >= y.left() - slack,
                 qPrintable(QStringLiteral("space %1..%2, halves end %3 / start %4").arg(s.left()).arg(s.right()).arg(t.right()).arg(y.left())));
        // Touch follows the moved keys.
        QTest::mouseClick(&view, Qt::LeftButton, {}, y.center().toPoint());
        QTRY_COMPARE(backend.commits.value(0), QStringLiteral("y"));
        // Number fields keep the plain number pad.
        bridge.setContentType(0, 4);
        QTRY_VERIFY(findNamed(view.rootObject(), QStringLiteral("numpadKey_5")));
    }

    void longPressOnAnEmojiOffersSkinTones()
    {
        Tastra::EmojiCatalog::setSystemDataPathForTesting(QStringLiteral(TASTRA_TEST_DATA "/emoji-system/emoji-test.txt"));
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        Tastra::EmojiCatalog::setSystemDataPathForTesting(QString());
        QCOMPARE(bridge.emojiSkinTones(QStringLiteral("👍")).size(), 5);
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        bridge.activateToolbarAction(QStringLiteral("emoji"));
        QTRY_VERIFY(findText(view.rootObject(), QStringLiteral("👍")));
        QTest::qWait(100);
        QQuickItem *tile = findText(view.rootObject(), QStringLiteral("👍"));
        const QPoint at = tile->mapToScene(QPointF(tile->width() / 2, tile->height() / 2)).toPoint();
        QTest::mousePress(&view, Qt::LeftButton, {}, at);
        QTest::qWait(qApp->styleHints()->mousePressAndHoldInterval() + 200);
        QTest::mouseRelease(&view, Qt::LeftButton, {}, at);
        QTRY_VERIFY(findNamed(view.rootObject(), QStringLiteral("emojiTonePicker")));
        QVERIFY(backend.commits.isEmpty());                         // the long press itself inserts nothing
        QQuickItem *dark = findText(view.rootObject(), QStringLiteral("👍🏿"));
        QVERIFY(dark);
        QTest::qWait(50);
        QTest::mouseClick(&view, Qt::LeftButton, {}, dark->mapToScene(QPointF(dark->width() / 2, dark->height() / 2)).toPoint());
        QTRY_COMPARE(backend.commits, QStringList({QStringLiteral("👍🏿")}));
        QTRY_VERIFY(!findNamed(view.rootObject(), QStringLiteral("emojiTonePicker")));
    }

    void justCopiedTextIsOfferedForPasting()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        QGuiApplication::clipboard()->setText(QStringLiteral("https://example.org/a"));
        QTRY_VERIFY(findNamed(view.rootObject(), QStringLiteral("clipboardChip")));
        QTest::qWait(50);
        QQuickItem *chip = findNamed(view.rootObject(), QStringLiteral("clipboardChip"));
        QTest::mouseClick(&view, Qt::LeftButton, {}, chip->mapToScene(QPointF(chip->width() / 2, chip->height() / 2)).toPoint());
        QTRY_COMPARE(backend.commits.join(QString()), QStringLiteral("https://example.org/a"));
        QTRY_VERIFY(!findNamed(view.rootObject(), QStringLiteral("clipboardChip")));    // used once

        // Typing dismisses a fresh offer.
        QGuiApplication::clipboard()->setText(QStringLiteral("second"));
        QTRY_VERIFY(findNamed(view.rootObject(), QStringLiteral("clipboardChip")));
        bridge.tapLetter(QStringLiteral("a"));
        bridge.space();
        QTRY_VERIFY(!findNamed(view.rootObject(), QStringLiteral("clipboardChip")));
    }

    void secondSymbolPageAndSymbolExtras()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);
        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        auto centre = [&](QQuickItem *item) { return item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint(); };
        bridge.setLanguage(QStringLiteral("ru"));
        bridge.toggleSymbols();
        QTRY_VERIFY(findText(view.rootObject(), QStringLiteral("₽")));          // the language's currency
        QQuickItem *pageKey = findNamed(view.rootObject(), QStringLiteral("symbolPageKey"));
        QVERIFY(pageKey);
        QTest::qWait(50);
        QTest::mouseClick(&view, Qt::LeftButton, {}, centre(pageKey));
        QTRY_VERIFY(findText(view.rootObject(), QStringLiteral("<")));
        QTest::qWait(50);
        // The third row (page key + symbols + Backspace) is as wide as the others.
        {
            auto rowOf = [&](const QString &label) { return findText(view.rootObject(), label)->parentItem()->parentItem(); };
            const QRectF r1 = rowOf(QStringLiteral("~"))->mapRectToScene(QRectF(0, 0, rowOf(QStringLiteral("~"))->width(), 1));
            const QRectF r3 = rowOf(QStringLiteral("<"))->mapRectToScene(QRectF(0, 0, rowOf(QStringLiteral("<"))->width(), 1));
            QVERIFY2(qAbs(r3.width() - r1.width()) < 4.0, qPrintable(QStringLiteral("row1 %1 vs row3 %2").arg(r1.width()).arg(r3.width())));
        }
        QTest::mouseClick(&view, Qt::LeftButton, {}, centre(findText(view.rootObject(), QStringLiteral("<"))));
        QTRY_COMPARE(backend.commits.value(0), QStringLiteral("<"));
        // Back to page 1, long-press the quote for guillemets.
        QTest::mouseClick(&view, Qt::LeftButton, {}, centre(findNamed(view.rootObject(), QStringLiteral("symbolPageKey"))));
        QTRY_VERIFY(findText(view.rootObject(), QStringLiteral("\"")));
        QTest::qWait(50);
        static QPointingDevice *touch = QTest::createTouchDevice();
        const QPoint q = centre(findText(view.rootObject(), QStringLiteral("\"")));
        QTest::touchEvent(&view, touch).press(0, q);
        QTest::qWait(bridge.longPressDelay() + 150);
        QTRY_VERIFY(findText(view.rootObject(), QStringLiteral("«")));          // picker open
        QTest::touchEvent(&view, touch).release(0, q);                          // first choice
        QTRY_COMPARE(backend.commits.value(1), QStringLiteral("«"));
    }

    void caseChangesDoNotRecreateLetterKeys()
    {
        FakeBackend backend;
        Tastra::KeyboardController controller(backend);
        Tastra::KeyboardModel model;
        Tastra::KeyboardUiBridge bridge(controller, model);

        QQuickView view;
        view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(TASTRA_MAIN_QML)));
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
