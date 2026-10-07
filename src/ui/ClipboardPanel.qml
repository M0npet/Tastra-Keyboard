// SPDX-License-Identifier: GPL-3.0-or-later
// Clipboard history panel (Gboard), loaded only while open.

import QtQuick
import QtQuick.Window

Column {
    anchors.fill: parent
    spacing: root.keyGap

    Rectangle {
        id: currentClipboardCard
        width: parent.width
        height: root.portrait ? 72 : 58
        radius: 12
        color: root.keyColor
        border.width: keyboardBridge.keyBorders ? 1 : 0
        border.color: root.borderColor

        Text {
            anchors.fill: parent
            anchors.margins: 10
            text: keyboardBridge.hasClipboardText
                ? keyboardBridge.clipboardPreview
                : qsTr("Clipboard is empty")
            color: keyboardBridge.hasClipboardText ? root.textColor : root.secondaryTextColor
            font.pixelSize: root.portrait ? 15 : 13
            wrapMode: Text.Wrap
            elide: Text.ElideRight
            maximumLineCount: 2
        }
    }

    Row {
        id: clipboardControls
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: root.keyGap

        PanelButton {
            width: root.portrait ? 120 : 108
            label: qsTr("Paste")
            accent: true
            enabled: keyboardBridge.hasClipboardText
            onTriggered: keyboardBridge.pasteClipboard()
        }
        PanelButton {
            width: root.portrait ? 150 : 132
            label: qsTr("Clear history")
            enabled: keyboardBridge.clipboardHistory.length > 0
            onTriggered: keyboardBridge.clearClipboardHistory()
        }
    }

    ListView {
        width: parent.width
        height: parent.height - currentClipboardCard.height - clipboardControls.height - root.keyGap * 2
        clip: true
        spacing: root.keyGap
        model: keyboardBridge.clipboardHistory

        delegate: Rectangle {
            required property string modelData
            required property int index
            width: ListView.view.width
            height: root.portrait ? 66 : 54
            radius: 12
            color: historyMouse.pressed ? root.specialKeyColor : root.keyColor
            border.width: keyboardBridge.keyBorders ? 1 : 0
            border.color: root.borderColor

            Text {
                anchors.left: parent.left
                anchors.right: removeHistory.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 12
                anchors.rightMargin: 8
                // Long-press pins (Gboard): pinned items stay past the hour.
                objectName: "clipboardItem"
                text: (keyboardBridge.clipboardHistory, keyboardBridge.isClipboardPinned(modelData) ? "\uD83D\uDCCC " : "") + modelData
                color: root.textColor
                font.pixelSize: root.portrait ? 15 : 13
                elide: Text.ElideRight
                maximumLineCount: 2
                wrapMode: Text.Wrap
            }

            Rectangle {
                id: removeHistory
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                width: root.portrait ? 40 : 34
                height: width
                radius: width / 2
                color: removeHistoryMouse.pressed ? root.borderColor : "transparent"
                Text {
                    anchors.centerIn: parent
                    text: "×"
                    color: root.secondaryTextColor
                    font.pixelSize: root.portrait ? 22 : 19
                }
                MouseArea {
                    id: removeHistoryMouse
                    anchors.fill: parent
                    onClicked: keyboardBridge.removeClipboardHistory(index)
                }
            }

            MouseArea {
                id: historyMouse
                anchors.left: parent.left
                anchors.right: removeHistory.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                property bool pinToggled: false
                onPressed: pinToggled = false
                onPressAndHold: {
                    pinToggled = true
                    keyboardBridge.toggleClipboardPin(modelData)
                }
                onClicked: {
                    if (pinToggled) return
                    keyboardBridge.pasteClipboardHistory(index)
                }
            }
        }
    }
}
