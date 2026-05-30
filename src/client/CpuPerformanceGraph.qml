import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

AppCard {
    id: root

    property var cpuConfig
    property var draftLimits: []
    property color elevatedColor: "#252b36"
    property color accentColor: "#3fa7ff"
    property color secondaryAccentColor: "#6bd0c4"
    property int frequencyEaseMs: 900
    property int usageEaseMs: 1100
    property int limitEaseMs: 260
    property int dragLimitEaseMs: 55
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
        property var targetFreqNorm: []
        property var targetUsageNorm: []
        property var targetLimitNorm: []
        property var displayFreqNorm: []
        property var displayUsageNorm: []
        property var displayLimitNorm: []
        property var filteredFreqNorm: []
        property var filteredUsageNorm: []
        property var freqEstimateError: []
        property var usageEstimateError: []
        property double lastFrameMs: 0
        property bool animating: false
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

    function limitFor(index) {
        var core = coreAt(index)
        if (!core)
            return 0
        if (draftLimits && draftLimits.length > index && Number(draftLimits[index]) > 0)
            return Number(draftLimits[index])
        return Number(core.scalingMaxFreq || core.maxFreq || 0)
    }

    function ghz(value) {
        return (Number(value || 0) / 1000000.0).toFixed(2) + " GHz"
    }

    function roundedRectPath(ctx, x, y, width, height, radius) {
        var r = Math.max(0, Math.min(radius, width / 2, height / 2))
        ctx.moveTo(x + r, y)
        ctx.lineTo(x + width - r, y)
        ctx.quadraticCurveTo(x + width, y, x + width, y + r)
        ctx.lineTo(x + width, y + height - r)
        ctx.quadraticCurveTo(x + width, y + height, x + width - r, y + height)
        ctx.lineTo(x + r, y + height)
        ctx.quadraticCurveTo(x, y + height, x, y + height - r)
        ctx.lineTo(x, y + r)
        ctx.quadraticCurveTo(x, y, x + r, y)
    }

    function valueAt(values, index, fallback) {
        return values && values.length > index ? Number(values[index]) : fallback
    }

    function smoothStep(current, target, factor) {
        if (Math.abs(current - target) < 0.00035)
            return target
        return current + (target - current) * factor
    }

    function easingFactor(deltaMs, durationMs) {
        return 1.0 - Math.exp(-Math.max(1, deltaMs) / Math.max(1, durationMs))
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
        if (filterTelemetry === false && internal.targetFreqNorm.length === coreCount) {
            freq = internal.targetFreqNorm.slice()
            usage = internal.targetUsageNorm.slice()
        } else if (resetFilter) {
            internal.filteredFreqNorm = measuredFreq.slice()
            internal.filteredUsageNorm = measuredUsage.slice()
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
            internal.filteredFreqNorm = freq.slice()
            internal.filteredUsageNorm = usage.slice()
            internal.freqEstimateError = freqErrors
            internal.usageEstimateError = usageErrors
        }

        internal.targetFreqNorm = freq
        internal.targetUsageNorm = usage
        internal.targetLimitNorm = limit

        if (resetDisplay || internal.displayFreqNorm.length !== coreCount) {
            internal.displayFreqNorm = freq.slice()
            internal.displayUsageNorm = usage.slice()
            internal.displayLimitNorm = limit.slice()
            internal.lastFrameMs = Date.now()
            internal.animating = false
            canvas.requestPaint()
            return
        }
        startAnimation()
    }

    function startAnimation() {
        if (coreCount <= 0) {
            internal.animating = false
            canvas.requestPaint()
            return
        }
        if (!internal.animating)
            internal.lastFrameMs = Date.now()
        internal.animating = true
    }

    function advanceAnimation() {
        if (coreCount <= 0) {
            internal.animating = false
            return
        }

        var now = Date.now()
        var deltaMs = internal.lastFrameMs > 0 ? Math.min(100, now - internal.lastFrameMs) : animationTimer.interval
        internal.lastFrameMs = now
        var freqFactor = easingFactor(deltaMs, root.frequencyEaseMs)
        var usageFactor = easingFactor(deltaMs, root.usageEaseMs)
        var limitFactor = easingFactor(deltaMs, plot.dragging ? root.dragLimitEaseMs : root.limitEaseMs)

        var freq = internal.displayFreqNorm.slice()
        var usage = internal.displayUsageNorm.slice()
        var limit = internal.displayLimitNorm.slice()
        var maxDelta = 0
        for (var i = 0; i < coreCount; ++i) {
            var targetFreq = valueAt(internal.targetFreqNorm, i, 0)
            var targetUsage = valueAt(internal.targetUsageNorm, i, 0)
            var targetLimit = valueAt(internal.targetLimitNorm, i, 0)
            freq[i] = smoothStep(valueAt(freq, i, targetFreq), targetFreq, freqFactor)
            usage[i] = smoothStep(valueAt(usage, i, targetUsage), targetUsage, usageFactor)
            limit[i] = smoothStep(valueAt(limit, i, targetLimit), targetLimit, limitFactor)
            maxDelta = Math.max(maxDelta,
                                Math.abs(freq[i] - targetFreq),
                                Math.abs(usage[i] - targetUsage),
                                Math.abs(limit[i] - targetLimit))
        }
        if (!plot.dragging && maxDelta < 0.003) {
            internal.displayFreqNorm = internal.targetFreqNorm.slice()
            internal.displayUsageNorm = internal.targetUsageNorm.slice()
            internal.displayLimitNorm = internal.targetLimitNorm.slice()
            internal.animating = false
        } else {
            internal.displayFreqNorm = freq
            internal.displayUsageNorm = usage
            internal.displayLimitNorm = limit
        }
        canvas.requestPaint()
    }

    Timer {
        id: animationTimer
        interval: 50
        running: root.visible && root.coreCount > 0 && internal.animating
        repeat: true
        onTriggered: root.advanceAnimation()
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

        property int hoveredIndex: -1
        property bool dragging: false
        property real tooltipX: 0
        property real tooltipY: 0
        property string tooltipText: ""

        function plotLeft() { return 14 }
        function plotRight() { return width - 14 }
        function plotTop() { return 18 }
        function plotBottom() { return height - 28 }
        function plotWidth() { return Math.max(1, plotRight() - plotLeft()) }
        function plotHeight() { return Math.max(1, plotBottom() - plotTop()) }
        function slotWidth() { return root.coreCount > 0 ? plotWidth() / root.coreCount : 0 }
        function barWidth() { return Math.max(8, Math.min(28, slotWidth() * 0.52)) }

        function indexFromX(x) {
            if (root.coreCount <= 0)
                return -1
            return root.clamp(Math.floor((x - plotLeft()) / Math.max(1, slotWidth())), 0, root.coreCount - 1)
        }

        function limitFromY(y, index) {
            var norm = 1 - root.clamp((y - plotTop()) / plotHeight(), 0, 1)
            return root.minFreq(index) + norm * (root.maxFreq(index) - root.minFreq(index))
        }

        function updateHover(x, y) {
            hoveredIndex = indexFromX(x)
            if (hoveredIndex < 0) {
                tooltipText = ""
                canvas.requestPaint()
                return
            }

            var core = root.coreAt(hoveredIndex)
            tooltipX = x
            tooltipY = y
            tooltipText = qsTr("Core") + " " + (hoveredIndex + 1) + "\n" +
                          qsTr("Frequency") + " " + root.ghz(core.currentFreq) + "\n" +
                          qsTr("Usage") + " " + Number(core.usage || 0).toFixed(0) + "%\n" +
                          qsTr("Limit") + " " + root.ghz(root.limitFor(hoveredIndex))
            canvas.requestPaint()
        }

        Canvas {
            id: canvas
            anchors.fill: parent
            antialiasing: true

            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            Connections {
                target: root
                function onAccentColorChanged() { canvas.requestPaint() }
                function onSecondaryAccentColorChanged() { canvas.requestPaint() }
            }

            onPaint: {
                var ctx = getContext("2d")
                ctx.setTransform(1, 0, 0, 1, 0, 0)
                ctx.clearRect(0, 0, width, height)

                var left = plot.plotLeft()
                var right = plot.plotRight()
                var top = plot.plotTop()
                var bottom = plot.plotBottom()
                var plotHeight = plot.plotHeight()
                var slot = plot.slotWidth()
                var bar = plot.barWidth()

                ctx.lineWidth = 1
                ctx.strokeStyle = root.borderColor
                for (var grid = 0; grid < 4; ++grid) {
                    var gy = top + plotHeight * grid / 3
                    ctx.beginPath()
                    ctx.moveTo(left, gy)
                    ctx.lineTo(right, gy)
                    ctx.stroke()
                }

                if (root.coreCount <= 0)
                    return

                for (var i = 0; i < root.coreCount; ++i) {
                    var core = root.coreAt(i)
                    var center = left + slot * i + slot / 2
                    var x = center - bar / 2
                    var freqNorm = root.valueAt(internal.displayFreqNorm, i, root.normalizeFreq(core.currentFreq, i))
                    var usageNorm = root.valueAt(internal.displayUsageNorm, i, root.clamp(Number(core.usage || 0) / 100.0, 0, 1))
                    var limitNorm = plot.dragging ? root.normalizeFreq(root.limitFor(i), i)
                                                   : root.valueAt(internal.displayLimitNorm, i, root.normalizeFreq(root.limitFor(i), i))
                    var freqHeight = Math.max(2, plotHeight * freqNorm)
                    var usageHeight = Math.max(2, plotHeight * usageNorm)
                    var limitY = bottom - plotHeight * limitNorm

                    ctx.fillStyle = Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.16)
                    ctx.beginPath()
                    root.roundedRectPath(ctx, x, top, bar, plotHeight, 5)
                    ctx.fill()

                    ctx.fillStyle = Qt.rgba(root.accentColor.r, root.accentColor.g, root.accentColor.b, 0.86)
                    ctx.beginPath()
                    root.roundedRectPath(ctx, x, bottom - freqHeight, bar, freqHeight, 5)
                    ctx.fill()

                    ctx.fillStyle = Qt.rgba(root.secondaryAccentColor.r, root.secondaryAccentColor.g, root.secondaryAccentColor.b, 0.72)
                    ctx.beginPath()
                    root.roundedRectPath(ctx, x + bar * 0.18, bottom - usageHeight, bar * 0.64, usageHeight, 4)
                    ctx.fill()

                    ctx.lineWidth = plot.hoveredIndex === i || plot.dragging ? 3 : 2
                    ctx.strokeStyle = root.accentColor
                    ctx.beginPath()
                    ctx.moveTo(x - 5, limitY)
                    ctx.lineTo(x + bar + 5, limitY)
                    ctx.stroke()

                    ctx.fillStyle = root.mutedTextColor
                    ctx.font = "11px sans-serif"
                    ctx.textAlign = "center"
                    ctx.textBaseline = "top"
                    ctx.fillText(String(i + 1), center, bottom + 8)
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: containsMouse ? Qt.SizeVerCursor : Qt.ArrowCursor

            onEntered: plot.updateHover(mouseX, mouseY)
            onExited: {
                if (!plot.dragging) {
                    plot.hoveredIndex = -1
                    plot.tooltipText = ""
                    canvas.requestPaint()
                }
            }
            onPositionChanged: function(mouse) {
                plot.updateHover(mouse.x, mouse.y)
                if (!pressed || plot.hoveredIndex < 0)
                    return
                root.limitEdited(plot.hoveredIndex, plot.limitFromY(mouse.y, plot.hoveredIndex))
                plot.updateHover(mouse.x, mouse.y)
            }
            onPressed: function(mouse) {
                plot.dragging = true
                root.editingChanged(true)
                plot.updateHover(mouse.x, mouse.y)
                if (plot.hoveredIndex >= 0)
                    root.limitEdited(plot.hoveredIndex, plot.limitFromY(mouse.y, plot.hoveredIndex))
                plot.updateHover(mouse.x, mouse.y)
            }
            onReleased: {
                plot.dragging = false
                root.commitRequested()
                root.editingChanged(false)
                canvas.requestPaint()
            }
            onCanceled: {
                plot.dragging = false
                root.editingChanged(false)
                canvas.requestPaint()
            }
        }

        Label {
            visible: plot.tooltipText.length > 0
            text: plot.tooltipText
            color: root.textColor
            font.pixelSize: 12
            padding: 8
            background: Rectangle {
                color: "#10141c"
                radius: 6
                border.color: root.borderColor
            }
            x: Math.max(8, Math.min(plot.tooltipX - width / 2, plot.width - width - 8))
            y: Math.max(8, Math.min(plot.tooltipY - height - 10, plot.height - height - 8))
            z: 10
        }
    }
}
