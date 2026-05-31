import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    implicitWidth: 860
    implicitHeight: 560

    property var proxy
    property string currentFirmware: ""
    property color surfaceColor: "#181e29"
    property color elevatedColor: "#202735"
    property color borderColor: "#2b3443"
    property color textColor: "#f4f7fb"
    property color mutedTextColor: "#9aa7b8"
    property color accentColor: "#3fa7ff"
    property color dangerColor: "#ff6b6b"
    property int selectedIndex: -1
    property var selectedProfile: ({})
    readonly property var filteredProfiles: filterProfiles(searchField.text)

    function profileCount() {
        return proxy && proxy.deviceProfiles ? proxy.deviceProfiles.length : 0
    }

    function profileAt(index) {
        if (!proxy || !proxy.deviceProfiles || index < 0 || index >= proxy.deviceProfiles.length)
            return null
        return proxy.deviceProfiles[index]
    }

    function normalizeFirmwareList(value) {
        if (value === undefined || value === null)
            return []
        if (typeof value === "string")
            return value.length > 0 ? [value] : []
        if (value.join)
            return value
        return value.toString && value.toString().length > 0 ? [value.toString()] : []
    }

    function firmwareList(profile) {
        if (!profile)
            return []
        if (profile.firmware && profile.firmware.length !== undefined)
            return normalizeFirmwareList(profile.firmware)
        if (profile.values && profile.values.AllowedFw)
            return normalizeFirmwareList(profile.values.AllowedFw)
        return []
    }

    function firmwareText(profile) {
        var firmwares = firmwareList(profile)
        if (firmwares === undefined || firmwares === null)
            return ""
        if (firmwares.join)
            return firmwares.join(", ")
        return firmwares.toString()
    }

    function profileSearchText(profile) {
        if (!profile)
            return ""
        return [
            profile.id || "",
            profile.source || "",
            firmwareText(profile),
            JSON.stringify(profile.values || {})
        ].join(" ").toLowerCase()
    }

    function filterProfiles(query) {
        var profiles = []
        var normalizedQuery = (query || "").trim().toLowerCase()
        for (var i = 0; i < profileCount(); ++i) {
            var profile = profileAt(i)
            if (!profile)
                continue
            if (normalizedQuery.length === 0 || profileSearchText(profile).indexOf(normalizedQuery) >= 0)
                profiles.push({sourceIndex: i, profile: profile})
        }
        return profiles
    }

    function selectProfile(index) {
        var profile = profileAt(index)
        if (!profile)
            return

        selectedIndex = index
        selectedProfile = profile
        profileIdField.text = profile.id || ""
        firmwareField.text = firmwareText(profile)
        valuesEditor.text = JSON.stringify(profile.values || {}, null, 2)
        statusLabel.text = ""
    }

    function selectProfileById(id) {
        for (var i = 0; i < profileCount(); ++i) {
            var profile = profileAt(i)
            if (profile && profile.id === id) {
                selectProfile(i)
                return
            }
        }
        if (profileCount() > 0)
            selectProfile(0)
    }

    function splitFirmware(text) {
        var result = []
        var parts = text.split(",")
        for (var i = 0; i < parts.length; ++i) {
            var item = parts[i].trim()
            if (item.length > 0)
                result.push(item)
        }
        return result
    }

    function newProfile() {
        selectedIndex = -1
        selectedProfile = ({source: "user", values: {}})
        profileIdField.text = "USER_" + (currentFirmware && currentFirmware.length > 0 ? currentFirmware.replace(/[^A-Za-z0-9_.-]/g, "_") : "PROFILE")
        firmwareField.text = currentFirmware || ""
        valuesEditor.text = JSON.stringify({
            AllowedFw: splitFirmware(firmwareField.text),
            BatteryThresholdEc: "0xef",
            WebCamEc: "0x2e",
            FnSuperSwapEc: "0xbf",
            ShiftModeEc: "0xf2",
            ShiftModeAvailable: ["Eco:0xc2", "Comfort:0xc1", "Turbo:0xc4"],
            FanModeEc: "0xf4",
            FanModeAvailable: ["Auto:0x0d", "Basic:0x4d", "Advanced:0x8d"],
            GpuTempEc: "0x80"
        }, null, 2)
        statusLabel.text = ""
    }

    function cloneProfile() {
        var profile = selectedProfile
        if (!profile || !profile.values)
            return
        selectedIndex = -1
        selectedProfile = ({source: "user", values: profile.values})
        profileIdField.text = (profile.id || "PROFILE") + "_USER"
        firmwareField.text = firmwareText(profile)
        valuesEditor.text = JSON.stringify(profile.values, null, 2)
        statusLabel.text = ""
    }

    function saveProfile() {
        var values
        try {
            values = JSON.parse(valuesEditor.text)
        } catch (error) {
            statusLabel.text = qsTr("Invalid JSON: ") + error.message
            return
        }

        var id = profileIdField.text.trim()
        if (id.length === 0) {
            statusLabel.text = qsTr("Profile id is empty")
            return
        }

        values.AllowedFw = splitFirmware(firmwareField.text)
        if (values.AllowedFw.length === 0) {
            statusLabel.text = qsTr("Firmware list is empty")
            return
        }

        if (proxy)
            proxy.saveDeviceProfile({id: id, values: values})
    }

    function removeProfile() {
        if (proxy && selectedProfile && selectedProfile.source === "user")
            proxy.removeDeviceProfile(selectedProfile.id)
    }

    Component.onCompleted: {
        if (proxy)
            proxy.refreshDeviceProfiles()
    }

    Connections {
        target: proxy
        function onDeviceProfilesChanged() {
            var previousId = selectedProfile && selectedProfile.id ? selectedProfile.id : ""
            if (previousId.length > 0)
                selectProfileById(previousId)
            else if (profileCount() > 0)
                selectProfile(0)
        }
        function onDeviceProfileStatusChanged() {
            statusLabel.text = proxy.deviceProfileStatus || ""
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
                    text: qsTr("Supported devices")
                    color: root.textColor
                    font.pixelSize: 22
                    font.bold: true
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    text: currentFirmware && currentFirmware.length > 0
                          ? qsTr("Current firmware ") + currentFirmware
                          : qsTr("Current firmware N/A")
                    color: root.mutedTextColor
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
            }

            StyledButton {
                text: qsTr("New")
                accent: true
                onClicked: newProfile()
            }

            StyledButton {
                text: qsTr("Clone")
                enabled: !!selectedProfile && !!selectedProfile.values
                onClicked: cloneProfile()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            Rectangle {
                Layout.preferredWidth: 272
                Layout.fillHeight: true
                radius: 8
                color: root.surfaceColor
                border.color: root.borderColor

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8

                    SearchField {
                        id: searchField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Search profile or firmware")
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("%1 of %2 profiles").arg(root.filteredProfiles.length).arg(root.profileCount())
                            color: root.mutedTextColor
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }

                        StyledButton {
                            text: qsTr("Current")
                            compact: true
                            enabled: root.currentFirmware.length > 0
                            onClicked: searchField.text = root.currentFirmware
                        }
                    }

                    ListView {
                        id: profilesList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 6
                        model: root.filteredProfiles

                        delegate: Rectangle {
                            id: profileDelegate

                            width: profilesList.width
                            height: 58
                            radius: 7
                            color: modelData.sourceIndex === root.selectedIndex
                                   ? Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.18)
                                   : "transparent"
                            border.color: modelData.sourceIndex === root.selectedIndex ? root.accentColor : root.borderColor

                            readonly property var profileData: modelData.profile

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 2

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6

                                    Label {
                                        Layout.fillWidth: true
                                        text: profileDelegate.profileData.id || ""
                                        color: root.textColor
                                        font.pixelSize: 13
                                        font.bold: true
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        text: profileDelegate.profileData.source === "user" ? qsTr("User") : qsTr("Built-in")
                                        color: profileDelegate.profileData.source === "user" ? root.accentColor : root.mutedTextColor
                                        font.pixelSize: 10
                                    }
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: root.firmwareText(profileDelegate.profileData)
                                    color: root.mutedTextColor
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.selectProfile(modelData.sourceIndex)
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 8
                color: root.surfaceColor
                border.color: root.borderColor

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Label {
                            text: qsTr("Profile")
                            color: root.textColor
                            font.pixelSize: 14
                            font.bold: true
                        }

                        Label {
                            Layout.fillWidth: true
                            text: selectedProfile && selectedProfile.source === "builtin"
                                  ? qsTr("Saving creates a user override")
                                  : ""
                            color: root.mutedTextColor
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                    }

                    SearchField {
                        id: profileIdField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Profile id")
                    }

                    SearchField {
                        id: firmwareField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Firmware versions, comma separated")
                    }

                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        TextArea {
                            id: valuesEditor
                            textFormat: TextEdit.PlainText
                            wrapMode: TextEdit.NoWrap
                            color: root.textColor
                            selectedTextColor: "#ffffff"
                            selectionColor: root.accentColor
                            font.family: "monospace"
                            font.pixelSize: 12
                            background: Rectangle {
                                radius: 7
                                color: root.elevatedColor
                                border.color: root.borderColor
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Label {
                            id: statusLabel
                            Layout.fillWidth: true
                            color: text.indexOf("Invalid") === 0 || text.indexOf("empty") >= 0 ? root.dangerColor : root.mutedTextColor
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }

                        StyledButton {
                            text: qsTr("Delete")
                            danger: true
                            enabled: selectedProfile && selectedProfile.source === "user"
                            onClicked: removeProfile()
                        }

                        StyledButton {
                            text: qsTr("Save")
                            accent: true
                            onClicked: saveProfile()
                        }
                    }
                }
            }
        }
    }

    component SearchField: TextField {
        id: field

        color: root.textColor
        placeholderTextColor: root.mutedTextColor
        selectedTextColor: "#ffffff"
        selectionColor: root.accentColor
        font.pixelSize: 12
        leftPadding: 12
        rightPadding: clearButton.visible ? 34 : 12

        background: Rectangle {
            radius: 8
            color: root.elevatedColor
            border.color: field.activeFocus ? root.accentColor : root.borderColor
            border.width: 1
        }

        Button {
            id: clearButton
            width: 24
            height: 24
            anchors.right: parent.right
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            visible: field.text.length > 0
            padding: 0
            text: "x"
            onClicked: field.text = ""

            background: Rectangle {
                radius: 12
                color: clearButton.hovered ? Qt.rgba(root.borderColor.r, root.borderColor.g, root.borderColor.b, 0.65)
                                           : "transparent"
            }

            contentItem: Label {
                text: clearButton.text
                color: root.mutedTextColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 12
                font.bold: true
            }
        }
    }

    component StyledButton: Button {
        id: button

        property bool accent: false
        property bool danger: false
        property bool compact: false

        implicitWidth: Math.max(compact ? 72 : 88, label.implicitWidth + (compact ? 22 : 30))
        implicitHeight: compact ? 30 : 34
        padding: 0

        background: Rectangle {
            radius: 8
            color: !button.enabled ? Qt.rgba(root.elevatedColor.r, root.elevatedColor.g, root.elevatedColor.b, 0.35)
                  : button.accent ? root.accentColor
                  : button.danger && button.hovered ? Qt.rgba(root.dangerColor.r, root.dangerColor.g, root.dangerColor.b, 0.16)
                  : button.hovered || button.pressed ? Qt.rgba(root.elevatedColor.r, root.elevatedColor.g, root.elevatedColor.b, 0.82)
                  : "transparent"
            border.width: button.accent ? 0 : 1
            border.color: button.danger ? root.dangerColor : root.borderColor
            opacity: button.enabled ? 1.0 : 0.48
        }

        contentItem: Label {
            id: label
            text: button.text
            color: button.accent ? "#ffffff" : (button.danger ? root.dangerColor : root.textColor)
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 12
            font.bold: button.accent
            elide: Text.ElideRight
        }
    }
}
