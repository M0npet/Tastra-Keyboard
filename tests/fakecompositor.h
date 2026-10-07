// SPDX-License-Identifier: GPL-3.0-or-later
//
// A small compositor for tests (libwayland-server, its own thread): wl_seat
// and ext-data-control-v1 the way KWin's seat handles them (one selection,
// offered to every data-control device, its source cancelled when replaced).
#pragma once

#include <wayland-server.h>

#include "ext-data-control-v1-server-protocol.h"

#include <algorithm>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

namespace TestWayland
{

class FakeCompositor
{
public:
    explicit FakeCompositor(bool withDataControl = true)
    {
        m_display = wl_display_create();
        m_socket = wl_display_add_socket_auto(m_display);
        wl_global_create(m_display, &wl_seat_interface, 5, this, bindSeat);
        if (withDataControl) wl_global_create(m_display, &ext_data_control_manager_v1_interface, 1, this, bindManager);
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
};

}
