// SPDX-License-Identifier: GPL-3.0-or-later
// Emoji panel, loaded only while open.

import QtQuick
import QtQuick.Window

Column {
    anchors.fill: parent
    spacing: root.keyGap

    Rectangle {
        width: parent.width
        height: root.portrait ? 44 : 38
        radius: 11
        color: root.keyColor
        border.width: keyboardBridge.keyBorders ? 1 : 0
        border.color: root.borderColor

        // Gboard: tapping the search field brings the letters back
        // and types into the search (see emojiSearchBar).
        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 12
            text: "\uD83D\uDD0D  " + qsTr("Search emoji")
            color: root.secondaryTextColor
            font.pixelSize: root.portrait ? 15 : 13
        }
        MouseArea {
            objectName: "emojiSearchField"
            anchors.fill: parent
            onClicked: keyboardBridge.startEmojiSearch()
        }
    }

    Flickable {
        width: parent.width
        height: root.portrait ? 46 : 40
        contentWidth: emojiCategoriesRow.implicitWidth
        contentHeight: height
        clip: true

        Row {
            id: emojiCategoriesRow
            spacing: 8
            Repeater {
                model: keyboardBridge.emojiCategories
                delegate: Rectangle {
                    objectName: "emojiCategory_" + modelData
                    height: root.portrait ? 40 : 34
                    width: categoryText.implicitWidth + 24
                    radius: height / 2
                    color: root.emojiCategory === modelData ? root.selectedColor : root.keyColor
                    Text {
                        id: categoryText
                        anchors.centerIn: parent
                        text: modelData
                        color: root.emojiCategory === modelData ? root.accentColor : root.textColor
                        font.pixelSize: root.portrait ? 14 : 12
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.emojiCategory = modelData
                    }
                }
            }
        }
    }

    GridView {
        // Gboard's ":-)" tab: text faces get wide tiles.
        readonly property bool faces: root.emojiCategory === ":-)"
        width: parent.width
        height: parent.height - (root.portrait ? 100 : 88)
        clip: true
        cellWidth: faces ? Math.floor(width / (root.portrait ? 3 : 5)) : (root.portrait ? 58 : 50)
        cellHeight: root.portrait ? 58 : 50
        model: (keyboardBridge.emojiCategories, keyboardBridge.emojiSearch("", root.emojiCategory))

        delegate: Rectangle {
            width: GridView.view.cellWidth - root.keyGap
            height: GridView.view.cellHeight - root.keyGap
            radius: 12
            color: emojiMouse.pressed ? root.specialKeyColor : root.keyColor
            border.width: keyboardBridge.keyBorders ? 1 : 0
            border.color: root.borderColor

            Text {
                anchors.centerIn: parent
                text: modelData
                color: root.textColor
                font.pixelSize: parent.GridView.view.faces ? (root.portrait ? 18 : 16) : (root.portrait ? 28 : 24)
                // A long face shrinks to fit its tile.
                width: parent.GridView.view.faces ? parent.width - 8 : implicitWidth
                horizontalAlignment: Text.AlignHCenter
                fontSizeMode: parent.GridView.view.faces ? Text.HorizontalFit : Text.FixedSize
                minimumPixelSize: 10
            }

            MouseArea {
                id: emojiMouse
                anchors.fill: parent
                property bool picked: false
                onPressed: picked = false
                // Gboard: long-press an emoji for its skin tones.
                onPressAndHold: {
                    const tones = keyboardBridge.emojiSkinTones(modelData)
                    if (tones.length === 0) return
                    picked = true
                    root.openTonePicker(modelData, tones, parent)
                }
                onClicked: {
                    if (picked) return
                    if (parent.GridView.view.faces) keyboardBridge.insertEmoticon(modelData)
                    else keyboardBridge.insertEmoji(modelData)
                }
            }
        }
    }
}
