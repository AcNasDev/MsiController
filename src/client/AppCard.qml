import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: root

    property string title: ""
    property string subtitle: ""
    property color surfaceColor: "#20242d"
    property color borderColor: "#303642"
    property color textColor: "#f3f6fb"
    property color mutedTextColor: "#9aa6b6"
    property int cardRadius: 8
    default property alias content: body.data

    padding: 0
    implicitWidth: 280
    implicitHeight: header.visible ? 154 : 124

    background: Rectangle {
        color: root.surfaceColor
        radius: root.cardRadius
        border.color: root.borderColor
        border.width: 1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        ColumnLayout {
            id: header
            Layout.fillWidth: true
            spacing: 2
            visible: root.title.length > 0 || root.subtitle.length > 0

            Label {
                Layout.fillWidth: true
                text: root.title
                color: root.textColor
                font.pixelSize: 15
                font.bold: true
                elide: Text.ElideRight
                visible: root.title.length > 0
            }

            Label {
                Layout.fillWidth: true
                text: root.subtitle
                color: root.mutedTextColor
                font.pixelSize: 12
                elide: Text.ElideRight
                visible: root.subtitle.length > 0
            }
        }

        ColumnLayout {
            id: body
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10
        }
    }
}
