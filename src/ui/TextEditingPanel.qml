// SPDX-License-Identifier: GPL-3.0-or-later
// Text-editing panel, loaded only while open.

import QtQuick
import QtQuick.Window

Grid {
    id: editGrid
    // All buttons on the panel: as many rows as its height takes, then as
    // many columns as the buttons need (narrower buttons when upright).
    readonly property real availableWidth: parent && parent.parent ? parent.parent.width - 4 * root.keyGap : 0
    readonly property real availableHeight: parent && parent.parent ? parent.parent.height : 0
    readonly property real buttonHeight: root.portrait ? 52 : 44
    readonly property real preferredWidth: root.portrait ? 150 : 124
    readonly property int shownButtons: {
        var n = 0
        for (var i = 0; i < children.length; ++i) if (children[i].visible) ++n
        return n
    }
    readonly property int rowsThatFit: Math.max(1, Math.floor((availableHeight + spacing) / (buttonHeight + spacing)))
    readonly property int columnsThatFit: Math.max(2, Math.min(6, Math.floor((availableWidth + spacing) / (preferredWidth + spacing))))
    columns: Math.max(columnsThatFit, Math.ceil(shownButtons / rowsThatFit))
    readonly property real buttonWidth: Math.max(80, Math.min(preferredWidth, (availableWidth - (columns - 1) * spacing) / columns))
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
        enabled: keyboardBridge.canCut
        onTriggered: keyboardBridge.cutSelection()
    }
    PanelButton {
        objectName: "pasteButton"
        width: editGrid.buttonWidth
        label: qsTr("Paste")
        enabled: keyboardBridge.hasClipboardText
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
