import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore

Item {
    id: root

    property var proxy
    property color surfaceColor: "#181e29"
    property color elevatedColor: "#202735"
    property color borderColor: "#2b3443"
    property color textColor: "#f4f7fb"
    property color mutedTextColor: "#9aa7b8"
    property color accentColor: "#3fa7ff"
    property color goodColor: "#5bd48a"
    property color warnColor: "#f2b84b"
    property color dangerColor: "#ff6b6b"
    readonly property var diagnostics: proxy && proxy.diagnostics ? proxy.diagnostics : ({})
    readonly property var firmware: diagnostics.firmware || ({})
    readonly property var memory: diagnostics.memory || ({})
    readonly property var activeProfile: diagnostics.activeProfile || ({})
    readonly property var system: diagnostics.system || ({})
    readonly property var dbus: diagnostics.dbus || ({})
    readonly property var ecWritePolicy: diagnostics.ecWritePolicy || ({})
    readonly property var warnings: diagnostics.warnings || []
    property bool rawDetailsExpanded: false

    function boolText(value) {
        return value ? qsTr("OK") : qsTr("Missing")
    }

    function textValue(value, fallback) {
        if (value === undefined || value === null || value === "")
            return fallback || "N/A"
        return value.toString()
    }

    function listText(values) {
        var items = []
        for (var i = 0; i < values.length; i++)
            items.push(values[i].toString())
        return items.join("\n")
    }

    function healthTitle() {
        if (!proxy || !proxy.diagnostics || Object.keys(root.diagnostics).length === 0)
            return qsTr("Waiting for diagnostics")
        if (proxy.restartRequired)
            return qsTr("Restart required")
        if (root.warnings.length > 0)
            return qsTr("Needs attention")
        return qsTr("Healthy")
    }

    function healthDetail() {
        if (!proxy || !proxy.diagnostics || Object.keys(root.diagnostics).length === 0)
            return qsTr("Refresh diagnostics after the service connects.")
        if (proxy.restartRequired)
            return qsTr("Restart the desktop client so it reconnects to the updated service API.")
        if (root.warnings.length > 0)
            return root.warnings[0].toString()
        return qsTr("Service, firmware profile and EC memory are available.")
    }

    function healthColor() {
        if (proxy && proxy.restartRequired)
            return root.warnColor
        if (root.warnings.length > 0)
            return root.warnColor
        if (root.diagnostics.apiVersion !== undefined)
            return root.goodColor
        return root.accentColor
    }

    function saveBundle() {
        if (!proxy)
            return
        var result = proxy.saveSupportBundle(bundlePathField.text)
        statusLabel.text = result && result.ok
                           ? qsTr("Support bundle saved to ") + result.path
                           : (result && result.error ? result.error : qsTr("Support bundle failed"))
    }

    function exportSettings() {
        if (!proxy)
            return
        var result = proxy.exportAppSettings(settingsPathField.text)
        statusLabel.text = result && result.ok
                           ? qsTr("Settings exported to ") + result.path
                           : (result && result.error ? result.error : qsTr("Settings export failed"))
    }

    function importSettings() {
        if (!proxy)
            return
        var result = proxy.importAppSettings(settingsPathField.text)
        statusLabel.text = result && result.ok
                           ? qsTr("Settings imported")
                           : (result && result.error ? result.error : qsTr("Settings import failed"))
    }

    function exportTelemetry() {
        if (!proxy)
            return
        var result = proxy.exportTelemetryHistory(telemetryPathField.text)
        statusLabel.text = result && result.ok
                           ? qsTr("Telemetry exported to ") + result.path
                           : (result && result.error ? result.error : qsTr("Telemetry export failed"))
    }

    Component.onCompleted: if (proxy) proxy.refreshDiagnostics()

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
                    text: qsTr("Diagnostics")
                    color: root.textColor
                    font.pixelSize: 22
                    font.bold: true
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    text: proxy && proxy.diagnosticsStatus ? proxy.diagnosticsStatus : qsTr("Waiting for service")
                    color: proxy && proxy.restartRequired ? root.warnColor : root.mutedTextColor
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
            }

            StyledButton {
                text: qsTr("Refresh")
                accent: true
                onClicked: proxy.refreshDiagnostics()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 72
            radius: 8
            color: Qt.rgba(root.healthColor().r, root.healthColor().g, root.healthColor().b, 0.12)
            border.color: root.healthColor()

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12

                Rectangle {
                    Layout.preferredWidth: 10
                    Layout.preferredHeight: 10
                    radius: 5
                    color: root.healthColor()
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        Layout.fillWidth: true
                        text: root.healthTitle()
                        color: root.textColor
                        font.pixelSize: 15
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: root.healthDetail()
                        color: root.mutedTextColor
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }
                }

                StyledButton {
                    text: qsTr("Save bundle")
                    accent: true
                    onClicked: root.saveBundle()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            radius: 8
            visible: proxy && proxy.restartRequired
            color: Qt.rgba(root.warnColor.r, root.warnColor.g, root.warnColor.b, 0.14)
            border.color: root.warnColor

            Label {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                verticalAlignment: Text.AlignVCenter
                text: qsTr("Client and service versions differ. Restart the client after package updates.")
                color: root.textColor
                font.pixelSize: 12
                elide: Text.ElideRight
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(96, 36 + root.warnings.length * 18)
            radius: 8
            visible: root.warnings.length > 0
            color: Qt.rgba(root.warnColor.r, root.warnColor.g, root.warnColor.b, 0.11)
            border.color: root.warnColor

            Label {
                anchors.fill: parent
                anchors.margins: 12
                verticalAlignment: Text.AlignVCenter
                text: root.listText(root.warnings)
                color: root.textColor
                font.pixelSize: 12
                wrapMode: Text.Wrap
                elide: Text.ElideRight
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width > 820 ? 3 : 1
            columnSpacing: 12
            rowSpacing: 12

            InfoCard {
                title: qsTr("Service")
                rows: [
                    [qsTr("API"), root.textValue(root.diagnostics.apiVersion)],
                    [qsTr("Version"), root.textValue(root.diagnostics.serviceVersion)],
                    [qsTr("Qt"), root.textValue(root.diagnostics.qtVersion)],
                    [qsTr("Module"), root.boolText(root.diagnostics.kernelModuleLoaded)]
                ]
            }

            InfoCard {
                title: qsTr("Device")
                rows: [
                    [qsTr("Firmware"), root.textValue(root.firmware.version)],
                    [qsTr("Profile"), root.textValue(root.activeProfile.id)],
                    [qsTr("Parameters"), root.textValue(root.diagnostics.parameterCount)],
                    [qsTr("EC memory"), root.memory.ok ? qsTr("Available") : root.textValue(root.memory.error, qsTr("Unavailable"))]
                ]
            }

            InfoCard {
                title: qsTr("System")
                rows: [
                    [qsTr("OS"), root.textValue(root.system.product)],
                    [qsTr("Kernel"), root.textValue(root.system.kernel)],
                    [qsTr("Arch"), root.textValue(root.system.architecture)],
                    [qsTr("D-Bus"), root.textValue(root.dbus.service)]
                ]
            }

            InfoCard {
                title: qsTr("EC safety")
                rows: [
                    [qsTr("Allowlist"), root.textValue(root.ecWritePolicy.allowedRangeCount)],
                    [qsTr("Override"), root.ecWritePolicy.unsafeOverride ? qsTr("Enabled") : qsTr("Off")],
                    [qsTr("Audit"), root.ecWritePolicy.auditLog ? root.ecWritePolicy.auditLog.length : 0],
                    [qsTr("Mode"), root.diagnostics.memoryBackend && root.diagnostics.memoryBackend.simulator ? qsTr("Simulator") : qsTr("Hardware")]
                ]
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 90
            radius: 8
            color: root.surfaceColor
            border.color: root.borderColor

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Support bundle")
                        color: root.textColor
                        font.pixelSize: 14
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    PathField {
                        id: bundlePathField
                        Layout.fillWidth: true
                        text: StandardPaths.writableLocation(StandardPaths.HomeLocation) + "/msicontroller-support.json"
                    }
                }

                StyledButton {
                    text: qsTr("Save")
                    accent: true
                    onClicked: root.saveBundle()
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: width > 820 ? 2 : 1
            columnSpacing: 12
            rowSpacing: 12

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 104
                radius: 8
                color: root.surfaceColor
                border.color: root.borderColor

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 6

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Application settings")
                        color: root.textColor
                        font.pixelSize: 14
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        PathField {
                            id: settingsPathField
                            Layout.fillWidth: true
                            text: StandardPaths.writableLocation(StandardPaths.HomeLocation) + "/msicontroller-settings.json"
                        }

                        StyledButton {
                            text: qsTr("Import")
                            onClicked: root.importSettings()
                        }

                        StyledButton {
                            text: qsTr("Export")
                            accent: true
                            onClicked: root.exportSettings()
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 104
                radius: 8
                color: root.surfaceColor
                border.color: root.borderColor

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 6

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Telemetry history")
                        color: root.textColor
                        font.pixelSize: 14
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        PathField {
                            id: telemetryPathField
                            Layout.fillWidth: true
                            text: StandardPaths.writableLocation(StandardPaths.HomeLocation) + "/msicontroller-telemetry.json"
                        }

                        StyledButton {
                            text: qsTr("Save")
                            accent: true
                            onClicked: root.exportTelemetry()
                        }
                    }
                }
            }
        }

        Label {
            id: statusLabel
            Layout.fillWidth: true
            color: text.toLowerCase().indexOf("failed") >= 0 || text.toLowerCase().indexOf("error") >= 0
                   ? root.dangerColor
                   : root.mutedTextColor
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Label {
                Layout.fillWidth: true
                text: qsTr("Raw diagnostics")
                color: root.textColor
                font.pixelSize: 14
                font.bold: true
                elide: Text.ElideRight
            }

            StyledButton {
                text: root.rawDetailsExpanded ? qsTr("Hide") : qsTr("Show")
                onClicked: root.rawDetailsExpanded = !root.rawDetailsExpanded
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: root.rawDetailsExpanded
            Layout.preferredHeight: root.rawDetailsExpanded ? 220 : 0
            radius: 8
            visible: root.rawDetailsExpanded
            color: root.surfaceColor
            border.color: root.borderColor

            ScrollView {
                anchors.fill: parent
                anchors.margins: 12
                clip: true

                Label {
                    width: parent.availableWidth
                    text: JSON.stringify(root.diagnostics, null, 2)
                    color: root.mutedTextColor
                    font.family: "monospace"
                    font.pixelSize: 11
                    wrapMode: Text.WrapAnywhere
                }
            }
        }
    }

    component InfoCard: Rectangle {
        id: card

        property string title: ""
        property var rows: []

        Layout.fillWidth: true
        Layout.preferredHeight: 154
        radius: 8
        color: root.surfaceColor
        border.color: root.borderColor

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 7

            Label {
                Layout.fillWidth: true
                text: card.title
                color: root.textColor
                font.pixelSize: 14
                font.bold: true
                elide: Text.ElideRight
            }

            Repeater {
                model: card.rows

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        Layout.preferredWidth: 82
                        text: modelData[0]
                        color: root.mutedTextColor
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }

                    Label {
                        Layout.fillWidth: true
                        text: modelData[1]
                        color: root.textColor
                        font.pixelSize: 12
                        font.bold: true
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }

    component PathField: TextField {
        color: root.textColor
        placeholderTextColor: root.mutedTextColor
        selectedTextColor: "#ffffff"
        selectionColor: root.accentColor
        font.pixelSize: 12
        leftPadding: 10
        rightPadding: 10
        background: Rectangle {
            radius: 8
            color: root.elevatedColor
            border.color: parent.activeFocus ? root.accentColor : root.borderColor
        }
    }

    component StyledButton: Button {
        id: button

        property bool accent: false

        implicitWidth: Math.max(86, label.implicitWidth + 28)
        implicitHeight: 34
        padding: 0

        background: Rectangle {
            radius: 8
            color: !button.enabled ? Qt.rgba(root.elevatedColor.r, root.elevatedColor.g, root.elevatedColor.b, 0.35)
                  : button.accent ? root.accentColor
                  : button.hovered || button.pressed ? Qt.rgba(root.elevatedColor.r, root.elevatedColor.g, root.elevatedColor.b, 0.82)
                  : "transparent"
            border.width: button.accent ? 0 : 1
            border.color: root.borderColor
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
