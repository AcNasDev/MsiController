import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MsiController 1.0

Pane {
    id: root

    property string title: ""
    property string subtitle: qsTr("Drag points to tune the curve")
    property string unit: "%"
    property string tempUnit: "°C"
    property real minTemp: 0
    property real maxTemp: 100
    property real minSpeed: 0
    property real maxSpeed: 150
    property color surfaceColor: "#20242d"
    property color borderColor: "#303642"
    property color textColor: "#f3f6fb"
    property color mutedTextColor: "#9aa6b6"
    property color gridColor: "#323946"
    property color accentColor: "#3fa7ff"
    property color fillColor: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.18)
    signal curveChanged()
    signal pointEdited(int pointIndex, real temperature, real speed)

    property real temp1: 0
    property real speed1: 30
    property real temp2: 15
    property real speed2: 40
    property real temp3: 30
    property real speed3: 50
    property real temp4: 45
    property real speed4: 60
    property real temp5: 60
    property real speed5: 70
    property real temp6: 75
    property real speed6: 80
    property real temp7: 90
    property real speed7: 100

    property int dragIndex: -1
    property bool isDragging: false
    property string hoverText: ""
    property point hoverPoint: Qt.point(0, 0)
    property var draftTemps: []
    property var draftSpeeds: []

    padding: 0
    implicitHeight: 260

    background: Rectangle {
        color: root.surfaceColor
        radius: 8
        border.color: root.borderColor
        border.width: 1
    }

    onTemp1Changed: syncDraft(false)
    onSpeed1Changed: syncDraft(false)
    onTemp2Changed: syncDraft(false)
    onSpeed2Changed: syncDraft(false)
    onTemp3Changed: syncDraft(false)
    onSpeed3Changed: syncDraft(false)
    onTemp4Changed: syncDraft(false)
    onSpeed4Changed: syncDraft(false)
    onTemp5Changed: syncDraft(false)
    onSpeed5Changed: syncDraft(false)
    onTemp6Changed: syncDraft(false)
    onSpeed6Changed: syncDraft(false)
    onTemp7Changed: syncDraft(false)
    onSpeed7Changed: syncDraft(false)

    Component.onCompleted: syncDraft(true)

    function sourceTemps() {
        return normalizeTemps([temp1, temp2, temp3, temp4, temp5, temp6, temp7])
    }

    function sourceSpeeds() {
        return normalizeSpeeds([speed1, speed2, speed3, speed4, speed5, speed6, speed7])
    }

    function syncDraft(force) {
        if (root.isDragging && !force)
            return

        root.draftTemps = sourceTemps()
        root.draftSpeeds = sourceSpeeds()
    }

    function pointTemp(index) {
        return root.draftTemps.length > index ? root.draftTemps[index] : sourceTemps()[index]
    }

    function pointSpeed(index) {
        return root.draftSpeeds.length > index ? root.draftSpeeds[index] : sourceSpeeds()[index]
    }

    function boundedNumber(value, minValue, maxValue, fallback) {
        var n = Number(value)
        if (isNaN(n))
            n = fallback
        return Math.max(minValue, Math.min(maxValue, n))
    }

    function normalizeTemps(values) {
        var result = []
        var prev = root.minTemp
        for (var i = 0; i < 7; ++i) {
            var fallback = i === 0 ? root.minTemp : prev
            var value = boundedNumber(values[i], root.minTemp, root.maxTemp, fallback)
            if (i === 0)
                value = root.minTemp
            else
                value = Math.max(prev, value)
            result.push(value)
            prev = value
        }
        return result
    }

    function normalizeSpeeds(values) {
        var result = []
        for (var i = 0; i < 7; ++i)
            result.push(boundedNumber(values[i], root.minSpeed, root.maxSpeed, root.minSpeed))
        return result
    }

    function setPoint(index, temperature, speed) {
        var temps = root.draftTemps.length === 7 ? root.draftTemps.slice() : sourceTemps()
        var speeds = root.draftSpeeds.length === 7 ? root.draftSpeeds.slice() : sourceSpeeds()
        temps[index] = index === 0 ? root.minTemp : temperature
        speeds[index] = speed
        root.draftTemps = temps
        root.draftSpeeds = speeds
    }

    function movePoint(index, item, mouseX, mouseY) {
        var localPoint = item.mapToItem(chartArea, mouseX, mouseY)
        var newTemp = Math.round(chartArea.xToTemp(localPoint.x))
        var newSpeed = Math.round(chartArea.yToSpeed(localPoint.y))

        if (index === 0)
            newTemp = root.minTemp
        var minAllowedTemp = index > 0 ? root.pointTemp(index - 1) : root.minTemp
        var maxAllowedTemp = index < 6 ? root.pointTemp(index + 1) : root.maxTemp
        if (maxAllowedTemp - minAllowedTemp > 1) {
            minAllowedTemp += index > 0 ? 1 : 0
            maxAllowedTemp -= index < 6 ? 1 : 0
        }
        if (maxAllowedTemp < minAllowedTemp)
            maxAllowedTemp = minAllowedTemp
        newTemp = Math.max(minAllowedTemp, Math.min(maxAllowedTemp, newTemp))
        newTemp = Math.max(root.minTemp, Math.min(root.maxTemp, newTemp))
        newSpeed = Math.max(root.minSpeed, Math.min(root.maxSpeed, newSpeed))

        root.setPoint(index, newTemp, newSpeed)
        root.hoverPoint = localPoint
        root.hoverText = newSpeed + root.unit + " / " + newTemp + root.tempUnit
        root.curveChanged()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

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

            Rectangle {
                Layout.preferredWidth: valueLabel.implicitWidth + 22
                Layout.preferredHeight: 32
                radius: 8
                color: Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.14)
                border.color: Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.45)

                Label {
                    id: valueLabel
                    anchors.centerIn: parent
                    text: Math.round(pointSpeed(6)) + root.unit
                    color: root.accentColor
                    font.pixelSize: 13
                    font.bold: true
                }
            }
        }

        Rectangle {
            id: chartArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 170
            color: "transparent"
            clip: true

            property real leftPadding: 14
            property real rightPadding: 14
            property real topPadding: 12
            property real bottomPadding: 24
            readonly property real plotX: leftPadding
            readonly property real plotY: topPadding
            readonly property real plotWidth: Math.max(1, width - leftPadding - rightPadding)
            readonly property real plotHeight: Math.max(1, height - topPadding - bottomPadding)

            function tempSpan() {
                return Math.max(1, root.maxTemp - root.minTemp)
            }

            function speedSpan() {
                return Math.max(1, root.maxSpeed - root.minSpeed)
            }

            function tempToX(value) {
                return plotX + (Math.max(root.minTemp, Math.min(root.maxTemp, value)) - root.minTemp) / tempSpan() * plotWidth
            }

            function speedToY(value) {
                return plotY + (1 - (Math.max(root.minSpeed, Math.min(root.maxSpeed, value)) - root.minSpeed) / speedSpan()) * plotHeight
            }

            function xToTemp(value) {
                return root.minTemp + Math.max(0, Math.min(plotWidth, value - plotX)) / plotWidth * tempSpan()
            }

            function yToSpeed(value) {
                return root.minSpeed + (1 - Math.max(0, Math.min(plotHeight, value - plotY)) / plotHeight) * speedSpan()
            }

            GpuLineChart {
                anchors.fill: parent
                values: root.draftSpeeds
                xValues: root.draftTemps
                minValue: root.minSpeed
                maxValue: root.maxSpeed
                minX: root.minTemp
                maxX: root.maxTemp
                leftPadding: chartArea.leftPadding
                rightPadding: chartArea.rightPadding
                topPadding: chartArea.topPadding
                bottomPadding: chartArea.bottomPadding
                gridRows: 5
                gridColumns: 6
                lineWidth: 3
                showGrid: true
                extendLastToMaxX: true
                lineColor: root.accentColor
                fillColor: root.fillColor
                gridColor: root.gridColor
            }

            Label {
                x: chartArea.plotX + 2
                y: chartArea.plotY + 3
                text: Math.round(root.maxSpeed) + root.unit
                color: root.mutedTextColor
                font.pixelSize: 11
            }

            Label {
                x: chartArea.plotX + 2
                y: chartArea.plotY + chartArea.plotHeight - height - 2
                text: Math.round(root.minSpeed) + root.unit
                color: root.mutedTextColor
                font.pixelSize: 11
            }

            Label {
                x: chartArea.plotX + 2
                y: chartArea.height - height - 2
                text: Math.round(root.minTemp) + root.tempUnit
                color: root.mutedTextColor
                font.pixelSize: 11
            }

            Label {
                x: chartArea.plotX + chartArea.plotWidth - width - 2
                y: chartArea.height - height - 2
                text: Math.round(root.maxTemp) + root.tempUnit
                color: root.mutedTextColor
                font.pixelSize: 11
            }

            Label {
                visible: root.hoverText.length > 0
                text: root.hoverText
                x: Math.max(0, Math.min(root.hoverPoint.x - width / 2, chartArea.width - width))
                y: Math.max(0, root.hoverPoint.y - height - 10)
                z: 4
                color: root.textColor
                font.pixelSize: 12
                padding: 7
                background: Rectangle {
                    color: "#10141c"
                    radius: 6
                    border.color: root.borderColor
                }
            }

            Repeater {
                model: 7

                Rectangle {
                    id: pointHandle
                    z: 10
                    width: 22
                    height: 22
                    radius: 11
                    color: root.surfaceColor
                    border.color: root.accentColor
                    border.width: 3

                    property point chartPoint: Qt.point(chartArea.tempToX(root.pointTemp(index)), chartArea.speedToY(root.pointSpeed(index)))
                    x: chartPoint.x - width / 2
                    y: chartPoint.y - height / 2

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        hoverEnabled: true
                        preventStealing: true
                        cursorShape: Qt.PointingHandCursor

                        onEntered: pointHandle.color = Qt.lighter(root.surfaceColor, 1.25)
                        onExited: {
                            if (!pressed)
                                pointHandle.color = root.surfaceColor
                        }
                        onPressed: function(mouse) {
                            mouse.accepted = true
                            root.dragIndex = index
                            root.isDragging = true
                            root.movePoint(index, pointHandle, mouse.x, mouse.y)
                        }
                        onPositionChanged: function(mouse) {
                            mouse.accepted = true
                            if (pressed)
                                root.movePoint(index, pointHandle, mouse.x, mouse.y)
                        }
                        onReleased: {
                            root.pointEdited(index, root.pointTemp(index), root.pointSpeed(index))
                            root.isDragging = false
                            root.dragIndex = -1
                            root.hoverText = ""
                            pointHandle.color = root.surfaceColor
                        }
                        onCanceled: {
                            root.isDragging = false
                            root.dragIndex = -1
                            root.hoverText = ""
                            pointHandle.color = root.surfaceColor
                        }
                    }
                }
            }
        }
    }
}
