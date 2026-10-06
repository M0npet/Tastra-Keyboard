// SPDX-License-Identifier: GPL-3.0-or-later
// Text-editing panel, loaded only while open.

import QtQuick
import QtQuick.Window

Grid {
    columns: root.portrait ? 3 : 6
    spacing: root.keyGap * 2

    PanelButton {
        width: root.portrait ? 150 : 124
        label: "↑ Up"
        onTriggered: keyboardBridge.moveUp()
    }
    PanelButton {
        width: root.portrait ? 150 : 124
        label: "↓ Down"
        onTriggered: keyboardBridge.moveDown()
    }
    PanelButton {
        width: root.portrait ? 150 : 124
        label: "← Left"
        onTriggered: keyboardBridge.moveLeft()
    }
    PanelButton {
        width: root.portrait ? 150 : 124
        label: "Right →"
        onTriggered: keyboardBridge.moveRight()
    }
    PanelButton {
        width: root.portrait ? 150 : 124
        label: "Home"
        onTriggered: keyboardBridge.moveHome()
    }
    PanelButton {
        width: root.portrait ? 150 : 124
        label: "End"
        onTriggered: keyboardBridge.moveEnd()
    }
    PanelButton {
        width: root.portrait ? 150 : 124
        label: "Delete"
        onTriggered: keyboardBridge.deleteForward()
    }
    PanelButton {
        width: root.portrait ? 150 : 124
        label: "Backspace"
        onTriggered: keyboardBridge.backspace()
    }
}
