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
    property var draftMaxFreqScaling: []
    property var committedMaxFreqScaling: []
    property bool editingCpuLimit: false
    property bool waitingCpuLimitConfirmation: false

    clip: true
    contentWidth: width
    contentHeight: contentColumn.implicitHeight
    boundsBehavior: Flickable.StopAtBounds

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

    function clampFreq(value, idx) {
        if (!cpuConfig || !cpuConfig.cpus || !cpuConfig.cpus[idx])
            return value
        var core = cpuConfig.cpus[idx]
        return Math.max(core.minFreq, Math.min(core.maxFreq, value))
    }

    function currentMaxFreqScaling() {
        if (!root.cpuConfig || !root.cpuConfig.cpus)
            return []

        var values = []
        for (var i = 0; i < root.cpuConfig.cpus.length; i++)
            values.push(root.cpuConfig.cpus[i].scalingMaxFreq)
        return values
    }

    function updateDraftLimit(idx, value) {
        if (!root.cpuConfig || !root.cpuConfig.cpus || !root.cpuConfig.cpus[idx])
            return

        var values = root.draftMaxFreqScaling.length === root.cpuConfig.cpus.length
            ? root.draftMaxFreqScaling.slice()
            : Array(root.cpuConfig.cpus.length).fill(0)
        values[idx] = Math.round(clampFreq(value, idx))
        root.draftMaxFreqScaling = values
    }

    function updateAllDraftLimits(value) {
        if (!root.cpuConfig || !root.cpuConfig.cpus)
            return

        var values = []
        for (var i = 0; i < root.cpuConfig.cpus.length; ++i)
            values.push(Math.round(clampFreq(value, i)))
        root.draftMaxFreqScaling = values
    }

    function commitDraftLimits() {
        if (!root.cpuConfig || !root.cpuConfig.cpus || root.draftMaxFreqScaling.length === 0)
            return

        var config = root.cpuConfig
        for (var i = 0; i < config.cpus.length && i < root.draftMaxFreqScaling.length; ++i)
            config.cpus[i].scalingMaxFreq = Math.round(clampFreq(root.draftMaxFreqScaling[i], i))
        root.committedMaxFreqScaling = root.draftMaxFreqScaling.slice()
        root.waitingCpuLimitConfirmation = true
        cpuConfirmTimeout.restart()
        root.setCpuConfig(config)
    }

    function displayedGlobalFreqGhz() {
        if (root.draftMaxFreqScaling.length > 0)
            return root.draftMaxFreqScaling[0] / 1000000.0
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

        var curMaxFreqScaling = currentMaxFreqScaling()

        if (root.waitingCpuLimitConfirmation) {
            if (arraysEqual(curMaxFreqScaling, root.committedMaxFreqScaling)) {
                root.waitingCpuLimitConfirmation = false
                cpuConfirmTimeout.stop()
                root.draftMaxFreqScaling = curMaxFreqScaling
            }
            return
        }

        if (!root.editingCpuLimit && !arraysEqual(curMaxFreqScaling, draftMaxFreqScaling))
            draftMaxFreqScaling = curMaxFreqScaling
    }

    Timer {
        id: cpuConfirmTimeout
        interval: 2500
        repeat: false
        onTriggered: {
            root.waitingCpuLimitConfirmation = false
            if (!root.editingCpuLimit)
                root.draftMaxFreqScaling = root.currentMaxFreqScaling()
        }
    }

    ColumnLayout {
        id: contentColumn
        width: root.width
        spacing: 12

        CpuPerformanceGraph {
            Layout.fillWidth: true
            Layout.preferredHeight: 340
            cpuConfig: root.cpuConfig
            draftLimits: root.draftMaxFreqScaling
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
