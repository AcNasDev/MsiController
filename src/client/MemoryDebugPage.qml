import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    implicitWidth: 860
    implicitHeight: 560

    property var proxy
    property color surfaceColor: "#181e29"
    property color elevatedColor: "#202735"
    property color borderColor: "#2b3443"
    property color textColor: "#f4f7fb"
    property color mutedTextColor: "#9aa7b8"
    property color accentColor: "#3fa7ff"
    property color dangerColor: "#ff6b6b"
    property color warnColor: "#f2b84b"
    property bool active: true
    property int rangeOffset: 0
    property int memorySize: 0
    property int bytesPerRow: 16
    property var bytes: []
    property var changedAddresses: ({})
    property int changeHighlightMs: 2200
    property int selectedAddress: -1
    property int selectedOriginalValue: 0
    property int selectedDraftValue: 0
    property string statusText: ""
    property string pendingWriteMode: ""
    property bool autoRefresh: false
    readonly property int rowCount: Math.ceil(bytes.length / bytesPerRow)

    function parseNumber(text, fallbackValue) {
        var raw = String(text || "").trim()
        if (raw.length === 0)
            return fallbackValue

        var normalized = raw.replace(/^0x/i, "")
        var base = raw.match(/^0x/i) || raw.match(/[a-f]/i) ? 16 : 10
        var parsed = parseInt(normalized, base)
        return isNaN(parsed) ? fallbackValue : parsed
    }

    function hex(value, width) {
        var text = Math.max(0, Number(value) || 0).toString(16).toUpperCase()
        while (text.length < width)
            text = "0" + text
        return "0x" + text
    }

    function byteText(value) {
        return hex(value, 2).substring(2)
    }

    function binaryText(value) {
        var text = clampByte(value).toString(2)
        while (text.length < 8)
            text = "0" + text
        return text
    }

    function clampByte(value) {
        return Math.max(0, Math.min(255, Math.round(Number(value) || 0)))
    }

    function byteIndex(address) {
        return address - rangeOffset
    }

    function byteAt(address) {
        var index = byteIndex(address)
        if (index < 0 || index >= bytes.length)
            return 0
        return clampByte(bytes[index])
    }

    function isAddressChanged(address) {
        return changedAddresses[address] !== undefined
    }

    function markChangedAddress(address) {
        var next = Object.assign({}, changedAddresses)
        next[address] = Date.now()
        changedAddresses = next
        changePruneTimer.restart()
    }

    function updateChangedAddresses(previousOffset, previousBytes, nextOffset, nextBytes) {
        if (!previousBytes || previousBytes.length === 0)
            return 0

        var previousByAddress = ({})
        for (var i = 0; i < previousBytes.length; ++i)
            previousByAddress[previousOffset + i] = clampByte(previousBytes[i])

        var now = Date.now()
        var nextChanges = Object.assign({}, changedAddresses)
        var hasChanges = false
        var changedCount = 0
        for (var j = 0; j < nextBytes.length; ++j) {
            var address = nextOffset + j
            if (previousByAddress[address] !== undefined &&
                    previousByAddress[address] !== clampByte(nextBytes[j])) {
                nextChanges[address] = now
                hasChanges = true
                changedCount++
            }
        }

        if (hasChanges) {
            changedAddresses = nextChanges
            changePruneTimer.restart()
        }
        return changedCount
    }

    function pruneChangedAddresses() {
        var now = Date.now()
        var next = ({})
        var hasVisibleChanges = false
        for (var address in changedAddresses) {
            if (now - changedAddresses[address] < changeHighlightMs) {
                next[address] = changedAddresses[address]
                hasVisibleChanges = true
            }
        }

        changedAddresses = next
        if (!hasVisibleChanges)
            changePruneTimer.stop()
    }

    function selectAddress(address) {
        if (address < rangeOffset || address >= rangeOffset + bytes.length)
            return

        selectedAddress = address
        selectedOriginalValue = byteAt(address)
        selectedDraftValue = selectedOriginalValue
        byteValueField.text = byteText(selectedDraftValue)
    }

    function setDraftValue(value) {
        selectedDraftValue = clampByte(value)
        byteValueField.text = byteText(selectedDraftValue)
    }

    function bitChecked(bit) {
        return (selectedDraftValue & (1 << bit)) !== 0
    }

    function setDraftBit(bit, checked) {
        var mask = 1 << bit
        setDraftValue(checked ? (selectedDraftValue | mask) : (selectedDraftValue & ~mask))
    }

    function updateLocalByte(address, value) {
        var index = byteIndex(address)
        if (index < 0 || index >= bytes.length)
            return

        var nextBytes = bytes.slice()
        nextBytes[index] = clampByte(value)
        bytes = nextBytes
        selectedOriginalValue = clampByte(value)
        selectedDraftValue = selectedOriginalValue
        byteValueField.text = byteText(selectedDraftValue)
    }

    function readRange(showStatus) {
        if (!active)
            return

        if (!proxy) {
            statusText = qsTr("Proxy is not available")
            return
        }

        var offset = Math.max(0, parseNumber(addressField.text, rangeOffset))
        var length = Math.max(1, Math.min(256, parseNumber(lengthField.text, 256)))
        var result = proxy.readEcMemory(offset, length)
        if (!result || !result.ok) {
            statusText = result && result.error ? result.error : qsTr("Read failed")
            return
        }

        var previousOffset = rangeOffset
        var previousBytes = bytes ? bytes.slice() : []
        var nextOffset = Number(result.offset || offset)
        var nextBytes = result.bytes || []
        var changedCount = updateChangedAddresses(previousOffset, previousBytes, nextOffset, nextBytes)

        rangeOffset = nextOffset
        memorySize = Number(result.size || 0)
        bytes = nextBytes
        if (selectedAddress < rangeOffset || selectedAddress >= rangeOffset + bytes.length)
            selectAddress(rangeOffset)
        else
            selectAddress(selectedAddress)

        if (showStatus)
            statusText = qsTr("Read ") + bytes.length + qsTr(" bytes") +
                         (changedCount > 0 ? qsTr(", changed ") + changedCount : "")
    }

    function writeSelectedByte() {
        if (selectedAddress < 0)
            return

        setDraftValue(parseNumber(byteValueField.text, selectedDraftValue))
        var result = proxy.writeEcMemory(selectedAddress, [selectedDraftValue])
        if (!result || !result.ok) {
            statusText = result && result.error ? result.error : qsTr("Write failed")
            return
        }

        updateLocalByte(selectedAddress, selectedDraftValue)
        markChangedAddress(selectedAddress)
        statusText = qsTr("Written ") + hex(selectedAddress, 2)
    }

    function applySelectedBits() {
        if (selectedAddress < 0)
            return

        var mask = selectedOriginalValue ^ selectedDraftValue
        if (mask === 0) {
            statusText = qsTr("No bit changes")
            return
        }

        var result = proxy.writeEcMemoryBits(selectedAddress, mask, selectedDraftValue)
        if (!result || !result.ok) {
            statusText = result && result.error ? result.error : qsTr("Bit write failed")
            return
        }

        updateLocalByte(selectedAddress, result.byte !== undefined ? result.byte : selectedDraftValue)
        markChangedAddress(selectedAddress)
        statusText = qsTr("Bits written ") + hex(selectedAddress, 2)
    }

    function requestWriteSelectedByte() {
        if (selectedAddress < 0)
            return
        setDraftValue(parseNumber(byteValueField.text, selectedDraftValue))
        pendingWriteMode = "byte"
        writeConfirmDialog.open()
    }

    function requestApplySelectedBits() {
        if (selectedAddress < 0)
            return
        var mask = selectedOriginalValue ^ selectedDraftValue
        if (mask === 0) {
            statusText = qsTr("No bit changes")
            return
        }
        pendingWriteMode = "bits"
        writeConfirmDialog.open()
    }

    Component.onCompleted: {
        if (active)
            readRange(false)
    }

    onActiveChanged: {
        if (!active) {
            autoRefresh = false
        } else if (bytes.length === 0) {
            readRange(false)
        }
    }

    Connections {
        target: proxy
        function onConnectionChanged(isConnected) {
            if (isConnected && root.active)
                readRange(false)
        }
    }

    Timer {
        interval: 1000
        repeat: true
        running: root.active && root.autoRefresh
        onTriggered: root.readRange(false)
    }

    Timer {
        id: changePruneTimer

        interval: 250
        repeat: true
        onTriggered: root.pruneChangedAddresses()
    }

    Dialog {
        id: writeConfirmDialog

        modal: true
        focus: true
        title: qsTr("Confirm EC write")
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: Math.min(430, root.width - 40)
        x: Math.round((root.width - width) / 2)
        y: Math.round((root.height - height) / 2)
        onAccepted: {
            if (root.pendingWriteMode === "bits")
                root.applySelectedBits()
            else
                root.writeSelectedByte()
            root.pendingWriteMode = ""
        }
        onRejected: root.pendingWriteMode = ""

        contentItem: Label {
            width: parent.width
            text: qsTr("Write EC address ") + root.hex(root.selectedAddress, 2) +
                  qsTr(" from ") + root.hex(root.selectedOriginalValue, 2) +
                  qsTr(" to ") + root.hex(root.selectedDraftValue, 2) +
                  qsTr(". Wrong EC writes can freeze the laptop or break cooling until reboot.")
            color: root.textColor
            wrapMode: Text.WordWrap
            font.pixelSize: 12
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Memory debug")
                    color: root.textColor
                    font.pixelSize: 22
                    font.bold: true
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    text: memorySize > 0 ? qsTr("EC buffer ") + memorySize + qsTr(" bytes") : qsTr("EC buffer")
                    color: root.mutedTextColor
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
            }

            StyledButton {
                text: qsTr("Refresh")
                accent: true
                onClicked: root.readRange(true)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 8
                color: root.surfaceColor
                border.color: root.borderColor

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: qsTr("Address")
                            color: root.textColor
                            font.pixelSize: 12
                            font.bold: true
                        }

                        DebugField {
                            id: addressField
                            Layout.preferredWidth: 92
                            text: "0x00"
                            onAccepted: root.readRange(true)
                        }

                        Label {
                            text: qsTr("Length")
                            color: root.textColor
                            font.pixelSize: 12
                            font.bold: true
                        }

                        DebugField {
                            id: lengthField
                            Layout.preferredWidth: 86
                            text: "0x100"
                            onAccepted: root.readRange(true)
                        }

                        StyledButton {
                            text: qsTr("Read")
                            compact: true
                            onClicked: root.readRange(true)
                        }

                        Item { Layout.fillWidth: true }

                        ToggleButton {
                            text: qsTr("Auto")
                            checked: root.autoRefresh
                            onClicked: root.autoRefresh = !root.autoRefresh
                        }
                    }

                    ScrollView {
                        id: memoryScroll
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        Column {
                            width: Math.max(memoryScroll.availableWidth, 670)
                            spacing: 5

                            Repeater {
                                model: root.rowCount

                                Row {
                                    id: memoryRow

                                    property int rowIndex: modelData

                                    width: parent.width
                                    height: 28
                                    spacing: 5

                                    Label {
                                        width: 58
                                        height: parent.height
                                        text: root.hex(root.rangeOffset + memoryRow.rowIndex * root.bytesPerRow, 2)
                                        color: root.mutedTextColor
                                        font.family: "monospace"
                                        font.pixelSize: 11
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    Repeater {
                                        model: root.bytesPerRow

                                        ByteCell {
                                            readonly property int flatIndex: memoryRow.rowIndex * root.bytesPerRow + modelData

                                            absoluteAddress: root.rangeOffset + flatIndex
                                            byteValue: flatIndex < root.bytes.length ? root.bytes[flatIndex] : 0
                                            visible: flatIndex < root.bytes.length
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.statusText
                        color: root.statusText.toLowerCase().indexOf("failed") >= 0 ||
                               root.statusText.toLowerCase().indexOf("invalid") >= 0 ? root.dangerColor : root.mutedTextColor
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }
                }
            }

            Rectangle {
                Layout.preferredWidth: 270
                Layout.fillHeight: true
                radius: 8
                color: root.surfaceColor
                border.color: root.borderColor

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Byte editor")
                            color: root.textColor
                            font.pixelSize: 15
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        Label {
                            Layout.fillWidth: true
                            text: selectedAddress >= 0 ? root.hex(selectedAddress, 2) : qsTr("No byte selected")
                            color: root.mutedTextColor
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        DebugField {
                            id: byteValueField
                            Layout.fillWidth: true
                            enabled: selectedAddress >= 0
                            text: "00"
                            inputMethodHints: Qt.ImhPreferUppercase | Qt.ImhNoPredictiveText
                            onAccepted: root.requestWriteSelectedByte()
                            onEditingFinished: root.setDraftValue(root.parseNumber(text, root.selectedDraftValue))
                        }

                        StyledButton {
                            text: qsTr("Write")
                            accent: true
                            enabled: selectedAddress >= 0
                            onClicked: root.requestWriteSelectedByte()
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: root.borderColor
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 4
                        columnSpacing: 8
                        rowSpacing: 8

                        Repeater {
                            model: [7, 6, 5, 4, 3, 2, 1, 0]

                            BitCell {
                                bit: modelData
                                checked: root.bitChecked(modelData)
                                enabled: selectedAddress >= 0
                                onClicked: root.setDraftBit(bit, !checked)
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        StyledButton {
                            text: qsTr("Apply bits")
                            accent: true
                            enabled: selectedAddress >= 0
                            onClicked: root.requestApplySelectedBits()
                        }

                        StyledButton {
                            text: qsTr("Reset")
                            enabled: selectedAddress >= 0
                            onClicked: root.setDraftValue(root.selectedOriginalValue)
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 76
                        radius: 8
                        color: root.elevatedColor
                        border.color: root.borderColor

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Decimal ") + root.selectedDraftValue
                                color: root.textColor
                                font.pixelSize: 12
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Binary ") + root.binaryText(root.selectedDraftValue)
                                color: root.mutedTextColor
                                font.family: "monospace"
                                font.pixelSize: 12
                                elide: Text.ElideRight
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }
    }

    component ByteCell: Rectangle {
        id: byteCell

        property int absoluteAddress: 0
        property int byteValue: 0
        readonly property bool selected: absoluteAddress === root.selectedAddress
        readonly property bool changed: root.isAddressChanged(absoluteAddress)

        width: 34
        height: 28
        radius: 6
        color: selected ? Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.24)
                        : changed ? Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.14)
                        : Qt.rgba(root.elevatedColor.r, root.elevatedColor.g, root.elevatedColor.b, mouseArea.containsMouse ? 0.82 : 0.44)
        border.width: selected ? 2 : (changed ? 1 : 0)
        border.color: Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, changed && !selected ? 0.82 : 1.0)

        Behavior on color { ColorAnimation { duration: 140 } }
        Behavior on border.color { ColorAnimation { duration: 140 } }

        Label {
            anchors.centerIn: parent
            text: root.byteText(byteCell.byteValue)
            color: byteCell.selected || byteCell.changed ? root.textColor : root.mutedTextColor
            font.family: "monospace"
            font.pixelSize: 12
            font.bold: byteCell.selected || byteCell.changed
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.selectAddress(byteCell.absoluteAddress)
        }
    }

    component BitCell: Rectangle {
        id: bitCell

        property int bit: 0
        property bool checked: false
        signal clicked()

        Layout.fillWidth: true
        Layout.preferredHeight: 34
        radius: 8
        color: checked ? Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.22)
                       : Qt.rgba(root.elevatedColor.r, root.elevatedColor.g, root.elevatedColor.b, 0.48)
        border.color: checked ? root.accentColor : root.borderColor
        opacity: enabled ? 1.0 : 0.44

        Label {
            anchors.centerIn: parent
            text: bitCell.bit
            color: bitCell.checked ? root.textColor : root.mutedTextColor
            font.pixelSize: 12
            font.bold: bitCell.checked
        }

        MouseArea {
            anchors.fill: parent
            enabled: bitCell.enabled
            cursorShape: Qt.PointingHandCursor
            onClicked: bitCell.clicked()
        }
    }

    component ToggleButton: Button {
        id: button

        implicitWidth: 72
        implicitHeight: 32
        padding: 0

        background: Rectangle {
            radius: 8
            color: button.hovered || button.pressed
                   ? Qt.rgba(root.elevatedColor.r, root.elevatedColor.g, root.elevatedColor.b, 0.82)
                   : Qt.rgba(root.elevatedColor.r, root.elevatedColor.g, root.elevatedColor.b, 0.42)
            border.color: button.checked
                          ? Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.92)
                          : root.borderColor
            Behavior on color { ColorAnimation { duration: 120 } }
            Behavior on border.color { ColorAnimation { duration: 120 } }
        }

        contentItem: RowLayout {
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: button.text
                color: button.checked ? root.textColor : root.mutedTextColor
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 12
                font.bold: button.checked
                elide: Text.ElideRight
            }

            Rectangle {
                Layout.preferredWidth: 30
                Layout.preferredHeight: 16
                radius: 8
                color: button.checked
                       ? Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.86)
                       : Qt.rgba(root.borderColor.r, root.borderColor.g, root.borderColor.b, 0.74)
                border.color: button.checked ? root.accentColor : root.borderColor

                Rectangle {
                    width: 12
                    height: 12
                    radius: 6
                    anchors.verticalCenter: parent.verticalCenter
                    x: button.checked ? parent.width - width - 2 : 2
                    color: button.checked ? "#ffffff" : root.mutedTextColor

                    Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                    Behavior on color { ColorAnimation { duration: 120 } }
                }

                Behavior on color { ColorAnimation { duration: 120 } }
                Behavior on border.color { ColorAnimation { duration: 120 } }
            }
        }
    }

    component DebugField: TextField {
        id: field

        color: root.textColor
        placeholderTextColor: root.mutedTextColor
        selectedTextColor: "#ffffff"
        selectionColor: root.accentColor
        font.family: "monospace"
        font.pixelSize: 12
        leftPadding: 10
        rightPadding: 10

        background: Rectangle {
            radius: 8
            color: root.elevatedColor
            border.color: field.activeFocus ? root.accentColor : root.borderColor
        }
    }

    component StyledButton: Button {
        id: button

        property bool accent: false
        property bool compact: false

        implicitWidth: Math.max(compact ? 70 : 92, label.implicitWidth + (compact ? 22 : 30))
        implicitHeight: compact ? 32 : 34
        padding: 0

        background: Rectangle {
            radius: 8
            color: !button.enabled ? Qt.rgba(root.elevatedColor.r, root.elevatedColor.g, root.elevatedColor.b, 0.35)
                  : button.accent ? root.accentColor
                  : button.hovered || button.pressed ? Qt.rgba(root.elevatedColor.r, root.elevatedColor.g, root.elevatedColor.b, 0.82)
                  : "transparent"
            border.width: button.accent ? 0 : 1
            border.color: root.borderColor
            opacity: button.enabled ? 1.0 : 0.48
        }

        contentItem: Label {
            id: label
            text: button.text
            color: button.accent ? "#ffffff" : root.textColor
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 12
            font.bold: button.accent
            elide: Text.ElideRight
        }
    }
}
