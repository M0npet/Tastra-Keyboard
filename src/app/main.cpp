// SPDX-License-Identifier: GPL-3.0-or-later

#include "keyboardhider.h"
#include "keyboarduibridge.h"
#ifdef TASTRA_DATA_CONTROL
#include "platform/kwin/datacontrolclipboard.h"
#endif
#ifdef TASTRA_WAYLAND_EXTRAS
#include "platform/kwin/fakeinputkeychords.h"
#endif
#include "panelvisibility.h"
#include "tracelog.h"
#include "voicecontroller.h"

#ifdef TASTRA_HAVE_VOICE
#include "voice/qtaudiorecorder.h"
#include "voice/whisperrecognizer.h"
#endif

#include "core/keyboardcontroller.h"
#include "core/legacymigration.h"
#include "core/keyboardmodel.h"
#include "platform/kwin/kwininputmethodv1backend.h"
#include "platform/kwin/kwininputmethodv1connection.h"
#include "platform/kwin/kwininputpanelintegration.h"

#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlContext>
#include <QQuickView>
#include <QUrl>

Q_LOGGING_CATEGORY(lcMigration, "tastra.migration", QtInfoMsg)

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    // Settings live in ~/.config/tastra/tastra.conf, next to dictionary.txt
    // and shortcuts.txt.
    app.setOrganizationName(QStringLiteral("tastra"));
    app.setApplicationName(QStringLiteral("tastra"));
    // Up to 0.6.1 the keyboard had another name (see legacymigration.h):
    // bring its files along before anything reads settings, words or the
    // trace switch.
    const QStringList migrated = Tastra::migrateLegacyPaths(Tastra::defaultMigrationRoots());
    // touch ~/.local/state/tastra/trace.enable to capture a privacy-safe
    // protocol/engine trace in ~/.local/state/tastra/trace.log
    Tastra::enableTraceIfRequested(Tastra::defaultStateDir());
    for (const QString &line : migrated) qCInfo(lcMigration).noquote() << line;

    Tastra::KWin::KWinInputMethodV1Backend backend;
    Tastra::KeyboardController controller(backend);
    Tastra::KeyboardModel model;
    Tastra::KeyboardUiBridge bridge(controller, model);
#ifdef TASTRA_HAVE_VOICE
    // Offline dictation; the model is only loaded when the mic key is used.
    Tastra::QtAudioRecorder voiceRecorder;
    Tastra::WhisperRecognizer voiceRecognizer(Tastra::WhisperRecognizer::defaultModelPath());
    Tastra::VoiceController voice(&voiceRecorder, &voiceRecognizer);
    bridge.setVoiceController(&voice);
#endif
    Tastra::KWin::KWinInputMethodV1Connection inputMethod(backend);

    QQuickView view;
    view.setColor(Qt::transparent);
    view.setResizeMode(QQuickView::SizeViewToRootObject);
    view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
    bridge.setQmlEngine(view.engine());
#ifdef TASTRA_DATA_CONTROL
    // KWin offers the Wayland clipboard only to the focused window, which an
    // input panel never is: read and set it through data control instead.
    const auto dataControlClipboard = Tastra::KWin::DataControlClipboard::createForApplication();
    if (dataControlClipboard) bridge.setSystemClipboard(dataControlClipboard.get());
#endif
#ifdef TASTRA_WAYLAND_EXTRAS
    // Select / Select all / Copy / Cut as keyboard shortcuts (KWin drops the
    // modifiers of an input method's own keys).
    const auto keyChords = Tastra::KWin::FakeInputKeyChords::createForApplication();
    if (keyChords) bridge.setKeyChordSender(keyChords.get());
#endif
    view.setSource(QUrl(QStringLiteral("qrc:/tastra/Main.qml")));

    if (view.status() == QQuickView::Error) {
        return 2;
    }

    if (!Tastra::KWin::initializeInputPanel(&view)) {
        return 3;
    }

    QObject::connect(
        &inputMethod,
        &Tastra::KWin::KWinInputMethodV1Connection::surroundingTextChanged,
        &bridge,
        &Tastra::KeyboardUiBridge::setSurroundingText);

    QObject::connect(
        &inputMethod,
        &Tastra::KWin::KWinInputMethodV1Connection::contentTypeChanged,
        &bridge,
        &Tastra::KeyboardUiBridge::setContentType);

    QObject::connect(
        &inputMethod,
        &Tastra::KWin::KWinInputMethodV1Connection::contextReset,
        &bridge,
        &Tastra::KeyboardUiBridge::resetCompositionFromClient);

    QObject::connect(
        &inputMethod,
        &Tastra::KWin::KWinInputMethodV1Connection::preferredLanguageChanged,
        &bridge,
        &Tastra::KeyboardUiBridge::setPreferredLanguage);

    // Hide with a short grace time (see PanelVisibility). requestActivate()
    // was dropped: for an input-panel surface it is a no-op in QtWayland, and
    // KWin never gives input panels the focus anyway.
    Tastra::KWinKeyboardHider hider;
    bridge.setKeyboardHider(&hider);

    Tastra::PanelVisibility panel;
    QObject::connect(&panel, &Tastra::PanelVisibility::visibleChanged, &view,
                     [&view](bool visible) { view.setVisible(visible); });

    QObject::connect(
        &inputMethod,
        &Tastra::KWin::KWinInputMethodV1Connection::contextActiveChanged,
        &view,
        [&panel, &bridge](bool active) {
            // Every activation is a new client context (text-input version,
            // surrounding-text support, content type); never inherit the old one.
            bridge.resetInputContext();
            panel.setActive(active);
        });

    panel.setActive(inputMethod.hasContext());
    view.setVisible(panel.visible());

    return app.exec();
}
