// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Window

Rectangle {
    id: panelButton
    property string label: ""
    property bool accent: false
    signal triggered()

    height: root.portrait ? 52 : 44
    radius: 12
    opacity: enabled ? 1.0 : 0.42
    color: panelButtonMouse.pressed
        ? root.pressedColor
        : (accent ? root.accentColor : root.specialKeyColor)
    border.width: keyboardBridge.keyBorders ? 1 : 0
    border.color: root.borderColor

    Text {
        anchors.centerIn: parent
        text: panelButton.label
        color: panelButton.accent ? root.backgroundColor : root.textColor
        font.pixelSize: root.portrait ? 17 : 15
        font.weight: Font.Medium
    }

    MouseArea {
        id: panelButtonMouse
        anchors.fill: parent
        enabled: panelButton.enabled
        onClicked: panelButton.triggered()
    }
}
