// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Window

Rectangle {
    id: root

    readonly property bool portrait: Screen.height > Screen.width
    readonly property var metrics: {
        var dependency = keyboardBridge.keyScale
        return keyboardBridge.layoutMetrics(portrait)
    }
    readonly property real keyGap: metrics.keyGap
    readonly property real keyHeight: metrics.keyHeight
    readonly property real toolbarHeight: portrait ? 54 : 48
    readonly property real contentWidth: Math.min(
        width - metrics.outerMargin * 2,
        width * metrics.contentWidthRatio,
        metrics.maxContentWidth
    )
    readonly property real baseKeyWidth: (contentWidth - keyGap * 9) / 10
    readonly property color backgroundColor: keyboardBridge.amoled ? "#000000" : "#202124"
    readonly property color panelColor: keyboardBridge.amoled ? "#080808" : "#292a2d"
    readonly property color keyColor: keyboardBridge.amoled ? "#171717" : "#3c4043"
    readonly property color specialKeyColor: keyboardBridge.amoled ? "#242424" : "#5f6368"
    readonly property color borderColor: keyboardBridge.amoled ? "#333333" : "#56595e"
    readonly property color textColor: "#f1f3f4"
    readonly property color accentColor: "#8ab4f8"
    property bool toolbarExpanded: false
    property string emojiCategory: "All"
    property var glideKeys: []
    property var glideStartItem: null
    property real glideStartX: 0
    property real glideStartY: 0
    property bool glideActive: false
    property string glideLastValue: ""

    width: Screen.width
    height: toolbarHeight + metrics.panelHeight
    color: backgroundColor

    function dynamicKeyWidth(count) {
        return (contentWidth - keyGap * (count - 1)) / count
    }

    function toolbarGlyph(id) {
        if (id === "clipboard") return "▣"
        if (id === "emoji") return "☺"
        if (id === "text-editing") return "↔"
        if (id === "settings") return "⚙"
        return "•"
    }

    function registerGlideKey(item) {
        if (glideKeys.indexOf(item) < 0) glideKeys.push(item)
    }

    function unregisterGlideKey(item) {
        var i = glideKeys.indexOf(item)
        if (i >= 0) glideKeys.splice(i, 1)
    }

    function glideHit(px, py) {
        for (var i = 0; i < glideKeys.length; ++i) {
            var k = glideKeys[i]
            if (!k || !k.visible || !k.glideEligible) continue
            var local = k.mapFromItem(root, px, py)
            if (local.x >= 0 && local.y >= 0 && local.x <= k.width && local.y <= k.height) return k
        }
        return null
    }

    function beginGlideCandidate(item, x, y) {
        if (!keyboardBridge.glideEnabled || keyboardBridge.symbolsActive || !item.glideEligible) return
        var p = item.mapToItem(root, x, y)
        glideStartItem = item
        glideStartX = p.x
        glideStartY = p.y
        glideActive = false
        glideLastValue = ""
    }

    function updateGlideCandidate(item, x, y) {
        if (!glideStartItem || !keyboardBridge.glideEnabled) return
        var p = item.mapToItem(root, x, y)
        var dx = p.x - glideStartX
        var dy = p.y - glideStartY
        if (!glideActive && Math.sqrt(dx * dx + dy * dy) > 18) {
            glideActive = true
            glideStartItem.consumeRelease = true
            item.consumeRelease = true
            keyboardBridge.beginGlide(glideStartItem.glideValue)
            glideLastValue = glideStartItem.glideValue
        }
        if (!glideActive) return
        item.consumeRelease = true
        var hit = glideHit(p.x, p.y)
        if (hit && hit.glideValue !== glideLastValue) {
            keyboardBridge.glideThrough(hit.glideValue)
            glideLastValue = hit.glideValue
        }
    }

    function finishGlideCandidate(item) {
        if (glideActive) {
            item.consumeRelease = true
            keyboardBridge.endGlide()
        }
        glideStartItem = null
        glideActive = false
        glideLastValue = ""
    }

    component Key: Rectangle {
        id: key

        property string label: ""
        property string iconSource: ""
        property string alternate: ""
        property real preferredWidth: root.baseKeyWidth
        property bool special: false
        property bool accent: false
        property bool popupEnabled: keyboardBridge.keyPopups
        property bool longPressEnabled: alternate.length > 0
        property bool consumeRelease: false
        property bool glideEligible: false
        property string glideValue: ""

        signal triggered()
        signal longPressed()
        signal pressStarted(real x, real y)
        signal pointerMoved(real x, real y)
        signal pressEnded(real x, real y)

        width: preferredWidth
        height: root.keyHeight
        radius: root.metrics.keyRadius
        border.width: keyboardBridge.keyBorders ? 1 : 0
        border.color: root.borderColor

        color: mouse.pressed
            ? (accent ? "#9fc3ff" : "#60656a")
            : (accent
                ? root.accentColor
                : (special ? root.specialKeyColor : root.keyColor))

        scale: mouse.pressed ? 0.965 : 1.0

        Behavior on scale {
            NumberAnimation { duration: 45 }
        }

        Text {
            anchors.centerIn: parent
            visible: key.iconSource.length === 0
            text: key.label
            color: key.accent ? root.backgroundColor : root.textColor
            font.pixelSize: root.metrics.fontSize
            font.weight: Font.Medium
        }

        Image {
            anchors.centerIn: parent
            visible: key.iconSource.length > 0
            source: key.iconSource
            width: 30
            height: 30
            fillMode: Image.PreserveAspectFit
            sourceSize.width: 40
            sourceSize.height: 40
        }

        Rectangle {
            id: popup
            visible: mouse.pressed
                && key.popupEnabled
                && key.label.length > 0
                && key.iconSource.length === 0
            z: 100
            width: Math.max(key.width * 0.82, 58)
            height: root.metrics.popupHeight
            radius: 14
            color: root.specialKeyColor
            border.width: keyboardBridge.keyBorders ? 1 : 0
            border.color: root.borderColor
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.top
            anchors.bottomMargin: 7

            Text {
                anchors.centerIn: parent
                text: mouse.held && key.alternate.length > 0 ? key.alternate : key.label
                color: "#ffffff"
                font.pixelSize: root.metrics.fontSize + 5
                font.weight: Font.Medium
            }
        }

        MouseArea {
            id: mouse
            anchors.fill: parent
            property bool held: false

            onPressed: {
                held = false
                key.consumeRelease = false
                key.pressStarted(mouse.x, mouse.y)
            }
            onPositionChanged: {
                if (pressed) key.pointerMoved(mouse.x, mouse.y)
            }
            onPressAndHold: {
                if (key.longPressEnabled) {
                    held = true
                    key.longPressed()
                }
            }
            onReleased: key.pressEnded(mouse.x, mouse.y)
            onClicked: {
                if (!held && !key.consumeRelease) {
                    key.triggered()
                }
            }
        }
    }

    component PanelButton: Rectangle {
        id: panelButton
        property string label: ""
        property bool accent: false
        signal triggered()

        height: root.portrait ? 52 : 44
        radius: 12
        opacity: enabled ? 1.0 : 0.42
        color: panelButtonMouse.pressed
            ? "#60656a"
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

    Rectangle {
        id: topToolbar
        z: 120
        width: root.contentWidth
        height: root.toolbarHeight
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        color: root.backgroundColor

        Item {
            anchors.fill: parent

            Row {
                id: suggestionRow
                visible: keyboardBridge.activePanel === "typing"
                    && keyboardBridge.suggestionsEnabled
                    && keyboardBridge.suggestions.length > 0
                    && !root.toolbarExpanded
                anchors.centerIn: parent
                spacing: root.keyGap

                Repeater {
                    model: keyboardBridge.suggestions

                    delegate: Rectangle {
                        width: Math.min(root.portrait ? 180 : 220,
                            (root.contentWidth - toolToggle.width - root.keyGap * 4) / Math.max(1, keyboardBridge.suggestions.length))
                        height: root.portrait ? 42 : 38
                        radius: 12
                        color: suggestionMouse.pressed ? root.specialKeyColor : "transparent"

                        Text {
                            anchors.centerIn: parent
                            width: parent.width - 12
                            horizontalAlignment: Text.AlignHCenter
                            text: modelData
                            elide: Text.ElideRight
                            color: root.textColor
                            font.pixelSize: root.portrait ? 17 : 15
                            font.weight: Font.Medium
                        }

                        MouseArea {
                            id: suggestionMouse
                            anchors.fill: parent
                            onClicked: {
                                keyboardBridge.selectSuggestion(modelData)
                                root.toolbarExpanded = false
                            }
                        }
                    }
                }

                Rectangle {
                    id: toolToggle
                    width: root.portrait ? 46 : 42
                    height: root.portrait ? 42 : 38
                    radius: 12
                    color: toolToggleMouse.pressed ? root.specialKeyColor : "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "•••"
                        color: root.textColor
                        font.pixelSize: root.portrait ? 18 : 16
                    }
                    MouseArea {
                        id: toolToggleMouse
                        anchors.fill: parent
                        onClicked: root.toolbarExpanded = true
                    }
                }
            }

            Row {
                visible: !suggestionRow.visible
                anchors.centerIn: parent
                spacing: root.portrait ? 18 : 14

                Rectangle {
                    visible: root.toolbarExpanded && keyboardBridge.suggestions.length > 0
                    width: root.portrait ? 52 : 46
                    height: root.portrait ? 42 : 38
                    radius: 12
                    color: backToolsMouse.pressed ? root.specialKeyColor : "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "‹"
                        color: root.textColor
                        font.pixelSize: root.portrait ? 28 : 24
                    }
                    MouseArea {
                        id: backToolsMouse
                        anchors.fill: parent
                        onClicked: root.toolbarExpanded = false
                    }
                }

                Repeater {
                    model: keyboardBridge.toolbarActions

                    delegate: Rectangle {
                        required property var modelData
                        readonly property bool selected: keyboardBridge.activePanel === modelData.id
                        width: root.portrait ? 52 : 46
                        height: root.portrait ? 42 : 38
                        radius: 12
                        color: toolbarMouse.pressed
                            ? root.specialKeyColor
                            : (selected ? "#465c78" : "transparent")
                        border.width: selected && keyboardBridge.keyBorders ? 1 : 0
                        border.color: root.accentColor
                        opacity: modelData.enabled ? 1.0 : 0.40

                        Text {
                            anchors.centerIn: parent
                            text: root.toolbarGlyph(modelData.id)
                            color: selected ? root.accentColor : root.textColor
                            font.pixelSize: root.portrait ? 24 : 21
                            font.weight: Font.Medium
                        }

                        MouseArea {
                            id: toolbarMouse
                            anchors.fill: parent
                            enabled: modelData.enabled
                            onClicked: {
                                root.toolbarExpanded = false
                                if (keyboardBridge.activePanel === modelData.id) {
                                    keyboardBridge.closePanel()
                                } else {
                                    keyboardBridge.activateToolbarAction(modelData.id)
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: languageChooserPanel
        z: 150
        visible: keyboardBridge.activePanel === "language"

        width: Math.min(420, Math.max(330, root.contentWidth * 0.30))
        height: (root.portrait ? 50 : 44)
            + keyboardBridge.languageCodes.length * (root.portrait ? 58 : 50)
            + root.keyGap * 5

        x: Math.max(
            root.metrics.outerMargin,
            Math.min(
                root.width - width - root.metrics.outerMargin,
                globeKey.mapToItem(root, 0, 0).x + globeKey.width / 2 - width / 2
            )
        )
        y: Math.max(
            root.toolbarHeight + root.metrics.topPadding,
            globeKey.mapToItem(root, 0, 0).y - height - root.keyGap
        )

        radius: 18
        color: root.panelColor
        border.width: 1
        border.color: root.borderColor

        Column {
            anchors.fill: parent
            anchors.margins: root.keyGap
            spacing: root.keyGap

            Item {
                width: parent.width
                height: root.portrait ? 42 : 36

                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Keyboard language"
                    color: root.textColor
                    font.pixelSize: root.portrait ? 19 : 17
                    font.weight: Font.Medium
                }

                Rectangle {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.height
                    height: parent.height
                    radius: height / 2
                    color: closeLanguageMouse.pressed ? root.specialKeyColor : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "×"
                        color: root.textColor
                        font.pixelSize: root.portrait ? 26 : 23
                    }

                    MouseArea {
                        id: closeLanguageMouse
                        anchors.fill: parent
                        onClicked: keyboardBridge.closePanel()
                    }
                }
            }

            Repeater {
                model: keyboardBridge.languageCodes

                delegate: Rectangle {
                    width: parent.width
                    height: root.portrait ? 58 : 50
                    radius: 13
                    color: keyboardBridge.languageCode === modelData
                        ? "#465c78"
                        : (languageMouse.pressed ? root.keyColor : "transparent")

                    Rectangle {
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        width: root.portrait ? 38 : 34
                        height: width
                        radius: width / 2
                        color: keyboardBridge.languageCode === modelData ? "#4f7db8" : root.keyColor

                        Text {
                            anchors.centerIn: parent
                            text: modelData.toUpperCase()
                            color: root.textColor
                            font.pixelSize: root.portrait ? 13 : 12
                            font.weight: Font.Medium
                        }
                    }

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: root.portrait ? 60 : 54
                        anchors.verticalCenter: parent.verticalCenter
                        text: keyboardBridge.languageLabels[index]
                        color: root.textColor
                        font.pixelSize: root.portrait ? 18 : 16
                        font.weight: Font.Medium
                    }

                    Text {
                        anchors.right: parent.right
                        anchors.rightMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        visible: keyboardBridge.languageCode === modelData
                        text: "✓"
                        color: root.accentColor
                        font.pixelSize: 22
                    }

                    MouseArea {
                        id: languageMouse
                        anchors.fill: parent
                        onClicked: {
                            keyboardBridge.setLanguage(modelData)
                            keyboardBridge.closePanel()
                        }
                    }
                }
            }
        }
    }

    Column {
        id: keyboardRows
        anchors {
            horizontalCenter: parent.horizontalCenter
            top: parent.top
            topMargin: root.toolbarHeight + root.metrics.topPadding
        }
        spacing: root.keyGap

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: root.keyGap

            Repeater {
                model: keyboardBridge.symbolsActive
                    ? ["1", "2", "3", "4", "5", "6", "7", "8", "9", "0"]
                    : keyboardBridge.row1

                delegate: Key {
                    id: row1LetterKey
                    preferredWidth: root.dynamicKeyWidth(keyboardBridge.symbolsActive ? 10 : keyboardBridge.row1.length)
                    label: keyboardBridge.symbolsActive
                        ? modelData
                        : (keyboardBridge.uppercase ? modelData.toUpperCase() : modelData)
                    alternate: keyboardBridge.symbolsActive ? "" : keyboardBridge.alternateForKey(modelData)
                    longPressEnabled: alternate.length > 0
                    glideEligible: !keyboardBridge.symbolsActive
                    glideValue: modelData
                    Component.onCompleted: root.registerGlideKey(row1LetterKey)
                    Component.onDestruction: root.unregisterGlideKey(row1LetterKey)
                    onPressStarted: root.beginGlideCandidate(row1LetterKey, x, y)
                    onPointerMoved: root.updateGlideCandidate(row1LetterKey, x, y)
                    onPressEnded: root.finishGlideCandidate(row1LetterKey)

                    onTriggered: {
                        if (keyboardBridge.symbolsActive) keyboardBridge.tapText(modelData)
                        else keyboardBridge.tapLetter(modelData)
                    }
                    onLongPressed: keyboardBridge.tapAlternate(modelData)
                }
            }
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: root.keyGap

            Repeater {
                model: keyboardBridge.symbolsActive
                    ? ["@", "#", "$", "%", "&", "-", "+", "(", ")"]
                    : keyboardBridge.row2

                delegate: Key {
                    id: row2LetterKey
                    preferredWidth: root.dynamicKeyWidth(keyboardBridge.symbolsActive ? 9 : keyboardBridge.row2.length)
                    label: keyboardBridge.symbolsActive
                        ? modelData
                        : (keyboardBridge.uppercase ? modelData.toUpperCase() : modelData)
                    alternate: keyboardBridge.symbolsActive ? "" : keyboardBridge.alternateForKey(modelData)
                    longPressEnabled: alternate.length > 0
                    glideEligible: !keyboardBridge.symbolsActive
                    glideValue: modelData
                    Component.onCompleted: root.registerGlideKey(row2LetterKey)
                    Component.onDestruction: root.unregisterGlideKey(row2LetterKey)
                    onPressStarted: root.beginGlideCandidate(row2LetterKey, x, y)
                    onPointerMoved: root.updateGlideCandidate(row2LetterKey, x, y)
                    onPressEnded: root.finishGlideCandidate(row2LetterKey)

                    onTriggered: {
                        if (keyboardBridge.symbolsActive) keyboardBridge.tapText(modelData)
                        else keyboardBridge.tapLetter(modelData)
                    }
                    onLongPressed: keyboardBridge.tapAlternate(modelData)
                }
            }
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: root.keyGap

            Key {
                visible: !keyboardBridge.symbolsActive
                preferredWidth: root.baseKeyWidth * 1.25
                special: true
                popupEnabled: false
                iconSource: keyboardBridge.capsLock
                    ? "qrc:/v3keyboard/icons/shift-lock.svg"
                    : "qrc:/v3keyboard/icons/shift.svg"
                onTriggered: keyboardBridge.shift()
            }

            Repeater {
                model: keyboardBridge.symbolsActive
                    ? ["*", "\"", "'", ":", ";", "!", "?", "/"]
                    : keyboardBridge.row3

                delegate: Key {
                    id: row3LetterKey
                    preferredWidth: {
                        var count = keyboardBridge.symbolsActive ? 8 : keyboardBridge.row3.length
                        var sideWidth = root.baseKeyWidth * (keyboardBridge.symbolsActive ? 1.25 : 2.5)
                        var gaps = keyboardBridge.symbolsActive ? count : count + 1
                        return (root.contentWidth - sideWidth - root.keyGap * gaps) / count
                    }
                    label: keyboardBridge.symbolsActive
                        ? modelData
                        : (keyboardBridge.uppercase ? modelData.toUpperCase() : modelData)
                    alternate: keyboardBridge.symbolsActive ? "" : keyboardBridge.alternateForKey(modelData)
                    longPressEnabled: alternate.length > 0
                    glideEligible: !keyboardBridge.symbolsActive
                    glideValue: modelData
                    Component.onCompleted: root.registerGlideKey(row3LetterKey)
                    Component.onDestruction: root.unregisterGlideKey(row3LetterKey)
                    onPressStarted: root.beginGlideCandidate(row3LetterKey, x, y)
                    onPointerMoved: root.updateGlideCandidate(row3LetterKey, x, y)
                    onPressEnded: root.finishGlideCandidate(row3LetterKey)

                    onTriggered: {
                        if (keyboardBridge.symbolsActive) keyboardBridge.tapText(modelData)
                        else keyboardBridge.tapLetter(modelData)
                    }
                    onLongPressed: keyboardBridge.tapAlternate(modelData)
                }
            }

            Key {
                id: backspaceKey
                property real dragStartX: 0
                property int dragStep: 0
                preferredWidth: root.baseKeyWidth * 1.25
                special: true
                popupEnabled: false
                longPressEnabled: true
                iconSource: "qrc:/v3keyboard/icons/backspace.svg"
                onTriggered: keyboardBridge.backspace()
                onLongPressed: {
                    consumeRelease = true
                    keyboardBridge.backspaceRepeated(3)
                }
                onPressStarted: {
                    dragStartX = x
                    dragStep = 0
                }
                onPointerMoved: {
                    var step = Math.floor((dragStartX - x) / Math.max(18, width * 0.20))
                    if (step > dragStep) {
                        consumeRelease = true
                        keyboardBridge.backspaceRepeated(step - dragStep)
                        dragStep = step
                    }
                }
            }
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: root.keyGap

            Key {
                preferredWidth: root.baseKeyWidth * 1.35
                special: true
                popupEnabled: false
                label: keyboardBridge.symbolsActive ? "ABC" : "?123"
                onTriggered: keyboardBridge.toggleSymbols()
            }

            Key {
                preferredWidth: root.baseKeyWidth
                label: ","
                onTriggered: keyboardBridge.tapText(",")
            }

            Key {
                id: globeKey
                preferredWidth: root.baseKeyWidth
                label: "🌐"
                popupEnabled: false
                longPressEnabled: true
                onTriggered: keyboardBridge.nextLanguage()
                onLongPressed: keyboardBridge.openLanguagePanel()
            }

            Key {
                id: spaceKey
                property real dragStartX: 0
                property int cursorStep: 0
                preferredWidth: root.baseKeyWidth * 4.05
                label: keyboardBridge.languageLabel
                popupEnabled: false
                longPressEnabled: true
                onTriggered: keyboardBridge.space()
                onLongPressed: keyboardBridge.openLanguagePanel()
                onPressStarted: {
                    dragStartX = x
                    cursorStep = 0
                }
                onPointerMoved: {
                    var nextStep = Math.round((x - dragStartX) / Math.max(24, width * 0.08))
                    var delta = nextStep - cursorStep
                    if (delta !== 0) {
                        consumeRelease = true
                        keyboardBridge.moveCursor(delta)
                        cursorStep = nextStep
                    }
                }
            }

            Key {
                preferredWidth: root.baseKeyWidth
                label: "."
                onTriggered: keyboardBridge.tapText(".")
            }

            Key {
                preferredWidth: root.baseKeyWidth * 1.35
                accent: true
                popupEnabled: false
                iconSource: "qrc:/v3keyboard/icons/enter.svg"
                onTriggered: keyboardBridge.enter()
            }
        }
    }

    Rectangle {
        id: panelSurface
        z: 140
        visible: keyboardBridge.activePanel !== "typing"
            && keyboardBridge.activePanel !== "language"
        width: root.contentWidth
        height: root.metrics.panelHeight
        anchors.top: parent.top
        anchors.topMargin: root.toolbarHeight
        anchors.horizontalCenter: parent.horizontalCenter
        color: root.panelColor
        radius: 16
        border.width: keyboardBridge.keyBorders ? 1 : 0
        border.color: root.borderColor

        Item {
            id: panelHeader
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: root.keyGap * 2
            height: root.portrait ? 42 : 36

            Text {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                text: {
                    if (keyboardBridge.activePanel === "clipboard") return "Clipboard"
                    if (keyboardBridge.activePanel === "emoji") return "Emoji"
                    if (keyboardBridge.activePanel === "text-editing") return "Text editing"
                    if (keyboardBridge.activePanel === "settings") return "Settings"
                    return ""
                }
                color: root.textColor
                font.pixelSize: root.portrait ? 20 : 18
                font.weight: Font.DemiBold
            }

            Rectangle {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: parent.height
                height: parent.height
                radius: height / 2
                color: closePanelMouse.pressed ? root.specialKeyColor : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "×"
                    color: root.textColor
                    font.pixelSize: root.portrait ? 26 : 23
                }

                MouseArea {
                    id: closePanelMouse
                    anchors.fill: parent
                    onClicked: keyboardBridge.closePanel()
                }
            }
        }

        Item {
            id: panelBody
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: panelHeader.bottom
            anchors.bottom: parent.bottom
            anchors.margins: root.keyGap * 2

            Column {
                visible: keyboardBridge.activePanel === "clipboard"
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
                        text: keyboardBridge.clipboardText.length > 0
                            ? keyboardBridge.clipboardText
                            : "Clipboard is empty"
                        color: keyboardBridge.clipboardText.length > 0 ? root.textColor : "#9aa0a6"
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
                        label: "Paste"
                        accent: true
                        enabled: keyboardBridge.clipboardText.length > 0
                        onTriggered: keyboardBridge.pasteClipboard()
                    }
                    PanelButton {
                        width: root.portrait ? 150 : 132
                        label: "Clear history"
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
                            text: modelData
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
                                color: "#bdc1c6"
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
                            onClicked: keyboardBridge.pasteClipboardHistory(index)
                        }
                    }
                }
            }

            Column {
                visible: keyboardBridge.activePanel === "emoji"
                anchors.fill: parent
                spacing: root.keyGap

                Rectangle {
                    width: parent.width
                    height: root.portrait ? 44 : 38
                    radius: 11
                    color: root.keyColor
                    border.width: keyboardBridge.keyBorders ? 1 : 0
                    border.color: root.borderColor

                    TextInput {
                        id: emojiSearchInput
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        verticalAlignment: TextInput.AlignVCenter
                        color: root.textColor
                        selectionColor: root.accentColor
                        selectedTextColor: root.backgroundColor
                        font.pixelSize: root.portrait ? 16 : 14
                        clip: true
                        inputMethodHints: Qt.ImhNoPredictiveText
                        Text {
                            visible: emojiSearchInput.text.length === 0
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Search emoji (physical keyboard or category chips)"
                            color: "#9aa0a6"
                            font.pixelSize: root.portrait ? 15 : 13
                        }
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
                                height: root.portrait ? 40 : 34
                                width: categoryText.implicitWidth + 24
                                radius: height / 2
                                color: root.emojiCategory === modelData ? "#465c78" : root.keyColor
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
                    width: parent.width
                    height: parent.height - (root.portrait ? 100 : 88)
                    clip: true
                    cellWidth: root.portrait ? 58 : 50
                    cellHeight: cellWidth
                    model: keyboardBridge.emojiSearch(emojiSearchInput.text, root.emojiCategory)

                    delegate: Rectangle {
                        width: GridView.view.cellWidth - root.keyGap
                        height: width
                        radius: 12
                        color: emojiMouse.pressed ? root.specialKeyColor : root.keyColor
                        border.width: keyboardBridge.keyBorders ? 1 : 0
                        border.color: root.borderColor

                        Text {
                            anchors.centerIn: parent
                            text: modelData
                            font.pixelSize: root.portrait ? 28 : 24
                        }

                        MouseArea {
                            id: emojiMouse
                            anchors.fill: parent
                            onClicked: keyboardBridge.tapText(modelData)
                        }
                    }
                }
            }

            Grid {
                visible: keyboardBridge.activePanel === "text-editing"
                anchors.centerIn: parent
                columns: root.portrait ? 3 : 6
                spacing: root.keyGap * 2

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

            Flickable {
                visible: keyboardBridge.activePanel === "settings"
                anchors.fill: parent
                clip: true
                contentWidth: width
                contentHeight: settingsColumn.implicitHeight + root.keyGap * 2

                Column {
                    id: settingsColumn
                    width: Math.min(parent.width, root.portrait ? 640 : 780)
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: root.portrait ? 12 : 8

                    Row {
                        width: parent.width; spacing: 12
                        Text { width: parent.width - themeButton.width - parent.spacing; height: themeButton.height; verticalAlignment: Text.AlignVCenter; text: "Theme"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
                        PanelButton { id: themeButton; width: root.portrait ? 170 : 150; label: keyboardBridge.amoled ? "AMOLED" : "Dark"; onTriggered: keyboardBridge.setAmoled(!keyboardBridge.amoled) }
                    }

                    Row {
                        width: parent.width; spacing: 12
                        Text { width: parent.width - keySizeControls.width - parent.spacing; height: keySizeControls.height; verticalAlignment: Text.AlignVCenter; text: "Key size"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
                        Row {
                            id: keySizeControls; spacing: 8
                            PanelButton { width: root.portrait ? 62 : 54; label: "−"; enabled: keyboardBridge.keyScale > 0.851; onTriggered: keyboardBridge.setKeyScale(keyboardBridge.keyScale - 0.05) }
                            Rectangle { width: root.portrait ? 82 : 72; height: root.portrait ? 52 : 44; radius: 12; color: root.keyColor; Text { anchors.centerIn: parent; text: Math.round(keyboardBridge.keyScale * 100) + "%"; color: root.textColor; font.pixelSize: root.portrait ? 16 : 14 } }
                            PanelButton { width: root.portrait ? 62 : 54; label: "+"; enabled: keyboardBridge.keyScale < 1.199; onTriggered: keyboardBridge.setKeyScale(keyboardBridge.keyScale + 0.05) }
                        }
                    }

                    Row {
                        width: parent.width; spacing: 12
                        Text { width: parent.width - borderButton.width - parent.spacing; height: borderButton.height; verticalAlignment: Text.AlignVCenter; text: "Key borders"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
                        PanelButton { id: borderButton; width: root.portrait ? 170 : 150; label: keyboardBridge.keyBorders ? "On" : "Off"; onTriggered: keyboardBridge.setKeyBorders(!keyboardBridge.keyBorders) }
                    }

                    Row {
                        width: parent.width; spacing: 12
                        Text { width: parent.width - popupButton.width - parent.spacing; height: popupButton.height; verticalAlignment: Text.AlignVCenter; text: "Key popups"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
                        PanelButton { id: popupButton; width: root.portrait ? 170 : 150; label: keyboardBridge.keyPopups ? "On" : "Off"; onTriggered: keyboardBridge.setKeyPopups(!keyboardBridge.keyPopups) }
                    }

                    Rectangle { width: parent.width; height: 1; color: root.borderColor }

                    Row {
                        width: parent.width; spacing: 12
                        Text { width: parent.width - suggestionsButton.width - parent.spacing; height: suggestionsButton.height; verticalAlignment: Text.AlignVCenter; text: "Suggestions"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
                        PanelButton { id: suggestionsButton; width: root.portrait ? 170 : 150; label: keyboardBridge.suggestionsEnabled ? "On" : "Off"; onTriggered: keyboardBridge.setSuggestionsEnabled(!keyboardBridge.suggestionsEnabled) }
                    }

                    Row {
                        width: parent.width; spacing: 12
                        Text { width: parent.width - autocorrectButton.width - parent.spacing; height: autocorrectButton.height; verticalAlignment: Text.AlignVCenter; text: "Autocorrect"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
                        PanelButton { id: autocorrectButton; width: root.portrait ? 170 : 150; label: keyboardBridge.autocorrectEnabled ? "On" : "Off"; onTriggered: keyboardBridge.setAutocorrectEnabled(!keyboardBridge.autocorrectEnabled) }
                    }

                    Row {
                        width: parent.width; spacing: 12
                        Text { width: parent.width - learningButton.width - parent.spacing; height: learningButton.height; verticalAlignment: Text.AlignVCenter; text: "Local learning"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
                        PanelButton { id: learningButton; width: root.portrait ? 170 : 150; label: keyboardBridge.learningEnabled ? "On" : "Off"; onTriggered: keyboardBridge.setLearningEnabled(!keyboardBridge.learningEnabled) }
                    }

                    Row {
                        width: parent.width; spacing: 12
                        Text { width: parent.width - autoCapButton.width - parent.spacing; height: autoCapButton.height; verticalAlignment: Text.AlignVCenter; text: "Auto-capitalization"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
                        PanelButton { id: autoCapButton; width: root.portrait ? 170 : 150; label: keyboardBridge.autoCapitalizationEnabled ? "On" : "Off"; onTriggered: keyboardBridge.setAutoCapitalizationEnabled(!keyboardBridge.autoCapitalizationEnabled) }
                    }

                    Row {
                        width: parent.width; spacing: 12
                        Text { width: parent.width - doubleSpaceButton.width - parent.spacing; height: doubleSpaceButton.height; verticalAlignment: Text.AlignVCenter; text: "Double-space period"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
                        PanelButton { id: doubleSpaceButton; width: root.portrait ? 170 : 150; label: keyboardBridge.doubleSpacePeriodEnabled ? "On" : "Off"; onTriggered: keyboardBridge.setDoubleSpacePeriodEnabled(!keyboardBridge.doubleSpacePeriodEnabled) }
                    }

                    Row {
                        width: parent.width; spacing: 12
                        Text { width: parent.width - glideButton.width - parent.spacing; height: glideButton.height; verticalAlignment: Text.AlignVCenter; text: "Glide typing engine"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
                        PanelButton { id: glideButton; width: root.portrait ? 170 : 150; label: keyboardBridge.glideEnabled ? "On" : "Off"; onTriggered: keyboardBridge.setGlideEnabled(!keyboardBridge.glideEnabled) }
                    }

                    Row {
                        width: parent.width; spacing: 12
                        Text { width: parent.width - clipboardHistoryButton.width - parent.spacing; height: clipboardHistoryButton.height; verticalAlignment: Text.AlignVCenter; text: "Clipboard history"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
                        PanelButton { id: clipboardHistoryButton; width: root.portrait ? 170 : 150; label: keyboardBridge.clipboardHistoryEnabled ? "On" : "Off"; onTriggered: keyboardBridge.setClipboardHistoryEnabled(!keyboardBridge.clipboardHistoryEnabled) }
                    }

                    PanelButton {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: root.portrait ? 230 : 210
                        label: "Clear learned words"
                        onTriggered: keyboardBridge.clearLearnedWords()
                    }
                }
            }
        }
    }
}
