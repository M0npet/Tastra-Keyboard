// SPDX-License-Identifier: GPL-3.0-or-later
//
// The clipboard through ext-data-control-v1 against a small in-process
// compositor (libwayland-server, its own thread) that implements wl_seat and
// the data-control globals the way KWin's seat does: one selection, offered
// to every data-control device, with its source cancelled when replaced.
// Two clients stand for "another application" and the keyboard.

#include <QtTest/QTest>

#include <QSignalSpy>
#include <QSocketNotifier>
#include <QTimer>

#include "fakecompositor.h"
#include "platform/kwin/datacontrolclipboard.h"

#include <wayland-client.h>

#include "ext-data-control-v1-client-protocol.h"

#include <algorithm>
#include <memory>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

using TestWayland::FakeCompositor;

namespace
{

// A client connection whose events the Qt event loop dispatches, as Qt's
// Wayland plugin does for the keyboard.
class Connection
{
public:
    explicit Connection(const char *socket)
        : m_display(wl_display_connect(socket))
    {
        if (!m_display) return;
        m_notifier = std::make_unique<QSocketNotifier>(wl_display_get_fd(m_display), QSocketNotifier::Read);
        // Never block: read what is there, then dispatch (as Qt does).
        QObject::connect(m_notifier.get(), &QSocketNotifier::activated, [this] {
            if (wl_display_prepare_read(m_display) == 0) wl_display_read_events(m_display);
            wl_display_dispatch_pending(m_display);
        });
        m_timer.setInterval(5);
        QObject::connect(&m_timer, &QTimer::timeout, [this] {
            wl_display_dispatch_pending(m_display);
            wl_display_flush(m_display);
        });
        m_timer.start();
    }
    ~Connection()
    {
        m_timer.stop();
        m_notifier.reset();
        if (m_display) wl_display_disconnect(m_display);
    }
    wl_display *display() const { return m_display; }

private:
    wl_display *m_display = nullptr;
    std::unique_ptr<QSocketNotifier> m_notifier;
    QTimer m_timer;
};

}

class DataControlClipboardTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void anotherApplicationsTextIsRead()
    {
        FakeCompositor compositor;
        Connection appConnection(compositor.socketName()), keyboardConnection(compositor.socketName());
        auto app = Tastra::KWin::DataControlClipboard::create(appConnection.display());
        auto keyboard = Tastra::KWin::DataControlClipboard::create(keyboardConnection.display());
        QVERIFY(app && keyboard);
        QSignalSpy changed(keyboard.get(), &Tastra::SystemClipboard::changed);

        app->setText(QStringLiteral("Привіт 👋 from another app"));
        QTRY_COMPARE(keyboard->text(), QStringLiteral("Привіт 👋 from another app"));
        QVERIFY(changed.count() >= 1);

        // Longer text arrives whole, in several reads. (Both clients share
        // one thread here, so it stays below a pipe's 64 KiB buffer; in the
        // keyboard the writer is another process.)
        const QString big = QString(QStringLiteral("0123456789")).repeated(6000);
        app->setText(big);
        QTRY_COMPARE_WITH_TIMEOUT(keyboard->text().size(), big.size(), 10000);
        QCOMPARE(keyboard->text(), big);
    }

    void theKeyboardsCopyReachesOtherApplications()
    {
        // Copy / Cut in the text-editing panel: the keyboard owns the
        // clipboard without having keyboard focus; its own offer is not read
        // back through a pipe (no waiting on itself).
        FakeCompositor compositor;
        Connection appConnection(compositor.socketName()), keyboardConnection(compositor.socketName());
        auto app = Tastra::KWin::DataControlClipboard::create(appConnection.display());
        auto keyboard = Tastra::KWin::DataControlClipboard::create(keyboardConnection.display());
        QVERIFY(app && keyboard);
        keyboard->setText(QStringLiteral("copied on the keyboard"));
        QTRY_COMPARE(app->text(), QStringLiteral("copied on the keyboard"));
        QCOMPARE(keyboard->text(), QStringLiteral("copied on the keyboard"));

        // The other application copies: the keyboard's source is cancelled
        // and the new text arrives.
        app->setText(QStringLiteral("newer"));
        QTRY_COMPARE(keyboard->text(), QStringLiteral("newer"));
        keyboard->setText(QStringLiteral("again"));
        QTRY_COMPARE(app->text(), QStringLiteral("again"));

        // Clearing empties it for everyone.
        QSignalSpy changed(app.get(), &Tastra::SystemClipboard::changed);
        keyboard->clear();
        QTRY_VERIFY(changed.count() >= 1);
        QTRY_VERIFY(app->text().isEmpty());
    }

    void imagesAreNotText()
    {
        FakeCompositor compositor;
        Connection appConnection(compositor.socketName()), keyboardConnection(compositor.socketName());
        auto keyboard = Tastra::KWin::DataControlClipboard::create(keyboardConnection.display());
        QVERIFY(keyboard);
        auto text = Tastra::KWin::DataControlClipboard::create(appConnection.display());
        text->setText(QStringLiteral("text first"));
        QTRY_COMPARE(keyboard->text(), QStringLiteral("text first"));

        // A raw client that only offers an image.
        wl_display *display = appConnection.display();
        wl_registry *registry = wl_display_get_registry(display);
        struct Globals { ext_data_control_manager_v1 *manager = nullptr; wl_seat *seat = nullptr; } globals;
        static const wl_registry_listener listener = {
            [](void *data, wl_registry *r, uint32_t name, const char *interface, uint32_t) {
                auto *g = static_cast<Globals *>(data);
                if (std::string(interface) == ext_data_control_manager_v1_interface.name)
                    g->manager = static_cast<ext_data_control_manager_v1 *>(wl_registry_bind(r, name, &ext_data_control_manager_v1_interface, 1));
                if (std::string(interface) == wl_seat_interface.name)
                    g->seat = static_cast<wl_seat *>(wl_registry_bind(r, name, &wl_seat_interface, 1));
            },
            [](void *, wl_registry *, uint32_t) {}};
        wl_registry_add_listener(registry, &listener, &globals);
        wl_display_roundtrip(display);
        QVERIFY(globals.manager && globals.seat);
        ext_data_control_device_v1 *device = ext_data_control_manager_v1_get_data_device(globals.manager, globals.seat);
        ext_data_control_source_v1 *source = ext_data_control_manager_v1_create_data_source(globals.manager);
        ext_data_control_source_v1_offer(source, "image/png");
        QSignalSpy changed(keyboard.get(), &Tastra::SystemClipboard::changed);
        ext_data_control_device_v1_set_selection(device, source);
        wl_display_flush(display);
        QTRY_VERIFY(changed.count() >= 1);
        QVERIFY(keyboard->text().isEmpty());
        ext_data_control_source_v1_destroy(source);
        ext_data_control_device_v1_destroy(device);
        ext_data_control_manager_v1_destroy(globals.manager);
        wl_seat_destroy(globals.seat);
        wl_registry_destroy(registry);
        wl_display_flush(display);
    }

    void withoutDataControlThereIsNoClipboardObject()
    {
        // KWin before Plasma 6.4: the keyboard keeps QClipboard.
        FakeCompositor compositor(false);
        Connection connection(compositor.socketName());
        QVERIFY(connection.display());
        QVERIFY(!Tastra::KWin::DataControlClipboard::create(connection.display()));
    }
};

QTEST_GUILESS_MAIN(DataControlClipboardTest)
#include "tst_datacontrolclipboard.moc"
