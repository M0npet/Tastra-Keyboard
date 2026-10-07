// SPDX-License-Identifier: GPL-3.0-or-later

#include "fakeinputkeychords.h"
#include "waylanddisplay.h"

#include <QLoggingCategory>

#include <wayland-client.h>

#include "fake-input-client-protocol.h"

#include <cstring>

Q_LOGGING_CATEGORY(lcFakeInput, "tastra.shortcuts")

namespace Tastra::KWin
{

namespace
{
// The keysym of each evdev key the panel sends (xkbcommon-keysyms.h).
// Keysyms follow the user's layout: with a German layout the key labelled Z
// is evdev KEY_Y, so KEY_Z would be Ctrl+Y.
uint32_t keysymFor(int evdev)
{
    switch (evdev) {
    case EvdevKey::LeftCtrl: return 0xffe3;    // XKB_KEY_Control_L
    case EvdevKey::LeftShift: return 0xffe1;   // XKB_KEY_Shift_L
    case EvdevKey::A: return 0x61;
    case EvdevKey::C: return 0x63;
    case EvdevKey::X: return 0x78;
    case EvdevKey::V: return 0x76;
    case EvdevKey::Z: return 0x7a;
    case EvdevKey::Home: return 0xff50;
    case EvdevKey::Left: return 0xff51;
    case EvdevKey::Up: return 0xff52;
    case EvdevKey::Right: return 0xff53;
    case EvdevKey::Down: return 0xff54;
    case EvdevKey::End: return 0xff57;
    default: return 0;
    }
}
}

const wl_registry_listener fakeInputRegistryListener = {FakeInputKeyChords::registryGlobal, FakeInputKeyChords::registryGlobalRemove};

std::unique_ptr<FakeInputKeyChords> FakeInputKeyChords::create(wl_display *display)
{
    if (!display) return nullptr;
    std::unique_ptr<FakeInputKeyChords> chords(new FakeInputKeyChords(display));
    chords->m_registry = wl_display_get_registry(display);
    wl_registry_add_listener(chords->m_registry, &fakeInputRegistryListener, chords.get());
    wl_display_roundtrip(display);
    if (!chords->m_fakeInput) {
        qCInfo(lcFakeInput) << "no org_kde_kwin_fake_input >= 4: no editing shortcuts";
        return nullptr;
    }
    org_kde_kwin_fake_input_authenticate(chords->m_fakeInput, "Tastra",
                                         "On-screen keyboard: select, select all, copy and cut in the text-editing panel");
    wl_display_flush(display);
    qCInfo(lcFakeInput) << "editing shortcuts through org_kde_kwin_fake_input";
    return chords;
}

std::unique_ptr<FakeInputKeyChords> FakeInputKeyChords::createForApplication()
{
    return create(applicationWaylandDisplay());
}

FakeInputKeyChords::~FakeInputKeyChords()
{
    if (m_fakeInput) {
        if (org_kde_kwin_fake_input_get_version(m_fakeInput) >= ORG_KDE_KWIN_FAKE_INPUT_DESTROY_SINCE_VERSION)
            org_kde_kwin_fake_input_destroy(m_fakeInput);
        else
            wl_proxy_destroy(reinterpret_cast<wl_proxy *>(m_fakeInput));
    }
    if (m_registry) wl_registry_destroy(m_registry);
    wl_display_flush(m_display);
}

void FakeInputKeyChords::registryGlobal(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
    auto *self = static_cast<FakeInputKeyChords *>(data);
    if (std::strcmp(interface, org_kde_kwin_fake_input_interface.name) != 0 || self->m_fakeInput) return;
    if (version < ORG_KDE_KWIN_FAKE_INPUT_KEYBOARD_KEY_SINCE_VERSION) return;
    self->m_fakeInput = static_cast<org_kde_kwin_fake_input *>(
        wl_registry_bind(registry, name, &org_kde_kwin_fake_input_interface, qMin(version, 6u)));
}

void FakeInputKeyChords::registryGlobalRemove(void *, wl_registry *, uint32_t) {}

void FakeInputKeyChords::send(const QList<int> &modifiers, int key)
{
    constexpr uint32_t Pressed = WL_KEYBOARD_KEY_STATE_PRESSED, Released = WL_KEYBOARD_KEY_STATE_RELEASED;
    // KWin 6.5+ (version 6): keysyms, which KWin maps through the user's
    // layout (fakeinputbackend.cpp keyboard_keysym), so Ctrl+Z is Undo on
    // QWERTZ and AZERTY too. Older KWin: plain key codes (US positions).
    bool keysyms = org_kde_kwin_fake_input_get_version(m_fakeInput) >= ORG_KDE_KWIN_FAKE_INPUT_KEYBOARD_KEYSYM_SINCE_VERSION
        && keysymFor(key) != 0;
    for (const int modifier : modifiers) keysyms = keysyms && keysymFor(modifier) != 0;
    if (keysyms) {
        for (const int modifier : modifiers) org_kde_kwin_fake_input_keyboard_keysym(m_fakeInput, keysymFor(modifier), Pressed);
        org_kde_kwin_fake_input_keyboard_keysym(m_fakeInput, keysymFor(key), Pressed);
        org_kde_kwin_fake_input_keyboard_keysym(m_fakeInput, keysymFor(key), Released);
        for (auto it = modifiers.crbegin(); it != modifiers.crend(); ++it)
            org_kde_kwin_fake_input_keyboard_keysym(m_fakeInput, keysymFor(*it), Released);
        wl_display_flush(m_display);
        return;
    }
    for (const int modifier : modifiers) org_kde_kwin_fake_input_keyboard_key(m_fakeInput, uint32_t(modifier), Pressed);
    org_kde_kwin_fake_input_keyboard_key(m_fakeInput, uint32_t(key), Pressed);
    org_kde_kwin_fake_input_keyboard_key(m_fakeInput, uint32_t(key), Released);
    for (auto it = modifiers.crbegin(); it != modifiers.crend(); ++it)
        org_kde_kwin_fake_input_keyboard_key(m_fakeInput, uint32_t(*it), Released);
    wl_display_flush(m_display);
}

}
