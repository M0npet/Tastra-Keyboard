// SPDX-License-Identifier: GPL-3.0-or-later

#include "kwininputmethodv1connection.h"

#include <QLoggingCategory>

// Enable with ~/.config/QtProject/qtlogging.ini:  [Rules] tastra.*.debug=true
// Output goes to stderr, which KWin forwards to the user journal.
Q_LOGGING_CATEGORY(lcKWinInput, "tastra.input.protocol", QtWarningMsg)

namespace Tastra::KWin
{

class ProtocolInputMethodV1Context final
    : public QtWayland::zwp_input_method_context_v1
    , public InputMethodV1Context
{
public:
    ProtocolInputMethodV1Context(
        ::zwp_input_method_context_v1 *object,
        KWinInputMethodV1Session &session,
        KWinInputMethodV1Connection &owner)
        : QtWayland::zwp_input_method_context_v1(object)
        , m_session(session)
        , m_owner(owner)
    {
    }

    ~ProtocolInputMethodV1Context() override
    {
        // input-method-v1: after deactivate the client must destroy the
        // context. The generated wrapper's destructor does not, which leaked
        // one proxy per focus change and left its listener pointing at this
        // freed object.
        if (object()) {
            destroy();
        }
    }

    void commitString(quint32 serial, const QString &text) override
    {
        commit_string(serial, text);
    }

    void deleteSurroundingText(qint32 index, quint32 length) override
    {
        delete_surrounding_text(index, length);
    }

    void preeditString(quint32 serial, const QString &text, const QString &commit) override
    {
        preedit_string(serial, text, commit);
    }

    void preeditCursor(qint32 index) override
    {
        preedit_cursor(index);
    }

    void keySym(
        quint32 serial,
        quint32 time,
        quint32 sym,
        quint32 state,
        quint32 modifiers) override
    {
        keysym(serial, time, sym, state, modifiers);
    }

private:
    void zwp_input_method_context_v1_surrounding_text(
        const QString &text, uint32_t cursor, uint32_t anchor) override
    {
        // Privacy: log sizes and offsets only, never the field contents.
        qCDebug(lcKWinInput) << "surrounding_text bytes" << text.toUtf8().size() << "cursor" << cursor << "anchor" << anchor;
        Q_EMIT m_owner.surroundingTextChanged(
            text, static_cast<int>(cursor), static_cast<int>(anchor));
    }

    void zwp_input_method_context_v1_reset() override
    {
        qCDebug(lcKWinInput) << "reset";
        Q_EMIT m_owner.contextReset();
    }

    void zwp_input_method_context_v1_content_type(uint32_t hint, uint32_t purpose) override
    {
        qCDebug(lcKWinInput) << "content_type hint" << Qt::hex << hint << "purpose" << Qt::dec << purpose;
        Q_EMIT m_owner.contentTypeChanged(hint, purpose);
    }

    void zwp_input_method_context_v1_preferred_language(const QString &language) override
    {
        Q_EMIT m_owner.preferredLanguageChanged(language);
    }

    void zwp_input_method_context_v1_commit_state(uint32_t serial) override
    {
        qCDebug(lcKWinInput) << "commit_state" << serial;
        m_session.commitState(serial);
    }

    KWinInputMethodV1Session &m_session;
    KWinInputMethodV1Connection &m_owner;
};

KWinInputMethodV1Connection::KWinInputMethodV1Connection(KWinInputMethodV1Backend &backend)
    : QWaylandClientExtensionTemplate<KWinInputMethodV1Connection>(1)
    , m_session(backend)
{
}

KWinInputMethodV1Connection::~KWinInputMethodV1Connection()
{
    if (m_context) {
        m_session.deactivate(*m_context);
    }
}

bool KWinInputMethodV1Connection::hasContext() const
{
    return bool(m_context);
}

void KWinInputMethodV1Connection::zwp_input_method_v1_activate(::zwp_input_method_context_v1 *id)
{
    if (m_context) {
        m_session.deactivate(*m_context);
        m_context.reset();
    }

    qCDebug(lcKWinInput) << "activate";
    m_context = std::make_unique<ProtocolInputMethodV1Context>(id, m_session, *this);
    m_session.activate(*m_context);
    Q_EMIT contextActiveChanged(true);
}

void KWinInputMethodV1Connection::zwp_input_method_v1_deactivate(::zwp_input_method_context_v1 *context)
{
    if (!m_context || m_context->object() != context) {
        return;
    }

    qCDebug(lcKWinInput) << "deactivate";
    m_session.deactivate(*m_context);
    m_context.reset();
    Q_EMIT contextActiveChanged(false);
}

}
