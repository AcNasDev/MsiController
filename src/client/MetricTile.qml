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
    onChartValueChanged: chartCanvas.requestPaint()
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
        interval: 1000
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

            Canvas {
                id: chartCanvas
                Layout.fillWidth: true
                Layout.preferredHeight: root.chartEnabled ? 20 : 0
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

                    var bottom = height - 2
                    var top = 2
                    var plotHeight = Math.max(1, bottom - top)
                    var plotWidth = Math.max(1, width)

                    ctx.strokeStyle = root.borderColor
                    ctx.globalAlpha = 0.55
                    ctx.lineWidth = 1
                    ctx.beginPath()
                    ctx.moveTo(0, bottom)
                    ctx.lineTo(width, bottom)
                    ctx.stroke()
                    ctx.globalAlpha = 1.0

                    ctx.beginPath()
                    ctx.moveTo(0, bottom)
                    for (var i = 0; i < values.length; ++i) {
                        var x = root.maxHistoryLength <= 1 ? 0 : plotWidth * i / (root.maxHistoryLength - 1)
                        var y = bottom - root.normalizedChartValue(values[i]) * plotHeight
                        ctx.lineTo(x, y)
                    }
                    var lastX = root.maxHistoryLength <= 1 ? 0 : plotWidth * (values.length - 1) / (root.maxHistoryLength - 1)
                    ctx.lineTo(lastX, bottom)
                    ctx.closePath()
                    ctx.fillStyle = Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.18)
                    ctx.fill()

                    ctx.beginPath()
                    for (var p = 0; p < values.length; ++p) {
                        var lineX = root.maxHistoryLength <= 1 ? 0 : plotWidth * p / (root.maxHistoryLength - 1)
                        var lineY = bottom - root.normalizedChartValue(values[p]) * plotHeight
                        if (p === 0)
                            ctx.moveTo(lineX, lineY)
                        else
                            ctx.lineTo(lineX, lineY)
                    }
                    ctx.lineWidth = 2
                    ctx.lineJoin = "round"
                    ctx.lineCap = "round"
                    ctx.strokeStyle = root.accentColor
                    ctx.stroke()
                }
            }
        }
    }
}
