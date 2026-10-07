// SPDX-License-Identifier: GPL-3.0-or-later
//
// A small compositor for tests (libwayland-server, its own thread): wl_seat
// and ext-data-control-v1 the way KWin's seat handles them (one selection,
// offered to every data-control device, its source cancelled when replaced).
#pragma once

#include <wayland-server.h>

#include "ext-data-control-v1-server-protocol.h"
#include "xdg-shell-server-protocol.h"
#include "fake-input-server-protocol.h"

#include <algorithm>
#include <mutex>
#include <utility>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

namespace TestWayland
{

class FakeCompositor
{
public:
    // fakeInputVersion 6 (Plasma 6.5+) has keyboard_keysym, 5 only keycodes.
    explicit FakeCompositor(bool withDataControl = true, int fakeInputVersion = 6)
    {
        m_display = wl_display_create();
        m_socket = wl_display_add_socket_auto(m_display);
        wl_global_create(m_display, &wl_seat_interface, 5, this, bindSeat);
        // What Qt's Wayland platform needs to start (Qt 6.11 refuses to run
        // without a shell); no window is ever made.
        wl_global_create(m_display, &wl_compositor_interface, 4, this, bindCompositor);
        wl_global_create(m_display, &xdg_wm_base_interface, 1, this, bindShell);
        if (withDataControl) wl_global_create(m_display, &ext_data_control_manager_v1_interface, 1, this, bindManager);
        wl_global_create(m_display, &org_kde_kwin_fake_input_interface, fakeInputVersion, this, bindFakeInput);
        m_thread = std::thread([this] { wl_display_run(m_display); });
    }
    ~FakeCompositor()
    {
        wl_display_terminate(m_display);
        m_thread.join();
        wl_display_destroy_clients(m_display);
        wl_display_destroy(m_display);
    }
    const char *socketName() const { return m_socket; }
    // Keys received through org_kde_kwin_fake_input: (evdev code, pressed).
    std::vector<std::pair<uint32_t, bool>> fakeKeys() const
    {
        std::lock_guard lock(m_mutex);
        return m_fakeKeys;
    }
    // Keysyms received through keyboard_keysym (version 6): (keysym, pressed).
    std::vector<std::pair<uint32_t, bool>> fakeKeysyms() const
    {
        std::lock_guard lock(m_mutex);
        return m_fakeKeysyms;
    }
    // How often a device was told the clipboard is empty.
    int emptySelectionsSent() const
    {
        std::lock_guard lock(m_mutex);
        return m_emptySelections;
    }
    bool fakeInputAuthenticated() const
    {
        std::lock_guard lock(m_mutex);
        return m_authenticated;
    }

private:
    struct Source { FakeCompositor *owner = nullptr; int id = 0; wl_resource *resource = nullptr; std::vector<std::string> mimes; };
    struct Offer { FakeCompositor *owner = nullptr; int sourceId = 0; };

    static FakeCompositor *self(wl_resource *resource) { return static_cast<FakeCompositor *>(wl_resource_get_user_data(resource)); }

    static void bindSeat(wl_client *client, void *, uint32_t version, uint32_t id)
    {
        static const struct wl_seat_interface seatImpl = {
            [](wl_client *, wl_resource *, uint32_t) {}, [](wl_client *, wl_resource *, uint32_t) {},
            [](wl_client *, wl_resource *, uint32_t) {}, [](wl_client *, wl_resource *r) { wl_resource_destroy(r); }};
        wl_resource *seat = wl_resource_create(client, &wl_seat_interface, int(version), id);
        wl_resource_set_implementation(seat, &seatImpl, nullptr, nullptr);
        wl_seat_send_capabilities(seat, 0);
    }

    static void bindCompositor(wl_client *client, void *, uint32_t version, uint32_t id)
    {
        static const struct wl_surface_interface surfaceImpl = {
            [](wl_client *, wl_resource *r) { wl_resource_destroy(r); },
            [](wl_client *, wl_resource *, wl_resource *, int32_t, int32_t) {},
            [](wl_client *, wl_resource *, int32_t, int32_t, int32_t, int32_t) {},
            [](wl_client *c, wl_resource *r, uint32_t callback) {
                wl_resource_create(c, &wl_callback_interface, 1, callback);
                (void)r;
            },
            [](wl_client *, wl_resource *, wl_resource *) {}, [](wl_client *, wl_resource *, wl_resource *) {},
            [](wl_client *, wl_resource *) {}, [](wl_client *, wl_resource *, int32_t) {},
            [](wl_client *, wl_resource *, int32_t) {}, [](wl_client *, wl_resource *, int32_t, int32_t, int32_t, int32_t) {},
            [](wl_client *, wl_resource *, int32_t, int32_t) {}};
        static const struct wl_region_interface regionImpl = {
            [](wl_client *, wl_resource *r) { wl_resource_destroy(r); },
            [](wl_client *, wl_resource *, int32_t, int32_t, int32_t, int32_t) {},
            [](wl_client *, wl_resource *, int32_t, int32_t, int32_t, int32_t) {}};
        static const struct wl_compositor_interface compositorImpl = {
            [](wl_client *c, wl_resource *r, uint32_t id) {
                wl_resource *surface = wl_resource_create(c, &wl_surface_interface, wl_resource_get_version(r), id);
                wl_resource_set_implementation(surface, &surfaceImpl, nullptr, nullptr);
            },
            [](wl_client *c, wl_resource *, uint32_t id) {
                wl_resource *region = wl_resource_create(c, &wl_region_interface, 1, id);
                wl_resource_set_implementation(region, &regionImpl, nullptr, nullptr);
            }};
        wl_resource *compositor = wl_resource_create(client, &wl_compositor_interface, int(version), id);
        wl_resource_set_implementation(compositor, &compositorImpl, nullptr, nullptr);
    }

    static void bindShell(wl_client *client, void *, uint32_t version, uint32_t id)
    {
        static const struct xdg_wm_base_interface shellImpl = {
            [](wl_client *, wl_resource *r) { wl_resource_destroy(r); },
            [](wl_client *, wl_resource *, uint32_t) {},
            [](wl_client *, wl_resource *, uint32_t, wl_resource *) {},
            [](wl_client *, wl_resource *, uint32_t) {}};
        wl_resource *shell = wl_resource_create(client, &xdg_wm_base_interface, int(version), id);
        wl_resource_set_implementation(shell, &shellImpl, nullptr, nullptr);
    }

    static void bindFakeInput(wl_client *client, void *data, uint32_t version, uint32_t id)
    {
        static const struct org_kde_kwin_fake_input_interface fakeImpl = {
            [](wl_client *, wl_resource *r, const char *, const char *) {
                std::lock_guard lock(self(r)->m_mutex);
                self(r)->m_authenticated = true;
            },
            [](wl_client *, wl_resource *, wl_fixed_t, wl_fixed_t) {},
            [](wl_client *, wl_resource *, uint32_t, uint32_t) {},
            [](wl_client *, wl_resource *, uint32_t, wl_fixed_t) {},
            [](wl_client *, wl_resource *, uint32_t, wl_fixed_t, wl_fixed_t) {},
            [](wl_client *, wl_resource *, uint32_t, wl_fixed_t, wl_fixed_t) {},
            [](wl_client *, wl_resource *, uint32_t) {},
            [](wl_client *, wl_resource *) {},
            [](wl_client *, wl_resource *) {},
            [](wl_client *, wl_resource *, wl_fixed_t, wl_fixed_t) {},
            [](wl_client *, wl_resource *r, uint32_t key, uint32_t state) {
                std::lock_guard lock(self(r)->m_mutex);
                self(r)->m_fakeKeys.emplace_back(key, state == WL_KEYBOARD_KEY_STATE_PRESSED);
            },
            [](wl_client *, wl_resource *r) { wl_resource_destroy(r); },
            [](wl_client *, wl_resource *r, uint32_t keysym, uint32_t state) {
                std::lock_guard lock(self(r)->m_mutex);
                self(r)->m_fakeKeysyms.emplace_back(keysym, state == WL_KEYBOARD_KEY_STATE_PRESSED);
            }};
        wl_resource *fake = wl_resource_create(client, &org_kde_kwin_fake_input_interface, int(version), id);
        wl_resource_set_implementation(fake, &fakeImpl, data, nullptr);
    }

    static void bindManager(wl_client *client, void *data, uint32_t version, uint32_t id)
    {
        static const struct ext_data_control_manager_v1_interface managerImpl = {
            createSource, getDevice, [](wl_client *, wl_resource *r) { wl_resource_destroy(r); }};
        wl_resource *manager = wl_resource_create(client, &ext_data_control_manager_v1_interface, int(version), id);
        wl_resource_set_implementation(manager, &managerImpl, data, nullptr);
    }

    static void createSource(wl_client *client, wl_resource *manager, uint32_t id)
    {
        static const struct ext_data_control_source_v1_interface sourceImpl = {
            [](wl_client *, wl_resource *r, const char *mime) { static_cast<Source *>(wl_resource_get_user_data(r))->mimes.push_back(mime); },
            [](wl_client *, wl_resource *r) { wl_resource_destroy(r); }};
        FakeCompositor *compositor = self(manager);
        auto *source = new Source{compositor, ++compositor->m_nextId, nullptr, {}};
        source->resource = wl_resource_create(client, &ext_data_control_source_v1_interface, wl_resource_get_version(manager), id);
        compositor->m_sources.push_back(source);
        wl_resource_set_implementation(source->resource, &sourceImpl, source, [](wl_resource *r) {
            auto *gone = static_cast<Source *>(wl_resource_get_user_data(r));
            FakeCompositor *owner = gone->owner;
            owner->m_sources.erase(std::remove(owner->m_sources.begin(), owner->m_sources.end(), gone), owner->m_sources.end());
            if (owner->m_selection == gone) {
                owner->m_selection = nullptr;                 // gone, no "cancelled"
                for (wl_resource *device : owner->m_devices) owner->offer(device);
            }
            delete gone;
        });
    }

    static void getDevice(wl_client *client, wl_resource *manager, uint32_t id, wl_resource *)
    {
        static const struct ext_data_control_device_v1_interface deviceImpl = {
            [](wl_client *, wl_resource *r, wl_resource *source) {
                self(r)->setSelection(source ? static_cast<Source *>(wl_resource_get_user_data(source)) : nullptr);
            },
            [](wl_client *, wl_resource *r) { wl_resource_destroy(r); },
            [](wl_client *, wl_resource *, wl_resource *) {}};
        FakeCompositor *compositor = self(manager);
        wl_resource *device = wl_resource_create(client, &ext_data_control_device_v1_interface, wl_resource_get_version(manager), id);
        wl_resource_set_implementation(device, &deviceImpl, compositor, [](wl_resource *r) {
            auto &devices = self(r)->m_devices;
            devices.erase(std::remove(devices.begin(), devices.end(), r), devices.end());
        });
        compositor->m_devices.push_back(device);
        compositor->offer(device);
    }

    void setSelection(Source *source)
    {
        if (m_selection == source) return;
        if (m_selection) ext_data_control_source_v1_send_cancelled(m_selection->resource);
        m_selection = source;
        for (wl_resource *device : m_devices) offer(device);
    }

    void offer(wl_resource *device)
    {
        if (!m_selection) {
            {
                std::lock_guard lock(m_mutex);
                ++m_emptySelections;
            }
            ext_data_control_device_v1_send_selection(device, nullptr);
            return;
        }
        static const struct ext_data_control_offer_v1_interface offerImpl = {
            [](wl_client *, wl_resource *r, const char *mime, int32_t fd) {
                auto *offer = static_cast<Offer *>(wl_resource_get_user_data(r));
                Source *selection = offer->owner->m_selection;
                if (selection && selection->id == offer->sourceId) ext_data_control_source_v1_send_send(selection->resource, mime, fd);
                ::close(fd);
            },
            [](wl_client *, wl_resource *r) { wl_resource_destroy(r); }};
        wl_resource *offer = wl_resource_create(wl_resource_get_client(device), &ext_data_control_offer_v1_interface,
                                                wl_resource_get_version(device), 0);
        wl_resource_set_implementation(offer, &offerImpl, new Offer{this, m_selection->id},
                                       [](wl_resource *r) { delete static_cast<Offer *>(wl_resource_get_user_data(r)); });
        ext_data_control_device_v1_send_data_offer(device, offer);
        for (const std::string &mime : m_selection->mimes) ext_data_control_offer_v1_send_offer(offer, mime.c_str());
        ext_data_control_device_v1_send_selection(device, offer);
    }

    wl_display *m_display = nullptr;
    const char *m_socket = nullptr;
    std::thread m_thread;
    std::vector<Source *> m_sources;
    std::vector<wl_resource *> m_devices;
    Source *m_selection = nullptr;
    int m_nextId = 0;
    mutable std::mutex m_mutex;
    std::vector<std::pair<uint32_t, bool>> m_fakeKeys;
    std::vector<std::pair<uint32_t, bool>> m_fakeKeysyms;
    bool m_authenticated = false;
    int m_emptySelections = 0;
};

}
