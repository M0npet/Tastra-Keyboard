// SPDX-License-Identifier: GPL-3.0-or-later
//
// The keyboard's path: DataControlClipboard on Qt's own Wayland display
// (createForApplication), with Qt's event loop dispatching its events,
// against the test compositor. A second plain connection is "another
// application".

#include <QtTest/QTest>

#include <QGuiApplication>
#include <QSignalSpy>
#include <QTimer>

#include "fakecompositor.h"
#include "platform/kwin/datacontrolclipboard.h"

#include <wayland-client.h>

#include <memory>

class DataControlQtDisplayTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void theKeyboardReadsAndOwnsTheClipboardThroughQtsDisplay()
    {
        QVERIFY(QGuiApplication::platformName().startsWith(QLatin1String("wayland")));
        auto keyboard = Tastra::KWin::DataControlClipboard::createForApplication();
        QVERIFY(keyboard);

        wl_display *other = wl_display_connect(nullptr);
        QVERIFY(other);
        auto application = Tastra::KWin::DataControlClipboard::create(other);
        QVERIFY(application);
        QTimer pump;
        QObject::connect(&pump, &QTimer::timeout, [other] {
            if (wl_display_prepare_read(other) == 0) wl_display_read_events(other);
            wl_display_dispatch_pending(other);
            wl_display_flush(other);
        });
        pump.start(5);

        application->setText(QStringLiteral("copied in Firefox"));
        QTRY_COMPARE(keyboard->text(), QStringLiteral("copied in Firefox"));
        keyboard->setText(QStringLiteral("copied on the keyboard"));
        QTRY_COMPARE(application->text(), QStringLiteral("copied on the keyboard"));

        pump.stop();
        application.reset();
        wl_display_disconnect(other);
    }
};

int main(int argc, char **argv)
{
    // The compositor first, then Qt's Wayland platform connects to it.
    TestWayland::FakeCompositor compositor;
    qputenv("WAYLAND_DISPLAY", compositor.socketName());
    qputenv("QT_QPA_PLATFORM", "wayland");
    QGuiApplication app(argc, argv);
    DataControlQtDisplayTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_datacontrolqtdisplay.moc"
