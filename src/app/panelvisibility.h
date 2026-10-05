// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QTimer>

namespace V3Keyboard
{

// Shows the panel as soon as a text field activates the input method, but
// hides it only after a short grace time: rich web editors (ProseMirror &c.)
// disable and re-enable text input while re-rendering, and a panel that
// vanished for that moment would let the next tap fall through onto the page
// and take the focus away from the field.
class PanelVisibility final : public QObject
{
    Q_OBJECT

public:
    explicit PanelVisibility(int hideDelayMs = 250, QObject *parent = nullptr);

    void setActive(bool active);
    bool visible() const;

Q_SIGNALS:
    void visibleChanged(bool visible);

private:
    QTimer m_hideTimer;
    bool m_visible = false;
};

}
