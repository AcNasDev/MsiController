import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: root

    property string title: ""
    property string subtitle: ""
    property bool checked: false
    property bool active: true
    property color surfaceColor: "#20242d"
    property color borderColor: "#303642"
    property color textColor: "#f3f6fb"
    property color mutedTextColor: "#9aa6b6"
    property color accentColor: "#3fa7ff"
    signal toggled(bool checked)

    padding: 0
    implicitHeight: 66
    enabled: active
    opacity: active ? 1.0 : 0.55

    background: Rectangle {
        color: root.surfaceColor
        radius: 8
        border.color: root.checked ? root.accentColor : root.borderColor
        border.width: root.checked ? 1.5 : 1
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Label {
                Layout.fillWidth: true
                text: root.title
                color: root.textColor
                font.pixelSize: 13
                font.bold: true
                elide: Text.ElideRight
            }

            Label {
                Layout.fillWidth: true
                text: root.subtitle
                color: root.mutedTextColor
                font.pixelSize: 11
                elide: Text.ElideRight
                visible: root.subtitle.length > 0
            }
        }

        Switch {
            checked: root.checked
            enabled: root.active
            onToggled: root.toggled(checked)
        }
    }
}
