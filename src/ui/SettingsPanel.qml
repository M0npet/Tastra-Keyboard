// SPDX-License-Identifier: GPL-3.0-or-later
// Settings panel, loaded only while open.

import QtQuick
import QtQuick.Window

Flickable {
    id: settingsFlick
    objectName: "settingsFlick"
    anchors.fill: parent
    clip: true
    contentWidth: width
    contentHeight: settingsColumn.implicitHeight + root.keyGap * 2

    // The list is long: show that it scrolls.
    Rectangle {
        objectName: "settingsScrollIndicator"
        parent: settingsFlick
        visible: settingsFlick.contentHeight > settingsFlick.height
        anchors.right: parent.right
        anchors.rightMargin: 4
        width: 5
        radius: 2.5
        color: root.secondaryTextColor
        opacity: 0.7
        y: settingsFlick.visibleArea.yPosition * settingsFlick.height
        height: Math.max(24, settingsFlick.visibleArea.heightRatio * settingsFlick.height)
    }

    Column {
        id: settingsColumn
        width: Math.min(parent.width, root.portrait ? 640 : 780)
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: root.portrait ? 12 : 8

        Text {
            objectName: "settingsSection_Languages"
            width: parent.width
            topPadding: 14
            text: "Languages"
            color: root.accentColor
            font.pixelSize: root.portrait ? 15 : 13
            font.weight: Font.DemiBold
            font.capitalization: Font.AllUppercase
        }

        Repeater {
            // Gboard: choose which languages the globe cycles through.
            model: keyboardBridge.allLanguageCodes
            delegate: Row {
                required property string modelData
                required property int index
                width: parent ? parent.width : 0
                spacing: 12
                Text { width: parent.width - languageToggle.width - parent.spacing; height: languageToggle.height; verticalAlignment: Text.AlignVCenter; text: keyboardBridge.allLanguageLabels[index]; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
                PanelButton {
                    id: languageToggle
                    objectName: "languageToggle_" + modelData
                    width: root.portrait ? 170 : 150
                    label: (keyboardBridge.languageCodes, keyboardBridge.isLanguageEnabled(modelData)) ? "On" : "Off"
                    onTriggered: keyboardBridge.setLanguageEnabled(modelData, !keyboardBridge.isLanguageEnabled(modelData))
                }
            }
        }

        Text {
            objectName: "settingsSection_Preferences"
            width: parent.width
            topPadding: 14
            text: "Preferences"
            color: root.accentColor
            font.pixelSize: root.portrait ? 15 : 13
            font.weight: Font.DemiBold
            font.capitalization: Font.AllUppercase
        }

        Row {
            width: parent.width; spacing: 12
            Text { width: parent.width - numberRowButton.width - parent.spacing; height: numberRowButton.height; verticalAlignment: Text.AlignVCenter; text: "Number row"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
            PanelButton { id: numberRowButton; width: root.portrait ? 170 : 150; label: keyboardBridge.numberRow ? "On" : "Off"; onTriggered: keyboardBridge.setNumberRow(!keyboardBridge.numberRow) }
        }

        Row {
            width: parent.width; spacing: 12
            Text { width: parent.width - hintsButton.width - parent.spacing; height: hintsButton.height; verticalAlignment: Text.AlignVCenter; text: "Long-press symbols (key hints)"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
            PanelButton { id: hintsButton; width: root.portrait ? 170 : 150; label: keyboardBridge.symbolHints ? "On" : "Off"; onTriggered: keyboardBridge.setSymbolHints(!keyboardBridge.symbolHints) }
        }

        Row {
            width: parent.width; spacing: 12
            Text { width: parent.width - emojiRowButton.width - parent.spacing; height: emojiRowButton.height; verticalAlignment: Text.AlignVCenter; text: "Emoji fast-access row"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
            PanelButton { id: emojiRowButton; width: root.portrait ? 170 : 150; label: keyboardBridge.emojiRow ? "On" : "Off"; onTriggered: keyboardBridge.setEmojiRow(!keyboardBridge.emojiRow) }
        }

        Row {
            visible: keyboardBridge.canPlayKeySound
            width: parent.width; spacing: 12
            Text { width: parent.width - keySoundButton.width - parent.spacing; height: keySoundButton.height; verticalAlignment: Text.AlignVCenter; text: "Sound on keypress"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
            PanelButton { id: keySoundButton; width: root.portrait ? 170 : 150; label: keyboardBridge.keySound ? "On" : "Off"; onTriggered: keyboardBridge.setKeySound(!keyboardBridge.keySound) }
        }

        Row {
            width: parent.width; spacing: 12
            Text { width: parent.width - popupButton.width - parent.spacing; height: popupButton.height; verticalAlignment: Text.AlignVCenter; text: "Key popups"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
            PanelButton { id: popupButton; width: root.portrait ? 170 : 150; label: keyboardBridge.keyPopups ? "On" : "Off"; onTriggered: keyboardBridge.setKeyPopups(!keyboardBridge.keyPopups) }
        }
        Row {
            width: parent.width; spacing: 12
            Text { width: parent.width - longPressButton.width - parent.spacing; height: longPressButton.height; verticalAlignment: Text.AlignVCenter; text: "Long-press delay"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
            PanelButton { id: longPressButton; width: root.portrait ? 170 : 150; label: keyboardBridge.longPressDelay + " ms"; onTriggered: keyboardBridge.cycleLongPressDelay() }
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
            Text { width: parent.width - layoutButton.width - parent.spacing; height: layoutButton.height; verticalAlignment: Text.AlignVCenter; text: "Keyboard layout"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
            PanelButton { id: layoutButton; width: root.portrait ? 170 : 150; label: keyboardBridge.layoutMode === "left" ? "Compact left" : keyboardBridge.layoutMode === "right" ? "Compact right" : keyboardBridge.layoutMode === "split" ? "Split" : "Full width"; onTriggered: keyboardBridge.cycleLayoutMode() }
        }

        Text {
            objectName: "settingsSection_Theme"
            width: parent.width
            topPadding: 14
            text: "Theme"
            color: root.accentColor
            font.pixelSize: root.portrait ? 15 : 13
            font.weight: Font.DemiBold
            font.capitalization: Font.AllUppercase
        }

        Row {
            width: parent.width; spacing: 12
            Text { width: parent.width - themeButton.width - parent.spacing; height: themeButton.height; verticalAlignment: Text.AlignVCenter; text: "Theme"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
            PanelButton { id: themeButton; width: root.portrait ? 170 : 150; label: keyboardBridge.theme === "system" ? "System" : keyboardBridge.theme === "light" ? "Light" : keyboardBridge.theme === "amoled" ? "AMOLED" : "Dark"; onTriggered: keyboardBridge.cycleTheme() }
        }

        Row {
            width: parent.width; spacing: 12
            Text { width: parent.width - borderButton.width - parent.spacing; height: borderButton.height; verticalAlignment: Text.AlignVCenter; text: "Key borders"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
            PanelButton { id: borderButton; width: root.portrait ? 170 : 150; label: keyboardBridge.keyBorders ? "On" : "Off"; onTriggered: keyboardBridge.setKeyBorders(!keyboardBridge.keyBorders) }
        }

        Text {
            objectName: "settingsSection_Text correction"
            width: parent.width
            topPadding: 14
            text: "Text correction"
            color: root.accentColor
            font.pixelSize: root.portrait ? 15 : 13
            font.weight: Font.DemiBold
            font.capitalization: Font.AllUppercase
        }

        Row {
            width: parent.width; spacing: 12
            Text { width: parent.width - suggestionsButton.width - parent.spacing; height: suggestionsButton.height; verticalAlignment: Text.AlignVCenter; text: "Suggestions"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
            PanelButton { id: suggestionsButton; width: root.portrait ? 170 : 150; label: keyboardBridge.suggestionsEnabled ? "On" : "Off"; onTriggered: keyboardBridge.setSuggestionsEnabled(!keyboardBridge.suggestionsEnabled) }
        }

        Row {
            width: parent.width; spacing: 12
            Text { width: parent.width - nextWordButton.width - parent.spacing; height: nextWordButton.height; verticalAlignment: Text.AlignVCenter; text: "Next-word suggestions"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
            PanelButton { id: nextWordButton; objectName: "nextWordSuggestionsButton"; width: root.portrait ? 170 : 150; label: keyboardBridge.nextWordSuggestions ? "On" : "Off"; onTriggered: keyboardBridge.setNextWordSuggestions(!keyboardBridge.nextWordSuggestions) }
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
            Text { width: parent.width - autocorrectButton.width - parent.spacing; height: autocorrectButton.height; verticalAlignment: Text.AlignVCenter; text: "Autocorrect"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
            PanelButton { id: autocorrectButton; width: root.portrait ? 170 : 150; label: keyboardBridge.autocorrectEnabled ? "On" : "Off"; onTriggered: keyboardBridge.setAutocorrectEnabled(!keyboardBridge.autocorrectEnabled) }
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
            Text { width: parent.width - autoSpaceButton.width - parent.spacing; height: autoSpaceButton.height; verticalAlignment: Text.AlignVCenter; text: "Auto-space after punctuation"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
            PanelButton { id: autoSpaceButton; width: root.portrait ? 170 : 150; label: keyboardBridge.autoSpaceAfterPunctuation ? "On" : "Off"; onTriggered: keyboardBridge.setAutoSpaceAfterPunctuation(!keyboardBridge.autoSpaceAfterPunctuation) }
        }

        Text {
            objectName: "settingsSection_Glide typing"
            width: parent.width
            topPadding: 14
            text: "Glide typing"
            color: root.accentColor
            font.pixelSize: root.portrait ? 15 : 13
            font.weight: Font.DemiBold
            font.capitalization: Font.AllUppercase
        }

        Row {
            width: parent.width; spacing: 12
            Text { width: parent.width - glideButton.width - parent.spacing; height: glideButton.height; verticalAlignment: Text.AlignVCenter; text: "Glide typing engine"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
            PanelButton { id: glideButton; width: root.portrait ? 170 : 150; label: keyboardBridge.glideEnabled ? "On" : "Off"; onTriggered: keyboardBridge.setGlideEnabled(!keyboardBridge.glideEnabled) }
        }

        Row {
            visible: true
            width: parent.width; spacing: 12
            Text { width: parent.width - glideTrailButton.width - parent.spacing; height: glideTrailButton.height; verticalAlignment: Text.AlignVCenter; text: "Gesture trail"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
            PanelButton { id: glideTrailButton; width: root.portrait ? 170 : 150; label: keyboardBridge.glideTrail ? "On" : "Off"; onTriggered: keyboardBridge.setGlideTrail(!keyboardBridge.glideTrail) }
        }

        Text {
            objectName: "settingsSection_Clipboard"
            width: parent.width
            topPadding: 14
            text: "Clipboard"
            color: root.accentColor
            font.pixelSize: root.portrait ? 15 : 13
            font.weight: Font.DemiBold
            font.capitalization: Font.AllUppercase
        }

        Row {
            width: parent.width; spacing: 12
            Text { width: parent.width - clipboardHistoryButton.width - parent.spacing; height: clipboardHistoryButton.height; verticalAlignment: Text.AlignVCenter; text: "Clipboard history"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
            PanelButton { id: clipboardHistoryButton; width: root.portrait ? 170 : 150; label: keyboardBridge.clipboardHistoryEnabled ? "On" : "Off"; onTriggered: keyboardBridge.setClipboardHistoryEnabled(!keyboardBridge.clipboardHistoryEnabled) }
        }

        Text {
            objectName: "settingsSection_Dictionary"
            width: parent.width
            topPadding: 14
            text: "Dictionary"
            color: root.accentColor
            font.pixelSize: root.portrait ? 15 : 13
            font.weight: Font.DemiBold
            font.capitalization: Font.AllUppercase
        }

        Row {
            width: parent.width; spacing: 12
            Text { width: parent.width - learningButton.width - parent.spacing; height: learningButton.height; verticalAlignment: Text.AlignVCenter; text: "Local learning"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
            PanelButton { id: learningButton; width: root.portrait ? 170 : 150; label: keyboardBridge.learningEnabled ? "On" : "Off"; onTriggered: keyboardBridge.setLearningEnabled(!keyboardBridge.learningEnabled) }
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

        PanelButton {
            objectName: "addWordButton"
            anchors.horizontalCenter: parent.horizontalCenter
            width: root.portrait ? 230 : 210
            label: "\uFF0B Add word"
            onTriggered: keyboardBridge.startWordEntry()
        }

        Text {
            width: parent.width
            topPadding: 8
            visible: keyboardBridge.shortcutList.length > 0
            text: "Shortcuts (" + keyboardBridge.shortcutList.length + ") \u2014 tap one to remove it"
            color: root.textColor
            font.pixelSize: root.portrait ? 17 : 15
        }

        Flow {
            width: parent.width
            spacing: 8
            Repeater {
                model: keyboardBridge.shortcutList
                delegate: Rectangle {
                    required property string modelData
                    objectName: "shortcut_" + modelData.split(" = ")[0]
                    width: shortcutText.implicitWidth + 24
                    height: root.portrait ? 40 : 34
                    radius: height / 2
                    color: shortcutMouse.pressed ? root.pressedColor : root.keyColor
                    border.width: 1
                    border.color: root.borderColor
                    Text {
                        id: shortcutText
                        anchors.centerIn: parent
                        text: modelData.replace(" = ", " \u2192 ") + "  \u00D7"
                        color: root.textColor
                        font.pixelSize: root.portrait ? 16 : 14
                    }
                    MouseArea {
                        id: shortcutMouse
                        anchors.fill: parent
                        onClicked: keyboardBridge.removeShortcut(modelData.split(" = ")[0])
                    }
                }
            }
        }

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            color: root.secondaryTextColor
            font.pixelSize: root.portrait ? 14 : 12
            text: "Add words with \uFF0B (a shortcut is optional: typing it offers the word), or tap a "
                + "typed word in the suggestion strip. Many at once: ~/.config/tastra/dictionary.txt "
                + "(one per line) and ~/.config/tastra/shortcuts.txt (shortcut = text)."
        }

        Text {
            objectName: "settingsSection_Advanced"
            width: parent.width
            topPadding: 14
            text: "Advanced"
            color: root.accentColor
            font.pixelSize: root.portrait ? 15 : 13
            font.weight: Font.DemiBold
            font.capitalization: Font.AllUppercase
        }

        Row {
            width: parent.width; spacing: 12
            Text { width: parent.width - compositionButton.width - parent.spacing; height: compositionButton.height; verticalAlignment: Text.AlignVCenter; text: "Underline word while typing"; color: root.textColor; font.pixelSize: root.portrait ? 17 : 15 }
            PanelButton { id: compositionButton; width: root.portrait ? 170 : 150; label: keyboardBridge.compositionEnabled ? "On" : "Off"; onTriggered: keyboardBridge.setCompositionEnabled(!keyboardBridge.compositionEnabled) }
        }

        Text {
            // About: version, license and data sources (1.0).
            objectName: "aboutText"
            width: parent.width
            topPadding: 12
            wrapMode: Text.WordWrap
            color: root.secondaryTextColor
            font.pixelSize: root.portrait ? 14 : 12
            text: "Tastra " + keyboardBridge.version + " · GPL-3.0-or-later · works offline\n"
                + "Data: FrequencyWords (CC BY-SA 4.0), Unicode CLDR (Unicode License V3), "
                + "LDNOOBW (CC BY 4.0), Hunspell dictionaries (system packages), "
                + "LibreOffice uk_UA (MPL-1.1); optional whisper.cpp (MIT).\n"
                + "Full texts: ~/.local/share/doc/tastra"
        }
    }
}
