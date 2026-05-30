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
    onChartMinChanged: chartCanvas.requestPaint()
    onChartMaxChanged: chartCanvas.requestPaint()
    onAccentColorChanged: chartCanvas.requestPaint()

    function normalizedChartValue(value) {
        var span = Math.max(1, root.chartMax - root.chartMin)
        return Math.max(0, Math.min(1, (Number(value || 0) - root.chartMin) / span))
    }

    function appendChartValue() {
        if (!root.chartEnabled)
            return

        var values = internal.history.slice()
        values.push(Number(root.chartValue || 0))
        while (values.length > root.maxHistoryLength)
            values.shift()
        internal.history = values
        chartCanvas.requestPaint()
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

    Canvas {
        id: chartCanvas
        anchors.fill: parent
        anchors.margins: 8
        visible: root.chartEnabled
        antialiasing: true

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        onPaint: {
            var ctx = getContext("2d")
            ctx.setTransform(1, 0, 0, 1, 0, 0)
            ctx.clearRect(0, 0, width, height)

            var values = internal.history
            if (!values || values.length === 0)
                return

            var left = 12
            var right = width - 4
            var top = Math.max(26, height * 0.28)
            var bottom = height - 5
            var plotHeight = Math.max(1, bottom - top)
            var plotWidth = Math.max(1, right - left)

            ctx.strokeStyle = Qt.rgba(root.borderColor.r, root.borderColor.g, root.borderColor.b, 0.55)
            ctx.lineWidth = 1
            ctx.beginPath()
            ctx.moveTo(left, bottom)
            ctx.lineTo(right, bottom)
            ctx.stroke()

            ctx.beginPath()
            ctx.moveTo(left, bottom)
            var lastX = left
            var lastY = bottom
            for (var i = 0; i < values.length; ++i) {
                var x = left + (root.maxHistoryLength <= 1 ? 0 : plotWidth * i / (root.maxHistoryLength - 1))
                var y = bottom - root.normalizedChartValue(values[i]) * plotHeight
                ctx.lineTo(x, y)
                lastX = x
                lastY = y
            }
            ctx.lineTo(lastX, bottom)
            ctx.closePath()
            ctx.fillStyle = Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.16)
            ctx.fill()

            ctx.beginPath()
            for (var p = 0; p < values.length; ++p) {
                var lineX = left + (root.maxHistoryLength <= 1 ? 0 : plotWidth * p / (root.maxHistoryLength - 1))
                var lineY = bottom - root.normalizedChartValue(values[p]) * plotHeight
                if (p === 0)
                    ctx.moveTo(lineX, lineY)
                else
                    ctx.lineTo(lineX, lineY)
                lastX = lineX
                lastY = lineY
            }
            ctx.lineWidth = 2
            ctx.lineJoin = "round"
            ctx.lineCap = "round"
            ctx.strokeStyle = root.accentColor
            ctx.stroke()
        }
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
