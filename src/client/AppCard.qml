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
    readonly property int contentMargin: 14
    readonly property int contentSpacing: 10
    default property alias content: body.data

    padding: 0
    implicitWidth: 280
    implicitHeight: Math.max(header.visible ? 154 : 124,
                             contentMargin * 2 + body.implicitHeight +
                             (header.visible ? header.implicitHeight + contentSpacing : 0))

    background: Rectangle {
        color: root.surfaceColor
        radius: root.cardRadius
        border.color: root.borderColor
        border.width: 1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.contentMargin
        spacing: root.contentSpacing

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
