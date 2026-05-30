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
    property bool embedded: false

    property var cpuParameter: proxy ? proxy.getProxyParameter(Msi.Parametr.CpuConfig) : null
    property var cpuControlParameter: proxy ? proxy.getProxyParameter(Msi.Parametr.CpuControlConfig) : null
    property var cpuConfig: cpuParameter ? cpuParameter.value : null
    property var cpuControlConfig: cpuControlParameter && cpuControlParameter.value && cpuControlParameter.value.cpus
                                   ? cpuControlParameter.value
                                   : cpuConfig
    property var draftMaxFreqScaling: []
    property bool editingCpuLimit: false
    readonly property int cpuLimitConfirmTolerance: 25000

    clip: true
    implicitHeight: contentColumn.implicitHeight
    interactive: !embedded
    contentWidth: width
    contentHeight: contentColumn.implicitHeight
    boundsBehavior: Flickable.StopAtBounds

    function arraysEqual(a, b) {
        if (a === b)
            return true
        if (!a || !b || a.length !== b.length)
            return false
        for (var i = 0; i < a.length; ++i) {
            var aValue = Number(a[i])
            var bValue = Number(b[i])
            if (!isNaN(aValue) && !isNaN(bValue)) {
                if (Math.abs(aValue - bValue) > root.cpuLimitConfirmTolerance)
                    return false
            } else if (a[i] !== b[i]) {
                return false
            }
        }
        return true
    }

    function clampFreq(value, idx) {
        if (!cpuControlConfig || !cpuControlConfig.cpus || !cpuControlConfig.cpus[idx])
            return value
        var core = cpuControlConfig.cpus[idx]
        return Math.max(core.minFreq, Math.min(core.maxFreq, value))
    }

    function currentMaxFreqScaling() {
        if (!root.cpuControlConfig || !root.cpuControlConfig.cpus)
            return []

        var values = []
        for (var i = 0; i < root.cpuControlConfig.cpus.length; i++)
            values.push(root.cpuControlConfig.cpus[i].scalingMaxFreq)
        return values
    }

    function updateDraftLimit(idx, value) {
        if (!root.cpuControlConfig || !root.cpuControlConfig.cpus || !root.cpuControlConfig.cpus[idx])
            return

        var values = root.draftMaxFreqScaling.length === root.cpuControlConfig.cpus.length
            ? root.draftMaxFreqScaling.slice()
            : Array(root.cpuControlConfig.cpus.length).fill(0)
        values[idx] = Math.round(clampFreq(value, idx))
        root.draftMaxFreqScaling = values
    }

    function updateAllDraftLimits(value) {
        if (!root.cpuControlConfig || !root.cpuControlConfig.cpus)
            return

        var values = []
        for (var i = 0; i < root.cpuControlConfig.cpus.length; ++i)
            values.push(Math.round(clampFreq(value, i)))
        root.draftMaxFreqScaling = values
    }

    function commitDraftLimits() {
        if (!root.cpuControlConfig || !root.cpuControlConfig.cpus || root.draftMaxFreqScaling.length === 0)
            return

        var values = []
        for (var i = 0; i < root.cpuControlConfig.cpus.length && i < root.draftMaxFreqScaling.length; ++i)
            values.push(Math.round(clampFreq(root.draftMaxFreqScaling[i], i)))

        if (root.proxy && typeof root.proxy.setCpuScalingMaxFrequencies === "function")
            root.proxy.setCpuScalingMaxFrequencies(values)
        else {
            var config = root.cpuControlConfig
            for (var j = 0; j < config.cpus.length && j < values.length; ++j)
                config.cpus[j].scalingMaxFreq = values[j]
            root.setCpuControlConfig(config)
        }
    }

    function displayedGlobalFreqGhz() {
        if (root.draftMaxFreqScaling.length > 0)
            return root.draftMaxFreqScaling[0] / 1000000.0
        if (root.cpuControlConfig && root.cpuControlConfig.cpus && root.cpuControlConfig.cpus.length > 0)
            return root.cpuControlConfig.cpus[0].scalingMaxFreq / 1000000.0
        return 0
    }

    function setCpuControlConfig(config) {
        if (cpuControlParameter && cpuControlParameter.isValid)
            cpuControlParameter.value = config
        else if (cpuParameter)
            cpuParameter.value = config
    }

    function controlWritePending() {
        if (root.cpuControlParameter && root.cpuControlParameter.isValid)
            return root.cpuControlParameter.isPending
        return root.cpuParameter && root.cpuParameter.isPending
    }

    onCpuControlConfigChanged: {
        if (!cpuControlConfig || !cpuControlConfig.cpus)
            return

        var curMaxFreqScaling = currentMaxFreqScaling()

        if (root.editingCpuLimit || root.controlWritePending())
            return

        if (!arraysEqual(curMaxFreqScaling, draftMaxFreqScaling))
            draftMaxFreqScaling = curMaxFreqScaling
    }

    Connections {
        target: root.cpuControlParameter
        function onIsPendingChanged() {
            if (!root.cpuControlParameter || root.cpuControlParameter.isPending || root.editingCpuLimit)
                return

            var curMaxFreqScaling = root.currentMaxFreqScaling()
            if (!root.arraysEqual(curMaxFreqScaling, root.draftMaxFreqScaling))
                root.draftMaxFreqScaling = curMaxFreqScaling
        }
    }

    Connections {
        target: root.cpuParameter
        function onIsPendingChanged() {
            if (root.cpuControlParameter && root.cpuControlParameter.isValid)
                return
            if (!root.cpuParameter || root.cpuParameter.isPending || root.editingCpuLimit)
                return

            var curMaxFreqScaling = root.currentMaxFreqScaling()
            if (!root.arraysEqual(curMaxFreqScaling, root.draftMaxFreqScaling))
                root.draftMaxFreqScaling = curMaxFreqScaling
        }
    }

    ColumnLayout {
        id: contentColumn
        width: root.width
        spacing: 12

        CpuPerformanceGraph {
            Layout.fillWidth: true
            Layout.preferredHeight: root.embedded ? 300 : 340
            cpuConfig: root.cpuConfig
            draftLimits: root.draftMaxFreqScaling
            editingLimits: root.editingCpuLimit
            surfaceColor: root.surfaceColor
            elevatedColor: root.elevatedColor
            borderColor: root.borderColor
            textColor: root.textColor
            mutedTextColor: root.mutedTextColor
            accentColor: root.accentColor
            secondaryAccentColor: root.secondaryAccentColor
            onLimitEdited: function(coreIndex, limitHz) { root.updateDraftLimit(coreIndex, limitHz) }
            onEditingChanged: function(editing) { root.editingCpuLimit = editing }
            onCommitRequested: root.commitDraftLimits()
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
                    from: root.cpuControlConfig && root.cpuControlConfig.cpus.length > 0 ? root.cpuControlConfig.cpus[0].minFreq / 1000000.0 : 0
                    to: root.cpuControlConfig && root.cpuControlConfig.cpus.length > 0 ? root.cpuControlConfig.cpus[0].maxFreq / 1000000.0 : 100
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
                        if (!root.cpuControlConfig)
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

                property var availableGovernors: root.cpuControlConfig && root.cpuControlConfig.cpus.length > 0 ? root.cpuControlConfig.cpus[0].availableGovernors || [] : []
                property string availableGovernor: root.cpuControlConfig && root.cpuControlConfig.cpus.length > 0 ? root.cpuControlConfig.cpus[0].availableGovernor || "N/A" : "N/A"

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
                                if (!root.cpuControlConfig)
                                    return
                                if (root.proxy && typeof root.proxy.setCpuGovernor === "function") {
                                    root.proxy.setCpuGovernor(modelData)
                                } else {
                                    var config = root.cpuControlConfig
                                    for (var i = 0; i < config.cpus.length; i++)
                                        config.cpus[i].availableGovernor = modelData
                                    root.setCpuControlConfig(config)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
