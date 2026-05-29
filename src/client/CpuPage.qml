import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MsiController 1.0
import Msi 1.0

Flickable {
    id: root

    property var proxy
    property color surfaceColor: "#20242d"
    property color elevatedColor: "#252b36"
    property color borderColor: "#303642"
    property color textColor: "#f3f6fb"
    property color mutedTextColor: "#9aa6b6"
    property color accentColor: "#3fa7ff"
    property color secondaryAccentColor: "#6bd0c4"

    property var cpuParameter: proxy ? proxy.getProxyParameter(Msi.Parametr.CpuConfig) : null
    property var cpuConfig: cpuParameter ? cpuParameter.value : null
    property var valueFregs: []
    property var valueUsage: []
    property var prevFregs: []
    property var prevUsage: []
    property var rawFregs: []
    property var rawUsage: []
    property var prevMaxFreqScaling: []
    property bool editingCpuLimit: false

    clip: true
    contentWidth: width
    contentHeight: contentColumn.implicitHeight
    boundsBehavior: Flickable.StopAtBounds

    function kalmanFilter(prev, measurement, estimateError, measurementError) {
        var gain = estimateError / (estimateError + measurementError)
        return prev + gain * (measurement - prev)
    }

    function arraysEqual(a, b) {
        if (a === b)
            return true
        if (!a || !b || a.length !== b.length)
            return false
        for (var i = 0; i < a.length; ++i) {
            if (a[i] !== b[i])
                return false
        }
        return true
    }

    function normalized(value, idx) {
        if (!cpuConfig || !cpuConfig.cpus || !cpuConfig.cpus[idx])
            return 0
        var core = cpuConfig.cpus[idx]
        var span = core.maxFreq - core.minFreq
        return span > 0 ? (value - core.minFreq) / span : 0
    }

    function denormalized(value, idx) {
        if (!cpuConfig || !cpuConfig.cpus || !cpuConfig.cpus[idx])
            return 0
        var core = cpuConfig.cpus[idx]
        return value * (core.maxFreq - core.minFreq) + core.minFreq
    }

    function clampFreq(value, idx) {
        if (!cpuConfig || !cpuConfig.cpus || !cpuConfig.cpus[idx])
            return value
        var core = cpuConfig.cpus[idx]
        return Math.max(core.minFreq, Math.min(core.maxFreq, value))
    }

    function draftLimitValue(idx) {
        if (root.prevMaxFreqScaling.length > idx)
            return root.prevMaxFreqScaling[idx]
        if (root.cpuConfig && root.cpuConfig.cpus && root.cpuConfig.cpus[idx])
            return root.cpuConfig.cpus[idx].scalingMaxFreq
        return 0
    }

    function updateDraftLimit(idx, value) {
        if (!root.cpuConfig || !root.cpuConfig.cpus || !root.cpuConfig.cpus[idx])
            return

        var values = root.prevMaxFreqScaling.length === root.cpuConfig.cpus.length
            ? root.prevMaxFreqScaling.slice()
            : Array(root.cpuConfig.cpus.length).fill(0)
        values[idx] = Math.round(clampFreq(value, idx))
        root.prevMaxFreqScaling = values
    }

    function updateAllDraftLimits(value) {
        if (!root.cpuConfig || !root.cpuConfig.cpus)
            return

        var values = []
        for (var i = 0; i < root.cpuConfig.cpus.length; ++i)
            values.push(Math.round(clampFreq(value, i)))
        root.prevMaxFreqScaling = values
    }

    function commitDraftLimits() {
        if (!root.cpuConfig || !root.cpuConfig.cpus || root.prevMaxFreqScaling.length === 0)
            return

        var config = root.cpuConfig
        for (var i = 0; i < config.cpus.length && i < root.prevMaxFreqScaling.length; ++i)
            config.cpus[i].scalingMaxFreq = Math.round(clampFreq(root.prevMaxFreqScaling[i], i))
        root.setCpuConfig(config)
    }

    function displayedGlobalFreqGhz() {
        if (root.prevMaxFreqScaling.length > 0)
            return root.prevMaxFreqScaling[0] / 1000000.0
        if (root.cpuConfig && root.cpuConfig.cpus && root.cpuConfig.cpus.length > 0)
            return root.cpuConfig.cpus[0].scalingMaxFreq / 1000000.0
        return 0
    }

    function setCpuConfig(config) {
        if (cpuParameter)
            cpuParameter.value = config
    }

    onCpuConfigChanged: {
        if (!cpuConfig || !cpuConfig.cpus)
            return

        if (root.rawFregs.length !== cpuConfig.cpus.length || root.rawUsage.length !== cpuConfig.cpus.length) {
            root.rawFregs = Array(cpuConfig.cpus.length).fill(0)
            root.rawUsage = Array(cpuConfig.cpus.length).fill(0)
        }

        var curMaxFreqScaling = Array(cpuConfig.cpus.length).fill(0)
        for (var i = 0; i < cpuConfig.cpus.length; i++) {
            var core = cpuConfig.cpus[i]
            var span = core.maxFreq - core.minFreq
            root.rawFregs[i] = span > 0 ? (core.currentFreq - core.minFreq) / span : 0
            root.rawUsage[i] = core.usage / 100.0
            curMaxFreqScaling[i] = core.scalingMaxFreq
        }

        if (!root.editingCpuLimit && !arraysEqual(curMaxFreqScaling, prevMaxFreqScaling))
            prevMaxFreqScaling = curMaxFreqScaling
        if (prevFregs.length !== rawFregs.length)
            prevFregs = Array(rawFregs.length).fill(0)
        if (prevUsage.length !== rawUsage.length)
            prevUsage = Array(rawUsage.length).fill(0)
    }

    Timer {
        interval: 66
        running: root.visible
        repeat: true
        onTriggered: {
            var estimateError = 0.003 * interval
            var measurementError = 10.0
            for (var i = 0; i < root.rawFregs.length; i++) {
                var filteredFreq = root.kalmanFilter(root.prevFregs[i], root.rawFregs[i], estimateError, measurementError)
                root.valueFregs[i] = filteredFreq
                root.prevFregs[i] = filteredFreq

                var filteredUsage = root.kalmanFilter(root.prevUsage[i], root.rawUsage[i], estimateError, measurementError)
                root.valueUsage[i] = filteredUsage
                root.prevUsage[i] = filteredUsage
            }
            root.valueUsage = root.valueUsage.slice()
            root.prevUsage = root.prevUsage.slice()
            root.valueFregs = root.valueFregs.slice()
            root.prevFregs = root.prevFregs.slice()
        }
    }

    ColumnLayout {
        id: contentColumn
        width: root.width
        spacing: 12

        AppCard {
            Layout.fillWidth: true
            Layout.preferredHeight: 360
            title: qsTr("CPU performance")
            subtitle: cpuConfig && cpuConfig.cpus ? cpuConfig.cpus.length + qsTr(" logical cores") : qsTr("Waiting for telemetry")
            surfaceColor: root.surfaceColor
            borderColor: root.borderColor
            textColor: root.textColor
            mutedTextColor: root.mutedTextColor

            RowLayout {
                Layout.fillWidth: true
                spacing: 14

                Repeater {
                    model: [
                        {name: qsTr("Frequency"), color: root.accentColor},
                        {name: qsTr("Usage"), color: root.secondaryAccentColor},
                        {name: qsTr("Limit"), color: root.accentColor}
                    ]

                    RowLayout {
                        spacing: 6
                        Rectangle {
                            Layout.preferredWidth: 18
                            Layout.preferredHeight: 8
                            radius: 4
                            color: modelData.color
                            opacity: index === 1 ? 0.65 : 1
                            border.color: index === 2 ? root.surfaceColor : "transparent"
                            border.width: index === 2 ? 1 : 0
                        }
                        Label {
                            text: modelData.name
                            color: root.mutedTextColor
                            font.pixelSize: 12
                        }
                    }
                }
            }

            Rectangle {
                id: freqGraph
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "transparent"

                property int barCount: root.valueFregs.length
                property real gap: barCount > 0 ? Math.max(5, width / (barCount * 5)) : 5
                property real barWidth: barCount > 0 ? Math.max(8, (width - gap * (barCount + 1)) / barCount) : 0
                property int hoveredIndex: -1
                property real tooltipX: 0
                property real tooltipY: 0
                property string tooltipText: ""

                Rectangle {
                    anchors.fill: parent
                    color: root.elevatedColor
                    radius: 8
                    border.color: root.borderColor
                }

                Label {
                    visible: freqGraph.hoveredIndex >= 0
                    text: freqGraph.tooltipText
                    color: root.textColor
                    font.pixelSize: 12
                    padding: 7
                    background: Rectangle {
                        color: "#10141c"
                        radius: 6
                        border.color: root.borderColor
                    }
                    x: Math.max(8, Math.min(freqGraph.tooltipX - width / 2, freqGraph.width - width - 8))
                    y: Math.max(8, freqGraph.tooltipY - height - 8)
                    z: 20
                }

                Repeater {
                    model: freqGraph.barCount
                    Rectangle {
                        x: freqGraph.gap + index * (freqGraph.barWidth + freqGraph.gap)
                        width: freqGraph.barWidth
                        height: Math.max(2, (freqGraph.height - 30) * root.valueFregs[index])
                        y: freqGraph.height - 22 - height
                        color: root.accentColor
                        radius: 5
                        opacity: 0.82
                    }
                }

                Repeater {
                    model: freqGraph.barCount
                    Rectangle {
                        x: freqGraph.gap + index * (freqGraph.barWidth + freqGraph.gap)
                        width: freqGraph.barWidth
                        height: Math.max(2, (freqGraph.height - 30) * root.valueUsage[index] * root.valueFregs[index])
                        y: freqGraph.height - 22 - height
                        color: root.secondaryAccentColor
                        radius: 5
                        opacity: 0.72
                    }
                }

                Repeater {
                    model: freqGraph.barCount
                    Rectangle {
                        id: controlRect
                        property int idx: index
                        property real dragStartY: 0

                        x: freqGraph.gap + idx * (freqGraph.barWidth + freqGraph.gap) - 4
                        width: freqGraph.barWidth + 8
                        height: 14
                        y: freqGraph.height - 22 - ((freqGraph.height - 30) * root.normalized(root.prevMaxFreqScaling[idx], idx)) - height / 2
                        color: root.surfaceColor
                        border.color: root.accentColor
                        border.width: 2
                        radius: 7

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.SizeVerCursor

                            onEntered: controlRect.color = Qt.lighter(root.surfaceColor, 1.25)
                            onExited: controlRect.color = root.surfaceColor
                            onPressed: function(mouse) {
                                controlRect.dragStartY = mouse.y
                                freqGraph.hoveredIndex = index
                                root.editingCpuLimit = true
                            }
                            onPositionChanged: function(mouse) {
                                if (!pressed)
                                    return
                                var newY = controlRect.y + mouse.y - controlRect.dragStartY + controlRect.height / 2
                                var norm = 1 - Math.max(0, Math.min(newY, freqGraph.height - 30)) / (freqGraph.height - 30)
                                norm = Math.max(0, Math.min(norm, 1))
                                root.updateDraftLimit(controlRect.idx, root.denormalized(norm, controlRect.idx))
                                freqGraph.tooltipX = controlRect.x + controlRect.width / 2
                                freqGraph.tooltipY = controlRect.y
                                freqGraph.tooltipText = (root.denormalized(norm, controlRect.idx) / 1000000).toFixed(2) + " GHz"
                            }
                            onReleased: {
                                root.commitDraftLimits()
                                root.editingCpuLimit = false
                                freqGraph.hoveredIndex = -1
                            }
                            onCanceled: {
                                root.editingCpuLimit = false
                                freqGraph.hoveredIndex = -1
                            }
                        }
                    }
                }

                Repeater {
                    model: freqGraph.barCount
                    Label {
                        text: index + 1
                        color: root.mutedTextColor
                        font.pixelSize: 11
                        width: freqGraph.barWidth
                        horizontalAlignment: Text.AlignHCenter
                        x: freqGraph.gap + index * (freqGraph.barWidth + freqGraph.gap)
                        y: freqGraph.height - height
                    }
                }
            }
        }

        AppCard {
            Layout.fillWidth: true
            title: qsTr("CPU controls")
            subtitle: qsTr("Frequency limit and governor")
            surfaceColor: root.surfaceColor
            borderColor: root.borderColor
            textColor: root.textColor
            mutedTextColor: root.mutedTextColor

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Label {
                    text: qsTr("All cores")
                    color: root.textColor
                    font.pixelSize: 13
                    font.bold: true
                }

                Slider {
                    id: globalFreqSlider
                    Layout.fillWidth: true
                    from: root.cpuConfig && root.cpuConfig.cpus.length > 0 ? root.cpuConfig.cpus[0].minFreq / 1000000.0 : 0
                    to: root.cpuConfig && root.cpuConfig.cpus.length > 0 ? root.cpuConfig.cpus[0].maxFreq / 1000000.0 : 100
                    stepSize: 0.1
                    value: root.displayedGlobalFreqGhz() > 0 ? root.displayedGlobalFreqGhz() : to

                    onPressedChanged: {
                        if (pressed) {
                            root.editingCpuLimit = true
                        } else {
                            root.commitDraftLimits()
                            root.editingCpuLimit = false
                        }
                    }

                    onMoved: {
                        if (!root.cpuConfig)
                            return
                        var newFreq = value * 1000000
                        root.updateAllDraftLimits(newFreq)
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 96
                    Layout.preferredHeight: 36
                    radius: 8
                    color: root.elevatedColor
                    border.color: root.borderColor

                    Label {
                        anchors.centerIn: parent
                        text: root.displayedGlobalFreqGhz().toFixed(2) + " GHz"
                        color: root.accentColor
                        font.pixelSize: 13
                        font.bold: true
                    }
                }
            }

            Flow {
                Layout.fillWidth: true
                spacing: 8

                property var availableGovernors: root.cpuConfig && root.cpuConfig.cpus.length > 0 ? root.cpuConfig.cpus[0].availableGovernors || [] : []
                property string availableGovernor: root.cpuConfig && root.cpuConfig.cpus.length > 0 ? root.cpuConfig.cpus[0].availableGovernor || "N/A" : "N/A"

                Repeater {
                    model: parent.availableGovernors

                    delegate: Rectangle {
                        width: Math.max(96, governorLabel.implicitWidth + 26)
                        height: 34
                        radius: 8
                        color: selected ? root.accentColor : "transparent"
                        border.color: selected ? root.accentColor : root.borderColor
                        border.width: 1
                        property bool selected: parent.availableGovernor === modelData

                        Label {
                            id: governorLabel
                            anchors.centerIn: parent
                            text: modelData
                            color: parent.selected ? "#ffffff" : root.textColor
                            font.pixelSize: 12
                            font.bold: parent.selected
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (!root.cpuConfig)
                                    return
                                var config = root.cpuConfig
                                for (var i = 0; i < config.cpus.length; i++)
                                    config.cpus[i].availableGovernor = modelData
                                root.setCpuConfig(config)
                            }
                        }
                    }
                }
            }
        }
    }
}
