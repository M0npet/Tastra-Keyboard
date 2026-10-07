// SPDX-License-Identifier: GPL-3.0-or-later
// Text-editing panel, loaded only while open.

import QtQuick
import QtQuick.Window

Grid {
    columns: root.portrait ? 3 : 6
    spacing: root.keyGap * 2

    PanelButton {
        width: root.portrait ? 150 : 124
        label: "↑ " + qsTr("Up")
        onTriggered: keyboardBridge.moveUp()
    }
    PanelButton {
        width: root.portrait ? 150 : 124
        label: "↓ " + qsTr("Down")
        onTriggered: keyboardBridge.moveDown()
    }
    PanelButton {
        width: root.portrait ? 150 : 124
        label: "← " + qsTr("Left")
        onTriggered: keyboardBridge.moveLeft()
    }
    PanelButton {
        width: root.portrait ? 150 : 124
        label: qsTr("Right") + " →"
        onTriggered: keyboardBridge.moveRight()
    }
    PanelButton {
        width: root.portrait ? 150 : 124
        label: qsTr("Home")
        onTriggered: keyboardBridge.moveHome()
    }
    PanelButton {
        width: root.portrait ? 150 : 124
        label: qsTr("End")
        onTriggered: keyboardBridge.moveEnd()
    }
    PanelButton {
        width: root.portrait ? 150 : 124
        label: qsTr("Delete")
        onTriggered: keyboardBridge.deleteForward()
    }
    PanelButton {
        width: root.portrait ? 150 : 124
        label: qsTr("Backspace")
        onTriggered: keyboardBridge.backspace()
    }
}
