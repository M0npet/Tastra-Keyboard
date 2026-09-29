// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

namespace V3Keyboard
{

class InputMethodBackend;

class KeyboardController
{
public:
    explicit KeyboardController(InputMethodBackend &backend);

    void tapText(const QString &text);
    void backspace();
    void deleteForward();
    void moveLeft();
    void moveRight();
    void moveHome();
    void moveEnd();
    void enter();

private:
    InputMethodBackend &m_backend;
};

}
