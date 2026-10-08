// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// A text field in a GTK 3 application (or Firefox) behind KWin, driven by the
// keyboard through input-method-unstable-v1, for tests. Modelled on the
// sources (read 2026-10-08):
// - KWin src/inputmethod.cpp (master): commit_string becomes text-input-v3
//   preedit_string("") + commit_string + done; preedit_string becomes
//   preedit_string + done; delete_surrounding_text becomes
//   delete_surrounding_text + done; a keysym becomes a key event on
//   wl_keyboard (forwardKeySym).
// - GTK 3 modules/input/imwayland.c: at done, the pending deletion, commit
//   and preedit are applied in that order, and the surrounding text is
//   retrieved from the widget and sent back (notify_im_change). A deletion
//   arrives in bytes and is turned into characters with the surrounding
//   text GTK has (text_input_delete_surrounding_text); a deletion longer
//   than the text GTK has before its cursor would read before that text.
// - GDK queues key events and handles them after the Wayland events read
//   in the same dispatch, so text-input events sent after a key event are
//   applied before it when both arrive together.
// - Firefox (widget/gtk/IMContextWrapper.cpp) answers GTK from a content
//   cache that lags behind the page: Echo::Lagging sends back the text as it
//   was before the last batch.
//
// deliver() hands everything the keyboard sent since the last call to the
// client as one batch, then lets the keyboard see the client's answers.

#include <functional>

#include <QList>
#include <QString>

#include "core/inputmethodbackend.h"
#include "core/typingengine.h"

class TextInputClient final : public Tastra::InputMethodBackend
{
public:
    enum class Echo { Current, Lagging, None };

    explicit TextInputClient(Echo echo = Echo::Current) : m_echoMode(echo) {}

    // The keyboard's requests.
    void commitText(const QString &text) override { m_wire.append({Op::Commit, text, 0, 0}); }
    bool setPreedit(const QString &text) override
    {
        if (!preeditSupport) return false;
        m_wire.append({Op::Preedit, text, 0, 0});
        return true;
    }
    bool deleteBeforeCursor(const QString &text) override
    {
        if (text.isEmpty()) return false;
        m_wire.append({Op::Delete, {}, int(text.toUtf8().size()), 0});
        return true;
    }
    bool deleteAroundCursor(const QString &before, const QString &after) override
    {
        m_wire.append({Op::Delete, {}, int(before.toUtf8().size()), int(after.toUtf8().size())});
        return true;
    }
    void backspace() override { m_wire.append({Op::Key, QStringLiteral("BackSpace"), 0, 0}); }
    void deleteForward() override { m_wire.append({Op::Key, QStringLiteral("Delete"), 0, 0}); }
    void moveLeft() override { m_wire.append({Op::Key, QStringLiteral("Left"), 0, 0}); }
    void moveRight() override { m_wire.append({Op::Key, QStringLiteral("Right"), 0, 0}); }
    void moveHome() override { m_wire.append({Op::Key, QStringLiteral("Home"), 0, 0}); }
    void moveEnd() override { m_wire.append({Op::Key, QStringLiteral("End"), 0, 0}); }
    void enter() override { m_wire.append({Op::Key, QStringLiteral("Return"), 0, 0}); }

    // The client reads and handles one batch, then answers (unless the
    // answer is still on its way: `answer` false).
    void deliver(bool answer = true)
    {
        const QList<Op> batch = m_wire;
        m_wire.clear();
        QList<Snapshot> echoes;
        QList<Op> keys;
        for (const Op &op : batch) {
            if (op.kind == Op::Key) {
                keys.append(op);                  // GDK handles these later
                continue;
            }
            // done: GTK applies the request; when something changed (any
            // commit, even an empty one, a different preedit, a deletion) it
            // asks the widget for its text and sends that back.
            bool changed = true;
            if (op.kind == Op::Delete) applyDelete(op.before, op.after);
            if (op.kind == Op::Commit) {
                preedit.clear();
                insert(op.text);
            }
            if (op.kind == Op::Preedit) {
                changed = preedit != op.text;
                preedit = op.text;
            }
            if (!changed) continue;
            m_gtkSurrounding = m_echoMode == Echo::Lagging ? m_cache : current();
            echoes.append(m_gtkSurrounding);
        }
        for (const Op &key : keys) applyKey(key.text);
        // A GTK widget resets its input context around edits made by keys
        // (gtk_im_context_reset -> notify_im_change): approximated as one
        // answer after them. Firefox answers from its cache: nothing new.
        if (!keys.isEmpty() && m_echoMode == Echo::Current) {
            m_gtkSurrounding = current();
            echoes.append(m_gtkSurrounding);
        }
        // The page catches up after the batch (Firefox's content cache).
        m_cache = current();
        if (!engine || !answer || m_echoMode == Echo::None) return;
        for (const Snapshot &s : echoes) {
            if (advance) advance(5);
            engine->syncSurroundingText(s.text, s.cursorBytes(), s.cursorBytes());
        }
    }

    // The user puts the cursor somewhere (a tap in the text): the client
    // reports it at human speed.
    void placeCursor(int position)
    {
        cursor = qBound(0, position, int(text.size()));
        preedit.clear();
        m_cache = current();
        m_gtkSurrounding = current();
        if (advance) advance(1000);
        if (engine && m_echoMode != Echo::None) {
            engine->syncSurroundingText(m_cache.text, m_cache.cursorBytes(), m_cache.cursorBytes());
        }
    }

    void setText(const QString &value)
    {
        text = value;
        placeCursor(int(value.size()));
    }

    QString visible() const { return text.left(cursor) + preedit + text.mid(cursor); }
    bool pending() const { return !m_wire.isEmpty(); }

    QString text;
    int cursor = 0;
    QString preedit;
    bool preeditSupport = true;
    // A deletion GTK could not map onto the text it has (it would read memory
    // before that text) or that the widget refused.
    int badDeletes = 0;
    Tastra::TypingEngine *engine = nullptr;
    std::function<void(qint64)> advance;

private:
    struct Op {
        enum Kind { Commit, Preedit, Delete, Key } kind;
        QString text;
        int before;
        int after;
    };
    struct Snapshot {
        QString text;
        int cursor = 0;
        int cursorBytes() const { return int(text.left(cursor).toUtf8().size()); }
    };

    Snapshot current() const { return {text, cursor}; }

    void insert(const QString &value)
    {
        text.insert(cursor, value);
        cursor += int(value.size());
    }

    void applyDelete(int beforeBytes, int afterBytes)
    {
        // GTK: bytes -> characters over the surrounding text it has.
        const QByteArray known = m_gtkSurrounding.text.toUtf8();
        const int knownCursor = m_gtkSurrounding.cursorBytes();
        if (beforeBytes > knownCursor || knownCursor + afterBytes > known.size()) {
            ++badDeletes;
            return;
        }
        const int charsBefore = int(QString::fromUtf8(known.mid(knownCursor - beforeBytes, beforeBytes)).size());
        const int charsAfter = int(QString::fromUtf8(known.mid(knownCursor, afterBytes)).size());
        // The widget deletes around its own cursor.
        if (charsBefore > cursor || cursor + charsAfter > text.size()) {
            ++badDeletes;
            return;
        }
        text.remove(cursor - charsBefore, charsBefore + charsAfter);
        cursor -= charsBefore;
    }

    void applyKey(const QString &key)
    {
        if (key == QLatin1String("BackSpace")) {
            if (cursor > 0) {
                text.remove(cursor - 1, 1);
                --cursor;
            }
        } else if (key == QLatin1String("Delete")) {
            if (cursor < text.size()) text.remove(cursor, 1);
        } else if (key == QLatin1String("Left")) {
            cursor = qMax(0, cursor - 1);
        } else if (key == QLatin1String("Right")) {
            cursor = qMin(int(text.size()), cursor + 1);
        } else if (key == QLatin1String("Home")) {
            cursor = 0;
        } else if (key == QLatin1String("End")) {
            cursor = int(text.size());
        } else if (key == QLatin1String("Return")) {
            insert(QStringLiteral("\n"));
        }
    }

    Echo m_echoMode;
    QList<Op> m_wire;
    Snapshot m_cache;
    Snapshot m_gtkSurrounding;
};
