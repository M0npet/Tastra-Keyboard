// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "kwininputmethodv1backend.h"
#include "kwininputmethodv1session.h"

#include <QtWaylandClient/QWaylandClientExtensionTemplate>

#include <memory>

#include <qwayland-input-method-unstable-v1.h>

namespace Tastra::KWin
{

class ProtocolInputMethodV1Context;

class KWinInputMethodV1Connection final
    : public QWaylandClientExtensionTemplate<KWinInputMethodV1Connection>
    , public QtWayland::zwp_input_method_v1
{
    Q_OBJECT

public:
    explicit KWinInputMethodV1Connection(KWinInputMethodV1Backend &backend);
    ~KWinInputMethodV1Connection() override;

    bool hasContext() const;

Q_SIGNALS:
    void contextActiveChanged(bool active);
    void surroundingTextChanged(const QString &text, int cursorByte, int anchorByte);
    void contentTypeChanged(quint32 hint, quint32 purpose);
    void contextReset();
    void preferredLanguageChanged(const QString &language);

private:
    friend class ProtocolInputMethodV1Context;
    void zwp_input_method_v1_activate(::zwp_input_method_context_v1 *id) override;
    void zwp_input_method_v1_deactivate(::zwp_input_method_context_v1 *context) override;

    KWinInputMethodV1Session m_session;
    std::unique_ptr<ProtocolInputMethodV1Context> m_context;
};

}
