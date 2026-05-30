import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models
import MsiController 1.0

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

    function sensorEnabled(sensor) {
        return sensor && sensor.isSensor && sensor.visible !== false
    }

    function sensorKey(sensor, index) {
        return sensor.name && sensor.name.length > 0 ? sensor.name : "sensor-" + index
    }

    function historyFor(sensor, index) {
        var key = root.sensorKey(sensor, index)
        return internal.histories[key] || [Number(sensor && sensor.value || 0)]
    }

    function appendValues() {
        var histories = ({})
        for (var oldKey in internal.histories)
            histories[oldKey] = internal.histories[oldKey]

        for (var i = 0; i < sensorsContainer.children.length; ++i) {
            var sensor = sensorsContainer.children[i]
            if (!sensorEnabled(sensor))
                continue

            var key = sensorKey(sensor, i)
            var values = histories[key] ? histories[key].slice() : []
            values.push(Number(sensor.value || 0))
            while (values.length > root.maxHistoryLength)
                values.shift()
            histories[key] = values
        }
        internal.histories = histories
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

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 130

            GpuLineChart {
                anchors.fill: parent
                values: []
                minValue: root.min
                maxValue: root.max
                topPadding: 10
                bottomPadding: 8
                gridRows: 4
                gridColumns: 0
                showGrid: true
                fillVisible: false
                lineColor: "transparent"
                gridColor: root.gridColor
            }

            Repeater {
                model: sensorsContainer.children

                GpuLineChart {
                    anchors.fill: parent
                    visible: root.sensorEnabled(modelData)
                    values: root.historyFor(modelData, index)
                    minValue: root.min
                    maxValue: root.max
                    sampleCapacity: root.maxHistoryLength
                    topPadding: 10
                    bottomPadding: 8
                    showGrid: false
                    lineWidth: 2
                    lineColor: modelData.color
                    fillColor: Qt.rgba(modelData.color.r, modelData.color.g, modelData.color.b, 0.20)
                }
            }
        }
    }
}
