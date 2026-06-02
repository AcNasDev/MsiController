import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

AppCard {
    id: root

    property var parameter
    property var proxy
    property color accentColor: "#3fa7ff"
    property color secondaryAccentColor: "#6bd0c4"
    property color elevatedColor: "#202735"
    property var displayedDevices: []
    property int updateLocks: 0
    readonly property var devices: parameter && parameter.isValid && parameter.value ? parameter.value : []
    readonly property bool updatesLocked: updateLocks > 0

    title: qsTr("GPU controls")
    subtitle: displayedDevices.length > 1 ? qsTr("%1 devices").arg(displayedDevices.length) : qsTr("Driver controls")
    visible: displayedDevices.length > 0
    implicitHeight: Math.max(150, devicesColumn.implicitHeight + 76)

    Component.onCompleted: syncDisplayedDevices()
    onDevicesChanged: {
        if (!updatesLocked)
            syncDisplayedDevices()
    }
    onUpdatesLockedChanged: {
        if (!updatesLocked)
            syncDisplayedDevicesTimer.restart()
    }

    Timer {
        id: syncDisplayedDevicesTimer
        interval: 200
        repeat: false
        onTriggered: {
            if (!root.updatesLocked)
                root.syncDisplayedDevices()
        }
    }

    function syncDisplayedDevices() {
        displayedDevices = devices
    }

    function beginUpdateLock() {
        updateLocks += 1
    }

    function endUpdateLock() {
        updateLocks = Math.max(0, updateLocks - 1)
    }

    function deviceById(deviceId) {
        for (var i = 0; i < devices.length; ++i) {
            if (devices[i].id === deviceId)
                return devices[i]
        }
        return ({})
    }

    function deviceNumber(deviceId, key, fallback) {
        var value = Number(deviceById(deviceId)[key])
        return isNaN(value) ? fallback : value
    }

    function setDeviceValue(deviceId, key, value) {
        if (!parameter || !parameter.isValid || !proxy || typeof proxy.setGpuControlValue !== "function")
            return
        proxy.setGpuControlValue(deviceId, key, value)
    }

    function indexOfValue(values, value) {
        if (!values)
            return -1
        for (var i = 0; i < values.length; ++i) {
            if (values[i] === value)
                return i
        }
        return -1
    }

    function wattsText(value) {
        var number = Number(value)
        if (isNaN(number) || number <= 0)
            return qsTr("N/A")
        return number.toFixed(number >= 100 ? 0 : 1) + qsTr(" W")
    }

    function tempText(value) {
        var number = Number(value)
        if (isNaN(number) || number <= 0)
            return qsTr("N/A")
        return number.toFixed(0) + qsTr(" °C")
    }

    function metadata(device) {
        return [device.vendor, device.driver, device.pciBusId].filter(function(item) {
            return item !== undefined && item !== null && item.toString().length > 0
        }).join("  ")
    }

    ColumnLayout {
        id: devicesColumn

        Layout.fillWidth: true
        spacing: 12

        Repeater {
            model: root.displayedDevices

            delegate: ColumnLayout {
                id: deviceSection

                readonly property bool hasPowerLimit: modelData.canSetPowerLimit === true
                readonly property bool hasPerformance: modelData.canSetPerformanceLevel === true
                readonly property bool hasPersistence: modelData.canSetPersistenceMode === true
                readonly property color deviceAccent: modelData.vendor === "AMD" ? root.secondaryAccentColor : root.accentColor

                Layout.fillWidth: true
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Rectangle {
                        Layout.preferredWidth: 4
                        Layout.preferredHeight: 48
                        radius: 3
                        color: deviceSection.deviceAccent
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.minimumWidth: 0
                        spacing: 7

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Label {
                                Layout.fillWidth: true
                                text: modelData.name || modelData.vendor || qsTr("GPU")
                                color: root.textColor
                                font.pixelSize: 14
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            Label {
                                Layout.fillWidth: true
                                text: root.metadata(modelData)
                                color: root.mutedTextColor
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: 8

                            MetricPill {
                                label: qsTr("Temp")
                                value: root.tempText(modelData.temperatureC)
                                accent: deviceSection.deviceAccent
                            }

                            MetricPill {
                                label: qsTr("Power")
                                value: root.wattsText(modelData.powerDrawWatts)
                                accent: root.accentColor
                            }

                            MetricPill {
                                visible: modelData.powerLimitWatts !== undefined
                                label: qsTr("Limit")
                                value: root.wattsText(modelData.powerLimitWatts)
                                accent: root.accentColor
                            }

                            MetricPill {
                                visible: root.parameter && root.parameter.isPending
                                label: qsTr("State")
                                value: qsTr("Applying")
                                accent: root.accentColor
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    ControlSwitch {
                        Layout.fillWidth: true
                        visible: deviceSection.hasPersistence
                        title: qsTr("Persistence")
                        subtitle: qsTr("NVIDIA driver stays ready")
                        checked: modelData.persistenceMode === true
                        enabled: root.parameter && root.parameter.isValid
                        accent: root.accentColor
                        surface: root.elevatedColor
                        borderColor: root.borderColor
                        textColor: root.textColor
                        mutedTextColor: root.mutedTextColor
                        onToggled: function(checked) {
                            root.setDeviceValue(modelData.id, "persistenceMode", checked)
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        visible: deviceSection.hasPerformance
                        radius: 8
                        color: Qt.rgba(root.elevatedColor.r, root.elevatedColor.g, root.elevatedColor.b, 0.34)
                        border.color: root.borderColor

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 10

                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                spacing: 1

                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("Performance")
                                    color: root.textColor
                                    font.pixelSize: 12
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("AMD power profile")
                                    color: root.mutedTextColor
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                }
                            }

                            StyledComboBox {
                                id: performanceCombo
                                Layout.preferredWidth: Math.min(190, Math.max(126, parent.width * 0.34))
                                Layout.minimumWidth: 112
                                model: modelData.availablePerformanceLevels || []
                                currentIndex: root.indexOfValue(modelData.availablePerformanceLevels, modelData.performanceLevel)
                                enabled: root.parameter && root.parameter.isValid
                                onActivated: function(index) {
                                    root.setDeviceValue(modelData.id, "performanceLevel", performanceCombo.textAt(index))
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 50
                        visible: deviceSection.hasPowerLimit
                        radius: 8
                        color: Qt.rgba(root.elevatedColor.r, root.elevatedColor.g, root.elevatedColor.b, 0.34)
                        border.color: root.borderColor

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 10

                            Label {
                                Layout.preferredWidth: 92
                                text: qsTr("Power limit")
                                color: root.textColor
                                font.pixelSize: 12
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            Slider {
                                id: powerSlider

                                Layout.fillWidth: true
                                from: Math.max(1, Number(modelData.minPowerLimitWatts || 1))
                                to: Math.max(from, Number(modelData.maxPowerLimitWatts || from))
                                stepSize: 1
                                snapMode: Slider.SnapAlways
                                enabled: root.parameter && root.parameter.isValid
                                property real draftWatts: root.deviceNumber(modelData.id,
                                                                             "powerLimitWatts",
                                                                             Number(modelData.powerLimitWatts || from))
                                value: Math.max(from, Math.min(to, draftWatts))
                                onMoved: draftWatts = Math.round(value)
                                onPressedChanged: {
                                    if (pressed) {
                                        root.beginUpdateLock()
                                    } else {
                                        root.setDeviceValue(modelData.id, "powerLimitWatts", Math.round(value))
                                        root.endUpdateLock()
                                    }
                                }

                                Connections {
                                    target: root.parameter
                                    function onValueChanged() {
                                        if (!powerSlider.pressed) {
                                            powerSlider.draftWatts = root.deviceNumber(modelData.id,
                                                                                       "powerLimitWatts",
                                                                                       powerSlider.draftWatts)
                                        }
                                    }
                                }
                            }

                            ValueBadge {
                                value: Math.round(powerSlider.value) + qsTr(" W")
                                accent: root.accentColor
                                surface: root.surfaceColor
                                borderColor: root.borderColor
                                textColor: root.textColor
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    visible: index < root.displayedDevices.length - 1
                    color: Qt.rgba(root.borderColor.r, root.borderColor.g, root.borderColor.b, 0.72)
                }
            }
        }
    }

    component MetricPill: Rectangle {
        property string label: ""
        property string value: ""
        property color accent: "#3fa7ff"

        width: Math.max(84, pillContent.implicitWidth + 18)
        height: 30
        radius: 8
        color: Qt.rgba(accent.r, accent.g, accent.b, 0.11)
        border.color: Qt.rgba(accent.r, accent.g, accent.b, 0.54)

        RowLayout {
            id: pillContent

            anchors.centerIn: parent
            spacing: 5

            Label {
                text: label
                color: root.mutedTextColor
                font.pixelSize: 11
                elide: Text.ElideRight
            }

            Label {
                text: value
                color: root.textColor
                font.pixelSize: 11
                font.bold: true
                elide: Text.ElideRight
            }
        }
    }

    component ControlSwitch: Rectangle {
        id: control

        property string title: ""
        property string subtitle: ""
        property bool checked: false
        property color accent: "#3fa7ff"
        property color surface: "#202735"
        property color borderColor: "#2b3443"
        property color textColor: "#f4f7fb"
        property color mutedTextColor: "#9aa6b6"
        signal toggled(bool checked)

        implicitHeight: 48
        radius: 8
        color: control.enabled
               ? Qt.rgba(surface.r, surface.g, surface.b, 0.34)
               : Qt.rgba(surface.r, surface.g, surface.b, 0.22)
        border.color: checked ? Qt.rgba(accent.r, accent.g, accent.b, 0.78) : borderColor
        opacity: control.enabled ? 1.0 : 0.55

        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            ColumnLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: 1

                Label {
                    Layout.fillWidth: true
                    text: control.title
                    color: control.textColor
                    font.pixelSize: 12
                    font.bold: true
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    text: control.subtitle
                    color: control.mutedTextColor
                    font.pixelSize: 11
                    elide: Text.ElideRight
                    visible: control.subtitle.length > 0
                }
            }

            InlineSwitch {
                checked: control.checked
                accent: control.accent
                track: control.surface
                borderColor: control.borderColor
                enabled: control.enabled
                onToggled: function(checked) {
                    control.toggled(checked)
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            enabled: control.enabled
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: control.toggled(!control.checked)
        }
    }

    component StyledComboBox: ComboBox {
        id: combo

        property bool updateLockHeld: false

        implicitHeight: 34
        leftPadding: 11
        rightPadding: 32

        function setUpdateLock(active) {
            if (updateLockHeld === active)
                return
            updateLockHeld = active
            if (active)
                root.beginUpdateLock()
            else
                root.endUpdateLock()
        }

        Component.onDestruction: {
            if (updateLockHeld)
                root.endUpdateLock()
        }

        Connections {
            target: combo.popup
            function onVisibleChanged() {
                combo.setUpdateLock(combo.popup.visible)
            }
        }

        contentItem: Label {
            leftPadding: combo.leftPadding
            rightPadding: combo.rightPadding
            text: combo.displayText
            color: root.textColor
            font.pixelSize: 12
            font.bold: true
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        indicator: Item {
            x: combo.width - width - 12
            y: (combo.height - height) / 2
            width: 12
            height: 8

            Rectangle {
                x: 1
                y: 3
                width: 7
                height: 2
                radius: 1
                color: root.mutedTextColor
                rotation: 45
                antialiasing: true
            }

            Rectangle {
                x: 5
                y: 3
                width: 7
                height: 2
                radius: 1
                color: root.mutedTextColor
                rotation: -45
                antialiasing: true
            }
        }

        background: Rectangle {
            radius: 8
            color: combo.pressed || combo.popup.visible
                   ? Qt.rgba(root.elevatedColor.r, root.elevatedColor.g, root.elevatedColor.b, 0.72)
                   : Qt.rgba(root.elevatedColor.r, root.elevatedColor.g, root.elevatedColor.b, 0.42)
            border.color: combo.popup.visible ? root.accentColor : root.borderColor
        }

        delegate: ItemDelegate {
            id: comboDelegate
            width: combo.width - 12
            height: 34
            highlighted: combo.highlightedIndex === index

            contentItem: Label {
                text: modelData
                color: comboDelegate.highlighted || combo.currentIndex === index ? root.textColor : root.mutedTextColor
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 12
                font.bold: combo.currentIndex === index
                elide: Text.ElideRight
            }

            background: Rectangle {
                radius: 7
                color: comboDelegate.highlighted
                       ? Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.18)
                       : "transparent"
                border.width: combo.currentIndex === index ? 1 : 0
                border.color: Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.56)
            }
        }

        popup: Popup {
            y: combo.height + 6
            width: combo.width
            implicitHeight: contentItem.implicitHeight + topPadding + bottomPadding
            padding: 6

            contentItem: ListView {
                clip: true
                implicitHeight: contentHeight
                model: combo.popup.visible ? combo.delegateModel : null
                currentIndex: combo.highlightedIndex
                interactive: contentHeight > height
            }

            background: Rectangle {
                radius: 8
                color: root.surfaceColor
                border.color: root.borderColor
            }
        }
    }

    component ValueBadge: Rectangle {
        property string value: ""
        property color accent: "#3fa7ff"
        property color surface: "#202735"
        property color borderColor: "#2b3443"
        property color textColor: "#f4f7fb"

        Layout.preferredWidth: Math.max(78, valueText.implicitWidth + 20)
        Layout.preferredHeight: 36
        radius: 8
        color: surface
        border.color: borderColor

        Label {
            id: valueText
            anchors.centerIn: parent
            text: value
            color: accent
            font.pixelSize: 13
            font.bold: true
            elide: Text.ElideRight
        }
    }

    component InlineSwitch: Item {
        id: switchRoot

        property bool checked: false
        property color accent: "#3fa7ff"
        property color track: "#202735"
        property color borderColor: "#2b3443"
        signal toggled(bool checked)

        implicitWidth: 38
        implicitHeight: 20

        Rectangle {
            anchors.fill: parent
            radius: height / 2
            color: switchRoot.checked
                   ? Qt.rgba(switchRoot.accent.r, switchRoot.accent.g, switchRoot.accent.b, 0.86)
                   : Qt.rgba(switchRoot.borderColor.r, switchRoot.borderColor.g, switchRoot.borderColor.b, 0.72)
            border.color: switchRoot.checked ? switchRoot.accent : switchRoot.borderColor

            Behavior on color { ColorAnimation { duration: 120 } }
            Behavior on border.color { ColorAnimation { duration: 120 } }
        }

        Rectangle {
            width: 14
            height: 14
            radius: 7
            anchors.verticalCenter: parent.verticalCenter
            x: switchRoot.checked ? switchRoot.width - width - 3 : 3
            color: switchRoot.checked ? "#ffffff" : root.mutedTextColor

            Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
            Behavior on color { ColorAnimation { duration: 120 } }
        }
    }
}
