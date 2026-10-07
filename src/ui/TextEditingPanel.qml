// SPDX-License-Identifier: GPL-3.0-or-later
// Text-editing panel, loaded only while open.

import QtQuick
import QtQuick.Window

Grid {
    id: editGrid
    // As many columns as fit the panel (at most 3 upright, 6 across).
    readonly property real availableWidth: parent && parent.parent ? parent.parent.width - 4 * root.keyGap : 0
    readonly property real buttonWidth: root.portrait ? 150 : 124
    columns: Math.max(2, Math.min(root.portrait ? 3 : 6, Math.floor((availableWidth + spacing) / (buttonWidth + spacing))))
    spacing: root.keyGap * 2

    PanelButton {
        width: editGrid.buttonWidth
        label: "↑ " + qsTr("Up")
        onTriggered: keyboardBridge.moveUp()
    }
    PanelButton {
        width: editGrid.buttonWidth
        label: "↓ " + qsTr("Down")
        onTriggered: keyboardBridge.moveDown()
    }
    PanelButton {
        width: editGrid.buttonWidth
        label: "← " + qsTr("Left")
        onTriggered: keyboardBridge.moveLeft()
    }
    PanelButton {
        width: editGrid.buttonWidth
        label: qsTr("Right") + " →"
        onTriggered: keyboardBridge.moveRight()
    }
    PanelButton {
        width: editGrid.buttonWidth
        label: qsTr("Home")
        onTriggered: keyboardBridge.moveHome()
    }
    PanelButton {
        width: editGrid.buttonWidth
        label: qsTr("End")
        onTriggered: keyboardBridge.moveEnd()
    }
    // Gboard's Select / Select all / Copy / Cut / Paste. With KWin's fake
    // input the keyboard sends the shortcuts (Shift+arrows, Ctrl+A, Ctrl+C,
    // Ctrl+X); otherwise Copy and Cut take the selection the application
    // reports (select by touch there).
    PanelButton {
        objectName: "selectButton"
        visible: keyboardBridge.canSelect
        width: editGrid.buttonWidth
        label: qsTr("Select")
        accent: keyboardBridge.selectMode
        onTriggered: keyboardBridge.toggleSelectMode()
    }
    PanelButton {
        objectName: "selectAllButton"
        visible: keyboardBridge.canSelect
        width: editGrid.buttonWidth
        label: qsTr("Select all")
        onTriggered: keyboardBridge.selectAll()
    }
    PanelButton {
        objectName: "copyButton"
        width: editGrid.buttonWidth
        label: qsTr("Copy")
        enabled: keyboardBridge.canCopy
        onTriggered: keyboardBridge.copySelection()
    }
    PanelButton {
        objectName: "cutButton"
        width: editGrid.buttonWidth
        label: qsTr("Cut")
        enabled: keyboardBridge.canCopy
        onTriggered: keyboardBridge.cutSelection()
    }
    PanelButton {
        objectName: "pasteButton"
        width: editGrid.buttonWidth
        label: qsTr("Paste")
        enabled: keyboardBridge.clipboardText.length > 0
        onTriggered: keyboardBridge.pasteClipboard()
    }
    PanelButton {
        objectName: "undoButton"
        visible: keyboardBridge.canSelect
        width: editGrid.buttonWidth
        label: "↶ " + qsTr("Undo")
        onTriggered: keyboardBridge.undo()
    }
    PanelButton {
        objectName: "redoButton"
        visible: keyboardBridge.canSelect
        width: editGrid.buttonWidth
        label: "↷ " + qsTr("Redo")
        onTriggered: keyboardBridge.redo()
    }
    PanelButton {
        width: editGrid.buttonWidth
        label: qsTr("Delete")
        onTriggered: keyboardBridge.deleteForward()
    }
    PanelButton {
        width: editGrid.buttonWidth
        label: qsTr("Backspace")
        onTriggered: keyboardBridge.backspace()
    }
}
