// SPDX-License-Identifier: GPL-3.0-or-later

#include "keyboardhider.h"
#include "keyboarduibridge.h"
#include "panelvisibility.h"
#include "tracelog.h"
#include "voicecontroller.h"

#ifdef V3KBD_HAVE_VOICE
#include "voice/qtaudiorecorder.h"
#include "voice/whisperrecognizer.h"
#endif

#include "core/keyboardcontroller.h"
#include "core/keyboardmodel.h"
#include "platform/kwin/kwininputmethodv1backend.h"
#include "platform/kwin/kwininputmethodv1connection.h"
#include "platform/kwin/kwininputpanelintegration.h"

#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QUrl>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("V3Keyboard"));
    app.setApplicationName(QStringLiteral("V3 Keyboard"));
    // touch ~/.local/state/v3-keyboard/trace.enable to capture a privacy-safe
    // protocol/engine trace in ~/.local/state/v3-keyboard/trace.log
    V3Keyboard::enableTraceIfRequested(V3Keyboard::defaultStateDir());

    V3Keyboard::KWin::KWinInputMethodV1Backend backend;
    V3Keyboard::KeyboardController controller(backend);
    V3Keyboard::KeyboardModel model;
    V3Keyboard::KeyboardUiBridge bridge(controller, model);
#ifdef V3KBD_HAVE_VOICE
    // Offline dictation; the model is only loaded when the mic key is used.
    V3Keyboard::QtAudioRecorder voiceRecorder;
    V3Keyboard::WhisperRecognizer voiceRecognizer(V3Keyboard::WhisperRecognizer::defaultModelPath());
    V3Keyboard::VoiceController voice(&voiceRecorder, &voiceRecognizer);
    bridge.setVoiceController(&voice);
#endif
    V3Keyboard::KWin::KWinInputMethodV1Connection inputMethod(backend);

    QQuickView view;
    view.setColor(Qt::transparent);
    view.setResizeMode(QQuickView::SizeViewToRootObject);
    view.rootContext()->setContextProperty(QStringLiteral("keyboardBridge"), &bridge);
    view.setSource(QUrl(QStringLiteral("qrc:/v3keyboard/Main.qml")));

    if (view.status() == QQuickView::Error) {
        return 2;
    }

    if (!V3Keyboard::KWin::initializeInputPanel(&view)) {
        return 3;
    }

    QObject::connect(
        &inputMethod,
        &V3Keyboard::KWin::KWinInputMethodV1Connection::surroundingTextChanged,
        &bridge,
        &V3Keyboard::KeyboardUiBridge::setSurroundingText);

    QObject::connect(
        &inputMethod,
        &V3Keyboard::KWin::KWinInputMethodV1Connection::contentTypeChanged,
        &bridge,
        &V3Keyboard::KeyboardUiBridge::setContentType);

    QObject::connect(
        &inputMethod,
        &V3Keyboard::KWin::KWinInputMethodV1Connection::contextReset,
        &bridge,
        &V3Keyboard::KeyboardUiBridge::resetCompositionFromClient);

    QObject::connect(
        &inputMethod,
        &V3Keyboard::KWin::KWinInputMethodV1Connection::preferredLanguageChanged,
        &bridge,
        &V3Keyboard::KeyboardUiBridge::setPreferredLanguage);

    // Hide with a short grace time (see PanelVisibility). requestActivate()
    // was dropped: for an input-panel surface it is a no-op in QtWayland, and
    // KWin never gives input panels the focus anyway.
    V3Keyboard::KWinKeyboardHider hider;
    bridge.setKeyboardHider(&hider);

    V3Keyboard::PanelVisibility panel;
    QObject::connect(&panel, &V3Keyboard::PanelVisibility::visibleChanged, &view,
                     [&view](bool visible) { view.setVisible(visible); });

    QObject::connect(
        &inputMethod,
        &V3Keyboard::KWin::KWinInputMethodV1Connection::contextActiveChanged,
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
