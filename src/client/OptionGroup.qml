import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MSI.Helpers 1.0

AppCard {
    id: root

    property var parameter
    property string enumName: ""
    property color accentColor: "#3fa7ff"
    readonly property bool active: parameter && parameter.isValid

    opacity: active ? 1.0 : 0.55

    Flow {
        Layout.fillWidth: true
        spacing: 8

        Repeater {
            model: root.active && root.parameter.availableValues ? root.parameter.availableValues : []

            delegate: Rectangle {
                width: Math.max(86, optionLabel.implicitWidth + 26)
                height: 34
                radius: 8
                color: selected ? root.accentColor : "transparent"
                border.color: selected ? root.accentColor : root.borderColor
                border.width: 1

                property bool selected: root.parameter.value === modelData

                Label {
                    id: optionLabel
                    anchors.centerIn: parent
                    text: EnumHelper.enumToString(modelData, root.enumName) || "N/A"
                    color: parent.selected ? "#ffffff" : root.textColor
                    font.pixelSize: 12
                    font.bold: parent.selected
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: root.active
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.parameter.value = modelData
                }
            }
        }
    }
}
