import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Qt.labs.platform as Platform
import QtCore 6.5 as QtCore

import MsiController 1.0
import Msi 1.0
import MSI.Helpers 1.0

ApplicationWindow {
    id: mainWindow

    width: Math.max(minimumWidth, Math.min(1180, Screen.availableWidth > 0 ? Screen.availableWidth - 48 : 1180))
    height: Math.max(minimumHeight, Math.min(720, Screen.availableHeight > 0 ? Screen.availableHeight - 72 : 720))
    minimumWidth: 820
    minimumHeight: 520
    visible: false
    title: qsTr("MSI Control Center")

    property bool allowQuit: false
    property int currentPage: 0

    QtCore.Settings {
        id: appSettings
        property string savedTheme: "midnight"
    }

    property string currentTheme: appSettings.savedTheme
    readonly property var themeChoices: [
        {key: "midnight", label: qsTr("Midnight")},
        {key: "graphite", label: qsTr("Graphite")},
        {key: "webshare", label: qsTr("WebShare")},
        {key: "daylight", label: qsTr("Daylight")},
        {key: "contrast", label: qsTr("High Contrast")}
    ]
    readonly property var palettes: ({
        midnight: {
            window: "#10141c", surface: "#181e29", elevated: "#202735", text: "#f4f7fb",
            muted: "#9aa7b8", accent: "#3fa7ff", accent2: "#6bd0c4", good: "#5bd48a",
            warn: "#f2b84b", danger: "#ff6b6b", border: "#2b3443", track: "#293241"
        },
        graphite: {
            window: "#151719", surface: "#202327", elevated: "#2a2e33", text: "#f0f2f4",
            muted: "#a1a8b2", accent: "#ff4d5e", accent2: "#7dd3fc", good: "#70d48c",
            warn: "#f0b75e", danger: "#ff6b6b", border: "#353b42", track: "#31363d"
        },
        webshare: {
            window: "#030101", surface: "#100607", elevated: "#16090b", text: "#f7f0f0",
            muted: "#9ca8bd", accent: "#f23d45", accent2: "#ff5a61", good: "#29cf86",
            warn: "#fbbf24", danger: "#b91c1c", border: "#321113", track: "#241315"
        },
        daylight: {
            window: "#f4f6f8", surface: "#ffffff", elevated: "#eef2f6", text: "#17202c",
            muted: "#667386", accent: "#1167d8", accent2: "#0f9f8f", good: "#218a55",
            warn: "#b66b00", danger: "#c53434", border: "#d8dee8", track: "#e4e9f0"
        },
        contrast: {
            window: "#000000", surface: "#101010", elevated: "#1b1b1b", text: "#ffffff",
            muted: "#d8d8d8", accent: "#fff06a", accent2: "#63e6ff", good: "#6aff96",
            warn: "#ffbf47", danger: "#ff6b6b", border: "#565656", track: "#2d2d2d"
        }
    })
    readonly property var theme: palettes[currentTheme] ? palettes[currentTheme] : palettes.midnight
    readonly property color goodColor: theme.good
    readonly property color dangerColor: theme.danger

    Component.onCompleted: visible = true

    palette: Palette {
        window: mainWindow.theme.window
        windowText: mainWindow.theme.text
        base: mainWindow.theme.surface
        alternateBase: mainWindow.theme.elevated
        text: mainWindow.theme.text
        button: mainWindow.theme.elevated
        buttonText: mainWindow.theme.text
        highlight: mainWindow.theme.accent
        highlightedText: "#ffffff"
        link: mainWindow.theme.accent
        toolTipBase: mainWindow.theme.elevated
        toolTipText: mainWindow.theme.text
        placeholderText: mainWindow.theme.muted
        mid: mainWindow.theme.border
        shadow: "#000000"
    }

    onCurrentThemeChanged: appSettings.savedTheme = currentTheme
    onClosing: function(close) {
        if (!allowQuit) {
            close.accepted = false
            mainWindow.hide()
        }
    }

    EsProxy { id: proxy }

    property var cpuTemp: proxy.getProxyParameter(Msi.Parametr.CpuTempEc)
    property var gpuTemp: proxy.getProxyParameter(Msi.Parametr.GpuTempEc)
    property var fanCpu: proxy.getProxyParameter(Msi.Parametr.FanCpuEc)
    property var fanGpu: proxy.getProxyParameter(Msi.Parametr.FanGpuEc)
    property var batteryCharge: proxy.getProxyParameter(Msi.Parametr.BatteryChargeEc)
    property var batteryStatus: proxy.getProxyParameter(Msi.Parametr.BatteryChargingStatusEc)
    property var batteryThreshold: proxy.getProxyParameter(Msi.Parametr.BatteryThresholdEc)
    property var firmwareVersion: proxy.getProxyParameter(Msi.Parametr.FirmwareVersionEc)
    property var firmwareDate: proxy.getProxyParameter(Msi.Parametr.FirmwareReleaseDateEc)
    property var firmwareTime: proxy.getProxyParameter(Msi.Parametr.FirmwareReleaseTimeEc)
    property var shiftModeParam: proxy.getProxyParameter(Msi.Parametr.ShiftModeEc)
    property var fanModeParam: proxy.getProxyParameter(Msi.Parametr.FanModeEc)
    property var keyboardBacklightParam: proxy.getProxyParameter(Msi.Parametr.KeyboardBacklightEc)
    property var coolerBoostParam: proxy.getProxyParameter(Msi.Parametr.CoolerBoostEc)
    property var usbPowerParam: proxy.getProxyParameter(Msi.Parametr.UsbPowerShareEc)
    property var webcamParam: proxy.getProxyParameter(Msi.Parametr.WebCamEc)
    property var webcamBlockParam: proxy.getProxyParameter(Msi.Parametr.WebCamBlockEc)
    property var fnSwapParam: proxy.getProxyParameter(Msi.Parametr.FnSuperSwapEc)
    property var superBatteryParam: proxy.getProxyParameter(Msi.Parametr.SuperBatteryEc)
    property var micMuteParam: proxy.getProxyParameter(Msi.Parametr.MicMuteEc)
    property var muteLedParam: proxy.getProxyParameter(Msi.Parametr.MuteLedEc)
    property var keyboardModeParam: proxy.getProxyParameter(Msi.Parametr.KeyboardBacklightModeEc)
    property var fanControlModeParam: proxy.getProxyParameter(Msi.Parametr.FanControlMode)
    property var fanTargetCpuTempParam: proxy.getProxyParameter(Msi.Parametr.FanTargetCpuTemp)
    property var fanTargetGpuTempParam: proxy.getProxyParameter(Msi.Parametr.FanTargetGpuTemp)
    readonly property bool coolerBoostActive: binaryChecked(coolerBoostParam)
    readonly property bool targetFanModeActive: fanControlModeValue() === 1
    readonly property bool manualFanCurveActive: !coolerBoostActive && !targetFanModeActive && fanModeValue() === 3

    function fanControlModeValue() {
        if (!fanControlModeParam || !fanControlModeParam.isValid || fanControlModeParam.value === undefined ||
                fanControlModeParam.value === null) {
            return 0
        }

        var value = Number(fanControlModeParam.value)
        if (!isNaN(value))
            return Math.round(value)

        var text = fanControlModeParam.value.toString()
        return text.indexOf("TargetTemperature") >= 0 ? 1 : 0
    }

    function fanModeValue() {
        if (!fanModeParam || !fanModeParam.isValid || fanModeParam.value === undefined ||
                fanModeParam.value === null) {
            return -1
        }

        var value = Number(fanModeParam.value)
        if (!isNaN(value))
            return Math.round(value)

        var text = enumText(fanModeParam.value, "FanMode")
        if (text === "Auto")
            return 0
        if (text === "Silent")
            return 1
        if (text === "Basic")
            return 2
        if (text === "Advanced")
            return 3
        return -1
    }

    function valueText(parameter, unit, decimals) {
        if (!parameter || !parameter.isValid || parameter.value === undefined)
            return "N/A"
        var value = Number(parameter.value)
        if (isNaN(value))
            return parameter.value.toString()
        return value.toFixed(decimals)
    }

    function themeIndex(key) {
        for (var i = 0; i < themeChoices.length; i++) {
            if (themeChoices[i].key === key)
                return i
        }
        return 0
    }

    function formatDate(value) {
        var s = value ? value.toString() : ""
        if (s.length !== 8)
            return "N/A"
        return new Date(s.substring(4, 8), Number(s.substring(0, 2)) - 1, s.substring(2, 4)).toLocaleDateString(Locale.ShortFormat)
    }

    function formatTime(value) {
        var s = value ? value.toString() : ""
        var parts = s.split(":")
        if (parts.length !== 3)
            return "N/A"
        return new Date(0, 0, 0, Number(parts[0]), Number(parts[1]), Number(parts[2])).toLocaleTimeString(Locale.ShortFormat)
    }

    function enumText(value, enumName) {
        var text = EnumHelper.enumToString(value, enumName)
        return text && text !== "Unknown" ? text : "N/A"
    }

    function binaryChecked(parameter) {
        if (!parameter || !parameter.isValid || parameter.value === undefined)
            return false
        if (parameter.availableValues && parameter.availableValues.length > 1)
            return parameter.value === parameter.availableValues[1]
        return !!parameter.value
    }

    function setBinaryParameter(parameter, checked) {
        if (!parameter || !parameter.isValid)
            return
        if (parameter.availableValues && parameter.availableValues.length > 1)
            parameter.value = parameter.availableValues[checked ? 1 : 0]
    }

    function writeFanCurvePoint(pointIndex, temperature, speed, speedParams, tempParams) {
        if (pointIndex < 0 || pointIndex >= speedParams.length)
            return

        var speedParam = speedParams[pointIndex]
        if (speedParam && speedParam.isValid)
            speedParam.value = Math.round(speed)

        if (pointIndex > 0 && pointIndex - 1 < tempParams.length) {
            var tempParam = tempParams[pointIndex - 1]
            if (tempParam && tempParam.isValid)
                tempParam.value = Math.round(temperature)
        }
    }

    function batteryStatusText() {
        if (!batteryStatus || !batteryStatus.isValid)
            return qsTr("Unknown")
        return enumText(batteryStatus.value, "ChargingStatus")
    }

    function exitApplication() {
        allowQuit = true
        Qt.quit()
    }

    Platform.SystemTrayIcon {
        visible: true
        icon.source: "qrc:/resources/icon/logo.svg"
        onActivated: {
            mainWindow.show()
            mainWindow.raise()
            mainWindow.requestActivate()
        }
        menu: Platform.Menu {
            Platform.MenuItem {
                text: qsTr("Show")
                onTriggered: {
                    mainWindow.show()
                    mainWindow.raise()
                    mainWindow.requestActivate()
                }
            }
            Platform.MenuItem {
                text: qsTr("Exit")
                onTriggered: exitApplication()
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: mainWindow.theme.window

        RowLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 12

            Rectangle {
                Layout.preferredWidth: 218
                Layout.fillHeight: true
                radius: 8
                color: mainWindow.theme.surface
                border.color: mainWindow.theme.border

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Image {
                            Layout.preferredWidth: 36
                            Layout.preferredHeight: 36
                            source: "qrc:/resources/icon/logo.svg"
                            smooth: true
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("MSI Control")
                                color: mainWindow.theme.text
                                font.pixelSize: 17
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Center")
                                color: mainWindow.theme.muted
                                font.pixelSize: 12
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        radius: 8
                        color: proxy.isConnected ? Qt.rgba(mainWindow.goodColor.r, mainWindow.goodColor.g, mainWindow.goodColor.b, 0.14)
                                                 : Qt.rgba(mainWindow.dangerColor.r, mainWindow.dangerColor.g, mainWindow.dangerColor.b, 0.14)
                        border.color: proxy.isConnected ? mainWindow.goodColor : mainWindow.dangerColor

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 8

                            Rectangle {
                                Layout.preferredWidth: 10
                                Layout.preferredHeight: 10
                                radius: 5
                                color: proxy.isConnected ? mainWindow.goodColor : mainWindow.dangerColor
                            }

                            Label {
                                Layout.fillWidth: true
                                text: proxy.isConnected ? qsTr("Connected") : qsTr("Disconnected")
                                color: mainWindow.theme.text
                                font.pixelSize: 13
                                font.bold: true
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 7

                        Repeater {
                            model: [
                                {label: qsTr("Dashboard"), page: 0}
                            ]

                            delegate: Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 38
                                radius: 8
                                color: mainWindow.currentPage === modelData.page ? mainWindow.theme.accent : "transparent"
                                border.color: mainWindow.currentPage === modelData.page ? mainWindow.theme.accent : mainWindow.theme.border
                                border.width: mainWindow.currentPage === modelData.page ? 0 : 1

                                Label {
                                    anchors.fill: parent
                                    anchors.leftMargin: 14
                                    anchors.rightMargin: 14
                                    verticalAlignment: Text.AlignVCenter
                                    text: modelData.label
                                    color: mainWindow.currentPage === modelData.page ? "#ffffff" : mainWindow.theme.text
                                font.pixelSize: 14
                                    font.bold: mainWindow.currentPage === modelData.page
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: mainWindow.currentPage = modelData.page
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        ComboBox {
                            Layout.fillWidth: true
                            model: mainWindow.themeChoices
                            textRole: "label"
                            valueRole: "key"
                            currentIndex: mainWindow.themeIndex(mainWindow.currentTheme)
                            onActivated: function(index) {
                                if (index >= 0 && index < mainWindow.themeChoices.length)
                                    mainWindow.currentTheme = mainWindow.themeChoices[index].key
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 112
                            radius: 8
                            color: Qt.rgba(mainWindow.theme.elevated.r, mainWindow.theme.elevated.g,
                                           mainWindow.theme.elevated.b, 0.42)
                            border.color: Qt.rgba(mainWindow.theme.border.r, mainWindow.theme.border.g,
                                                  mainWindow.theme.border.b, 0.78)

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 4

                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("Version ") + appversion
                                    color: mainWindow.theme.text
                                    font.pixelSize: 12
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("Qt ") + qtversion
                                    color: mainWindow.theme.muted
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 1
                                    color: Qt.rgba(mainWindow.theme.border.r, mainWindow.theme.border.g,
                                                   mainWindow.theme.border.b, 0.72)
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: firmwareVersion && firmwareVersion.isValid ? firmwareVersion.value : qsTr("Firmware N/A")
                                    color: mainWindow.theme.text
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: (firmwareDate && firmwareDate.isValid ? formatDate(firmwareDate.value) : "N/A") + "  " +
                                          (firmwareTime && firmwareTime.isValid ? formatTime(firmwareTime.value) : "N/A")
                                    color: mainWindow.theme.muted
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }
            }

            Flickable {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: width
                contentHeight: dashboardColumn.implicitHeight
                interactive: !(fanCurveCpu.isDragging || fanCurveGpu.isDragging || cpuPage.editingCpuLimit)
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {}

                ColumnLayout {
                    id: dashboardColumn
                    width: parent.width
                    spacing: 12

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("Dashboard")
                                    color: mainWindow.theme.text
                                    font.pixelSize: 24
                                    font.bold: true
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: qsTr("Thermals, power and device controls")
                                    color: mainWindow.theme.muted
                                    font.pixelSize: 13
                                }
                            }

                            Rectangle {
                                Layout.preferredWidth: 160
                                Layout.preferredHeight: 42
                                radius: 8
                                color: mainWindow.theme.surface
                                border.color: mainWindow.theme.border

                                Label {
                                    anchors.centerIn: parent
                                    text: qsTr("Battery ") + valueText(batteryCharge, "%", 0) + "%"
                                    color: mainWindow.theme.text
                                    font.pixelSize: 14
                                    font.bold: true
                                }
                            }
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: width >= 920 ? 3 : width >= 600 ? 2 : 1
                            columnSpacing: 12
                            rowSpacing: 12
                            uniformCellWidths: true
                            uniformCellHeights: true

                            MetricTile {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 116
                                title: qsTr("CPU temperature")
                                value: valueText(cpuTemp, "°C", 0)
                                unit: cpuTemp && cpuTemp.isValid ? "°C" : ""
                                detail: qsTr("Embedded controller")
                                chartEnabled: cpuTemp && cpuTemp.isValid
                                chartValue: cpuTemp && cpuTemp.isValid ? Number(cpuTemp.value || 0) : 0
                                chartMin: 0
                                chartMax: 100
                                accentColor: mainWindow.theme.accent
                                surfaceColor: mainWindow.theme.surface
                                borderColor: mainWindow.theme.border
                                textColor: mainWindow.theme.text
                                mutedTextColor: mainWindow.theme.muted
                            }

                            MetricTile {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 116
                                title: qsTr("GPU temperature")
                                value: valueText(gpuTemp, "°C", 0)
                                unit: gpuTemp && gpuTemp.isValid ? "°C" : ""
                                detail: qsTr("Embedded controller")
                                chartEnabled: gpuTemp && gpuTemp.isValid
                                chartValue: gpuTemp && gpuTemp.isValid ? Number(gpuTemp.value || 0) : 0
                                chartMin: 0
                                chartMax: 100
                                accentColor: mainWindow.theme.accent2
                                surfaceColor: mainWindow.theme.surface
                                borderColor: mainWindow.theme.border
                                textColor: mainWindow.theme.text
                                mutedTextColor: mainWindow.theme.muted
                            }

                            MetricTile {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 116
                                title: qsTr("CPU fan")
                                value: valueText(fanCpu, "%", 0)
                                unit: fanCpu && fanCpu.isValid ? "%" : ""
                                detail: coolerBoostActive ? qsTr("Cooler Boost") :
                                                               (targetFanModeActive ? qsTr("Target temp") :
                                                               (fanModeParam && fanModeParam.isValid ? enumText(fanModeParam.value, "FanMode") : qsTr("Fan mode"))
                                                               )
                                chartEnabled: fanCpu && fanCpu.isValid
                                chartValue: fanCpu && fanCpu.isValid ? Number(fanCpu.value || 0) : 0
                                chartMin: 0
                                chartMax: 150
                                accentColor: mainWindow.theme.warn
                                surfaceColor: mainWindow.theme.surface
                                borderColor: mainWindow.theme.border
                                textColor: mainWindow.theme.text
                                mutedTextColor: mainWindow.theme.muted
                            }

                            MetricTile {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 116
                                title: qsTr("GPU fan")
                                value: valueText(fanGpu, "%", 0)
                                unit: fanGpu && fanGpu.isValid ? "%" : ""
                                detail: coolerBoostActive ? qsTr("Cooler Boost") :
                                                            (targetFanModeActive ? qsTr("Target temp") : qsTr("Fan curve"))
                                chartEnabled: fanGpu && fanGpu.isValid
                                chartValue: fanGpu && fanGpu.isValid ? Number(fanGpu.value || 0) : 0
                                chartMin: 0
                                chartMax: 150
                                accentColor: mainWindow.theme.good
                                surfaceColor: mainWindow.theme.surface
                                borderColor: mainWindow.theme.border
                                textColor: mainWindow.theme.text
                                mutedTextColor: mainWindow.theme.muted
                            }

                            MetricTile {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 116
                                title: qsTr("Battery")
                                value: valueText(batteryCharge, "%", 0)
                                unit: batteryCharge && batteryCharge.isValid ? "%" : ""
                                detail: batteryStatusText()
                                chartEnabled: batteryCharge && batteryCharge.isValid
                                chartValue: batteryCharge && batteryCharge.isValid ? Number(batteryCharge.value || 0) : 0
                                chartMin: 0
                                chartMax: 100
                                accentColor: mainWindow.theme.good
                                surfaceColor: mainWindow.theme.surface
                                borderColor: mainWindow.theme.border
                                textColor: mainWindow.theme.text
                                mutedTextColor: mainWindow.theme.muted
                            }
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: width > 850 ? 2 : 1
                            columnSpacing: 16
                            rowSpacing: 16
                            uniformCellWidths: true
                            uniformCellHeights: true

                            OptionGroup {
                                Layout.fillWidth: true
                                title: qsTr("Shift mode")
                                subtitle: qsTr("Laptop performance profile")
                                parameter: shiftModeParam
                                enumName: "ShiftMode"
                                accentColor: mainWindow.theme.accent
                                surfaceColor: mainWindow.theme.surface
                                borderColor: mainWindow.theme.border
                                textColor: mainWindow.theme.text
                                mutedTextColor: mainWindow.theme.muted
                            }

                            TargetTemperatureCard {
                                Layout.fillWidth: true
                                Layout.preferredHeight: implicitHeight
                                title: qsTr("Cooling mode")
                                subtitle: coolerBoostActive ? qsTr("Maximum cooling") :
                                                               (targetFanModeActive ? qsTr("Target temperature") :
                                                                  (manualFanCurveActive ? qsTr("Manual fan curve") : qsTr("Firmware profile"))
                                                               )
                                visible: (fanModeParam && fanModeParam.isValid) ||
                                         (fanControlModeParam && fanControlModeParam.isValid) ||
                                         (coolerBoostParam && coolerBoostParam.isValid)
                                hardwareModeParameter: fanModeParam
                                coolerBoostParameter: coolerBoostParam
                                modeParameter: fanControlModeParam
                                cpuTargetParameter: fanTargetCpuTempParam
                                gpuTargetParameter: fanTargetGpuTempParam
                                surfaceColor: mainWindow.theme.surface
                                elevatedColor: mainWindow.theme.elevated
                                borderColor: mainWindow.theme.border
                                textColor: mainWindow.theme.text
                                mutedTextColor: mainWindow.theme.muted
                                accentColor: mainWindow.theme.warn
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Cooling")
                            color: mainWindow.theme.text
                            font.pixelSize: 18
                            font.bold: true
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: width > 900 ? 2 : 1
                            columnSpacing: 16
                            rowSpacing: 16
                            uniformCellWidths: true
                            uniformCellHeights: true

                            FanCurveCard {
                                id: fanCurveCpu
                                Layout.fillWidth: true
                                Layout.preferredHeight: 270
                                title: qsTr("CPU fan curve")
                                subtitle: coolerBoostActive ? qsTr("Cooler Boost") :
                                                              (targetFanModeActive ? qsTr("Managed by service") :
                                                                 (manualFanCurveActive ? qsTr("Temperature to speed map") : qsTr("Managed by firmware"))
                                                              )
                                surfaceColor: mainWindow.theme.surface
                                borderColor: mainWindow.theme.border
                                textColor: mainWindow.theme.text
                                mutedTextColor: mainWindow.theme.muted
                                gridColor: mainWindow.theme.track
                                accentColor: mainWindow.theme.warn
                                enabled: manualFanCurveActive
                                opacity: manualFanCurveActive ? 1.0 : 0.55
                                visible: isValid

                                property var fssg1: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedCpu1Ec)
                                property var fssg2: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedCpu2Ec)
                                property var fssg3: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedCpu3Ec)
                                property var fssg4: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedCpu4Ec)
                                property var fssg5: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedCpu5Ec)
                                property var fssg6: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedCpu6Ec)
                                property var fssg7: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedCpu7Ec)
                                property var fstg1: proxy.getProxyParameter(Msi.Parametr.FanSetTempCpu1Ec)
                                property var fstg2: proxy.getProxyParameter(Msi.Parametr.FanSetTempCpu2Ec)
                                property var fstg3: proxy.getProxyParameter(Msi.Parametr.FanSetTempCpu3Ec)
                                property var fstg4: proxy.getProxyParameter(Msi.Parametr.FanSetTempCpu4Ec)
                                property var fstg5: proxy.getProxyParameter(Msi.Parametr.FanSetTempCpu5Ec)
                                property var fstg6: proxy.getProxyParameter(Msi.Parametr.FanSetTempCpu6Ec)
                                property bool isValid: fssg1.isValid && fssg2.isValid && fssg3.isValid && fssg4.isValid && fssg5.isValid && fssg6.isValid && fssg7.isValid &&
                                                       fstg1.isValid && fstg2.isValid && fstg3.isValid && fstg4.isValid && fstg5.isValid && fstg6.isValid

                                temp1: 0
                                temp2: isValid ? fstg1.value || 0 : 0
                                temp3: isValid ? fstg2.value || 0 : 0
                                temp4: isValid ? fstg3.value || 0 : 0
                                temp5: isValid ? fstg4.value || 0 : 0
                                temp6: isValid ? fstg5.value || 0 : 0
                                temp7: isValid ? fstg6.value || 0 : 0
                                speed1: isValid ? fssg1.value || 0 : 0
                                speed2: isValid ? fssg2.value || 0 : 0
                                speed3: isValid ? fssg3.value || 0 : 0
                                speed4: isValid ? fssg4.value || 0 : 0
                                speed5: isValid ? fssg5.value || 0 : 0
                                speed6: isValid ? fssg6.value || 0 : 0
                                speed7: isValid ? fssg7.value || 0 : 0
                                minTemp: fstg1.isValid && fstg1.availableValues ? fstg1.availableValues.min : 0
                                maxTemp: fstg1.isValid && fstg1.availableValues ? fstg1.availableValues.max : 100
                                minSpeed: fssg1.isValid && fssg1.availableValues ? fssg1.availableValues.min : 0
                                maxSpeed: fssg1.isValid && fssg1.availableValues ? fssg1.availableValues.max : 150

                                onPointEdited: function(pointIndex, temperature, speed) {
                                    if (!isValid)
                                        return
                                    writeFanCurvePoint(pointIndex,
                                                       temperature,
                                                       speed,
                                                       [fssg1, fssg2, fssg3, fssg4, fssg5, fssg6, fssg7],
                                                       [fstg1, fstg2, fstg3, fstg4, fstg5, fstg6])
                                }
                            }

                            FanCurveCard {
                                id: fanCurveGpu
                                Layout.fillWidth: true
                                Layout.preferredHeight: 270
                                title: qsTr("GPU fan curve")
                                subtitle: coolerBoostActive ? qsTr("Cooler Boost") :
                                                              (targetFanModeActive ? qsTr("Managed by service") :
                                                                 (manualFanCurveActive ? qsTr("Temperature to speed map") : qsTr("Managed by firmware"))
                                                              )
                                surfaceColor: mainWindow.theme.surface
                                borderColor: mainWindow.theme.border
                                textColor: mainWindow.theme.text
                                mutedTextColor: mainWindow.theme.muted
                                gridColor: mainWindow.theme.track
                                accentColor: mainWindow.theme.good
                                enabled: manualFanCurveActive
                                opacity: manualFanCurveActive ? 1.0 : 0.55
                                visible: isValid

                                property var fssg1: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedGpu1Ec)
                                property var fssg2: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedGpu2Ec)
                                property var fssg3: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedGpu3Ec)
                                property var fssg4: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedGpu4Ec)
                                property var fssg5: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedGpu5Ec)
                                property var fssg6: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedGpu6Ec)
                                property var fssg7: proxy.getProxyParameter(Msi.Parametr.FanSetSpeedGpu7Ec)
                                property var fstg1: proxy.getProxyParameter(Msi.Parametr.FanSetTempGpu1Ec)
                                property var fstg2: proxy.getProxyParameter(Msi.Parametr.FanSetTempGpu2Ec)
                                property var fstg3: proxy.getProxyParameter(Msi.Parametr.FanSetTempGpu3Ec)
                                property var fstg4: proxy.getProxyParameter(Msi.Parametr.FanSetTempGpu4Ec)
                                property var fstg5: proxy.getProxyParameter(Msi.Parametr.FanSetTempGpu5Ec)
                                property var fstg6: proxy.getProxyParameter(Msi.Parametr.FanSetTempGpu6Ec)
                                property bool isValid: fssg1.isValid && fssg2.isValid && fssg3.isValid && fssg4.isValid && fssg5.isValid && fssg6.isValid && fssg7.isValid &&
                                                       fstg1.isValid && fstg2.isValid && fstg3.isValid && fstg4.isValid && fstg5.isValid && fstg6.isValid

                                temp1: 0
                                temp2: isValid ? fstg1.value || 0 : 0
                                temp3: isValid ? fstg2.value || 0 : 0
                                temp4: isValid ? fstg3.value || 0 : 0
                                temp5: isValid ? fstg4.value || 0 : 0
                                temp6: isValid ? fstg5.value || 0 : 0
                                temp7: isValid ? fstg6.value || 0 : 0
                                speed1: isValid ? fssg1.value || 0 : 0
                                speed2: isValid ? fssg2.value || 0 : 0
                                speed3: isValid ? fssg3.value || 0 : 0
                                speed4: isValid ? fssg4.value || 0 : 0
                                speed5: isValid ? fssg5.value || 0 : 0
                                speed6: isValid ? fssg6.value || 0 : 0
                                speed7: isValid ? fssg7.value || 0 : 0
                                minTemp: fstg1.isValid && fstg1.availableValues ? fstg1.availableValues.min : 0
                                maxTemp: fstg1.isValid && fstg1.availableValues ? fstg1.availableValues.max : 100
                                minSpeed: fssg1.isValid && fssg1.availableValues ? fssg1.availableValues.min : 0
                                maxSpeed: fssg1.isValid && fssg1.availableValues ? fssg1.availableValues.max : 150

                                onPointEdited: function(pointIndex, temperature, speed) {
                                    if (!isValid)
                                        return
                                    writeFanCurvePoint(pointIndex,
                                                       temperature,
                                                       speed,
                                                       [fssg1, fssg2, fssg3, fssg4, fssg5, fssg6, fssg7],
                                                       [fstg1, fstg2, fstg3, fstg4, fstg5, fstg6])
                                }
                            }
                        }

                CpuPage {
                    id: cpuPage
                    Layout.fillWidth: true
                    Layout.preferredHeight: implicitHeight
                    embedded: true
                    proxy: proxy
                    surfaceColor: mainWindow.theme.surface
                    elevatedColor: mainWindow.theme.elevated
                    borderColor: mainWindow.theme.border
                    textColor: mainWindow.theme.text
                    mutedTextColor: mainWindow.theme.muted
                    accentColor: mainWindow.theme.accent
                    secondaryAccentColor: mainWindow.theme.accent2
                }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Device controls")
                            color: mainWindow.theme.text
                            font.pixelSize: 18
                            font.bold: true
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: width > 980 ? 3 : width > 660 ? 2 : 1
                            columnSpacing: 12
                            rowSpacing: 12
                            uniformCellWidths: true
                            uniformCellHeights: true

                            Repeater {
                                model: [
                                    {title: qsTr("USB Power Share"), subtitle: qsTr("Power USB while sleeping"), parameter: usbPowerParam},
                                    {title: qsTr("Webcam"), subtitle: qsTr("Camera power"), parameter: webcamParam},
                                    {title: qsTr("Webcam Block"), subtitle: qsTr("Hardware camera block"), parameter: webcamBlockParam},
                                    {title: qsTr("FN / Meta swap"), subtitle: qsTr("Keyboard layout"), parameter: fnSwapParam},
                                    {title: qsTr("Super Battery"), subtitle: qsTr("Power saving mode"), parameter: superBatteryParam},
                                    {title: qsTr("Mic mute"), subtitle: qsTr("Microphone state"), parameter: micMuteParam},
                                    {title: qsTr("Mute LED"), subtitle: qsTr("LED indicator"), parameter: muteLedParam},
                                    {title: qsTr("Keyboard mode"), subtitle: qsTr("Backlight mode"), parameter: keyboardModeParam}
                                ]

                                ToggleTile {
                                    Layout.fillWidth: true
                                    visible: modelData.parameter && modelData.parameter.isValid
                                    title: modelData.title
                                    subtitle: modelData.subtitle
                                    checked: binaryChecked(modelData.parameter)
                                    active: modelData.parameter && modelData.parameter.isValid
                                    surfaceColor: mainWindow.theme.surface
                                    borderColor: mainWindow.theme.border
                                    textColor: mainWindow.theme.text
                                    mutedTextColor: mainWindow.theme.muted
                                    accentColor: mainWindow.theme.accent
                                    onToggled: function(checked) {
                                        setBinaryParameter(modelData.parameter, checked)
                                    }
                                }
                            }
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            columns: width > 850 ? 2 : 1
                            columnSpacing: 16
                            rowSpacing: 16
                            uniformCellWidths: true
                            uniformCellHeights: true

                            OptionGroup {
                                Layout.fillWidth: true
                                title: qsTr("Keyboard backlight")
                                subtitle: qsTr("Brightness")
                                parameter: keyboardBacklightParam
                                enumName: "KeyboardBacklight"
                                accentColor: mainWindow.theme.accent2
                                surfaceColor: mainWindow.theme.surface
                                borderColor: mainWindow.theme.border
                                textColor: mainWindow.theme.text
                                mutedTextColor: mainWindow.theme.muted
                                visible: keyboardBacklightParam.isValid
                            }

                            AppCard {
                                Layout.fillWidth: true
                                title: qsTr("Battery threshold")
                                subtitle: qsTr("Charge limit")
                                visible: batteryThreshold.isValid
                                surfaceColor: mainWindow.theme.surface
                                borderColor: mainWindow.theme.border
                                textColor: mainWindow.theme.text
                                mutedTextColor: mainWindow.theme.muted

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 12

                                    Slider {
                                        id: batteryThresholdSlider
                                        Layout.fillWidth: true
                                        property int draftValue: batteryThreshold && batteryThreshold.isValid ? Number(batteryThreshold.value || 0) : 0
                                        from: 0
                                        to: 100
                                        stepSize: 1
                                        snapMode: Slider.SnapAlways
                                        value: draftValue
                                        onMoved: draftValue = Math.round(value)
                                        onPressedChanged: {
                                            if (!pressed && batteryThreshold && batteryThreshold.isValid)
                                                batteryThreshold.value = draftValue
                                        }

                                        Connections {
                                            target: batteryThreshold
                                            function onValueChanged() {
                                                if (!batteryThresholdSlider.pressed)
                                                    batteryThresholdSlider.draftValue = Number(batteryThreshold.value || 0)
                                            }
                                        }
                                    }

                                    Rectangle {
                                        Layout.preferredWidth: 72
                                        Layout.preferredHeight: 36
                                        radius: 8
                                        color: mainWindow.theme.elevated
                                        border.color: mainWindow.theme.border

                                        Label {
                                            anchors.centerIn: parent
                                            text: batteryThresholdSlider.draftValue + "%"
                                            color: mainWindow.theme.accent
                                            font.pixelSize: 14
                                            font.bold: true
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
