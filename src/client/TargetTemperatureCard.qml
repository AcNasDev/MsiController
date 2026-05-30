import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

AppCard {
    id: root

    property var modeParameter
    property var cpuTargetParameter
    property var gpuTargetParameter
    property color accentColor: "#3fa7ff"
    property color elevatedColor: "#202735"
    readonly property int curveMode: 0
    readonly property int targetTemperatureMode: 1
    readonly property bool active: modeParameter && modeParameter.isValid
    readonly property bool targetMode: active && modeValue() === targetTemperatureMode

    function modeValue() {
        if (!modeParameter || modeParameter.value === undefined || modeParameter.value === null)
            return curveMode

        var value = Number(modeParameter.value)
        if (!isNaN(value))
            return Math.round(value)

        var text = modeParameter.value.toString()
        return text.indexOf("TargetTemperature") >= 0 ? targetTemperatureMode : curveMode
    }

    function setMode(mode) {
        if (modeParameter && modeParameter.isValid)
            modeParameter.value = mode
    }

    function rangeMin(parameter, fallback) {
        return parameter && parameter.availableValues ? Number(parameter.availableValues.min) : fallback
    }

    function rangeMax(parameter, fallback) {
        return parameter && parameter.availableValues ? Number(parameter.availableValues.max) : fallback
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        ModeButton {
            text: qsTr("Curve")
            checked: root.modeValue() === root.curveMode
            onClicked: root.setMode(root.curveMode)
        }

        ModeButton {
            text: qsTr("Target temp")
            checked: root.modeValue() === root.targetTemperatureMode
            onClicked: root.setMode(root.targetTemperatureMode)
        }

        Item { Layout.fillWidth: true }
    }

    component ModeButton: Button {
        id: modeButton

        enabled: root.active
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
        columns: width > 520 ? 2 : 1
        columnSpacing: 12
        rowSpacing: 10
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
