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
    readonly property bool compact: keyboardBridge.layoutMode !== "full"
    // Compact mode: a narrower keyboard docked to one side, for one-handed use
    // of a large tablet (the input panel itself stays docked by KWin).
    readonly property real contentWidth: Math.min(
        width - metrics.outerMargin * 2,
        width * metrics.contentWidthRatio,
        metrics.maxContentWidth
    ) * (compact ? 0.62 : 1.0)
    readonly property real baseKeyWidth: (contentWidth - keyGap * 9) / 10
    // Palette: Gboard-like light and dark themes; "system" follows Plasma.
    readonly property string themeName: keyboardBridge.effectiveTheme
    readonly property bool lightTheme: themeName === "light"
    readonly property bool amoledTheme: themeName === "amoled"
    readonly property color backgroundColor: lightTheme ? "#e8eaed" : amoledTheme ? "#000000" : "#202124"
    readonly property color panelColor: lightTheme ? "#f1f3f4" : amoledTheme ? "#080808" : "#292a2d"
    readonly property color keyColor: lightTheme ? "#ffffff" : amoledTheme ? "#171717" : "#3c4043"
    readonly property color specialKeyColor: lightTheme ? "#cdd0d4" : amoledTheme ? "#242424" : "#5f6368"
    readonly property color borderColor: lightTheme ? "#c4c7c5" : amoledTheme ? "#333333" : "#56595e"
    readonly property color textColor: lightTheme ? "#202124" : "#f1f3f4"
    readonly property color secondaryTextColor: lightTheme ? "#5f6368" : "#9aa0a6"
    readonly property color accentColor: lightTheme ? "#1a73e8" : "#8ab4f8"
    readonly property color pressedColor: lightTheme ? "#d2d5d9" : "#60656a"
    readonly property color accentPressedColor: lightTheme ? "#8ab4f8" : "#9fc3ff"
    readonly property color selectedColor: lightTheme ? "#d2e3fc" : "#465c78"
    property bool toolbarExpanded: false
    property string emojiCategory: "All"
    property var glideKeys: []
    property var glideStartItem: null
    property real glideStartX: 0
    property real glideStartY: 0
    property bool glideActive: false
    property string glideLastValue: ""

    width: Screen.width
    // Gboard shows a number pad in number/phone fields.
    readonly property bool numpadShown: keyboardBridge.inputPurpose === "number" || keyboardBridge.inputPurpose === "phone"
    readonly property bool numberRowShown: keyboardBridge.numberRow && !keyboardBridge.symbolsActive && !numpadShown
    readonly property bool emojiRowShown: keyboardBridge.emojiRow && keyboardBridge.recentEmojis.length > 0 && !numpadShown
    height: toolbarHeight + metrics.panelHeight + (numberRowShown ? keyHeight + keyGap : 0)
            + (emojiRowShown ? keyHeight + keyGap : 0)
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
        if (glideStartItem && glideStartItem !== item) {
            // A second finger is down: this is two-thumb tapping, not a glide.
            if (!glideActive) {
                glideStartItem = null
                glideLastValue = ""
            }
            return
        }
        var p = item.mapToItem(root, x, y)
        glideStartItem = item
        glideStartX = p.x
        glideStartY = p.y
        glideActive = false
        glideLastValue = ""
    }

    // Finds the character key under a point in root coordinates.
    function keyAt(x, y) {
        var item = keyboardRows
        var p = root.mapToItem(keyboardRows, x, y)
        for (var depth = 0; depth < 8 && item; ++depth) {
            var child = item.childAt(p.x, p.y)
            if (!child) return null
            if (child.label !== undefined && child.special === false && child.label.length > 0) return child
            p = item.mapToItem(child, p.x, p.y)
            item = child
        }
        return null
    }

    property var glidePoints: []

    Canvas {
        id: glideTrail
        objectName: "glideTrail"
        anchors.fill: parent
        z: 200
        visible: root.glidePoints.length > 1
        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            var pts = root.glidePoints
            if (pts.length < 2) return
            ctx.strokeStyle = root.accentColor
            ctx.lineWidth = 6
            ctx.lineCap = "round"
            ctx.lineJoin = "round"
            ctx.globalAlpha = 0.75
            ctx.beginPath()
            ctx.moveTo(pts[0].x, pts[0].y)
            for (var i = 1; i < pts.length; ++i) ctx.lineTo(pts[i].x, pts[i].y)
            ctx.stroke()
        }
    }

    function updateGlideCandidate(item, x, y) {
        // Only the finger that started the candidate can turn it into a glide
        // (its key keeps the touch grab and reports every move).
        if (!glideStartItem || item !== glideStartItem || !keyboardBridge.glideEnabled) return
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
        // Gboard-style gesture trail.
        var points = glidePoints.length === 0 ? [Qt.point(glideStartX, glideStartY)] : glidePoints
        points.push(Qt.point(p.x, p.y))
        if (points.length > 96) points.shift()
        glidePoints = points
        glideTrail.requestPaint()
        var hit = glideHit(p.x, p.y)
        if (hit && hit.glideValue !== glideLastValue) {
            keyboardBridge.glideThrough(hit.glideValue)
            glideLastValue = hit.glideValue
        }
    }

    function finishGlideCandidate(item) {
        if (item !== glideStartItem) return
        if (glideActive) {
            item.consumeRelease = true
            keyboardBridge.endGlide()
        }
        glideStartItem = null
        glideActive = false
        glideLastValue = ""
        glidePoints = []
        glideTrail.requestPaint()
    }

    component Key: Rectangle {
        id: key

        property string label: ""
        property string iconSource: ""
        property string alternate: ""
        // Gboard-style long-press choices; first is preselected. More than
        // one opens a picker: slide to choose, release to insert.
        property var alternates: []
        property string hint: ""
        property bool choosing: false
        property int choiceIndex: 0
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
        signal alternateChosen(string text)

        function choiceCellWidth() { return Math.max(key.width * 0.86, 44) }
        // The picker starts at the key's left edge, clamped inside the keyboard.
        function choiceOriginX() {
            var total = choiceCellWidth() * alternates.length
            var left = key.mapToItem(root, 0, 0).x
            var shift = Math.min(0, root.width - (left + total) - 4)
            return Math.max(-left + 4, shift)
        }
        function updateChoice(x) {
            var index = Math.floor((x - choiceOriginX()) / choiceCellWidth())
            choiceIndex = Math.max(0, Math.min(alternates.length - 1, index))
        }

        width: preferredWidth
        height: root.keyHeight
        radius: root.metrics.keyRadius
        border.width: keyboardBridge.keyBorders ? 1 : 0
        border.color: root.borderColor

        color: mouse.pressed
            ? (accent ? root.accentPressedColor : root.pressedColor)
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

        Text {
            // Symbol hint in the corner (Gboard "long press for symbols").
            visible: key.hint.length > 0 && key.iconSource.length === 0
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 4
            text: key.hint
            color: root.textColor
            opacity: 0.55
            font.pixelSize: Math.max(10, root.metrics.fontSize * 0.45)
        }

        Rectangle {
            id: choicePicker
            visible: key.choosing
            z: 110
            x: key.choiceOriginX()
            anchors.bottom: parent.top
            anchors.bottomMargin: 7
            width: key.choiceCellWidth() * key.alternates.length
            height: root.metrics.popupHeight
            radius: 14
            color: root.specialKeyColor
            border.width: 1
            border.color: root.borderColor

            Row {
                anchors.fill: parent
                Repeater {
                    model: key.choosing ? key.alternates : []
                    delegate: Rectangle {
                        required property var modelData
                        required property int index
                        width: key.choiceCellWidth()
                        height: choicePicker.height
                        radius: 12
                        color: index === key.choiceIndex ? root.accentColor : "transparent"
                        Text {
                            anchors.centerIn: parent
                            text: modelData
                            color: index === key.choiceIndex ? root.backgroundColor : root.textColor
                            font.pixelSize: root.metrics.fontSize + 3
                            font.weight: Font.Medium
                        }
                    }
                }
            }
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
                && !key.choosing
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
                color: root.textColor
                font.pixelSize: root.metrics.fontSize + 5
                font.weight: Font.Medium
            }
        }

        // One touch point per key: overlapping two-thumb taps land on
        // different keys and are handled independently (MouseArea only ever
        // sees the first touch). Mouse input keeps working (mouseEnabled).
        MultiPointTouchArea {
            id: mouse
            anchors.fill: parent
            mouseEnabled: true
            maximumTouchPoints: 1
            property bool held: false
            property bool pressed: false
            property real lastX: 0
            property real lastY: 0

            function inside(x, y) { return x >= 0 && y >= 0 && x <= width && y <= height }

            Timer {
                id: holdTimer
                interval: Qt.styleHints.mousePressAndHoldInterval
                onTriggered: {
                    if (mouse.pressed && key.longPressEnabled && mouse.inside(mouse.lastX, mouse.lastY)) {
                        mouse.held = true
                        if (key.alternates.length > 1) {
                            key.choiceIndex = 0
                            key.choosing = true
                        } else {
                            key.longPressed()
                        }
                    }
                }
            }

            onPressed: (touchPoints) => {
                const point = touchPoints[0]
                pressed = true
                held = false
                lastX = point.x
                lastY = point.y
                key.consumeRelease = false
                holdTimer.restart()
                key.pressStarted(point.x, point.y)
            }
            onUpdated: (touchPoints) => {
                if (!pressed || touchPoints.length === 0) return
                lastX = touchPoints[0].x
                lastY = touchPoints[0].y
                if (key.choosing) {
                    key.updateChoice(lastX)
                    return
                }
                key.pointerMoved(lastX, lastY)
            }
            onReleased: (touchPoints) => {
                if (!pressed) return
                const point = touchPoints.length > 0 ? touchPoints[0] : null
                const x = point ? point.x : lastX
                const y = point ? point.y : lastY
                pressed = false
                holdTimer.stop()
                if (key.choosing) {
                    key.choosing = false
                    key.alternateChosen(key.alternates[key.choiceIndex])
                    return
                }
                key.pressEnded(x, y)
                if (inside(x, y) && !held && !key.consumeRelease) {
                    key.triggered()
                }
            }
            onCanceled: (touchPoints) => {
                pressed = false
                held = false
                key.choosing = false
                holdTimer.stop()
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

    Rectangle {
        id: topToolbar
        objectName: "topToolbar"
        z: 120
        width: root.contentWidth
        height: root.toolbarHeight
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        color: root.backgroundColor

        Item {
            anchors.fill: parent

            Text {
                // Voice status replaces the suggestions while it matters.
                id: voiceStatus
                visible: keyboardBridge.voiceBuilt && !root.toolbarExpanded
                    && (keyboardBridge.voiceState === "recording" || keyboardBridge.voiceState === "recognizing"
                        || keyboardBridge.voiceMessage.length > 0)
                anchors.centerIn: parent
                width: parent.width * 0.7
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                color: root.textColor
                font.pixelSize: root.portrait ? 17 : 15
                text: keyboardBridge.voiceState === "recording" ? "Listening… tap 🎤 to finish"
                    : keyboardBridge.voiceState === "recognizing" ? "Recognizing…"
                    : keyboardBridge.voiceMessage
            }

            Rectangle {
                // Gboard/LatinIME "touch again to save": after keeping an
                // unknown word, offer to add it to the personal dictionary.
                id: saveWordChip
                objectName: "saveWordChip"
                visible: keyboardBridge.saveWordCandidate.length > 0 && !root.toolbarExpanded && !voiceStatus.visible
                anchors.centerIn: parent
                width: saveWordText.implicitWidth + 32
                height: parent.height - 10
                radius: height / 2
                color: saveWordMouse.pressed ? root.pressedColor : root.selectedColor
                Text {
                    id: saveWordText
                    anchors.centerIn: parent
                    text: "\uFF0B Add \u201C" + keyboardBridge.saveWordCandidate + "\u201D to dictionary"
                    color: root.textColor
                    font.pixelSize: root.portrait ? 17 : 15
                    font.weight: Font.Medium
                }
                MouseArea {
                    id: saveWordMouse
                    anchors.fill: parent
                    onClicked: keyboardBridge.addWordToDictionary(keyboardBridge.saveWordCandidate)
                }
            }

            Row {
                id: suggestionRow
                visible: keyboardBridge.activePanel === "typing"
                    && keyboardBridge.suggestionsEnabled
                    && keyboardBridge.suggestions.length > 0
                    && !root.toolbarExpanded
                    && !voiceStatus.visible
                    && !saveWordChip.visible
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
                            // Gboard: the typed word is quoted when Space would correct it; the
                            // correction itself is bold.
                            text: (keyboardBridge.autocorrectSuggestion.length > 0 || keyboardBridge.typedWordUnknown)
                                && modelData === keyboardBridge.currentWord
                                ? "\u201C" + modelData + "\u201D" : modelData
                            elide: Text.ElideRight
                            color: root.textColor
                            font.pixelSize: root.portrait ? 17 : 15
                            font.weight: modelData === keyboardBridge.autocorrectSuggestion ? Font.Bold : Font.Medium
                        }

                        MouseArea {
                            id: suggestionMouse
                            anchors.fill: parent
                            property bool forgot: false
                            onPressed: forgot = false
                            // Gboard: long-press a suggestion to remove it.
                            onPressAndHold: {
                                if (modelData.length > 0 && modelData[0].toLowerCase() !== modelData[0].toUpperCase()) {
                                    forgot = true
                                    keyboardBridge.forgetSuggestion(modelData)
                                }
                            }
                            onClicked: {
                                if (forgot) return
                                // selectSuggestion() replaces the model and
                                // destroys this delegate; touch nothing after it.
                                root.toolbarExpanded = false
                                keyboardBridge.selectSuggestion(modelData)
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

                Rectangle {
                    // Offline dictation: tap to record, tap again to insert.
                    visible: keyboardBridge.voiceBuilt
                    width: root.portrait ? 52 : 46
                    height: root.portrait ? 42 : 38
                    radius: 12
                    color: keyboardBridge.voiceState === "recording" ? "#c0392b"
                         : voiceMouse.pressed ? root.specialKeyColor : "transparent"
                    opacity: keyboardBridge.voiceState === "unavailable" ? 0.55 : 1.0

                    Text {
                        anchors.centerIn: parent
                        text: keyboardBridge.voiceState === "recognizing" ? "…" : "🎤"
                        color: root.textColor
                        font.pixelSize: root.portrait ? 22 : 19
                    }

                    MouseArea {
                        id: voiceMouse
                        anchors.fill: parent
                        onClicked: keyboardBridge.toggleVoice()
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
                            : (selected ? root.selectedColor : "transparent")
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
                        ? root.selectedColor
                        : (languageMouse.pressed ? root.keyColor : "transparent")

                    Rectangle {
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        width: root.portrait ? 38 : 34
                        height: width
                        radius: width / 2
                        color: keyboardBridge.languageCode === modelData ? root.selectedColor : root.keyColor

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
        objectName: "keyboardRows"
        // Above the toolbar (z 120) so key previews and long-press pickers of
        // the top row are not drawn underneath it; panels (140+) stay on top.
        z: 130
        x: keyboardBridge.layoutMode === "left" ? root.metrics.outerMargin
           : keyboardBridge.layoutMode === "right" ? root.width - width - root.metrics.outerMargin
           : (root.width - width) / 2
        anchors {
            top: parent.top
            topMargin: root.toolbarHeight + root.metrics.topPadding
        }
        spacing: root.keyGap

        Row {
            // Gboard "Emoji fast-access row": recently used emoji.
            visible: root.emojiRowShown
            spacing: root.keyGap
            Repeater {
                model: keyboardBridge.recentEmojis
                delegate: Key {
                    required property string modelData
                    objectName: "emojiRowKey_" + modelData
                    preferredWidth: root.dynamicKeyWidth(10)
                    label: modelData
                    special: true
                    onTriggered: keyboardBridge.insertEmoji(modelData)
                }
            }
        }
        Row {
            // Gboard "Number row" preference.
            visible: root.numberRowShown
            spacing: root.keyGap
            Repeater {
                model: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "0"]
                delegate: Key {
                    required property string modelData
                    objectName: "numberKey_" + modelData
                    preferredWidth: root.dynamicKeyWidth(10)
                    label: modelData
                    special: true
                    onTriggered: keyboardBridge.tapText(modelData)
                }
            }
        }
        Column {
            // Number pad for number/phone fields (Gboard behaviour).
            visible: root.numpadShown
            spacing: root.keyGap
            anchors.horizontalCenter: parent.horizontalCenter
            Repeater {
                model: [["1", "2", "3"], ["4", "5", "6"], ["7", "8", "9"],
                        [keyboardBridge.inputPurpose === "phone" ? "+" : ",", "0", "⌫"]]
                delegate: Row {
                    required property var modelData
                    spacing: root.keyGap
                    Repeater {
                        model: modelData
                        delegate: Key {
                            required property string modelData
                            objectName: "numpadKey_" + modelData
                            preferredWidth: (root.contentWidth - root.keyGap * 2) / 3
                            label: modelData
                            special: modelData === "⌫"
                            onTriggered: modelData === "⌫" ? keyboardBridge.backspace() : keyboardBridge.tapText(modelData)
                        }
                    }
                }
            }
        }

        Row {
            visible: !root.numpadShown
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
                    alternates: keyboardBridge.symbolsActive ? [] : (keyboardBridge.uppercase, keyboardBridge.alternatesForKey(modelData))
                    hint: keyboardBridge.symbolsActive ? "" : (keyboardBridge.symbolHints, keyboardBridge.symbolHintForKey(modelData))
                    longPressEnabled: alternates.length > 0
                    onAlternateChosen: (text) => keyboardBridge.tapAlternateText(text)
                    glideEligible: !keyboardBridge.symbolsActive
                    glideValue: modelData
                    Component.onCompleted: root.registerGlideKey(row1LetterKey)
                    Component.onDestruction: root.unregisterGlideKey(row1LetterKey)
                    onPressStarted: (x, y) => root.beginGlideCandidate(row1LetterKey, x, y)
                    onPointerMoved: (x, y) => root.updateGlideCandidate(row1LetterKey, x, y)
                    onPressEnded: root.finishGlideCandidate(row1LetterKey)

                    onTriggered: {
                        if (keyboardBridge.symbolsActive) keyboardBridge.tapText(modelData)
                        else keyboardBridge.tapLetter(modelData)
                    }
                    onLongPressed: keyboardBridge.tapAlternateText(alternates[0])
                }
            }
        }

        Row {
            visible: !root.numpadShown
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
                    alternates: keyboardBridge.symbolsActive ? [] : (keyboardBridge.uppercase, keyboardBridge.alternatesForKey(modelData))
                    hint: keyboardBridge.symbolsActive ? "" : (keyboardBridge.symbolHints, keyboardBridge.symbolHintForKey(modelData))
                    longPressEnabled: alternates.length > 0
                    onAlternateChosen: (text) => keyboardBridge.tapAlternateText(text)
                    glideEligible: !keyboardBridge.symbolsActive
                    glideValue: modelData
                    Component.onCompleted: root.registerGlideKey(row2LetterKey)
                    Component.onDestruction: root.unregisterGlideKey(row2LetterKey)
                    onPressStarted: (x, y) => root.beginGlideCandidate(row2LetterKey, x, y)
                    onPointerMoved: (x, y) => root.updateGlideCandidate(row2LetterKey, x, y)
                    onPressEnded: root.finishGlideCandidate(row2LetterKey)

                    onTriggered: {
                        if (keyboardBridge.symbolsActive) keyboardBridge.tapText(modelData)
                        else keyboardBridge.tapLetter(modelData)
                    }
                    onLongPressed: keyboardBridge.tapAlternateText(alternates[0])
                }
            }
        }

        Row {
            visible: !root.numpadShown
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: root.keyGap

            Key {
                id: shiftKey
                objectName: "shiftKey"
                visible: !keyboardBridge.symbolsActive
                preferredWidth: root.baseKeyWidth * 1.25
                special: true
                popupEnabled: false
                iconSource: keyboardBridge.capsLock
                    ? "qrc:/v3keyboard/icons/shift-lock.svg"
                    : "qrc:/v3keyboard/icons/shift.svg"
                // Gboard: touch Shift and slide onto a letter for one capital.
                property bool sliding: false
                onPressStarted: (x, y) => { sliding = false }
                onPointerMoved: (x, y) => {
                    if (!sliding && (x < 0 || x > width || y < 0 || y > height)) {
                        sliding = true
                        consumeRelease = true
                        if (!keyboardBridge.uppercase) keyboardBridge.shift()
                    }
                }
                onPressEnded: (x, y) => {
                    if (!sliding) return
                    sliding = false
                    var p = mapToItem(root, x, y)
                    var target = root.keyAt(p.x, p.y)
                    if (target && target.glideValue && target.glideValue.length > 0) keyboardBridge.tapLetter(target.glideValue)
                    else if (keyboardBridge.uppercase && !keyboardBridge.capsLock) keyboardBridge.shift()
                }
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
                    alternates: keyboardBridge.symbolsActive ? [] : (keyboardBridge.uppercase, keyboardBridge.alternatesForKey(modelData))
                    hint: keyboardBridge.symbolsActive ? "" : (keyboardBridge.symbolHints, keyboardBridge.symbolHintForKey(modelData))
                    longPressEnabled: alternates.length > 0
                    onAlternateChosen: (text) => keyboardBridge.tapAlternateText(text)
                    glideEligible: !keyboardBridge.symbolsActive
                    glideValue: modelData
                    Component.onCompleted: root.registerGlideKey(row3LetterKey)
                    Component.onDestruction: root.unregisterGlideKey(row3LetterKey)
                    onPressStarted: (x, y) => root.beginGlideCandidate(row3LetterKey, x, y)
                    onPointerMoved: (x, y) => root.updateGlideCandidate(row3LetterKey, x, y)
                    onPressEnded: root.finishGlideCandidate(row3LetterKey)

                    onTriggered: {
                        if (keyboardBridge.symbolsActive) keyboardBridge.tapText(modelData)
                        else keyboardBridge.tapLetter(modelData)
                    }
                    onLongPressed: keyboardBridge.tapAlternateText(alternates[0])
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
                onPressStarted: (x, y) => {
                    dragStartX = x
                    dragStep = 0
                }
                onPointerMoved: (x, y) => {
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
                objectName: "symbolsKey"
                preferredWidth: root.baseKeyWidth * 1.35
                special: true
                popupEnabled: false
                label: keyboardBridge.symbolsActive ? "ABC" : "?123"
                // Gboard fast symbols: touch ?123, slide onto a symbol, release.
                property bool sliding: false
                onPressStarted: (x, y) => { sliding = false }
                onPointerMoved: (x, y) => {
                    if (!sliding && !keyboardBridge.symbolsActive && (y < -8 || x > width + 8)) {
                        sliding = true
                        consumeRelease = true
                        keyboardBridge.toggleSymbols()
                    }
                }
                onPressEnded: (x, y) => {
                    if (!sliding) return
                    sliding = false
                    var p = mapToItem(root, x, y)
                    var target = root.keyAt(p.x, p.y)
                    if (target) keyboardBridge.tapText(target.label)
                    if (keyboardBridge.symbolsActive) keyboardBridge.toggleSymbols()
                }
                onTriggered: keyboardBridge.toggleSymbols()
            }

            Key {
                id: commaKey
                readonly property string glyph: keyboardBridge.inputPurpose === "email" ? "@"
                    : keyboardBridge.inputPurpose === "url" ? "/" : ","
                preferredWidth: root.baseKeyWidth
                label: glyph
                // Gboard: long-press the comma for emoji.
                longPressEnabled: true
                onLongPressed: keyboardBridge.activateToolbarAction("emoji")
                onTriggered: keyboardBridge.tapText(glyph)
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
                onPressStarted: (x, y) => {
                    dragStartX = x
                    cursorStep = 0
                }
                onPointerMoved: (x, y) => {
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
                // Gboard: long-press the period for punctuation.
                alternates: keyboardBridge.symbolsActive ? [] : (keyboardBridge.inputPurpose, keyboardBridge.alternatesForKey("."))
                longPressEnabled: alternates.length > 0
                onAlternateChosen: (text) => keyboardBridge.tapAlternateText(text)
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
                        color: keyboardBridge.clipboardText.length > 0 ? root.textColor : root.secondaryTextColor
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
                            color: root.secondaryTextColor
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
                    width: parent.width
                    height: parent.height - (root.portrait ? 100 : 88)
                    clip: true
                    cellWidth: root.portrait ? 58 : 50
                    cellHeight: cellWidth
                    model: (keyboardBridge.emojiCategories, keyboardBridge.emojiSearch(emojiSearchInput.text, root.emojiCategory))

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
                            onClicked: keyboardBridge.insertEmoji(modelData)
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
                        PanelButton { id: themeButton; width: root.portrait ? 170 : 150; label: keyboardBridge.theme === "system" ? "System" : keyboardBridge.theme === "light" ? "Light" : keyboardBridge.theme === "amoled" ? "AMOLED" : "Dark"; onTriggered: keyboardBridge.cycleTheme() }
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
                        Text { width: parent.width - hintsButton.width - parent.spacing; height: hintsButton.height; verticalAlignment: Text.AlignVCenter; text: "Long-press symbols (key hints)"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
                        PanelButton { id: hintsButton; width: root.portrait ? 170 : 150; label: keyboardBridge.symbolHints ? "On" : "Off"; onTriggered: keyboardBridge.setSymbolHints(!keyboardBridge.symbolHints) }
                    }

                    Row {
                        width: parent.width; spacing: 12
                        Text { width: parent.width - numberRowButton.width - parent.spacing; height: numberRowButton.height; verticalAlignment: Text.AlignVCenter; text: "Number row"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
                        PanelButton { id: numberRowButton; width: root.portrait ? 170 : 150; label: keyboardBridge.numberRow ? "On" : "Off"; onTriggered: keyboardBridge.setNumberRow(!keyboardBridge.numberRow) }
                    }

                    Row {
                        width: parent.width; spacing: 12
                        Text { width: parent.width - emojiSuggestButton.width - parent.spacing; height: emojiSuggestButton.height; verticalAlignment: Text.AlignVCenter; text: "Emoji suggestions"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
                        PanelButton { id: emojiSuggestButton; width: root.portrait ? 170 : 150; label: keyboardBridge.emojiSuggestionsEnabled ? "On" : "Off"; onTriggered: keyboardBridge.setEmojiSuggestionsEnabled(!keyboardBridge.emojiSuggestionsEnabled) }
                    }

                    Row {
                        width: parent.width; spacing: 12
                        Text { width: parent.width - offensiveButton.width - parent.spacing; height: offensiveButton.height; verticalAlignment: Text.AlignVCenter; text: "Block offensive words"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
                        PanelButton { id: offensiveButton; width: root.portrait ? 170 : 150; label: keyboardBridge.blockOffensive ? "On" : "Off"; onTriggered: keyboardBridge.setBlockOffensive(!keyboardBridge.blockOffensive) }
                    }

                    Row {
                        width: parent.width; spacing: 12
                        Text { width: parent.width - autoSpaceButton.width - parent.spacing; height: autoSpaceButton.height; verticalAlignment: Text.AlignVCenter; text: "Auto-space after punctuation"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
                        PanelButton { id: autoSpaceButton; width: root.portrait ? 170 : 150; label: keyboardBridge.autoSpaceAfterPunctuation ? "On" : "Off"; onTriggered: keyboardBridge.setAutoSpaceAfterPunctuation(!keyboardBridge.autoSpaceAfterPunctuation) }
                    }

                    Row {
                        width: parent.width; spacing: 12
                        Text { width: parent.width - emojiRowButton.width - parent.spacing; height: emojiRowButton.height; verticalAlignment: Text.AlignVCenter; text: "Emoji fast-access row"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
                        PanelButton { id: emojiRowButton; width: root.portrait ? 170 : 150; label: keyboardBridge.emojiRow ? "On" : "Off"; onTriggered: keyboardBridge.setEmojiRow(!keyboardBridge.emojiRow) }
                    }

                    Row {
                        width: parent.width; spacing: 12
                        Text { width: parent.width - layoutButton.width - parent.spacing; height: layoutButton.height; verticalAlignment: Text.AlignVCenter; text: "Keyboard layout"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
                        PanelButton { id: layoutButton; width: root.portrait ? 170 : 150; label: keyboardBridge.layoutMode === "left" ? "Compact left" : keyboardBridge.layoutMode === "right" ? "Compact right" : "Full width"; onTriggered: keyboardBridge.cycleLayoutMode() }
                    }

                    Row {
                        width: parent.width; spacing: 12
                        Text { width: parent.width - compositionButton.width - parent.spacing; height: compositionButton.height; verticalAlignment: Text.AlignVCenter; text: "Underline word while typing"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
                        PanelButton { id: compositionButton; width: root.portrait ? 170 : 150; label: keyboardBridge.compositionEnabled ? "On" : "Off"; onTriggered: keyboardBridge.setCompositionEnabled(!keyboardBridge.compositionEnabled) }
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
                
                    Text {
                        width: parent.width
                        topPadding: 8
                        text: "Personal dictionary (" + keyboardBridge.userWords.length + ") \u2014 tap a word to remove it"
                        color: root.textColor
                        font.pixelSize: root.portrait ? 17 : 15
                    }

                    Flow {
                        width: parent.width
                        spacing: 8
                        Repeater {
                            model: keyboardBridge.userWords
                            delegate: Rectangle {
                                required property string modelData
                                objectName: "userWord_" + modelData
                                width: userWordText.implicitWidth + 24
                                height: root.portrait ? 40 : 34
                                radius: height / 2
                                color: userWordMouse.pressed ? root.pressedColor : root.keyColor
                                border.width: 1
                                border.color: root.borderColor
                                Text {
                                    id: userWordText
                                    anchors.centerIn: parent
                                    text: modelData + "  \u00D7"
                                    color: root.textColor
                                    font.pixelSize: root.portrait ? 16 : 14
                                }
                                MouseArea {
                                    id: userWordMouse
                                    anchors.fill: parent
                                    onClicked: keyboardBridge.removeWordFromDictionary(modelData)
                                }
                            }
                        }
                    }

                    Text {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        color: root.secondaryTextColor
                        font.pixelSize: root.portrait ? 14 : 12
                        text: "Add words: tap a typed word in the suggestion strip, then \uFF0B. "
                            + "Many at once: ~/.config/v3-keyboard/dictionary.txt (one per line, all languages)."
                    }

                    Text {
                        // About: version, license and data sources (1.0).
                        objectName: "aboutText"
                        width: parent.width
                        topPadding: 12
                        wrapMode: Text.WordWrap
                        color: root.secondaryTextColor
                        font.pixelSize: root.portrait ? 14 : 12
                        text: "V3 Keyboard " + keyboardBridge.version + " · GPL-3.0-or-later · works offline\n"
                            + "Data: FrequencyWords (CC BY-SA 4.0), Unicode CLDR (Unicode License V3), "
                            + "LDNOOBW (CC BY 4.0), Hunspell dictionaries (system packages), "
                            + "LibreOffice uk_UA (MPL-1.1); optional whisper.cpp (MIT).\n"
                            + "Full texts: ~/.local/share/doc/v3-keyboard"
                    }
                }
            }
        }
    }
}
