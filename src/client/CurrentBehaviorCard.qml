import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MSI.Helpers 1.0

AppCard {
    id: root

    property var proxy
    property var shiftModeParameter
    property color accentColor: "#3fa7ff"
    property color elevatedColor: "#202735"
    readonly property bool shiftActive: shiftModeParameter && shiftModeParameter.isValid
    readonly property bool autoActive: proxy && shiftActive

    function enumText(value, enumName) {
        var text = EnumHelper.enumToString(value, enumName)
        return text && text !== "Unknown" ? text : qsTr("N/A")
    }

    function selectShiftMode(mode) {
        if (root.shiftActive)
            root.shiftModeParameter.value = mode
    }

    function toggleAutoProfile() {
        if (root.autoActive)
            root.proxy.setAutoProfileEnabled(!root.proxy.autoProfileEnabled)
    }

    Flow {
        Layout.fillWidth: true
        spacing: 8

        Repeater {
            model: root.proxy ? root.proxy.behaviorProfiles : []

            Rectangle {
                width: Math.max(92, profileLabel.implicitWidth + 24)
                height: 32
                radius: 8
                color: selected ? root.accentColor : "transparent"
                border.color: selected ? root.accentColor : root.borderColor
                border.width: 1

                property bool selected: root.proxy && root.proxy.activeBehaviorProfile === modelData.id

                Label {
                    id: profileLabel
                    anchors.centerIn: parent
                    text: modelData.title
                    color: parent.selected ? "#ffffff" : root.textColor
                    font.pixelSize: 12
                    font.bold: parent.selected
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.proxy.applyBehaviorProfile(modelData.id)
                }
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: root.borderColor
        opacity: 0.72
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 12

        Label {
            Layout.preferredWidth: 48
            text: qsTr("Shift")
            color: root.mutedTextColor
            font.pixelSize: 11
            font.bold: true
            elide: Text.ElideRight
        }

        Flow {
            Layout.fillWidth: true
            spacing: 7

            Repeater {
                model: root.shiftActive && root.shiftModeParameter.availableValues ? root.shiftModeParameter.availableValues : []

                Rectangle {
                    width: Math.max(76, shiftLabel.implicitWidth + 22)
                    height: 30
                    radius: 8
                    color: selected ? root.accentColor : root.elevatedColor
                    border.color: selected ? root.accentColor : root.borderColor
                    border.width: 1

                    property bool selected: root.shiftModeParameter.value === modelData

                    Label {
                        id: shiftLabel
                        anchors.centerIn: parent
                        text: root.enumText(modelData, "ShiftMode")
                        color: parent.selected ? "#ffffff" : root.textColor
                        font.pixelSize: 11
                        font.bold: parent.selected
                    }

                    MouseArea {
                        anchors.fill: parent
                        enabled: root.shiftActive
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.selectShiftMode(modelData)
                    }
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 12

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Label {
                Layout.fillWidth: true
                text: qsTr("Auto profile")
                color: root.textColor
                font.pixelSize: 12
                font.bold: true
                elide: Text.ElideRight
            }

            Label {
                Layout.fillWidth: true
                text: root.proxy && root.proxy.autoProfileEnabled
                      ? root.proxy.autoProfileStatus
                      : (root.proxy ? root.proxy.behaviorProfileStatus : qsTr("Waiting for service"))
                color: root.mutedTextColor
                font.pixelSize: 11
                elide: Text.ElideRight
            }
        }

        Rectangle {
            Layout.preferredWidth: 38
            Layout.preferredHeight: 20
            radius: 10
            color: root.proxy && root.proxy.autoProfileEnabled ? root.accentColor : root.elevatedColor
            border.color: root.proxy && root.proxy.autoProfileEnabled ? root.accentColor : root.borderColor
            opacity: root.autoActive ? 1.0 : 0.55

            Rectangle {
                width: 16
                height: 16
                radius: 8
                anchors.verticalCenter: parent.verticalCenter
                x: root.proxy && root.proxy.autoProfileEnabled ? parent.width - width - 2 : 2
                color: root.proxy && root.proxy.autoProfileEnabled ? "#ffffff" : root.mutedTextColor
            }

            MouseArea {
                anchors.fill: parent
                enabled: root.autoActive
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.toggleAutoProfile()
            }
        }
    }
}
