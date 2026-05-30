import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MSI.Helpers 1.0

AppCard {
    id: root

    property var hardwareModeParameter
    property var coolerBoostParameter
    property var modeParameter
    property var cpuTargetParameter
    property var gpuTargetParameter
    property color accentColor: "#3fa7ff"
    property color elevatedColor: "#202735"
    readonly property int curveMode: 0
    readonly property int targetTemperatureMode: 1
    readonly property bool active: modeParameter && modeParameter.isValid
    readonly property bool hardwareActive: hardwareModeParameter && hardwareModeParameter.isValid
    readonly property bool coolerBoostActive: coolerBoostParameter && coolerBoostParameter.isValid
    readonly property bool boostMode: coolerBoostChecked()
    readonly property bool targetMode: active && modeValue() === targetTemperatureMode
    readonly property bool targetControlsVisible: targetMode && !boostMode
    readonly property bool twoColumnTargets: width > 380
    property var pendingHardwareMode: null

    implicitHeight: targetControlsVisible && !twoColumnTargets ? 206 : 154

    function modeValue() {
        if (!modeParameter || modeParameter.value === undefined || modeParameter.value === null)
            return curveMode

        var value = Number(modeParameter.value)
        if (!isNaN(value))
            return Math.round(value)

        var text = modeParameter.value.toString()
        return text.indexOf("TargetTemperature") >= 0 ? targetTemperatureMode : curveMode
    }

    function setControlMode(mode) {
        if (modeParameter && modeParameter.isValid)
            modeParameter.value = mode
    }

    function setHardwareMode(mode) {
        if (hardwareModeParameter && hardwareModeParameter.isValid)
            hardwareModeParameter.value = mode
    }

    function coolerBoostChecked() {
        if (!coolerBoostParameter || !coolerBoostParameter.isValid || coolerBoostParameter.value === undefined)
            return false

        if (coolerBoostParameter.availableValues && coolerBoostParameter.availableValues.length > 1)
            return coolerBoostParameter.value === coolerBoostParameter.availableValues[1]

        return !!coolerBoostParameter.value
    }

    function setCoolerBoost(enabled) {
        if (!coolerBoostParameter || !coolerBoostParameter.isValid)
            return

        if (coolerBoostParameter.availableValues && coolerBoostParameter.availableValues.length > 1)
            coolerBoostParameter.value = coolerBoostParameter.availableValues[enabled ? 1 : 0]
        else
            coolerBoostParameter.value = enabled
    }

    function selectHardwareMode(mode) {
        if (!hardwareActive)
            return

        setCoolerBoost(false)
        if (targetMode) {
            pendingHardwareMode = mode
            setControlMode(curveMode)
            hardwareModeDelay.restart()
        } else {
            setControlMode(curveMode)
            setHardwareMode(mode)
        }
    }

    function selectTargetMode() {
        if (!active)
            return

        hardwareModeDelay.stop()
        pendingHardwareMode = null
        setCoolerBoost(false)
        setControlMode(targetTemperatureMode)
    }

    function selectCoolerBoost() {
        if (!coolerBoostActive)
            return

        hardwareModeDelay.stop()
        pendingHardwareMode = null
        setControlMode(curveMode)
        setCoolerBoost(true)
    }

    function hardwareModeText(mode) {
        var text = EnumHelper.enumToString(mode, "FanMode")
        return text && text !== "Unknown" ? text : qsTr("N/A")
    }

    function sameHardwareMode(mode) {
        return hardwareModeParameter && hardwareModeParameter.value === mode
    }

    function rangeMin(parameter, fallback) {
        return parameter && parameter.availableValues ? Number(parameter.availableValues.min) : fallback
    }

    function rangeMax(parameter, fallback) {
        return parameter && parameter.availableValues ? Number(parameter.availableValues.max) : fallback
    }

    Timer {
        id: hardwareModeDelay
        interval: 180
        repeat: false
        onTriggered: {
            if (root.pendingHardwareMode !== null)
                root.setHardwareMode(root.pendingHardwareMode)
            root.pendingHardwareMode = null
        }
    }

    Flow {
        Layout.fillWidth: true
        spacing: 8

        Repeater {
            model: root.hardwareActive && root.hardwareModeParameter.availableValues ? root.hardwareModeParameter.availableValues : []

            delegate: ModeButton {
                text: root.hardwareModeText(modelData)
                checked: !root.boostMode && !root.targetMode && root.sameHardwareMode(modelData)
                buttonEnabled: root.hardwareActive
                onClicked: root.selectHardwareMode(modelData)
            }
        }

        ModeButton {
            text: qsTr("Target temp")
            checked: !root.boostMode && root.targetMode
            buttonEnabled: root.active
            onClicked: root.selectTargetMode()
        }

        ModeButton {
            text: qsTr("Cooler Boost")
            checked: root.boostMode
            buttonEnabled: root.coolerBoostActive
            onClicked: root.selectCoolerBoost()
        }
    }

    component ModeButton: Button {
        id: modeButton

        property bool buttonEnabled: root.active

        enabled: buttonEnabled
        checkable: true
        implicitWidth: Math.max(98, modeText.implicitWidth + 26)
        implicitHeight: 34
        padding: 0

        background: Rectangle {
            radius: 8
            color: modeButton.checked ? root.accentColor : "transparent"
            border.color: modeButton.checked ? root.accentColor : root.borderColor
            border.width: 1
        }

        contentItem: Label {
            id: modeText
            text: modeButton.text
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: modeButton.checked ? "#ffffff" : root.textColor
            font.pixelSize: 12
            font.bold: modeButton.checked
        }
    }

    GridLayout {
        Layout.fillWidth: true
        columns: root.twoColumnTargets ? 2 : 1
        columnSpacing: 12
        rowSpacing: 10
        visible: root.targetControlsVisible
        opacity: root.active ? 1.0 : 0.55

        TargetSlider {
            Layout.fillWidth: true
            label: qsTr("CPU")
            parameter: root.cpuTargetParameter
            accentColor: root.accentColor
            surfaceColor: root.elevatedColor
            borderColor: root.borderColor
            textColor: root.textColor
            minValue: root.rangeMin(root.cpuTargetParameter, 50)
            maxValue: root.rangeMax(root.cpuTargetParameter, 95)
        }

        TargetSlider {
            Layout.fillWidth: true
            label: qsTr("GPU")
            parameter: root.gpuTargetParameter
            accentColor: root.accentColor
            surfaceColor: root.elevatedColor
            borderColor: root.borderColor
            textColor: root.textColor
            minValue: root.rangeMin(root.gpuTargetParameter, 50)
            maxValue: root.rangeMax(root.gpuTargetParameter, 95)
        }
    }

    component TargetSlider: RowLayout {
        id: sliderRoot

        property string label: ""
        property var parameter
        property color accentColor: "#3fa7ff"
        property color surfaceColor: "#202735"
        property color borderColor: "#2b3443"
        property color textColor: "#f4f7fb"
        property int minValue: 50
        property int maxValue: 95

        spacing: 8

        Label {
            Layout.preferredWidth: 34
            text: sliderRoot.label
            color: sliderRoot.textColor
            font.pixelSize: 12
            font.bold: true
        }

        Slider {
            id: targetSlider
            Layout.fillWidth: true
            from: sliderRoot.minValue
            to: sliderRoot.maxValue
            stepSize: 1
            snapMode: Slider.SnapAlways
            enabled: sliderRoot.parameter && sliderRoot.parameter.isValid
            property int draftValue: enabled ? Number(sliderRoot.parameter.value || sliderRoot.minValue) : sliderRoot.minValue
            value: draftValue
            onMoved: draftValue = Math.round(value)
            onPressedChanged: {
                if (!pressed && sliderRoot.parameter && sliderRoot.parameter.isValid)
                    sliderRoot.parameter.value = draftValue
            }

            Connections {
                target: sliderRoot.parameter
                function onValueChanged() {
                    if (!targetSlider.pressed)
                        targetSlider.draftValue = Number(sliderRoot.parameter.value || sliderRoot.minValue)
                }
            }
        }

        Rectangle {
            Layout.preferredWidth: 58
            Layout.preferredHeight: 34
            radius: 8
            color: sliderRoot.surfaceColor
            border.color: sliderRoot.borderColor

            Label {
                anchors.centerIn: parent
                text: targetSlider.draftValue + "°C"
                color: sliderRoot.accentColor
                font.pixelSize: 13
                font.bold: true
            }
        }
    }
}
