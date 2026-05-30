import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MsiController 1.0

AppCard {
    id: root

    property var cpuConfig
    property var draftLimits: []
    property color elevatedColor: "#252b36"
    property color accentColor: "#3fa7ff"
    property color secondaryAccentColor: "#6bd0c4"
    property bool editingLimits: false
    property int telemetryEaseMs: 620
    property int limitEaseMs: 240
    property int dragLimitEaseMs: 45
    property real frequencyKalmanProcessNoise: 0.0007
    property real frequencyKalmanMeasurementNoise: 0.22
    property real usageKalmanProcessNoise: 0.0006
    property real usageKalmanMeasurementNoise: 0.24
    property real initialKalmanError: 0.02

    signal limitEdited(int coreIndex, real limitHz)
    signal editingChanged(bool editing)
    signal commitRequested()

    readonly property int coreCount: cpuConfig && cpuConfig.cpus ? cpuConfig.cpus.length : 0

    QtObject {
        id: internal

        property var displayFreqNorm: []
        property var displayUsageNorm: []
        property var displayLimitNorm: []
        property var filteredFreqNorm: []
        property var filteredUsageNorm: []
        property var freqEstimateError: []
        property var usageEstimateError: []
    }

    implicitHeight: 320
    title: qsTr("CPU performance")
    subtitle: coreCount > 0 ? coreCount + qsTr(" logical cores") : qsTr("Waiting for telemetry")

    Component.onCompleted: syncTargets(true)
    onCpuConfigChanged: syncTargets(internal.displayFreqNorm.length !== coreCount, true)
    onDraftLimitsChanged: syncTargets(false, false)
    onCoreCountChanged: syncTargets(true, true)

    function coreAt(index) {
        return cpuConfig && cpuConfig.cpus && index >= 0 && index < cpuConfig.cpus.length ? cpuConfig.cpus[index] : null
    }

    function clamp(value, minValue, maxValue) {
        return Math.max(minValue, Math.min(maxValue, value))
    }

    function minFreq(index) {
        var core = coreAt(index)
        return core ? Number(core.minFreq || 0) : 0
    }

    function maxFreq(index) {
        var core = coreAt(index)
        return core ? Math.max(Number(core.maxFreq || 0), minFreq(index) + 1) : 1
    }

    function normalizeFreq(value, index) {
        var minValue = minFreq(index)
        var maxValue = maxFreq(index)
        return clamp((Number(value || 0) - minValue) / Math.max(1, maxValue - minValue), 0, 1)
    }

    function denormalizeFreq(value, index) {
        return minFreq(index) + clamp(value, 0, 1) * (maxFreq(index) - minFreq(index))
    }

    function limitFor(index) {
        var core = coreAt(index)
        if (!core)
            return 0
        if (draftLimits && draftLimits.length > index && Number(draftLimits[index]) > 0)
            return Number(draftLimits[index])
        return Number(core.scalingMaxFreq || core.maxFreq || 0)
    }

    function valueAt(values, index, fallback) {
        return values && values.length > index ? Number(values[index]) : fallback
    }

    function ghz(value) {
        return (Number(value || 0) / 1000000.0).toFixed(2) + " GHz"
    }

    function kalmanFilter(measurement, previous, estimateError, processNoise, measurementNoise) {
        var predictedError = estimateError + processNoise
        var gain = predictedError / (predictedError + measurementNoise)
        return {
            value: previous + gain * (measurement - previous),
            error: (1.0 - gain) * predictedError
        }
    }

    function syncTargets(resetDisplay, filterTelemetry) {
        var measuredFreq = []
        var measuredUsage = []
        var limit = []
        for (var i = 0; i < coreCount; ++i) {
            var core = coreAt(i)
            measuredFreq.push(normalizeFreq(core ? core.currentFreq : 0, i))
            measuredUsage.push(clamp(Number(core ? core.usage || 0 : 0) / 100.0, 0, 1))
            limit.push(normalizeFreq(limitFor(i), i))
        }

        var freq = measuredFreq
        var usage = measuredUsage
        var resetFilter = resetDisplay || internal.filteredFreqNorm.length !== coreCount
        if (filterTelemetry === false && internal.displayFreqNorm.length === coreCount) {
            freq = internal.displayFreqNorm.slice()
            usage = internal.displayUsageNorm.slice()
        } else if (resetFilter) {
            internal.freqEstimateError = Array(coreCount).fill(root.initialKalmanError)
            internal.usageEstimateError = Array(coreCount).fill(root.initialKalmanError)
        } else {
            freq = []
            usage = []
            var freqErrors = internal.freqEstimateError.slice()
            var usageErrors = internal.usageEstimateError.slice()
            for (var j = 0; j < coreCount; ++j) {
                var filteredFreq = kalmanFilter(measuredFreq[j],
                                                valueAt(internal.filteredFreqNorm, j, measuredFreq[j]),
                                                valueAt(freqErrors, j, root.initialKalmanError),
                                                root.frequencyKalmanProcessNoise,
                                                root.frequencyKalmanMeasurementNoise)
                var filteredUsage = kalmanFilter(measuredUsage[j],
                                                 valueAt(internal.filteredUsageNorm, j, measuredUsage[j]),
                                                 valueAt(usageErrors, j, root.initialKalmanError),
                                                 root.usageKalmanProcessNoise,
                                                 root.usageKalmanMeasurementNoise)
                freq.push(filteredFreq.value)
                usage.push(filteredUsage.value)
                freqErrors[j] = filteredFreq.error
                usageErrors[j] = filteredUsage.error
            }
            internal.freqEstimateError = freqErrors
            internal.usageEstimateError = usageErrors
        }

        internal.filteredFreqNorm = freq.slice()
        internal.filteredUsageNorm = usage.slice()
        internal.displayFreqNorm = freq
        internal.displayUsageNorm = usage
        internal.displayLimitNorm = limit
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 14

        LegendItem { label: qsTr("Frequency"); markerColor: root.accentColor; opacityValue: 0.9 }
        LegendItem { label: qsTr("Usage"); markerColor: root.secondaryAccentColor; opacityValue: 0.7 }
        LegendItem { label: qsTr("Limit"); markerColor: root.accentColor; hollow: true }
        Item { Layout.fillWidth: true }
    }

    component LegendItem: RowLayout {
        id: legendRoot

        property string label: ""
        property color markerColor: "#3fa7ff"
        property bool hollow: false
        property real opacityValue: 1.0

        spacing: 6

        Rectangle {
            Layout.preferredWidth: 18
            Layout.preferredHeight: 8
            radius: 4
            color: legendRoot.hollow ? "transparent" : legendRoot.markerColor
            opacity: legendRoot.opacityValue
            border.color: legendRoot.markerColor
            border.width: legendRoot.hollow ? 2 : 0
        }

        Label {
            text: legendRoot.label
            color: root.mutedTextColor
            font.pixelSize: 12
        }
    }

    Rectangle {
        id: plot

        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumHeight: 210
        radius: 8
        color: root.elevatedColor
        border.color: root.borderColor
        clip: true

        property int hoveredIndex: -1
        property int limitHoverIndex: -1
        property int editingIndex: -1
        property bool dragging: false
        property real tooltipX: 0
        property real tooltipY: 0

        function plotLeft() { return graph.leftPadding }
        function plotRight() { return graph.width - graph.rightPadding }
        function plotTop() { return graph.topPadding }
        function plotBottom() { return graph.plotBottom() }
        function plotWidth() { return Math.max(1, plotRight() - plotLeft()) }
        function plotHeight() { return Math.max(1, plotBottom() - plotTop()) }
        function slotWidth() { return graph.slotWidth() }
        function barWidth() { return graph.barWidth() }

        function indexFromX(x) {
            return graph.coreAt(x)
        }

        function limitFromY(y, index) {
            var norm = graph.limitNormFromY(y)
            return root.minFreq(index) + norm * (root.maxFreq(index) - root.minFreq(index))
        }

        function limitNorm(index) {
            return graph.displayLimitNorm(index)
        }

        function limitY(index) {
            return graph.limitY(index)
        }

        function coreCenter(index) {
            return graph.coreCenter(index)
        }

        function limitIndexAt(x, y) {
            return graph.limitAt(x, y)
        }

        function updateHover(x, y) {
            hoveredIndex = dragging && editingIndex >= 0 ? editingIndex : indexFromX(x)
            limitHoverIndex = dragging && editingIndex >= 0 ? editingIndex : limitIndexAt(x, y)
            tooltipX = x
            tooltipY = y
        }

        function currentTooltipText(frameTick) {
            var tick = frameTick
            if (hoveredIndex < 0)
                return ""

            var core = root.coreAt(hoveredIndex)
            var freqNorm = graph.displayFrequencyNorm(hoveredIndex)
            var usageNorm = graph.displayUsageNorm(hoveredIndex)
            return qsTr("Core") + " " + (hoveredIndex + 1) + "\n" +
                   qsTr("Frequency") + " " + root.ghz(root.denormalizeFreq(freqNorm, hoveredIndex)) + "\n" +
                   qsTr("Usage") + " " + (usageNorm * 100).toFixed(0) + "%\n" +
                   qsTr("Limit") + " " + root.ghz(root.limitFor(hoveredIndex))
        }

        GpuCpuPerformanceGraph {
            id: graph
            anchors.fill: parent
            frequencyNorms: internal.displayFreqNorm
            usageNorms: internal.displayUsageNorm
            limitNorms: internal.displayLimitNorm
            trackColor: Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.13)
            frequencyColor: Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.86)
            usageColor: Qt.rgba(root.secondaryAccentColor.r, root.secondaryAccentColor.g, root.secondaryAccentColor.b, 0.72)
            limitColor: root.accentColor
            gridColor: root.borderColor
            limitHoverIndex: plot.limitHoverIndex
            editingIndex: plot.editingIndex
            animationFps: 30
            telemetryEaseMs: root.telemetryEaseMs
            limitEaseMs: root.limitEaseMs
            dragLimitEaseMs: root.dragLimitEaseMs
            changeEpsilon: 0.0035
        }

        Repeater {
            model: root.coreCount

            Label {
                readonly property real labelSlotWidth: root.coreCount > 0
                                                           ? Math.max(1, (plot.width - graph.leftPadding - graph.rightPadding) / root.coreCount)
                                                           : 0
                x: graph.leftPadding + labelSlotWidth * index
                y: plot.height - graph.bottomPadding + 8
                width: labelSlotWidth
                text: String(index + 1)
                color: root.mutedTextColor
                font.pixelSize: 11
                horizontalAlignment: Text.AlignHCenter
            }
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            preventStealing: true
            cursorShape: plot.dragging || plot.limitHoverIndex >= 0 ? Qt.SizeVerCursor : Qt.ArrowCursor

            onEntered: plot.updateHover(mouseX, mouseY)
            onExited: {
                if (!plot.dragging) {
                    plot.hoveredIndex = -1
                    plot.limitHoverIndex = -1
                }
            }
            onPositionChanged: function(mouse) {
                if (plot.dragging)
                    mouse.accepted = true
                plot.updateHover(mouse.x, mouse.y)
                if (!pressed || !plot.dragging || plot.editingIndex < 0)
                    return
                root.limitEdited(plot.editingIndex, plot.limitFromY(mouse.y, plot.editingIndex))
                plot.updateHover(mouse.x, mouse.y)
            }
            onPressed: function(mouse) {
                plot.updateHover(mouse.x, mouse.y)
                var editIndex = plot.limitIndexAt(mouse.x, mouse.y)
                if (editIndex < 0) {
                    mouse.accepted = false
                    return
                }

                mouse.accepted = true
                plot.dragging = true
                plot.editingIndex = editIndex
                plot.limitHoverIndex = editIndex
                root.editingChanged(true)
                root.limitEdited(plot.editingIndex, plot.limitFromY(mouse.y, plot.editingIndex))
                plot.updateHover(mouse.x, mouse.y)
            }
            onReleased: function(mouse) {
                if (!plot.dragging)
                    return
                plot.dragging = false
                plot.limitHoverIndex = plot.limitIndexAt(mouse.x, mouse.y)
                plot.editingIndex = -1
                root.commitRequested()
                root.editingChanged(false)
            }
            onCanceled: {
                plot.dragging = false
                plot.limitHoverIndex = -1
                plot.editingIndex = -1
                root.editingChanged(false)
            }
        }

        Label {
            visible: plot.hoveredIndex >= 0
            text: plot.currentTooltipText(graph.frameTick)
            color: Qt.rgba(root.textColor.r, root.textColor.g, root.textColor.b, 0.96)
            font.pixelSize: 12
            padding: 8
            background: Rectangle {
                color: Qt.rgba(root.surfaceColor.r, root.surfaceColor.g, root.surfaceColor.b, 0.72)
                radius: 6
                border.color: Qt.rgba(root.borderColor.r, root.borderColor.g, root.borderColor.b, 0.65)
            }
            x: Math.max(8, Math.min(plot.tooltipX - width / 2, plot.width - width - 8))
            y: Math.max(8, Math.min(plot.tooltipY - height - 10, plot.height - height - 8))
            z: 10
        }
    }
}
