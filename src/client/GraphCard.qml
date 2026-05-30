import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models

Pane {
    id: root

    property int maxHistoryLength: 60
    property bool showLegend: true
    property string title: ""
    property string subtitle: ""
    property real min: 0
    property real max: 100
    property color surfaceColor: "#20242d"
    property color borderColor: "#303642"
    property color textColor: "#f3f6fb"
    property color mutedTextColor: "#9aa6b6"
    property color gridColor: "#323946"
    property color accentColor: "#3fa7ff"

    ObjectModel { id: sensorsContainer }
    default property alias sensors: sensorsContainer.children

    QtObject {
        id: internal
        property var histories: ({})
    }

    padding: 0
    implicitWidth: 360
    implicitHeight: 210

    background: Rectangle {
        color: root.surfaceColor
        radius: 8
        border.color: root.borderColor
        border.width: 1
    }

    Component.onCompleted: appendValues()
    onVisibleChanged: if (visible) chartCanvas.requestPaint()
    onMinChanged: chartCanvas.requestPaint()
    onMaxChanged: chartCanvas.requestPaint()
    onGridColorChanged: chartCanvas.requestPaint()
    onAccentColorChanged: chartCanvas.requestPaint()

    function sensorEnabled(sensor) {
        return sensor && sensor.isSensor && sensor.visible !== false
    }

    function sensorKey(sensor, index) {
        return sensor.name && sensor.name.length > 0 ? sensor.name : "sensor-" + index
    }

    function normalizedValue(value) {
        var span = Math.max(1, root.max - root.min)
        var normalized = (Number(value || 0) - root.min) / span
        return Math.max(0, Math.min(1, normalized))
    }

    function appendValues() {
        var histories = internal.histories
        for (var i = 0; i < sensorsContainer.children.length; ++i) {
            var sensor = sensorsContainer.children[i]
            if (!sensorEnabled(sensor))
                continue

            var key = sensorKey(sensor, i)
            var values = histories[key] || []
            values.push(Number(sensor.value || 0))
            while (values.length > root.maxHistoryLength)
                values.shift()
            histories[key] = values
        }
        internal.histories = histories
        chartCanvas.requestPaint()
    }

    Timer {
        interval: 1500
        running: root.visible
        repeat: true
        onTriggered: root.appendValues()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: root.title
                    color: root.textColor
                    font.pixelSize: 15
                    font.bold: true
                    elide: Text.ElideRight
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

            Flow {
                Layout.preferredWidth: 190
                spacing: 8
                visible: root.showLegend

                Repeater {
                    model: sensorsContainer.children

                    delegate: RowLayout {
                        visible: root.sensorEnabled(modelData)
                        width: 86
                        height: 18
                        spacing: 5

                        Rectangle {
                            Layout.preferredWidth: 8
                            Layout.preferredHeight: 8
                            radius: 4
                            color: modelData.color
                        }

                        Label {
                            Layout.preferredWidth: 70
                            text: modelData.name + " " + Number(modelData.value || 0).toFixed(0) + modelData.unit
                            color: root.textColor
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }

        Canvas {
            id: chartCanvas
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 130
            antialiasing: true

            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            onPaint: {
                var ctx = getContext("2d")
                ctx.setTransform(1, 0, 0, 1, 0, 0)
                ctx.globalAlpha = 1.0
                ctx.clearRect(0, 0, width, height)

                var left = 0
                var top = 10
                var right = width
                var bottom = height - 8
                var plotWidth = Math.max(1, right - left)
                var plotHeight = Math.max(1, bottom - top)

                ctx.lineWidth = 1
                ctx.strokeStyle = root.gridColor
                for (var gy = 0; gy < 4; ++gy) {
                    var y = top + plotHeight * gy / 3
                    ctx.beginPath()
                    ctx.moveTo(left, y)
                    ctx.lineTo(right, y)
                    ctx.stroke()
                }

                for (var i = 0; i < sensorsContainer.children.length; ++i) {
                    var sensor = sensorsContainer.children[i]
                    if (!root.sensorEnabled(sensor))
                        continue

                    var key = root.sensorKey(sensor, i)
                    var values = internal.histories[key] || [Number(sensor.value || 0)]
                    if (values.length === 0)
                        continue

                    ctx.beginPath()
                    ctx.moveTo(left, bottom)
                    for (var p = 0; p < values.length; ++p) {
                        var x = left + (root.maxHistoryLength <= 1 ? 0 : plotWidth * p / (root.maxHistoryLength - 1))
                        var py = bottom - root.normalizedValue(values[p]) * plotHeight
                        ctx.lineTo(x, py)
                    }
                    var lastX = left + (root.maxHistoryLength <= 1 ? 0 : plotWidth * (values.length - 1) / (root.maxHistoryLength - 1))
                    ctx.lineTo(lastX, bottom)
                    ctx.closePath()
                    ctx.fillStyle = Qt.rgba(sensor.color.r, sensor.color.g, sensor.color.b, 0.20)
                    ctx.fill()

                    ctx.beginPath()
                    for (var lp = 0; lp < values.length; ++lp) {
                        var lineX = left + (root.maxHistoryLength <= 1 ? 0 : plotWidth * lp / (root.maxHistoryLength - 1))
                        var lineY = bottom - root.normalizedValue(values[lp]) * plotHeight
                        if (lp === 0)
                            ctx.moveTo(lineX, lineY)
                        else
                            ctx.lineTo(lineX, lineY)
                    }
                    ctx.lineWidth = 2
                    ctx.lineJoin = "round"
                    ctx.lineCap = "round"
                    ctx.strokeStyle = sensor.color
                    ctx.stroke()
                }
            }
        }
    }
}
