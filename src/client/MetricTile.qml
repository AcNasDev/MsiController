import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MsiController 1.0

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
    property bool chartEnabled: false
    property real chartValue: 0
    property real chartMin: 0
    property real chartMax: 100
    property int maxHistoryLength: 48

    QtObject {
        id: internal
        property var history: []
    }

    padding: 0
    clip: true
    implicitWidth: 240
    implicitHeight: chartEnabled ? 116 : 96

    Component.onCompleted: appendChartValue()

    function appendChartValue() {
        if (!root.chartEnabled)
            return

        var values = internal.history.slice()
        values.push(Number(root.chartValue || 0))
        while (values.length > root.maxHistoryLength)
            values.shift()
        internal.history = values
    }

    Timer {
        interval: 1500
        running: root.visible && root.chartEnabled
        repeat: true
        onTriggered: root.appendChartValue()
    }

    background: Rectangle {
        color: root.surfaceColor
        radius: 8
        border.color: root.borderColor
        border.width: 1
    }

    GpuLineChart {
        anchors.fill: parent
        anchors.margins: 8
        visible: root.chartEnabled
        values: internal.history
        minValue: root.chartMin
        maxValue: root.chartMax
        sampleCapacity: root.maxHistoryLength
        leftPadding: 12
        rightPadding: 4
        topPadding: Math.max(26, height * 0.28)
        bottomPadding: 5
        gridRows: 1
        gridColumns: 0
        showGrid: true
        lineWidth: 2
        lineColor: root.accentColor
        fillColor: Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.16)
        gridColor: Qt.rgba(root.borderColor.r, root.borderColor.g, root.borderColor.b, 0.55)
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        Rectangle {
            Layout.preferredWidth: 4
            Layout.fillHeight: true
            radius: 3
            color: root.accentColor
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 3

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
                    font.pixelSize: 24
                    font.bold: true
                    elide: Text.ElideRight
                }

                Label {
                    text: root.unit
                    color: root.mutedTextColor
                    font.pixelSize: 12
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
