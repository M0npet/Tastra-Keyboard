// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "core/keychords.h"

#include <memory>

struct wl_display;
struct wl_registry;
struct org_kde_kwin_fake_input;

namespace Tastra::KWin
{

// Editing shortcuts through KWin's org_kde_kwin_fake_input, the protocol
// KDE Connect uses for remote keyboards: its keys go through KWin's normal
// keyboard handling, modifiers included, to the focused window. KWin
// accepts every client's authenticate request (fakeinputbackend.cpp) and
// announces the protocol to the input method (wayland_server.cpp).
class FakeInputKeyChords final : public KeyChordSender
{
public:
    // Null when the compositor has no org_kde_kwin_fake_input (version >= 4,
    // which added keyboard keys).
    static std::unique_ptr<FakeInputKeyChords> create(wl_display *display);
    static std::unique_ptr<FakeInputKeyChords> createForApplication();
    ~FakeInputKeyChords() override;

    void send(const QList<int> &modifiers, int key) override;

    static void registryGlobal(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
    static void registryGlobalRemove(void *data, wl_registry *registry, uint32_t name);

private:
    explicit FakeInputKeyChords(wl_display *display) : m_display(display) {}

    wl_display *m_display = nullptr;
    wl_registry *m_registry = nullptr;
    org_kde_kwin_fake_input *m_fakeInput = nullptr;
};

}
