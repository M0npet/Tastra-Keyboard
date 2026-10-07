# Vendored Wayland protocols

- `fake-input.xml` — KDE's `org_kde_kwin_fake_input` (version 6), from
  [plasma-wayland-protocols](https://invent.kde.org/libraries/plasma-wayland-protocols)
  (`src/protocols/fake-input.xml`, commit e14966e of 2026-10-02).
  SPDX-FileCopyrightText: 2015 Martin Gräßlin; SPDX-License-Identifier:
  LGPL-2.1-or-later. Vendored so that building Tastra does not need the
  plasma-wayland-protocols package; used for the editing shortcuts of the
  text-editing panel (Select, Select all, Copy, Cut).

Other protocols (input-method-unstable-v1, ext-data-control-v1, xdg-shell)
come from the system's wayland-protocols package.
