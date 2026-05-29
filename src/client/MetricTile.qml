import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: root

    property string title: ""
    property string value: "N/A"
    property string unit: ""
    property string detail: ""
    property color surfaceColor: "#20242d"
    property color borderColor: "#303642"
    property color textColor: "#f3f6fb"
    property color mutedTextColor: "#9aa6b6"
    property color accentColor: "#3fa7ff"

    padding: 0
    implicitWidth: 240
    implicitHeight: 96

    background: Rectangle {
        color: root.surfaceColor
        radius: 8
        border.color: root.borderColor
        border.width: 1
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        Rectangle {
            Layout.preferredWidth: 5
            Layout.fillHeight: true
            radius: 3
            color: root.accentColor
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 4

            Label {
                Layout.fillWidth: true
                text: root.title
                color: root.mutedTextColor
                font.pixelSize: 12
                elide: Text.ElideRight
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                Label {
                    text: root.value
                    color: root.textColor
                    font.pixelSize: 26
                    font.bold: true
                    elide: Text.ElideRight
                }

                Label {
                    text: root.unit
                    color: root.mutedTextColor
                    font.pixelSize: 13
                    Layout.alignment: Qt.AlignBottom
                    visible: root.unit.length > 0
                }
            }

            Label {
                Layout.fillWidth: true
                text: root.detail
                color: root.mutedTextColor
                font.pixelSize: 12
                elide: Text.ElideRight
                visible: root.detail.length > 0
            }
        }
    }
}
